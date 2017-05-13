/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_GUARD_4B6A00F6_E33D_4114_9F57_D2C2E984E809
#define FURFURYLIC_GUARD_4B6A00F6_E33D_4114_9F57_D2C2E984E809

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <ios>
#include <memory>
#include <string>
#include <streambuf>
#include <utility>

#include "key_chars.hpp"

namespace furfurylic {
namespace commata {

namespace detail {

template <class T>
struct npos_impl
{
    static const T npos;
};

template <class T>
const T npos_impl<T>::npos = static_cast<T>(-1);

}

class csv_error :
    public std::exception, public detail::npos_impl<std::size_t>
{
    std::shared_ptr<std::string> what_;
    std::pair<std::size_t, std::size_t> physical_position_;

public:
    explicit csv_error(std::string what_arg) :
        what_(std::make_shared<std::string>(std::move(what_arg))),
        physical_position_(npos, npos)
    {}

    const char* what() const noexcept override
    {
        return what_->c_str();
    }

    void set_physical_position(std::size_t row, std::size_t col) noexcept
    {
        physical_position_ = std::make_pair(row, col);
    }

    const std::pair<std::size_t, std::size_t>* get_physical_position() const noexcept
    {
        return (physical_position_ != std::make_pair(npos, npos)) ?
            &physical_position_ : nullptr;
    }
};

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
            throw parse_error("A quotation mark found in an non-escaped value");
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
        switch (c) {
        case key_chars<typename Parser::char_type>::DQUOTE:
            parser.set_first_last();
            parser.change_state(state::in_quoted_value_after_quote);
            break;
        default:
            parser.set_first_last();
            parser.update_last();
            parser.change_state(state::in_quoted_value);
            break;
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
            throw parse_error("An invalid character found after a closed escaped value");
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
            parser.change_state(state::after_cr);
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

}

template <class Ch, class Sink>
class primitive_parser
{
    const Ch* p_;
    Sink f_;

    bool record_started_;
    detail::state s_;
    const Ch* field_start_;
    const Ch* field_end_;
    bool continues_;

    std::size_t physical_row_;
    const Ch* physical_row_begin_;
    std::size_t physical_row_chars_passed_away_;

private:
    template <detail::state>
    friend struct detail::handler;

public:
    using char_type = Ch;

    explicit primitive_parser(Sink f) :
        f_(std::move(f)),
        record_started_(false), s_(detail::state::after_lf),
        continues_(true),
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
                with_handler(std::bind(call_normal(), std::placeholders::_1, this, *p_));
                if (!continues_) {
                    return false;
                }
                ++p_;
            }
            with_handler(std::bind(call_underflow(), std::placeholders::_1, this));
            if (!continues_) {
                return false;
            }
            f_.end_buffer(end);
            physical_row_chars_passed_away_ += p_ - physical_row_begin_;
            return true;
        } catch (csv_error& e) {
            e.set_physical_position(
                physical_row_,
                (p_ - physical_row_begin_) + physical_row_chars_passed_away_);
            throw;
        }
    }

    bool eof()
    {
        p_ = nullptr;
        set_first_last();
        f_.start_buffer(nullptr);
        with_handler(std::bind(call_eof(), std::placeholders::_1, this));
        if (!continues_) {
            return false;
        }
        if (record_started_) {
            end_record();
        }
        f_.end_buffer(nullptr);
        return continues_;
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

    void change_state(detail::state s)
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
            if (continues_) {
                f_.start_record(field_start_);
            }
            record_started_ = true;
        }
        if (field_start_ < field_end_) {
            if (continues_) {
                continues_ = f_.update(field_start_, field_end_);
            }
        }
    }

    void finalize()
    {
        if (!record_started_) {
            if (continues_) {
                f_.start_record(field_start_);
            }
            record_started_ = true;
        }
        if (continues_) {
            continues_ = f_.finalize(field_start_, field_end_);
        }
    }

    void end_record()
    {
        if (continues_) {
            continues_ = f_.end_record(p_);
        }
        record_started_ = false;
    }

private:
    template <class F>
    void with_handler(F f) {
        switch (s_) {
        case detail::state::left_of_value:
            f(detail::handler<detail::state::left_of_value>());
            break;
        case detail::state::in_value:
            f(detail::handler<detail::state::in_value>());
            break;
        case detail::state::right_of_open_quote:
            f(detail::handler<detail::state::right_of_open_quote>());
            break;
        case detail::state::in_quoted_value:
            f(detail::handler<detail::state::in_quoted_value>());
            break;
        case detail::state::in_quoted_value_after_quote:
            f(detail::handler<detail::state::in_quoted_value_after_quote>());
            break;
        case detail::state::after_cr:
            f(detail::handler<detail::state::after_cr>());
            break;
        case detail::state::after_lf:
            f(detail::handler<detail::state::after_lf>());
            break;
        default:
            assert(false);
            break;
        }
    }

    struct call_normal
    {
        template <class Handler>
        void operator()(const Handler& handler, primitive_parser* me, Ch c) const
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

template <class Ch, class Tr, class Sink>
bool parse(std::basic_streambuf<Ch, Tr>& in, std::streamsize buffer_size, Sink sink)
{
    std::unique_ptr<Ch[]> buffer(new Ch[buffer_size]);
    primitive_parser<Ch, Sink> p(std::move(sink));
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
