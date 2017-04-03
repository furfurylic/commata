/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_GUARD_4B6A00F6_E33D_4114_9F57_D2C2E984E809
#define FURFURYLIC_GUARD_4B6A00F6_E33D_4114_9F57_D2C2E984E809

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ios>
#include <memory>
#include <string>
#include <streambuf>
#include <utility>

#include "csv_error.hpp"
#include "key_chars.hpp"

namespace furfurylic {
namespace commata {

class parse_error :
    public csv_error
{
public:
    explicit parse_error(std::string what_arg) :
        csv_error(std::move(what_arg))
    {}
};

namespace detail {

enum class state : std::int_fast8_t
{
    left_of_value,
    in_value,
    right_of_open_quote,
    in_quoted_value,
    in_quoted_value_after_quote,
    after_cr,
    after_lf
};

template <state s>
struct handler
{};

template <>
struct handler<state::left_of_value>
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
struct handler<state::in_value>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::char_type c) const
    {
        switch (c) {
        case key_chars<typename Parser::char_type>::COMMA:
            parser.finalize();
            parser.change_state(state::left_of_value);
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
struct handler<state::right_of_open_quote>
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
struct handler<state::in_quoted_value>
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
struct handler<state::in_quoted_value_after_quote>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::char_type c) const
    {
        switch (c) {
        case key_chars<typename Parser::char_type>::COMMA:
            parser.finalize();
            parser.change_state(state::left_of_value);
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
struct handler<state::after_cr>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::char_type c) const
    {
        switch (c) {
        case key_chars<typename Parser::char_type>::COMMA:
            parser.new_physical_row();
            parser.set_first_last();
            parser.finalize();
            parser.change_state(state::left_of_value);
            break;
        case key_chars<typename Parser::char_type>::DQUOTE:
            parser.new_physical_row();
            parser.change_state(state::right_of_open_quote);
            break;
        case key_chars<typename Parser::char_type>::CR:
            parser.new_physical_row();
            break;
        case key_chars<typename Parser::char_type>::LF:
            parser.change_state(state::after_lf);
            break;
        default:
            parser.new_physical_row();
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
struct handler<state::after_lf>
{
    template <class Parser>
    void normal(Parser& parser, typename Parser::char_type c) const
    {
        switch (c) {
        case key_chars<typename Parser::char_type>::COMMA:
            parser.new_physical_row();
            parser.set_first_last();
            parser.finalize();
            parser.change_state(state::left_of_value);
            break;
        case key_chars<typename Parser::char_type>::DQUOTE:
            parser.new_physical_row();
            parser.change_state(state::right_of_open_quote);
            break;
        case key_chars<typename Parser::char_type>::CR:
            parser.new_physical_row();
            parser.change_state(state::after_cr);
            break;
        case key_chars<typename Parser::char_type>::LF:
            parser.new_physical_row();
            break;
        default:
            parser.new_physical_row();
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

template <class Ch, class Sink>
class primitive_parser
{
    const Ch* p_;
    Sink f_;

    bool record_started_;
    state s_;
    const Ch* field_start_;
    const Ch* field_end_;

    std::size_t physical_row_;
    const Ch* physical_row_begin_;
    std::size_t physical_row_chars_passed_away_;

private:
    template <state>
    friend struct handler;

    // To make control flows clearer, we adopt exceptions. Sigh...
    struct parse_aborted
    {};

public:
    using char_type = Ch;

    explicit primitive_parser(Sink f) :
        f_(std::move(f)),
        record_started_(false), s_(state::after_lf),
        physical_row_(parse_error::npos), physical_row_chars_passed_away_(0)
    {}

    primitive_parser(primitive_parser&&) = default;
    primitive_parser& operator=(primitive_parser&&) = default;

    bool parse(const Ch* begin, const Ch* end)
    {
        try {
            p_ = begin;
            physical_row_begin_ = begin;
            set_first_last();
            f_.start_buffer(begin);
            while (p_ < end) {
                with_handler(
                    std::bind(
                        call_normal(), std::placeholders::_1, this, *p_));
                ++p_;
            }
            with_handler(
                std::bind(
                    call_underflow(), std::placeholders::_1, this));
            f_.end_buffer(end);
            physical_row_chars_passed_away_ += p_ - physical_row_begin_;
            return true;
        } catch (csv_error& e) {
            e.set_physical_position(
                physical_row_,
                (p_ - physical_row_begin_) + physical_row_chars_passed_away_);
            throw;
        } catch (const parse_aborted&) {
            return false;
        }
    }

    bool eof()
    {
        try {
            p_ = nullptr;
            set_first_last();
            f_.start_buffer(nullptr);
            with_handler(
                std::bind(
                    call_eof(), std::placeholders::_1, this));
            if (record_started_) {
                end_record();
            }
            f_.end_buffer(nullptr);
            return true;
        } catch (const parse_aborted&) {
            return false;
        }
    }

private:
    void new_physical_row()
    {
        if (physical_row_ == parse_error::npos) {
            physical_row_ = 0;
        } else {
            ++physical_row_;
        }
        physical_row_begin_ = p_;
        physical_row_chars_passed_away_ = 0;
    }

    void change_state(state s)
    {
        s_ = s;
    }

    void set_first_last()
    {
        field_start_ = p_;
        field_end_ = p_;
    }

    void update_last()
    {
        field_end_ = p_ + 1;
    }

    void update()
    {
        if (!record_started_) {
            f_.start_record(field_start_);
            record_started_ = true;
        }
        if (field_start_ < field_end_) {
            if (!f_.update(field_start_, field_end_)) {
                throw parse_aborted();
            }
        }
    }

    void finalize()
    {
        if (!record_started_) {
            f_.start_record(field_start_);
            record_started_ = true;
        }
        if (!f_.finalize(field_start_, field_end_)) {
            throw parse_aborted();
        }
    }

    void end_record()
    {
        if (!f_.end_record(p_)) {
            throw parse_aborted();
        }
        record_started_ = false;
    }

private:
    template <class F>
    void with_handler(F f)
    {
        switch (s_) {
        case state::left_of_value:
            f(handler<state::left_of_value>());
            break;
        case state::in_value:
            f(handler<state::in_value>());
            break;
        case state::right_of_open_quote:
            f(handler<state::right_of_open_quote>());
            break;
        case state::in_quoted_value:
            f(handler<state::in_quoted_value>());
            break;
        case state::in_quoted_value_after_quote:
            f(handler<state::in_quoted_value_after_quote>());
            break;
        case state::after_cr:
            f(handler<state::after_cr>());
            break;
        case state::after_lf:
            f(handler<state::after_lf>());
            break;
        default:
            assert(false);
            break;
        }
    }

    struct call_normal
    {
        template <class Handler>
        void operator()(const Handler& handler, primitive_parser* me, Ch c)
            const
        {
            handler.normal(*me, c);
        }
    };

    struct call_underflow
    {
        template <class Handler>
        void operator()(const Handler& handler, primitive_parser* me) const
        {
            handler.underflow(*me);
        }
    };

    struct call_eof
    {
        template <class Handler>
        void operator()(const Handler& handler, primitive_parser* me) const
        {
            handler.eof(*me);
        }
    };
};

} // end namespace detail

template <class Ch, class Tr, class Sink>
bool parse(std::basic_streambuf<Ch, Tr>& in, std::streamsize buffer_size,
      Sink sink)
{
    std::unique_ptr<Ch[]> buffer(new Ch[buffer_size]);
    detail::primitive_parser<Ch, Sink> p(std::move(sink));
    for (;;) {
        std::streamsize loaded_size = in.sgetn(buffer.get(), buffer_size);
        if (loaded_size == 0) {
            break;
        }
        if (!p.parse(buffer.get(), buffer.get() + loaded_size)) {
            return false;
        }
    }
    return p.eof();
}

}}

#endif
