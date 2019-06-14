/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_4B6A00F6_E33D_4114_9F57_D2C2E984E809
#define COMMATA_GUARD_4B6A00F6_E33D_4114_9F57_D2C2E984E809

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ios>
#include <istream>
#include <memory>
#include <string>
#include <streambuf>
#include <utility>

#include "handler_decorator.hpp"
#include "key_chars.hpp"
#include "member_like_base.hpp"
#include "parse_error.hpp"
#include "typing_aid.hpp"

namespace commata {
namespace detail {

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
    void normal(Parser& parser, typename Parser::char_type c) const
    {
        switch (c) {
        case key_chars<typename Parser::char_type>::COMMA:
            parser.set_first_last();
            parser.finalize();
            break;
        case key_chars<typename Parser::char_type>::DQUOTE:
            parser.change_state(state::right_of_open_quote);
            break;
        case key_chars<typename Parser::char_type>::CR:
            parser.set_first_last();
            parser.finalize();
            parser.end_record();
            parser.change_state(state::after_cr);
            break;
        case key_chars<typename Parser::char_type>::LF:
            parser.set_first_last();
            parser.finalize();
            parser.end_record();
            parser.change_state(state::after_lf);
            break;
        default:
            parser.set_first_last();
            parser.update_last();
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
    void normal(Parser& parser, typename Parser::char_type c) const
    {
        switch (c) {
        case key_chars<typename Parser::char_type>::COMMA:
            parser.finalize();
            parser.change_state(state::after_comma);
            break;
        case key_chars<typename Parser::char_type>::DQUOTE:
            throw parse_error(
                "A quotation mark found in a non-escaped value");
        case key_chars<typename Parser::char_type>::CR:
            parser.finalize();
            parser.end_record();
            parser.change_state(state::after_cr);
            break;
        case key_chars<typename Parser::char_type>::LF:
            parser.finalize();
            parser.end_record();
            parser.change_state(state::after_lf);
            break;
        default:
            parser.update_last();
            break;
        }
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
    void normal(Parser& parser, typename Parser::char_type c) const
    {
        parser.set_first_last();
        if (c == key_chars<typename Parser::char_type>::DQUOTE) {
            parser.change_state(state::in_quoted_value_after_quote);
        } else {
            parser.update_last();
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
    void normal(Parser& parser, typename Parser::char_type c) const
    {
        if (c == key_chars<typename Parser::char_type>::DQUOTE) {
            parser.update();
            parser.set_first_last();
            parser.change_state(state::in_quoted_value_after_quote);
        } else {
            parser.update_last();
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
        throw parse_error("EOF reached with an open escaped value");
    }
};

template <>
struct parse_step<state::in_quoted_value_after_quote>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::char_type c) const
    {
        switch (c) {
        case key_chars<typename Parser::char_type>::COMMA:
            parser.finalize();
            parser.change_state(state::after_comma);
            break;
        case key_chars<typename Parser::char_type>::DQUOTE:
            parser.set_first_last();
            parser.update_last();
            parser.change_state(state::in_quoted_value);
            break;
        case key_chars<typename Parser::char_type>::CR:
            parser.finalize();
            parser.end_record();
            parser.change_state(state::after_cr);
            break;
        case key_chars<typename Parser::char_type>::LF:
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
    void normal(Parser& parser, typename Parser::char_type c) const
    {
        switch (c) {
        case key_chars<typename Parser::char_type>::COMMA:
            parser.new_physical_line();
            parser.set_first_last();
            parser.finalize();
            parser.change_state(state::after_comma);
            break;
        case key_chars<typename Parser::char_type>::DQUOTE:
            parser.new_physical_line();
            parser.force_start_record();
            parser.change_state(state::right_of_open_quote);
            break;
        case key_chars<typename Parser::char_type>::CR:
            parser.new_physical_line();
            parser.empty_physical_line();
            break;
        case key_chars<typename Parser::char_type>::LF:
            parser.change_state(state::after_lf);
            break;
        default:
            parser.new_physical_line();
            parser.set_first_last();
            parser.update_last();
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
    void normal(Parser& parser, typename Parser::char_type c) const
    {
        switch (c) {
        case key_chars<typename Parser::char_type>::COMMA:
            parser.new_physical_line();
            parser.set_first_last();
            parser.finalize();
            parser.change_state(state::after_comma);
            break;
        case key_chars<typename Parser::char_type>::DQUOTE:
            parser.new_physical_line();
            parser.force_start_record();
            parser.change_state(state::right_of_open_quote);
            break;
        case key_chars<typename Parser::char_type>::CR:
            parser.new_physical_line();
            parser.empty_physical_line();
            parser.change_state(state::after_cr);
            break;
        case key_chars<typename Parser::char_type>::LF:
            parser.new_physical_line();
            parser.empty_physical_line();
            break;
        default:
            parser.new_physical_line();
            parser.set_first_last();
            parser.update_last();
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

template <class T>
struct with_buffer_control :
    std::integral_constant<bool,
        has_get_buffer<T>::value && has_release_buffer<T>::value>
{};

template <class T>
struct without_buffer_control :
    std::integral_constant<bool,
        (!has_get_buffer<T>::value) && (!has_release_buffer<T>::value)>
{};

template <class Allocator>
class default_buffer_control :
    detail::member_like_base<Allocator>
{
    using alloc_traits_t = std::allocator_traits<Allocator>;

    std::size_t buffer_size_;
    typename alloc_traits_t::pointer buffer_;

public:
    default_buffer_control(
        std::size_t buffer_size, const Allocator& alloc) noexcept :
        detail::member_like_base<Allocator>(alloc),
        buffer_size_((buffer_size < 1) ? 8192 : buffer_size),
        buffer_()
    {}

    default_buffer_control(default_buffer_control&& other) noexcept :
        detail::member_like_base<Allocator>(other.get()),
        buffer_size_(other.buffer_size_),
        buffer_(other.buffer_)
    {
        other.buffer_ = nullptr;
    }

    ~default_buffer_control()
    {
        if (buffer_) {
            alloc_traits_t::deallocate(this->get(), buffer_, buffer_size_);
        }
    }

    std::pair<typename alloc_traits_t::value_type*, std::size_t>
        do_get_buffer(...)
    {
        if (!buffer_) {
            buffer_ = alloc_traits_t::allocate(
                this->get(), buffer_size_);     // throw
        }
        return std::make_pair(std::addressof(*buffer_), buffer_size_);
    }

    void do_release_buffer(...) noexcept
    {}
};

struct thru_buffer_control
{
    template <class Handler>
    std::pair<typename Handler::char_type*, std::size_t> do_get_buffer(
        Handler* f)
    {
        return f->get_buffer(); // throw
    }

    template <class Handler>
    void do_release_buffer(
        const typename Handler::char_type* buffer, Handler* f) noexcept
    {
        return f->release_buffer(buffer);
    }
};

template <class Handler>
struct is_full_fledged :
    std::integral_constant<bool,
        with_buffer_control<Handler>::value
     && has_start_buffer<Handler>::value && has_end_buffer<Handler>::value
     && has_empty_physical_line<Handler>::value>
{};

template <class Handler, class BufferControl>
class full_fledged_handler :
    BufferControl
{
    static_assert(!std::is_reference<Handler>::value, "");
    static_assert(!is_full_fledged<Handler>::value, "");

    Handler handler_;

public:
    using char_type = typename Handler::char_type;

    full_fledged_handler(Handler&& handler, BufferControl&& buffer_engine)
        noexcept(std::is_nothrow_move_constructible<Handler>::value) :
        BufferControl(std::move(buffer_engine)),
        handler_(std::move(handler))
    {}

    std::pair<char_type*, std::size_t> get_buffer()
    {
        return this->do_get_buffer(std::addressof(handler_));
    }

    void release_buffer(const char_type* buffer) noexcept
    {
        this->do_release_buffer(buffer, std::addressof(handler_));
    }

    void start_buffer(
        const char_type* buffer_begin, const char_type* buffer_end)
    {
        start_buffer(has_start_buffer<Handler>(), buffer_begin, buffer_end);
    }

    void end_buffer(const char_type* buffer_end)
    {
        end_buffer(has_end_buffer<Handler>(), buffer_end);
    }

    auto start_record(const char_type* record_begin)
    {
        return handler_.start_record(record_begin);
    }

    auto update(const char_type* first, const char_type* last)
    {
        return handler_.update(first, last);
    }

    auto finalize(const char_type* first, const char_type* last)
    {
        return handler_.finalize(first, last);
    }

    auto end_record(const char_type* end)
    {
        return handler_.end_record(end);
    }

    auto empty_physical_line(const char_type* where)
    {
        return empty_physical_line(has_empty_physical_line<Handler>(), where);
    }

private:
    void start_buffer(std::true_type,
        const char_type* buffer_begin, const char_type* buffer_end)
    {
        handler_.start_buffer(buffer_begin, buffer_end);
    }

    void start_buffer(std::false_type, ...)
    {}

    void end_buffer(std::true_type, const char_type* buffer_end)
    {
        handler_.end_buffer(buffer_end);
    }

    void end_buffer(std::false_type, ...)
    {}

    auto empty_physical_line(std::true_type, const char_type* where)
    {
        return handler_.empty_physical_line(where);
    }

    auto empty_physical_line(std::false_type, ...)
    {
        return true;
    }
};

template <class Handler,
    std::enable_if_t<!is_full_fledged<Handler>::value, std::nullptr_t>
        = nullptr>
auto make_full_fledged(Handler&& handler)
    noexcept(std::is_nothrow_move_constructible<Handler>::value)
{
    static_assert(!std::is_reference<Handler>::value, "");
    static_assert(with_buffer_control<Handler>::value, "");
    return full_fledged_handler<Handler, thru_buffer_control>(
        std::forward<Handler>(handler), thru_buffer_control());
}

template <class Handler,
    std::enable_if_t<is_full_fledged<Handler>::value, std::nullptr_t>
        = nullptr>
decltype(auto) make_full_fledged(Handler&& handler) noexcept
{
    static_assert(!std::is_reference<Handler>::value, "");
    return std::forward<Handler>(handler);
}

template <class Handler, class Allocator>
auto make_full_fledged(Handler&& handler,
    std::size_t buffer_size, const Allocator& alloc)
    noexcept(std::is_nothrow_move_constructible<Handler>::value)
{
    static_assert(!std::is_reference<Handler>::value, "");
    static_assert(without_buffer_control<Handler>::value, "");
    static_assert(std::is_same<
        typename Handler::char_type,
        typename std::allocator_traits<Allocator>::value_type>::value, "");
    return full_fledged_handler<Handler, default_buffer_control<Allocator>>(
        std::forward<Handler>(handler),
        default_buffer_control<Allocator>(buffer_size, alloc));
}

template <class Handler>
class primitive_parser
{
    using char_type = typename Handler::char_type;

private:
    // Reading position
    const char_type* p_;
    Handler f_;

    bool record_started_;
    state s_;
    // [first, last) is the current field value
    const char_type* first_;
    const char_type* last_;

    std::size_t physical_line_index_;
    const char_type* physical_line_or_buffer_begin_;
    // Number of chars of this line before physical_line_or_buffer_begin_
    std::size_t physical_line_chars_passed_away_;

private:
    template <state>
    friend struct parse_step;

    // To make control flows clearer, we adopt exceptions. Sigh...
    struct parse_aborted
    {};

public:
    explicit primitive_parser(Handler&& f)
        noexcept(std::is_nothrow_move_constructible<Handler>::value) :
        f_(std::move(f)),
        record_started_(false), s_(state::after_lf),
        physical_line_index_(parse_error::npos),
        physical_line_chars_passed_away_(0)
    {}

    primitive_parser(primitive_parser&&) = default;

    template <class Tr>
    bool parse_csv(std::basic_streambuf<char_type, Tr>* in)
    {
        auto release = [f = std::addressof(f_)](const char_type* buffer) {
            f->release_buffer(buffer);
        };

        bool eof_reached = false;
        do {
            std::unique_ptr<char_type, decltype(release)> buffer(
                nullptr, release);
            std::size_t buffer_size;
            {
                auto allocated = f_.get_buffer();   // throw
                buffer.reset(allocated.first);
                buffer_size = allocated.second;
                if (buffer_size < 1) {
                    throw std::out_of_range(
                        "Specified buffer length is shorter than one");
                }
            }

            std::streamsize loaded_size = 0;
            do {
                const auto length = in->sgetn(buffer.get() + loaded_size,
                    buffer_size - loaded_size);
                if (length == 0) {
                    eof_reached = true;
                    break;
                }
                loaded_size += length;
            } while (buffer_size - loaded_size > 0);

            f_.start_buffer(buffer.get(), buffer.get() + buffer_size);
            if (!parse_partial(buffer.get(),
                    buffer.get() + loaded_size, eof_reached)) {
                return false;
            }
            f_.end_buffer(buffer.get() + loaded_size);
        } while (!eof_reached);

        return true;
    }

private:
    bool parse_partial(const char_type* begin, const char_type* end,
        bool eof_reached)
    {
        try {
            p_ = begin;
            physical_line_or_buffer_begin_ = begin;
            set_first_last();
            while (p_ < end) {
                step([this](const auto& h) { h.normal(*this, *p_); });
                ++p_;
            }
            step([this](const auto& h) { h.underflow(*this); });
            if (eof_reached) {
                set_first_last();
                step([this](const auto& h) { h.eof(*this); });
                if (record_started_) {
                    end_record();
                }
            }
            physical_line_chars_passed_away_ +=
                p_ - physical_line_or_buffer_begin_;
            return true;
        } catch (text_error& e) {
            e.set_physical_position(
                physical_line_index_,
                (p_ - physical_line_or_buffer_begin_)
                    + physical_line_chars_passed_away_);
            throw;
        } catch (const parse_aborted&) {
            return false;
        }
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

    void update_last() noexcept
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
    static auto do_or_abort(F f)
     -> std::enable_if_t<std::is_void<decltype(f())>::value>
    {
        f();
    }

    template <class F>
    static auto do_or_abort(F f)
     -> std::enable_if_t<!std::is_void<decltype(f())>::value>
    {
        if (!f()) {
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

template <class Handler>
primitive_parser<Handler> make_primitive_parser(Handler&& handler)
    noexcept(std::is_nothrow_constructible<
        primitive_parser<Handler>, Handler&&>::value)
{
    static_assert(!std::is_reference<Handler>::value, "");
    return primitive_parser<Handler>(std::move(handler));
}

} // end namespace detail

template <class Tr, class Handler>
auto parse_csv(std::basic_streambuf<typename Handler::char_type, Tr>* in,
    Handler handler)
 -> std::enable_if_t<detail::with_buffer_control<Handler>::value, bool>
{
    return detail::make_primitive_parser(
        detail::make_full_fledged(std::move(handler))).parse_csv(in);
}

template <class Tr, class Handler>
auto parse_csv(std::basic_istream<typename Handler::char_type, Tr>& in,
    Handler handler)
 -> std::enable_if_t<detail::with_buffer_control<Handler>::value, bool>
{
    return parse_csv(in.rdbuf(), std::move(handler));
}

template <class Tr, class Handler,
    class Allocator = std::allocator<typename Handler::char_type>>
auto parse_csv(std::basic_streambuf<typename Handler::char_type, Tr>* in,
    Handler handler,
    std::size_t buffer_size = 0, const Allocator& alloc = Allocator())
 -> std::enable_if_t<detail::without_buffer_control<Handler>::value, bool>
{
    return detail::make_primitive_parser(
        detail::make_full_fledged(
            std::move(handler), buffer_size, alloc)).parse_csv(in);
}

template <class Tr, class Handler,
    class Allocator = std::allocator<typename Handler::char_type>>
auto parse_csv(std::basic_istream<typename Handler::char_type, Tr>& in,
    Handler handler,
    std::size_t buffer_size = 0, const Allocator& alloc = Allocator())
 -> std::enable_if_t<detail::without_buffer_control<Handler>::value, bool>
{
    return parse_csv(in.rdbuf(), std::move(handler), buffer_size, alloc);
}

template <class Input, class Handler, class... Args>
auto parse_csv(Input&& in,
    const std::reference_wrapper<Handler>& handler, Args&&... args)
{
    return parse_csv(std::forward<Input>(in),
        detail::wrapper_handler<Handler>(handler.get()),
        std::forward<Args>(args)...);
}

}

#endif
