/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_2D1E2667_42AD_4B4C_B679_3A6C6F513FB4
#define COMMATA_GUARD_2D1E2667_42AD_4B4C_B679_3A6C6F513FB4

#include <algorithm>
#include <cstddef>
#include <limits>
#include <streambuf>
#include <string>
#include <type_traits>
#include <utility>

namespace commata {

namespace detail {

template <class Ch, class Tr>
auto read(std::basic_streambuf<Ch, Tr>& in,
    Ch* out, std::make_unsigned_t<std::streamsize> n)
{
    decltype(n) nmax = std::numeric_limits<std::streamsize>::max();
    decltype(n) m = 0;
    while (n > nmax) {
        const auto mm = in.sgetn(out, nmax);
        if (mm == 0) {
            return m;
        }
        m += mm;
        n -= mm;
        out += mm;
    }
    m += in.sgetn(out, n);
    return m;
}

}

template <class Ch, class Tr = std::char_traits<Ch>>
class streambuf_input
{
    std::basic_streambuf<Ch, Tr>* in_;

public:
    static_assert(std::is_same<Ch, typename Tr::char_type>::value, "");

    using char_type = Ch;
    using traits_type = Tr;
    using size_type = std::make_unsigned_t<std::streamsize>;

    streambuf_input() noexcept :
        in_(nullptr)
    {}

    explicit streambuf_input(std::basic_streambuf<Ch, Tr>* in) :
        in_(in)
    {}

    explicit streambuf_input(std::basic_istream<Ch, Tr>& in) noexcept :
        streambuf_input(in.rdbuf())
    {}

    streambuf_input(streambuf_input&& other) noexcept :
        in_(other.in_)
    {
        other.in_ = nullptr;
    }

    ~streambuf_input() = default;

    streambuf_input& operator=(streambuf_input&& other) noexcept
    {
        in_ = other.in_;
        other.in_ = nullptr;
        return *this;
    }

    size_type operator()(Ch* out, size_type n)
    {
        return in_ ? detail::read(*in_, out, n) : 0;
    }

    void swap(streambuf_input& other) noexcept
    {
        std::swap(in_, other.in_);
    }
};

template <class Ch, class Tr>
void swap(streambuf_input<Ch, Tr>& left,
          streambuf_input<Ch, Tr>& right) noexcept
{
    left.swap(right);
}

template <class Streambuf>
class owned_streambuf_input
{
    Streambuf in_;

public:
    static_assert(
        std::is_base_of<
            std::basic_streambuf<
                typename Streambuf::char_type,
                typename Streambuf::traits_type>,
            Streambuf>::value, "");

    using char_type = typename Streambuf::char_type;
    using traits_type = typename Streambuf::traits_type;
    using size_type = std::make_unsigned_t<std::streamsize>;

    owned_streambuf_input() = default;

    explicit owned_streambuf_input(Streambuf&& in)
        noexcept(std::is_nothrow_move_constructible<Streambuf>::value) :
        in_(std::move(in))
    {}

    owned_streambuf_input(owned_streambuf_input&& other) = default;
    ~owned_streambuf_input() = default;
    owned_streambuf_input& operator=(owned_streambuf_input&& other) = default;

    size_type operator()(char_type* out, size_type n)
    {
        return detail::read(in_, out, n);
    }

    void swap(owned_streambuf_input& other)
        noexcept(noexcept(
            std::declval<Streambuf&>().swap(std::declval<Streambuf&>())))
    {
        using std::swap;
        swap(in_, other.in_);
    }
};

template <class Streambuf>
void swap(owned_streambuf_input<Streambuf>& left,
          owned_streambuf_input<Streambuf>& right)
    noexcept(noexcept(left.swap(right)))
{
    left.swap(right);
}

template <class IStream>
class owned_istream_input
{
    IStream in_;

public:
    static_assert(
        std::is_base_of<
            std::basic_istream<
                typename IStream::char_type,
                typename IStream::traits_type>,
            IStream>::value, "");

    using char_type = typename IStream::char_type;
    using traits_type = typename IStream::traits_type;
    using size_type = std::make_unsigned_t<std::streamsize>;

    owned_istream_input() = default;

    explicit owned_istream_input(IStream&& in)
        noexcept(std::is_nothrow_move_constructible<IStream>::value) :
        in_(std::move(in))
    {}

    owned_istream_input(owned_istream_input&& other) = default;
    ~owned_istream_input() = default;
    owned_istream_input& operator=(owned_istream_input&& other) = default;

    size_type operator()(char_type* out, size_type n)
    {
        return detail::read(*in_.rdbuf(), out, n);
    }

    void swap(owned_istream_input& other)
        noexcept(noexcept(
            std::declval<IStream&>().swap(std::declval<IStream&>())))
    {
        using std::swap;
        swap(in_, other.in_);
    }
};

template <class IStream>
void swap(owned_istream_input<IStream>& left,
          owned_istream_input<IStream>& right)
    noexcept(noexcept(left.swap(right)))
{
    left.swap(right);
}

template <class Ch, class Tr = std::char_traits<Ch>>
class string_input
{
    const Ch* begin_;
    const Ch* end_;

public:
    static_assert(std::is_same<Ch, typename Tr::char_type>::value, "");

    using char_type = Ch;
    using traits_type = Tr;
    using size_type = std::size_t;

    string_input() noexcept :
        begin_(nullptr), end_(nullptr)
    {}

    explicit string_input(const Ch* str) :
        string_input(str, Tr::length(str))
    {}

    string_input(const Ch* data, std::size_t length) :
        begin_(data), end_(data + length)
    {}

