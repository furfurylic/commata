/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_6D6885A1_6415_4BBB_A161_AA45E94D7448
#define COMMATA_GUARD_6D6885A1_6415_4BBB_A161_AA45E94D7448

#include <iterator>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include "typing_aid.hpp"

namespace commata::detail {

template <class Ch, class Tr, class Allocator>
class string_eq
{
    std::basic_string<Ch, Tr, Allocator> c_;

public:
    explicit string_eq(std::basic_string<Ch, Tr, Allocator> c) :
        c_(std::move(c))
    {}

    template <class T>
    bool operator()(const T& other) const
    {
        return c_ == other;
    }

    friend std::basic_ostream<Ch, Tr>& operator<<(
        std::basic_ostream<Ch, Tr>& os, const string_eq& eq)
    {
        return os << eq.c_;
    }
};

template <class T, class Ch, class Tr>
constexpr bool is_string_pred_v =
    std::is_invocable_r_v<bool, T&, std::basic_string_view<Ch, Tr>>;

template <class Ch, class Tr, class Allocator2, class Allocator>
auto make_string_pred(
    std::basic_string<Ch, Tr, Allocator2>&& s, const Allocator&)
{
    return string_eq(std::move(s));
}

template <class Ch, class Tr, class T, class Allocator>
decltype(auto) make_string_pred(T&& s, const Allocator& alloc)
{
    using string_t = std::basic_string<Ch, Tr, Allocator>;

    if constexpr (std::is_constructible_v<string_t, T, const Allocator&>) {
        // Arrays are captured here
        return string_eq(string_t(std::forward<T>(s), alloc));
    } else if constexpr (
            is_range_accessible_v<std::remove_reference_t<T>, Ch>) {
        auto f = std::cbegin(s);
        auto l = std::cend(s);
        if constexpr (std::is_same_v<decltype(f), decltype(l)>) {
            return string_eq(string_t(f, l, alloc));
        } else {
            string_t str(alloc);
            while (!(f == l)) {
                str.push_back(*f);
                ++f;
            }
            return string_eq(std::move(str));
        }
    } else {
        return std::forward<T>(s);
    }
}

template <class Ch, class Tr, class Allocator>
auto make_string_pred(std::basic_string<Ch, Tr, Allocator> s)
{
    return string_eq(std::move(s));
}

template <class Ch, class Tr, class T>
std::enable_if_t<!is_std_string_v<std::decay_t<T>>,
        decltype(make_string_pred<Ch, Tr>(
            std::declval<T>(), std::declval<std::allocator<Ch>>()))>
    make_string_pred(T&& s)
{
    return make_string_pred<Ch, Tr>(std::forward<T>(s), std::allocator<Ch>());
}

}

#endif
