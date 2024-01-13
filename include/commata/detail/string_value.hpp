/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_2501F2DC_A59A_46EC_ABF7_C898072C318D
#define COMMATA_GUARD_2501F2DC_A59A_46EC_ABF7_C898072C318D

#include <string_view>
#include <type_traits>

namespace commata::detail {

namespace string_value {

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

template <class T>
auto string_value_eq(
    const typename T::value_type* left,
    const T& right)
 -> std::enable_if_t<
        std::is_convertible_v<const T&, string_value::view_t<T>>, bool>
{
    return string_value_eq(right, left);
}

template <class T, class U>
auto string_value_lt(
    const T& left,
    const U& right)
    noexcept(noexcept(string_value::view_t<T>(left))
          && noexcept(string_value::view_t<U>(right)))
 -> std::enable_if_t<
        std::is_convertible_v<const T&, string_value::view_t<T>>
     && std::is_convertible_v<const U&, string_value::view_t<U>>, bool>
{
    return std::basic_string_view<std::remove_const_t<typename T::value_type>,
            typename T::traits_type>(left)
         < std::basic_string_view<std::remove_const_t<typename U::value_type>,
            typename U::traits_type>(right);
}

template <class T>
auto string_value_lt(
    const T& left,
    const typename T::value_type* right)
 -> std::enable_if_t<
        std::is_convertible_v<const T&, string_value::view_t<T>>, bool>
{
    std::basic_string_view<std::remove_const_t<typename T::value_type>>
        lv = left;

    using tr_t = typename T::traits_type;
    for (auto l : lv) {
        const auto r = *right;
        if (tr_t::eq(r, typename T::value_type()) || tr_t::lt(r, l)) {
            return false;
        } else if (tr_t::lt(l, r)) {
            return true;
        }
        ++right;
    }
    return !tr_t::eq(*right, typename T::value_type());
}

template <class T>
auto string_value_lt(
    const typename T::value_type* left,
    const T& right)
 -> std::enable_if_t<
        std::is_convertible_v<const T&, string_value::view_t<T>>, bool>
{
    std::basic_string_view<std::remove_const_t<typename T::value_type>>
        rv = right;

    using tr_t = typename T::traits_type;
    for (auto r : rv) {
        const auto l = *left;
        if (tr_t::eq(l, typename T::value_type()) || tr_t::lt(l, r)) {
            return true;
        } else if (tr_t::lt(r, l)) {
            return false;
        }
        ++left;
    }
    return false;   // at least left == rv
}

}

#endif
