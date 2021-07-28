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
#include <functional>
#include <ios>
#include <istream>
#include <memory>
#include <streambuf>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "buffer_control.hpp"
#include "buffer_size.hpp"
#include "char_input.hpp"
#include "key_chars.hpp"
#include "member_like_base.hpp"
#include "parse_error.hpp"
#include "typing_aid.hpp"
#include "wrapper_handlers.hpp"

namespace commata {

namespace detail::csv {

template <class T>
bool do_yield([[maybe_unused]] T& f, [[maybe_unused]] std::size_t location)
{
    if constexpr (has_yield_v<T>) {
        return f.yield(location);
    } else {
        return false;
    }
}

template <class T>
std::size_t do_yield_location([[maybe_unused]] T& f)
{
    if constexpr (has_yield_location_v<T>) {
        return f.yield_location();
    } else {
        return 0;
    }
}

enum class state : std::int_fast8_t
{
    after_comma,
    in_value,
    right_of_open_quote,
    in_quoted_value,
    in_quoted_value_after_quote,
    after_cr,
    after_lf
};

template <state s>
struct parse_step
{};

template <>
struct parse_step<state::after_comma>
{
    template <class Parser>
    void normal(Parser& parser, const typename Parser::char_type* p, ...) const
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
    void normal(Parser& parser, const typename Parser::char_type*& p,
        const typename Parser::char_type* pe) const
    {
        while (p < pe) {
            switch (*p) {
            case key_chars<typename Parser::char_type>::comma_c:
                parser.finalize();
                parser.change_state(state::after_comma);
                return;
            case key_chars<typename Parser::char_type>::dquote_c:
                throw parse_error(
                    "A quotation mark found in a non-escaped value");
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
    void normal(Parser& parser, const typename Parser::char_type* p, ...) const
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
        throw parse_error("EOF reached with an open escaped value");
    }
};

template <>
struct parse_step<state::in_quoted_value>
{
    template <class Parser>
    void normal(Parser& parser, const typename Parser::char_type*& p,
        const typename Parser::char_type* pe) const
    {
        while (p < pe) {
            if (*p == key_chars<typename Parser::char_type>::dquote_c) {
                parser.update();
                parser.set_first_last();
                parser.change_state(state::in_quoted_value_after_quote);
                return;
            } else {
                parser.renew_last();
                ++p;
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
    void eof(Parser& /*parser*/) const
    {
        throw parse_error("EOF reached with an open escaped value");
    }
};

template <>
struct parse_step<state::in_quoted_value_after_quote>
{
    template <class Parser>
    void normal(Parser& parser, const typename Parser::char_type* p, ...) const
    {
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
                "An invalid character found after a closed escaped value");
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
struct parse_step<state::after_cr>
{
    template <class Parser>
    void normal(Parser& parser, const typename Parser::char_type* p, ...) const
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
            parser.empty_physical_line();
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
struct parse_step<state::after_lf>
{
    template <class Parser>
    void normal(Parser& parser, const typename Parser::char_type* p, ...) const
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
            parser.empty_physical_line();
            parser.change_state(state::after_cr);
            break;
        case key_chars<typename Parser::char_type>::lf_c:
            parser.new_physical_line();
            parser.empty_physical_line();
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

template <class Input, class Handler>
class parser
{
    static_assert(
        std::is_same_v<
            typename Input::char_type, typename Handler::char_type>,
            "Input::char_type and Handler::char_type are inconsistent; "
            "they shall be the same type");

    using char_type = typename Handler::char_type;
    using handler_type = Handler;

private:
    // Reading position
    const char_type* p_;
    Handler f_;

    // [first, last) is the current field value
    const char_type* first_;
    const char_type* last_;

    std::size_t physical_line_index_;
    const char_type* physical_line_or_buffer_begin_;
    // Number of chars of this line before physical_line_or_buffer_begin_
    std::size_t physical_line_chars_passed_away_;

    Input in_;
    char_type* buffer_;
    char_type* buffer_last_;

    state s_;
    bool record_started_;
    bool eof_reached_;

private:
    template <state>
    friend struct parse_step;

    // To make control flows clearer, we adopt exceptions. Sigh...
    struct parse_aborted
    {};

public:
    template <class InputR, class HandlerR,
        std::enable_if_t<
            std::is_constructible_v<Input, InputR&&>
         && std::is_constructible_v<Handler, HandlerR&&>>* = nullptr>
    explicit parser(InputR&& in, HandlerR&& f)
        noexcept(std::is_nothrow_constructible_v<Input, InputR&&>
              && std::is_nothrow_constructible_v<Handler, HandlerR&&>) :
        p_(nullptr), f_(std::forward<HandlerR>(f)),
        physical_line_index_(parse_error::npos),
        physical_line_or_buffer_begin_(nullptr),
        physical_line_chars_passed_away_(0),
        in_(std::forward<InputR>(in)), buffer_(nullptr),
        s_(state::after_lf), record_started_(false), eof_reached_(false)
    {}

    parser(parser&& other) :
        p_(other.p_), f_(std::move(other.f_)),
        first_(other.first_), last_(other.last_),
        physical_line_index_(other.physical_line_index_),
        physical_line_or_buffer_begin_(other.physical_line_or_buffer_begin_),
        physical_line_chars_passed_away_(
            other.physical_line_chars_passed_away_),
        in_(std::move(other.in_)),
        buffer_(std::exchange(other.buffer_, nullptr)),
        buffer_last_(other.buffer_last_),
        s_(other.s_), record_started_(other.record_started_),
        eof_reached_(other.eof_reached_)
    {}

    ~parser()
    {
        if (buffer_) {
            f_.release_buffer(buffer_);
        }
    }

    bool operator()()
    try {
        switch (do_yield_location(f_)) {
        case 0:
            break;
        case 1:
            goto yield_1;
        case 2:
            goto yield_2;
        case static_cast<std::size_t>(-1):
            goto yield_end;
        default:
            assert(!"Invalid yield location");
            break;
        }

        do {
            {
                const auto sizes = arrange_buffer();
                p_ = buffer_;
                physical_line_or_buffer_begin_ = buffer_;
                buffer_last_ = buffer_ + sizes.second;
                f_.start_buffer(buffer_, buffer_ + sizes.first);
            }

            set_first_last();

            while (p_ < buffer_last_) {
                step([this](const auto& h) {
                    h.normal(*this, p_, buffer_last_);
                });

                if (do_yield(f_, 1)) {
                    return true;
                }
yield_1:
                ++p_;
            }
            step([this](const auto& h) { h.underflow(*this); });
            if (eof_reached_) {
                set_first_last();
                step([this](const auto& h) { h.eof(*this); });
                if (record_started_) {
                    end_record();
                }
            }

            f_.end_buffer(buffer_last_);
            if (do_yield(f_, 2)) {
                return true;
            }
yield_2:
            f_.release_buffer(buffer_);
            buffer_ = nullptr;
            physical_line_chars_passed_away_ +=
                p_ - physical_line_or_buffer_begin_;
        } while (!eof_reached_);

        do_yield(f_, static_cast<std::size_t>(-1));
yield_end:
        return true;
    } catch (text_error& e) {
        e.set_physical_position(
            physical_line_index_, get_physical_column_index());
        throw;
    } catch (const parse_aborted&) {
        return false;
    }

    std::pair<std::size_t, std::size_t> get_physical_position() const noexcept
    {
        return { physical_line_index_, get_physical_column_index() };
    }

private:
    std::size_t get_physical_column_index() const noexcept
    {
        return (p_ - physical_line_or_buffer_begin_)
                    + physical_line_chars_passed_away_;
    }

    std::pair<std::size_t, std::size_t> arrange_buffer()
    {
        std::size_t buffer_size;
        std::tie(buffer_, buffer_size) = f_.get_buffer();   // throw
        if (buffer_size < 1) {
            throw std::out_of_range(
                "Specified buffer length is shorter than one");
        }

        std::size_t loaded_size = 0;
        do {
            using min_t =
                std::common_type_t<std::size_t, typename Input::size_type>;
            const auto length = static_cast<std::size_t>(
                in_(buffer_ + loaded_size,
                    static_cast<std::size_t>(
                        std::min<min_t>(
                            buffer_size - loaded_size,
                            std::numeric_limits<typename Input::size_type>::
                                max()))));
            if (length == 0) {
                eof_reached_ = true;
                break;
            }
            loaded_size += length;
        } while (buffer_size > loaded_size);

        return { buffer_size, loaded_size };
    }

private:
    void new_physical_line() noexcept
    {
        if (physical_line_index_ == parse_error::npos) {
            physical_line_index_ = 0;
        } else {
            ++physical_line_index_;
        }
        physical_line_or_buffer_begin_ = p_;
        physical_line_chars_passed_away_ = 0;
    }

    void change_state(state s) noexcept
    {
        s_ = s;
    }

    void set_first_last() noexcept
    {
        first_ = p_;
        last_ = p_;
    }

    void renew_last() noexcept
    {
        last_ = p_ + 1;
    }

    void update()
    {
        if (!record_started_) {
            do_or_abort([this] {
                return f_.start_record(first_);
            });
            record_started_ = true;
        }
        if (first_ < last_) {
            do_or_abort([this] {
                return f_.update(first_, last_);
            });
        }
    }

    void finalize()
    {
        if (!record_started_) {
            do_or_abort([this] {
                return f_.start_record(first_);
            });
            record_started_ = true;
        }
        do_or_abort([this] {
            return f_.finalize(first_, last_);
        });
    }

    void force_start_record()
    {
        do_or_abort([this] {
            return f_.start_record(p_);
        });
        record_started_ = true;
    }

    void end_record()
    {
        do_or_abort([this] {
            return f_.end_record(p_);
        });
        record_started_ = false;
    }

    void empty_physical_line()
    {
        assert(!record_started_);
        do_or_abort([this] {
            f_.empty_physical_line(p_);
        });
    }

private:
    template <class F>
    static void do_or_abort(F f)
    {
        if constexpr (std::is_void_v<decltype(f())>) {
            f();
        } else if (!f()) {
            throw parse_aborted();
        }
    }

    template <class F>
    void step(F f)
    {
        switch (s_) {
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
        case state::after_cr:
            f(parse_step<state::after_cr>());
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
class csv_source
{
    CharInput in_;

public:
    using char_type = typename CharInput::char_type;
    using traits_type = typename CharInput::traits_type;

    template <class... Args,
        std::enable_if_t<
            std::is_constructible_v<CharInput, Args&&...>
         && ((sizeof...(Args) != 1)
          || !std::is_base_of_v<
                csv_source,
                std::decay_t<detail::first_t<Args...>>>)>* = nullptr>
    explicit csv_source(Args&&... args) noexcept(
            std::is_nothrow_constructible_v<CharInput, Args&&...>) :
        in_(std::forward<Args>(args)...)
    {}

    csv_source(const csv_source&)  = default;
    csv_source(csv_source&&) = default;
    ~csv_source() = default;
    csv_source& operator=(const csv_source&) = default;
    csv_source& operator=(csv_source&&) = default;

private:
    template <class Handler>
    class without_allocator
    {
        using handler_t = std::decay_t<Handler>;

    public:
        static constexpr bool enabled =
            !detail::is_std_reference_wrapper_v<handler_t>
         && detail::is_with_buffer_control_v<handler_t>;

        using full_fledged_handler_t =
            std::conditional_t<
                detail::is_full_fledged_v<handler_t>,
                handler_t,
                detail::full_fledged_handler<
                    handler_t, detail::thru_buffer_control>>;

        using ret_t =
            detail::csv::parser<CharInput, full_fledged_handler_t>;

        template <class HandlerR, class CharInputR>
        static auto invoke(HandlerR&& handler, CharInputR&& in)
        {
            static_assert(std::is_same_v<handler_t, std::decay_t<HandlerR>>);
            static_assert(
                std::is_same_v<
                    typename std::decay_t<Handler>::char_type,
                    typename traits_type::char_type>,
                "std::decay_t<Handler>::char_type and traits_type::char_type "
                "are inconsistent; they shall be the same type");
            using specs_t = without_allocator<Handler>;
            return ret_t(
                    std::forward<CharInputR>(in),
                    typename specs_t::full_fledged_handler_t(
                        std::forward<Handler>(handler)));
        }
    };

    template <class Handler, class Allocator>
    class with_allocator
    {
        using handler_t = std::decay_t<Handler>;

        using buffer_engine_t = detail::default_buffer_control<Allocator>;

        using full_fledged_handler_t =
            detail::full_fledged_handler<handler_t, buffer_engine_t>;

    public:
        static constexpr bool enabled =
            !detail::is_std_reference_wrapper_v<handler_t>
         && detail::is_without_buffer_control_v<handler_t>;

        using ret_t = detail::csv::parser<CharInput, full_fledged_handler_t>;

        template <class HandlerR, class CharInputR>
        static auto invoke(HandlerR&& handler,
            std::size_t buffer_size, const Allocator& alloc, CharInputR&& in)
        {
            static_assert(
                std::is_same_v<
                    typename handler_t::char_type,
                    typename traits_type::char_type>,
                "std::decay_t<Handler>::char_type and traits_type::char_type "
                "are inconsistent; they shall be the same type");
            static_assert(
                std::is_same_v<
                    typename handler_t::char_type,
                    typename std::allocator_traits<Allocator>::value_type>,
                "std::decay_t<Handler>::char_type and "
                "std::allocator_traits<Allocator>::value_type are "
                "inconsistent; they shall be the same type");
            return ret_t(
                    std::forward<CharInputR>(in),
                    full_fledged_handler_t(
                        std::forward<HandlerR>(handler),
                        buffer_engine_t(buffer_size, alloc)));
        }
    };

public:
    template <class Handler, class Allocator = void>
    using parser_type = std::conditional_t<
            without_allocator<Handler>::enabled,
            typename without_allocator<Handler>::ret_t,
            typename with_allocator<
                Handler,
                std::conditional_t<
                    std::is_same_v<Allocator, void>,
                    std::allocator<char_type>,
                    Allocator>>::ret_t>;

    template <class Handler>
    auto operator()(Handler&& handler) const&
        noexcept(
            std::is_nothrow_constructible_v<std::decay_t<Handler>, Handler>
         && std::is_nothrow_copy_constructible_v<CharInput>)
     -> std::enable_if_t<without_allocator<Handler>::enabled,
            parser_type<std::decay_t<Handler>>>
    {
        return without_allocator<Handler>::invoke(
            std::forward<Handler>(handler), in_);
    }

    template <class Handler>
    auto operator()(Handler&& handler) &&
        noexcept(
            std::is_nothrow_constructible_v<std::decay_t<Handler>, Handler>
         && std::is_nothrow_move_constructible_v<CharInput>)
     -> std::enable_if_t<without_allocator<Handler>::enabled,
            parser_type<std::decay_t<Handler>>>
    {
        return without_allocator<Handler>::invoke(
            std::forward<Handler>(handler), std::move(in_));
    }

    template <class Handler, class Allocator = std::allocator<char_type>>
    auto operator()(Handler&& handler, std::size_t buffer_size = 0,
            const Allocator& alloc = Allocator()) const&
        noexcept(
            std::is_nothrow_constructible_v<std::decay_t<Handler>, Handler>
         && std::is_nothrow_copy_constructible_v<CharInput>)
     -> std::enable_if_t<
            with_allocator<Handler, Allocator>::enabled,
            parser_type<Handler, Allocator>>
    {
        return with_allocator<Handler, Allocator>::invoke(
            std::forward<Handler>(handler),
            buffer_size, std::move(alloc), in_);
    }

    template <class Handler, class Allocator = std::allocator<char_type>>
    auto operator()(Handler&& handler,
        std::size_t buffer_size = 0, const Allocator& alloc = Allocator()) &&
        noexcept(
            std::is_nothrow_constructible_v<std::decay_t<Handler>, Handler>
         && std::is_nothrow_move_constructible_v<CharInput>)
     -> std::enable_if_t<
            with_allocator<Handler, Allocator>::enabled,
            parser_type<Handler, Allocator>>
    {
        return with_allocator<Handler, Allocator>::invoke(
            std::forward<Handler>(handler),
            buffer_size, std::move(alloc), std::move(in_));
    }

    template <class Handler, class... Args>
    auto operator()(std::reference_wrapper<Handler> handler,
            Args&&... args) const&
        noexcept(std::is_nothrow_copy_constructible_v<CharInput>)
     -> decltype((*this)(reference_handler(
            handler.get()), std::forward<Args>(args)...))
    {
        return (*this)(wrap_ref(handler.get()), std::forward<Args>(args)...);
    }

    template <class Handler, class... Args>
    auto operator()(std::reference_wrapper<Handler> handler,
            Args&&... args) &&
        noexcept(std::is_nothrow_move_constructible_v<CharInput>)
     -> decltype(std::move(*this)(reference_handler(
            handler.get()), std::forward<Args>(args)...))
    {
        return std::move(*this)(wrap_ref(handler.get()),
                                std::forward<Args>(args)...);
    }

    void swap(csv_source& other)
        noexcept(std::is_nothrow_swappable_v<CharInput>)
    {
        using std::swap;
        swap(in_, other.in_);
    }
};

template <class CharInput>
void swap(csv_source<CharInput>& left, csv_source<CharInput>& right)
    noexcept(noexcept(left.swap(right)))
{
    left.swap(right);
}

template <class... Args>
auto make_csv_source(Args&&... args)
    noexcept(std::is_nothrow_constructible_v<
        decltype(make_char_input(std::forward<Args>(args)...))>)
 -> std::enable_if_t<
        std::is_constructible_v<
            csv_source<decltype(make_char_input(std::forward<Args>(args)...))>,
            Args&&...>,
        csv_source<decltype(make_char_input(std::forward<Args>(args)...))>>
{
    return csv_source<decltype(make_char_input(std::forward<Args>(args)...))>(
        std::forward<Args>(args)...);
}

namespace detail::csv {

struct are_make_csv_source_args1_impl
{
    template <class Arg1>
    static auto check(std::decay_t<Arg1>*) -> decltype(
        make_csv_source(std::declval<Arg1>()),
        std::true_type());

    template <class Arg1>
    static auto check(...) -> std::false_type;
};

template <class Arg1>
constexpr bool are_make_csv_source_args1_v =
    decltype(are_make_csv_source_args1_impl::check<Arg1>(nullptr))();

struct are_make_csv_source_args2_impl
{
    template <class Arg1, class Arg2>
    static auto check(std::decay_t<Arg1>*) -> decltype(
        make_csv_source(std::declval<Arg1>(), std::declval<Arg2>()),
        std::true_type());

    template <class Arg1, class Arg2>
    static auto check(...) -> std::false_type;
};

template <class Arg1, class Arg2>
constexpr bool are_make_csv_source_args2_v =
    decltype(are_make_csv_source_args2_impl::check<Arg1, Arg2>(nullptr))();

} // detail::csv

template <class Arg1, class Arg2, class... OtherArgs>
auto parse_csv(Arg1&& arg1, Arg2&& arg2, OtherArgs&&... other_args)
 -> std::enable_if_t<
        detail::csv::are_make_csv_source_args1_v<Arg1>
     || detail::csv::are_make_csv_source_args2_v<Arg1, Arg2>,
        bool>
{
    if constexpr (detail::csv::are_make_csv_source_args2_v<Arg1, Arg2>) {
        return make_csv_source(
                std::forward<Arg1>(arg1), std::forward<Arg2>(arg2))
            (std::forward<OtherArgs>(other_args)...)();
    } else {
        return make_csv_source(std::forward<Arg1>(arg1))
            (std::forward<Arg2>(arg2),
                std::forward<OtherArgs>(other_args)...)();
    }
}

}

#endif
