/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_2D1E2667_42AD_4B4C_B679_3A6C6F513FB4
#define COMMATA_GUARD_2D1E2667_42AD_4B4C_B679_3A6C6F513FB4

#include <algorithm>
#include <cstddef>
#include <istream>
#include <limits>
#include <memory>
#include <streambuf>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace commata {

namespace detail { namespace input {

template <class Ch, class Tr>
auto read(std::basic_streambuf<Ch, Tr>& in,
    Ch* out, std::make_unsigned_t<std::streamsize> n)
{
    constexpr decltype(n) nmax = std::numeric_limits<std::streamsize>::max();
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

}} // end detail::input

template <class Ch, class Tr = std::char_traits<Ch>>
class streambuf_input
{
    std::basic_streambuf<Ch, Tr>* in_;

public:
    static_assert(std::is_same_v<Ch, typename Tr::char_type>, "");

    using streambuf_type = std::basic_streambuf<Ch, Tr>;
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

    streambuf_input(const streambuf_input& other) = default;
    streambuf_input& operator=(const streambuf_input&) =default;

    size_type operator()(Ch* out, size_type n)
    {
        return in_ ? detail::input::read(*in_, out, n) : 0;
    }
};

template <class Streambuf>
class owned_streambuf_input
{
    Streambuf in_;

public:
    static_assert(
        std::is_base_of_v<
            std::basic_streambuf<
                typename Streambuf::char_type,
                typename Streambuf::traits_type>,
            Streambuf>, "");

    using streambuf_type = Streambuf;
    using char_type = typename Streambuf::char_type;
    using traits_type = typename Streambuf::traits_type;
    using size_type = std::make_unsigned_t<std::streamsize>;

    owned_streambuf_input() = default;

    explicit owned_streambuf_input(Streambuf&& in)
        noexcept(std::is_nothrow_move_constructible_v<Streambuf>) :
        in_(std::move(in))
    {}

    owned_streambuf_input(owned_streambuf_input&& other) = default;
    ~owned_streambuf_input() = default;
    owned_streambuf_input& operator=(owned_streambuf_input&& other) = default;

    size_type operator()(char_type* out, size_type n)
    {
        return detail::input::read(in_, out, n);
    }

    void swap(owned_streambuf_input& other)
        noexcept(std::is_nothrow_swappable_v<Streambuf>)
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
        std::is_base_of_v<
            std::basic_istream<
                typename IStream::char_type,
                typename IStream::traits_type>,
            IStream>, "");

    using istream_type = IStream;
    using char_type = typename IStream::char_type;
    using traits_type = typename IStream::traits_type;
    using size_type = std::make_unsigned_t<std::streamsize>;

    owned_istream_input() = default;

    explicit owned_istream_input(IStream&& in)
        noexcept(std::is_nothrow_move_constructible_v<IStream>) :
        in_(std::move(in))
    {}

    owned_istream_input(owned_istream_input&& other) = default;
    ~owned_istream_input() = default;
    owned_istream_input& operator=(owned_istream_input&& other) = default;

    size_type operator()(char_type* out, size_type n)
    {
        return detail::input::read(*in_.rdbuf(), out, n);
    }

    void swap(owned_istream_input& other)
        noexcept(std::is_nothrow_swappable_v<IStream>)
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
    std::basic_string_view<Ch, Tr> v_;

public:
    static_assert(std::is_same_v<Ch, typename Tr::char_type>, "");

    using char_type = Ch;
    using traits_type = Tr;
    using size_type = std::size_t;

    string_input() noexcept :
        v_()
    {}

    explicit string_input(const Ch* str) :
        v_(str)
    {}

    string_input(const Ch* data, std::size_t length) :
        v_(data, length)
    {}

    template <class Allocator>
    explicit string_input(const std::basic_string<Ch, Tr, Allocator>& str)
        noexcept :
        v_(str)
    {}

    explicit string_input(std::basic_string_view<Ch, Tr> str) noexcept :
        v_(str)
    {}

    string_input(const string_input& other) = default;
    string_input& operator=(const string_input& other) = default;

    size_type operator()(Ch* out, size_type n)
    {
        const auto len = v_.copy(out, n);
        v_.remove_prefix(len);
        return len;
    }
};

template <class Ch, class Tr = std::char_traits<Ch>,
    class Allocator = std::allocator<Ch>>
