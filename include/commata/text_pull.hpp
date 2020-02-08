/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_43DAA7B6_A9E7_4410_9210_CAADC6EA9D6F
#define COMMATA_GUARD_43DAA7B6_A9E7_4410_9210_CAADC6EA9D6F

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iosfwd>
#include <memory>
#include <streambuf>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "allocation_only_allocator.hpp"
#include "empty_string.hpp"
#include "formatted_output.hpp"
#include "handler_decorator.hpp"
#include "member_like_base.hpp"

namespace commata {

enum class primitive_text_pull_state : std::uint_fast8_t
{
    eof,
    moved,
    before_parse,
    start_record,
    end_record,
    update,
    finalize,
    empty_physical_line,
    start_buffer,
    end_buffer
};

enum primitive_text_pull_handle : std::uint_fast8_t
{
    primitive_text_pull_handle_start_buffer        = 1,
    primitive_text_pull_handle_end_buffer          = 1 << 1,
    primitive_text_pull_handle_start_record        = 1 << 2,
    primitive_text_pull_handle_end_record          = 1 << 3,
    primitive_text_pull_handle_empty_physical_line = 1 << 4,
    primitive_text_pull_handle_update              = 1 << 5,
    primitive_text_pull_handle_finalize            = 1 << 6,
    primitive_text_pull_handle_all = static_cast<std::uint_fast8_t>(-1)
};

namespace detail {

struct has_get_physical_position_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<std::pair<std::size_t, std::size_t>&>() =
            std::declval<const T&>().get_physical_position(),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

template <class T>
struct has_get_physical_position :
    decltype(has_get_physical_position_impl::check<T>(nullptr))
{};

template <class Ch, class Allocator,
    std::underlying_type_t<primitive_text_pull_handle> Handle>
class pull_handler
{
    using at_t = std::allocator_traits<Allocator>;

    using state_queue_a_t = allocation_only_allocator<
        typename at_t::template rebind_alloc<primitive_text_pull_state>>;
    using data_queue_a_t = allocation_only_allocator<
        typename at_t::template rebind_alloc<Ch*>>;

public:
    using char_type = Ch;
    using state_queue_type =
        std::vector<primitive_text_pull_state, state_queue_a_t>;
    using data_queue_type = std::vector<Ch*, data_queue_a_t>;

private:
    base_member_pair<Allocator, std::size_t> alloc_;
    typename at_t::pointer buffer_;
    state_queue_type sq_;
    data_queue_type dq_;
    std::size_t yield_location_;

public:
    explicit pull_handler(
        std::allocator_arg_t, const Allocator& alloc,
        std::size_t buffer_size) :
        alloc_(alloc,
            (buffer_size < 1) ? 8192 : (buffer_size < 2) ? 2 : buffer_size),
        buffer_(nullptr), yield_location_(0)
    {}

    pull_handler(const pull_handler& other) = delete;

    ~pull_handler()
    {
        if (buffer_) {
            at_t::deallocate(alloc_.base(), buffer_, alloc_.member());
        }
    }

    const state_queue_type& state_queue() const noexcept
    {
        return sq_;
    }

    state_queue_type& state_queue() noexcept
    {
        return sq_;
    }

    const data_queue_type& data_queue() const noexcept
    {
        return dq_;
    }

    data_queue_type& data_queue() noexcept
    {
        return dq_;
    }

    std::pair<char_type*, std::size_t> get_buffer()
    {
        if (!buffer_) {
            buffer_ = at_t::allocate(alloc_.base(), alloc_.member());
        }
        return { std::addressof(*buffer_), alloc_.member() - 1 };
    }

    void release_buffer(const char_type*) noexcept
    {}

    void start_buffer(const Ch* buffer_begin, const Ch* buffer_end)
    {
        start_buffer(buffer_begin, buffer_end,
            std::integral_constant<bool,
                (Handle & primitive_text_pull_handle_start_buffer) != 0>());
    }

private:
    void start_buffer(const Ch* buffer_begin, const Ch* buffer_end,
        std::true_type)
    {
        sq_.push_back(primitive_text_pull_state::start_buffer);
        dq_.push_back(uc(buffer_begin));
        dq_.push_back(uc(buffer_end));
    }

