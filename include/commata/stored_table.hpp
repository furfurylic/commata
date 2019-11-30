/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_44AB64F1_C45A_45AC_9277_F2735CCE832E
#define COMMATA_GUARD_44AB64F1_C45A_45AC_9277_F2735CCE832E

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "formatted_output.hpp"
#include "key_chars.hpp"
#include "member_like_base.hpp"
#include "propagation_controlled_allocator.hpp"

namespace commata {

template <class Ch, class Tr = std::char_traits<std::remove_const_t<Ch>>>
class basic_stored_value
{
    Ch* begin_;
    Ch* end_;   // must point the terminating zero

    static Ch empty_value[];

public:
    static_assert(
        std::is_same<
            std::remove_const_t<Ch>,
            typename Tr::char_type>::value,
        "Inconsistent char type and traits type specified "
        "for commata::basic_stored_value");

    using value_type      = Ch;
    using reference       = Ch&;
    using const_reference = const Ch&;
    using pointer         = Ch*;
    using const_pointer   = const Ch*;
    using iterator        = Ch*;
    using const_iterator  = const Ch*;
    using difference_type = std::ptrdiff_t;
    using size_type       = std::size_t;
    using traits_type     = Tr;

    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr size_type npos = static_cast<size_type>(-1);

    basic_stored_value() noexcept :
        basic_stored_value(empty_value, empty_value)
    {}

    basic_stored_value(Ch* begin, Ch* end) :
        begin_(begin), end_(end)
    {
        assert(*end_ == Ch());
    }

    basic_stored_value(const basic_stored_value& other) noexcept = default;
    basic_stored_value& operator=(const basic_stored_value& other) noexcept
        = default;

    template <
        class OtherCh,
        std::enable_if_t<std::is_const<Ch>::value
                      && std::is_same<Ch, const OtherCh>::value>* = nullptr>
    basic_stored_value(const basic_stored_value<OtherCh, Tr>& other)
        noexcept : basic_stored_value(other.begin(), other.end())
    {}

    template <class OtherCh>
    auto operator=(const basic_stored_value<OtherCh, Tr>& other) noexcept
     -> std::enable_if_t<std::is_const<Ch>::value
                      && std::is_same<Ch, const OtherCh>::value,
                            basic_stored_value&>
    {
        return *this = basic_stored_value(other);
    }

    iterator begin() noexcept
    {
        return begin_;
    }

    const_iterator begin() const noexcept
    {
        return begin_;
    }

    iterator end() noexcept
    {
        return end_;
    }

    const_iterator end() const noexcept
    {
        return end_;
    }

    reverse_iterator rbegin() noexcept
    {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    reverse_iterator rend() noexcept
    {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    const_iterator cbegin() const noexcept
    {
        return begin_;
    }

    const_iterator cend() const noexcept
    {
        return end_;
    }

    const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(cend());
    }

    const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(cbegin());
    }

    reference operator[](size_type pos)
    {
        // pos == size() is OK, IAW std::basic_string
        assert(pos <= size());
        return begin()[pos];
    }

    const_reference operator[](size_type pos) const
    {
        // ditto
        assert(pos <= size());
        return begin()[pos];
    }

    reference at(size_type pos)
    {
        // pos == size() is NG, IAW std::basic_string
        if (pos >= size()) {
            throw_pos(pos);     // throw
        }
        return (*this)[pos];
    }

    const_reference at(size_type pos) const
    {
        // ditto
        if (pos >= size()) {
            throw_pos(pos);     // throw
        }
        return (*this)[pos];
    }

    Ch* c_str() noexcept
    {
        return begin_;
    }

    const Ch* c_str() const noexcept
    {
        return begin_;
    }

    Ch* data() noexcept
    {
        return begin_;
    }

    const Ch* data() const noexcept
    {
        return begin_;
    }

    template <class Allocator>
    explicit operator
        std::basic_string<std::remove_const_t<Ch>, Tr, Allocator>() const
    {
        return std::basic_string<std::remove_const_t<Ch>, Tr, Allocator>(
            cbegin(), cend());
    }

    size_type size() const noexcept
    {
        return static_cast<size_type>(end_ - begin_);
    }

    size_type length() const noexcept
    {
        return size();
    }

    bool empty() const noexcept
    {
        return begin_ == end_;
    }

    constexpr size_type max_size() const noexcept
    {
        return npos;
    }

    reference front()
    {
        assert(!empty());
        return *begin_;
    }

    const_reference front() const
    {
        assert(!empty());
        return *begin_;
    }

    reference back()
    {
        assert(!empty());
        return *(end_ - 1);
    }

    const_reference back() const
    {
        assert(!empty());
        return *(end_ - 1);
    }

    void pop_front()
    {
        assert(!empty());
        erase(cbegin());
    }

    void pop_back()
    {
        assert(!empty());
        erase(cend() - 1);
    }

    basic_stored_value& erase(size_type pos = 0, size_type n = npos)
    {
        if (pos > size()) {
            throw_pos(pos);
        }
        const auto xlen = std::min(n, size() - pos);    // length of removal
        const auto first = cbegin() + pos;
        erase(first, first + xlen);
        return *this;
    }

    iterator erase(const_iterator position)
    {
        return erase(position, position + 1);
    }

    iterator erase(const_iterator first, const_iterator last)
    {
        const auto erase_len = last - first;
        if (first == begin_) {
            begin_ += erase_len;
            return begin_;
        } else if (last == end_) {
            end_ -= erase_len;
            *end_ = Ch();
            return end_;
        } else {
            const auto prefix_len = first - begin_;
            const auto postfix_len = end_ - last;
            if (prefix_len <= postfix_len) {
                Tr::move(begin_ + erase_len, begin_, prefix_len);
                begin_ += erase_len;
            } else {
                Tr::move(begin_ + prefix_len, last, postfix_len + 1);
                end_ -= erase_len;
                assert(*end_ == Ch());
            }
            return begin_ + prefix_len;
        }
    }

    void clear() noexcept
    {
        begin_ = end_;
    }

