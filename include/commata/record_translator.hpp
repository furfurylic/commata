/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_794F5003_D1ED_48ED_9D52_59EDE1761698
#define COMMATA_GUARD_794F5003_D1ED_48ED_9D52_59EDE1761698

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "field_scanners.hpp"
#include "table_scanner.hpp"

#include "detail/tuple_transform.hpp"
#include "detail/typing_aid.hpp"
#include "detail/member_like_base.hpp"

namespace commata {

template <class T>
struct arithmetic_field_translator_factory
{
    using target_type = T;

    template <class Sink>
    arithmetic_field_translator<T, std::decay_t<Sink>> operator()(Sink&& sink)
        const
    {
        return arithmetic_field_translator<T, std::decay_t<Sink>>(
            std::forward<Sink>(sink));
    }
};

template <class T>
struct string_field_translator_factory
{
    using target_type = T;

    template <class Sink>
    string_field_translator<T, std::decay_t<Sink>>operator()(Sink&& sink) const
    {
        return string_field_translator<T, std::decay_t<Sink>>(
            std::forward<Sink>(sink));
    }
};

template <class T>
struct string_view_field_translator_factory
{
    using target_type = T;

    template <class Sink>
    string_view_field_translator<T, std::decay_t<Sink>>
        operator()(Sink&& sink) const
    {
        return string_view_field_translator<T, std::decay_t<Sink>>(
            std::forward<Sink>(sink));
    }
};

template <class T, class = void>
struct default_field_translator_factory;

template <class T>
struct default_field_translator_factory<T,
    std::enable_if_t<is_default_translatable_arithmetic_type_v<detail::unwrap_optional_t<T>>>>
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
    std::enable_if_t<detail::is_std_string_view_v<detail::unwrap_optional_t<T>>>>
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

    using target_type = T;

    basic_field_spec(std::basic_string<Ch, Tr, Allocator> text_field_name,
        FieldTranslatorFactory&& field_translator_factory
            = FieldTranslatorFactory()) :
        field_name(std::move(text_field_name)),
        factory(std::move(field_translator_factory))
    {}
};

template <class T, class Ch, class Tr, class Allocator,
    class FieldTranslatorFactory = default_field_translator_factory_t<T>>
basic_field_spec<Ch, Tr, Allocator, T, std::decay_t<FieldTranslatorFactory>>
    field_spec(std::basic_string<Ch, Tr, Allocator>&& text_field_name,
        FieldTranslatorFactory&& field_translator_factory
            = std::decay_t<FieldTranslatorFactory>())
{
    return basic_field_spec<Ch, Tr, Allocator, T,
        std::decay_t<FieldTranslatorFactory>>(
            std::basic_string<Ch, Tr, Allocator>(text_field_name),
            std::forward<FieldTranslatorFactory>(field_translator_factory));
}

template <class T, class Ch, class Tr,
    class FieldTranslatorFactory = default_field_translator_factory_t<T>>
basic_field_spec<Ch, Tr, std::allocator<Ch>, T,
        std::decay_t<FieldTranslatorFactory>>
    field_spec(std::basic_string_view<Ch, Tr> text_field_name,
        FieldTranslatorFactory&& field_translator_factory
            = std::decay_t<FieldTranslatorFactory>())
{
    return field_spec<T>(
        std::basic_string<Ch, Tr, std::allocator<Ch>>(text_field_name),
        std::forward<FieldTranslatorFactory>(field_translator_factory));
}

template <class T, class Ch,
    class FieldTranslatorFactory = default_field_translator_factory_t<T>>
basic_field_spec<Ch, std::char_traits<Ch>, std::allocator<Ch>, T,
        std::decay_t<FieldTranslatorFactory>>
    field_spec(const Ch* text_field_name,
        FieldTranslatorFactory&& field_translator_factory
            = std::decay_t<FieldTranslatorFactory>())
{
    return field_spec<T>(
        std::basic_string_view<Ch, std::char_traits<Ch>>(text_field_name),
        std::forward<FieldTranslatorFactory>(field_translator_factory));
}