    void start_buffer(const Ch*, const Ch*, std::false_type) noexcept
    {}

public:
    void end_buffer(const Ch* buffer_end)
    {
        end_buffer(buffer_end,
            std::integral_constant<bool,
                (Handle & primitive_text_pull_handle_end_buffer) != 0>());
    }

private:
    void end_buffer(const Ch* buffer_end, std::true_type)
    {
        sq_.push_back(primitive_text_pull_state::end_buffer);
        dq_.push_back(uc(buffer_end));
    }

    void end_buffer(const Ch*, std::false_type) noexcept
    {}

public:
    void start_record(const char_type* record_begin)
    {
        start_record(record_begin,
            std::integral_constant<bool,
                (Handle & primitive_text_pull_handle_start_record) != 0>());
    }

private:
    void start_record(const char_type* record_begin, std::true_type)
    {
        sq_.push_back(primitive_text_pull_state::start_record);
        dq_.push_back(uc(record_begin));
    }

    void start_record(const char_type*, std::false_type)
    {}

public:
    void update(const char_type* first, const char_type* last)
    {
        update(first, last,
            std::integral_constant<bool,
                (Handle & primitive_text_pull_handle_update) != 0>());
    }

private:
    void update(const char_type* first, const char_type* last,
        std::true_type)
    {
        sq_.push_back(primitive_text_pull_state::update);
        dq_.push_back(uc(first));
        dq_.push_back(uc(last));
    }

    void update(const char_type*, const char_type*, std::false_type)
    {}

public:
    void finalize(const char_type* first, const char_type* last)
    {
        finalize(first, last,
            std::integral_constant<bool,
                (Handle & primitive_text_pull_handle_finalize) != 0>());
    }

private:
    void finalize(const char_type* first, const char_type* last,
        std::true_type)
    {
        sq_.push_back(primitive_text_pull_state::finalize);
        dq_.push_back(uc(first));
        dq_.push_back(uc(last));
    }

    void finalize(const char_type*, const char_type*, std::false_type)
    {}

public:
    void end_record(const char_type* record_end)
    {
        end_record(record_end,
            std::integral_constant<bool,
                (Handle & primitive_text_pull_handle_end_record) != 0>());
    }

private:
    void end_record(const char_type* record_end, std::true_type)
    {
        sq_.push_back(primitive_text_pull_state::end_record);
        dq_.push_back(uc(record_end));
    }

    void end_record(const char_type*, std::false_type)
    {}

public:
    void empty_physical_line(const char_type* where)
    {
        empty_physical_line(where,
            std::integral_constant<bool,
                (Handle
               & primitive_text_pull_handle_empty_physical_line) != 0>());
    }

private:
    void empty_physical_line(const char_type* where, std::true_type)
    {
        sq_.push_back(primitive_text_pull_state::empty_physical_line);
        dq_.push_back(uc(where));
    }

    void empty_physical_line(const char_type*, std::false_type)
    {}

public:
    bool yield(std::size_t location) noexcept
    {
        if ((location != static_cast<std::size_t>(-1)) && sq_.empty()) {
            return false;
        } else {
            yield_location_ = location;
            return true;
        }
    }

    std::size_t yield_location() const noexcept
    {
        return yield_location_;
    }

private:
    char_type* uc(const char_type* s) const noexcept
    {
        return buffer_ + (s - buffer_);
    }
};

}

template <class TextSource,
    class Allocator = std::allocator<typename TextSource::char_type>,
    std::underlying_type_t<primitive_text_pull_handle> Handle =
        primitive_text_pull_handle_all>
class primitive_text_pull
{
public:
    using char_type = typename TextSource::char_type;
    using allocator_type = Allocator;
    using size_type = std::size_t;

private:
    using at_t = std::allocator_traits<Allocator>;

    using handler_t = detail::pull_handler<char_type, Allocator, Handle>;
    using handler_a_t = detail::allocation_only_allocator<
        typename at_t::template rebind_alloc<handler_t>>;
    using handler_at_t = std::allocator_traits<handler_a_t>;
    using handler_p_t = typename handler_at_t::pointer;

