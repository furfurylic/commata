/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_7236D715_EBB1_4153_9220_B15A6810ACDB
#define COMMATA_GUARD_7236D715_EBB1_4153_9220_B15A6810ACDB

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>

#include "char_input.hpp"
#include "parse_error.hpp"
#include "wrapper_handlers.hpp"

#include "detail/base_parser.hpp"
#include "detail/base_source.hpp"
#include "detail/key_chars.hpp"
#include "detail/typing_aid.hpp"

namespace commata {

namespace detail::tsv {

enum class state : std::int_fast8_t
{
    after_tab,
    in_value,
    after_cr,
    after_crs,
    after_lf
};

template <state s>
struct parse_step
{};

template <>
struct parse_step<state::after_tab>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::buffer_char_t* p, ...) const
    {
        switch (*p) {
        case key_chars<typename Parser::char_type>::tab_c:
            parser.set_first_last();
            parser.finalize();
            break;
        case key_chars<typename Parser::char_type>::cr_c:
            parser.set_first_last();
            parser.finalize();
            parser.end_record();
            parser.change_state(state::after_cr);
            break;
        case key_chars<typename Parser::char_type>::lf_c:
            parser.set_first_last();
            parser.finalize();
            parser.end_record();
            parser.change_state(state::after_lf);
            break;
        default:
            parser.set_first_last();
            parser.renew_last();
            parser.change_state(state::in_value);
            break;
        }
    }

    template <class Parser>
    void underflow(Parser& /*parser*/) const
    {}

    template <class Parser>
    void eof(Parser& parser) const
    {
        parser.finalize();
    }
};

template <>
struct parse_step<state::in_value>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::buffer_char_t*& p,
        typename Parser::buffer_char_t* pe) const
    {
        while (p < pe) {
            switch (*p) {
            case key_chars<typename Parser::char_type>::tab_c:
                parser.finalize();
                parser.change_state(state::after_tab);
                return;
            case key_chars<typename Parser::char_type>::cr_c:
                parser.finalize();
                parser.end_record();
                parser.change_state(state::after_cr);
                return;
            case key_chars<typename Parser::char_type>::lf_c:
                parser.finalize();
                parser.end_record();
                parser.change_state(state::after_lf);
                return;
            default:
                parser.renew_last();
                ++p;
                break;
            }
        }
        --p;
    }

    template <class Parser>
    void underflow(Parser& parser) const
    {
        parser.update();
    }

    template <class Parser>
    void eof(Parser& parser) const
    {
        parser.finalize();
    }
};

template <>
struct parse_step<state::after_cr>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::buffer_char_t* p, ...) const
    {
        switch (*p) {
        case key_chars<typename Parser::char_type>::tab_c:
            parser.new_physical_line();
            parser.set_first_last();
            parser.finalize();
            parser.change_state(state::after_tab);
            break;
        case key_chars<typename Parser::char_type>::cr_c:
            parser.new_physical_line();
            parser.change_state(state::after_crs);
            break;
        case key_chars<typename Parser::char_type>::lf_c:
            parser.change_state(state::after_lf);
            break;
        default:
            parser.new_physical_line();
            parser.set_first_last();
            parser.renew_last();
            parser.change_state(state::in_value);
            break;
        }
    }

    template <class Parser>
    void underflow(Parser& /*parser*/) const
    {}

    template <class Parser>
    void eof(Parser& /*parser*/) const
    {}
};

template <>
struct parse_step<state::after_crs>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::buffer_char_t* p, ...) const
    {
        switch (*p) {
        case key_chars<typename Parser::char_type>::tab_c:
            parser.new_physical_line();
            parser.set_first_last();
            parser.finalize();
            parser.change_state(state::after_tab);
            break;
        case key_chars<typename Parser::char_type>::cr_c:
            break;
        case key_chars<typename Parser::char_type>::lf_c:
            parser.change_state(state::after_lf);
            break;
        default:
            parser.new_physical_line();
            parser.empty_physical_line();
            parser.set_first_last();
            parser.renew_last();
            parser.change_state(state::in_value);
            break;
        }
    }

    template <class Parser>
    void underflow(Parser& /*parser*/) const
    {}

    template <class Parser>
    void eof(Parser& /*parser*/) const
    {}
};

template <>
struct parse_step<state::after_lf>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::buffer_char_t* p, ...) const
    {
        parser.new_physical_line();
        switch (*p) {
        case key_chars<typename Parser::char_type>::tab_c:
            parser.set_first_last();
            parser.finalize();
            parser.change_state(state::after_tab);
            break;
        case key_chars<typename Parser::char_type>::cr_c:
            parser.empty_physical_line();
            parser.change_state(state::after_cr);
            break;
        case key_chars<typename Parser::char_type>::lf_c:
            parser.empty_physical_line();
            break;
        default:
            parser.set_first_last();
            parser.renew_last();
            parser.change_state(state::in_value);
            break;
        }
    }

    template <class Parser>
    void underflow(Parser& /*parser*/) const
    {}

    template <class Parser>
    void eof(Parser& /*parser*/) const
    {}
};

