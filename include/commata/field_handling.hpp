/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_CD40E918_ACCD_4879_BB48_7D9B8B823369
#define COMMATA_GUARD_CD40E918_ACCD_4879_BB48_7D9B8B823369

#include <type_traits>

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

struct replaced_type_impossible_t
{};

struct replaced_type_not_found_t
{};

template <class... Ts>
struct replaced_type_from_impl;

template <class Found>
struct replaced_type_from_impl<Found>
{
    using type = std::conditional_t<
        std::is_same_v<replaced_type_impossible_t, Found>,
        replaced_type_not_found_t, Found>;
};

struct has_common_type_impl
{
    template <class... Ts>
    static std::true_type check(typename std::common_type<Ts...>::type*);

    template <class... Ts>
    static std::false_type check(...);
};

template <class... Ts>
constexpr bool has_common_type_v =
    decltype(has_common_type_impl::check<Ts...>(nullptr))::value;

template <class Found, class Head, class... Tails>
struct replaced_type_from_impl<Found, Head, Tails...>
{
    using type = typename replaced_type_from_impl<
        typename std::conditional<
            std::is_base_of_v<replacement_fail_t, Head>
         || std::is_base_of_v<replacement_ignore_t, Head>,
            std::type_identity<std::type_identity<Found>>,
            typename std::conditional<
                std::is_same_v<Found, replaced_type_not_found_t>,
                std::type_identity<Head>,
                std::conditional_t<
                    has_common_type_v<Found, Head>,
                    std::common_type<Found, Head>,
                    std::type_identity<replaced_type_impossible_t>
                >>>::type::type::type,
        Tails...>::type;
};

template <class... Ts>
using replaced_type_from_t =
    typename replaced_type_from_impl<replaced_type_not_found_t, Ts...>::type;

} // end detail

template <class T, class... As>
decltype(auto) invoke_typing_as(As&&... as)
{
    [[maybe_unused]] constexpr T* n = static_cast<T*>(nullptr);
    if constexpr (std::is_invocable_v<As...>) {
        return std::invoke(std::forward<As>(as)...);
    } else {
        return std::invoke(std::forward<As>(as)..., n);
    }
}

template <class T, class F, class X, class Ch, class... As>
decltype(auto) invoke_with_range_typing_as(
    F&& f, X x, [[maybe_unused]] Ch* first, [[maybe_unused]] Ch* last,
    [[maybe_unused]] As&&... as)
{
    [[maybe_unused]] constexpr T* n = static_cast<T*>(nullptr);
    if constexpr (std::is_invocable_v<F, X, Ch*, Ch*, As...>) {
        return std::invoke(std::forward<F>(f), x, first, last,
                           std::forward<As>(as)...);
    } else if constexpr (std::is_invocable_v<F, X, Ch*, Ch*, As..., T*>) {
        return std::invoke(std::forward<F>(f), x, first, last,
                           std::forward<As>(as)..., n);
    } else if constexpr (std::is_invocable_v<F, X, As...>) {
        return std::invoke(std::forward<F>(f), x, std::forward<As>(as)...);
    } else if constexpr (std::is_invocable_v<F, X, As..., T*>) {
        return std::invoke(std::forward<F>(f), x, std::forward<As>(as)..., n);
    } else if constexpr (std::is_invocable_v<F, X>) {
        return std::invoke(std::forward<F>(f), x);
    } else {
        return std::invoke(std::forward<F>(f), x, n);
    }
}

}

#endif
