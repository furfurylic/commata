/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_2501F2DC_A59A_46EC_ABF7_C898072C318D
#define COMMATA_GUARD_2501F2DC_A59A_46EC_ABF7_C898072C318D

#include <compare>
#include <string_view>
#include <type_traits>

namespace commata::detail {

namespace string_value {

struct has_comparison_category_impl
{
    template <class Tr>
    static auto check(Tr*) ->
        decltype(std::declval<typename Tr::comparison_category>(),
            std::true_type());

    template <class Tr>
    static auto check(...) -> std::false_type;
};

template <class Tr>
constexpr bool has_comparison_category_v =
    decltype(has_comparison_category_impl::check<Tr>(nullptr))::value;

template <class Tr, class = void>
struct comparison_category
{
    using type = std::weak_ordering;
};

template <class Tr>
struct comparison_category<Tr, std::enable_if_t<has_comparison_category_v<Tr>>>
{
    using type = typename Tr::comparison_category;
};

template <class Tr>
using comparison_category_t = typename comparison_category<Tr>::type;

template <class T>
using view_t = std::basic_string_view<
    std::remove_const_t<typename T::value_type>, typename T::traits_type>;

}

template <class Ch, class Tr, class T>
constexpr bool is_comparable_with_string_value_v =
    std::is_convertible_v<T, std::basic_string_view<Ch, Tr>>;

template <class T, class U>
auto string_value_eq(
    const T& left,
    const U& right)
    noexcept(noexcept(string_value::view_t<T>(left))
          && noexcept(string_value::view_t<U>(right)))
 -> std::enable_if_t<
        std::is_convertible_v<const T&, string_value::view_t<T>>
     && std::is_convertible_v<const U&, string_value::view_t<U>>, bool>
{
    return string_value::view_t<T>(left) == string_value::view_t<U>(right);
}

template <class T>
auto string_value_eq(
    const T& left,
    const typename T::value_type* right)
 -> std::enable_if_t<
        std::is_convertible_v<const T&, string_value::view_t<T>>, bool>
{
    std::basic_string_view<std::remove_const_t<typename T::value_type>>
        lv = left;
    const auto lve = lv.cend();

    // If left is "abc\0def" and right is "abc" followed by '\0'
    // then left == right shall be false
    // and any overrun on right must not occur
    using tr_t = typename T::traits_type;
    auto i = lv.cbegin();
    while (!tr_t::eq(*right, typename T::value_type())) {
        if ((i == lve) || !tr_t::eq(*i, *right)) {
            return false;
        }
        ++i;
        ++right;
    }
    return i == lve;
}

template <class T, class U>
auto string_value_comp(
    const T& left,
    const U& right)
    noexcept(noexcept(string_value::view_t<T>(left))
          && noexcept(string_value::view_t<U>(right)))
 -> std::enable_if_t<
        std::is_convertible_v<const T&, string_value::view_t<T>>
     && std::is_convertible_v<const U&, string_value::view_t<U>>,
        decltype(std::declval<string_value::view_t<T>>()
             <=> std::declval<string_value::view_t<U>>())>
{
    return string_value::view_t<T>(left) <=> string_value::view_t<T>(right);
}

template <class T>
auto string_value_comp(
    const T& left,
    const typename T::value_type* right)
 -> std::enable_if_t<
        std::is_convertible_v<const T&, string_value::view_t<T>>,
        string_value::comparison_category_t<typename T::traits_type>>
{
    using result_t =
        string_value::comparison_category_t<typename T::traits_type>;

    std::basic_string_view<std::remove_const_t<typename T::value_type>>
        lv = left;

    using tr_t = typename T::traits_type;
    for (auto l : lv) {
        const auto r = *right;
        if (tr_t::eq(r, typename T::value_type()) || tr_t::lt(r, l)) {
            return result_t::greater;
        } else if (tr_t::lt(l, r)) {
            return result_t::less;
        }
        ++right;
    }
    if (tr_t::eq(*right, typename T::value_type())) {
        if constexpr (std::is_same_v<std::strong_ordering, result_t>) {
            return result_t::equal;
        } else {
            return result_t::equivalent;
        }
    } else {
        return result_t::less;
    }
}

}

#endif
