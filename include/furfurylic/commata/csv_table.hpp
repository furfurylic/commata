/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_GUARD_44AB64F1_C45A_45AC_9277_F2735CCE832E
#define FURFURYLIC_GUARD_44AB64F1_C45A_45AC_9277_F2735CCE832E

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <deque>
#include <functional>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "formatted_output.hpp"
#include "key_chars.hpp"
#include "member_like_base.hpp"

namespace furfurylic {
namespace commata {

template <class Ch, class Tr = std::char_traits<Ch>>
class basic_csv_value
{
    Ch* begin_;
    Ch* end_;   // must point the terminating zero

    static Ch empty_value[];

public:
    using value_type      = Ch;
    using reference       = Ch&;
    using const_reference = const Ch&;
    using iterator        = Ch*;
    using const_iterator  = const Ch*;
    using difference_type = std::ptrdiff_t;
    using size_type       = std::size_t;
    using traits_type     = Tr;

    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr size_type npos = static_cast<size_type>(-1);

    basic_csv_value() noexcept :
        begin_(empty_value), end_(empty_value)
    {
        assert(*end_ == Ch());
    }

    basic_csv_value(Ch* begin, const Ch* end) noexcept :
        begin_(begin), end_(begin + (end - begin))
    {
        assert(*end_ == Ch());
    }

    basic_csv_value(const basic_csv_value& other) noexcept :
        begin_(other.begin_), end_(other.end_)
    {}

    basic_csv_value& operator=(const basic_csv_value& other) noexcept
    {
        begin_ = other.begin_;
        end_ = other.end_;
        return *this;
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
        check_pos(pos); // throw
        return (*this)[pos];
    }

