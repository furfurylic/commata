/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_43D98002_9D9B_407A_9017_9020D03E7A46
#define COMMATA_GUARD_43D98002_9D9B_407A_9017_9020D03E7A46

#include <cstddef>
#include <memory>
#include <type_traits>

#include "buffer_size.hpp"
#include "handler_decorator.hpp"
#include "member_like_base.hpp"

namespace commata::detail {

template <class T>
constexpr bool is_with_buffer_control_v =
    has_get_buffer_v<T> && has_release_buffer_v<T>;

template <class T>
constexpr bool is_without_buffer_control_v =
    (!has_get_buffer_v<T>) && (!has_release_buffer_v<T>);

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
        buffer_size_(detail::sanitize_buffer_size(buffer_size, this->get())),
        buffer_()
    {}

    default_buffer_control(default_buffer_control&& other) noexcept :
        detail::member_like_base<Allocator>(std::move(other.get())),
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
        const typename Handler::char_type* buffer, Handler* f)
    {
        return f->release_buffer(buffer);
    }
};

template <class Handler>
constexpr bool is_full_fledged_v =
    is_with_buffer_control_v<Handler>
 && has_start_buffer_v<Handler> && has_end_buffer_v<Handler>
 && has_empty_physical_line_v<Handler>;

// noexcept-ness of the member functions except the ctor and the dtor does not
// count because they are invoked as parts of a willingly-throwing operation,
// so we do not specify "noexcept" to the member functions except the ctor and
// the dtor
template <class Handler, class BufferControl>
class full_fledged_handler :
    BufferControl,
    public yield_t<Handler, full_fledged_handler<Handler, BufferControl>>,
    public yield_location_t<Handler,
        full_fledged_handler<Handler, BufferControl>>
{
    static_assert(!std::is_reference_v<Handler>);
    static_assert(!is_full_fledged_v<Handler>);

    Handler handler_;

public:
    using char_type = typename Handler::char_type;

    template <class HandlerR, class BufferControlR = BufferControl,
        std::enable_if_t<
            std::is_constructible_v<Handler, HandlerR&&>
         && std::is_constructible_v<BufferControl, BufferControlR&&>
         && !std::is_base_of_v<full_fledged_handler,
                               std::decay_t<HandlerR>>>* = nullptr>
    explicit full_fledged_handler(HandlerR&& handler,
        BufferControlR&& buffer_engine = BufferControl())
        noexcept(std::is_nothrow_constructible_v<Handler, HandlerR&&>) :
        BufferControl(std::forward<BufferControlR>(buffer_engine)),
        handler_(std::forward<HandlerR>(handler))
    {}

    [[nodiscard]] std::pair<char_type*, std::size_t> get_buffer()
    {
        return this->do_get_buffer(std::addressof(handler_));
    }

    void release_buffer(const char_type* buffer)
    {
        this->do_release_buffer(buffer, std::addressof(handler_));
    }

    void start_buffer(
        [[maybe_unused]] const char_type* buffer_begin,
        [[maybe_unused]] const char_type* buffer_end)
    {
        if constexpr (has_start_buffer_v<Handler>) {
            handler_.start_buffer(buffer_begin, buffer_end);
        }
    }

    void end_buffer([[maybe_unused]] const char_type* buffer_end)
    {
        if constexpr (has_end_buffer_v<Handler>) {
            handler_.end_buffer(buffer_end);
        }
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

    auto empty_physical_line([[maybe_unused]] const char_type* where)
    {
        if constexpr (has_empty_physical_line_v<Handler>) {
            return handler_.empty_physical_line(where);
        }
    }

    Handler& base() noexcept
    {
        return handler_;
    }

    const Handler& base() const noexcept
    {
        return handler_;
    }
};

}

#endif
