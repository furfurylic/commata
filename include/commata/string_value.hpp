/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_2501F2DC_A59A_46EC_ABF7_C898072C318D
#define COMMATA_GUARD_2501F2DC_A59A_46EC_ABF7_C898072C318D

#include <string>
#include <type_traits>

namespace commata { namespace detail {

template <class T, class Ch, class Tr>
struct is_std_string_of2 :
    std::false_type
{};

template <class Ch, class Tr, class... Args>
struct is_std_string_of2<std::basic_string<Ch, Tr, Args...>, Ch, Tr> :
    std::true_type
{};

template <class Ch, class Tr, class T>
struct is_comparable_with_string_value :
    std::integral_constant<bool,
        std::is_convertible<T, const Ch*>::value
     || detail::is_std_string_of2<T, Ch, Tr>::value>
{};

template <class T, class U>
bool string_value_eq(
    const T& left,
    const U& right) noexcept
{
    static_assert(std::is_same<typename T::traits_type,
                               typename U::traits_type>::value, "");
    using tr_t = typename T::traits_type;
    return (left.size() == right.size())
        && (tr_t::compare(left.data(), right.data(), left.size()) == 0);
}

template <class T>
bool string_value_eq(
    const T& left,
    const typename T::value_type* right) noexcept
{
    const auto le = left.cend();

    // If left is "abc\0def" and right is "abc" followed by '\0'
    // then left == right shall be false
    // and any overrun on right must not occur
    using tr_t = typename T::traits_type;
    auto i = left.cbegin();
    while (!tr_t::eq(*right, typename T::value_type())) {
        if ((i == le) || !tr_t::eq(*i, *right)) {
            return false;
        }
        ++i;
        ++right;
    }
    return i == le;
}

template <class T>
bool string_value_eq(
    const typename T::value_type* left,
    const T& right) noexcept
{
    return string_value_eq(right, left);
}

template <class T, class U>
bool string_value_lt(
    const T& left,
    const U& right) noexcept
{
    static_assert(std::is_same<typename T::traits_type,
                               typename U::traits_type>::value, "");
    using tr_t = typename T::traits_type;
    if (left.size() < right.size()) {
        return tr_t::compare(left.data(), right.data(), left.size()) <= 0;
    } else {
        return tr_t::compare(left.data(), right.data(), right.size()) < 0;
    }
}

template <class T>
bool string_value_lt(
    const T& left,
    const typename T::value_type* right) noexcept
{
    using tr_t = typename T::traits_type;
    for (auto l : left) {
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
bool string_value_lt(
    const typename T::value_type* left,
    const T& right) noexcept
{
    using tr_t = typename T::traits_type;
    for (auto r : right) {
        const auto l = *left;
        if (tr_t::eq(l, typename T::value_type()) || tr_t::lt(l, r)) {
            return true;
        } else if (tr_t::lt(r, l)) {
            return false;
        }
        ++left;
    }
    return false;   // at least left == right
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
        // gcc 6.3 dislikes const_iterator here
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
        // gcc 6.3 dislikes const_iterator here
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
