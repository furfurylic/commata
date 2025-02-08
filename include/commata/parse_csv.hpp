/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_4B6A00F6_E33D_4114_9F57_D2C2E984E809
#define COMMATA_GUARD_4B6A00F6_E33D_4114_9F57_D2C2E984E809

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

namespace detail::csv {

enum class state : std::int_fast8_t
{
    after_comma,
    in_value,
    right_of_open_quote,
    in_quoted_value,
    in_quoted_value_after_quote,
    in_quoted_value_after_cr,
    in_quoted_value_after_crs,
    in_quoted_value_after_lf,
    after_cr,
    after_crs,
    after_lf
};

template <state s>
struct parse_step
{};

template <>
struct parse_step<state::after_comma>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::buffer_char_t* p, ...) const
    {
        switch (*p) {
        case key_chars<typename Parser::char_type>::comma_c:
            parser.set_first_last();
            parser.finalize();
            break;
        case key_chars<typename Parser::char_type>::dquote_c:
            parser.change_state(state::right_of_open_quote);
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
        using namespace std::string_view_literals;
        while (p < pe) {
            switch (*p) {
            case key_chars<typename Parser::char_type>::comma_c:
                parser.finalize();
                parser.change_state(state::after_comma);
                return;
            case key_chars<typename Parser::char_type>::dquote_c:
                throw parse_error(
                    "A quotation mark found in an unquoted value"sv);
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
struct parse_step<state::right_of_open_quote>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::buffer_char_t* p, ...) const
    {
        parser.set_first_last();
        if (*p == key_chars<typename Parser::char_type>::dquote_c) {
            parser.change_state(state::in_quoted_value_after_quote);
        } else {
            parser.renew_last();
            parser.change_state(state::in_quoted_value);
        }
    }

    template <class Parser>
    void underflow(Parser& /*parser*/) const
    {}

    template <class Parser>
    void eof(Parser& /*parser*/) const
    {
        using namespace std::string_view_literals;
        throw parse_error("EOF reached with an open quoted value"sv);
    }
};

template <>
struct parse_step<state::in_quoted_value>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::buffer_char_t*& p,
        typename Parser::buffer_char_t* pe) const
    {
        while (p < pe) {
            switch (*p) {
            case key_chars<typename Parser::char_type>::dquote_c:
                parser.update();
                parser.set_first_last();
                parser.change_state(state::in_quoted_value_after_quote);
                return;
            case key_chars<typename Parser::char_type>::cr_c:
                parser.renew_last();
                parser.change_state(state::in_quoted_value_after_cr);
                return;
            case key_chars<typename Parser::char_type>::lf_c:
                parser.renew_last();
                parser.change_state(state::in_quoted_value_after_lf);
                return;
            default:
                parser.renew_last();
                break;
            }
            ++p;
        }
        --p;
    }

    template <class Parser>
    void underflow(Parser& parser) const
    {
        parser.update();
    }

    template <class Parser>
    void eof(Parser& /*parser*/) const
    {
        using namespace std::string_view_literals;
        throw parse_error("EOF reached with an open quoted value"sv);
    }
};

template <>
struct parse_step<state::in_quoted_value_after_quote>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::buffer_char_t* p, ...) const
    {
        using namespace std::string_view_literals;
        switch (*p) {
        case key_chars<typename Parser::char_type>::comma_c:
            parser.finalize();
            parser.change_state(state::after_comma);
            break;
        case key_chars<typename Parser::char_type>::dquote_c:
            parser.set_first_last();
            parser.renew_last();
            parser.change_state(state::in_quoted_value);
            break;
        case key_chars<typename Parser::char_type>::cr_c:
            parser.finalize();
            parser.end_record();
            parser.change_state(state::after_cr);
            break;
        case key_chars<typename Parser::char_type>::lf_c:
            parser.finalize();
            parser.end_record();
            parser.change_state(state::after_lf);
            break;
        default:
            throw parse_error(
                "An invalid character found after a closed quoted value"sv);
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
struct parse_step<state::in_quoted_value_after_cr>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::buffer_char_t* p, ...) const
    {
        switch (*p) {
        case key_chars<typename Parser::char_type>::dquote_c:
            parser.new_physical_line();
            parser.update();
            parser.set_first_last();
            parser.change_state(state::in_quoted_value_after_quote);
            break;
        case key_chars<typename Parser::char_type>::cr_c:
            parser.renew_last();
            parser.change_state(state::in_quoted_value_after_crs);
            break;
        case key_chars<typename Parser::char_type>::lf_c:
            parser.renew_last();
            parser.change_state(state::in_quoted_value_after_lf);
            break;
        default:
            parser.new_physical_line();
            parser.renew_last();
            parser.change_state(state::in_quoted_value);
            break;
        }
    }

    template <class Parser>
    void underflow(Parser& parser) const
    {
        parser.update();
    }

    template <class Parser>
    void eof(Parser& /*parser*/) const
    {
        using namespace std::string_view_literals;
        throw parse_error("EOF reached with an open quoted value"sv);
    }
};

