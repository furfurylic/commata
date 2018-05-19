/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_GUARD_4B6A00F6_E33D_4114_9F57_D2C2E984E809
#define FURFURYLIC_GUARD_4B6A00F6_E33D_4114_9F57_D2C2E984E809

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <istream>
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
            parser.force_start_record();
            parser.change_state(state::right_of_open_quote);
            break;
        case key_chars<typename Parser::char_type>::CR:
            parser.new_physical_row();
            parser.empty_physical_row();
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
            parser.force_start_record();
            parser.change_state(state::right_of_open_quote);
            break;
        case key_chars<typename Parser::char_type>::CR:
            parser.new_physical_row();
            parser.empty_physical_row();
            parser.change_state(state::after_cr);
            break;
        case key_chars<typename Parser::char_type>::LF:
            parser.new_physical_row();
            parser.empty_physical_row();
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

struct has_get_buffer_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<std::pair<typename T::char_type*, std::size_t>&>() =
            std::declval<T&>().get_buffer(),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

template <class T>
struct has_get_buffer :
    decltype(has_get_buffer_impl::check<T>(nullptr))
{};

struct has_release_buffer_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<T&>().release_buffer(
            std::declval<const typename T::char_type*>()),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

template <class T>
struct has_release_buffer :
    decltype(has_release_buffer_impl::check<T>(nullptr))
{};

template <class Sink, bool X>
class buffer_control;

template <class Sink>
class buffer_control<Sink, false>
{
    std::size_t buffer_size_;
    typename Sink::char_type* buffer_;

protected:
    explicit buffer_control(std::size_t buffer_size) :
        buffer_size_((buffer_size < 1) ? 8192 : buffer_size),
        buffer_(nullptr)
    {}

    ~buffer_control()
    {
        delete [] buffer_;
    }

    std::pair<typename Sink::char_type*, std::size_t> do_get_buffer(Sink&)
    {
        if (!buffer_) {
            buffer_ = new typename Sink::char_type[buffer_size_];   // throw
        }
        return std::make_pair(buffer_, buffer_size_);
    }

    void do_release_buffer(Sink&, const typename Sink::char_type*) noexcept
    {}
};

template <class Sink>
class buffer_control<Sink, true>
{
protected:
    explicit buffer_control(std::size_t)
    {}

    std::pair<typename Sink::char_type*, std::size_t> do_get_buffer(Sink& f)
    {
        return f.get_buffer();  // throw
    }

    void do_release_buffer(
        Sink& f, const typename Sink::char_type* buffer) noexcept
    {
        return f.release_buffer(buffer);
    }
};

struct has_start_buffer_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<T&>().start_buffer(
            std::declval<const typename T::char_type*>()),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

template <class T>
struct has_start_buffer :
    decltype(has_start_buffer_impl::check<T>(nullptr))
{};

struct has_end_buffer_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<T&>().end_buffer(
            std::declval<const typename T::char_type*>()),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

template <class T>
struct has_end_buffer :
    decltype(has_end_buffer_impl::check<T>(nullptr))
{};

struct has_empty_record_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<bool&>() = std::declval<T&>().empty_physical_row(
            std::declval<const typename T::char_type*>()),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

template <class T>
struct has_empty_record :
    decltype(has_empty_record_impl::check<T>(nullptr))
{};

template <class Sink>
struct is_full_fledged_sink :
    std::integral_constant<bool,
        has_get_buffer<Sink>::value && has_release_buffer<Sink>::value
     && has_start_buffer<Sink>::value && has_end_buffer<Sink>::value
     && has_empty_record<Sink>::value>
{};

