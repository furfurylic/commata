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

template <class Ch, class Tr, class Alloc>
bool operator==(
    const basic_csv_value<Ch, Tr>& left,
    const std::basic_string<Ch, Tr, Alloc>& right) noexcept
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

template <class Ch>
class basic_csv_store
{
    class buffer_type
    {
        std::unique_ptr<Ch[]> buffer_;
        Ch* hwl_;   // high water level: last-past-one of the used elements
        Ch* end_;   // last-past-one of the buffer_

    public:
        buffer_type() noexcept :
            hwl_(nullptr), end_(nullptr)
        {}

        void attach(
            std::unique_ptr<Ch[]>&& buffer, std::size_t size) noexcept
        {
            buffer_ = std::move(buffer);
            hwl_ = buffer_.get();
            end_ = buffer_.get() + size;
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
            hwl_ = buffer_.get();
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

public:
    using security = std::vector<Ch*>;

    basic_csv_store() :
        buffers_(nullptr), buffers_back_(nullptr)
    {}

    basic_csv_store(basic_csv_store&& other) noexcept :
        buffers_     (other.buffers_),
        buffers_back_(other.buffers_back_)
    {
        other.buffers_ = nullptr;
        other.buffers_back_ = nullptr;
    }

    ~basic_csv_store()
    {
        for (auto i = buffers_; i;) {
            auto j = i->next;
            delete i;
            i = j;
        }
    }

    basic_csv_store& operator=(basic_csv_store&& other) noexcept
    {
        basic_csv_store(std::move(other)).swap(*this);
        return *this;
    }

    void add_buffer(std::unique_ptr<Ch[]>&& buffer, std::size_t size)
    {
        // "push_front"-like behaviour
        buffers_ = new node_type(buffers_); // throw
        buffers_->attach(std::move(buffer), size);
        if (!buffers_back_) {
            buffers_back_ = buffers_;
        }
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

    void swap(basic_csv_store& other) noexcept
    {
        std::swap(buffers_, other.buffers_);
        std::swap(buffers_back_, other.buffers_back_);
    }

    basic_csv_store& merge(basic_csv_store&& other) noexcept
    {
        if (buffers_back_) {
            buffers_back_->next = other.buffers_;
        } else {
            buffers_ = other.buffers_;
            buffers_back_ = buffers_;
        }
        other.buffers_ = nullptr;
        other.buffers_back_ = nullptr;
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
        auto i = buffers_;
        auto j = s.cbegin();
        for (; i; i = i->next, ++j) {
            i->secure_upto(*j);
        }
        assert(i == nullptr);
    }
};

template <class Ch>
void swap(basic_csv_store<Ch>& left, basic_csv_store<Ch>& right) noexcept
{
    left.swap(right);
}

template <class T, class = void>
class nothrow_move_and_swap
{
    std::unique_ptr<T> t_;

public:
    nothrow_move_and_swap() :
        t_(new T)
    {}

    nothrow_move_and_swap(const nothrow_move_and_swap& other) :
        t_(new T(*other.t_))
    {}

    nothrow_move_and_swap(nothrow_move_and_swap&&) = default;
    nothrow_move_and_swap& operator=(nothrow_move_and_swap&&) = default;

    T& operator*() noexcept
    {
        return *t_;
    }

    const T& operator*() const noexcept
    {
        return *t_;
    }

    T* operator->() noexcept
    {
        return t_.get();
    }

    const T* operator->() const noexcept
    {
        return t_.get();
    }

    void swap(nothrow_move_and_swap& other) noexcept
    {
        t_.swap(other.t_);
    }
};

template <class T>
class nothrow_move_and_swap<
    T,
    std::enable_if_t<
        std::is_nothrow_move_constructible<T>::value
     && std::is_nothrow_move_assignable<T>::value
     && is_nothrow_swappable<T>::value>>
{
    T t_;

public:
    T& operator*() noexcept
    {
        return t_;
    }

    const T& operator*() const noexcept
    {
        return t_;
    }

    T* operator->() noexcept
    {
        return &t_;
    }

    const T* operator->() const noexcept
    {
        return &t_;
    }

    void swap(nothrow_move_and_swap& other) noexcept
    {
        t_.swap(other.t_);
    }
};

template <class T, class V>
void swap(
    nothrow_move_and_swap<T, V>& left, nothrow_move_and_swap<T, V>& right)
    noexcept
{
    left.swap(right);
}

} // end namespace detail

template <class Content, bool Transposes>
class csv_table_builder;

template <class Content>
class basic_csv_table
{
public:
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
    using store_type =
        detail::basic_csv_store<typename value_type::value_type>;

