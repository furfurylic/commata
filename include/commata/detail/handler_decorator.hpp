/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_9BB11E06_54C5_4E51_BB52_0E8ACAA3146E
#define COMMATA_GUARD_9BB11E06_54C5_4E51_BB52_0E8ACAA3146E

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace commata::detail {

namespace handler_decoration {

struct has_get_buffer_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<std::pair<std::remove_const_t<typename T::char_type>*,
                               std::size_t>&>() =
            std::declval<T&>().get_buffer(),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

struct has_release_buffer_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<T&>().release_buffer(
            std::declval<std::remove_const_t<typename T::char_type>*>()),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

struct has_start_buffer_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<T&>().start_buffer(
            std::declval<typename T::char_type*>(),
            std::declval<typename T::char_type*>()),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

struct has_end_buffer_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<T&>().end_buffer(
            std::declval<typename T::char_type*>()),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

struct has_empty_physical_line_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<T&>().empty_physical_line(
            std::declval<typename T::char_type*>()),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

struct has_yield_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<bool&>() =
            std::declval<T&>().yield(std::declval<std::size_t>()),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

struct has_yield_location_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<std::size_t&>() =
            std::declval<const T&>().yield_location(),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

struct has_handle_exception_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<T&>().handle_exception(),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

} // end handler_decoration

template <class T>
constexpr bool has_get_buffer_v =
    decltype(handler_decoration::has_get_buffer_impl::check<T>(nullptr))();

template <class Handler, class D, class = void>
struct get_buffer_t
{};

template <class Handler, class D>
struct get_buffer_t<Handler, D,
    std::enable_if_t<has_get_buffer_v<Handler>>>
{
    [[nodiscard]]
    auto get_buffer() -> decltype(std::declval<Handler&>().get_buffer())
    {
        return static_cast<D*>(this)->base().get_buffer();
    }
};

template <class T>
constexpr bool has_release_buffer_v =
    decltype(handler_decoration::has_release_buffer_impl::check<T>(nullptr))();

template <class Handler, class D, class = void>
struct release_buffer_t
{};

template <class Handler, class D>
struct release_buffer_t<Handler, D,
    std::enable_if_t<has_release_buffer_v<Handler>>>
{
    auto release_buffer(typename Handler::char_type* buffer)
     -> decltype(std::declval<Handler&>().release_buffer(buffer))
    {
        return static_cast<D*>(this)->base().release_buffer(buffer);
    }
};

template <class T>
constexpr bool has_start_buffer_v =
    decltype(handler_decoration::has_start_buffer_impl::check<T>(nullptr))();

template <class Handler, class D, class = void>
struct start_buffer_t
{};

template <class Handler, class D>
struct start_buffer_t<Handler, D,
    std::enable_if_t<has_start_buffer_v<Handler>>>
{
    auto start_buffer(
        typename Handler::char_type* buffer_begin,
        typename Handler::char_type* buffer_end)
     -> decltype(std::declval<Handler&>()
                    .start_buffer(buffer_begin, buffer_end))
    {
        return static_cast<D*>(this)->base().start_buffer(
            buffer_begin, buffer_end);
    }
};

template <class T>
constexpr bool has_end_buffer_v =
    decltype(handler_decoration::has_end_buffer_impl::check<T>(nullptr))();

template <class Handler, class D, class = void>
struct end_buffer_t
{};

template <class Handler, class D>
struct end_buffer_t<Handler, D,
    std::enable_if_t<has_end_buffer_v<Handler>>>
{
    auto end_buffer(typename Handler::char_type* buffer_end)
     -> decltype(std::declval<Handler&>().end_buffer(buffer_end))
    {
        return static_cast<D*>(this)->base().end_buffer(buffer_end);
    }
};

template <class T>
constexpr bool has_empty_physical_line_v =
    decltype(handler_decoration::has_empty_physical_line_impl::
        check<T>(nullptr))();

template <class Handler, class D, class = void>
struct empty_physical_line_t
{};

template <class Handler, class D>
struct empty_physical_line_t<Handler, D,
    std::enable_if_t<has_empty_physical_line_v<Handler>>>
{
    auto empty_physical_line(typename Handler::char_type* where)
     -> decltype(std::declval<Handler&>().empty_physical_line(where))
    {
        return static_cast<D*>(this)->base().empty_physical_line(where);
    }
};

template <class T>
constexpr bool has_yield_v =
    decltype(handler_decoration::has_yield_impl::check<T>(nullptr))();

template <class Handler, class D, class = void>
struct yield_t
{};

template <class Handler, class D>
struct yield_t<Handler, D, std::enable_if_t<has_yield_v<Handler>>>
{
    auto yield(std::size_t p)
     -> decltype(std::declval<Handler&>().yield(p))
    {
        return static_cast<D*>(this)->base().yield(p);
    }
};

template <class T>
constexpr bool has_yield_location_v =
    decltype(handler_decoration::has_yield_location_impl::check<T>(nullptr))();

template <class Handler, class D, class = void>
struct yield_location_t
{};

template <class Handler, class D>
struct yield_location_t<Handler, D,
    std::enable_if_t<has_yield_location_v<Handler>>>
{
    auto yield_location() const
     -> decltype(std::declval<const Handler&>().yield_location())
    {
        return static_cast<const D*>(this)->base().yield_location();
    }
};

template <class T>
constexpr bool has_handle_exception_v = decltype(
    handler_decoration::has_handle_exception_impl::check<T>(nullptr))();

template <class Handler, class D, class = void>
struct handle_exception_t
{};

template <class Handler, class D>
struct handle_exception_t<Handler, D,
    std::enable_if_t<has_handle_exception_v<Handler>>>
{
    auto handle_exception()
     -> decltype(std::declval<Handler&>().handle_exception())
    {
        return static_cast<D*>(this)->base().handle_exception();
    }
};

// handler_decorator forwards all invocations on TextHandler requirements
// to base()'s member functions with corresponding names; it does not expose
// any excess member functions that Handler does not expose
template <class Handler, class D>
struct handler_decorator :
    get_buffer_t<Handler, D>, release_buffer_t<Handler, D>,
    start_buffer_t<Handler, D>, end_buffer_t<Handler, D>,
    empty_physical_line_t<Handler, D>,
    yield_t<Handler, D>, yield_location_t<Handler, D>,
    handle_exception_t<Handler, D>
{
    using char_type = typename Handler::char_type;

    auto start_record(char_type* record_begin)
     -> decltype(std::declval<Handler&>().start_record(record_begin))
    {
        return static_cast<D*>(this)->base().start_record(record_begin);
    }

    auto update(char_type* first, char_type* last)
     -> decltype(std::declval<Handler&>().update(first, last))
    {
        return static_cast<D*>(this)->base().update(first, last);
    }

    auto finalize(char_type* first, char_type* last)
     -> decltype(std::declval<Handler&>().finalize(first, last))
    {
        return static_cast<D*>(this)->base().finalize(first, last);
    }

    auto end_record(char_type* end)
     -> decltype(std::declval<Handler&>().end_record(end))
    {
        return static_cast<D*>(this)->base().end_record(end);
    }
};

}

#endif
