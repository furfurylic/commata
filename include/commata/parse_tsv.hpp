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
    void normal(Parser& parser, typename Parser::char_type* p, ...) const
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
    void normal(Parser& parser, typename Parser::char_type*& p,
        typename Parser::char_type* pe) const
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
    void normal(Parser& parser, typename Parser::char_type* p, ...) const
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
    void normal(Parser& parser, typename Parser::char_type* p, ...) const
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
    void normal(Parser& parser, typename Parser::char_type* p, ...) const
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
    char_type* p_;
    Handler f_;

    // [first, last) is the current field value
    char_type* first_;
    char_type* last_;

    std::size_t physical_line_index_;
    char_type* physical_line_or_buffer_begin_;
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
        first_(nullptr), last_(nullptr),
        physical_line_index_(parse_error::npos),
        physical_line_or_buffer_begin_(nullptr),
        physical_line_chars_passed_away_(0),
        in_(std::forward<InputR>(in)), buffer_(nullptr), buffer_last_(nullptr),
        s_(state::after_lf), record_started_(false), eof_reached_(false)
    {}

    parser(parser&& other) noexcept(
            std::is_nothrow_move_constructible_v<Handler>
         && std::is_nothrow_move_constructible_v<Input>) :
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
    {
        if constexpr (has_handle_exception_v<Handler>) {
            try {
                return invoke_impl();
            } catch (...) {
                f_.handle_exception();
                throw;
            }
        } else {
            return invoke_impl();
        }
    }

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4102)
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-label"
#endif
    bool invoke_impl()
    try {
        if constexpr (has_yield_location_v<Handler>) {
            switch (f_.yield_location()) {
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
        }

        do {
            {
                const auto [buffer_size, loaded_size] = arrange_buffer();
                p_ = buffer_;
                physical_line_or_buffer_begin_ = buffer_;
                buffer_last_ = buffer_ + loaded_size;
                f_.start_buffer(buffer_, buffer_ + buffer_size);
            }

            set_first_last();

            while (p_ < buffer_last_) {
                step([this](const auto& h) {
                    h.normal(*this, p_, buffer_last_);
                });

                if constexpr (has_yield_v<Handler>) {
                    if (f_.yield(1)) {
                        return true;
                    }
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
            if constexpr (has_yield_v<Handler>) {
                if (f_.yield(2)) {
                    return true;
                }
            }
yield_2:
            f_.release_buffer(buffer_);
            buffer_ = nullptr;
            physical_line_chars_passed_away_ +=
                p_ - physical_line_or_buffer_begin_;
        } while (!eof_reached_);

        if constexpr (has_yield_v<Handler>) {
            f_.yield(static_cast<std::size_t>(-1));
        }
yield_end:
        return true;
    } catch (text_error& e) {
        e.set_physical_position(
            physical_line_index_, get_physical_column_index());
        throw;
    } catch (const parse_aborted&) {
        return false;
    }
#ifdef _MSC_VER
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

public:
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
            using input_size_t = typename Input::size_type;
            using min_t = std::common_type_t<std::size_t, input_size_t>;
            constexpr auto x = std::numeric_limits<input_size_t>::max();
            const auto n = static_cast<std::size_t>(
                            std::min<min_t>(buffer_size - loaded_size, x));
            const auto length =
                static_cast<std::size_t>(in_(buffer_ + loaded_size, n));
            loaded_size += length;
            if (length < n) {
                eof_reached_ = true;
                break;
            }
        } while (buffer_size > loaded_size);

        return { buffer_size, loaded_size };
    }

private:
    // Makes p_ become the first char of the new line
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

    // Makes both of first_ and last_ point p_
    void set_first_last() noexcept
    {
        first_ = p_;
        last_ = p_;
    }

    // Makes last_ point the next char of p_
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

    void swap(tsv_source& other) noexcept(std::is_nothrow_swappable_v<base_t>)
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
auto make_tsv_source(Args&&... args)
    noexcept(std::is_nothrow_constructible_v<
        decltype(make_char_input(std::forward<Args>(args)...)), Args&&...>)
 -> tsv_source<decltype(make_char_input(std::forward<Args>(args)...))>
{
    return tsv_source<decltype(make_char_input(std::forward<Args>(args)...))>(
        make_char_input(std::forward<Args>(args)...));
}

template <class CharInput>
auto make_tsv_source(CharInput&& input)
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
    template <class... Args>
    static auto check(std::void_t<Args...>*) -> decltype(
        make_tsv_source(std::declval<Args>()...),
        std::true_type());

    template <class...>
    static auto check(...) -> std::false_type;
};

template <class... Args>
constexpr bool are_make_tsv_source_args_v =
    decltype(are_make_tsv_source_args_impl::check<Args...>(nullptr))();

template <class T>
constexpr bool is_tsv_source = false;

template <class CharInput>
constexpr bool is_tsv_source<tsv_source<CharInput>> = true;

}

template <class CharInput, class... OtherArgs>
bool parse_tsv(const tsv_source<CharInput>& src, OtherArgs&&... other_args)
{
    return src(std::forward<OtherArgs>(other_args)...)();
}

template <class CharInput, class... OtherArgs>
bool parse_tsv(tsv_source<CharInput>&& src, OtherArgs&&... other_args)
{
    return std::move(src)(std::forward<OtherArgs>(other_args)...)();
}

template <class Arg1, class Arg2, class... OtherArgs>
auto parse_tsv(Arg1&& arg1, Arg2&& arg2, OtherArgs&&... other_args)
 -> std::enable_if_t<
        !detail::tsv::is_tsv_source<std::decay_t<Arg1>>
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
