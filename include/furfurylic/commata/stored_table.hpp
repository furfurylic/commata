/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_GUARD_44AB64F1_C45A_45AC_9277_F2735CCE832E
#define FURFURYLIC_GUARD_44AB64F1_C45A_45AC_9277_F2735CCE832E

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
#include <type_traits>
#include <utility>
#include <vector>

#include "formatted_output.hpp"
#include "key_chars.hpp"
#include "member_like_base.hpp"
#include "propagation_controlled_allocator.hpp"

namespace furfurylic {
namespace commata {

template <class Ch, class Tr = std::char_traits<Ch>>
class basic_stored_value
{
    Ch* begin_;
    Ch* end_;   // must point the terminating zero

    static Ch empty_value[];

public:
    static_assert(std::is_same<Ch, typename Tr::char_type>::value, "");

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

template <class Ch, class Tr>
bool operator==(
    const basic_stored_value<Ch, Tr>& left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return detail::stored_value_eq(left, right);
}

template <class Ch, class Tr>
bool operator==(
    const basic_stored_value<Ch, Tr>& left,
    const Ch* right) noexcept
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

template <class Ch, class Tr, class Allocator>
bool operator==(
    const basic_stored_value<Ch, Tr>& left,
    const std::basic_string<Ch, Tr, Allocator>& right) noexcept
{
    return detail::stored_value_eq(left, right);
}

template <class Ch, class Tr>
bool operator==(
    const Ch* left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return right == left;
}

template <class Ch, class Tr, class Allocator>
bool operator==(
    const std::basic_string<Ch, Tr, Allocator>& left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return right == left;
}

template <class Ch, class Tr>
bool operator!=(
    const basic_stored_value<Ch, Tr>& left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return !(left == right);
}

template <class Ch, class Tr>
bool operator!=(
    const basic_stored_value<Ch, Tr>& left,
    const Ch* right) noexcept
{
    return !(left == right);
}

template <class Ch, class Tr, class Allocator>
bool operator!=(
    const basic_stored_value<Ch, Tr>& left,
    const std::basic_string<Ch, Tr, Allocator>& right) noexcept
{
    return !(left == right);
}

template <class Ch, class Tr>
bool operator!=(
    const Ch* left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return !(left == right);
}

template <class Ch, class Tr, class Allocator>
bool operator!=(
    const std::basic_string<Ch, Tr, Allocator>& left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return !(left == right);
}

template <class Ch, class Tr>
bool operator<(
    const basic_stored_value<Ch, Tr>& left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return detail::stored_value_lt(left, right);
}

template <class Ch, class Tr>
bool operator<(
    const basic_stored_value<Ch, Tr>& left,
    const Ch* right) noexcept
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

template <class Ch, class Tr>
bool operator<(
    const basic_stored_value<Ch, Tr>& left,
    const std::basic_string<Ch, Tr>& right) noexcept
{
    return detail::stored_value_lt(left, right);
}

template <class Ch, class Tr>
bool operator<(
    const Ch* left,
    const basic_stored_value<Ch, Tr>& right) noexcept
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

template <class Ch, class Tr>
bool operator<(
    const std::basic_string<Ch, Tr>& left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return detail::stored_value_lt(left, right);
}

template <class Ch, class Tr>
bool operator>(
    const basic_stored_value<Ch, Tr>& left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return right < left;
}

template <class Ch, class Tr>
bool operator>(
    const basic_stored_value<Ch, Tr>& left,
    const Ch* right) noexcept
{
    return right < left;
}

template <class Ch, class Tr>
bool operator>(
    const basic_stored_value<Ch, Tr>& left,
    const std::basic_string<Ch, Tr>& right) noexcept
{
    return right < left;
}

template <class Ch, class Tr>
bool operator>(
    const Ch* left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return right < left;
}

template <class Ch, class Tr>
bool operator>(
    const std::basic_string<Ch, Tr>& left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return right < left;
}

template <class Ch, class Tr>
bool operator<=(
    const basic_stored_value<Ch, Tr>& left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return !(right < left);
}

template <class Ch, class Tr>
bool operator<=(
    const basic_stored_value<Ch, Tr>& left,
    const Ch* right) noexcept
{
    return !(right < left);
}

template <class Ch, class Tr>
bool operator<=(
    const basic_stored_value<Ch, Tr>& left,
    const std::basic_string<Ch, Tr>& right) noexcept
{
    return !(right < left);
}

template <class Ch, class Tr>
bool operator<=(
    const Ch* left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return !(right < left);
}

template <class Ch, class Tr>
bool operator<=(
    const std::basic_string<Ch, Tr>& left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return !(right < left);
}

template <class Ch, class Tr>
bool operator>=(
    const basic_stored_value<Ch, Tr>& left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return !(left < right);
}

template <class Ch, class Tr>
bool operator>=(
    const basic_stored_value<Ch, Tr>& left,
    const Ch* right) noexcept
{
    return !(left < right);
}

template <class Ch, class Tr>
bool operator>=(
    const basic_stored_value<Ch, Tr>& left,
    const std::basic_string<Ch, Tr>& right) noexcept
{
    return !(left < right);
}

template <class Ch, class Tr>
bool operator>=(
    const Ch* left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return !(left < right);
}

template <class Ch, class Tr>
bool operator>=(
    const std::basic_string<Ch, Tr>& left,
    const basic_stored_value<Ch, Tr>& right) noexcept
{
    return !(left < right);
}

template <class Ch, class Tr>
std::basic_ostream<Ch, Tr>& operator<<(
    std::basic_ostream<Ch, Tr>& os, const basic_stored_value<Ch, Tr>& o)
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

}}

namespace std {

template <class Ch, class Tr>
struct hash<furfurylic::commata::basic_stored_value<Ch, Tr>>
{
    std::size_t operator()(
        const furfurylic::commata::basic_stored_value<Ch, Tr>& x)
            const noexcept
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

namespace furfurylic { namespace commata {

namespace detail {

template <class T>
struct is_std_vector :
    std::false_type
{};

template <class... Args>
struct is_std_vector<std::vector<Args...>> :
    std::true_type
{};

template <class T>
struct is_std_deque :
    std::false_type
{};

template <class... Args>
struct is_std_deque<std::deque<Args...>> :
    std::true_type
{};

template <class T>
struct is_std_list :
    std::false_type
{};

template <class... Args>
struct is_std_list<std::list<Args...>> :
    std::true_type
{};

template <class T>
struct is_nothrow_swappable :
    std::integral_constant<bool,
        // According to C++14 23.2.1 (11), STL containers throw no exceptions
        // with their swap()
        is_std_vector<T>::value
     || is_std_deque<T>::value
     || is_std_list<T>::value
     || noexcept(std::declval<T&>().swap(std::declval<T&>()))>
        // We would like to declare using std::swap; and test swap(a, b),
        // but freaking VS2015 dislikes it
        // (in fact we shouldn't care since all containers shall have member
        // function swap)
{};

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
        buffers_(nullptr), buffers_back_(nullptr),
        buffers_size_(0)
    {}

