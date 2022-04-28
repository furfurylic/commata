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
#include <new>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "buffer_size.hpp"
#include "empty_string.hpp"
#include "key_chars.hpp"
#include "member_like_base.hpp"
#include "propagation_controlled_allocator.hpp"
#include "string_value.hpp"

namespace commata {

template <class Ch, class Tr = std::char_traits<std::remove_const_t<Ch>>>
class basic_stored_value
{
    Ch* begin_;
    Ch* end_;   // must point the terminating zero

public:
    static_assert(
        std::is_same_v<
            std::remove_const_t<Ch>,
            typename Tr::char_type>,
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
        basic_stored_value(&detail::nul<Ch>::value, &detail::nul<Ch>::value)
    {}

    basic_stored_value(Ch* begin, Ch* end) :
        begin_(begin), end_(end)
    {
        assert(*end_ == Ch());
    }

    basic_stored_value(const basic_stored_value&) = default;
    basic_stored_value& operator=(const basic_stored_value&) = default;

    template <
        class OtherCh,
        // Visual Studio 2019 dislikes "is_const_v && is_same_v" order.
        // I don't know why.
        std::enable_if_t<std::is_same_v<Ch, const OtherCh>
                      && std::is_const_v<Ch>>* = nullptr>
    basic_stored_value(const basic_stored_value<OtherCh, Tr>& other)
        noexcept : basic_stored_value(other.begin(), other.end())
    {}

    template <
        class OtherCh,
        std::enable_if_t<std::is_same_v<Ch, const OtherCh>
                      && std::is_const_v<Ch>>* = nullptr>
    basic_stored_value& operator=(
        const basic_stored_value<OtherCh, Tr>& other) noexcept
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

    template <class OtherTr = std::char_traits<std::remove_const_t<Ch>>>
    operator std::basic_string_view<std::remove_const_t<Ch>, Tr>() const
        noexcept
    {
        return std::basic_string_view<std::remove_const_t<Ch>, OtherTr>(
            cbegin(), size());
    }

    template <class OtherTr = std::char_traits<std::remove_const_t<Ch>>,
        class Allocator = std::allocator<std::remove_const_t<Ch>>>
    explicit operator
    std::basic_string<std::remove_const_t<Ch>, OtherTr, Allocator>() const
    {
        return std::basic_string<std::remove_const_t<Ch>, OtherTr, Allocator>(
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

    size_type max_size() const noexcept
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
std::basic_string_view<std::remove_const_t<Ch>,
        std::char_traits<std::remove_const_t<Ch>>>
    to_string_view(const basic_stored_value<Ch, Tr>& v) noexcept
{
    return std::basic_string_view<std::remove_const_t<Ch>,
            std::char_traits<std::remove_const_t<Ch>>>(v.data(), v.size());
}

template <class Ch, class Tr,
    class Allocator = std::allocator<std::remove_const_t<Ch>>>
std::basic_string<std::remove_const_t<Ch>,
        std::char_traits<std::remove_const_t<Ch>>, Allocator>
    to_string(const basic_stored_value<Ch, Tr>& v,
        const Allocator& alloc = Allocator())
{
    return std::basic_string<std::remove_const_t<Ch>,
            std::char_traits<std::remove_const_t<Ch>>, Allocator>(
                v.cbegin(), v.cend(), alloc);
}

template <class ChL, class ChR, class Tr>
auto operator==(
    const basic_stored_value<ChL, Tr>& left,
    const basic_stored_value<ChR, Tr>& right) noexcept
 -> std::enable_if_t<
        std::is_same_v<
            std::remove_const_t<ChL>,
            std::remove_const_t<ChR>>, bool>
{
    return detail::string_value_eq(left, right);
}

template <class ChC, class Tr, class Right>
auto operator==(
    const basic_stored_value<ChC, Tr>& left,
    const Right& right)
    noexcept(noexcept(detail::string_value_eq(left, right)))
 -> std::enable_if_t<detail::is_comparable_with_string_value_v<
        std::remove_const_t<ChC>, Tr, Right>, bool>
{
    return detail::string_value_eq(left, right);
}

template <class ChC, class Tr, class Left>
auto operator==(
    const Left& left,
    const basic_stored_value<ChC, Tr>& right)
    noexcept(noexcept(detail::string_value_eq(left, right)))
 -> std::enable_if_t<detail::is_comparable_with_string_value_v<
        std::remove_const_t<ChC>, Tr, Left>, bool>
{
    return detail::string_value_eq(left, right);
}

template <class ChL, class ChR, class Tr>
auto operator!=(
    const basic_stored_value<ChL, Tr>& left,
    const basic_stored_value<ChR, Tr>& right) noexcept
 -> std::enable_if_t<
        std::is_same_v<
            std::remove_const_t<ChL>,
            std::remove_const_t<ChR>>, bool>
{
    return !(left == right);
}

template <class ChC, class Tr, class Right>
auto operator!=(
    const basic_stored_value<ChC, Tr>& left,
    const Right& right)
    noexcept(noexcept(!(left == right)))
 -> std::enable_if_t<detail::is_comparable_with_string_value_v<
        std::remove_const_t<ChC>, Tr, Right>, bool>
{
    return !(left == right);
}

template <class ChC, class Tr, class Left>
auto operator!=(
    const Left& left,
    const basic_stored_value<ChC, Tr>& right)
    noexcept(noexcept(!(left == right)))
 -> std::enable_if_t<detail::is_comparable_with_string_value_v<
        std::remove_const_t<ChC>, Tr, Left>, bool>
{
    return !(left == right);
}

template <class ChL, class ChR, class Tr>
auto operator<(
    const basic_stored_value<ChL, Tr>& left,
    const basic_stored_value<ChR, Tr>& right)
    noexcept(noexcept(detail::string_value_lt(left, right)))
 -> std::enable_if_t<
        std::is_same_v<
            std::remove_const_t<ChL>,
            std::remove_const_t<ChR>>, bool>
{
    return detail::string_value_lt(left, right);
}

template <class ChC, class Tr, class Right>
auto operator<(
    const basic_stored_value<ChC, Tr>& left,
    const Right& right)
    noexcept(noexcept(detail::string_value_lt(left, right)))
 -> std::enable_if_t<detail::is_comparable_with_string_value_v<
        std::remove_const_t<ChC>, Tr, Right>, bool>
{
    return detail::string_value_lt(left, right);
}

template <class ChC, class Tr, class Left>
auto operator<(
    const Left& left,
    const basic_stored_value<ChC, Tr>& right)
    noexcept(noexcept(detail::string_value_lt(left, right)))
 -> std::enable_if_t<detail::is_comparable_with_string_value_v<
        std::remove_const_t<ChC>, Tr, Left>, bool>
{
    return detail::string_value_lt(left, right);
}

template <class ChL, class ChR, class Tr>
auto operator>(
    const basic_stored_value<ChL, Tr>& left,
    const basic_stored_value<ChR, Tr>& right) noexcept
 -> std::enable_if_t<
        std::is_same_v<
            std::remove_const_t<ChL>,
            std::remove_const_t<ChR>>, bool>
{
    return right < left;
}

template <class ChC, class Tr, class Right>
auto operator>(
    const basic_stored_value<ChC, Tr>& left,
    const Right& right)
    noexcept(noexcept(right < left))
 -> std::enable_if_t<detail::is_comparable_with_string_value_v<
        std::remove_const_t<ChC>, Tr, Right>, bool>
{
    return right < left;
}

template <class ChC, class Tr, class Left>
auto operator>(
    const Left& left,
    const basic_stored_value<ChC, Tr>& right)
    noexcept(noexcept(right < left))
 -> std::enable_if_t<detail::is_comparable_with_string_value_v<
        std::remove_const_t<ChC>, Tr, Left>, bool>
{
    return right < left;
}

template <class ChL, class ChR, class Tr>
auto operator<=(
    const basic_stored_value<ChL, Tr>& left,
    const basic_stored_value<ChR, Tr>& right)
    noexcept(noexcept(!(right < left)))
 -> std::enable_if_t<
        std::is_same_v<
            std::remove_const_t<ChL>,
            std::remove_const_t<ChR>>, bool>
{
    return !(right < left);
}

template <class ChC, class Tr, class Right>
auto operator<=(
    const basic_stored_value<ChC, Tr>& left,
    const Right& right)
    noexcept(noexcept(!(right < left)))
 -> std::enable_if_t<detail::is_comparable_with_string_value_v<
        std::remove_const_t<ChC>, Tr, Right>, bool>
{
    return !(right < left);
}

template <class ChC, class Tr, class Left>
auto operator<=(
    const Left& left,
    const basic_stored_value<ChC, Tr>& right)
    noexcept(noexcept(!(right < left)))
 -> std::enable_if_t<detail::is_comparable_with_string_value_v<
        std::remove_const_t<ChC>, Tr, Left>, bool>
{
    return !(right < left);
}

template <class ChL, class ChR, class Tr>
auto operator>=(
    const basic_stored_value<ChL, Tr>& left,
    const basic_stored_value<ChR, Tr>& right)
    noexcept(noexcept(!(left < right)))
 -> std::enable_if_t<
        std::is_same_v<
            std::remove_const_t<ChL>,
            std::remove_const_t<ChR>>, bool>
{
    return !(left < right);
}

template <class ChC, class Tr, class Right>
auto operator>=(
    const basic_stored_value<ChC, Tr>& left,
    const Right& right)
    noexcept(noexcept(!(left < right)))
 -> std::enable_if_t<detail::is_comparable_with_string_value_v<
        std::remove_const_t<ChC>, Tr, Right>, bool>
{
    return !(left < right);
}

template <class ChC, class Tr, class Left>
auto operator>=(
    const Left& left,
    const basic_stored_value<ChC, Tr>& right)
    noexcept(noexcept(!(left < right)))
 -> std::enable_if_t<detail::is_comparable_with_string_value_v<
        std::remove_const_t<ChC>, Tr, Left>, bool>
{
    return !(left < right);
}

template <class ChC, class Tr>
auto operator<<(
    std::basic_ostream<std::remove_const_t<ChC>, Tr>& os,
    const basic_stored_value<ChC, Tr>& o)
 -> decltype(os)
{
    return os << to_string_view(o);
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
        return std::hash<std::basic_string_view<std::remove_const_t<Ch>, Tr>>
                ()(x);
    }
};

} // end namespace std

namespace commata {

namespace detail::stored {

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

    // Throws nothing.
    void attach(Ch* buffer, std::size_t size)
    {
        assert(!buffer_);
        buffer_ = buffer;
        hwl_ = buffer_;
        end_ = buffer_ + size;
    }

    // Throws nothing.
    std::pair<Ch*, std::size_t> detach()
    {
        assert(buffer_);
        std::pair<Ch*, std::size_t> p(buffer_, end_ - buffer_);
        buffer_ = nullptr;
        return p;
    }

    std::pair<Ch*, Ch*> unsecured_range() const noexcept
    {
        return std::pair<Ch*, Ch*>(hwl_, end_);
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
    node_type* buffers_;        // "front" of buffers; "current" is the front
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
        const Allocator& alloc = Allocator()) noexcept :
        member_like_base<Allocator>(alloc),
        buffers_(nullptr), buffers_back_(nullptr), buffers_size_(0),
        buffers_cleared_(nullptr), buffers_cleared_back_(nullptr)
    {}

    table_store(table_store&& other) noexcept :
        table_store(std::allocator_arg, other.get_allocator(),
            std::move(other))
    {}

    // Requires allocators to be equal. Throws nothing.
    table_store(std::allocator_arg_t, const Allocator& alloc,
        table_store&& other) noexcept(at_t::is_always_equal::value) :
        member_like_base<Allocator>(alloc),
        buffers_             (std::exchange(other.buffers_, nullptr)),
        buffers_back_        (std::exchange(other.buffers_back_, nullptr)),
        buffers_size_        (std::exchange(other.buffers_size_, 0)),
        buffers_cleared_     (std::exchange(other.buffers_cleared_, nullptr)),
        buffers_cleared_back_(std::exchange(other.buffers_cleared_back_,
                                            nullptr))
    {}

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

    table_store& operator=(table_store&& other)
        noexcept(at_t::propagate_on_container_move_assignment::value
              || at_t::is_always_equal::value)
    {
        assert(at_t::propagate_on_container_move_assignment::value
            || (get_allocator() == other.get_allocator()));
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

    std::pair<Ch*, Ch*> get_current() noexcept
    {
        return buffers_ ?
            buffers_->unsecured_range() :
            std::pair<Ch*, Ch*>(nullptr, nullptr);
    }

    [[nodiscard]]
    std::pair<Ch*, std::size_t> generate_buffer(std::size_t min_size)
    {
        if (min_size > at_t::max_size(this->get())) {
            throw std::bad_alloc();
        }
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
    try {
        buffers_cleared_ = hello(p, size, buffers_cleared_);        // throw
        if (!buffers_cleared_back_) {
            buffers_cleared_back_ = buffers_cleared_;
        }
    } catch (...) {
        at_t::deallocate(this->get(), pt_t::pointer_to(*p), size);
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
        static_assert(std::is_trivially_destructible_v<node_type>);
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
        noexcept(at_t::propagate_on_container_swap::value
              || at_t::is_always_equal::value)
    {
        assert(at_t::propagate_on_container_swap::value
            || (get_allocator() == other.get_allocator()));
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
        noexcept(at_t::is_always_equal::value)
    {
        assert(get_allocator() == other.get_allocator());

        *(buffers_back_ ? &buffers_back_->next : &buffers_) =
            std::exchange(other.buffers_, nullptr);
        buffers_back_ = std::exchange(other.buffers_back_, nullptr);
        buffers_size_ += std::exchange(other.buffers_size_, 0);

        *(buffers_cleared_back_ ?
                &buffers_cleared_back_->next : &buffers_cleared_) =
            std::exchange(other.buffers_cleared_, nullptr);
        buffers_cleared_back_ =
            std::exchange(other.buffers_cleared_back_, nullptr);

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
        std::allocator_traits<Allocator>::propagate_on_container_swap::value
     || std::allocator_traits<Allocator>::is_always_equal::value)
{
    left.swap(right);
}

template <class T>
constexpr bool is_basic_stored_value_v = false;

template <class... Args>
constexpr bool is_basic_stored_value_v<basic_stored_value<Args...>> = true;

// Placed here to avoid bloat
template <class Tr>
struct null_termination
{
    template <class InputIterator>
    friend bool operator!=(InputIterator left, null_termination)
    {
        return !Tr::eq(*left, typename Tr::char_type());
    }
};

template <class Tr, class Ch>
void move_chs(const Ch* begin1, std::size_t size, Ch* begin2)
{
    Tr::move(begin2, begin1, size);
}

template <class Tr, class InputIterator, class Ch>
void move_chs(InputIterator begin1, std::size_t size, Ch* begin2)
{
    for (std::size_t i = 0; i < size; ++i) {
        Tr::assign(*begin2, *begin1);
        ++begin1;
        ++begin2;
    }
}

template <class InputIterator>
std::size_t distance(InputIterator begin, InputIterator end)
{
    return static_cast<std::size_t>(std::distance(begin, end));
}

template <class InputIterator, class InputIteratorEnd>
std::size_t distance(InputIterator begin, InputIteratorEnd end)
{
    std::size_t length = 0;
    while (begin != end) {
        ++begin;
        ++length;
    }
    return length;
}

template <class Tr, class InputIterator>
std::size_t length(InputIterator begin)
{
    return distance(begin, null_termination<Tr>());
}

template <class Tr>
std::size_t length(const char* begin)
{
    return Tr::length(begin);
}

struct is_equatable_with_impl
{
    template <class L, class R>
    static auto check(L*, R*) -> decltype(
        (std::declval<const L&>() == std::declval<const R&>()),
        std::true_type());

    static auto check(...) -> std::false_type;
};

template <class L, class R>
constexpr bool is_equatable_with_v =
    decltype(is_equatable_with_impl::check<L, R>(nullptr, nullptr))::value;

} // end detail::stored

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

    static_assert(detail::stored::is_basic_stored_value_v<value_type>,
        "Content shall be a sequence-container-of-sequence-container "
        "type of basic_stored_value");
    static_assert(
        std::is_same_v<content_type, typename Allocator::value_type>,
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
    using store_type = detail::stored::table_store<char_type, ca_t>;

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
        buffer_size_(sanitize_buffer_size(buffer_size, store_.get_allocator()))
    {}

    basic_stored_table(const basic_stored_table& other) :
        basic_stored_table(std::allocator_arg, 
            at_t::select_on_container_copy_construction(other.get_allocator()),
            other)
    {}

    basic_stored_table(std::allocator_arg_t, const Allocator& alloc,
        const basic_stored_table& other) :
        store_(std::allocator_arg, ca_t(ca_base_t(alloc))), records_(nullptr),
        buffer_size_(std::min(cat_t::max_size(ca_t(store_.get_allocator())),
            other.buffer_size_))
    {
        if (!other.records_) {
            // leave also *this moved-from
            return;
        }
        records_ = allocate_create_content(alloc);      // throw
        try {
            import_leaky(other.content(), content());   // throw
        } catch (...) {
            destroy_deallocate_content(alloc, records_);
            throw;
        }
    }

    basic_stored_table(basic_stored_table&& other) noexcept :
        store_(std::move(other.store_)),
        records_(std::exchange(other.records_, nullptr)),
        buffer_size_(other.buffer_size_)
    {}

    // Throws nothing if alloc == other.get_allocator().
    basic_stored_table(std::allocator_arg_t, const Allocator& alloc,
        basic_stored_table&& other) noexcept(at_t::is_always_equal::value) :
        store_(std::allocator_arg, ca_t(ca_base_t(alloc))), records_(nullptr),
        buffer_size_(std::min(cat_t::max_size(ca_t(store_.get_allocator())),
            other.buffer_size_))
    {
        if constexpr (!at_t::is_always_equal::value) {
            if (alloc != other.get_allocator()) {
                basic_stored_table(std::allocator_arg, alloc, other).
                    swap_force(*this);  // throw
                return;
            }
        }
        store_type s(std::allocator_arg,
            store_.get_allocator(), std::move(other.store_));
        store_.swap_force(s);
        using std::swap;
        swap(records_/*nullptr*/, other.records_);
    }

    ~basic_stored_table()
    {
        destroy_deallocate_content(get_allocator(), records_);
    }

private:
    static std::size_t sanitize_buffer_size(
        std::size_t buffer_size, const ca_t& alloc) noexcept
    {
        return std::max(
            static_cast<std::size_t>(2U),
            detail::sanitize_buffer_size(buffer_size, alloc));
    }

    template <class... Args>
    [[nodiscard]]
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
        Allocator a, typename at_t::pointer records) noexcept
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
            detail::stored::select_allocator(*this, other,
                typename at_t::propagate_on_container_copy_assignment()),
            other).swap_force(*this);   // throw
        return *this;
    }

    basic_stored_table& operator=(basic_stored_table&& other)
        noexcept(at_t::propagate_on_container_move_assignment::value
              || at_t::is_always_equal::value)
    {
        basic_stored_table(
            std::allocator_arg,
            detail::stored::select_allocator(*this, other,
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

    content_type& content()
    {
        assert(records_);
        return *records_;
    }

    const content_type& content() const
    {
        assert(records_);
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

    value_type& resize_value(value_type& value,
        typename value_type::size_type n)
    {
        if (n <= value.size()) {
            value.erase(value.cbegin() + n, value.cend());
        } else {
            const auto secured = secure_n(n + 1);   // throw
            detail::stored::move_chs<traits_type>(
                value.data(), value.size(), secured);
            traits_type::assign(secured + value.size(), n + 1 - value.size(),
                char_type());
            value = value_type(secured, secured + n);
        }
        return value;
    }

    value_type make_value(typename value_type::size_type n)
    {
        value_type value;
        resize_value(value, n); // throw
        return value;
    }

    template <class InputIterator, class InputIteratorEnd>
    value_type& rewrite_value(value_type& value,
        InputIterator new_value_begin, InputIteratorEnd new_value_end)
    {
        using it_cat =
            typename std::iterator_traits<InputIterator>::iterator_category;

        if constexpr (std::is_base_of_v<std::forward_iterator_tag, it_cat>) {
            return rewrite_value_n(value, new_value_begin,
                detail::stored::distance(new_value_begin, new_value_end));
        } else {
            return rewrite_value_input(value, new_value_begin, new_value_end);
        }
    }

    template <class InputIterator>
    auto rewrite_value(value_type& value, InputIterator new_value)
     -> std::enable_if_t<
            std::is_base_of_v<
                std::input_iterator_tag,
                typename std::iterator_traits<InputIterator>::
                    iterator_category>,
            value_type&>
    {
        using it_tr = std::iterator_traits<InputIterator>;
        using it_cat = typename it_tr::iterator_category;
        if constexpr (std::is_base_of_v<std::forward_iterator_tag, it_cat>) {
            return rewrite_value_n(value, new_value,
                detail::stored::length<traits_type>(new_value));
        } else {
            return rewrite_value(value, new_value,
                detail::stored::null_termination<traits_type>());
        }
    }

private:
    template <class InputIterator>
    value_type& rewrite_value_n(value_type& value,
        InputIterator new_value_begin, std::size_t new_value_size)
    {
        if constexpr (!std::is_const_v<typename value_type::value_type>) {
            if (new_value_size <= value.size()) {
                detail::stored::move_chs<traits_type>(
                    new_value_begin, new_value_size, value.begin());
                value.erase(value.cbegin() + new_value_size, value.cend());
                return value;
            }
        }
        const auto secured = secure_n(new_value_size + 1);
        detail::stored::move_chs<traits_type>(
            new_value_begin, new_value_size, secured);
        traits_type::assign(secured[new_value_size], char_type());
        value = value_type(secured, secured + new_value_size);
        return value;
    }

    char_type* secure_n(std::size_t n)
    {
        auto secured = store_.secure_any(n);
        if (!secured) {
            auto alloc_size = std::max(n, buffer_size_);
            std::tie(secured, alloc_size) = generate_buffer(alloc_size);
                                                // throw
            add_buffer(secured, alloc_size);    // throw
            // No need to deallocate secured even when an exception is
            // thrown by add_buffer because add_buffer consumes secured
            secure_current_upto(secured + n);
        }
        return secured;
    }

    class generated_buffer
    {
        char_type* generated_;
        std::size_t generated_size_;
        basic_stored_table* table_;

    public:
        explicit generated_buffer(basic_stored_table& table) noexcept :
            generated_(nullptr), table_(&table)
        {}

        ~generated_buffer() noexcept
        {
            purge();
        }

        void reset(char_type* generated, std::size_t generated_size) noexcept
        {
            purge();
            generated_ = generated;
            generated_size_ = generated_size;
        }

        void commit_if_any()
        {
            if (generated_) {
                table_->add_buffer(generated_, generated_size_);
                generated_ = nullptr;
            }
        }

    private:
        void purge() noexcept
        {
            if (generated_) {
                table_->consume_buffer(generated_, generated_size_);
            }
        }
    };

    template <class InputIterator, class InputIteratorEnd>
    value_type& rewrite_value_input(value_type& value,
        InputIterator new_value_begin, InputIteratorEnd new_value_end)
    {
        generated_buffer generated(*this);

        auto [cb, ce] = store_.get_current();
        if (!cb) {
            const auto [b, bn] = generate_buffer(buffer_size_);     // throw
            std::tie(cb, ce) = std::make_pair(b, b + bn);
            generated.reset(b, bn);
        }

        constexpr auto smax = std::numeric_limits<std::size_t>::max();

        char_type* i = cb;
        for (; new_value_begin != new_value_end; ++new_value_begin) {
            traits_type::assign(*i, *new_value_begin);
            ++i;
            if (i == ce) {
                const std::size_t cn = i - cb;
                const auto gn = (cn > smax / 2) ?
                    smax : std::max(2 * cn, buffer_size_);
                const auto [b, bn] = generate_buffer(gn);           // throw
                traits_type::move(b, cb, cn);
                std::tie(cb, i, ce) = std::make_tuple(b, b + cn, b + bn);
                generated.reset(b, bn);
            }
        }
        traits_type::assign(*i, char_type());
        generated.commit_if_any();                                  // throw
        secure_current_upto(i + 1);
        value = value_type(cb, i);
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
        rewrite_value(value, std::forward<Args>(args)...);  // throw
        return value;
    }

    template <class F>
    auto guard_rewrite(F f) -> decltype(f(*this))
    {
        const auto security = store_.get_security();    // throw
        try {
            return f(*this);                            // throw
        } catch (...) {
            store_.set_security(security);
            throw;
        }
    }

    size_type size() const
        noexcept(noexcept(std::declval<const content_type&>().size()))
    {
        return records_ ? content().size() : 0;
    }

    bool empty() const
        noexcept(noexcept(std::declval<const content_type&>().empty()))
    {
        return (!records_) || content().empty();
    }

    void clear()
        noexcept(noexcept(std::declval<content_type&>().clear()))
    {
        if (records_) {
            content().clear();
        }
        store_.clear();
    }

    void shrink_to_fit()
    {
        basic_stored_table(*this).swap(*this);     // throw
    }

    void swap(basic_stored_table& other)
        noexcept(cat_t::propagate_on_container_swap::value
              || cat_t::is_always_equal::value)
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

    template <class OtherContent, class OtherAllocator>
    basic_stored_table& operator+=(
        basic_stored_table<OtherContent, OtherAllocator>&& other)
    {
        return operator_plus_assign_impl(std::move(other));
    }

    [[nodiscard]]
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
        if (!records_) {
            if (other.records_) {
                basic_stored_table t(
                    std::allocator_arg, get_allocator());           // throw
                t.append_no_singular(
                    std::forward<OtherTable>(other));               // throw
                swap(t);
            }
        } else if (other.records_) {
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

    template <class OtherContent, class OtherAllocator>
    void append_no_singular(
        basic_stored_table<OtherContent, OtherAllocator>&& other)
    {
        using oca_t =
            typename basic_stored_table<OtherContent, OtherAllocator>::ca_t;
        if constexpr (std::is_same_v<ca_t, oca_t>
                   && cat_t::is_always_equal::value) {
            append_no_singular_merge(std::move(other));         // throw
        } else if constexpr (
                detail::stored::is_equatable_with_v<ca_t, oca_t>) {
            if (store_.get_allocator() == other.store_.get_allocator()) {
                append_no_singular_merge(std::move(other));     // throw
            } else {
                append_no_singular(other);                      // throw
            }
        } else {
            append_no_singular(other);                          // throw
        }
    }

    template <class ContentTo, class AllocatorTo>
    void copy_to(basic_stored_table<ContentTo, AllocatorTo>& to) const
    {
        // We require:
        // - if an exception is thrown by ContentTo's emplace() at the end,
        //   there are no effects on the before-end elements.
        // - ContentTo's erase() at the end does not throw an exception.

        to.guard_rewrite([c = std::addressof(content())](auto& t) {
            auto& tc = t.content();
            const auto original_size = tc.size();   // ?
            try {
                t.import_leaky(*c, tc);             // throw
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
            t.import_leaky(*c, r2);                     // throw
        });
        to.content().splice(to.content().cend(), r2);
    }

    template <class OtherContent>
    void import_leaky(const OtherContent& other, content_type& records)
    {
        reserve(records, records.size() + other.size());            // throw

        if constexpr (std::is_const_v<typename value_type::value_type>) {
            using va_t = typename at_t::template rebind_alloc<value_type>;
            using canon_t = std::unordered_set<value_type,
                std::hash<value_type>, std::equal_to<value_type>, va_t>;

            canon_t canonicals(va_t{get_allocator()});              // throw

            for (const auto& r : other) {
                const auto e = records.emplace(records.cend());     // throw
                reserve(*e, r.size());                              // throw
                for (const auto& v : r) {
                    const auto i = canonicals.find(v);
                    if (i == canonicals.cend()) {
                        const auto j =
                            canonicals.insert(import_value(v));     // throw
                        e->insert(e->cend(), *j.first);             // throw
                    } else {
                        e->insert(e->cend(), *i);                   // throw
                    }
                }
            }

        } else {
            for (const auto& r : other) {
                const auto e = records.emplace(records.cend());     // throw
                reserve(*e, r.size());                              // throw
                for (const auto& v : r) {
                    e->insert(e->cend(), import_value(v));          // throw
                }
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

    template <class OtherContent, class OtherAllocator>
    void append_no_singular_merge(
        basic_stored_table<OtherContent, OtherAllocator>&& other);

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

namespace detail { namespace stored {

template <class Container>

struct invoke_clear
{
    void operator()(Container* c)
    {
        c->clear();
    }
};

template <class ContainerFrom, class ContainerTo>
void  emigrate(ContainerFrom&& from, ContainerTo& to)
{
    static_assert(!std::is_reference_v<ContainerFrom>);
        // so we'll use move instead of forward

    std::unique_ptr<ContainerFrom, invoke_clear<ContainerFrom>> p(&from);
    if constexpr (std::is_same_v<typename ContainerFrom::value_type,
                                 typename ContainerTo::value_type>) {
        to.insert(to.end(), std::make_move_iterator(from.begin()),
                            std::make_move_iterator(from.end()));   // throw
    } else {
        for (const auto& f : from) {
            to.emplace(to.cend(), f.cbegin(), f.cend());            // throw
        }
    }
}

template <class ContentL, class ContentR>
void append_stored_table_content(ContentL& l, ContentR&& r)
{
    static_assert(!std::is_reference_v<ContentR>);
        // so we'll use move instead of forward

    // We require:
    // - if an exception is thrown by ContentL's emplace() or insert() at the
    //   end, there are no effects on the before-end elements.
    // - ContentL's erase() at the end does not throw an exception.
    // - ContentR's clear() does not throw an exception.

    const auto left_original_size = l.size();    // ?

    // Make sure right_content has no reference to moved values
    std::unique_ptr<ContentR, invoke_clear<ContentR>> p(&r);

    try {
        emigrate(std::move(r), l);              // throw
    } catch (...) {
        l.erase(std::next(l.begin(), left_original_size), l.cend());
        throw;
    }
}

template <class RecordL, class AllocatorL, class RecordR, class AllocatorR>
void append_stored_table_content(
    std::list<RecordL, AllocatorL>& l, std::list<RecordL, AllocatorR>&& r,
    std::nullptr_t = nullptr/*just a tag*/)
{
    std::list<RecordL, AllocatorL> r2(l.get_allocator());           // throw
    emigrate(std::move(r), r2);                                     // throw
    l.splice(l.cend(), r2);
}

template <class Record, class Allocator>
void append_stored_table_content(
    std::list<Record, Allocator>& l, std::list<Record, Allocator>&& r)
{
    // stores' allocator equality does not necessarily mean contents' one
    if constexpr (!std::allocator_traits<Allocator>::is_always_equal::value) {
        if (l.get_allocator() != r.get_allocator()) {
            append_stored_table_content(l, std::move(r), nullptr);  // throw
            return;
        }
    }
    l.splice(l.cend(), r);
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
    return std::move(left); // move is right for no copy elision would occur
}

}} // end namespace detail::stored

template <class Content, class Allocator>
template <class OtherContent, class OtherAllocator>
void basic_stored_table<Content, Allocator>::append_no_singular_merge(
    basic_stored_table<OtherContent, OtherAllocator>&& other)
{
    detail::stored::append_stored_table_content(
        content(), std::move(other.content()));         // throw
    store_.merge(std::move(other.store_));
}

template <class ContentL, class AllocatorL, class ContentR, class AllocatorR>
auto operator+(
    const basic_stored_table<ContentL, AllocatorL>& left,
    const basic_stored_table<ContentR, AllocatorR>& right)
{
    return detail::stored::plus_stored_table_impl(left, right);
}

template <class ContentL, class AllocatorL, class ContentR, class AllocatorR>
auto operator+(
    const basic_stored_table<ContentL, AllocatorL>& left,
    basic_stored_table<ContentR, AllocatorR>&& right)
{
    return detail::stored::plus_stored_table_impl(left, std::move(right));
}

template <class ContentL, class AllocatorL, class ContentR, class AllocatorR>
auto operator+(
    basic_stored_table<ContentL, AllocatorL>&& left,
    const basic_stored_table<ContentR, AllocatorR>& right)
{
    return detail::stored::plus_stored_table_impl(std::move(left), right);
}

template <class ContentL, class AllocatorL, class ContentR, class AllocatorR>
auto operator+(
    basic_stored_table<ContentL, AllocatorL>&& left,
    basic_stored_table<ContentR, AllocatorR>&& right)
{
    return detail::stored::plus_stored_table_impl(
        std::move(left), std::move(right));
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

namespace detail::stored {

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

} // end detail::stored

template <class Content, class Allocator,
    std::underlying_type_t<stored_table_builder_option> Options = 0U>
class stored_table_builder :
    detail::stored::arrange<Content, Options>
{
public:
    using table_type = basic_stored_table<Content, Allocator>;
    using char_type = typename table_type::char_type;

private:
    struct end_record_handler
    {
        virtual ~end_record_handler() {}
        virtual bool on_end_record(table_type& table) = 0;
    };

    template <class T>
    struct typed_end_record_handler :
        end_record_handler, private detail::member_like_base<T>
    {
        template <class U>
        explicit typed_end_record_handler(U&& t) :
            detail::member_like_base<T>(std::forward<U>(t))
        {}

        typed_end_record_handler(typed_end_record_handler&&) = delete;

        bool on_end_record(table_type& table)
        {
            return on_end_record_impl(this->get(), table);
        }

    private:
        template <class U>
        static auto on_end_record_impl(U& u, table_type& table)
         -> std::enable_if_t<std::is_invocable_v<U&, table_type&>, bool>
        {
            return on_end_record_impl_no_arg(
                std::bind(std::ref(u), std::ref(table)));
        }

        template <class U>
        static auto on_end_record_impl(U& u, table_type&)
         -> std::enable_if_t<
                !std::is_invocable_v<U&, table_type&>
             && std::is_invocable_v<U&>,
                bool>
        {
            return on_end_record_impl_no_arg(u);
        }

        template <class F>
        static auto on_end_record_impl_no_arg(F&& f)
         -> std::enable_if_t<
                std::is_void_v<decltype(std::forward<F>(f)())>, bool>
        {
            std::forward<F>(f)();
            return true;
        }

        template <class F>
        static auto on_end_record_impl_no_arg(F&& f)
         -> std::enable_if_t<
                !std::is_void_v<decltype(std::forward<F>(f)())>, bool>
        {
            return std::forward<F>(f)();
        }
    };

    using ph_t = typename std::allocator_traits<Allocator>::
                    template rebind_traits<end_record_handler>::pointer;

private:
    char_type* current_buffer_holder_;
    char_type* current_buffer_;
    std::size_t current_buffer_size_;

    char_type* field_begin_;
    char_type* field_end_;

    table_type* table_;

    ph_t end_record_;

public:
    explicit stored_table_builder(table_type& table,
                                  std::size_t max_record_num = 0) :
        detail::stored::arrange<Content, Options>(table.content()),
        current_buffer_holder_(nullptr), current_buffer_(nullptr),
        field_begin_(nullptr), table_(std::addressof(table)),
        end_record_((max_record_num > 0) ?
            allocate_construct(
                [remaining = max_record_num](table_type&) mutable {
                    if (remaining == 1) {
                        return false;
                    } else {
                        --remaining;
                        return true;
                    }
                }) : nullptr)
    {}

    template <class E,
              std::enable_if_t<!std::is_integral_v<std::decay_t<E>>>*
                  = nullptr>
    stored_table_builder(table_type& table, E&& e) :
        detail::stored::arrange<Content, Options>(table.content()),
        current_buffer_holder_(nullptr), current_buffer_(nullptr),
        field_begin_(nullptr), table_(std::addressof(table)),
        end_record_(allocate_construct(std::forward<E>(e)))
    {}

    stored_table_builder(stored_table_builder&& other) noexcept :
        detail::stored::arrange<Content, Options>(other),
        current_buffer_holder_(std::exchange(other.current_buffer_holder_,
                                             nullptr)),
        current_buffer_(other.current_buffer_),
        current_buffer_size_(other.current_buffer_size_),
        field_begin_(other.field_begin_), field_end_(other.field_end_),
        table_(other.table_),
        end_record_(std::exchange(other.end_record_, nullptr))
    {}

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
        if (end_record_) {
            destroy_deallocate(end_record_);
        }
    }

private:
    template <class T>
    ph_t allocate_construct(T&& t)
    {
        using dt_t = std::decay_t<T>;
        using h_t = typed_end_record_handler<dt_t>;

        using at_t = typename std::allocator_traits<Allocator>::
                        template rebind_traits<h_t>;
        using a_t = typename at_t::allocator_type;
        a_t a(table_->get_allocator());

        const auto p = at_t::allocate(a, 1);                    // throw
        try {
            ::new (std::addressof(*p)) h_t(std::forward<T>(t)); // throw
        } catch (...) {
            at_t::deallocate(a, p, 1);
            throw;
        }
        return p;
    }

    void destroy_deallocate(ph_t p) noexcept
    {
        using at_t = typename std::allocator_traits<Allocator>::
                        template rebind_traits<end_record_handler>;
        using a_t = typename at_t::allocator_type;
        a_t a(table_->get_allocator());

        p->~end_record_handler();
        at_t::deallocate(a, p, 1);
    }

public:
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

    bool end_record(const char_type* /*record_end*/)
    {
        return (!end_record_) || end_record_->on_end_record(*table_);
    }

    [[nodiscard]] std::pair<char_type*, std::size_t> get_buffer()
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
    std::size_t get_next_buffer_size(std::size_t occupied) const
    {
        constexpr std::size_t max = std::numeric_limits<std::size_t>::max();
        std::size_t next = table_->get_buffer_size();
        for (; occupied >= next / 2; next *= 2) {
            if (next >= max / 2 + 1) {  // if max==100, next>=51;
                                        // if max==101, next>=51
                if (occupied <= max - 2) {  // must have 2 more elements
                    return max;
                } else {
                    throw std::bad_alloc();
                }
            }
        }
        return next;
    }

public:
    void release_buffer(const char_type* /*buffer*/) noexcept
    {}
};

template <class Content, class Allocator, class... Args>
auto make_stored_table_builder(
    basic_stored_table<Content, Allocator>& table, Args&&... args)
{
    return stored_table_builder(table, std::forward<Args>(args)...);
}

template <class Content, class Allocator, class... Args>
auto make_transposed_stored_table_builder(
    basic_stored_table<Content, Allocator>& table, Args&&... args)
{
    return stored_table_builder<Content, Allocator,
        stored_table_builder_option_transpose>(
            table, std::forward<Args>(args)...);
}

}

#endif