    using parser_t =
        decltype(std::declval<TextSource&>()(
            std::declval<detail::wrapper_handler<handler_t>>()));

    std::size_t i_sq_;
    std::size_t i_dq_;
    handler_p_t handler_;
    detail::base_member_pair<allocator_type, parser_t> ap_;

public:
    static constexpr bool physical_position_available =
        detail::has_get_physical_position<parser_t>::value;

    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    explicit primitive_text_pull(TextSource in, std::size_t buffer_size = 0) :
        primitive_text_pull(std::allocator_arg, Allocator(),
            std::move(in), buffer_size)
    {}

    primitive_text_pull(std::allocator_arg_t, const Allocator& alloc,
        TextSource in, std::size_t buffer_size = 0) :
        i_sq_(0), i_dq_(0),
        handler_(create_handler(alloc,
            std::allocator_arg, alloc, buffer_size)),
        ap_(alloc, in(detail::wrapper_handler<handler_t>(*handler_)))
    {
        handler_->state_queue().push_back(
            primitive_text_pull_state::before_parse);
    }

    primitive_text_pull(primitive_text_pull&& other) noexcept :
        i_sq_(other.i_sq_), i_dq_(other.i_dq_),
        handler_(other.handler_), ap_(std::move(other.ap_))
    {
        other.handler_ = nullptr;
    }

    ~primitive_text_pull()
    {
        if (handler_) {
            destroy_handler(handler_);
        }
    }

    allocator_type get_allocator() const noexcept
    {
        return ap_.base();
    }

    primitive_text_pull_state state() const noexcept
    {
        if (handler_) {
            const auto& sq = handler_->state_queue();
            assert(sq.size() > i_sq_);    
            return sq[i_sq_];
        } else {
            return primitive_text_pull_state::moved;
        }
    }

    explicit operator bool() const noexcept
    {
        if (handler_) {
            const auto& sq = handler_->state_queue();
            assert(sq.size() > i_sq_);    
            return (sq[i_sq_] != primitive_text_pull_state::eof);
        } else {
            return false;
        }
    }

    primitive_text_pull& operator()()
    {
        if (!handler_) { // means 'moved'
            return *this;
        }

        auto& sq = handler_->state_queue();
        auto& dq = handler_->data_queue();

        assert(!sq.empty());
        switch (sq[i_sq_]) {
        case primitive_text_pull_state::start_buffer:
        case primitive_text_pull_state::update:
        case primitive_text_pull_state::finalize:
            if (i_dq_ == dq.size() - 2) {
                dq.clear();
                i_dq_ = 0;
            } else {
                i_dq_ += 2;
            }
            break;
        case primitive_text_pull_state::end_buffer:
        case primitive_text_pull_state::start_record:
        case primitive_text_pull_state::end_record:
        case primitive_text_pull_state::empty_physical_line:
            if (i_dq_ == dq.size() - 1) {
                dq.clear();
                i_dq_ = 0;
            } else {
                ++i_dq_;
            }
            break;
        case primitive_text_pull_state::before_parse:
            break;
        case primitive_text_pull_state::eof:
            return *this;
        default:
            assert(false);
            break;
        }
        if (i_sq_ == sq.size() - 1) {
            sq.clear();
            i_sq_ = 0;
        } else {
            ++i_sq_;
        }

        if (sq.empty()) {
            ap_.member()();
        }
        if (sq.empty()) {
            sq.push_back(primitive_text_pull_state::eof);
        }
        return *this;
    }

    char_type* operator[](size_type i)
    {
        assert(i < data_size());
        return handler_->data_queue()[i_dq_ + i];
    }

    const char_type* operator[](size_type i) const
    {
        assert(i < data_size());
        return handler_->data_queue()[i_dq_ + i];
    }

    size_type data_size() const noexcept
    {
        if (handler_) {
            switch (handler_->state_queue()[i_sq_]) {
            case primitive_text_pull_state::start_buffer:
            case primitive_text_pull_state::update:
            case primitive_text_pull_state::finalize:
                return 2;
            case primitive_text_pull_state::end_buffer:
            case primitive_text_pull_state::start_record:
            case primitive_text_pull_state::end_record:
            case primitive_text_pull_state::empty_physical_line:
                return 1;
            case primitive_text_pull_state::before_parse:
            case primitive_text_pull_state::eof:
                break;
            default:
                assert(false);
                break;
            }
        }
        return 0;
    }