    void swap(basic_stored_value& other) noexcept
    {
        std::swap(begin_, other.begin_);
        std::swap(end_,   other.end_);
    }

private:
    [[noreturn]]
    void throw_pos(size_type pos) const
    {
        std::ostringstream s;
        s << pos << " is too large for this value, whose size is " << size();
        throw std::out_of_range(s.str());
    }
};

template <class Ch, class Tr>
constexpr typename basic_stored_value<Ch, Tr>::size_type
    basic_stored_value<Ch, Tr>::npos;

template <class Ch, class Tr>
Ch basic_stored_value<Ch, Tr>::empty_value[] = { Ch() };

template <class Ch, class Tr,
    class Allocator = std::allocator<std::remove_const_t<Ch>>>
std::basic_string<std::remove_const_t<Ch>, Tr, Allocator>
    to_string(const basic_stored_value<Ch, Tr>& v,
        const Allocator& alloc = Allocator())
{
    return std::basic_string<std::remove_const_t<Ch>, Tr, Allocator>(
        v.cbegin(), v.cend(), alloc);
}

template <class Ch, class Tr>
void swap(
    basic_stored_value<Ch, Tr>& left,
    basic_stored_value<Ch, Tr>& right) noexcept
{
    return left.swap(right);
}

namespace detail {

template <class Left, class Right>
bool stored_value_eq(const Left& left, const Right& right) noexcept
{
    static_assert(std::is_same<typename Left::traits_type,
                               typename Right::traits_type>::value, "");
    using tr_t = typename Left::traits_type;
    return (left.size() == right.size())
        && (tr_t::compare(left.data(), right.data(), left.size()) == 0);
}

template <class Left, class Right>
bool stored_value_lt(const Left& left, const Right& right) noexcept
{
    static_assert(std::is_same<typename Left::traits_type,
                               typename Right::traits_type>::value, "");
    using tr_t = typename Left::traits_type;
    if (left.size() < right.size()) {
        return tr_t::compare(left.data(), right.data(), left.size()) <= 0;
    } else {
        return tr_t::compare(left.data(), right.data(), right.size()) < 0;
    }
}

} // end namespace detail

template <class ChL, class ChR, class Tr>
auto operator==(
    const basic_stored_value<ChL, Tr>& left,
    const basic_stored_value<ChR, Tr>& right) noexcept
 -> std::enable_if_t<
        std::is_same<
            std::remove_const_t<ChL>,
            std::remove_const_t<ChR>>::value, bool>
{
    return detail::stored_value_eq(left, right);
}

template <class Ch, class ChC, class Tr>
auto operator==(
    const basic_stored_value<ChC, Tr>& left,
    const Ch* right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    // If left is "abc\0def" and right is "abc" followed by '\0'
    // then left == right shall be false
    // and any overrun on right must not occur
    auto i = left.cbegin();
    while (!Tr::eq(*right, Ch())) {
        if (!Tr::eq(*i, *right)) {
            return false;
        }
        ++i;
        ++right;
    }
    return i == left.cend();
}

template <class Ch, class ChC, class Tr, class Allocator>
auto operator==(
    const basic_stored_value<ChC, Tr>& left,
    const std::basic_string<Ch, Tr, Allocator>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return detail::stored_value_eq(left, right);
}

template <class Ch, class ChC, class Tr>
auto operator==(
    const Ch* left,
    const basic_stored_value<ChC, Tr>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return right == left;
}

template <class Ch, class ChC, class Tr, class Allocator>
auto operator==(
    const std::basic_string<Ch, Tr, Allocator>& left,
    const basic_stored_value<ChC, Tr>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return right == left;
}

template <class ChL, class ChR, class Tr>
auto operator!=(
    const basic_stored_value<ChL, Tr>& left,
    const basic_stored_value<ChR, Tr>& right) noexcept
 -> std::enable_if_t<
        std::is_same<
            std::remove_const_t<ChL>,
            std::remove_const_t<ChR>>::value, bool>
{
    return !(left == right);
}

template <class Ch, class ChC, class Tr>
auto operator!=(
    const basic_stored_value<ChC, Tr>& left,
    const Ch* right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return !(left == right);
}

template <class Ch, class ChC, class Tr, class Allocator>
auto operator!=(
    const basic_stored_value<ChC, Tr>& left,
    const std::basic_string<Ch, Tr, Allocator>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return !(left == right);
}

template <class Ch, class ChC, class Tr>
auto operator!=(
    const Ch* left,
    const basic_stored_value<ChC, Tr>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return !(left == right);
}

template <class Ch, class ChC, class Tr, class Allocator>
auto operator!=(
    const std::basic_string<Ch, Tr, Allocator>& left,
    const basic_stored_value<ChC, Tr>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return !(left == right);
}

template <class ChL, class ChR, class Tr>
auto operator<(
    const basic_stored_value<ChL, Tr>& left,
    const basic_stored_value<ChR, Tr>& right) noexcept
 -> std::enable_if_t<
        std::is_same<
            std::remove_const_t<ChL>,
            std::remove_const_t<ChR>>::value, bool>
{
    return detail::stored_value_lt(left, right);
}

template <class Ch, class ChC, class Tr>
auto operator<(
    const basic_stored_value<ChC, Tr>& left,
    const Ch* right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    for (auto l : left) {
        const auto r = *right;
        if (Tr::eq(r, Ch()) || Tr::lt(r, l)) {
            return false;
        } else if (Tr::lt(l, r)) {
            return true;
        }
        ++right;
    }
    return !Tr::eq(*right, Ch());
}

template <class Ch, class ChC, class Tr, class Allocator>
auto operator<(
    const basic_stored_value<ChC, Tr>& left,
    const std::basic_string<Ch, Tr, Allocator>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return detail::stored_value_lt(left, right);
}

template <class Ch, class ChC, class Tr>
auto operator<(
    const Ch* left,
    const basic_stored_value<ChC, Tr>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    for (auto r : right) {
        const auto l = *left;
        if (Tr::eq(l, Ch()) || Tr::lt(l, r)) {
            return true;
        } else if (Tr::lt(r, l)) {
            return false;
        }
        ++left;
    }
    return false;   // at least left == right
}

template <class Ch, class ChC, class Tr, class Allocator>
auto operator<(
    const std::basic_string<Ch, Tr, Allocator>& left,
    const basic_stored_value<ChC, Tr>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return detail::stored_value_lt(left, right);
}

template <class ChL, class ChR, class Tr>
auto operator>(
    const basic_stored_value<ChL, Tr>& left,
    const basic_stored_value<ChR, Tr>& right) noexcept
 -> std::enable_if_t<
        std::is_same<
            std::remove_const_t<ChL>,
            std::remove_const_t<ChR>>::value, bool>
{
    return right < left;
}

template <class Ch, class ChC, class Tr>
auto operator>(
    const basic_stored_value<ChC, Tr>& left,
    const Ch* right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return right < left;
}

template <class Ch, class ChC, class Tr, class Allocator>
auto operator>(
    const basic_stored_value<ChC, Tr>& left,
    const std::basic_string<Ch, Tr, Allocator>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return right < left;
}

template <class Ch, class ChC, class Tr>
auto operator>(
    const Ch* left,
    const basic_stored_value<ChC, Tr>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return right < left;
}

template <class Ch, class ChC, class Tr, class Allocator>
auto operator>(
    const std::basic_string<Ch, Tr, Allocator>& left,
    const basic_stored_value<ChC, Tr>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return right < left;
}

template <class ChL, class ChR, class Tr>
auto operator<=(
    const basic_stored_value<ChL, Tr>& left,
    const basic_stored_value<ChR, Tr>& right) noexcept
 -> std::enable_if_t<
        std::is_same<
            std::remove_const_t<ChL>,
            std::remove_const_t<ChR>>::value, bool>
{
    return !(right < left);
}

template <class Ch, class ChC, class Tr>
auto operator<=(
    const basic_stored_value<ChC, Tr>& left,
    const Ch* right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return !(right < left);
}

template <class Ch, class ChC, class Tr, class Allocator>
auto operator<=(
    const basic_stored_value<ChC, Tr>& left,
    const std::basic_string<Ch, Tr, Allocator>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return !(right < left);
}

template <class Ch, class ChC, class Tr>
auto operator<=(
    const Ch* left,
    const basic_stored_value<ChC, Tr>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return !(right < left);
}

template <class Ch, class ChC, class Tr, class Allocator>
auto operator<=(
    const std::basic_string<Ch, Tr, Allocator>& left,
    const basic_stored_value<ChC, Tr>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return !(right < left);
}

template <class ChL, class ChR, class Tr>
auto operator>=(
    const basic_stored_value<ChL, Tr>& left,
    const basic_stored_value<ChR, Tr>& right) noexcept
 -> std::enable_if_t<
        std::is_same<
            std::remove_const_t<ChL>,
            std::remove_const_t<ChR>>::value, bool>
{
    return !(left < right);
}

template <class Ch, class ChC, class Tr>
auto operator>=(
    const basic_stored_value<ChC, Tr>& left,
    const Ch* right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return !(left < right);
}

template <class Ch, class ChC, class Tr, class Allocator>
auto operator>=(
    const basic_stored_value<ChC, Tr>& left,
    const std::basic_string<Ch, Tr, Allocator>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return !(left < right);
}

template <class Ch, class ChC, class Tr>
auto operator>=(
    const Ch* left,
    const basic_stored_value<ChC, Tr>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return !(left < right);
}

template <class Ch, class ChC, class Tr, class Allocator>
auto operator>=(
    const std::basic_string<Ch, Tr, Allocator>& left,
    const basic_stored_value<ChC, Tr>& right) noexcept
 -> std::enable_if_t<std::is_same<std::remove_const_t<ChC>, Ch>::value, bool>
{
    return !(left < right);
}

template <class Ch, class ChC, class Tr>
auto operator<<(
    std::basic_ostream<Ch, Tr>& os, const basic_stored_value<ChC, Tr>& o)
 -> std::enable_if_t<
        std::is_same<std::remove_const_t<ChC>, Ch>::value,
        std::basic_ostream<Ch, Tr>&>
{
    // In C++17, this function will be able to be implemented in terms of
    // string_view's operator<<
    const auto n = static_cast<std::streamsize>(o.size());
    return detail::formatted_output(
        os, n,
        [b = o.cbegin(), n](auto* sb) {
            return sb->sputn(b, n) == n;
        });
}

using stored_value = basic_stored_value<char>;
using wstored_value = basic_stored_value<wchar_t>;
using cstored_value = basic_stored_value<const char>;
using cwstored_value = basic_stored_value<const wchar_t>;

}

