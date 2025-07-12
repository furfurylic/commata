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

#include "detail/typing_aid.hpp"

namespace commata {

namespace detail::input {

template <class Ch, class Tr>
auto getn(std::basic_streambuf<Ch, Tr>& in, Ch* out, std::streamsize n)
{
    return in.sgetn(out, n);
}

template <class Ch, class Tr>
auto getn(std::basic_istream<Ch, Tr>& in, Ch* out, std::streamsize n)
{
    in.read(out, n);
    return in.gcount();
}

template <class In>
auto read(In& in,
    typename In::char_type* out, std::make_unsigned_t<std::streamsize> n)
{
    constexpr decltype(n) nmax = std::numeric_limits<std::streamsize>::max();
    decltype(n) m = 0;
    while (n > nmax) {
        const auto mm = getn(in, out, nmax);
        if (mm == 0) {
            return m;
        }
        m += mm;
        n -= mm;
        out += mm;
    }
    m += getn(in, out, n);
    return m;
}

} // end detail::input

template <class Ch, class Tr = std::char_traits<Ch>>
class streambuf_input
{
    std::basic_streambuf<Ch, Tr>* in_;

public:
    static_assert(std::is_same_v<Ch, typename Tr::char_type>);

    using streambuf_type = std::basic_streambuf<Ch, Tr>;
    using char_type = Ch;
    using traits_type = Tr;
    using size_type = std::make_unsigned_t<std::streamsize>;

    streambuf_input() noexcept :
        in_(nullptr)
    {}

    explicit streambuf_input(std::basic_streambuf<Ch, Tr>& in) noexcept :
        in_(std::addressof(in))
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
            Streambuf>);

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
        if (this != std::addressof(other)) {
            using std::swap;
            swap(in_, other.in_);
        }
    }
};

template <class Streambuf>
auto swap(owned_streambuf_input<Streambuf>& left,
          owned_streambuf_input<Streambuf>& right)
    noexcept(noexcept(left.swap(right)))
 -> std::enable_if_t<std::is_swappable_v<Streambuf>>
{
    left.swap(right);
}

template <class Ch, class Tr = std::char_traits<Ch>>
class istream_input
{
    std::basic_istream<Ch, Tr>* in_;

public:
    static_assert(std::is_same_v<Ch, typename Tr::char_type>);

    using istream_type = std::basic_istream<Ch, Tr>;
    using char_type = Ch;
    using traits_type = Tr;
    using size_type = std::make_unsigned_t<std::streamsize>;

    istream_input() noexcept :
        in_(nullptr)
    {}

    explicit istream_input(std::basic_istream<Ch, Tr>& in) noexcept :
        in_(std::addressof(in))
    {}

    istream_input(const istream_input& other) = default;
    istream_input& operator=(const istream_input&) =default;

    size_type operator()(Ch* out, size_type n)
    {
        return in_ ? detail::input::read(*in_, out, n) : 0;
    }
};

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
            IStream>);

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
        return detail::input::read(in_, out, n);
    }

    void swap(owned_istream_input& other)
        noexcept(std::is_nothrow_swappable_v<IStream>)
    {
        if (this != std::addressof(other)) {
            using std::swap;
            swap(in_, other.in_);
        }
    }
};

template <class IStream>
auto swap(owned_istream_input<IStream>& left,
          owned_istream_input<IStream>& right)
    noexcept(noexcept(left.swap(right)))
 -> std::enable_if_t<std::is_swappable_v<IStream>>
{
    left.swap(right);
}

template <class Ch, class Tr = std::char_traits<Ch>>
class string_input
{
    std::basic_string_view<Ch, Tr> v_;

public:
    static_assert(std::is_same_v<Ch, typename Tr::char_type>);

    using char_type = Ch;
    using traits_type = Tr;
    using size_type = std::size_t;

    static constexpr size_type npos = static_cast<size_type>(-1);

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