template <>
struct parse_step<state::in_quoted_value_after_crs>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::buffer_char_t* p, ...) const
    {
        switch (*p) {
        case key_chars<typename Parser::char_type>::dquote_c:
            parser.new_physical_line();
            parser.update();
            parser.set_first_last();
            parser.change_state(state::in_quoted_value_after_quote);
            break;
        case key_chars<typename Parser::char_type>::cr_c:
            parser.renew_last();
            break;
        case key_chars<typename Parser::char_type>::lf_c:
            parser.renew_last();
            parser.change_state(state::in_quoted_value_after_lf);
            break;
        default:
            parser.new_physical_line();
            parser.renew_last();
            parser.change_state(state::in_quoted_value);
            break;
        }
    }

    template <class Parser>
    void underflow(Parser& parser) const
    {
        parser.update();
    }

    template <class Parser>
    void eof(Parser& /*parser*/) const
    {
        using namespace std::string_view_literals;
        throw parse_error("EOF reached with an open quoted value"sv);
    }
};

template <>
struct parse_step<state::in_quoted_value_after_lf>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::buffer_char_t* p, ...) const
    {
        switch (*p) {
        case key_chars<typename Parser::char_type>::dquote_c:
            parser.new_physical_line();
            parser.update();
            parser.set_first_last();
            parser.change_state(state::in_quoted_value_after_quote);
            break;
        case key_chars<typename Parser::char_type>::cr_c:
            parser.renew_last();
            parser.change_state(state::in_quoted_value_after_cr);
            break;
        case key_chars<typename Parser::char_type>::lf_c:
            parser.new_physical_line();
            parser.renew_last();
            break;
        default:
            parser.new_physical_line();
            parser.renew_last();
            parser.change_state(state::in_quoted_value);
            break;
        }
    }

    template <class Parser>
    void underflow(Parser& parser) const
    {
        parser.update();
    }

    template <class Parser>
    void eof(Parser& /*parser*/) const
    {
        using namespace std::string_view_literals;
        throw parse_error("EOF reached with an open quoted value"sv);
    }
};

