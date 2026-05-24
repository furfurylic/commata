/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_0B1B1402_0D16_4F1B_857F_215F91887B64
#define COMMATA_GUARD_0B1B1402_0D16_4F1B_857F_215F91887B64

#include <algorithm>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace commata::detail {

template <std::size_t I, bool IsL, class F, class... Tuples>
constexpr decltype(auto) apply(F f, Tuples&&... t)
{
    if constexpr (IsL) {
        return std::invoke(f,
                           std::get<I>(std::forward<Tuples>(t))...);
    } else {
        return std::invoke(std::move(f),
                           std::get<I>(std::forward<Tuples>(t))...);
    }
}

template <std::size_t... Is, class F, class... Tuples>
constexpr auto transform_impl(std::index_sequence<Is...>,
    [[maybe_unused]] F&& f, [[maybe_unused]] Tuples&&... ts)
{
    constexpr bool IsL = std::is_lvalue_reference_v<F>;
    using r_t = std::tuple<decltype(
        apply<Is, IsL>(f, std::declval<Tuples>()...))...>;
    return r_t(apply<Is, IsL>(f, std::forward<Tuples>(ts)...)...);
        // Multiple forwarding of the elements of ts would not occur
        // because "apply" forwards only one element pointed by "I"
}

template <class F, class... Tuples>
constexpr auto transform(F&& f, Tuples&&... ts)
{
    constexpr auto size_min =
        std::min({std::tuple_size_v<std::decay_t<Tuples>>...});
    constexpr auto size_max =
        std::max({std::tuple_size_v<std::decay_t<Tuples>>...});
    static_assert(size_min == size_max, "Inconsistent tuple sizes");
    return transform_impl(std::make_index_sequence<size_min>(),
                          std::forward<F>(f), std::forward<Tuples>(ts)...);
}

}

#endif