template <class Sink>
class full_fledged_sink :
    public buffer_control<Sink,
        has_get_buffer<Sink>::value && has_release_buffer<Sink>::value>
{
    static_assert(has_get_buffer<Sink>::value ==
                  has_release_buffer<Sink>::value,
        "Sink has only one of get_buffer and release_buffer");

    static_assert(!is_full_fledged_sink<Sink>::value,
        "Sink is already full-fledged");

    Sink sink_;

public:
    using char_type = typename Sink::char_type;

    explicit full_fledged_sink(Sink&& sink, std::size_t buffer_size_hint) :
        buffer_control<Sink,
            has_get_buffer<Sink>::value && has_release_buffer<Sink>::value>(
                buffer_size_hint),
        sink_(std::move(sink))
    {}

    std::pair<char_type*, std::size_t> get_buffer()
    {
        return this->do_get_buffer(sink_);
    }

    void release_buffer(const char_type* buffer) noexcept
    {
        this->do_release_buffer(sink_, buffer);
    }

    void start_buffer(const char_type* buffer_begin)
    {
        start_buffer(has_start_buffer<Sink>(), buffer_begin);
    }

    void end_buffer(const char_type* buffer_end)
    {
        end_buffer(has_end_buffer<Sink>(), buffer_end);
    }

    void start_record(const char_type* record_begin)
    {
        sink_.start_record(record_begin);
    }

    bool update(const char_type* first, const char_type* last)
    {
        return sink_.update(first, last);
    }

    bool finalize(const char_type* first, const char_type* last)
    {
        return sink_.finalize(first, last);
    }

    bool end_record(const char_type* end)
    {
        return sink_.end_record(end);
    }

    bool empty_physical_row(const char_type* where)
    {
        return empty_physical_row(has_empty_record<Sink>(), where);
    }

private:
    void start_buffer(std::true_type, const char_type* buffer_begin)
    {
        sink_.start_buffer(buffer_begin);
    }

    void start_buffer(std::false_type, ...)
    {}

    void end_buffer(std::true_type, const char_type* buffer_end)
    {
        sink_.end_buffer(buffer_end);
    }

    void end_buffer(std::false_type, ...)
    {}

    bool empty_physical_row(std::true_type, const char_type* where)
    {
        return sink_.empty_physical_row(where);
    }

    bool empty_physical_row(std::false_type, ...)
    {
        return true;
    }
};

template <class Sink>
auto make_full_fledged(Sink&& sink, std::size_t buffer_size_hint)
 -> std::enable_if_t<
        !is_full_fledged_sink<Sink>::value, full_fledged_sink<Sink>>
{
    return full_fledged_sink<Sink>(
        std::forward<Sink>(sink), buffer_size_hint);
}

template <class Sink>
auto make_full_fledged(Sink&& sink, std::size_t)
 -> std::enable_if_t<is_full_fledged_sink<Sink>::value, Sink&&>
{
    return std::forward<Sink>(sink);
}

template <class Sink>
class primitive_parser
{
    using char_type = typename Sink::char_type;

private:
    const char_type* p_;
    Sink f_;

    bool record_started_;
    state s_;
    const char_type* field_start_;
    const char_type* field_end_;

    std::size_t physical_row_index_;
    const char_type* physical_row_or_buffer_begin_;
    std::size_t physical_row_chars_passed_away_;

private:
    template <state>
    friend struct handler;

    // To make control flows clearer, we adopt exceptions. Sigh...
    struct parse_aborted
    {};

public:
    explicit primitive_parser(Sink f) :
        f_(std::move(f)),
        record_started_(false), s_(state::after_lf),
        physical_row_index_(parse_error::npos),
        physical_row_chars_passed_away_(0)
    {}

    primitive_parser(primitive_parser&&) = default;
    primitive_parser& operator=(primitive_parser&&) = default;

