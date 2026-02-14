/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_794F5003_D1ED_48ED_9D52_59EDE1761698
#define COMMATA_GUARD_794F5003_D1ED_48ED_9D52_59EDE1761698

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "field_scanners.hpp"
#include "table_scanner.hpp"

#include "detail/allocate_deallocate.hpp"
#include "detail/allocation_only_allocator.hpp"
#include "detail/full_ebo.hpp"
#include "detail/string_pred.hpp"
#include "detail/tuple_transform.hpp"
#include "detail/typing_aid.hpp"
#include "detail/member_like_base.hpp"

namespace commata {

template <class T,
    class SkippingHandler = std::conditional_t<detail::is_std_optional_v<T>,
        ignore_if_skipped, fail_if_skipped>,
    class ConversionErrorHandler =
        std::conditional_t<detail::is_std_optional_v<T>,
            ignore_if_conversion_failed, fail_if_conversion_failed>>
class COMMATA_FULL_EBO arithmetic_field_translator_factory :
    detail::member_like_base<SkippingHandler>,
    detail::member_like_base<ConversionErrorHandler>
{
public:
    using value_type = T;

    explicit arithmetic_field_translator_factory(
            SkippingHandler skipping_handler = SkippingHandler(),
            ConversionErrorHandler conversion_error_handler
                = ConversionErrorHandler()) :
        detail::member_like_base<SkippingHandler>(skipping_handler),
        detail::member_like_base<ConversionErrorHandler>(
            conversion_error_handler)
    {}

    template <class Sink>
    arithmetic_field_translator<T, std::decay_t<Sink>,
        SkippingHandler, ConversionErrorHandler> operator()(Sink&& sink) &&
    {
        return arithmetic_field_translator<T, std::decay_t<Sink>,
                SkippingHandler, ConversionErrorHandler>(
            std::forward<Sink>(sink),
            std::move(this->
                detail::member_like_base<SkippingHandler>::get()),
            std::move(this->
                detail::member_like_base<ConversionErrorHandler>::get()));
    }
};

template <class T,
    class SkippingHandler = std::conditional_t<detail::is_std_optional_v<T>,
        ignore_if_skipped, fail_if_skipped>>
class string_field_translator_factory :
    detail::member_like_base<SkippingHandler>
{
public:
    using value_type = T;

    explicit string_field_translator_factory(
            SkippingHandler skipping_handler = SkippingHandler()) :
        detail::member_like_base<SkippingHandler>(skipping_handler)
    {}

    template <class Sink>
    string_field_translator<T, std::decay_t<Sink>, SkippingHandler>
        operator()(Sink&& sink) &&
    {
        return string_field_translator<T, std::decay_t<Sink>, SkippingHandler>(
            std::forward<Sink>(sink), std::move(this->get()));
    }
};

template <class T,
    class SkippingHandler = std::conditional_t<detail::is_std_optional_v<T>,
        ignore_if_skipped, fail_if_skipped>>
class string_view_field_translator_factory :
    detail::member_like_base<SkippingHandler>
{
public:
    using value_type = T;

    explicit string_view_field_translator_factory(
            SkippingHandler skipping_handler = SkippingHandler()) :
        detail::member_like_base<SkippingHandler>(skipping_handler)
    {}

    template <class Sink>
    string_view_field_translator<T, std::decay_t<Sink>, SkippingHandler>
        operator()(Sink&& sink) &&
    {
        return string_view_field_translator<
            T, std::decay_t<Sink>, SkippingHandler>(
                std::forward<Sink>(sink), std::move(this->get()));
    }
};

namespace detail::record_xlate {

template <class T, class = void>
struct default_field_translator_factory_impl
{};

template <class T>
struct default_field_translator_factory_impl<T,
    std::enable_if_t<is_default_translatable_arithmetic_type_v<
        unwrap_optional_t<T>>>>
{
    using type = arithmetic_field_translator_factory<T>;
};

template <class T>
struct default_field_translator_factory_impl<T,
    std::enable_if_t<is_std_string_v<unwrap_optional_t<T>>>>
{
    using type = string_field_translator_factory<T>;
};

template <class T>
struct default_field_translator_factory_impl<T,
    std::enable_if_t<is_std_string_view_v<unwrap_optional_t<T>>>>
{
    using type = string_view_field_translator_factory<T>;
};

}

template <class T>
struct default_field_translator_factory :
    detail::record_xlate::default_field_translator_factory_impl<T>
{};

template <class T>
using default_field_translator_factory_t =
    typename default_field_translator_factory<T>::type;

template <class FieldNamePred, class FieldTranslatorFactory>
std::tuple<std::decay_t<FieldNamePred>, std::decay_t<FieldTranslatorFactory>>
    field_spec(FieldNamePred&& field_name_pred,
               FieldTranslatorFactory&& factory)
{
    return { std::forward<FieldNamePred>(field_name_pred),
             std::forward<FieldTranslatorFactory>(factory) };
}

template <class T, class FieldNamePred>
std::tuple<std::decay_t<FieldNamePred>, default_field_translator_factory_t<T>>
    field_spec(FieldNamePred&& field_name_pred)
{
    return { std::forward<FieldNamePred>(field_name_pred),
             default_field_translator_factory_t<T>() };
}

namespace detail::record_xlate {

template <class Ch, class Tr, class Allocator>
struct field_scanner_setter
{
    virtual ~field_scanner_setter() {}
    virtual bool matches(
        std::size_t field_index,
        std::basic_string_view<Ch, Tr> field_name) const = 0;
    virtual void set(
        std::size_t field_index,
        basic_table_scanner<Ch, Tr, Allocator>& scanner) && = 0;
};

template <class FieldNamePred, class FieldTranslatorFactory,
    class Ch, class Tr, class Allocator>
class COMMATA_FULL_EBO typed_field_scanner_setter :
    public field_scanner_setter<Ch, Tr, Allocator>,
    member_like_base<FieldNamePred>, member_like_base<FieldTranslatorFactory>
{
public:
    using value_type = typename FieldTranslatorFactory::value_type;

private:
    std::optional<unwrap_optional_t<value_type>>* field_value_;

public:
    explicit typed_field_scanner_setter(FieldNamePred field_name_pred,
        FieldTranslatorFactory factory,
        std::optional<unwrap_optional_t<value_type>>& field_value) :
        member_like_base<FieldNamePred>(std::move(field_name_pred)),
        member_like_base<FieldTranslatorFactory>(std::move(factory)),
        field_value_(std::addressof(field_value))
    {}

    bool matches(
        [[maybe_unused]] std::size_t field_index,
        std::basic_string_view<Ch, Tr> field_name) const override
    {
        return this->member_like_base<FieldNamePred>::get()(field_name);
    }

    void set(
        std::size_t field_index,
        basic_table_scanner<Ch, Tr, Allocator>& scanner) && override
    {
        scanner.set_field_scanner(field_index,
            std::move(this->member_like_base<FieldTranslatorFactory>::get())(
            [field_value = field_value_](auto&& v) {
                using v_t = decltype(v);
                if constexpr (is_std_optional_v<value_type>) {
                    *field_value = std::forward<v_t>(v);
                } else {
                    field_value->emplace(std::forward<v_t>(v));
                }
            }));
    }
};

template <class T>
struct optionalized_target
{
    std::optional<T> o;