    std::pair<const Ch*, size_type> operator()(size_type n = npos) noexcept
    {
        const auto rlen = std::min(n, v_.size());
        std::pair<const Ch*, size_type> r(v_.data(), rlen);
        v_.remove_prefix(rlen);
        return r;
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
    // Owned string
    std::basic_string<Ch, Tr, Allocator> s_;

    // [0, head_) is the consumed range in s_
    size_type head_;

    // A character that must be written back on s_[head_] prior to the next
    // read
    // (s_[head_] may have been overwritten by the reader on the previous read)
    Ch front_;

public:
    static_assert(std::is_same_v<Ch, typename Tr::char_type>);

    using char_type = Ch;
    using traits_type = Tr;

    static constexpr size_type npos = static_cast<size_type>(-1);

    owned_string_input()
        noexcept(std::is_nothrow_default_constructible_v<
            std::basic_string<Ch, Tr, Allocator>>) :
        s_(), head_(0), front_(s_[head_])
    {}

    explicit owned_string_input(std::basic_string<Ch, Tr, Allocator>&& str)
        noexcept :
        s_(std::move(str)), head_(0), front_(s_[head_])
    {}

    owned_string_input(owned_string_input&& other) noexcept :
        s_(std::move(other.s_)),
        head_(std::exchange(other.head_, other.s_.size())), // depletes other
        front_(other.front_)
    {}

    ~owned_string_input() = default;

    owned_string_input& operator=(owned_string_input&& other)
        noexcept(std::is_nothrow_move_assignable_v<
            std::basic_string<Ch, Tr, Allocator>>)
    {
        s_ = std::move(other.s_);
        head_ = std::exchange(other.head_, other.s_.size());// depletes other
        front_ = other.front_;
        return *this;
    }

    size_type operator()(Ch* out, size_type n)
    {
        s_[head_] = front_;
        const auto len = s_.copy(out, n, head_);
        head_ += len;
        front_ = s_[head_];
        return len;
    }

    std::pair<Ch*, size_type> operator()(size_type n = npos) noexcept
    {
        s_[head_] = front_;
        const auto rlen = std::min(n, s_.size() - head_);
        std::pair<Ch*, size_type> r(s_.data() + head_, rlen);
        head_ += rlen;
        front_ = s_[head_];
        return r;
    }

    void swap(owned_string_input& other) noexcept(
        std::is_nothrow_swappable_v<std::basic_string<Ch, Tr, Allocator>>)
    {
        using std::swap;
        swap(s_, other.s_);
        swap(head_, other.head_);
        swap(front_, other.front_);
    }
};

template <class Ch, class Tr, class Allocator>
auto swap(owned_string_input<Ch, Tr, Allocator>& left,
          owned_string_input<Ch, Tr, Allocator>& right)
    noexcept(noexcept(left.swap(right)))
{
    left.swap(right);
}

template <class Input>
class indirect_input
{
    Input input_;

public:
    using base_type = Input;
    using char_type = typename Input::char_type;
    using traits_type = typename Input::traits_type;
    using size_type = typename Input::size_type;

    explicit indirect_input(Input input)
        noexcept(std::is_nothrow_move_constructible_v<Input>) :
        input_(std::move(input))
    {}

    indirect_input() = default;
    indirect_input(const indirect_input&) = default;
    indirect_input(indirect_input&&) = default;
    ~indirect_input() = default;
    indirect_input& operator=(const indirect_input& other) = default;
    indirect_input& operator=(indirect_input&& other) = default;

    const Input& base() const noexcept
    {
        return input_;
    }

    Input& base() noexcept
    {
        return input_;
    }

    size_type operator()(char_type* out, size_type n)
    {
        return input_(out, n);
    }

