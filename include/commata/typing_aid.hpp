/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_9F3EFB02_5C6D_449D_A895_01323928E6BC
#define COMMATA_GUARD_9F3EFB02_5C6D_449D_A895_01323928E6BC

#include <functional>
#include <string>
#include <type_traits>

namespace commata {
namespace detail {

template <class T>
struct is_std_string :
    std::false_type
{};

template <class... Args>
struct is_std_string<std::basic_string<Args...>> :
    std::true_type
{};

template <class T, class Ch>
struct is_std_string_of :
    std::false_type
{};

template <class Ch, class... Args>
struct is_std_string_of<std::basic_string<Ch, Args...>, Ch> :
    std::true_type
{};

template <class W>
struct is_std_reference_wrapper : std::false_type
{};

template <class T>
struct is_std_reference_wrapper<std::reference_wrapper<T>> : std::true_type
{};

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

}}

#endif
