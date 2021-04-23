/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_9F3EFB02_5C6D_449D_A895_01323928E6BC
#define COMMATA_GUARD_9F3EFB02_5C6D_449D_A895_01323928E6BC

#include <functional>
#include <string>
#include <type_traits>

namespace commata::detail {

template <class T>
constexpr bool is_std_string_v = false;

template <class... Args>
constexpr bool is_std_string_v<std::basic_string<Args...>> = true;

template <class T, class Ch>
constexpr bool is_std_string_of_ch_v = false;

template <class Ch, class... Args>
constexpr bool is_std_string_of_ch_v<std::basic_string<Ch, Args...>,
                                     Ch> = true;

template <class T, class Ch, class Tr>
constexpr bool is_std_string_of_ch_tr_v = false;

template <class Ch, class Tr, class... Args>
constexpr bool is_std_string_of_ch_tr_v<std::basic_string<Ch, Tr, Args...>,
                                        Ch, Tr> = true;

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

}

#endif
