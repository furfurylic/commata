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

#include "detail/typing_aid.hpp"
#include "detail/member_like_base.hpp"

namespace commata {

namespace detail {

template <std::size_t I, class F, class... Tuples>
constexpr auto apply(F f, Tuples&&... t) {
    return f(std::get<I>(std::forward<Tuples>(t))...);
}

template <std::size_t... Is, class F, class... Tuples>
constexpr auto transform_impl(std::index_sequence<Is...>,
        [[maybe_unused]] F f, [[maybe_unused]] Tuples&&... ts) {
    return std::make_tuple(apply<Is>(f, std::forward<Tuples>(ts)...)...);
}

template <class F, class... Tuples>
constexpr auto transform(F f, Tuples&&... ts) {
    constexpr auto Min = std::min({std::tuple_size_v<std::decay_t<Tuples>>...});
    constexpr auto Max = std::max({std::tuple_size_v<std::decay_t<Tuples>>...});
    static_assert(Min == Max, "Inconsistent tuple sizes");
    return transform_impl(std::make_index_sequence<Min>(),
                          std::move(f), std::forward<Tuples>(ts)...);
}

}

template <class T>
struct arithmetic_field_translator_factory
{
    using target_type = T;

    template <class Sink>
    arithmetic_field_translator<T, std::decay_t<Sink>> operator()(Sink&& sink)
        const /* TODO: noexcept? */
    {
        return arithmetic_field_translator<T, std::decay_t<Sink>>(
            std::forward<Sink>(sink));
    }
};

template <class Ch, class Tr, class Allocator>
struct string_field_translator_factory
{
    using target_type = std::basic_string<Ch, Tr, Allocator>;

    template <class Sink>
    string_field_translator<std::decay_t<Sink>, Ch, Tr, Allocator>
        operator()(Sink&& sink) const /* TODO: noexcept? */
    {
        return string_field_translator<std::decay_t<Sink>, Ch, Tr, Allocator>(
            std::forward<Sink>(sink));
    }
};

template <class Ch, class Tr>
struct string_view_field_translator_factory
{
    using target_type = std::basic_string_view<Ch, Tr>;

    template <class Sink>
    string_view_field_translator<std::decay_t<Sink>, Ch, Tr>
        operator()(Sink&& sink) const /* TODO: noexcept? */
    {
        return string_view_field_translator<std::decay_t<Sink>, Ch, Tr>(
            std::forward<Sink>(sink));
    }
};

template <class T, class = void>
struct default_field_translator_factory;

template <class T>
struct default_field_translator_factory<T,
    std::enable_if_t<is_default_translatable_arithmetic_type_v<T>>>
{
    using type = arithmetic_field_translator_factory<T>;
};

template <class Ch, class Tr, class Allocator>
struct default_field_translator_factory<
    std::basic_string<Ch, Tr, Allocator>, void>
{
    using type = string_field_translator_factory<Ch, Tr, Allocator>;
};

template <class Ch, class Tr>
struct default_field_translator_factory<std::basic_string_view<Ch, Tr>, void>
{
    using type = string_view_field_translator_factory<Ch, Tr>;
};

template <class T>
using default_field_translator_factory_t =
    typename default_field_translator_factory<T>::type;

template <class T,
    class FieldTranslatorFactory = default_field_translator_factory_t<T>>
struct field_spec
{
    std::string field_name;
    FieldTranslatorFactory factory;

    using target_type = T;

    template <class String,
        class FieldTranslatorFactoryR = FieldTranslatorFactory,
        std::enable_if_t<std::is_convertible_v<String, std::string>>*
            = nullptr>
    explicit field_spec(String&& text_field_name,
        FieldTranslatorFactoryR&& field_translator_factory
            = FieldTranslatorFactory()) :
        field_name(std::forward<String>(text_field_name)),
        factory(
            std::forward<FieldTranslatorFactoryR>(field_translator_factory))
    {}
};

namespace detail::record_xlate {

template <class FieldTranslatorFactory>
class field_scanner_setter :
    detail::member_like_base<FieldTranslatorFactory>
{
    using target_type = typename FieldTranslatorFactory::target_type;

    std::optional<target_type>* field_value_;

public:
    explicit field_scanner_setter(FieldTranslatorFactory factory,
        std::optional<target_type>& field_value) :
        detail::member_like_base<FieldTranslatorFactory>(std::move(factory)),
        field_value_(std::addressof(field_value))
    {}

    void operator()(std::size_t field_index, table_scanner& scanner)
    {
        scanner.set_field_scanner(field_index, this->get()(
            [field_value = field_value_](auto&& v) {
                using v_t = decltype(v);
                static_assert(std::is_same_v<target_type, std::decay_t<v_t>>);
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
    explicit record_translator_header_field_scanner(
        std::tuple<std::optional<Ts>...>& field_values,
        std::integral_constant<std::size_t, I>,
        FieldSpecR&& spec,
        FieldSpecRs&&... specs) :
        record_translator_header_field_scanner(
            field_values,
            std::integral_constant<std::size_t, I + 1>(),
            std::forward<FieldSpecRs>(specs)...)
    {
        // I really want to employ a fold expression, but the counter (here I)
        // is required here
        m_.emplace(
            detail::forward_if<FieldSpecR>(spec.field_name),
            field_scanner_setter(
                detail::forward_if<FieldSpecR>(spec.factory),
                std::get<I>(field_values)));
    }

    template <std::size_t I>
    explicit record_translator_header_field_scanner(
        std::tuple<std::optional<Ts>...>&,
        std::integral_constant<std::size_t, I>)
    {}

public:
    bool operator()(
            std::size_t field_index,
            std::optional<std::pair<const char*, const char*>> field_value,
            table_scanner& scanner) {
        if (field_value) {
            const std::string_view field_name(field_value->first,
                                field_value->second - field_value->first);
            if (const auto i = m_.find(field_name); i != m_.cend()) {
                i->second(field_index, scanner);
                m_.erase(i);
            }
            return true;
        }
        // TODO: What if !m_.empty()?
        return false;
    }
};

template <class F, class... FieldSpecs>
class record_translator_record_end_scanner
{
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
    using record_type = std::tuple<typename FieldSpecs::target_type...>;

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
            detail::transform(
                [](auto& v) { return std::move(*v); },
                    // TODO: Whether to dereference or not should be decided
                    // with regard to whether f_ receives std::optional or not
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
