/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_9AF7CB02_5702_4A95_AA5E_781F44203C7F
#define COMMATA_GUARD_9AF7CB02_5702_4A95_AA5E_781F44203C7F

#include <type_traits>

#include "handler_decorator.hpp"

namespace commata::detail {

template <class Input, class Handler, class State, class D>
class base_parser
{
    static_assert(
        std::is_same_v<
            typename Input::char_type, typename Handler::char_type>,
            "Input::char_type and Handler::char_type are inconsistent; "
            "they shall be the same type");

public:
    using char_type = typename Handler::char_type;

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

    State s_;
    bool record_started_;
    bool eof_reached_;

private:
    // To make control flows clearer, we adopt exceptions. Sigh...
    struct parse_aborted
    {};

public:
    template <class InputR, class HandlerR,
        std::enable_if_t<
            std::is_constructible_v<Input, InputR&&>
         && std::is_constructible_v<Handler, HandlerR&&>>* = nullptr>
    explicit base_parser(InputR&& in, HandlerR&& f)
        noexcept(std::is_nothrow_constructible_v<Input, InputR&&>
              && std::is_nothrow_constructible_v<Handler, HandlerR&&>) :
        p_(nullptr), f_(std::forward<HandlerR>(f)),
        first_(nullptr), last_(nullptr),
        physical_line_index_(parse_error::npos),
        physical_line_or_buffer_begin_(nullptr),
        physical_line_chars_passed_away_(0),
        in_(std::forward<InputR>(in)), buffer_(nullptr), buffer_last_(nullptr),
        s_(D::first_state), record_started_(false), eof_reached_(false)
    {}

    base_parser(base_parser&& other) noexcept(
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

    ~base_parser()
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
                D::step(s_, [this](const auto& h) {
                    h.normal(*static_cast<D*>(this), p_, buffer_last_);
                });

                if constexpr (has_yield_v<Handler>) {
                    if (f_.yield(1)) {
                        return true;
                    }
                }
yield_1:
                ++p_;
            }
            D::step(s_, [this](const auto& h) { h.underflow(*this); });
            if (eof_reached_) {
                set_first_last();
                D::step(s_, [this](const auto& h) { h.eof(*this); });
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

public:
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

    void change_state(State s) noexcept
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

    template <class F>
    static void do_or_abort(F f)
    {
        if constexpr (std::is_void_v<decltype(f())>) {
            f();
        } else if (!f()) {
            throw parse_aborted();
        }
    }
};

} // end commata::detail

#endif