    std::pair<std::size_t, std::size_t> get_physical_position() const
        noexcept((!detail::has_get_physical_position<parser_t>::value)
              || noexcept(std::declval<const parser_t&>()
                            .get_physical_position()))
    {
        return get_physical_position_impl(
            detail::has_get_physical_position<parser_t>());
    }

private:
     template <class... Args>
    static auto create_handler(const Allocator& alloc, Args&&... args)
    {
        handler_a_t a(alloc);
        const auto p = handler_at_t::allocate(a, 1);
        ::new(std::addressof(*p)) handler_t(std::forward<Args>(args)...);
        return p;
    }

    void destroy_handler(const handler_p_t p)
    {
        p->~handler_t();
        handler_a_t a(get_allocator());
        handler_at_t::deallocate(a, p, 1);
    }

    std::pair<std::size_t, std::size_t> get_physical_position_impl(
        std::true_type) const noexcept(
            noexcept(std::declval<const parser_t&>().get_physical_position()))
    {
        return ap_.member().get_physical_position();
    }

    std::pair<std::size_t, std::size_t> get_physical_position_impl(
        std::false_type) const noexcept
    {
        return { npos, npos };
    }
};

enum class text_pull_state : std::uint_fast8_t
{
    eof,
    error,
    moved,
    before_parse,
    field,
    record_end
};

template <class TextSource,
    class Allocator = std::allocator<typename TextSource::char_type>>
class text_pull
{
public:
    using char_type = typename TextSource::char_type;
    using allocator_type = Allocator;

private:
    primitive_text_pull<TextSource, allocator_type,
        (primitive_text_pull_handle_end_buffer
       | primitive_text_pull_handle_end_record
       | primitive_text_pull_handle_empty_physical_line
       | primitive_text_pull_handle_update
       | primitive_text_pull_handle_finalize)> p_;
    bool empty_physical_line_aware_;
    bool suppresses_error_;

    text_pull_state last_state_;
    std::pair<char_type*, char_type*> last_;
    std::vector<char_type, Allocator> value_;
    bool value_expiring_;

    std::size_t i_;
    std::size_t j_;
    std::exception_ptr suppressed_error_;

public:
    using value_type = const char_type;
    using reference = value_type&;
    using const_reference = reference;
    using pointer = value_type*;
    using const_pointer = pointer;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = value_type*;
    using const_iterator = iterator;

    static constexpr bool physical_position_available =
        decltype(p_)::physical_position_available;

    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    explicit text_pull(TextSource in,
        std::size_t buffer_size = 0) :
        text_pull(std::allocator_arg, Allocator(), std::move(in), buffer_size)
    {}

    text_pull(std::allocator_arg_t, const Allocator& alloc,
        TextSource in, std::size_t buffer_size = 0) :
        p_(std::allocator_arg, alloc, std::move(in),
            ((buffer_size > 1) ? buffer_size : 2)),
        empty_physical_line_aware_(false), suppresses_error_(false),
        last_state_(text_pull_state::before_parse), last_(empty_string()),
        value_(alloc), value_expiring_(false), i_(0), j_(0)
    {}

    text_pull(text_pull&& other) noexcept :
        p_(std::move(other.p_)),
        empty_physical_line_aware_(other.empty_physical_line_aware_),
        suppresses_error_(other.suppresses_error_),
        last_state_(other.last_state_), last_(other.last_),
        value_(std::move(other.value_)),
        value_expiring_(other.value_expiring_), i_(other.i_), j_(other.j_),
        suppressed_error_(std::move(other.suppressed_error_))
    {
        other.last_state_ = text_pull_state::moved;
        other.last_ = empty_string();
        other.i_ = 0;
        other.j_ = 0;
        other.suppressed_error_ = nullptr;
    }

    ~text_pull() = default;

    allocator_type get_allocator() const noexcept
    {
        return value_.get_allocator();
    }

    bool is_empty_physical_line_aware() const noexcept
    {
        return empty_physical_line_aware_;
    }

