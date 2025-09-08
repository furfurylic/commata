/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_794F5003_D1ED_48ED_9D52_59EDE1761698
#define COMMATA_GUARD_794F5003_D1ED_48ED_9D52_59EDE1761698

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "field_scanners.hpp"
#include "table_scanner.hpp"

#include "detail/allocate_deallocate.hpp"
#include "detail/allocation_only_allocator.hpp"
#include "detail/full_ebo.hpp"
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
    using target_type = T;

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
    using target_type = T;

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
    using target_type = T;

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

template <class T, class = void>
struct default_field_translator_factory;

template <class T>
struct default_field_translator_factory<T,
    std::enable_if_t<is_default_translatable_arithmetic_type_v<
        detail::unwrap_optional_t<T>>>>
{
    using type = arithmetic_field_translator_factory<T>;
};

template <class T>
struct default_field_translator_factory<T,
    std::enable_if_t<detail::is_std_string_v<detail::unwrap_optional_t<T>>>>
{
    using type = string_field_translator_factory<T>;
};

template <class T>
struct default_field_translator_factory<T,
    std::enable_if_t<
        detail::is_std_string_view_v<detail::unwrap_optional_t<T>>>>
{
    using type = string_view_field_translator_factory<T>;
};

template <class T>
using default_field_translator_factory_t =
    typename default_field_translator_factory<T>::type;

template <class Ch, class Tr, class Allocator, class T,
    class FieldTranslatorFactory = default_field_translator_factory_t<T>>
struct basic_field_spec
{
    std::basic_string<Ch, Tr, Allocator> field_name;
    FieldTranslatorFactory factory; // TODO: Employ EBO

    using char_type = Ch;
    using traits_type = Tr;
    using allocator_type = Allocator;
    using target_type = T;

    basic_field_spec(std::basic_string<Ch, Tr, Allocator> text_field_name,
        FieldTranslatorFactory field_translator_factory
            = FieldTranslatorFactory()) :
        field_name(std::move(text_field_name)),
        factory(std::move(field_translator_factory))
    {}
};

template <class T, class Ch, class Tr, class Allocator,
    class FieldTranslatorFactory>
basic_field_spec<Ch, Tr, Allocator, T, std::decay_t<FieldTranslatorFactory>>
    field_spec(std::basic_string<Ch, Tr, Allocator> text_field_name,
        FieldTranslatorFactory&& field_translator_factory)
{
    return basic_field_spec<Ch, Tr, Allocator, T,
        std::decay_t<FieldTranslatorFactory>>(
            std::move(text_field_name),
            std::forward<FieldTranslatorFactory>(field_translator_factory));
}

template <class T, class Ch, class Tr, class Allocator>
basic_field_spec<Ch, Tr, Allocator, T>
    field_spec(std::basic_string<Ch, Tr, Allocator> text_field_name)
{
    return basic_field_spec<Ch, Tr, Allocator, T,
        default_field_translator_factory_t<T>>(
            std::move(text_field_name));
}

template <class T, class Ch, class Tr, class FieldTranslatorFactory>
basic_field_spec<Ch, Tr, std::allocator<Ch>, T>
    field_spec(std::basic_string_view<Ch, Tr> text_field_name,
        FieldTranslatorFactory&& field_translator_factory)
{
    return field_spec<T>(
        std::basic_string<Ch, Tr, std::allocator<Ch>>(text_field_name),
        std::forward<FieldTranslatorFactory>(field_translator_factory));
}

template <class T, class Ch, class Tr>
basic_field_spec<Ch, Tr, std::allocator<Ch>, T,
        default_field_translator_factory_t<T>>
    field_spec(std::basic_string_view<Ch, Tr> text_field_name)
{
    return field_spec<T>(
        std::basic_string<Ch, Tr, std::allocator<Ch>>(text_field_name),
        default_field_translator_factory_t<T>());
}

template <class T, class FieldTranslatorFactory, class Ch>
basic_field_spec<Ch, std::char_traits<Ch>, std::allocator<Ch>, T,
        FieldTranslatorFactory>
    field_spec(const Ch* text_field_name,
        FieldTranslatorFactory&& field_translator_factory)
{
    return field_spec<T>(
        std::basic_string_view<Ch, std::char_traits<Ch>>(text_field_name),
        std::forward<FieldTranslatorFactory>(field_translator_factory));
}

template <class T, class Ch>
basic_field_spec<Ch, std::char_traits<Ch>, std::allocator<Ch>, T,
        default_field_translator_factory_t<T>>
    field_spec(const Ch* text_field_name)
{
    return field_spec<T>(
        std::basic_string_view<Ch, std::char_traits<Ch>>(text_field_name),
        default_field_translator_factory_t<T>());
}