namespace detail::record_xlate {

template <class FieldTranslatorFactory>
class field_scanner_setter :
    member_like_base<FieldTranslatorFactory>
{
    using target_type = typename FieldTranslatorFactory::target_type;

    std::optional<target_type>* field_value_;

public:
    explicit field_scanner_setter(FieldTranslatorFactory factory,
        std::optional<target_type>& field_value) :
        member_like_base<FieldTranslatorFactory>(std::move(factory)),
        field_value_(std::addressof(field_value))
    {}

    void operator()(std::size_t field_index, table_scanner& scanner)
    {
        scanner.set_field_scanner(field_index, this->get()(
            [field_value = field_value_](auto&& v) {
                using v_t = decltype(v);
                field_value->emplace(std::forward<v_t>(v));
            }));
    }
};

template <class... Ts>
class record_translator_header_field_scanner
{
    std::map<std::string,
             std::function<void (std::size_t, table_scanner&)>,
             std::less<>> m_;

public:
    template <class... FieldSpecRs>
    explicit record_translator_header_field_scanner(
        std::tuple<std::optional<Ts>...>& field_values,
        FieldSpecRs&&... specs) :
        record_translator_header_field_scanner(
            field_values,
            std::integral_constant<std::size_t, 0>(),
            std::forward<FieldSpecRs>(specs)...)
    {}

private:
    template <std::size_t I, class FieldSpecR, class... FieldSpecRs>
    record_translator_header_field_scanner(
        std::tuple<std::optional<Ts>...>& field_values,
        std::integral_constant<std::size_t, I>,
        FieldSpecR&& spec,
        FieldSpecRs&&... other_specs) :
        record_translator_header_field_scanner(
            field_values,
            std::integral_constant<std::size_t, I + 1>(),
            std::forward<FieldSpecRs>(other_specs)...)
    {
        // I really want to employ a fold expression, but the counter (here I)
        // is required here
        m_.emplace(
            forward_if<FieldSpecR>(spec.field_name),
            field_scanner_setter(
                forward_if<FieldSpecR>(spec.factory),
                std::get<I>(field_values)));
    }

    template <std::size_t I>
    explicit record_translator_header_field_scanner(
        std::tuple<std::optional<Ts>...>&,
        std::integral_constant<std::size_t, I>)
    {
        static_assert(I == sizeof...(Ts));
    }

public:
    bool operator()(
        std::size_t field_index,
        std::optional<std::pair<const char*, const char*>> field_value,
        table_scanner& scanner)
    {
        if (field_value) {
            const std::string_view field_name(field_value->first,
                                field_value->second - field_value->first);
            if (const auto i = m_.find(field_name); i != m_.cend()) {
                i->second(field_index, scanner);
                m_.erase(i); // makes duplicate fields ignored
            }
            return true;
        }
        for (const auto& e : m_) {
            // Make a field whose name did not appear in the header treated as
            // "skipped"
            e.second(static_cast<std::size_t>(-1), scanner);
        }
        return false;
    }
};

template <class F, class... FieldSpecs>
class record_translator_record_end_scanner
{
    static_assert(std::is_invocable_v<F, typename FieldSpecs::target_type...>);

    // TODO: std::optional<std:::optional<int>> is dull
    using to_t =
        std::tuple<std::optional<typename FieldSpecs::target_type>...>;
    using h_t = record_translator_header_field_scanner<
        typename FieldSpecs::target_type...>;

    F f_;
    std::unique_ptr<to_t> field_values_;

#ifdef NDEBUG
    h_t header_field_scanner_;
#else
    std::optional<h_t> header_field_scanner_;
#endif

public:
    template <class FR, class... FieldSpecRs>
    record_translator_record_end_scanner(FR&& f, FieldSpecRs&&... specs) :
        f_(std::forward<FR>(f)),
        field_values_(new to_t),
#ifdef NDEBUG
        header_field_scanner_(*field_values_,
            std::forward<FieldSpecRs>(specs)...)
#else
        header_field_scanner_(std::in_place, *field_values_,
            std::forward<FieldSpecRs>(specs)...)
#endif
    {}

    decltype(auto) header_field_scanner()
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
                [](auto& v) { return std::move(*v); },
                *field_values_));
    }
};

}

template <class FR, class... FieldSpecRs>
table_scanner make_record_translator(FR&& f, FieldSpecRs&&... specs)
{
    // "Provide a deduction guide for record_translator_record_end_scanner
    // and use it here" approach is repulsed by g++ 7.5.
    detail::record_xlate::record_translator_record_end_scanner<
            std::decay_t<FR>, std::decay_t<FieldSpecRs>...>
        s(std::forward<FR>(f), std::forward<FieldSpecRs>(specs)...);
    table_scanner scanner(s.header_field_scanner());
    scanner.set_record_end_scanner(std::move(s));
    return scanner;
}

}

#endif