    T&& unwrap() &&
    {
        return std::move(*o);
    }
};

template <class T>
struct optionalized_target<std::optional<T>>
{
    std::optional<T> o;

    std::optional<T>&& unwrap() &&
    {
        return std::move(o);
    }
};

template <class Ch, class Tr, class Allocator, class... Ts>
class record_translator_header_field_scanner
{
    using at_t = std::allocator_traits<Allocator>;
    using m_setter_t = field_scanner_setter<Ch, Tr, Allocator>;
    using m_value_t = typename allocation_only_allocator<
        typename at_t::template rebind_alloc<m_setter_t>>::pointer;
    using m_value_a_t = allocation_only_allocator<
        typename at_t::template rebind_alloc<m_value_t>>;
    using m_t = std::vector<m_value_t, m_value_a_t>;

    m_t m_;

public:
    template <class... FieldSpecRs>
    record_translator_header_field_scanner(
        std::allocator_arg_t, const Allocator& alloc,
        std::tuple<optionalized_target<Ts>...>& field_values,
        FieldSpecRs&&... specs) :
        record_translator_header_field_scanner(
            std::allocator_arg, alloc,
            field_values,
            std::integral_constant<std::size_t, 0>(),
            std::forward<FieldSpecRs>(specs)...)
    {}

    record_translator_header_field_scanner(
        record_translator_header_field_scanner&& other)
        noexcept(std::is_move_constructible_v<m_t>) :
        m_(std::move(other.m_))
    {
        other.m_.clear();
    }

