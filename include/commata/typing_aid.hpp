/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_9F3EFB02_5C6D_449D_A895_01323928E6BC
#define COMMATA_GUARD_9F3EFB02_5C6D_449D_A895_01323928E6BC

#include <functional>
#include <iterator>
#include <string>
#include <type_traits>

namespace commata::detail {

template <class T>
constexpr bool is_std_string_v = false;

template <class... Args>
constexpr bool is_std_string_v<std::basic_string<Args...>> = true;

template <class T>
constexpr bool is_std_string_view_v = false;

template <class... Args>
constexpr bool is_std_string_view_v<std::basic_string_view<Args...>> = true;

template <class W>
constexpr bool is_std_reference_wrapper_v = false;

template <class T>
constexpr bool is_std_reference_wrapper_v<std::reference_wrapper<T>> = true;

template <class... Ts>
struct first;

template <>
struct first<>
{
    using type = void;
};

template <class Head, class... Tail>
struct first<Head, Tail...>
{
    using type = Head;
};

template <class... Ts>
using first_t = typename first<Ts...>::type;

template <class E, class Category>
struct is_range_accessible_impl
{
    template <class T>
    static auto check(T*) ->
        decltype(
            std::declval<bool&>() = std::begin(std::declval<const T&>()) !=
                                    std::end  (std::declval<const T&>()),
            std::bool_constant<
                std::is_base_of_v<
                    Category,
                    typename std::iterator_traits<
                        decltype(std::begin(std::declval<const T&>()))>
                            ::iterator_category>
             && std::is_convertible_v<
                    typename std::iterator_traits<
                        decltype(std::begin(std::declval<const T&>()))>
                            ::value_type,
                    E>>());

    template <class T>
    static auto check(...)->std::false_type;
};

template <class T, class E, class Category>
constexpr bool is_range_accessible_v =
    decltype(is_range_accessible_impl<E, Category>::
        template check<T>(nullptr))::value;

}

#endif