namespace detail::record_xlate {

template <class Ch, class Tr, class Allocator>
struct field_scanner_setter
{
    virtual ~field_scanner_setter() {}
    virtual void operator()(
        std::size_t field_index,
        basic_table_scanner<Ch, Tr, Allocator>& scanner) && = 0;
};

template <class FieldTranslatorFactory, class Ch, class Tr, class Allocator>
class COMMATA_FULL_EBO typed_field_scanner_setter :
    public field_scanner_setter<Ch, Tr, Allocator>,
    member_like_base<FieldTranslatorFactory>
{
public:
    using target_type = typename FieldTranslatorFactory::target_type;

private:
    std::optional<unwrap_optional_t<target_type>>* field_value_;

public:
    explicit typed_field_scanner_setter(FieldTranslatorFactory factory,
        std::optional<unwrap_optional_t<target_type>>& field_value) :
        member_like_base<FieldTranslatorFactory>(std::move(factory)),
        field_value_(std::addressof(field_value))
    {}

    void operator()(
        std::size_t field_index,
        basic_table_scanner<Ch, Tr, Allocator>& scanner) && override
    {
        scanner.set_field_scanner(field_index, std::move(this->get())(
            [field_value = field_value_](auto&& v) {
                using v_t = decltype(v);
                if constexpr (is_std_optional_v<target_type>) {
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
    using m_key_t = std::basic_string<Ch, Tr, Allocator>;
    using m_setter_t = field_scanner_setter<Ch, Tr, Allocator>;
    using m_mapped_t = typename std::allocator_traits<
        typename at_t::template rebind_alloc<m_setter_t>>::pointer;
    using m_value_t = std::pair<const m_key_t, m_mapped_t>;
    using m_value_a_t = allocation_only_allocator<
        typename at_t::template rebind_alloc<m_value_t>>;
    using m_t = std::map<m_key_t, m_mapped_t, std::less<>, m_value_a_t>;

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
            destroy_deallocate(e.second);
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
        using typed_setter_t = typed_field_scanner_setter<
            std::decay_t<decltype(spec.factory)>, Ch, Tr, Allocator>;
        m_mapped_t p = allocate_construct<typed_setter_t>(
            detail::forward_if<FieldSpecR>(spec.factory),
            std::get<I>(field_values).o);
        try {
            if constexpr (
                std::is_constructible_v<
                    m_key_t,
                    decltype(forward_if<FieldSpecR>(spec.field_name))>) {
                m_.emplace(forward_if<FieldSpecR>(spec.field_name), p);
            } else {
                m_.emplace(
                    m_key_t(spec.field_name.data(), spec.field_name.size(),
                        Allocator(m_.get_allocator().base())),
                    p);
            }
        } catch (...) {
            destroy_deallocate(p);
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

public:
    bool operator()(
        std::size_t field_index,
        std::optional<std::pair<const Ch*, const Ch*>> field_value,
        basic_table_scanner<Ch, Tr, Allocator>& scanner)
    {
        if (field_value) {
            const std::basic_string_view<Ch, Tr> field_name(field_value->first,
                                field_value->second - field_value->first);
            if (const auto i = m_.find(field_name); i != m_.cend()) {
                std::move(*i->second)(field_index, scanner);
                destroy_deallocate(i->second);
                m_.erase(i); // makes duplicate fields ignored
            }
            return true;
        }
        std::size_t j = static_cast<std::size_t>(-1) - (m_.size() - 1);
        for (const auto& e : m_) {
            // Make a field whose name did not appear in the header treated as
            // "skipped"
            std::move(*e.second)(j, scanner);
            ++j;
        }
        return false;
    }
};

template <class... FieldSpecs>
using record_translator_targets_t =
    std::tuple<optionalized_target<typename FieldSpecs::target_type>...>;

template <class Ch, class Tr, class Allocator, class F, class... FieldSpecs>
class record_translator_record_end_scanner :
    member_like_base<typename std::allocator_traits<Allocator>::template
        rebind_alloc<record_translator_targets_t<FieldSpecs...>>>
{
    static_assert(std::is_invocable_v<F, typename FieldSpecs::target_type...>);

    using to_t = record_translator_targets_t<FieldSpecs...>;
    using h_t = record_translator_header_field_scanner<Ch, Tr, Allocator,
        typename FieldSpecs::target_type...>;
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

template <class FieldSpec, class Allocator = void>
using scanner_t = basic_table_scanner<
    typename FieldSpec::char_type,
    typename FieldSpec::traits_type,
    std::conditional_t<std::is_void_v<Allocator>,
        typename FieldSpec::allocator_type,
        Allocator>>;

}

template <class Allocator, class FR, class... FieldSpecRs>
detail::record_xlate::scanner_t<
    std::decay_t<detail::first_t<FieldSpecRs...>>, Allocator>
make_record_translator(std::allocator_arg_t, const Allocator& alloc,
    FR&& f, FieldSpecRs&&... specs)
{
    static_assert(sizeof...(FieldSpecRs) > 0);
    using table_scanner_t = detail::record_xlate::scanner_t<
        std::decay_t<detail::first_t<FieldSpecRs...>>, Allocator>;
    using record_end_scanner_t =
        detail::record_xlate::record_translator_record_end_scanner<
            std::remove_const_t<typename table_scanner_t::char_type>,
            typename table_scanner_t::traits_type,
            Allocator,
            std::decay_t<FR>, std::decay_t<FieldSpecRs>...>;

    record_end_scanner_t s(std::allocator_arg, alloc,
        std::forward<FR>(f), std::forward<FieldSpecRs>(specs)...);
    table_scanner_t scanner(s.bleed_header_field_scanner());
    scanner.set_record_end_scanner(std::move(s));
    return scanner;
}

template <class FR, class... FieldSpecRs>
detail::record_xlate::scanner_t<std::decay_t<detail::first_t<FieldSpecRs...>>>
make_record_translator(FR&& f, FieldSpecRs&&... specs)
{
    auto a = std::get<0>(std::tie(specs...)).field_name.get_allocator();
    return make_record_translator(std::allocator_arg, a,
        std::forward<FR>(f), std::forward<FieldSpecRs>(specs)...);
}

}

#endif