namespace std {

template <class Ch, class Tr>
struct hash<commata::basic_stored_value<Ch, Tr>>
{
    std::size_t operator()(
        const commata::basic_stored_value<Ch, Tr>& x) const noexcept
    {
        // We'd like this function to return the same hash value as
        // std::basic_string, but it is impossible, so we borrow
        // Java-String's hash algorithm at this time
        // In C++17, this function should be implemented in terms of
        // string_view's hash
        std::size_t h = 0;
        for (const auto c : x) {
            h *= 31;
            h += static_cast<std::size_t>(c);
        }
        return h;
    }
};

} // end namespace std

namespace commata {

namespace detail {

template <class T>
auto select_allocator(const T&, const T& r, std::true_type)
{
    return r.get_allocator();
}

template <class T>
auto select_allocator(const T& l, const T&, std::false_type)
{
    return l.get_allocator();
}

// Used by table_store; externalized to expunge Allocator template param
// to avoid bloat
template <class Ch>
class store_buffer
{
    Ch* buffer_;
    Ch* hwl_;   // high water level: last-past-one of the used elements
    Ch* end_;   // last-past-one of the buffer_

public:
    store_buffer() noexcept :
        buffer_(nullptr)
    {}

    store_buffer(const store_buffer&) = delete;

    void attach(Ch* buffer, std::size_t size) noexcept
    {
        assert(!buffer_);
        buffer_ = buffer;
        hwl_ = buffer_;
        end_ = buffer_ + size;
    }

    std::pair<Ch*, std::size_t> detach() noexcept
    {
        assert(buffer_);
        std::pair<Ch*, std::size_t> p(buffer_, end_ - buffer_);
        buffer_ = nullptr;
        return p;
    }

    Ch* secured() const noexcept
    {
        return hwl_;
    }

    void secure_upto(Ch* secured_last)
    {
        assert(secured_last <= end_);
        hwl_ = secured_last;
    }

    Ch* secure(std::size_t size) noexcept
    {
        if (size <= static_cast<std::size_t>(end_ - hwl_)) {
            const auto first = hwl_;
            hwl_ += size;
            return first;
        } else {
            return nullptr;
        }
    }

    void clear() noexcept
    {
        hwl_ = buffer_;
    }

    bool is_cleared() noexcept
    {
        return buffer_ == hwl_;
    }

    std::size_t size() noexcept
    {
        return end_ - buffer_;
    }
};

// ditto
template <class Ch>
struct store_node : store_buffer<Ch>
{
    store_node* next;