    template <class Tr>
    bool parse(std::basic_streambuf<char_type, Tr>* in)
    {
        auto release = [this](const char_type* buffer) {
            f_.release_buffer(buffer);
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

            f_.start_buffer(buffer.get());
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
            physical_row_or_buffer_begin_ = begin;
            set_first_last();
            while (p_ < end) {
                with_handler([this](const auto& h) { h.normal(*this, *p_); });
                ++p_;
            }
            with_handler([this](const auto& h) { h.underflow(*this); });
            if (eof_reached) {
                set_first_last();
                with_handler([this](const auto& h) { h.eof(*this); });
                if (record_started_) {
                    end_record();
                }
            }
            physical_row_chars_passed_away_ +=
                p_ - physical_row_or_buffer_begin_;
            return true;
        } catch (csv_error& e) {
            e.set_physical_position(
                physical_row_index_,
                (p_ - physical_row_or_buffer_begin_)
                    + physical_row_chars_passed_away_);
            throw;
        } catch (const parse_aborted&) {
            return false;
        }
    }

private:
    void new_physical_row()
    {
        if (physical_row_index_ == parse_error::npos) {
            physical_row_index_ = 0;
        } else {
            ++physical_row_index_;
        }
        physical_row_or_buffer_begin_ = p_;
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

    void force_start_record()
    {
        f_.start_record(p_);
        record_started_ = true;
    }

    void end_record()
    {
        if (!f_.end_record(p_)) {
            throw parse_aborted();
        }
        record_started_ = false;
    }

    void empty_physical_row()
    {
        assert(!record_started_);
        if (!f_.empty_physical_row(p_)) {
            throw parse_aborted();
        }
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
};

template <class Sink>
primitive_parser<Sink> make_primitive_parser(Sink&& sink)
{
    return primitive_parser<Sink>(std::move(sink));
}

} // end namespace detail

template <class Tr, class Sink>
bool parse(std::basic_streambuf<typename Sink::char_type, Tr>* in, Sink sink,
    std::size_t buffer_size = 0)
{
    return detail::make_primitive_parser(
        detail::make_full_fledged(
            std::move(sink), buffer_size)).parse(in);
}

template <class Tr, class Sink>
bool parse(std::basic_istream<typename Sink::char_type, Tr>& in, Sink sink,
    std::size_t buffer_size = 0)
{
    return parse(in.rdbuf(), std::move(sink), buffer_size);
}

namespace detail {

template <class Sink, class D, class = void>
struct get_buffer_t
{};

template <class Sink, class D>
struct get_buffer_t<Sink, D,
    std::enable_if_t<has_get_buffer<Sink>::value>>
{
    std::pair<typename Sink::char_type*, std::size_t> get_buffer()
    {
        return static_cast<D*>(this)->base().get_buffer();
    }
};

template <class Sink, class D, class = void>
struct release_buffer_t
{};

template <class Sink, class D>
struct release_buffer_t<Sink, D,
    std::enable_if_t<has_release_buffer<Sink>::value>>
{
    void release_buffer(const typename Sink::char_type* buffer)
    {
        static_cast<D*>(this)->base().release_buffer(buffer);
    }
};

template <class Sink, class D, class = void>
struct start_buffer_t
{};

template <class Sink, class D>
struct start_buffer_t<Sink, D,
    std::enable_if_t<has_start_buffer<Sink>::value>>
{
    void start_buffer(const typename Sink::char_type* buffer_begin)
    {
        static_cast<D*>(this)->base().start_buffer(buffer_begin);
    }
};

template <class Sink, class D, class = void>
struct end_buffer_t
{};

template <class Sink, class D>
struct end_buffer_t<Sink, D,
    std::enable_if_t<has_end_buffer<Sink>::value>>
{
    void end_buffer(const typename Sink::char_type* buffer_end)
    {
        static_cast<D*>(this)->base().end_buffer(buffer_end);
    }
};

}

template <class Sink>
class empty_physical_row_aware_sink :
    public detail::get_buffer_t<Sink, empty_physical_row_aware_sink<Sink>>,
    public detail::release_buffer_t<Sink, empty_physical_row_aware_sink<Sink>>,
    public detail::start_buffer_t<Sink, empty_physical_row_aware_sink<Sink>>,
    public detail::end_buffer_t<Sink, empty_physical_row_aware_sink<Sink>>
{
    Sink sink_;

public:
    using char_type = typename Sink::char_type;

    explicit empty_physical_row_aware_sink(Sink&& sink) :
        sink_(std::move(sink))
    {
        using sink_t = std::remove_pointer_t<decltype(this)>;
        static_assert(detail::has_get_buffer<sink_t>::value
            == detail::has_get_buffer<Sink>::value, "");
        static_assert(detail::has_release_buffer<sink_t>::value
            == detail::has_release_buffer<Sink>::value, "");
        static_assert(detail::has_start_buffer<sink_t>::value
            == detail::has_start_buffer<Sink>::value, "");
        static_assert(detail::has_end_buffer<sink_t>::value
            == detail::has_end_buffer<Sink>::value, "");
    }

    void start_record(const char_type* record_begin)
    {
        sink_.start_record(record_begin);
    }

    bool update(const char_type* first, const char_type* last)
    {
        return sink_.update(first, last);
    }

    bool finalize(const char_type* first, const char_type* last)
    {
        return sink_.finalize(first, last);
    }

    bool end_record(const char_type* end)
    {
        return sink_.end_record(end);
    }

    bool empty_physical_row(const char_type* where)
    {
        start_record(where);
        return end_record(where);
    }

    Sink& base()
    {
        return sink_;
    }
};

template <class Sink>
auto make_empty_physical_row_aware(Sink&& sink)
{
    return empty_physical_row_aware_sink<
        std::remove_reference_t<Sink>>(std::forward<Sink>(sink));
}

}}

#endif