    const_reference at(size_type pos) const
    {
        // ditto
        check_pos(pos); // throw
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

    basic_csv_value& erase(size_type pos = 0, size_type n = npos)
    {
        check_pos(pos); // throw
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

    void swap(basic_csv_value& other) noexcept
    {
        std::swap(begin_, other.begin_);
        std::swap(end_,   other.end_);
    }

private:
    void check_pos(size_type pos) const
    {
        if (pos >= size()) {
            throw std::out_of_range(
                std::to_string(pos) + " is too large for this value");
        }
    }
};

template <class Ch, class Tr>
constexpr typename basic_csv_value<Ch, Tr>::size_type
    basic_csv_value<Ch, Tr>::npos;

template <class Ch, class Tr>
Ch basic_csv_value<Ch, Tr>::empty_value[] = { Ch() };

template <class Ch, class Tr>
void swap(
    basic_csv_value<Ch, Tr>& left, basic_csv_value<Ch, Tr>& right) noexcept
{
    return left.swap(right);
}

namespace detail {

template <class Left, class Right>
bool csv_value_eq(const Left& left, const Right& right) noexcept
{
    return std::equal(
        left.cbegin(), left.cend(), right.cbegin(), right.cend(),
            [](auto l, auto r) { return Left::traits_type::eq(l, r); });
}

template <class Left, class Right>
bool csv_string_lt(const Left& left, const Right& right) noexcept
{
    return std::lexicographical_compare(
        left.cbegin(), left.cend(), right.cbegin(), right.cend(),
            [](auto l, auto r) { return Left::traits_type::lt(l, r); });
}

} // end namespace detail

template <class Ch, class Tr>
bool operator==(
    const basic_csv_value<Ch, Tr>& left,
    const basic_csv_value<Ch, Tr>& right) noexcept
{
    return detail::csv_value_eq(left, right);
}

template <class Ch, class Tr>
bool operator==(
    const basic_csv_value<Ch, Tr>& left,
    const Ch* right) noexcept
{
    // If left is "abc\0def" and right is "abc" followed by '\0'
    // then left == right shall be false
    // and any overrun on right must not occur
    auto i = left .cbegin();
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
    const basic_csv_value<Ch, Tr>& left,
    const std::basic_string<Ch, Tr, Allocator>& right) noexcept
{
    return detail::csv_value_eq(left, right);
}

template <class Left, class Ch, class Tr>
bool operator==(
    const Left& left,
    const basic_csv_value<Ch, Tr>& right) noexcept
{
    return right == left;
}

template <class Ch, class Tr>
bool operator!=(
    const basic_csv_value<Ch, Tr>& left,
    const basic_csv_value<Ch, Tr>& right) noexcept
{
    return !(left == right);
}

template <class Ch, class Tr, class Right>
bool operator!=(
    const basic_csv_value<Ch, Tr>& left,
    const Right& right) noexcept
{
    return !(left == right);
}

template <class Left, class Ch, class Tr>
bool operator!=(
    const Left& left,
    const basic_csv_value<Ch, Tr>& right) noexcept
{
    return !(left == right);
}

template <class Ch, class Tr>
bool operator<(
    const basic_csv_value<Ch, Tr>& left,
    const basic_csv_value<Ch, Tr>& right) noexcept
{
    return detail::csv_string_lt(left, right);
}

template <class Ch, class Tr>
bool operator<(
    const basic_csv_value<Ch, Tr>& left,
    const Ch* right) noexcept
{
    for (auto l : left) {
        if (Tr::eq(*right, Ch())) {
            return false;
        } else if (Tr::lt(l, *right)) {
            return true;
        } else if (Tr::lt(*right, l)) {
            return false;
        }
        ++right;
    }
    return !Tr::eq(*right, Ch());
}

template <class Ch, class Tr>
bool operator<(
    const Ch* left,
    const basic_csv_value<Ch, Tr>& right) noexcept
{
    for (auto r : right) {
        if (Tr::eq(*left, Ch())) {
            return true;
        } else if (Tr::lt(*left, r)) {
            return true;
        } else if (Tr::lt(r, *left)) {
            return false;
        }
        ++left;
    }
    return false;   // at least left == right
}

template <class Ch, class Tr>
bool operator<(
    const basic_csv_value<Ch, Tr>& left,
    const std::basic_string<Ch, Tr>& right) noexcept
{
    return detail::csv_string_lt(left, right);
}

template <class Ch, class Tr>
bool operator<(
    const std::basic_string<Ch, Tr>& left,
    const basic_csv_value<Ch, Tr>& right) noexcept
{
    return detail::csv_string_lt(left, right);
}

template <class Ch, class Tr>
bool operator>(
    const basic_csv_value<Ch, Tr>& left,
    const basic_csv_value<Ch, Tr>& right) noexcept
{
    return right < left;
}

template <class Ch, class Tr, class Right>
bool operator>(
    const basic_csv_value<Ch, Tr>& left,
    const Right& right) noexcept
{
    return right < left;
}

template <class Left, class Ch, class Tr>
bool operator>(
    const Left& left,
    const basic_csv_value<Ch, Tr>& right) noexcept
{
    return right < left;
}

template <class Ch, class Tr>
bool operator<=(
    const basic_csv_value<Ch, Tr>& left,
    const basic_csv_value<Ch, Tr>& right) noexcept
{
    return !(right < left);
}

template <class Ch, class Tr, class Right>
bool operator<=(
    const basic_csv_value<Ch, Tr>& left,
    const Right& right) noexcept
{
    return !(right < left);
}

template <class Left, class Ch, class Tr>
bool operator<=(
    const Left& left,
    const basic_csv_value<Ch, Tr>& right) noexcept
{
    return !(right < left);
}

template <class Ch, class Tr>
bool operator>=(
    const basic_csv_value<Ch, Tr>& left,
    const basic_csv_value<Ch, Tr>& right) noexcept
{
    return !(left < right);
}

template <class Ch, class Tr, class Right>
bool operator>=(
    const basic_csv_value<Ch, Tr>& left,
    const Right& right) noexcept
{
    return !(left < right);
}

template <class Left, class Ch, class Tr>
bool operator>=(
    const Left& left,
    const basic_csv_value<Ch, Tr>& right) noexcept
{
    return !(left < right);
}

template <class Ch, class Tr>
std::basic_ostream<Ch, Tr>& operator<<(
    std::basic_ostream<Ch, Tr>& os, const basic_csv_value<Ch, Tr>& o)
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

using csv_value = basic_csv_value<char, std::char_traits<char>>;
using wcsv_value = basic_csv_value<wchar_t, std::char_traits<wchar_t>>;

}}

namespace std {

template <class Ch, class Tr>
struct hash<furfurylic::commata::basic_csv_value<Ch, Tr>>
{
    std::size_t operator()(
        const furfurylic::commata::basic_csv_value<Ch, Tr>& x) const noexcept
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

template <class Ch, class Allocator>
class csv_store :
    member_like_base<Allocator>
{
    class buffer_type
    {
        Ch* buffer_;
        Ch* hwl_;   // high water level: last-past-one of the used elements
        Ch* end_;   // last-past-one of the buffer_

    public:
        buffer_type() noexcept :
            buffer_(nullptr)
        {}

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

    struct node_type : buffer_type
    {
        node_type* next;

        explicit node_type(node_type* n) :
            next(n)
        {}
    };

    // At this time we adopt a homemade forward list
    // taking advantage of its constant-time and nofail "splice" ability
    // and its nofail move construction and move assignment
    node_type* buffers_;        // "front" of buffers
    node_type* buffers_back_;   // "back" of buffers
    std::size_t buffers_size_;  // "size" of buffers

private:
    using at_t = std::allocator_traits<Allocator>;
    using pt_t = std::pointer_traits<typename at_t::pointer>;
    using nat_t = typename at_t::template rebind_traits<node_type>;
    using npt_t = std::pointer_traits<typename nat_t::pointer>;

public:
    using allocator_type = Allocator;
    using security = std::vector<Ch*>;

    csv_store() :
        csv_store(std::allocator_arg)
    {}

    csv_store(
        std::allocator_arg_t, const Allocator& alloc = Allocator()) :
        member_like_base<Allocator>(alloc),
        buffers_(nullptr), buffers_back_(nullptr),
        buffers_size_(0)
    {}

    csv_store(csv_store&& other) noexcept :
        member_like_base<Allocator>(std::move(other.get())),
        buffers_(other.buffers_), buffers_back_(other.buffers_back_),
        buffers_size_(other.buffers_size_)
    {
        other.buffers_ = nullptr;
        other.buffers_back_ = nullptr;
        other.buffers_size_ = 0;
    }

    ~csv_store()
    {
        while (buffers_) {
            reduce_buffer();
        }
    }

    csv_store& operator=(csv_store&& other) noexcept
    {
        csv_store(std::move(other)).swap(*this);
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

    void swap(csv_store& other) noexcept
    {
        using std::swap;
        if (at_t::propagate_on_container_swap::value) {
            swap(this->get(), other.get());
        } else {
            assert(this->get() == other.get());
        }
        swap(buffers_, other.buffers_);
        swap(buffers_back_, other.buffers_back_);
        swap(buffers_size_, other.buffers_size_);
    }

    csv_store& merge(csv_store&& other) noexcept
    {
        if (buffers_back_) {
            buffers_back_->next = other.buffers_;
        } else {
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
        security s;
        for (auto i = buffers_; i; i = i->next) {
            s.push_back(i->secured());
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
    void reduce_buffer()
    {
        typename nat_t::allocator_type na(this->get());

        auto next = buffers_->next;
        const auto p = buffers_->detach();
        at_t::deallocate(this->get(), pt_t::pointer_to(*p.first), p.second);
        buffers_->~node_type();
        nat_t::deallocate(na, npt_t::pointer_to(*buffers_), 1);
        buffers_ = next;
    }
};

template <class Ch, class Allocator>
void swap(
    csv_store<Ch, Allocator>& left,
    csv_store<Ch, Allocator>& right) noexcept
{
    left.swap(right);
}

} // end namespace detail

template <class Content, class Allocator, bool Transposes>
class csv_table_builder;

template <class Content, class Allocator =
    std::allocator<typename Content::value_type::value_type::value_type>>
class basic_csv_table
{
public:
    using allocator_type  = Allocator;
    using content_type    = Content;
    using record_type     = typename content_type::value_type;
    using value_type      = typename record_type::value_type;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using char_type       = typename value_type::value_type;
    using traits_type     = typename value_type::traits_type;
    using size_type       = typename content_type::size_type;

    static_assert(
        std::is_same<
            value_type,
            basic_csv_value<char_type, traits_type>>::value,
        "Content shall be a container-of-container type of basic_csv_value");

private:
    using store_type = detail::csv_store<char_type, allocator_type>;
    using at_t = typename std::allocator_traits<allocator_type>;
    using ra_t = typename at_t::template rebind_alloc<record_type>;

    friend class csv_table_builder<Content, Allocator, true>;
    friend class csv_table_builder<Content, Allocator, false>;

    template <class ContentL, class ContentR, class AllocatorXY>
    friend basic_csv_table<ContentL, AllocatorXY>& operator+=(
        basic_csv_table<ContentL, AllocatorXY>& left,
        basic_csv_table<ContentR, AllocatorXY>&& right);

private:
    store_type store_;
    content_type records_;

    std::size_t buffer_size_;

private:
    static constexpr std::size_t default_buffer_size =
        std::min<std::size_t>(std::numeric_limits<std::size_t>::max(), 8192U);

public:
    basic_csv_table(std::size_t buffer_size = default_buffer_size) :
        basic_csv_table(std::allocator_arg, Allocator(), buffer_size)
    {}

    basic_csv_table(std::allocator_arg_t,
        const Allocator& alloc = Allocator(),
        std::size_t buffer_size = default_buffer_size) :
        basic_csv_table(std::uses_allocator<content_type, ra_t>(),
            alloc, buffer_size)
    {}

private:
    // For allocator-aware containers
    basic_csv_table(std::true_type,
        const Allocator& alloc, std::size_t buffer_size) :
        store_(std::allocator_arg, alloc), records_(ra_t(alloc)),
        buffer_size_(sanitize_buffer_size(buffer_size, alloc))
    {}

    // For not-allocator-aware containers
    basic_csv_table(std::false_type,
        const Allocator& alloc, std::size_t buffer_size) :
        store_(std::allocator_arg, alloc), records_(),
        buffer_size_(sanitize_buffer_size(buffer_size, alloc))
    {}

    std::size_t sanitize_buffer_size(
        std::size_t buffer_size, const Allocator& alloc)
    {
        return std::min(
            std::max(buffer_size, static_cast<std::size_t>(2U)),
            at_t::max_size(alloc));
    }

public:
    basic_csv_table(const basic_csv_table& other) :
        basic_csv_table(std::allocator_arg,
            at_t::select_on_container_copy_construction(other.get_allocator()),
            other)
    {}

private:
    basic_csv_table(std::allocator_arg_t,
        const Allocator& alloc, const basic_csv_table& other) :
        basic_csv_table(std::allocator_arg, alloc, other.get_buffer_size())
    {
        for (const auto& r : other.content()) {
            const auto e = content().emplace(content().cend()); // throw
            for (const auto& v : r) {
                e->insert(e->cend(), import_value(v));          // throw
            }
        }
    }

public:
    basic_csv_table(basic_csv_table&& other)
        noexcept(std::is_nothrow_move_constructible<content_type>::value) :
        store_(std::move(other.store_)),
        records_(std::move(other.records_)),
        buffer_size_(other.buffer_size_)
    {}

    basic_csv_table& operator=(const basic_csv_table& other)
    {
        return assign(typename at_t::propagate_on_container_copy_assignment(),
            other);                 // throw
    }

    basic_csv_table& operator=(basic_csv_table&& other)
        noexcept(
            std::is_nothrow_move_assignable<content_type>::value
         && std::allocator_traits<allocator_type>::
                propagate_on_container_move_assignment::value)
    {
        return assign(typename at_t::propagate_on_container_move_assignment(),
            std::move(other));      // throw
    }

private:
    basic_csv_table& assign(std::true_type, const basic_csv_table& other)
    {
        basic_csv_table(other).swap(*this);                 // throw
        return *this;
    }

    basic_csv_table& assign(std::false_type, const basic_csv_table& other)
    {
        basic_csv_table(std::allocator_arg, get_allocator(), other)
            .swap(*this);                                   // throw
        return *this;
    }

    basic_csv_table& assign(std::true_type, basic_csv_table&& other) noexcept
    {
        basic_csv_table(std::move(other)).swap(*this);
        return *this;
    }

public:
    allocator_type get_allocator() const noexcept
    {
        return store_.get_allocator();
    }

    std::size_t get_buffer_size() const noexcept
    {
        return buffer_size_;
    }

    content_type& content() noexcept
    {
        return records_;
    }

    const content_type& content() const noexcept
    {
        return records_;
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
    value_type& rewrite_value(value_type& value,
        ForwardIterator new_value_begin, ForwardIterator new_value_end)
    {
        const auto length = static_cast<std::size_t>(
            std::distance(new_value_begin, new_value_end));
        if (length <= value.size()) {
            std::copy_n(new_value_begin, length, value.begin());
            value.erase(value.cbegin() + length, value.cend());
        } else {
            auto secured = store_.secure_any(length + 1);
            if (!secured) {
                auto a = store_.get_allocator();
                const auto alloc_size = std::max(length + 1, buffer_size_);
                secured = std::addressof(
                    *at_t::allocate(a, alloc_size));            // throw
                store_.add_buffer(secured, alloc_size);         // throw
                // No need to deallocate secured even when an exception is
                // thrown by add_buffer because add_buffer consumes secured
                store_.secure_current_upto(secured + length + 1);
            }
            std::copy_n(new_value_begin, length, secured);
            secured[length] = typename value_type::value_type();
            value = value_type(secured, secured + length);
        }
        return value;
    }

    template <class ForwardIterator>
    auto rewrite_value(value_type& value, ForwardIterator new_value)
     -> decltype(
            std::declval<typename std::iterator_traits<ForwardIterator>::
                iterator_category>(),
            std::declval<value_type>())&
    {
        auto last = new_value;
        while (*last != char_type()) {
            ++last;
        }
        return rewrite_value(value, new_value, last);
    }

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
        return content().size();
    }

    bool empty() const
        noexcept(noexcept(std::declval<const content_type&>().empty()))
    {
        return content().empty();
    }

    void clear()
        noexcept(noexcept(std::declval<content_type&>().clear()))
    {
        content().clear();
        store_   .clear();
    }

    void shrink_to_fit()
    {
        basic_csv_table(*this).swap(*this);     // throw
    }

    void swap(basic_csv_table& other)
        noexcept(noexcept(std::declval<content_type&>().
                            swap(std::declval<content_type&>())))
    {
        using std::swap;
        assert((at_t::propagate_on_container_swap::value
             || (get_allocator() == other.get_allocator())) &&
                "Swapping two basic_csv_tables with allocators of which "
                "POCS is false and which do not equal each other delivers "
                "an undefined behaviour; this is it");
        swap(content()   , other.content());        // ?
        swap(store_      , other.store_);
        swap(buffer_size_, other.buffer_size_);
    }

private:
    void add_buffer(char_type* buffer, std::size_t size)
    {
        store_.add_buffer(buffer, size);
    }

    void secure_current_upto(char_type* secured_last)
    {
        store_.secure_current_upto(secured_last);
    }
};

template <class Content, class Allocator>
void swap(
    basic_csv_table<Content, Allocator>& left,
    basic_csv_table<Content, Allocator>& right)
    noexcept(noexcept(left.swap(right)))
{
    left.swap(right);
}

namespace detail {

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

template <class T>
struct is_nothrow_propagating_swappable :
    std::integral_constant<bool,
        is_nothrow_swappable<T>::value
     && !has_non_pocs_allocator_type<T>::value>
{};

template <class T>
auto nothrow_emigrate(T& from, T& to)
 -> std::enable_if_t<!std::is_nothrow_move_assignable<T>::value>
{
    static_assert(is_nothrow_propagating_swappable<T>::value, "");
    from.swap(to);
}

template <class T>
auto nothrow_emigrate(T& from, T& to)
 -> std::enable_if_t<std::is_nothrow_move_assignable<T>::value>
{
    to = std::move(from);
}

template <class ContentL, class ContentR>
void append_csv_table_content_primitive(
    ContentL& left_content, ContentR&& right_content)
{
    // We require:
    // - if an exception is thrown by ContentL's emplace() at the end,
    //   there are no effects on the before-end elements.
    // - ContentL's erase() at the end does not throw an exception.
    // - ContentR's erase() at the end does not throw an exception.

    const auto left_original_size = left_content.size();        // ?
    try {
        for (auto& r : right_content) {
            left_content.emplace(
                left_content.cend(), r.cbegin(), r.cend());     // throw
        }
    } catch (...) {
        left_content.erase(
            std::next(left_content.begin(), left_original_size),
            left_content.cend());
        throw;
    }

    // Make sure right_content has no reference to moved values;
    // we use erase() instead of clear() to simplify requirements
    right_content.erase(right_content.cbegin(), right_content.cend());
}

template <class RecordL, class AllocatorL, class ContentR>
void append_csv_table_content_primitive(
    std::list<RecordL, AllocatorL>& left_content, ContentR&& right_content)
{
    std::list<RecordL, AllocatorL> right(
        left_content.get_allocator());                          // throw
    for (auto& r : right_content) {
        right.emplace_back(r.cbegin(), r.cend());               // throw
    }
    left_content.splice(left_content.cend(), right);
}

template <class ContentL, class ContentR>
auto append_csv_table_content_adaptive(
    ContentL& left_content, ContentR&& right_content)
 -> std::enable_if_t<
        std::is_nothrow_move_assignable<
            typename ContentL::value_type>::value
     || is_nothrow_propagating_swappable<
            typename ContentL::value_type>::value>
{
    static_assert(std::is_same<typename ContentL::value_type,
        typename ContentR::value_type>::value, "");

    // We require:
    // - if an exception is thrown by ContentL's emplace() at the end,
    //   there are no effects on the before-end elements.
    // - ContentL's erase() at the end does not throw an exception.
    // - ContentR's erase() at the end does not throw an exception.

    const auto left_original_size = left_content.size();    // ?
    try {
        // We do move-emplace instead of emplace-and-swap because it seems
        // unlikely that default-construct-then-swap is less exception-prone
        // than move-construct
        for (auto& r : right_content) {
            left_content.emplace(
                left_content.cend(), std::move(r));         // throw
        }
    } catch (...) {
        const auto le = std::next(left_content.begin(), left_original_size);
        auto j = right_content.begin();
        for (auto i = le, ie = left_content.end(); i != ie; ++i, ++j) {
            nothrow_emigrate(*i, *j);
        }
        left_content.erase(le, left_content.cend());
        throw;
    }

    // Make sure right_content has no reference to moved values;
    // we use erase() instead of clear() to simplify requirements
    right_content.erase(right_content.cbegin(), right_content.cend());
}

template <class Record, class AllocatorL, class ContentR>
auto append_csv_table_content_adaptive(
    std::list<Record, AllocatorL>& left_content, ContentR&& right_content)
 -> std::enable_if_t<
        std::is_nothrow_move_assignable<Record>::value
     || is_nothrow_propagating_swappable<Record>::value>
{
    static_assert(
        std::is_same<Record, typename ContentR::value_type>::value, "");

    std::list<Record, AllocatorL> right(
        left_content.get_allocator());                  // throw
    try {
        // We do move-emplace instead of emplace-and-swap because it seems
        // unlikely that default-construct-then-swap is less exception-prone
        // than move-construct
        for (auto& r : right_content) {
            right.push_back(std::move(r));              // throw
        }
    } catch (...) {
        auto j = right_content.begin();
        for (auto i = right.begin(), ie = right.end(); i != ie; ++i, ++j) {
            nothrow_emigrate(*i, *j);
        }
        throw;
    }
    left_content.splice(left_content.cend(), right);
}

template <class ContentL, class ContentR>
auto append_csv_table_content_adaptive(
    ContentL& left_content, ContentR&& right_content)
 -> std::enable_if_t<
        !std::is_nothrow_move_assignable<
            typename ContentL::value_type>::value
     && !is_nothrow_propagating_swappable<
            typename ContentL::value_type>::value>
{
    // In fact, even if POCS == false, swapping is OK if allocators are equal;
    // we've decided, however, not to rescue such rare cases
    append_csv_table_content_primitive(left_content, right_content);
}

template <class ContentL, class ContentR>
auto append_csv_table_content(
    ContentL& left_content, ContentR&& right_content)
 -> std::enable_if_t<!std::is_same<
        typename ContentL::value_type, typename ContentR::value_type>::value>
{
    append_csv_table_content_primitive(left_content, right_content);
}

template <class ContentL, class ContentR>
auto append_csv_table_content(
    ContentL& left_content, ContentR&& right_content)
 -> std::enable_if_t<std::is_same<
        typename ContentL::value_type, typename ContentR::value_type>::value>
{
    append_csv_table_content_adaptive(left_content, std::move(right_content));
}

template <class Record, class Allocator>
void append_csv_table_content(
    std::list<Record, Allocator>& left_content,
    std::list<Record, Allocator>&& right_content)
{
    if (left_content.get_allocator() == right_content.get_allocator()) {
        left_content.splice(left_content.cend(), right_content);
    } else {
        append_csv_table_content_adaptive(
            left_content, std::move(right_content));    // throw
    }
}

template <class ContentL, class AllocatorX, class ContentR, class AllocatorY>
basic_csv_table<ContentL, AllocatorX>& import_whole_csv_table(
    basic_csv_table<ContentL, AllocatorX>& left,
    const basic_csv_table<ContentR, AllocatorY>& right)
{
    // We require:
    // - if an exception is thrown by ContentL's emplace() at the end,
    //   there are no effects on the before-end elements.
    // - ContentL's erase() at the end does not throw an exception.

    left.guard_rewrite([&right](auto& t) {
        auto& tc = t.content();
        const auto original_size = tc.size();                       // ?
        try {
            for (const auto& r : right.content()) {
                const auto i = tc.emplace(tc.cend());               // throw
                for (const auto& v : r) {
                    i->insert(i->cend(), t.import_value(v));        // throw
                }
            }
        } catch (...) {
            tc.erase(std::next(tc.cbegin(), original_size), tc.cend());
            throw;
        }
    });
    return left;
}

template <
    class RecordL, class AllocatorL, class AllocatorX,
    class ContentR, class AllocatorY>
basic_csv_table<std::list<RecordL, AllocatorL>, AllocatorX>&
import_whole_csv_table(
    basic_csv_table<std::list<RecordL, AllocatorL>, AllocatorX>& left,
    const basic_csv_table<ContentR, AllocatorY>& right)
{
    std::list<RecordL, AllocatorL> l(
        left.content().get_allocator());            // throw
    left.guard_rewrite([&l, &right](auto& t) {
        for (const auto& r : right.content()) {
            l.emplace_back();                       // throw
            auto& ll = l.back();
            for (const auto& v : r) {
                ll.push_back(t.import_value(v));    // throw
            }
        }
    });
    left.content().splice(left.content().cend(), l);
    return left;
}

template <class T>
struct is_basic_csv_table : std::false_type
{};

template <class Content, class Allocator>
struct is_basic_csv_table<basic_csv_table<Content, Allocator>> : std::true_type
{};

} // end namespace detail

template <class ContentL, class ContentR, class AllocatorL, class AllocatorR>
basic_csv_table<ContentL, AllocatorL>& operator+=(
    basic_csv_table<ContentL, AllocatorL>& left,
    const basic_csv_table<ContentR, AllocatorR>& right)
{
    return detail::import_whole_csv_table(left, right);
}

template <class ContentL, class ContentR, class Allocator>
basic_csv_table<ContentL, Allocator>& operator+=(
    basic_csv_table<ContentL, Allocator>& left,
    basic_csv_table<ContentR, Allocator>&& right)
{
    if (left.store_.get_allocator() == right.store_.get_allocator()) {
        detail::append_csv_table_content(
            left.content(), std::move(right.content()));    // throw
        left.store_.merge(std::move(right.store_));
        return left;
    } else {
        return left += right;                               // throw
    }
}

template <class TableL, class TableR>
auto operator+(TableL&& left, TableR&& right)
 -> std::enable_if_t<
        detail::is_basic_csv_table<std::decay_t<TableL>>::value
     && detail::is_basic_csv_table<std::decay_t<TableR>>::value,
        std::decay_t<TableL>>
{
    std::decay_t<TableL> l(std::forward<TableL>(left));     // throw
    l += std::forward<TableR>(right);                       // throw
    return l;
}

using csv_table  = basic_csv_table<std::deque<std::vector<csv_value>>>;
using wcsv_table = basic_csv_table<std::deque<std::vector<wcsv_value>>>;

namespace detail {

template <class Content>
struct arrange_as_is
{
    using char_type = typename Content::value_type::value_type::value_type;

    explicit arrange_as_is(const Content&)
    {}

    void new_record(Content& content) const
    {
        content.emplace(content.cend());            // throw
    }

    void new_value(Content& content,
        char_type* first, const char_type* last) const
    {
        auto& back = *content.rbegin();
        back.emplace(back.cend(), first, last);     // throw
    }
};

template <class Content>
class arrange_transposing
{
    using record_t = typename Content::value_type;

    typename record_t::size_type i_;
    typename record_t::value_type::size_type j_;

public:
    using char_type = typename Content::value_type::value_type::value_type;

    explicit arrange_transposing(const Content& content) :
        i_(std::accumulate(
            content.cbegin(), content.cend(),
            static_cast<decltype(i_)>(0),
            [](const auto& a, const auto& rec) {
                return std::max(a, rec.size());
            })),
        j_(0)
    {}

    void new_record(Content& content)
    {
        for (auto& vertical : content) {
            vertical.emplace(vertical.cend());  // throw
        }
        ++i_;
        j_ = 0;
    }

    void new_value(Content& content, char_type* first, const char_type* last)
    {
        assert(i_ > 0);
        if (content.size() == j_) {
            // the second argument is supplied to comply with the sequence
            // container requirements
            content.emplace(content.cend(),
                i_, typename record_t::value_type());   // throw
        }
        std::next(content.begin(), j_)->back() =
            typename record_t::value_type(first, last);
        ++j_;
    }
};

template <class Content, bool Transposes>
using arrange = std::conditional_t<Transposes,
    arrange_transposing<Content>, arrange_as_is<Content>>;

} // end namespace detail

template <class Content, class Allocator, bool Transposes = false>
class csv_table_builder :
    detail::arrange<Content, Transposes>
{
public:
    using char_type = typename basic_csv_table<Content, Allocator>::char_type;

private:
    char_type* current_buffer_holder_;
    char_type* current_buffer_;
    std::size_t current_buffer_size_;

    char_type* field_begin_;
    char_type* field_end_;

    basic_csv_table<Content, Allocator>* table_;

private:
    using at_t = std::allocator_traits<Allocator>;

public:
    csv_table_builder(basic_csv_table<Content, Allocator>& table) :
        detail::arrange<Content, Transposes>(table.content()),
        current_buffer_holder_(nullptr), current_buffer_(nullptr),
        field_begin_(nullptr), table_(&table)
    {}

    csv_table_builder(csv_table_builder&& other) noexcept :
        detail::arrange<Content, Transposes>(other),
        current_buffer_holder_(other.current_buffer_holder_),
        current_buffer_(other.current_buffer_),
        current_buffer_size_(other.current_buffer_size_),
        field_begin_(other.field_begin_), field_end_(other.field_end_),
        table_(other.table_)
    {
        other.current_buffer_holder_ = nullptr;
    }

    ~csv_table_builder()
    {
        if (current_buffer_holder_) {
            auto a = table_->get_allocator();
            at_t::deallocate(a, current_buffer_holder_, current_buffer_size_);
        }
    }

    void start_record(const char_type* /*record_begin*/)
    {
        this->new_record(table_->content());    // throw
    }

    bool update(const char_type* first, const char_type* last)
    {
        if (field_begin_) {
            std::memmove(field_end_, first, last - first);
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
        *field_end_ = char_type();
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
            // in an active value, whose length is "length" so far
            length = static_cast<std::size_t>(field_end_ - field_begin_);
            // we'd like to move the active value to the beginning of the
            // returned buffer
            std::size_t next_buffer_size;
            for (next_buffer_size = table_->get_buffer_size();
                 length >= next_buffer_size / 2; next_buffer_size *= 2);
            if (current_buffer_holder_
             && (current_buffer_size_ >= next_buffer_size)) {
                // the current buffer contains no other values
                // and it suffices in terms of length
                std::memmove(
                    current_buffer_holder_, field_begin_, length);
            } else {
                // the current buffer has been committed to the store,
                // so we need a new one
                auto a = table_->get_allocator();
                const auto next_buffer = std::addressof(
                    *at_t::allocate(a, next_buffer_size));  // throw
                std::memcpy(next_buffer, field_begin_, length);
                if (current_buffer_holder_) {
                    at_t::deallocate(
                        a, current_buffer_holder_, current_buffer_size_);
                }
                current_buffer_holder_ = next_buffer;
                current_buffer_size_ = next_buffer_size;
            }
            field_begin_ = current_buffer_holder_;
            field_end_   = current_buffer_holder_ + length;
        } else {
            // out of any active values
            if (current_buffer_holder_) {
                // the current buffer contains no values,
                // so we would like to reuse it
            } else {
                // the current buffer has been committed to the store,
                // so we need a new one
                auto a = table_->get_allocator();
                current_buffer_holder_ = std::addressof(
                    *at_t::allocate(a, table_->get_buffer_size())); // throw
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

    void release_buffer(const char_type* /*buffer*/) noexcept
    {}
};

template <class Content, class Allocator>
auto make_csv_table_builder(basic_csv_table<Content, Allocator>& table)
{
    return csv_table_builder<Content, Allocator>(table);
}

template <class Content, class Allocator>
auto make_transposed_csv_table_builder(
    basic_csv_table<Content, Allocator>& table)
{
    return csv_table_builder<Content, Allocator, true>(table);
}

}}

#endif