    text_pull& set_empty_physical_line_aware(bool b = true) noexcept
    {
        empty_physical_line_aware_ = b;
        return *this;
    }

    bool suppresses_error() const noexcept
    {
        return suppresses_error_;
    }

    text_pull& set_suppresses_error(bool b = true) noexcept
    {
        suppresses_error_ = b;
        return *this;
    }

    text_pull_state state() const noexcept
    {
        return last_state_;
    }

    explicit operator bool() const noexcept
    {
        return (state() != text_pull_state::eof)
            && (state() != text_pull_state::error)
            && (state() != text_pull_state::moved);
    }

    std::pair<std::size_t, std::size_t> get_position() const noexcept
    {
        return std::make_pair(i_, j_);
    }

    std::pair<std::size_t, std::size_t> get_physical_position() const
        noexcept(noexcept(p_.get_physical_position()))
    {
        return p_.get_physical_position();
    }

    text_pull& operator()(std::size_t n = 0)
    {
        if (!*this) {
            return *this;
        }
        if (n < 1) {
            return next_field();
        }
        last_ = empty_string();
        value_.clear();
        for (;;) {
            if (value_expiring_) {
                ++j_;
                value_expiring_ = false;
            }
            try {
                if (!p_()) {
                    break;
                }
            } catch (...) {
                set_state(text_pull_state::error);
                if (suppresses_error_) {
                    suppressed_error_ = std::current_exception();
                    return *this;
                } else {
                    throw;
                }
            }
            switch (p_.state()) {
            case primitive_text_pull_state::update:
                break;
            case primitive_text_pull_state::finalize:
                set_state(text_pull_state::field);
                value_expiring_ = true;
                if (n == 1) {
                    return next_field();
                }
                --n;
                break;
            case primitive_text_pull_state::empty_physical_line:
                if (!empty_physical_line_aware_) {
                    break;
                }
                // fall through
            case primitive_text_pull_state::end_record:
                set_state(text_pull_state::record_end);
                return *this;
            case primitive_text_pull_state::end_buffer:
            default:
                break;
            }
        }
        set_state(text_pull_state::eof);
        return *this;
    }

private:
    text_pull& next_field()
    {
        assert(*this);
        if (value_expiring_) {
            value_.clear();
            last_ = empty_string();
            ++j_;
            value_expiring_ = false;
        }
        for (;;) {
            try {
                if (!p_()) {
                    break;
                }
            } catch (...) {
                set_state(text_pull_state::error);
                last_ = empty_string();
                if (suppresses_error_) {
                    suppressed_error_ = std::current_exception();
                    return *this;
                } else {
                    throw;
                }
            }
            switch (p_.state()) {
            case primitive_text_pull_state::update:
                do_update(p_[0], p_[1]);
                break;
            case primitive_text_pull_state::finalize:
                do_update(p_[0], p_[1]);
                if (value_.empty()) {
                    *last_.second = char_type();
                } else {
                    value_.push_back(char_type());
                    last_.first = value_.data();
                    last_.second = value_.data() + value_.size() - 1;
                }
                set_state(text_pull_state::field);
                value_expiring_ = true;
                return *this;
            case primitive_text_pull_state::empty_physical_line:
                if (!empty_physical_line_aware_) {
                    break;
                }
                // fall through
            case primitive_text_pull_state::end_record:
                set_state(text_pull_state::record_end);
                last_ = empty_string();
                return *this;
            case primitive_text_pull_state::end_buffer:
                if (last_.first != empty_string().first) {
                    value_.insert(value_.cend(), last_.first, last_.second);
                    last_.first = empty_string().first;
                }
                break;
            default:
                break;
            }
        }
        set_state(text_pull_state::eof);
        last_ = empty_string();
        return *this;
    }

public:
    text_pull& skip_record(std::size_t n = 0)
    {
        if (!*this) {
            return *this;
        }
        last_ = empty_string();
        value_.clear();
        for (;;) {
            if (value_expiring_) {
                ++j_;
                value_expiring_ = false;
            }
            try {
                if (!p_()) {
                    break;
                }
            } catch (...) {
                set_state(text_pull_state::error);
                if (suppresses_error_) {
                    suppressed_error_ = std::current_exception();
                    return *this;
                } else {
                    throw;
                }
            }
            switch (p_.state()) {
            case primitive_text_pull_state::update:
                break;
            case primitive_text_pull_state::finalize:
                set_state(text_pull_state::field);
                value_expiring_ = true;
                break;
            case primitive_text_pull_state::empty_physical_line:
                if (!empty_physical_line_aware_) {
                    break;
                }
                // fall through
            case primitive_text_pull_state::end_record:
                set_state(text_pull_state::record_end);
                if (n == 0) {
                    return *this;
                }
                --n;
                break;
            case primitive_text_pull_state::end_buffer:
            default:
                break;
            }
        }
        set_state(text_pull_state::eof);
        return *this;
    }