    table_store(table_store&& other) noexcept :
        table_store(std::allocator_arg, other.get_allocator(),
            std::move(other))
    {}

    table_store(std::allocator_arg_t, const Allocator& alloc,
        table_store&& other) noexcept :
        member_like_base<Allocator>(alloc),
        buffers_(other.buffers_), buffers_back_(other.buffers_back_),
        buffers_size_(other.buffers_size_)
    {
        assert(alloc == other.get_allocator());
        other.buffers_ = nullptr;
        other.buffers_back_ = nullptr;
        other.buffers_size_ = 0;
    }

    ~table_store()
    {
        while (buffers_) {
            reduce_buffer();
        }
    }

    table_store& operator=(table_store&& other) noexcept
    {
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
        typename nat_t::allocator_type na(this->get());

        // "push_front"-like behaviour
        const auto buffers0 = buffers_;
        try {
            buffers_ = std::addressof(*nat_t::allocate(na, 1)); // throw
        } catch (...) {
            at_t::deallocate(this->get(), pt_t::pointer_to(*buffer), size);
            throw;
        }
        ::new(buffers_) node_type(buffers0);
        buffers_->attach(buffer, size);
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

    void clear() noexcept
    {
        for (auto i = buffers_; i; i = i->next) {
            i->clear();
        }
    }

    void swap(table_store& other) noexcept
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

    table_store& merge(table_store&& other) noexcept
    {
        assert(this->get() == other.get());
        if (buffers_back_) {
            assert(!buffers_back_->next);
            buffers_back_->next = other.buffers_;
        } else {
            assert(!buffers_);
            buffers_ = other.buffers_;
            buffers_back_ = buffers_;
        }
        buffers_size_ += other.buffers_size_;
        other.buffers_ = nullptr;
        other.buffers_back_ = nullptr;
        other.buffers_size_ = 0;
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
            reduce_buffer();
        }

        auto i = buffers_;
        auto j = s.cbegin();
        for (; i; i = i->next, ++j) {
            i->secure_upto(*j);
        }
        assert(i == nullptr);
    }

private:
    void reduce_buffer() noexcept
    {
        assert(buffers_);

        typename nat_t::allocator_type na(this->get());

        auto next = buffers_->next;
        const auto p = buffers_->detach();
        at_t::deallocate(this->get(), pt_t::pointer_to(*p.first), p.second);
        static_assert(std::is_trivially_destructible<node_type>::value, "");
        nat_t::deallocate(na, npt_t::pointer_to(*buffers_), 1);
        buffers_ = next;
    }

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
    }
};

template <class Ch, class Allocator>
void swap(
    table_store<Ch, Allocator>& left,
    table_store<Ch, Allocator>& right) noexcept
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

} // end namespace detail

template <class Content, class Allocator = std::allocator<Content>>
class basic_stored_table
{
public:
    using allocator_type  = Allocator;
    using content_type    = Content;
    using record_type     = typename content_type::value_type;
    using value_type      = typename record_type::value_type;
    using char_type       = typename value_type::value_type;
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
    explicit basic_stored_table(
        std::size_t buffer_size = default_buffer_size) :
        basic_stored_table(std::allocator_arg, Allocator(), buffer_size)
    {}

