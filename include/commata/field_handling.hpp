/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_CD40E918_ACCD_4879_BB48_7D9B8B823369
#define COMMATA_GUARD_CD40E918_ACCD_4879_BB48_7D9B8B823369

#include <type_traits>
#include <functional>

namespace commata {

struct replacement_ignore_t {};
inline constexpr replacement_ignore_t replacement_ignore{};

struct replacement_fail_t {};
inline constexpr replacement_fail_t replacement_fail{};

namespace detail {

enum class replace_mode
{
    replace,
    fail,
    ignore
};

struct generic_args_t {};

namespace field {

// Proxy for replacement_ignore_t and replacement_fail_t on std::common_type_t
struct replacing_no_info_t
{
    template <class T> operator T() const;
};

template <class T>
using replacing_t =
    std::conditional_t<
        std::is_base_of_v<replacement_ignore_t, T>
     || std::is_base_of_v<replacement_fail_t, T>,
        replacing_no_info_t,
        T>;

template <class... Ts>
using replacing_common_type_t = std::common_type_t<replacing_t<Ts>...>;

} // end field

struct replaced_type_not_found_t
{};

template <class... Ts>
using replaced_type_from_t =
    std::conditional_t<
        std::is_same_v<
            field::replacing_no_info_t,
            field::replacing_common_type_t<Ts...>>,
        replaced_type_not_found_t,
        field::replacing_common_type_t<Ts...>>;

} // end detail

template <class T, class... As>
decltype(auto) invoke_typing_as(As&&... as)
{
    [[maybe_unused]] constexpr T* n = static_cast<T*>(nullptr);
    if constexpr (std::is_invocable_v<As..., T*>) {
        return std::invoke(std::forward<As>(as)..., n);
    } else {
        return std::invoke(std::forward<As>(as)...);
    }
}

template <class T, class F, class X, class Ch, class... As>
decltype(auto) invoke_with_range_typing_as(
    F&& f, X x, [[maybe_unused]] Ch* first, [[maybe_unused]] Ch* last,
    [[maybe_unused]] As&&... as)
{
    [[maybe_unused]] constexpr T* n = static_cast<T*>(nullptr);
    if constexpr (std::is_invocable_v<F, X, Ch*, Ch*, As..., T*>) {
        return std::invoke(std::forward<F>(f), x, first, last,
                           std::forward<As>(as)..., n);
    } else if constexpr (std::is_invocable_v<F, X, As..., T*>) {
        return std::invoke(std::forward<F>(f), x, std::forward<As>(as)..., n);
    } else if constexpr (std::is_invocable_v<F, X, T*>) {
        return std::invoke(std::forward<F>(f), x, n);
    } else if constexpr (std::is_invocable_v<F, X, Ch*, Ch*, As...>) {
        return std::invoke(std::forward<F>(f), x, first, last,
                           std::forward<As>(as)...);
    } else if constexpr (std::is_invocable_v<F, X, As...>) {
        return std::invoke(std::forward<F>(f), x, std::forward<As>(as)...);
    } else {
        return std::invoke(std::forward<F>(f), x);
    }
}

}

#endif
