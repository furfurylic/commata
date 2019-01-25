/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_9BB11E06_54C5_4E51_BB52_0E8ACAA3146E
#define COMMATA_GUARD_9BB11E06_54C5_4E51_BB52_0E8ACAA3146E

#include <cstddef>
#include <type_traits>
#include <utility>

namespace commata {
namespace detail {

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

template <class Handler, class D, class = void>
struct get_buffer_t
{};

template <class Handler, class D>
struct get_buffer_t<Handler, D,
    std::enable_if_t<has_get_buffer<Handler>::value>>
{
    std::pair<typename Handler::char_type*, std::size_t> get_buffer()
    {
        return static_cast<D*>(this)->base().get_buffer();
    }
};

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

template <class Handler, class D, class = void>
struct release_buffer_t
{};

template <class Handler, class D>
struct release_buffer_t<Handler, D,
    std::enable_if_t<has_release_buffer<Handler>::value>>
{
    void release_buffer(const typename Handler::char_type* buffer) noexcept
    {
        static_cast<D*>(this)->base().release_buffer(buffer);
    }
};

struct has_start_buffer_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<T&>().start_buffer(
            std::declval<const typename T::char_type*>(),
            std::declval<const typename T::char_type*>()),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

template <class T>
struct has_start_buffer :
    decltype(has_start_buffer_impl::check<T>(nullptr))
{};

template <class Handler, class D, class = void>
struct start_buffer_t
{};

template <class Handler, class D>
struct start_buffer_t<Handler, D,
    std::enable_if_t<has_start_buffer<Handler>::value>>
{
    void start_buffer(
        const typename Handler::char_type* buffer_begin,
        const typename Handler::char_type* buffer_end)
    {
        static_cast<D*>(this)->base().start_buffer(buffer_begin, buffer_end);
    }
};

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

template <class Handler, class D, class = void>
struct end_buffer_t
{};

template <class Handler, class D>
struct end_buffer_t<Handler, D,
    std::enable_if_t<has_end_buffer<Handler>::value>>
{
    void end_buffer(const typename Handler::char_type* buffer_end)
    {
        static_cast<D*>(this)->base().end_buffer(buffer_end);
    }
};

struct has_empty_physical_line_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<T&>().empty_physical_line(
            std::declval<const typename T::char_type*>()),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

template <class T>
struct has_empty_physical_line :
    decltype(has_empty_physical_line_impl::check<T>(nullptr))
{};

template <class Handler, class D, class = void>
struct empty_physical_line_t
{};

template <class Handler, class D>
struct empty_physical_line_t<Handler, D,
    std::enable_if_t<has_empty_physical_line<Handler>::value>>
{
    auto empty_physical_line(const typename Handler::char_type* where)
    {
        return static_cast<D*>(this)->base().empty_physical_line(where);
    }
};

template <class Handler, class D>
struct handler_decorator :
    get_buffer_t<Handler, D>, release_buffer_t<Handler, D>,
    start_buffer_t<Handler, D>, end_buffer_t<Handler, D>,
    empty_physical_line_t<Handler, D>
{
    using char_type = typename Handler::char_type;

    auto start_record(const char_type* record_begin)
    {
        return static_cast<D*>(this)->base().start_record(record_begin);
    }

    auto update(const char_type* first, const char_type* last)
    {
        return static_cast<D*>(this)->base().update(first, last);
    }

    auto finalize(const char_type* first, const char_type* last)
    {
        return static_cast<D*>(this)->base().finalize(first, last);
    }

    auto end_record(const char_type* end)
    {
        return static_cast<D*>(this)->base().end_record(end);
    }
};

template <class Handler>
class wrapper_handler :
    public handler_decorator<Handler, wrapper_handler<Handler>>
{
    Handler* handler_;

public:
    explicit wrapper_handler(Handler& handler) noexcept :
        handler_(&handler)
    {}

    Handler& base() const noexcept
    {
        return *handler_;
    }
};

}}

#endif