template <>
struct parse_step<state::after_cr>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::buffer_char_t* p, ...) const
    {
        switch (*p) {
        case key_chars<typename Parser::char_type>::comma_c:
            parser.new_physical_line();
            parser.set_first_last();
            parser.finalize();
            parser.change_state(state::after_comma);
            break;
        case key_chars<typename Parser::char_type>::dquote_c:
            parser.new_physical_line();
            parser.force_start_record();
            parser.change_state(state::right_of_open_quote);
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
        case key_chars<typename Parser::char_type>::comma_c:
            parser.new_physical_line();
            parser.set_first_last();
            parser.finalize();
            parser.change_state(state::after_comma);
            break;
        case key_chars<typename Parser::char_type>::dquote_c:
            parser.new_physical_line();
            parser.force_start_record();
            parser.change_state(state::right_of_open_quote);
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
        case key_chars<typename Parser::char_type>::comma_c:
            parser.set_first_last();
            parser.finalize();
            parser.change_state(state::after_comma);
            break;
        case key_chars<typename Parser::char_type>::dquote_c:
            parser.force_start_record();
            parser.change_state(state::right_of_open_quote);
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
        case state::after_comma:
            f(parse_step<state::after_comma>());
            break;
        case state::in_value:
            f(parse_step<state::in_value>());
            break;
        case state::right_of_open_quote:
            f(parse_step<state::right_of_open_quote>());
            break;
        case state::in_quoted_value:
            f(parse_step<state::in_quoted_value>());
            break;
        case state::in_quoted_value_after_quote:
            f(parse_step<state::in_quoted_value_after_quote>());
            break;
        case state::in_quoted_value_after_cr:
            f(parse_step<state::in_quoted_value_after_cr>());
            break;
        case state::in_quoted_value_after_crs:
            f(parse_step<state::in_quoted_value_after_crs>());
            break;
        case state::in_quoted_value_after_lf:
            f(parse_step<state::in_quoted_value_after_lf>());
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

} // end detail::csv

template <class CharInput>
class csv_source : public detail::base_source<CharInput, detail::csv::parser>
{
    using base_t = detail::base_source<CharInput, detail::csv::parser>;

public:
    explicit csv_source(const CharInput& input) noexcept(
            std::is_nothrow_constructible_v<base_t, const CharInput&>) :
        base_t(input)
    {}

    explicit csv_source(CharInput&& input) noexcept(
            std::is_nothrow_constructible_v<base_t, CharInput&&>) :
        base_t(std::move(input))
    {}

    csv_source(const csv_source&) = default;
    csv_source(csv_source&&) = default;
    ~csv_source() = default;
    csv_source& operator=(const csv_source&) = default;
    csv_source& operator=(csv_source&&) = default;

    void swap(csv_source& other) noexcept(base_t::swap_noexcept)
    {
        base_t::swap(other);
    }
};

template <class CharInput>
auto swap(csv_source<CharInput>& left, csv_source<CharInput>& right)
    noexcept(noexcept(left.swap(right)))
 -> std::enable_if_t<std::is_swappable_v<CharInput>>
{
    left.swap(right);
}

template <class... Args>
[[nodiscard]] auto make_csv_source(Args&&... args)
    noexcept(std::is_nothrow_constructible_v<
        decltype(make_char_input(std::forward<Args>(args)...)), Args&&...>)
 -> csv_source<decltype(make_char_input(std::forward<Args>(args)...))>
{
    return csv_source<decltype(make_char_input(std::forward<Args>(args)...))>(
        make_char_input(std::forward<Args>(args)...));
}

template <class CharInput>
[[nodiscard]] auto make_csv_source(CharInput&& input)
    noexcept(std::is_nothrow_constructible_v<
        std::decay_t<CharInput>, CharInput&&>)
 -> std::enable_if_t<
        !detail::are_make_char_input_args_v<CharInput&&>
     && std::is_invocable_r_v<typename std::decay_t<CharInput>::size_type,
            std::decay_t<CharInput>&,
            typename std::decay_t<CharInput>::char_type*,
            typename std::decay_t<CharInput>::size_type>,
        csv_source<std::decay_t<CharInput>>>
{
    return csv_source<std::decay_t<CharInput>>(std::forward<CharInput>(input));
}

namespace detail::csv {

struct are_make_csv_source_args_impl
{
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
#endif
    template <class... Args>
    static auto check(std::void_t<Args...>*) -> decltype(
        make_csv_source(std::declval<Args>()...),
        std::true_type());
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

    template <class...>
    static auto check(...) -> std::false_type;
};

template <class... Args>
constexpr bool are_make_csv_source_args_v =
    decltype(are_make_csv_source_args_impl::check<Args...>(nullptr))();

template <class T>
constexpr bool is_csv_source_v = false;

template <class CharInput>
constexpr bool is_csv_source_v<csv_source<CharInput>> = true;

template <class T>
constexpr bool is_indirect_t_v = false;

template <>
constexpr bool is_indirect_t_v<indirect_t> = true;

}

template <class CharInput, class... OtherArgs>
bool parse_csv(const csv_source<CharInput>& src, OtherArgs&&... other_args)
{
    return static_cast<bool>(src(std::forward<OtherArgs>(other_args)...)());
}

template <class CharInput, class... OtherArgs>
bool parse_csv(csv_source<CharInput>&& src, OtherArgs&&... other_args)
{
    return static_cast<bool>(
        std::move(src)(std::forward<OtherArgs>(other_args)...)());
}

template <class Arg1, class Arg2, class... OtherArgs>
auto parse_csv(Arg1&& arg1, Arg2&& arg2, OtherArgs&&... other_args)
 -> std::enable_if_t<
        !detail::csv::is_csv_source_v<std::decay_t<Arg1>>
     && !detail::csv::is_indirect_t_v<std::decay_t<Arg1>>
     && (detail::csv::are_make_csv_source_args_v<Arg1&&>
      || detail::csv::are_make_csv_source_args_v<Arg1&&, Arg2&&>),
        bool>
{
    if constexpr (detail::csv::are_make_csv_source_args_v<Arg1&&, Arg2&&>) {
        return parse_csv(make_csv_source(std::forward<Arg1>(arg1),
                                         std::forward<Arg2>(arg2)),
                         std::forward<OtherArgs>(other_args)...);
    } else {
        return parse_csv(make_csv_source(std::forward<Arg1>(arg1)),
                         std::forward<Arg2>(arg2),
                         std::forward<OtherArgs>(other_args)...);
    }
}

}

#endif