    explicit store_node(store_node* n) :
        next(n)
    {}
};

template <class Ch, class Allocator>
class table_store :
    member_like_base<Allocator>
{
    using node_type = store_node<Ch>;

    // At this time we adopt a homemade forward list
    // taking advantage of its constant-time and nofail "splice" ability
    // and its nofail move construction and move assignment
    node_type* buffers_;        // "front" of buffers
    node_type* buffers_back_;   // "back" of buffers, whose next is nullptr
    std::size_t buffers_size_;  // "size" of buffers

    node_type* buffers_cleared_;        // "front" of cleared buffers
    node_type* buffers_cleared_back_;   // "back" of cleared buffers,
                                        // whose next is nullptr

private:
    using at_t = std::allocator_traits<Allocator>;
    using pt_t = std::pointer_traits<typename at_t::pointer>;
    using nat_t = typename at_t::template rebind_traits<node_type>;
    using npt_t = std::pointer_traits<typename nat_t::pointer>;

public:
    using allocator_type = Allocator;
    using security = std::vector<Ch*>;

    explicit table_store(
        std::allocator_arg_t = std::allocator_arg,
        const Allocator& alloc = Allocator()) :
        member_like_base<Allocator>(alloc),
        buffers_(nullptr), buffers_back_(nullptr), buffers_size_(0),
        buffers_cleared_(nullptr), buffers_cleared_back_(nullptr)
    {}

    table_store(table_store&& other) noexcept :
        table_store(std::allocator_arg, other.get_allocator(),
            std::move(other))
    {}

    // Requires allocators to be equal and throws nothing.
    table_store(std::allocator_arg_t, const Allocator& alloc,
        table_store&& other) :
        member_like_base<Allocator>(alloc),
        buffers_(other.buffers_), buffers_back_(other.buffers_back_),
        buffers_size_(other.buffers_size_),
        buffers_cleared_(other.buffers_cleared_),
        buffers_cleared_back_(other.buffers_cleared_back_)
    {
        assert(alloc == other.get_allocator());
        other.buffers_ = nullptr;
        other.buffers_back_ = nullptr;
        other.buffers_size_ = 0;
        other.buffers_cleared_ = nullptr;
        other.buffers_cleared_back_ = nullptr;
    }

    ~table_store()
    {
        // First splice buffers_ and buffers_cleared_ into buffers_
        if (!buffers_) {
            buffers_ = buffers_cleared_;
        } else {
            assert(buffers_back_);
            assert(!buffers_back_->next);
            buffers_back_->next = buffers_cleared_;
        }

        // Then destroy all buffers in buffers_
        while (buffers_) {
            std::pair<Ch*, std::size_t> p;
            std::tie(p, buffers_) = byebye(buffers_);
            at_t::deallocate(this->get(),
                pt_t::pointer_to(*p.first), p.second);
        }
    }

    // Requires allocators to be equal and throws nothing.
    table_store& operator=(table_store&& other)
    {
        assert(get_allocator() == other.get_allocator());
        table_store(
            std::allocator_arg,
            select_allocator(*this, other,
                typename at_t::propagate_on_container_move_assignment()),
            std::move(other)).swap_force(*this);
        return *this;
    }

    allocator_type get_allocator() const noexcept
    {
        return this->get();
    }

    // Takes the ownership of "buffer" over when called (that is, callers
    // must not deallocate "buffer" even if an exception is thrown)
    void add_buffer(Ch* buffer, std::size_t size)
    {
        // "push_front"-like behaviour
        buffers_ = hello(buffer, size, buffers_);   // throw
        if (!buffers_back_) {
            buffers_back_ = buffers_;
        }
        ++buffers_size_;

        assert(buffers_);
        assert(buffers_back_);
        assert(!buffers_back_->next);
    }

    void secure_current_upto(Ch* secured_last)
    {
        assert(buffers_);
        buffers_->secure_upto(secured_last);
    }

    Ch* secure_any(std::size_t size) noexcept
    {
        for (auto i = buffers_; i; i = i->next) {
            if (const auto secured = i->secure(size)) {
                return secured;
            }
        }
        return nullptr;
    }

    std::pair<Ch*, std::size_t> generate_buffer(std::size_t min_size)
    {
        if (buffers_cleared_) {
            auto p_prev_next = &buffers_cleared_;
            auto p = *p_prev_next;
            decltype(p) prev = nullptr;
            for (; p; p_prev_next = &p->next, prev = p, p = *p_prev_next) {
                if (p->size() >= min_size) {
                    if (p == buffers_cleared_back_) {
                        buffers_cleared_back_ = prev;
                    }
                    std::pair<Ch*, std::size_t> r;
                    std::tie(r, *p_prev_next) = byebye(p);
                    return r;
                }
            }
        }
        return std::make_pair(
            std::addressof(*at_t::allocate(this->get(), min_size)), // throw
            min_size);
    }

    void consume_buffer(Ch* p, std::size_t size)
    {
        try {
            buffers_cleared_ = hello(p, size, buffers_cleared_);    // throw
            if (!buffers_cleared_back_) {
                buffers_cleared_back_ = buffers_cleared_;
            }
        } catch (...) {
            at_t::deallocate(this->get(), pt_t::pointer_to(*p), size);
        }
    }

private:
    // Creates a node which holds [buffer, buffer + size) in front of next;
    // this function consumes [buffer, buffer + size) at once
    node_type* hello(Ch* buffer, std::size_t size, node_type* next)
    {
        node_type* n;
        try {
            typename nat_t::allocator_type na(this->get());
            n = std::addressof(*nat_t::allocate(na, 1));    // throw
            ::new(n) node_type(next);                       // throw
        } catch (...) {
            // We'd better not call "consume_buffer" because it calls "hello,"
            // which is this function itself
            at_t::deallocate(this->get(), pt_t::pointer_to(*buffer), size);
            throw;
        }
        n->attach(buffer, size);
        return n;
    }

    // Detachs buffer from a node and then destroys the node; throws nothing
    std::pair<std::pair<Ch*, std::size_t>, node_type*> byebye(node_type* p)
    {
        const auto next = p->next;
        const auto r = p->detach();
        typename nat_t::allocator_type na(this->get());
        nat_t::deallocate(na, npt_t::pointer_to(*p), 1U);
        static_assert(std::is_trivially_destructible<node_type>::value, "");
        return std::make_pair(r, next);
    }

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4706)
#endif
    // Clears all elements in buffers_
    // and then splice buffers_ to buffers_cleared_back_
    void clear() noexcept
    {
        if (buffers_) {
            auto i = buffers_;
            do {
                i->clear();
            } while ((i = i->next));    // parens to squelch gcc's complaint
            if (buffers_cleared_back_) {
                buffers_cleared_back_->next = buffers_;
            } else {
                buffers_cleared_ = buffers_;
            }
            buffers_cleared_back_ = buffers_back_;
            buffers_ = nullptr;
            buffers_back_ = nullptr;
            buffers_size_ = 0;
        }
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    void swap(table_store& other)
        noexcept(at_t::propagate_on_container_swap::value)
    {
        assert(at_t::propagate_on_container_swap::value
            || (this->get() == other.get()));
        swap_alloc(other, typename at_t::propagate_on_container_swap());
        swap_data(other);
    }

    void swap_force(table_store& other) noexcept
    {
        swap_alloc(other, std::true_type());
        swap_data(other);
    }

    // Requires allocators to be equal and throws nothing.
    table_store& merge(table_store&& other)
    {
        assert(get_allocator() == other.get_allocator());

        if (buffers_back_) {
            assert(!buffers_back_->next);
            buffers_back_->next = other.buffers_;
        } else {
            assert(!buffers_);  // because buffers_back_ != nullptr
            buffers_ = other.buffers_;
        }
        buffers_back_ = other.buffers_back_;
        buffers_size_ += other.buffers_size_;
        other.buffers_ = nullptr;
        other.buffers_back_ = nullptr;
        other.buffers_size_ = 0;

        if (buffers_cleared_back_) {
            assert(!buffers_cleared_back_->next);
            buffers_cleared_back_->next = other.buffers_cleared_;
        } else {
            assert(!buffers_cleared_);
            buffers_cleared_ = other.buffers_cleared_;
        }
        buffers_cleared_back_ = other.buffers_cleared_back_;
        other.buffers_cleared_ = nullptr;
        other.buffers_cleared_back_ = nullptr;

        return *this;
    }

    security get_security() const
    {
        security s;                         // throw
        for (auto i = buffers_; i; i = i->next) {
            s.push_back(i->secured());      // throw
        }
        return s;
    }

    void set_security(const security& s)
    {
        // s = get_security() -> add_buffer() -> set_security(s) is OK
        assert(s.size() <= buffers_size_);
        for (auto k = s.size(); k < buffers_size_; ++k) {
            assert(buffers_);
            std::pair<Ch*, std::size_t> p;
            std::tie(p, buffers_) = byebye(buffers_);
            consume_buffer(p.first, p.second);
            --buffers_size_;
        }

        if (buffers_) {
            auto i = buffers_;
            auto j = s.cbegin();
            for (; i; i = i->next, ++j) {
                i->secure_upto(*j);
            }
        } else {
            buffers_back_ = nullptr;
        }
    }

private:
    void swap_alloc(table_store& other, std::true_type) noexcept
    {
        using std::swap;
        swap(this->get(), other.get());
    }

    void swap_alloc(table_store&, std::false_type) noexcept
    {}

    void swap_data(table_store& other) noexcept
    {
        using std::swap;
        swap(buffers_, other.buffers_);
        swap(buffers_back_, other.buffers_back_);
        swap(buffers_size_, other.buffers_size_);
        swap(buffers_cleared_, other.buffers_cleared_);
        swap(buffers_cleared_back_, other.buffers_cleared_back_);
    }
};

template <class Ch, class Allocator>
void swap(
    table_store<Ch, Allocator>& left,
    table_store<Ch, Allocator>& right) noexcept(
        std::allocator_traits<Allocator>::propagate_on_container_swap::value)
{
    left.swap(right);
}

template <class T>
struct is_basic_stored_value :
    std::false_type
{};

template <class... Args>
struct is_basic_stored_value<basic_stored_value<Args...>> :
    std::true_type
{};

// Placed here to avoid bloat
template <class Tr>
struct ntbs_end
{
    template <class InputIterator>
    friend bool operator!=(InputIterator left, ntbs_end)
    {
        return !Tr::eq(*left, typename Tr::char_type());
    }
};

} // end namespace detail

template <class Content, class Allocator = std::allocator<Content>>
class basic_stored_table
{
public:
    using allocator_type  = Allocator;
    using content_type    = Content;
    using record_type     = typename content_type::value_type;
    using value_type      = typename record_type::value_type;
    using char_type       = std::remove_const_t<
                                typename value_type::value_type>;
    using traits_type     = typename value_type::traits_type;
    using size_type       = typename content_type::size_type;

    static_assert(detail::is_basic_stored_value<value_type>::value,
        "Content shall be a sequence-container-of-sequence-container "
        "type of basic_stored_value");
    static_assert(
        std::is_same<content_type, typename Allocator::value_type>::value,
        "Allocator shall have value_type which is the same as content_type");

private:
    using at_t = std::allocator_traits<allocator_type>;
    using ca_base_t = typename at_t::template rebind_alloc<char_type>;
    using cat_t = std::allocator_traits<
        detail::propagation_controlled_allocator<
            ca_base_t,
            at_t::propagate_on_container_copy_assignment::value,
            at_t::propagate_on_container_move_assignment::value,
            at_t::propagate_on_container_swap::value>>;
    using ca_t = typename cat_t::allocator_type;
    using store_type = detail::table_store<char_type, ca_t>;

    template <class OtherContent, class OtherAllocator>
    friend class basic_stored_table;

private:
    store_type store_;
    typename at_t::pointer records_;

    std::size_t buffer_size_;

public:
    explicit basic_stored_table(std::size_t buffer_size = 0U) :
        basic_stored_table(std::allocator_arg, Allocator(), buffer_size)
    {}

    basic_stored_table(std::allocator_arg_t, const Allocator& alloc,
        std::size_t buffer_size = 0U) :
        store_(std::allocator_arg, ca_t(ca_base_t(alloc))),
        records_(allocate_create_content(alloc)),
        buffer_size_(sanitize_buffer_size(buffer_size, alloc))
    {}

    basic_stored_table(const basic_stored_table& other) :
        basic_stored_table(std::allocator_arg, 
            at_t::select_on_container_copy_construction(other.get_allocator()),
            other)
    {}