    explicit basic_stored_table(std::allocator_arg_t,
        const Allocator& alloc = Allocator(),
        std::size_t buffer_size = default_buffer_size) :
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
        store_(std::allocator_arg, ca_t(ca_base_t(alloc))),
        records_(nullptr), buffer_size_(other.buffer_size_)
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

    basic_stored_table(basic_stored_table&& other) noexcept :
        store_(std::move(other.store_)), records_(other.records_),
        buffer_size_(other.buffer_size_)
    {
        other.records_ = nullptr;
    }

    basic_stored_table(std::allocator_arg_t, const Allocator& alloc,
        basic_stored_table&& other) :
        store_(std::allocator_arg, ca_t(ca_base_t(alloc))),
        records_(nullptr), buffer_size_(other.buffer_size_)
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

    template <class ForwardIterator>
    auto rewrite_value(value_type& value,
        ForwardIterator new_value_begin, ForwardIterator new_value_end)
     -> std::enable_if_t<
            std::is_base_of<
                std::forward_iterator_tag,
                typename std::iterator_traits<ForwardIterator>::
                    iterator_category>::value,
            value_type&>
    {
        return rewrite_value_n(value, new_value_begin,
            static_cast<std::size_t>(
                std::distance(new_value_begin, new_value_end)));
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
        return rewrite_value_n(value, new_value, length_to_null(new_value));
    }

private:
    template <class InputIterator>
    std::size_t length_to_null(InputIterator i)
    {
        std::size_t length = 0;
        while (!traits_type::eq(*i, char_type())) {
            ++i;
            ++length;
        }
        return length;
    }

    std::size_t length_to_null(const char_type* p)
    {
        return traits_type::length(p);
    }

