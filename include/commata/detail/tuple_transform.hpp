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

template <std::size_t I, class F, class... Tuples>
constexpr auto apply(F f, Tuples&&... t) {
    return std::invoke(f, std::get<I>(std::forward<Tuples>(t))...);
}

template <std::size_t... Is, class F, class... Tuples>
constexpr auto transform_impl(std::index_sequence<Is...>,
        [[maybe_unused]] F f, [[maybe_unused]] Tuples&&... ts) {
    return std::make_tuple(apply<Is>(f, std::forward<Tuples>(ts)...)...);
}

template <class F, class... Tuples>
constexpr auto transform(F f, Tuples&&... ts) {
    constexpr auto Min =
        std::min({std::tuple_size_v<std::decay_t<Tuples>>...});
    constexpr auto Max =
        std::max({std::tuple_size_v<std::decay_t<Tuples>>...});
    static_assert(Min == Max, "Inconsistent tuple sizes");
    return transform_impl(std::make_index_sequence<Min>(),
                          std::move(f), std::forward<Tuples>(ts)...);
}

}

#endif