    basic_stored_table(std::allocator_arg_t, const Allocator& alloc,
        const basic_stored_table& other) :
        store_(std::allocator_arg, ca_t(ca_base_t(alloc))), records_(nullptr),
        buffer_size_(std::min(at_t::max_size(alloc), other.buffer_size_))
    {
        if (other.is_singular()) {
            // leave also *this singular
            assert(is_singular());
            return;
        }
        records_ = allocate_create_content(alloc);  // throw
        try {
            copy_from(other.content());             // throw
        } catch (...) {
            destroy_deallocate_content(alloc, records_);
            throw;
        }
    }

    // This ctor template is here only to get accordance with opetator+=
    template <class OtherContent, class OtherAllocator>
    basic_stored_table(
        const basic_stored_table<OtherContent, OtherAllocator>& other) :
        basic_stored_table(std::allocator_arg, Allocator(), other)
    {}

    template <class OtherContent, class OtherAllocator>
    basic_stored_table(std::allocator_arg_t, const Allocator& alloc,
        const basic_stored_table<OtherContent, OtherAllocator>& other) :
        basic_stored_table(
            std::allocator_arg, alloc, mimic_other_t(), other)
    {}

    basic_stored_table(basic_stored_table&& other) noexcept :
        store_(std::move(other.store_)), records_(other.records_),
        buffer_size_(other.buffer_size_)
    {
        other.records_ = nullptr;
    }

    basic_stored_table(std::allocator_arg_t, const Allocator& alloc,
        basic_stored_table&& other) :
        store_(std::allocator_arg, ca_t(ca_base_t(alloc))), records_(nullptr),
        buffer_size_(std::min(at_t::max_size(alloc), other.buffer_size_))
    {
        if (alloc == other.get_allocator()) {
            store_type s(std::allocator_arg,
                store_.get_allocator(), std::move(other.store_));
            store_.swap_force(s);
            using std::swap;
            swap(records_/*nullptr*/, other.records_);
        } else {
            basic_stored_table(std::allocator_arg, alloc, other).
                swap_force(*this);  // throw
        }
    }

    // This ctor template is here only to get accordance with opetator+=
    template <class OtherContent, class OtherAllocator>
    basic_stored_table(
        basic_stored_table<OtherContent, OtherAllocator>&& other) :
        basic_stored_table(std::allocator_arg, Allocator(), std::move(other))
    {}

    template <class OtherContent, class OtherAllocator>
    basic_stored_table(std::allocator_arg_t, const Allocator& alloc,
        basic_stored_table<OtherContent, OtherAllocator>&& other) :
        basic_stored_table(
            std::allocator_arg, alloc, mimic_other_t(), std::move(other))
    {}

private:
    struct mimic_other_t {};

    template <class Other>
    basic_stored_table(std::allocator_arg_t, const Allocator& alloc,
        mimic_other_t, Other&& other) :
        store_(std::allocator_arg, ca_t(ca_base_t(alloc))), records_(nullptr),
        buffer_size_(std::min(at_t::max_size(alloc), other.buffer_size_))
    {
        assert(is_singular());
        if (!other.is_singular()) {
            basic_stored_table t(
                std::allocator_arg, get_allocator(), get_buffer_size());
            t += std::forward<Other>(other);
            swap_force(t);
        }
    }

public:
    ~basic_stored_table()
    {
        destroy_deallocate_content(get_allocator(), records_);
    }

private:
    static constexpr std::size_t default_buffer_size =
        std::min<std::size_t>(std::numeric_limits<std::size_t>::max(), 8192U);

    static std::size_t sanitize_buffer_size(
        std::size_t buffer_size, const Allocator& alloc)
    {
        if (buffer_size == 0U) {
            buffer_size = default_buffer_size;
        }
        return std::min(
            std::max(buffer_size, static_cast<std::size_t>(2U)),
            at_t::max_size(alloc));
    }

    template <class... Args>
    static auto allocate_create_content(Allocator a, Args&&... args)
    {
        auto records = at_t::allocate(a, 1);                    // throw
        try {
            at_t::construct(a, std::addressof(*records),
                std::forward<Args>(args)...);                   // throw
        } catch (...) {
            at_t::deallocate(a, records, 1);
            throw;
        }
        return records;
    }

    static void destroy_deallocate_content(
        Allocator a, typename at_t::pointer records)
    {
        if (records) {
            at_t::destroy(a, std::addressof(*records));
            at_t::deallocate(a, records, 1);
        }
    }

public:
    basic_stored_table& operator=(const basic_stored_table& other)
    {
        basic_stored_table(
            std::allocator_arg,
            detail::select_allocator(*this, other,
                typename at_t::propagate_on_container_copy_assignment()),
            other).swap_force(*this);   // throw
        return *this;
    }

    basic_stored_table& operator=(basic_stored_table&& other)
        noexcept(at_t::propagate_on_container_move_assignment::value)
    {
        basic_stored_table(
            std::allocator_arg,
            detail::select_allocator(*this, other,
                typename at_t::propagate_on_container_move_assignment()),
            std::move(other)).swap_force(*this);    // throw
        return *this;
    }

public:
    allocator_type get_allocator() const noexcept
    {
        return typename at_t::allocator_type(store_.get_allocator().base());
    }

    std::size_t get_buffer_size() const noexcept
    {
        return buffer_size_;
    }

    bool is_singular() const noexcept
    {
        return !records_;
    }

    content_type& content()
    {
        assert(!is_singular());
        return *records_;
    }

    const content_type& content() const
    {
        assert(!is_singular());
        return *records_;
    }

    record_type& operator[](size_type record_index)
    {
        return content()[record_index];
    }

    const record_type& operator[](size_type record_index) const
    {
        return content()[record_index];
    }

    template <class ForwardIterator, class ForwardIteratorEnd>
    value_type& rewrite_value(value_type& value,
        ForwardIterator new_value_begin, ForwardIteratorEnd new_value_end)
    {
        return rewrite_value_impl(value, new_value_begin, new_value_end);
    }

    template <class ForwardIterator>
    auto rewrite_value(value_type& value, ForwardIterator new_value)
     -> std::enable_if_t<
            std::is_base_of<
                std::forward_iterator_tag,
                typename std::iterator_traits<ForwardIterator>::
                    iterator_category>::value,
            value_type&>
    {
        return rewrite_value_impl(value, new_value);
    }

private:
    template <class ForwardIterator>
    value_type& rewrite_value_impl(value_type& value,
        ForwardIterator new_value_begin, ForwardIterator new_value_end)
    {
        return rewrite_value_n(value, new_value_begin,
            static_cast<std::size_t>(
                std::distance(new_value_begin, new_value_end)));
    }

    template <class ForwardIterator, class ForwardIteratorEnd>
    value_type& rewrite_value_impl(value_type& value,
        ForwardIterator new_value_begin, ForwardIteratorEnd new_value_end)
    {
        ForwardIterator i = new_value_begin;
        std::size_t length = 0;
        while (i != new_value_end) {
            ++i;
            ++length;
        }
        return rewrite_value_n(value, new_value_begin, length);
    }

    template <class ForwardIterator>
    value_type& rewrite_value_impl(value_type& value,
        ForwardIterator new_value)
    {
        return rewrite_value_impl(value,
            new_value, detail::ntbs_end<traits_type>());
    }

    value_type& rewrite_value_impl(value_type& value, const char* new_value)
    {
        return rewrite_value_n(value,
            new_value, traits_type::length(new_value));
    }

    template <class InputIterator>
    value_type& rewrite_value_n(value_type& value,
        InputIterator new_value_begin, std::size_t new_value_size)
    {
        return rewrite_value_n_impl(
            std::is_const<typename value_type::value_type>(),
            value, new_value_begin, new_value_size);
    }

    template <class InputIterator>
    value_type& rewrite_value_n_impl(std::true_type, value_type& value,
        InputIterator new_value_begin, std::size_t new_value_size)
    {
        return rewrite_value_n_secure(value, new_value_begin, new_value_size);
    }

    static void move_chs(
        const char_type* begin1, std::size_t size, char_type* begin2)
    {
        traits_type::move(begin2, begin1, size);
    }