    template <class Allocator>
    explicit string_input(const std::basic_string<Ch, Tr, Allocator>& str)
        noexcept :
        string_input(str.data(), str.size())
    {}

    string_input(string_input&& other) noexcept :
        begin_(other.begin_), end_(other.end_)
    {
        other.begin_ = end_;
    }

    ~string_input() = default;

    string_input& operator=(string_input&& other) noexcept
    {
        string_input(std::move(other)).swap(*this);
        return *this;
    }

    size_type operator()(Ch* out, size_type n)
    {
        if (begin_ < end_) {
            const auto length = std::min(
                n, static_cast<size_type>(end_ - begin_));
            Tr::copy(out, begin_, length);
            begin_ += length;
            return length;
        } else {
            return 0;
        }
    }

    void swap(string_input& other) noexcept
    {
        std::swap(begin_, other.begin_);
        std::swap(end_, other.end_);
    }
};

template <class Ch, class Tr>
void swap(string_input<Ch, Tr>& left,
          string_input<Ch, Tr>& right) noexcept
{
    left.swap(right);
}

template <class Ch, class Tr = std::char_traits<Ch>,
    class Allocator = std::allocator<Ch>>
class owned_string_input
{
public:
    using size_type = typename std::basic_string<Ch, Tr, Allocator>::size_type;

private:
    std::basic_string<Ch, Tr, Allocator> s_;
    size_type head_;

public:
    static_assert(std::is_same<Ch, typename Tr::char_type>::value, "");

    using char_type = Ch;
    using traits_type = Tr;

    owned_string_input()
        noexcept(std::is_nothrow_default_constructible<
            std::basic_string<Ch, Tr, Allocator>>::value) :
        s_(), head_(0)
    {}

    explicit owned_string_input(
        const std::basic_string<Ch, Tr, Allocator>& str) :
        s_(str), head_(0)
    {}

    explicit owned_string_input(std::basic_string<Ch, Tr, Allocator>&& str)
        noexcept :
        s_(std::move(str)), head_(0)
    {}

    owned_string_input(owned_string_input&& other) noexcept :
        s_(std::move(other.s_)), head_(other.head_)
    {
        other.head_ = other.s_.size();
    }

    ~owned_string_input() = default;

    owned_string_input& operator=(owned_string_input&& other)
        noexcept(std::is_nothrow_move_assignable<
            std::basic_string<Ch, Tr, Allocator>>::value)
    {
        s_ = std::move(other.s_);
        head_ = other.head_;
        other.head_ = other.s_.size();
        return *this;
    }

    size_type operator()(Ch* out, size_type n)
    {
        const auto len = s_.copy(out, n, head_);
        head_ += len;
        return len;
    }

    void swap(owned_string_input& other)
        noexcept(noexcept(
            std::declval<std::basic_string<Ch, Tr, Allocator>&>()
                .swap(std::declval<std::basic_string<Ch, Tr, Allocator>&>())))
    {
        using std::swap;
        swap(s_, other.s_);
        swap(head_, other.head_);
    }
};

template <class Ch, class Tr>
void swap(owned_string_input<Ch, Tr>& left,
          owned_string_input<Ch, Tr>& right)
    noexcept(noexcept(left.swap(right)))
{
    left.swap(right);
}

template <class Ch, class Tr>
streambuf_input<Ch, Tr> make_char_input(std::basic_streambuf<Ch, Tr>* in)
{
    return streambuf_input<Ch, Tr>(in);
}

template <class Streambuf>
auto make_char_input(Streambuf&& in) noexcept
 -> std::enable_if_t<
        !std::is_lvalue_reference<Streambuf>::value
     && std::is_base_of<
            std::basic_streambuf<
                typename Streambuf::char_type,
                typename Streambuf::traits_type>,
            Streambuf>::value,
        owned_streambuf_input<Streambuf>>
{
    return owned_streambuf_input<Streambuf>(std::forward<Streambuf>(in));
}

template <class IStream>
auto make_char_input(IStream&& in) noexcept
 -> std::enable_if_t<
        !std::is_lvalue_reference<IStream>::value
     && std::is_base_of<
            std::basic_istream<
                typename IStream::char_type,
                typename IStream::traits_type>,
            IStream>::value,
        owned_istream_input<IStream>>
{
    return owned_istream_input<IStream>(std::forward<IStream>(in));
}

template <class Ch, class Tr>
streambuf_input<Ch, Tr> make_char_input(
    std::basic_istream<Ch, Tr>& in) noexcept
{
    return streambuf_input<Ch, Tr>(in);
}

template <class Ch, class Tr = std::char_traits<Ch>>
auto make_char_input(const Ch* in)
 -> std::enable_if_t<
        std::is_same<Ch, char>::value || std::is_same<Ch, wchar_t>::value,
        string_input<Ch, Tr>>
{
    return string_input<Ch, Tr>(in);
}

template <class Ch, class Tr = std::char_traits<Ch>>
auto make_char_input(const Ch* in, std::size_t length)
 -> std::enable_if_t<
        std::is_same<Ch, char>::value || std::is_same<Ch, wchar_t>::value,
        string_input<Ch, Tr>>
{
    return string_input<Ch, Tr>(in, length);
}

template <class Ch, class Tr, class Allocator>
string_input<Ch, Tr> make_char_input(
    const std::basic_string<Ch, Tr, Allocator>& in) noexcept
{
    return string_input<Ch, Tr>(in);
}

template <class Ch, class Tr, class Allocator>
owned_string_input<Ch, Tr, Allocator> make_char_input(
    std::basic_string<Ch, Tr, Allocator>&& in) noexcept
{
    return owned_string_input<Ch, Tr, Allocator>(std::move(in));
}

}

#endif
