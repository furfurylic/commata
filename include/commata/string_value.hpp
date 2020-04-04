/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_2501F2DC_A59A_46EC_ABF7_C898072C318D
#define COMMATA_GUARD_2501F2DC_A59A_46EC_ABF7_C898072C318D

#include <string_view>
#include <type_traits>

namespace commata { namespace detail {

template <class Ch, class Tr, class T>
struct is_comparable_with_string_value :
    std::is_convertible<T, std::basic_string_view<Ch, Tr>>
{};

template <class T, class U>
auto string_value_eq(
    const T& left,
    const U& right)
    noexcept(noexcept(static_cast<std::basic_string_view<
                std::remove_const_t<typename T::value_type>>>(left))
          && noexcept(static_cast<std::basic_string_view<
                std::remove_const_t<typename U::value_type>>>(right)))
 -> std::enable_if_t<
        std::is_convertible_v<const T&, std::basic_string_view<
            std::remove_const_t<typename T::value_type>>>
     && std::is_convertible_v<const U&, std::basic_string_view<
            std::remove_const_t<typename T::value_type>>>, bool>
{
    return static_cast<std::basic_string_view<
                std::remove_const_t<typename T::value_type>,
                typename T::traits_type>>(left)
        == static_cast<std::basic_string_view<
                std::remove_const_t<typename U::value_type>,
                typename U::traits_type>>(right);
}

template <class T>
auto string_value_eq(
    const T& left,
    const typename T::value_type* right)
 -> std::enable_if_t<
        std::is_convertible_v<const T&, std::basic_string_view<
            std::remove_const_t<typename T::value_type>>>, bool>
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
bool string_value_eq(
    const typename T::value_type* left,
    const T& right)
{
    return string_value_eq(right, left);
}

template <class T, class U>
auto string_value_lt(
    const T& left,
    const U& right)
    noexcept(noexcept(static_cast<std::basic_string_view<
                std::remove_const_t<typename T::value_type>>>(left))
          && noexcept(static_cast<std::basic_string_view<
                std::remove_const_t<typename U::value_type>>>(right)))
 -> std::enable_if_t<
        std::is_convertible_v<const T&, std::basic_string_view<
            std::remove_const_t<typename T::value_type>>>
     && std::is_convertible_v<const U&, std::basic_string_view<
            std::remove_const_t<typename T::value_type>>>, bool>
{
    return static_cast<std::basic_string_view<
                std::remove_const_t<typename T::value_type>,
                typename T::traits_type>>(left)
         < static_cast<std::basic_string_view<
                std::remove_const_t<typename U::value_type>,
                typename U::traits_type>>(right);
}

template <class T>
auto string_value_lt(
    const T& left,
    const typename T::value_type* right)
 -> std::enable_if_t<
        std::is_convertible_v<const T&, std::basic_string_view<
            std::remove_const_t<typename T::value_type>>>, bool>
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
        std::is_convertible_v<const T&, std::basic_string_view<
            std::remove_const_t<typename T::value_type>>>, bool>
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

template <class T, class Ch, class Tr, class Allocator>
std::basic_string<Ch, Tr, Allocator>& string_value_plus_assign(
    std::basic_string<Ch, Tr, Allocator>& left,
    const T& right)
{
    const auto ln = left.size();
    try {
        left.append(right.size(), typename T::value_type());    // throw 
    } catch (...) {
        // gcc 7.3.1 dislikes const_iterator here
        left.erase(left.begin() + ln, left.end());
        throw;
    }
    T::traits_type::copy(&*left.begin() + ln, right.data(), right.size());
    return left;
}

template <class T, class Ch, class Tr, class Allocator>
std::basic_string<Ch, Tr, Allocator> string_value_plus(
    const T& left,
    const std::basic_string<Ch, Tr, Allocator>& right)
{
    std::basic_string<Ch, Tr, Allocator> s;
    s.reserve(left.size() + right.size());  // throw
    s.append(left.cbegin(), left.cend()).append(right.cbegin(), right.cend());
    return s;
}

template <class T, class Ch, class Tr, class Allocator>
std::basic_string<Ch, Tr, Allocator> string_value_plus(
    const T& left,
    std::basic_string<Ch, Tr, Allocator>&& right)
{
    const auto rn = right.size();
    try {
        right.append(left.size(), typename T::value_type());    // throw
    } catch (...) {
        // gcc 7.3.1 dislikes const_iterator here
        right.erase(right.begin() + rn, right.end());
        throw;
    }
    using tr_t = typename T::traits_type;
    tr_t::move(&*right.begin() + left.size(), right.data(), rn);
    tr_t::copy(&*right.begin(), left.data(), left.size());
    return std::move(right);
}

template <class T, class Ch, class Tr, class Allocator>
std::basic_string<Ch, Tr, Allocator> string_value_plus(
    const std::basic_string<Ch, Tr, Allocator>& left,
    const T& right)
{
    std::basic_string<Ch, Tr, Allocator> s;
    s.reserve(left.size() + right.size());  // throw
    s.append(left.cbegin(), left.cend()).append(right.cbegin(), right.cend());
    return s;
}

template <class T, class Ch, class Tr, class Allocator>
std::basic_string<Ch, Tr, Allocator> string_value_plus(
    std::basic_string<Ch, Tr, Allocator>&& left,
    const T& right)
{
    return std::move(string_value_plus_assign(left, right));
}

}}

#endif