    template <class InputIterator>
    static void move_chs(
        InputIterator begin1, std::size_t size, char_type* begin2)
    {
        for (std::size_t i = 0; i < size; ++i) {
            traits_type::assign(*begin2, *begin1);
            ++begin1;
            ++begin2;
        }
    }

    template <class InputIterator>
    value_type& rewrite_value_n_impl(std::false_type, value_type& value,
        InputIterator new_value_begin, std::size_t new_value_size)
    {
        if (new_value_size <= value.size()) {
            move_chs(new_value_begin, new_value_size, value.begin());
            value.erase(value.cbegin() + new_value_size, value.cend());
            return value;
        }
        return rewrite_value_n_secure(value, new_value_begin, new_value_size);
    }

    template <class InputIterator>
    value_type& rewrite_value_n_secure(value_type& value,
        InputIterator new_value_begin, std::size_t new_value_size)
    {
        auto secured = store_.secure_any(new_value_size + 1);
        if (!secured) {
            auto alloc_size = std::max(new_value_size + 1, buffer_size_);
            std::tie(secured, alloc_size) = generate_buffer(alloc_size);
                                                // throw
            add_buffer(secured, alloc_size);    // throw
            // No need to deallocate secured even when an exception is
            // thrown by add_buffer because add_buffer consumes secured
            secure_current_upto(secured + new_value_size + 1);
        }
        move_chs(new_value_begin, new_value_size, secured);
        traits_type::assign(secured[new_value_size], char_type());
        value = value_type(secured, secured + new_value_size);
        return value;
    }

public:
    template <class OtherValue>
    auto rewrite_value(value_type& value, const OtherValue& new_value)
     -> decltype(
            std::declval<typename OtherValue::const_iterator>(),
            std::declval<value_type>())&
    {
        return rewrite_value(value, new_value.cbegin(), new_value.cend());
    }

    template <class... Args>
    value_type import_value(Args&&... args)
    {
        value_type value;
        return rewrite_value(value, std::forward<Args>(args)...);   // throw
    }

    template <class F>
    void guard_rewrite(F f)
    {
        const auto security = store_.get_security();    // throw
        try {
            f(*this);                                   // throw
        } catch (...) {
            store_.set_security(security);
            throw;
        }
    }

    size_type size() const
        noexcept(noexcept(std::declval<const content_type&>().size()))
    {
        return is_singular() ? 0 : content().size();
    }

    bool empty() const
        noexcept(noexcept(std::declval<const content_type&>().empty()))
    {
        return is_singular() || content().empty();
    }

    void clear()
        noexcept(noexcept(std::declval<content_type&>().clear()))
    {
        if (!is_singular()) {
            content().clear();
        }
        store_.clear();
    }

    void shrink_to_fit()
    {
        basic_stored_table(*this).swap(*this);     // throw
    }

    void swap(basic_stored_table& other)
        noexcept(cat_t::propagate_on_container_swap::value)
    {
        assert((cat_t::propagate_on_container_swap::value
             || (get_allocator() == other.get_allocator())) &&
                "Swapping two basic_stored_tables with allocators of which "
                "POCS is false and which do not equal each other delivers "
                "an undefined behaviour; this is it");
        using std::swap;
        swap(store_, other.store_);
        swap(records_, other.records_);
        swap(buffer_size_, other.buffer_size_);
    }

    template <class OtherContent, class OtherAllocator>
    basic_stored_table& operator+=(
        const basic_stored_table<OtherContent, OtherAllocator>& other)
    {
        return operator_plus_assign_impl(other);
    }

    template <class OtherContent>
    basic_stored_table& operator+=(
        basic_stored_table<OtherContent, Allocator>&& other)
    {
        return operator_plus_assign_impl(std::move(other));
    }

    std::pair<char_type*, std::size_t> generate_buffer(std::size_t min_size)
    {
        return store_.generate_buffer(min_size);
    }

    void consume_buffer(char_type* p, std::size_t size)
    {
        return store_.consume_buffer(p, size);
    }

    void add_buffer(char_type* buffer, std::size_t size)
    {
        store_.add_buffer(buffer, size);
    }

    void secure_current_upto(char_type* secured_last)
    {
        store_.secure_current_upto(secured_last);
    }

private:
    template <class OtherTable>
    basic_stored_table& operator_plus_assign_impl(OtherTable&& other)
    {
        if (is_singular()) {
            basic_stored_table t(
                std::allocator_arg, get_allocator());               // throw
            t.append_no_singular(std::forward<OtherTable>(other));  // throw
            swap(t);
        } else if (!other.is_singular()) {
            append_no_singular(std::forward<OtherTable>(other));    // throw
        }
        return *this;
    }

    template <class OtherContent, class OtherAllocator>
    void append_no_singular(
        const basic_stored_table<OtherContent, OtherAllocator>& other)
    {
        other.copy_to(*this);
    }

    template <class OtherContent>
    void append_no_singular(
        basic_stored_table<OtherContent, Allocator>&& other);

    template <class ContentTo, class AllocatorTo>
    void copy_to(basic_stored_table<ContentTo, AllocatorTo>& to) const
    {
        // We require:
        // - if an exception is thrown by ContentTo's emplace() at the end,
        //   there are no effects on the before-end elements.
        // - ContentTo's erase() at the end does not throw an exception.

        to.guard_rewrite([c = std::addressof(content())](auto& t) {
            auto& tc = t.content();
            const auto original_size = tc.size();       // ?
            try {
                t.copy_from(*c);                        // throw
            } catch (...) {
                tc.erase(std::next(tc.cbegin(), original_size), tc.cend());
                throw;
            }
        });
    }

    template <class RecordTo, class AllocatorRTo, class AllocatorTo>
    void copy_to(basic_stored_table<std::list<RecordTo, AllocatorRTo>,
                                    AllocatorTo>& to) const
    {
        std::list<RecordTo, AllocatorRTo> r2(
            to.content().get_allocator());              // throw
        to.guard_rewrite([&r2, c = std::addressof(content())](auto& t) {
            t.copy_from(*c, r2);                        // throw
        });
        to.content().splice(to.content().cend(), r2);
    }

    template <class OtherContent>
    void copy_from(const OtherContent& other)
    {
        copy_from(other, content());                            // throw
    }

    template <class OtherContent>
    void copy_from(const OtherContent& other, content_type& records)
    {
        reserve(records, records.size() + other.size());        // throw
        for (const auto& r : other) {
            const auto e = records.emplace(records.cend());     // throw
            reserve(*e, r.size());                              // throw
            for (const auto& v : r) {
                e->insert(e->cend(), import_value(v));          // throw
            }
        }
    }

    template <class Container>
    static void reserve(Container&, typename Container::size_type)
    {}