    template <class InputIterator>
    value_type& rewrite_value_n(value_type& value,
        InputIterator new_value_begin, std::size_t new_value_size)
    {
        // We require that [value.begin(), value.end()) and
        // [new_value_begin, new_value_begin + new_value_size) do not overlap

        if (new_value_size <= value.size()) {
            traits_type::copy(value.begin(), new_value_begin, new_value_size);
            value.erase(value.cbegin() + new_value_size, value.cend());
        } else {
            auto secured = store_.secure_any(new_value_size + 1);
            if (!secured) {
                const auto alloc_size =
                    std::max(new_value_size + 1, buffer_size_);
                secured = allocate_buffer(alloc_size);   // throw
                add_buffer(secured, alloc_size);         // throw
                // No need to deallocate secured even when an exception is
                // thrown by add_buffer because add_buffer consumes secured
                secure_current_upto(secured + new_value_size + 1);
            }
            traits_type::copy(secured, new_value_begin, new_value_size);
            traits_type::assign(secured[new_value_size], char_type());
            value = value_type(secured, secured + new_value_size);
        }
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

    void swap(basic_stored_table& other) noexcept
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

    char_type* allocate_buffer(std::size_t size)
    {
        auto a = store_.get_allocator();
        return std::addressof(*cat_t::allocate(a, size));
    }

    void deallocate_buffer(char_type* p, std::size_t size)
    {
        auto a = store_.get_allocator();
        return cat_t::deallocate(
            a,
            std::pointer_traits<typename cat_t::pointer>::pointer_to(*p),
            size);
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
            t.swap(*this);
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

        to.guard_rewrite([c = &content()](auto& t) {
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
        to.guard_rewrite([&r2, c = &content()](auto& t) {
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
    basic_stored_table<Content, Allocator>& right)
    noexcept(noexcept(left.swap(right)))
{
    left.swap(right);
}

namespace detail {

#ifndef NDEBUG

template <class T, class = void>
struct has_non_pocs_allocator_type :
    std::false_type
{};

template <class T>
struct has_non_pocs_allocator_type<T,
        std::enable_if_t<
            !T::allocator_type::propagate_on_container_swap::value>> :
    std::true_type
{};

struct is_allocator_aware_impl
{
    template <class T>
    static std::true_type check(typename T::allocator_type* = nullptr);

    template <class T>
    static std::false_type check(...);
};

template <class T>
struct is_allocator_aware :
    decltype(is_allocator_aware_impl::check<T>(nullptr))
{};

template <class T>
auto have_inequal_allocators(const T& l, const T& r)
 -> std::enable_if_t<is_allocator_aware<T>::value, bool>
{
    return l.get_allocator() != r.get_allocator();
}

template <class T>
auto have_inequal_allocators(const T& l, const T& r)
 -> std::enable_if_t<!is_allocator_aware<T>::value, bool>
{
    return false;
}

#endif

template <class T>
auto nothrow_emigrate(T& from, T& to)
 -> std::enable_if_t<!std::is_nothrow_move_assignable<T>::value>
{
    assert(!has_non_pocs_allocator_type<T>::value
        && !have_inequal_allocators(from, to));
    from.swap(to);
}

template <class T>
auto nothrow_emigrate(T& from, T& to)
 -> std::enable_if_t<std::is_nothrow_move_assignable<T>::value>
{
    to = std::move(from);
}

template <class T>
using is_nothrow_emigratable =
    std::integral_constant<bool,
        std::is_nothrow_move_assignable<T>::value
     || is_nothrow_swappable<T>::value>;

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
auto append_stored_table_content_adaptive(ContentL& l, ContentR&& r)
 -> std::enable_if_t<
        is_nothrow_emigratable<typename ContentL::value_type>::value>
{
    static_assert(std::is_same<typename ContentL::value_type,
        typename ContentR::value_type>::value, "");

    // We require:
    // - if an exception is thrown by ContentL's emplace() at the end,
    //   there are no effects on the before-end elements.
    // - ContentL's erase() at the end does not throw an exception.
    // - ContentR's erase() at the end does not throw an exception.
    // - If ContentL::value_type is not nothrow-move-assigable and allocator-
    //   aware, the allocator type has true_type for propagate_on_container_-
    //   swap or has is_always_equal property

    const auto left_original_size = l.size();    // ?
    try {
        // We do move-emplace instead of emplace-and-swap because it seems
        // unlikely that default-construct-then-swap is less exception-prone
        // than move-construct
        for (auto& rr : r) {
            l.emplace(l.cend(), std::move(rr));         // throw
        }
    } catch (...) {
        const auto le = std::next(l.begin(), left_original_size);
        for (auto i = le, ie = l.end(), j = r.begin(); i != ie; ++i, ++j) {
            nothrow_emigrate(*i, *j);
        }
        l.erase(le, l.cend());
        throw;
    }

    // Make sure right_content has no reference to moved values;
    // we use erase() instead of clear() to simplify requirements
    r.erase(r.cbegin(), r.cend());
}

template <class Record, class AllocatorL, class ContentR>
auto append_stored_table_content_adaptive(
    std::list<Record, AllocatorL>& l, ContentR&& r)
 -> std::enable_if_t<is_nothrow_emigratable<Record>::value>
{
    static_assert(
        std::is_same<Record, typename ContentR::value_type>::value, "");

    // We require:
    // - If Record is not nothrow-move-assigable and allocator-aware, the
    //   allocator type has true_type for propagate_on_container_swap or has
    //   is_always_equal property

    std::list<Record, AllocatorL> r2(l.get_allocator());    // throw
    try {
        // We do move-emplace instead of emplace-and-swap because it seems
        // unlikely that default-construct-then-swap is less exception-prone
        // than move-construct
        for (auto& rr : r) {
            r2.push_back(std::move(rr));                    // throw
        }
    } catch (...) {
        for (auto i = r2.begin(), ie = r2.end(), j = r.begin();
                i != ie; ++i, ++j) {
            nothrow_emigrate(*i, *j);
        }
        throw;
    }
    l.splice(l.cend(), r2);
}

template <class ContentL, class ContentR>
auto append_stored_table_content_adaptive(ContentL& l, ContentR&& r)
 -> std::enable_if_t<
        !is_nothrow_emigratable<typename ContentL::value_type>::value>
{
    append_stored_table_content_primitive(l, r);
}

template <class ContentL, class ContentR>
auto append_stored_table_content(ContentL& l, ContentR&& r)
 -> std::enable_if_t<!std::is_same<
        typename ContentL::value_type, typename ContentR::value_type>::value>
{
    append_stored_table_content_primitive(l, r);
}

template <class ContentL, class ContentR>
auto append_stored_table_content(ContentL& l, ContentR&& r)
 -> std::enable_if_t<std::is_same<
        typename ContentL::value_type, typename ContentR::value_type>::value>
{
    append_stored_table_content_adaptive(l, std::move(r));
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

template <class T>
struct is_basic_stored_table : std::false_type
{};

template <class Content, class Allocator>
struct is_basic_stored_table<basic_stored_table<Content, Allocator>> :
    std::true_type
{};

template <class TableL, class TableR>
auto plus_stored_table_impl(TableL&& left, TableR&& right)
{
    std::decay_t<TableL> l(std::forward<TableL>(left));     // throw
    l += std::forward<TableR>(right);                       // throw
    return l;
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
        for (auto& rec : content) {
            rec.insert(rec.cend(), i_ - rec.size(),
                typename record_t::value_type());   // throw
        }
        j_ = content.begin();
    }

    void new_value(Content& content, char_type* first, char_type* last)
    {
        assert(i_ > 0);
        if (content.end() == j_) {
            j_ = content.emplace(content.cend(),
                i_, typename record_t::value_type());   // throw
        }
        *j_->rbegin() = typename record_t::value_type(first, last);
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
        field_begin_(nullptr), table_(&table)
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
            table_->deallocate_buffer(
                current_buffer_holder_, current_buffer_size_);
        }
    }

    void start_record(const char_type* /*record_begin*/)
    {
        this->new_record(table_->content());    // throw
    }

    bool update(const char_type* first, const char_type* last)
    {
        if (field_begin_) {
            table_type::traits_type::move(field_end_, first, last - first);
            field_end_ += last - first;
        } else {
            field_begin_ = current_buffer_ + (first - current_buffer_);
            field_end_   = current_buffer_ + (last  - current_buffer_);
        }
        return true;
    }

    bool finalize(const char_type* first, const char_type* last)
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
        return true;
    }

    bool end_record(const char_type* /*record_end*/)
    {
        return true;
    }

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
                const auto next_buffer =
                    table_->allocate_buffer(next_buffer_size);      // throw
                traits_t::copy(next_buffer, field_begin_, length);
                if (current_buffer_holder_) {
                    table_->deallocate_buffer(
                        current_buffer_holder_, current_buffer_size_);
                }
                current_buffer_holder_ = next_buffer;
                current_buffer_size_ = next_buffer_size;
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
                current_buffer_holder_ = table_->allocate_buffer(
                    table_->get_buffer_size());                     // throw
                current_buffer_size_ = table_->get_buffer_size();
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

}}

#endif