template <class Input, class Handler>
class parser :
    public detail::base_parser<Input, Handler, state, parser<Input, Handler>>
{
public:
    static constexpr state first_state = state::after_lf;

    using detail::base_parser<Input, Handler, state, parser<Input, Handler>>::
        base_parser;

    template <class F>
    static void step(state s, F f)
    {
        switch (s) {
        case state::after_tab:
            f(parse_step<state::after_tab>());
            break;
        case state::in_value:
            f(parse_step<state::in_value>());
            break;
        case state::after_cr:
            f(parse_step<state::after_cr>());
            break;
        case state::after_crs:
            f(parse_step<state::after_crs>());
            break;
        case state::after_lf:
            f(parse_step<state::after_lf>());
            break;
        default:
            assert(false);
            break;
        }
    }
};

} // end detail::tsv

template <class CharInput>
class tsv_source : public detail::base_source<CharInput, detail::tsv::parser>
{
    using base_t = detail::base_source<CharInput, detail::tsv::parser>;

public:
    explicit tsv_source(const CharInput& input) noexcept(
            std::is_nothrow_constructible_v<base_t, const CharInput&>) :
        base_t(input)
    {}

    explicit tsv_source(CharInput&& input) noexcept(
            std::is_nothrow_constructible_v<base_t, CharInput&&>) :
        base_t(std::move(input))
    {}

    tsv_source(const tsv_source&) = default;
    tsv_source(tsv_source&&) = default;
    ~tsv_source() = default;
    tsv_source& operator=(const tsv_source&) = default;
    tsv_source& operator=(tsv_source&&) = default;

    void swap(tsv_source& other) noexcept(base_t::swap_noexcept)
    {
        base_t::swap(other);
    }
};

template <class CharInput>
auto swap(tsv_source<CharInput>& left, tsv_source<CharInput>& right)
    noexcept(noexcept(left.swap(right)))
 -> std::enable_if_t<std::is_swappable_v<CharInput>>
{
    left.swap(right);
}

template <class... Args>
[[nodiscard]] auto make_tsv_source(Args&&... args)
    noexcept(std::is_nothrow_constructible_v<
        decltype(make_char_input(std::forward<Args>(args)...)), Args&&...>)
 -> tsv_source<decltype(make_char_input(std::forward<Args>(args)...))>
{
    return tsv_source<decltype(make_char_input(std::forward<Args>(args)...))>(
        make_char_input(std::forward<Args>(args)...));
}

template <class CharInput>
[[nodiscard]] auto make_tsv_source(CharInput&& input)
    noexcept(std::is_nothrow_constructible_v<
        std::decay_t<CharInput>, CharInput&&>)
 -> std::enable_if_t<
        !detail::are_make_char_input_args_v<CharInput&&>
     && std::is_invocable_r_v<typename std::decay_t<CharInput>::size_type,
            std::decay_t<CharInput>&,
            typename std::decay_t<CharInput>::char_type*,
            typename std::decay_t<CharInput>::size_type>,
        tsv_source<std::decay_t<CharInput>>>
{
    return tsv_source<std::decay_t<CharInput>>(std::forward<CharInput>(input));
}

namespace detail::tsv {

struct are_make_tsv_source_args_impl
{
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
#endif
    template <class... Args>
    static auto check(std::void_t<Args...>*) -> decltype(
        make_tsv_source(std::declval<Args>()...),
        std::true_type());
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

    template <class...>
    static auto check(...) -> std::false_type;
};

template <class... Args>
constexpr bool are_make_tsv_source_args_v =
    decltype(are_make_tsv_source_args_impl::check<Args...>(nullptr))();

template <class T>
struct is_tsv_source : std::false_type
{};

template <class CharInput>
struct is_tsv_source<tsv_source<CharInput>> : std::true_type
{};

template <class T>
constexpr bool is_tsv_source_v = is_tsv_source<T>::value;

template <class T>
struct is_indirect_t : std::false_type
{};

template <>
struct is_indirect_t<indirect_t> : std::true_type
{};

template <class T>
constexpr bool is_indirect_t_v = is_indirect_t<T>::value;

}

template <class CharInput, class... OtherArgs>
bool parse_tsv(const tsv_source<CharInput>& src, OtherArgs&&... other_args)
{
    return static_cast<bool>(src(std::forward<OtherArgs>(other_args)...)());
}

template <class CharInput, class... OtherArgs>
bool parse_tsv(tsv_source<CharInput>&& src, OtherArgs&&... other_args)
{
    return static_cast<bool>(
        std::move(src)(std::forward<OtherArgs>(other_args)...)());
}

template <class Arg1, class Arg2, class... OtherArgs>
auto parse_tsv(Arg1&& arg1, Arg2&& arg2, OtherArgs&&... other_args)
 -> std::enable_if_t<
        !detail::tsv::is_tsv_source_v<std::decay_t<Arg1>>
     && !detail::tsv::is_indirect_t_v<std::decay_t<Arg1>>
     && (detail::tsv::are_make_tsv_source_args_v<Arg1&&>
      || detail::tsv::are_make_tsv_source_args_v<Arg1&&, Arg2&&>),
        bool>
{
    if constexpr (detail::tsv::are_make_tsv_source_args_v<Arg1&&, Arg2&&>) {
        return parse_tsv(make_tsv_source(std::forward<Arg1>(arg1),
                                         std::forward<Arg2>(arg2)),
                         std::forward<OtherArgs>(other_args)...);
    } else {
        return parse_tsv(make_tsv_source(std::forward<Arg1>(arg1)),
                         std::forward<Arg2>(arg2),
                         std::forward<OtherArgs>(other_args)...);
    }
}

}

#endif