    template <class... Ts>
    static void reserve(std::vector<Ts...>& c,
        typename std::vector<Ts...>::size_type n)
    {
        c.reserve(n);
    }

private:
    // Unconditionally swaps all contents including allocators
    void swap_force(basic_stored_table& other) noexcept
    {
        using std::swap;
        store_.swap_force(other.store_);
        swap(records_, other.records_);
        swap(buffer_size_, other.buffer_size_);
    }
};

template <class Content, class Allocator>
void swap(
    basic_stored_table<Content, Allocator>& left,
    basic_stored_table<Content, Allocator>& right) noexcept(
        std::allocator_traits<Allocator>::propagate_on_container_swap::value)
{
    left.swap(right);
}

namespace detail {

template <class T, class = void>
struct has_allocator_type :
    std::false_type
{};

template <class T>
struct has_allocator_type<T, typename T::allocator_type*> :
    std::true_type
{};

template <class T>
struct is_nothrow_swappable :
    std::integral_constant<bool,
        noexcept(std::declval<T&>().swap(std::declval<T&>()))>
    // With C++17, we should rely on std::is_throw_swappable.
{};

template <class T, class = void>
struct adaptive_manoeuvre;

template <class T>
struct adaptive_manoeuvre<T,
    std::enable_if_t<std::is_nothrow_move_assignable<T>::value>>
{
    template <class Container>
    static void emplace_back(Container& c, T&& t)
    {
        c.emplace(c.cend(), std::move(t));
    }

    static void emigrate(T&& from, T& to) noexcept
    {
        to = std::move(from);
    }
};

template <class T>
struct adaptive_manoeuvre<T,
    std::enable_if_t<
        !std::is_nothrow_move_assignable<T>::value
     && has_allocator_type<T>::value>>
{
    template <class Container>
    static void emplace_back(Container& c, T&& t)
    {
        c.emplace(c.cend(), c.get_allocator());
        c.end().swap(t);
    }

    // Requires allocators to be equal and throws nothing.
    static void emigrate(T&& from, T& to)
    {
        assert(from.get_allocator() == to.get_allocator());
        to.swap(from);
    }
};

template <class T>
struct adaptive_manoeuvre<T,
    std::enable_if_t<
        !std::is_nothrow_move_assignable<T>::value
     && !has_allocator_type<T>::value
     && is_nothrow_swappable<T>::value>>
{
    template <class Container>
    static void emplace_back(Container& c, T&& t)
    {
        c.emplace(c.cend());
        c.end().swap(t);
    }

    static void emigrate(T&& from, T& to) noexcept
    {
        to.swap(from);
    }
};

template <class T>
struct has_adaptive_manoeuvre :
    std::integral_constant<bool,
        std::is_nothrow_move_assignable<T>::value
     || has_allocator_type<T>::value
     || is_nothrow_swappable<T>::value>
{};

template <class ContentL, class ContentR>
void append_stored_table_content_primitive(ContentL& l, ContentR&& r)
{
    // We require:
    // - if an exception is thrown by ContentL's emplace() at the end,
    //   there are no effects on the before-end elements.
    // - ContentL's erase() at the end does not throw an exception.
    // - ContentR's erase() at the end does not throw an exception.

    const auto left_original_size = l.size();               // ?
    try {
        for (auto& rr : r) {
            l.emplace(l.cend(), rr.cbegin(), rr.cend());    // throw
        }
    } catch (...) {
        l.erase(std::next(l.begin(), left_original_size), l.cend());
        throw;
    }

    // Make sure right_content has no reference to moved values;
    // we use erase() instead of clear() to simplify requirements
    r.erase(r.cbegin(), r.cend());
}

template <class RecordL, class AllocatorL, class ContentR>
void append_stored_table_content_primitive(
    std::list<RecordL, AllocatorL>& l, ContentR&& r)
{
    std::list<RecordL, AllocatorL> r2(l.get_allocator());   // throw
    for (auto& rr : r) {
        r2.emplace_back(rr.cbegin(), rr.cend());            // throw
    }
    l.splice(l.cend(), r2);
}

template <class ContentL, class ContentR>
void append_stored_table_content_adaptive(ContentL& l, ContentR&& r)
{
    static_assert(std::is_same<typename ContentL::value_type,
        typename ContentR::value_type>::value, "");
    static_assert(
        has_adaptive_manoeuvre<typename ContentL::value_type>::value, "");

    // We require:
    // - if an exception is thrown by ContentL's emplace() at the end,
    //   there are no effects on the before-end elements.
    // - ContentL's erase() at the end does not throw an exception.
    // - ContentR's erase() at the end does not throw an exception.
    // - if ContentL::value_type has allocator_type member type, it shall be
    //   allocator aware and ContentL::value_type::swap on objects which have
    //   equal allocators shall not exit via an exception.

    using manoeuvre = adaptive_manoeuvre<typename ContentL::value_type>;

    const auto left_original_size = l.size();    // ?
    try {
        for (auto& rr : r) {
            manoeuvre::emplace_back(l, std::move(rr));  // throw
        }
    } catch (...) {
        const auto le = std::next(l.begin(), left_original_size);
        for (auto i = le, ie = l.end(), j = r.begin(); i != ie; ++i, ++j) {
            manoeuvre::emigrate(std::move(*i), *j);
        }
        l.erase(le, l.cend());
        throw;
    }

    // Make sure right_content has no reference to moved values;
    // we use erase() instead of clear() to simplify requirements
    r.erase(r.cbegin(), r.cend());
}

template <class Record, class AllocatorL, class ContentR>
void append_stored_table_content_adaptive(
    std::list<Record, AllocatorL>& l, ContentR&& r)
{
    static_assert(
        std::is_same<Record, typename ContentR::value_type>::value, "");
    static_assert(has_adaptive_manoeuvre<Record>::value, "");

    using manoeuvre = adaptive_manoeuvre<Record>;

    std::list<Record, AllocatorL> r2(l.get_allocator());    // throw
    try {
        for (auto& rr : r) {
            manoeuvre::emplace_back(r2, std::move(rr));     // throw
        }
    } catch (...) {
        for (auto i = r2.begin(), ie = r2.end(), j = r.begin();
                i != ie; ++i, ++j) {
            manoeuvre::emigrate(std::move(*i), *j);
        }
        throw;
    }
    l.splice(l.cend(), r2);
}

template <class ContentL, class ContentR>
auto append_stored_table_content(ContentL& l, ContentR&& r)
 -> std::enable_if_t<
        std::is_same<
            typename ContentL::value_type,
            typename ContentR::value_type>::value
     && has_adaptive_manoeuvre<
            typename ContentL::value_type>::value>
{
    append_stored_table_content_adaptive(l, std::move(r));
}

template <class ContentL, class ContentR>
auto append_stored_table_content(ContentL& l, ContentR&& r)
 -> std::enable_if_t<
        !std::is_same<
            typename ContentL::value_type,
            typename ContentR::value_type>::value
     || !has_adaptive_manoeuvre<
            typename ContentL::value_type>::value>
{
    append_stored_table_content_primitive(l, r);
}

template <class Record, class Allocator>
void append_stored_table_content(
    std::list<Record, Allocator>& l, std::list<Record, Allocator>&& r)
{
    if (l.get_allocator() == r.get_allocator()) {
        l.splice(l.cend(), r);
    } else {
        append_stored_table_content_adaptive(l, std::move(r));  // throw
    }
}

template <class ContentL, class AllocatorL, class TableR>
auto plus_stored_table_impl(
    const basic_stored_table<ContentL, AllocatorL>& left, TableR&& right)
{
    auto l(left);                           // throw
    l += std::forward<TableR>(right);       // throw
    return l;
}

template <class ContentL, class AllocatorL, class TableR>
auto plus_stored_table_impl(
    basic_stored_table<ContentL, AllocatorL>&& left, TableR&& right)
{
    left += std::forward<TableR>(right);    // throw
    return std::move(left);
}

} // end namespace detail

template <class Content, class Allocator>
template <class OtherContent>
void basic_stored_table<Content, Allocator>::append_no_singular(
    basic_stored_table<OtherContent, Allocator>&& other)
{
    if (store_.get_allocator() == other.store_.get_allocator()) {
        detail::append_stored_table_content(
            content(), std::move(other.content()));     // throw
        store_.merge(std::move(other.store_));
    } else {
        append_no_singular(other);                      // throw
    }
}

template <class ContentL, class AllocatorL, class ContentR, class AllocatorR>
auto operator+(
    const basic_stored_table<ContentL, AllocatorL>& left,
    const basic_stored_table<ContentR, AllocatorR>& right)
{
    return detail::plus_stored_table_impl(left, right);
}

template <class ContentL, class AllocatorL, class ContentR, class AllocatorR>
auto operator+(
    const basic_stored_table<ContentL, AllocatorL>& left,
    basic_stored_table<ContentR, AllocatorR>&& right)
{
    return detail::plus_stored_table_impl(left, std::move(right));
}

template <class ContentL, class AllocatorL, class ContentR, class AllocatorR>
auto operator+(
    basic_stored_table<ContentL, AllocatorL>&& left,
    const basic_stored_table<ContentR, AllocatorR>& right)
{
    return detail::plus_stored_table_impl(std::move(left), right);
}

template <class ContentL, class AllocatorL, class ContentR, class AllocatorR>
auto operator+(
    basic_stored_table<ContentL, AllocatorL>&& left,
    basic_stored_table<ContentR, AllocatorR>&& right)
{
    return detail::plus_stored_table_impl(std::move(left), std::move(right));
}

using stored_table  =
    basic_stored_table<std::deque<std::vector<stored_value>>>;
using wstored_table =
    basic_stored_table<std::deque<std::vector<wstored_value>>>;
using cstored_table =
    basic_stored_table<std::deque<std::vector<cstored_value>>>;
using cwstored_table =
    basic_stored_table<std::deque<std::vector<cwstored_value>>>;

enum stored_table_builder_option : std::uint_fast8_t
{
    stored_table_builder_option_transpose = 1
};

namespace detail {

template <class Content>
struct arrange_as_is
{
    using char_type = typename Content::value_type::value_type::value_type;