    friend class csv_table_builder<Content, true>;
    friend class csv_table_builder<Content, false>;

    template <class ContentL, class ContentR>
    friend basic_csv_table<ContentL>& operator+=(
        basic_csv_table<ContentL>& left,
        basic_csv_table<ContentR>&& right);

private:
    store_type store_;
    detail::nothrow_move_and_swap<content_type> records_;

public:
    basic_csv_table()
    {}

    basic_csv_table(basic_csv_table&& other) noexcept :
        store_(std::move(other.store_)),
        records_(std::move(other.records_))
    {}

    basic_csv_table& operator=(basic_csv_table&& other) noexcept
    {
        basic_csv_table(std::move(other)).swap(*this);
        return *this;
    }

    void add_buffer(std::unique_ptr<char_type[]>&& buffer, std::size_t size)
    {
        store_.add_buffer(std::move(buffer), size);
    }

    content_type& content() noexcept
    {
        return *records_;
    }

    const content_type& content() const noexcept
    {
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

    bool rewrite_value(value_type& value,
        const char_type* new_value_begin,
        const char_type* new_value_end) noexcept
    {
        using traits = typename value_type::traits_type;
        const auto length =
            static_cast<std::size_t>(new_value_end - new_value_begin);
        if (length <= value.size()) {
            traits::move(&*value.begin(), new_value_begin, length);
            value.erase(value.cbegin() + length, value.cend());
        } else {
            const auto secured = store().secure_any(length + 1);
            if (!secured) {
                return false;
            }
            traits::copy(secured, new_value_begin, length);
            secured[length] = typename value_type::value_type();
            value = value_type(secured, secured + length);
        }
        return true;
    }

    bool rewrite_value(value_type& value,
        const char_type* new_value) noexcept
    {
        return rewrite_value(value, new_value,
            new_value + value_type::traits_type::length(new_value));
    }

    template <class Alloc>
    bool rewrite_value(value_type& value,
        const std::basic_string<char_type, traits_type, Alloc>&
            new_value) noexcept
    {
        return rewrite_value(value,
            new_value.c_str(), new_value.c_str() + new_value.size());
    }

    bool rewrite_value(value_type& value,
        const value_type& new_value) noexcept
    {
        return rewrite_value(value,
            new_value.cbegin(), new_value.cend());
    }

    template <class OtherRecord>
    typename content_type::iterator import_record(
        typename content_type::const_iterator position,
        const OtherRecord& record)
    {
        const auto security = store_.get_security();    // throw
        record_type imported;           // throw
        for (const auto& value : record) {
            imported.emplace_back();    // throw
            if (!rewrite_value(
                    imported.back(), value.cbegin(), value.cend())) {
                store_.set_security(security);
                return content().end();
            }
        }
        try {
            return content().insert(position, std::move(imported)); // throw
        } catch (...) {
            store_.set_security(security);
            throw;
        }
    }

    size_type size() const
        noexcept(noexcept(std::declval<const content_type&>().size()))
    {
        return records_->size();
    }

    bool empty() const
        noexcept(noexcept(std::declval<const content_type&>().empty()))
    {
        return records_->empty();
    }

    void clear()
        noexcept(noexcept(std::declval<content_type&>().clear()))
    {
        records_->clear();
        store_   .clear();
    }

    void swap(basic_csv_table& other) noexcept
    {
        using std::swap;
        swap(records_, other.records_);
        swap(store_  , other.store_);
    }

private:
    store_type& store() noexcept
    {
        return store_;
    }

    const store_type& store() const noexcept
    {
        return store_;
    }
};

template <class Content>
void swap(
    basic_csv_table<Content>& left, basic_csv_table<Content>& right) noexcept
{
    left.swap(right);
}

namespace detail {

template <class T>
struct is_nothrow_pop_backable :
    std::integral_constant<bool,
        // According to C++14 23.2.1 (11), STL containers throw no exceptions
        // with their pop_back()
        is_std_vector<T>::value
     || is_std_deque<T>::value
     || is_std_list<T>::value
     || noexcept(std::declval<T&>().pop_back())>
{};

template <class ContentL, class ContentR>
struct has_rollbackable_move_insert :
    std::integral_constant<bool,
        is_nothrow_pop_backable<ContentL>::value
     && std::is_same<
            typename ContentL::value_type,
            typename ContentR::value_type>::value   // same record types
     && is_nothrow_swappable<typename ContentL::value_type>::value>
{};

template <class ContentL, class ContentR>
auto append_csv_table(
    nothrow_move_and_swap<ContentL>& left_content,
    basic_csv_store<
        typename ContentL::value_type::value_type::value_type>& left_store,
    nothrow_move_and_swap<ContentR>&& right_content,
    basic_csv_store<
        typename ContentR::value_type::value_type::value_type>&& right_store)
 -> std::enable_if_t<!has_rollbackable_move_insert<ContentL, ContentR>::value>
{
    // make a copy of *left_content
    // and then copy records in *right_content into it
    auto left = left_content;                                       // throw
    for (const auto& right_rec : *right_content) {
        left->emplace_back(right_rec.cbegin(), right_rec.cend());   // throw
    }
    left_content = std::move(left);
    left_store.merge(std::move(right_store));
}

template <class ContentL, class ContentR>
auto append_csv_table(
    nothrow_move_and_swap<ContentL>& left_content,
    basic_csv_store<
        typename ContentL::value_type::value_type::value_type>& left_store,
    nothrow_move_and_swap<ContentR>&& right_content,
    basic_csv_store<
        typename ContentR::value_type::value_type::value_type>&& right_store)
 -> std::enable_if_t<has_rollbackable_move_insert<ContentL, ContentR>::value>
{
    // expand *left_content in place
    // and then immigrate records in *right_content into it
    const auto left_original_size = left_content->size();   // ?
    try {
        left_content->resize(
            left_original_size + right_content->size());    // throw
    } catch (...) {
        while (left_content->size() > left_original_size) {
            left_content->pop_back();
        }
        throw;
    }
    auto i = left_content->begin();
    std::advance(i, left_original_size);
    std::swap_ranges(i, left_content->end(), right_content->begin());
    left_store.merge(std::move(right_store));
}

template <class RecordL, class AllocatorL, class ContentR>
auto append_csv_table(
    nothrow_move_and_swap<std::list<RecordL, AllocatorL>>& left_content,
    basic_csv_store<
        typename RecordL::value_type::value_type>& left_store,
    nothrow_move_and_swap<ContentR>&& right_content,
    basic_csv_store<
        typename ContentR::value_type::value_type::value_type>&& right_store)
 -> std::enable_if_t<!has_rollbackable_move_insert<
        std::list<RecordL, AllocatorL>, ContentR>::value>
{
    // make a copy of *right_content into a new list
    // and then splice it into *left_content
    std::list<RecordL, AllocatorL> right;                           // throw
    for (const auto& right_rec : *right_content) {
        right.emplace_back(right_rec.cbegin(), right_rec.cend());   // throw
    }
    left_content->splice(left_content->cend(), right);
    left_store.merge(std::move(right_store));
}

template <class Record, class AllocatorL, class ContentR>
auto append_csv_table(
    nothrow_move_and_swap<std::list<Record, AllocatorL>>& left_content,
    basic_csv_store<
        typename Record::value_type::value_type>& left_store,
    nothrow_move_and_swap<ContentR>&& right_content,
    basic_csv_store<
        typename ContentR::value_type::value_type::value_type>&& right_store)
 -> std::enable_if_t<has_rollbackable_move_insert<
        std::list<Record, AllocatorL>, ContentR>::value>
{
    // immigrate records in *right_content into a new list
    // and then splice it into *left_content
    std::list<Record, AllocatorL> right(right_content->size()); // throw
    std::swap_ranges(right.begin(), right.end(), right_content->begin());
    left_content->splice(left_content->cend(), right);
    left_store.merge(std::move(right_store));
}

template <class Record, class Alloc>
void append_csv_table(
    nothrow_move_and_swap<std::list<Record, Alloc>>& left_content,
    basic_csv_store<typename Record::value_type::value_type>& left_store,
    nothrow_move_and_swap<std::list<Record, Alloc>>&& right_content,
    basic_csv_store<typename Record::value_type::value_type>&& right_store)
{
    left_content->splice(left_content->cend(), *right_content);
    left_store.merge(std::move(right_store));
}

} // end namespace detail

template <class ContentL, class ContentR>
basic_csv_table<ContentL>& operator+=(
    basic_csv_table<ContentL>& left,
    basic_csv_table<ContentR>&& right)
{
    detail::append_csv_table(
        left.records_, left.store_,
        std::move(right.records_), std::move(right.store_));    // throw
    return left;
}

template <class ContentL, class ContentR>
basic_csv_table<ContentL> operator+(
    basic_csv_table<ContentL>&& left,
    basic_csv_table<ContentR>&& right)
{
    left += std::move(right);   // throw
    return std::move(left);
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
        content.emplace_back(); // throw
    }