    ~record_translator_header_field_scanner()
    {
        for (const auto& e : m_) {
            destroy_deallocate(e);
        }
    }

private:
    template <std::size_t I, class FieldSpecR, class... FieldSpecRs>
    record_translator_header_field_scanner(
        std::allocator_arg_t, const Allocator& alloc,
        std::tuple<optionalized_target<Ts>...>& field_values,
        std::integral_constant<std::size_t, I>,
        FieldSpecR&& spec,
        FieldSpecRs&&... other_specs) :
        record_translator_header_field_scanner(
            std::allocator_arg, alloc,
            field_values,
            std::integral_constant<std::size_t, I + 1>(),
            std::forward<FieldSpecRs>(other_specs)...)
    {
        // I really want to employ a fold expression, but the counter (here I)
        // is required here

        const auto setter = create_setter(
            std::forward<FieldSpecR>(spec), std::get<I>(field_values).o);
                                                    // throw
        try {
            m_.emplace_back(setter);                // throw
        } catch (...) {
            destroy_deallocate(setter);
            throw;
        }
    }

    template <std::size_t I>
    record_translator_header_field_scanner(
        std::allocator_arg_t, const Allocator& alloc,
        std::tuple<optionalized_target<Ts>...>&,
        std::integral_constant<std::size_t, I>) : m_(m_value_a_t(alloc))
    {
        static_assert(I == sizeof...(Ts));
        m_.reserve(I);
    }

    template <class T, class... Args>
    [[nodiscard]] auto allocate_construct(Args&&... args)
    {
        return allocate_construct_g<T>(
            m_.get_allocator().base(), std::forward<Args>(args)...);
    }

    template <class P>
    void destroy_deallocate(P p)
    {
        destroy_deallocate_g(m_.get_allocator().base(), p);
    }