    explicit arrange_as_is(Content&)
    {}

    void new_record(Content& content) const
    {
        content.emplace(content.cend());            // throw
    }

    void new_value(Content& content, char_type* first, char_type* last) const
    {
        auto& back = *content.rbegin();
        back.emplace(back.cend(), first, last);     // throw
    }
};

template <class Content>
class arrange_transposing
{
    using record_t = typename Content::value_type;
    using value_t = typename record_t::value_type;

    // Current physical record index; is my current field index
    typename record_t::size_type i_;

    // Points my current record
    typename Content::iterator j_;

public:
    using char_type = typename Content::value_type::value_type::value_type;

    explicit arrange_transposing(Content& content) :
        i_(std::accumulate(
            content.cbegin(), content.cend(),
            static_cast<decltype(i_)>(0),
            [](const auto& a, const auto& rec) {
                return std::max(a, rec.size());
            }))
    {}

    void new_record(Content& content)
    {
        ++i_;
        j_ = content.begin();
    }

    void new_value(Content& content, char_type* first, char_type* last)
    {
        assert(i_ > 0);
        if (content.end() == j_) {
            j_ = content.emplace(content.cend(), i_, value_t());    // throw
        } else {
            j_->insert(j_->cend(), i_ - j_->size(), value_t());     // throw
        }
        *j_->rbegin() = value_t(first, last);
        ++j_;
    }
};

template <class Content,
    std::underlying_type_t<stored_table_builder_option> Options>
using arrange = std::conditional_t<
    (Options & stored_table_builder_option_transpose) != 0U,
    arrange_transposing<Content>, arrange_as_is<Content>>;

} // end namespace detail

template <class Content, class Allocator,
    std::underlying_type_t<stored_table_builder_option> Options = 0U>
class stored_table_builder :
    detail::arrange<Content, Options>
{
public:
    using table_type = basic_stored_table<Content, Allocator>;
    using char_type = typename table_type::char_type;

private:
    char_type* current_buffer_holder_;
    char_type* current_buffer_;
    std::size_t current_buffer_size_;

    char_type* field_begin_;
    char_type* field_end_;

    table_type* table_;

public:
    explicit stored_table_builder(table_type& table) :
        detail::arrange<Content, Options>(table.content()),
        current_buffer_holder_(nullptr), current_buffer_(nullptr),
        field_begin_(nullptr), table_(std::addressof(table))
    {}

    stored_table_builder(stored_table_builder&& other) noexcept :
        detail::arrange<Content, Options>(other),
        current_buffer_holder_(other.current_buffer_holder_),
        current_buffer_(other.current_buffer_),
        current_buffer_size_(other.current_buffer_size_),
        field_begin_(other.field_begin_), field_end_(other.field_end_),
        table_(other.table_)
    {
        other.current_buffer_holder_ = nullptr;
    }

    ~stored_table_builder()
    {
        if (current_buffer_holder_) {
            using a_t = typename table_type::allocator_type;
            using cat_t = typename std::allocator_traits<a_t>::template
                rebind_traits<char_type>;
            using ca_t = typename cat_t::allocator_type;
            using p_t = typename cat_t::pointer;
            using pt_t = std::pointer_traits<p_t>;
            ca_t a(table_->get_allocator());
            cat_t::deallocate(
                a, pt_t::pointer_to(*current_buffer_), current_buffer_size_);
        }
    }

    void start_record(const char_type* /*record_begin*/)
    {
        this->new_record(table_->content());    // throw
    }

    void update(const char_type* first, const char_type* last)
    {
        if (field_begin_) {
            table_type::traits_type::move(field_end_, first, last - first);
            field_end_ += last - first;
        } else {
            field_begin_ = current_buffer_ + (first - current_buffer_);
            field_end_   = current_buffer_ + (last  - current_buffer_);
        }
    }

    void finalize(const char_type* first, const char_type* last)
    {
        update(first, last);
        table_type::traits_type::assign(*field_end_, char_type());
        if (current_buffer_holder_) {
            auto cbh = current_buffer_holder_;
            current_buffer_holder_ = nullptr;
            table_->add_buffer(cbh, current_buffer_size_);    // throw
        }
        this->new_value(table_->content(), field_begin_, field_end_); // throw
        table_->secure_current_upto(field_end_ + 1);
        field_begin_ = nullptr;
    }

    void end_record(const char_type* /*record_end*/)
    {}

    std::pair<char_type*, std::size_t> get_buffer()
    {
        std::size_t length;
        if (field_begin_) {
            // In an active value, whose length is "length" so far
            length = static_cast<std::size_t>(field_end_ - field_begin_);
            // We'd like to move the active value to the beginning of the
            // returned buffer
            const std::size_t next_buffer_size = get_next_buffer_size(length);
            using traits_t = typename table_type::traits_type;
            if (current_buffer_holder_
             && (current_buffer_size_ >= next_buffer_size)) {
                // The current buffer contains no other values
                // and it suffices in terms of length
                traits_t::move(current_buffer_holder_, field_begin_, length);
            } else {
                // The current buffer has been committed to the store or
                // seems to be too short, so we need a new one
                const auto p = table_->generate_buffer(next_buffer_size);
                        // throw
                traits_t::copy(p.first, field_begin_, length);
                if (current_buffer_holder_) {
                    table_->consume_buffer(
                        current_buffer_holder_, current_buffer_size_);
                }
                std::tie(current_buffer_holder_, current_buffer_size_) = p;
            }
            field_begin_ = current_buffer_holder_;
            field_end_   = current_buffer_holder_ + length;
        } else {
            // Out of any active values
            if (current_buffer_holder_) {
                // The current buffer contains no values,
                // so we would like to reuse it
            } else {
                // The current buffer has been committed to the store,
                // so we need a new one
                std::tie(current_buffer_holder_, current_buffer_size_) =
                    table_->generate_buffer(table_->get_buffer_size());
                        // throw
            }
            length = 0;
        }
        assert(current_buffer_holder_);
        current_buffer_ = current_buffer_holder_;
        // *p = Ch() may be performed where p is the "last" parameter of
        // "update" and "finalize" functions, so the length of buffers must
        // be shorter than the actual length by 1
        const auto effective_size = current_buffer_size_ - length;
        assert(effective_size > 1);
        return std::make_pair(current_buffer_ + length, effective_size - 1);
    }

private:
    std::size_t get_next_buffer_size(std::size_t occupied) const noexcept
    {
        std::size_t next = table_->get_buffer_size();
        for (; occupied >= next / 2; next *= 2);
        return next;
    }

public:
    void release_buffer(const char_type* /*buffer*/) noexcept
    {}
};

template <class Content, class Allocator>
auto make_stored_table_builder(basic_stored_table<Content, Allocator>& table)
{
    return stored_table_builder<Content, Allocator>(table);
}

template <class Content, class Allocator>
auto make_transposed_stored_table_builder(
    basic_stored_table<Content, Allocator>& table)
{
    return stored_table_builder<Content, Allocator,
        stored_table_builder_option_transpose>(table);
}

}

#endif