    void new_value(Content& content,
        char_type* first, const char_type* last) const
    {
        content.back().emplace_back(first, last);  // throw
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
            vertical.emplace_back();    // throw
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
            content.emplace_back(
                i_, typename record_t::value_type());   // throw
        }
        auto j = content.begin();
        std::advance(j, j_);
        j->back() = typename record_t::value_type(first, last);
        ++j_;
    }
};

template <class Content, bool Transposes>
using arrange = std::conditional_t<Transposes,
    arrange_transposing<Content>, arrange_as_is<Content>>;

} // end namespace detail

template <class Content, bool Transposes = false>
class csv_table_builder :
    detail::arrange<Content, Transposes>
{
public:
    using char_type = typename basic_csv_table<Content>::char_type;

private:
    std::unique_ptr<char_type[]> current_buffer_holder_;
    char_type* current_buffer_;
    std::size_t current_buffer_size_;

    std::size_t buffer_size_;

    char_type* field_begin_;
    char_type* field_end_;

    basic_csv_table<Content>* table_;

public:
    csv_table_builder(
        std::size_t buffer_size, basic_csv_table<Content>& table) :
        detail::arrange<Content, Transposes>(table.content()),
        current_buffer_(nullptr),
        buffer_size_(std::max(buffer_size, static_cast<std::size_t>(2U))),
        field_begin_(nullptr), table_(&table)
    {}

    csv_table_builder(csv_table_builder&&) = default;

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
            table_->store().add_buffer(
                std::move(current_buffer_holder_),
                current_buffer_size_);  // throw
        }
        this->new_value(table_->content(), field_begin_, field_end_); // throw
        table_->store().secure_current_upto(field_end_ + 1);
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
            for (next_buffer_size = buffer_size_;
                 length >= next_buffer_size / 2; next_buffer_size *= 2);
            if (current_buffer_holder_
             && (current_buffer_size_ >= next_buffer_size)) {
                // the current buffer contains no other values
                // and it suffices in terms of length
                std::memmove(
                    current_buffer_holder_.get(), field_begin_, length);
            } else {
                // the current buffer has been committed to the store,
                // so we need a new one
                std::unique_ptr<char_type[]> next_buffer_holder(
                    new char_type[next_buffer_size]);    // throw
                std::memcpy(next_buffer_holder.get(), field_begin_, length);
                current_buffer_holder_ = std::move(next_buffer_holder);
                current_buffer_size_ = next_buffer_size;
            }
            field_begin_ = current_buffer_holder_.get();
            field_end_   = current_buffer_holder_.get() + length;
        } else {
            // out of any active values
            if (current_buffer_holder_) {
                // the current buffer contains no values,
                // so we would like to reuse it
            } else {
                // the current buffer has been committed to the store,
                // so we need a new one
                current_buffer_holder_.reset(
                    new char_type[buffer_size_]); // throw
                current_buffer_size_ = buffer_size_;
            }
            length = 0;
        }
        assert(current_buffer_holder_);
        current_buffer_ = current_buffer_holder_.get();
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

template <class Content>
auto make_csv_table_builder(
    std::size_t buffer_size, basic_csv_table<Content>& table)
{
    return csv_table_builder<Content>(buffer_size, table);
}

template <class Content>
auto make_transposed_csv_table_builder(
    std::size_t buffer_size, basic_csv_table<Content>& table)
{
    return csv_table_builder<Content, true>(buffer_size, table);
}

}}

#endif
