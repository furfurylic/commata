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

namespace commata {

template <class Ch, class Tr = std::char_traits<Ch>>
class streambuf_input
{
    std::basic_streambuf<Ch, Tr>* in_;

public:
    static_assert(std::is_same<Ch, typename Tr::char_type>::value, "");

    using char_type = Ch;
    using traits_type = Tr;
    using size_type = std::make_unsigned_t<std::streamsize>;

    explicit streambuf_input(std::basic_streambuf<Ch, Tr>* in) :
        in_(in)
    {}

    explicit streambuf_input(std::basic_istream<Ch, Tr>& in) noexcept :
        streambuf_input(in.rdbuf())
    {}

    streambuf_input(const streambuf_input&) = default;
    ~streambuf_input() = default;

    size_type operator()(Ch* out, size_type n)
    {
        constexpr size_type nmax = std::numeric_limits<std::streamsize>::max();
        size_type m = 0;
        while (n > nmax) {
            const auto mm = in_->sgetn(out, nmax);
            if (mm == 0) {
                return m;
            }
            m += mm;
            n -= mm;
        }
        m += in_->sgetn(out, n);
        return m;
    }
};

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

    string_input(const string_input&) = default;
    ~string_input() = default;

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
};

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

    explicit owned_string_input(std::basic_string<Ch, Tr, Allocator>&& str)
        noexcept :
        s_(std::move(str)), head_(0)
    {}

    owned_string_input(owned_string_input&&) = default;
    ~owned_string_input() = default;

    size_type operator()(Ch* out, size_type n)
    {
        if (head_ < s_.size()) {
            const auto length = std::min(n, s_.size() - head_);
            Tr::copy(out, s_.data() + head_, length);
            head_ += length;
            return length;
        } else {
            return 0;
        }
    }
};

}

#endif
