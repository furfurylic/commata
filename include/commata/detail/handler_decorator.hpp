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

#include "full_ebo.hpp"

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
    template <class Ch>
    auto start_buffer(Ch* buffer_begin, Ch* buffer_end)
     -> std::enable_if_t<
            std::is_same_v<
                std::remove_const_t<typename Handler::char_type>,
                std::remove_const_t<Ch>>
         && std::is_convertible_v<Ch*, typename Handler::char_type*>,
            decltype(std::declval<Handler&>()
                        .start_buffer(buffer_begin, buffer_end))>
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
    template <class Ch>
    auto end_buffer(Ch* buffer_end)
     -> std::enable_if_t<
            std::is_same_v<
                std::remove_const_t<typename Handler::char_type>,
                std::remove_const_t<Ch>>
         && std::is_convertible_v<Ch*, typename Handler::char_type*>,
            decltype(std::declval<Handler&>().end_buffer(buffer_end))>
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
    template <class Ch>
    auto empty_physical_line(Ch* where)
     -> std::enable_if_t<
            std::is_same_v<
                std::remove_const_t<typename Handler::char_type>,
                std::remove_const_t<Ch>>
         && std::is_convertible_v<Ch*, typename Handler::char_type*>,
            decltype(std::declval<Handler&>().empty_physical_line(where))>
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

template <class Handler, class D>
class handler_core_t
{
    template <class Ch>
    static constexpr bool enables_on =
        std::is_same_v<
            std::remove_const_t<typename Handler::char_type>,
            std::remove_const_t<Ch>>
     && std::is_convertible_v<Ch*, typename Handler::char_type*>;

public:
    template <class Ch>
    auto start_record(Ch* record_begin)
     -> std::enable_if_t<enables_on<Ch>,
            decltype(std::declval<Handler&>().start_record(record_begin))>
    {
        return static_cast<D*>(this)->base().start_record(record_begin);
    }

    template <class Ch>
    auto end_record(Ch* record_end)
     -> std::enable_if_t<enables_on<Ch>,
            decltype(std::declval<Handler&>().end_record(record_end))>
    {
        return static_cast<D*>(this)->base().end_record(record_end);
    }

    template <class Ch>
    auto update(Ch* first, Ch* last)
     -> std::enable_if_t<enables_on<Ch>,
            decltype(std::declval<Handler&>().update(first, last))>
    {
        return static_cast<D*>(this)->base().update(first, last);
    }

    template <class Ch>
    auto finalize(Ch* first, Ch* last)
     -> std::enable_if_t<enables_on<Ch>,
            decltype(std::declval<Handler&>().finalize(first, last))>
    {
        return static_cast<D*>(this)->base().finalize(first, last);
    }
};

// handler_decorator forwards all invocations on TableHandler requirements
// to Handler's member functions with corresponding names; it does not expose
// any excess member functions
template <class Handler, class D>
struct COMMATA_FULL_EBO handler_decorator :
    get_buffer_t<Handler, D>, release_buffer_t<Handler, D>,
    start_buffer_t<Handler, D>, end_buffer_t<Handler, D>,
    empty_physical_line_t<Handler, D>, handler_core_t<Handler, D>,
    yield_t<Handler, D>, yield_location_t<Handler, D>,
    handle_exception_t<Handler, D>
{
    using char_type = typename Handler::char_type;
};

}

#endif