    void rethrow_suppressed()
    {
        if (suppressed_error_) {
            using std::swap;
            std::exception_ptr e;
            swap(e, suppressed_error_);
            std::rethrow_exception(e);
        }
    }

    const_iterator begin() const noexcept
    {
        return cbegin();
    }

    const_iterator end() const noexcept
    {
        return cend();
    }

    const_iterator cbegin() const noexcept
    {
        return last_.first;
    }

    const_iterator cend() const noexcept
    {
        return last_.second;
    }

    const_pointer c_str() const noexcept
    {
        return cbegin();
    }

    const_pointer data() const noexcept
    {
        return cbegin();
    }

    bool empty() const noexcept
    {
        return cbegin() == cend();
    }

    size_type size() const noexcept
    {
        return cend() - cbegin();
    }

    template <class OtherTr = std::char_traits<char_type>,
        class OtherAllocator = std::allocator<char_type>>
    explicit operator
    std::basic_string<char_type, OtherTr, OtherAllocator>() const
    {
        return { cbegin(), cend() };
    }

private:
    void set_state(text_pull_state s) noexcept
    {
        if (last_state_ == text_pull_state::record_end) {
            ++i_;
            j_ = 0;
        }
        last_state_ = s;
    }

    void do_update(char_type* first, char_type* last)
    {
        if (!value_.empty()) {
            value_.insert(value_.cend(), first, last);
        } else if (last_.first != empty_string().first) {
            TextSource::traits_type::move(last_.second, first, last - first);
            last_.second += last - first;
        } else {
            last_.first = first;
            last_.second = last;
        }
    }

    static std::pair<char_type*, char_type*>
    empty_string() noexcept
    {
        return std::make_pair(
            &detail::nul<char_type>::value, &detail::nul<char_type>::value);
    }
};

template <class TextSource, class Allocator,
    class OtherAllocator = std::allocator<typename TextSource::char_type>>
std::basic_string<typename TextSource::char_type,
                  std::char_traits<typename TextSource::char_type>,
                  OtherAllocator> to_string(
    const text_pull<TextSource, Allocator>& p,
    const OtherAllocator& alloc = Allocator())
{
    return { p.cbegin(), p.cend(), alloc };
}

template <class Tr, class TextSource, class Allocator>
std::basic_ostream<typename TextSource::char_type, Tr>& operator<<(
    std::basic_ostream<typename TextSource::char_type, Tr>& os,
    const text_pull<TextSource, Allocator>& p)
{
    // In C++17, this function will be able to be implemented in terms of
    // string_view's operator<<
    const auto n = static_cast<std::streamsize>(p.size());
    return detail::formatted_output(
        os, n,
        [b = p.cbegin(), n](auto* sb) {
            return sb->sputn(b, n) == n;
        });
}

template <class TextSource, class... Appendices>
text_pull<TextSource> make_text_pull(TextSource in, Appendices... appendices)
{
    return text_pull<TextSource>(
        std::move(in), std::forward<Appendices>(appendices)...);
}

template <class TextSource, class Allocator, class... Appendices>
text_pull<TextSource, Allocator> make_text_pull(
    std::allocator_arg_t, const Allocator& alloc,
    TextSource in, Appendices... appendices)
{
    return text_pull<TextSource, Allocator>(std::allocator_arg,
        alloc, std::move(in), std::forward<Appendices>(appendices)...);
}

}

#endif