class owned_string_input
{
public:
    using string_type = std::basic_string<Ch, Tr, Allocator>;
    using size_type = typename string_type::size_type;

private:
    std::basic_string<Ch, Tr, Allocator> s_;
    size_type head_;

public:
    static_assert(std::is_same_v<Ch, typename Tr::char_type>, "");

    using char_type = Ch;
    using traits_type = Tr;

    owned_string_input()
        noexcept(std::is_nothrow_default_constructible_v<
            std::basic_string<Ch, Tr, Allocator>>) :
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

    owned_string_input(const owned_string_input& other) :
        s_(other.s_, other.head_, std::basic_string<Ch, Tr, Allocator>::npos,
            std::allocator_traits<Allocator>::
                select_on_container_copy_construction(
                    other.s_.get_allocator())),
        head_(0)
    {}

    owned_string_input(owned_string_input&& other) noexcept :
        s_(std::move(other.s_)), head_(other.head_)
    {
        other.head_ = other.s_.size();
    }

    ~owned_string_input() = default;

    owned_string_input& operator=(const owned_string_input& other)
    {
        assign(other, std::allocator_traits<Allocator>::
                        propagate_on_container_copy_assignment());
        return *this;
    }

private:
    void assign(const owned_string_input& other, std::true_type)
    {
        // This implementation inhibits s_'s buffer from being reused,
        // but it seems necessary so that s_'s allocator is replaced with
        // other.s_'s allocator (even if the allocators are equivalent)
        s_ = std::basic_string<Ch, Tr, Allocator>(
            other.s_, other.head_, other.s_.get_allocator());   // throw
        head_ = 0;
    }

    void assign(const owned_string_input& other, std::false_type)
    {
        s_.assign(other.s_, other.head_);   // throw
        head_ = 0;
    }

public:
    owned_string_input& operator=(owned_string_input&& other)
        noexcept(std::is_nothrow_move_assignable_v<
            std::basic_string<Ch, Tr, Allocator>>)
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

    void swap(owned_string_input& other) noexcept(
        std::is_nothrow_swappable_v<std::basic_string<Ch, Tr, Allocator>>)
    {
        using std::swap;
        swap(s_, other.s_);
        swap(head_, other.head_);
    }
};

template <class Ch, class Tr, class Allocator>
void swap(owned_string_input<Ch, Tr, Allocator>& left,
          owned_string_input<Ch, Tr, Allocator>& right)
    noexcept(noexcept(left.swap(right)))
{
    left.swap(right);
}

template <class Ch, class Tr>
streambuf_input<Ch, Tr> make_char_input(std::basic_streambuf<Ch, Tr>* in)
{
    return streambuf_input<Ch, Tr>(in);
}

template <class Ch, class Tr>
streambuf_input<Ch, Tr> make_char_input(
    std::basic_istream<Ch, Tr>& in) noexcept
{
    return streambuf_input<Ch, Tr>(in);
}

template <class Streambuf>
auto make_char_input(Streambuf&& in)
    noexcept(std::is_nothrow_move_constructible_v<Streambuf>)
 -> std::enable_if_t<
        !std::is_lvalue_reference_v<Streambuf>
     && std::is_base_of_v<
            std::basic_streambuf<
                typename Streambuf::char_type,
                typename Streambuf::traits_type>,
            Streambuf>,
        owned_streambuf_input<Streambuf>>
{
    return owned_streambuf_input<Streambuf>(std::move(in));
}

template <class IStream>
auto make_char_input(IStream&& in)
    noexcept(std::is_nothrow_move_constructible_v<IStream>)
 -> std::enable_if_t<
        !std::is_lvalue_reference_v<IStream>
     && std::is_base_of_v<
            std::basic_istream<
                typename IStream::char_type,
                typename IStream::traits_type>,
            IStream>,
        owned_istream_input<IStream>>
{
    return owned_istream_input<IStream>(std::move(in));
}

template <class Ch, class Tr = std::char_traits<Ch>>
auto make_char_input(const Ch* in)
 -> std::enable_if_t<
        std::is_same_v<Ch, char> || std::is_same_v<Ch, wchar_t>,
        string_input<Ch, Tr>>
{
    return string_input<Ch, Tr>(in);
}

template <class Ch, class Tr = std::char_traits<Ch>>
auto make_char_input(const Ch* in, std::size_t length)
 -> std::enable_if_t<
        std::is_same_v<Ch, char> || std::is_same_v<Ch, wchar_t>,
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

template <class Ch, class Tr>
string_input<Ch, Tr> make_char_input(
    std::basic_string_view<Ch, Tr> in) noexcept
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