    template <class FieldSpecR, class U>
    auto create_setter(FieldSpecR&& spec, std::optional<U>& o)
    {
        auto pred = make_string_pred<Ch, Tr>(std::move(std::get<0>(spec)));
        auto&& fac = std::move(std::get<1>(spec));
        using typed_t = typed_field_scanner_setter<
            decltype(pred), std::decay_t<decltype(fac)>, Ch, Tr, Allocator>;
        return allocate_construct<typed_t>(std::move(pred), std::move(fac), o);
            // throw
    }

public:
    bool operator()(
        std::size_t field_index,
        std::optional<std::pair<const Ch*, const Ch*>> field_value,
        basic_table_scanner<Ch, Tr, Allocator>& scanner)
    {
        if (field_value) {
            const std::basic_string_view<Ch, Tr> field_name(
                field_value->first,
                field_value->second - field_value->first);
            for (decltype(m_.size()) i = m_.size(); i > 0U; --i) {
                const auto ii = i - 1;
                auto& setter = *m_[ii];
                if (setter.matches(field_index, field_name)) {
                    std::move(setter).set(field_index, scanner);
                    destroy_deallocate(m_[ii]);
                    m_.erase(m_.begin() + (ii));
                        // makes duplicate fields ignored
                }
            }
            return true;
        }
        std::size_t j = static_cast<std::size_t>(-1) - (m_.size() - 1);
        for (const auto& e : m_) {
            // Make a field whose name did not appear in the header treated as
            // "skipped"
            std::move(*e).set(j, scanner);
            ++j;
        }
        return false;
    }
};

template <class... FieldSpecs>
using record_translator_targets_t =
    std::tuple<
        optionalized_target<
            typename std::tuple_element_t<1, FieldSpecs>::value_type>...>;

template <class Ch, class Tr, class Allocator, class F, class... FieldSpecs>
class record_translator_record_end_scanner :
    member_like_base<typename std::allocator_traits<Allocator>::template
        rebind_alloc<record_translator_targets_t<FieldSpecs...>>>
{
    static_assert(std::is_invocable_v<F,
        typename std::tuple_element_t<1, FieldSpecs>::value_type...>);

    using to_t = record_translator_targets_t<FieldSpecs...>;
    using h_t = record_translator_header_field_scanner<Ch, Tr, Allocator,
        typename std::tuple_element_t<1, FieldSpecs>::value_type...>;
    using a_t = typename std::allocator_traits<Allocator>::template
        rebind_alloc<to_t>;
    using at_t = std::allocator_traits<a_t>;

    F f_;
    typename at_t::pointer field_values_;

#ifdef NDEBUG
    h_t header_field_scanner_;
#else
    std::optional<h_t> header_field_scanner_;
#endif

public:
    template <class FR, class... FieldSpecRs>
    record_translator_record_end_scanner(
            std::allocator_arg_t, const Allocator& alloc,
            FR&& f, FieldSpecRs&&... specs) :
        member_like_base<a_t>(alloc),
        f_(std::forward<FR>(f)),
        field_values_(allocate_construct<to_t>()),
#ifdef NDEBUG
        header_field_scanner_(std::allocator_arg, alloc,
            *field_values_, std::forward<FieldSpecRs>(specs)...)
#else
        header_field_scanner_(std::in_place, std::allocator_arg, alloc,
            *field_values_, std::forward<FieldSpecRs>(specs)...)
#endif
    {}

    record_translator_record_end_scanner(
            record_translator_record_end_scanner&& other) :
        member_like_base<a_t>(std::move(other.get())),
        f_(std::move(other.f_)),
        field_values_(std::exchange(other.field_values_, nullptr))
    {}

    ~record_translator_record_end_scanner()
    {
        if (field_values_) {
            destroy_deallocate(field_values_);
        }
    }

    decltype(auto) bleed_header_field_scanner()
    {
#ifdef NDEBUG
        return std::move(header_field_scanner_);
#else
        assert(header_field_scanner_.has_value());
        return std::move(*header_field_scanner_);
#endif
    }

    void operator()()
    {
        std::apply(
            f_,
            transform(
                [](auto& v) { return std::move(v).unwrap(); },
                *field_values_));
    }

private:
    template <class T, class... Args>
    [[nodiscard]] auto allocate_construct(Args&&... args)
    {
        return allocate_construct_g<T>(
            this->get(), std::forward<Args>(args)...);
    }

    template <class P>
    void destroy_deallocate(P p)
    {
        destroy_deallocate_g(this->get(), p);
    }
};

}

template <class Ch, class Tr, class Allocator, class FR,
          class FieldSpecR0, class... FieldSpecRs>
basic_table_scanner<Ch, Tr, Allocator> make_basic_record_translator(
    std::allocator_arg_t, const Allocator& alloc,
    FR&& f, FieldSpecR0&& spec0, FieldSpecRs&&... specs)
{
    using table_scanner_t = basic_table_scanner<Ch, Tr, Allocator>;
    using record_end_scanner_t =
        detail::record_xlate::record_translator_record_end_scanner<
            Ch, Tr,
            Allocator,
            std::decay_t<FR>,
            std::decay_t<FieldSpecR0>, std::decay_t<FieldSpecRs>...>;

    record_end_scanner_t s(std::allocator_arg, alloc,
        std::forward<FR>(f),
        std::forward<FieldSpecR0>(spec0),
        std::forward<FieldSpecRs>(specs)...);
    table_scanner_t scanner(s.bleed_header_field_scanner());
    scanner.set_record_end_scanner(std::move(s));
    return scanner;
}

template <class Ch, class Tr, class FR,
          class FieldSpecR0, class... FieldSpecRs>
auto make_basic_record_translator(
    FR&& f, FieldSpecR0&& spec0, FieldSpecRs&&... specs)
 -> std::enable_if_t<
        !std::is_base_of_v<std::allocator_arg_t, std::decay_t<FR>>,
        basic_table_scanner<Ch, Tr, std::allocator<Ch>>>
{
    return make_basic_record_translator<Ch, Tr>(
        std::allocator_arg, std::allocator<Ch>(),
        std::forward<FR>(f),
        std::forward<FieldSpecR0>(spec0),
        std::forward<FieldSpecRs>(specs)...);
}

template <class FR, class FieldSpecR0, class... FieldSpecRs>
table_scanner make_record_translator(
    FR&& f, FieldSpecR0&& spec0, FieldSpecRs&&... specs)
{
    return make_basic_record_translator<char, std::char_traits<char>>(
        std::forward<FR>(f),
        std::forward<FieldSpecR0>(spec0),
        std::forward<FieldSpecRs>(specs)...);
}

template <class FR, class FieldSpecR0, class... FieldSpecRs>
wtable_scanner make_wrecord_translator(
    FR&& f, FieldSpecR0&& spec0, FieldSpecRs&&... specs)
{
    return make_basic_record_translator<wchar_t, std::char_traits<wchar_t>>(
        std::forward<FR>(f),
        std::forward<FieldSpecR0>(spec0),
        std::forward<FieldSpecRs>(specs)...);
}

}

#endif