    void swap(indirect_input& other) noexcept(
        std::is_nothrow_swappable_v<Input>)
    {
        using std::swap;
        swap(input_, other.input_);
    }
};

template <class Input>
auto swap(indirect_input<Input>& left, indirect_input<Input>& right)
    noexcept(noexcept(left.swap(right)))
 -> std::enable_if_t<std::is_swappable_v<Input>>
{
    left.swap(right);
}

struct indirect_t
{};

constexpr inline indirect_t indirect{};

template <class Ch, class Tr>
[[nodiscard]] streambuf_input<Ch, Tr> make_char_input(
    std::basic_streambuf<Ch, Tr>& in) noexcept
{
    return streambuf_input(in);
}

template <class Ch, class Tr>
[[nodiscard]] istream_input<Ch, Tr> make_char_input(
    std::basic_istream<Ch, Tr>& in) noexcept
{
    return istream_input(in);
}

template <class Streambuf>
[[nodiscard]] auto make_char_input(Streambuf&& in)
    noexcept(std::is_nothrow_move_constructible_v<
                    std::remove_reference_t<Streambuf>>)
 -> std::enable_if_t<
        !std::is_lvalue_reference_v<Streambuf>
     && std::is_base_of_v<
            std::basic_streambuf<
                typename std::remove_reference_t<Streambuf>::char_type,
                typename std::remove_reference_t<Streambuf>::traits_type>,
            std::remove_reference_t<Streambuf>>,
        owned_streambuf_input<Streambuf>>
{
    return owned_streambuf_input(std::forward<Streambuf>(in));
}

template <class IStream>
[[nodiscard]] auto make_char_input(IStream&& in)
    noexcept(std::is_nothrow_move_constructible_v<
                    std::remove_reference_t<IStream>>)
 -> std::enable_if_t<
        !std::is_lvalue_reference_v<IStream>
     && std::is_base_of_v<
            std::basic_istream<
                typename std::remove_reference_t<IStream>::char_type,
                typename std::remove_reference_t<IStream>::traits_type>,
            std::remove_reference_t<IStream>>,
        owned_istream_input<IStream>>
{
    return owned_istream_input(std::forward<IStream>(in));
}

template <class Ch, class Tr = std::char_traits<Ch>>
[[nodiscard]] auto make_char_input(const Ch* in)
 -> std::enable_if_t<
        std::is_same_v<Ch, char> || std::is_same_v<Ch, wchar_t>,
        string_input<Ch, Tr>>
{
    return string_input(in);
}

template <class Ch, class Tr = std::char_traits<Ch>>
[[nodiscard]] auto make_char_input(const Ch* in, std::size_t length)
 -> std::enable_if_t<
        std::is_same_v<Ch, char> || std::is_same_v<Ch, wchar_t>,
        string_input<Ch, Tr>>
{
    return string_input(in, length);
}

template <class Ch, class Tr, class Allocator>
[[nodiscard]] string_input<Ch, Tr> make_char_input(
    const std::basic_string<Ch, Tr, Allocator>& in) noexcept
{
    return string_input(in);
}

template <class Ch, class Tr>
[[nodiscard]] string_input<Ch, Tr> make_char_input(
    std::basic_string_view<Ch, Tr> in) noexcept
{
    return string_input(in);
}

template <class Ch, class Tr, class Allocator>
[[nodiscard]] owned_string_input<Ch, Tr, Allocator> make_char_input(
    std::basic_string<Ch, Tr, Allocator>&& in) noexcept
{
    return owned_string_input(std::move(in));
}

namespace detail {

struct are_make_char_input_args_impl
{
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
#endif
    template <class... Args>
    static auto check(std::void_t<Args...>*) -> decltype(
        make_char_input(std::declval<Args>()...),
        std::true_type());
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

    template <class... Args>
    static auto check(indirect_t, std::void_t<Args...>* p) -> decltype(
        check<Args...>(p));

    template <class...>
    static auto check(...) -> std::false_type;
};

template <class... Args>
constexpr bool are_make_char_input_args_v =
    decltype(are_make_char_input_args_impl::check<Args...>(nullptr))();

namespace input {

template <class... Args>
struct char_input_for_primitive
{
    using type = decltype(make_char_input(std::declval<Args>()...));
};

template <class...>
struct char_input_for
{};

template <class Head, class... Tails>
struct char_input_for<Head, Tails...>
{
    using type = typename std::conditional_t<
        std::is_base_of_v<indirect_t, std::decay_t<Head>>,
        char_input_for<Tails...>,
        char_input_for_primitive<Head, Tails...>>::type;
};

template <class... Args>
constexpr auto make_char_input_noexcept_na(Args&&...)
 -> std::bool_constant<
        noexcept(make_char_input(std::declval<Args>()...))
     && std::is_move_constructible_v<
            decltype(make_char_input(std::declval<Args>()...))>>;

template <class Input>
constexpr auto make_char_input_noexcept_na(Input&&)
 -> std::enable_if_t<!are_make_char_input_args_v<Input>,
        std::is_move_constructible<std::decay_t<Input>>>;

template <class... Args>
constexpr auto make_char_input_noexcept_na(indirect_t, Args&&...)
 -> decltype(make_char_input_noexcept_na(std::declval<Args>()...));

} // end input

} // end detail

template <class Input>
[[nodiscard]]
indirect_input<Input> make_char_input(indirect_t, indirect_input<Input> input)
{
    return indirect_input(input.base());
}

template <class... Args>
[[nodiscard]] auto make_char_input(indirect_t, Args&&... args) noexcept(
    decltype(detail::input::make_char_input_noexcept_na(
        std::declval<Args>()...))())
 -> std::enable_if_t<
        detail::are_make_char_input_args_v<Args...> || (sizeof...(Args) == 1),
        indirect_input<typename std::conditional_t<
            detail::are_make_char_input_args_v<Args...>,
            detail::input::char_input_for<Args...>,
            std::decay<detail::first_t<Args...>>
        >::type>>
{
    if constexpr (std::is_base_of_v<indirect_t,
                                    std::decay_t<detail::first_t<Args...>>>) {
        return make_char_input(std::forward<Args>(args)...);    // recursive
    } else if constexpr (detail::are_make_char_input_args_v<Args...>) {
        return indirect_input(make_char_input(std::forward<Args>(args)...));
    } else {
        return indirect_input(std::forward<Args>(args)...);
    }
}

}

#endif
