/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_E8F031F6_10D8_4585_9012_CFADC2F95BA7
#define COMMATA_GUARD_E8F031F6_10D8_4585_9012_CFADC2F95BA7

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <type_traits>
#include <utility>

#include "detail/formatted_output.hpp"

namespace commata {

class text_error :
    public std::exception
{
    struct what_holder
    {
        virtual ~what_holder() {}
        virtual const char* what() const noexcept = 0;
    };

    template <class Tr, class Allocator>
    class string_holder : public what_holder
    {
        std::basic_string<char, Tr, Allocator> s_;

    public:
        template <class T>
        explicit string_holder(T&& s) : s_(std::forward<T>(s))
        {}

        string_holder(const string_holder&) = delete;
        ~string_holder() = default;

        const char* what() const noexcept override
        {
            return s_.c_str();
        }
    };

    std::shared_ptr<what_holder> what_;
    std::pair<std::size_t, std::size_t> pos_;

public:
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    text_error() noexcept :
        pos_(npos, npos)
    {}

    template <class Tr>
    explicit text_error(std::basic_string_view<char, Tr> what_arg) :
        text_error(std::string(what_arg))
    {}

    template <class Tr, class Allocator>
    explicit text_error(std::basic_string<char, Tr, Allocator>&& what_arg) :
        what_(std::make_shared<string_holder<Tr, Allocator>>(
                std::move(what_arg))),
        pos_(npos, npos)
    {}

    explicit text_error(const char* what_arg) :
        text_error(std::string(what_arg))
    {}

    text_error(const text_error& other) = default;
    text_error(text_error&& other) = default;

    text_error& operator=(const text_error& other) noexcept
    {
        std::exception::operator=(other);
        what_ = other.what_;
        pos_ = other.pos_;
        // According to C++17 23.4.2 (1), pair's copy assignment does not throw
        // but are not marked as noexcept
        return *this;
    }

    text_error& operator=(text_error&& other) = default;

    const char* what() const noexcept override
    {
        return what_ ? what_->what() : "";
    }

    text_error& set_physical_position(
        std::size_t line = npos, std::size_t col = npos) noexcept
    {
        pos_ = std::make_pair(line, col);
        return *this;
    }

    std::optional<std::pair<std::size_t, std::size_t>>
        get_physical_position() const noexcept
    {
        if (pos_ != std::make_pair(npos, npos)) {
            return pos_;
        } else {
            return std::nullopt;
        }
    }
};

class text_error_info
{
    const text_error* ex_;
    std::size_t base_;

public:
    text_error_info(const text_error& ex, std::size_t base = 1) noexcept :
        ex_(std::addressof(ex)), base_(base)
    {}

    text_error_info(const text_error_info& ex) = default;
    text_error_info& operator=(const text_error_info& ex) = default;

    const text_error& error() const noexcept
    {
        return *ex_;
    }

    std::size_t get_base() const noexcept
    {
        return base_;
    }
};

namespace detail::ex {

using length_t = std::common_type_t<std::make_unsigned_t<std::streamsize>,
                                    std::size_t>;

constexpr std::streamsize nmax = std::numeric_limits<std::streamsize>::max();
constexpr length_t unmax = static_cast<length_t>(nmax);

namespace literals {

constexpr char na                [] = "n/a";
constexpr char and_line          [] = "; line ";
constexpr char text_error_at_line[] = "Text error at line ";
constexpr char column            [] = " column ";

}

constexpr std::size_t print_pos_min_n = std::max<std::size_t>(
    std::numeric_limits<std::size_t>::digits10 + 2, sizeof literals::na);

// Prints a non-negative integer value in the decimal system
// into a sufficient-length buffer
template <std::size_t N>
std::size_t print_pos(char (&s)[N], std::size_t pos, std::size_t base)
{
    using literals::na;
    static_assert(N >= print_pos_min_n);
    static_assert(na[(sizeof na) - 1] == '\0');
    const auto len = (pos != text_error::npos)
                  && (text_error::npos - base >= pos) ?
        std::snprintf(s, N, "%zu", pos + base) :
        (std::memcpy(s, na, sizeof na), (sizeof na) - 1);
    assert((len > 0) && (static_cast<std::size_t>(len) < N));
        // According to the spec of snprintf, len shall not be greater than
        // the maximum value of the type of N, which is std::size_t,
        // so casting it to std::size_t shall be safe
    assert(s[len] == '\0');
    return static_cast<std::size_t>(len);
}

template <class T1, class T2>
constexpr std::optional<std::common_type_t<T1, T2>> add(T1 a, T2 b)
{
    static_assert(std::is_unsigned_v<T1>);
    static_assert(std::is_unsigned_v<T2>);
    using ret_t = std::common_type_t<T1, T2>;
    static_assert(std::is_unsigned_v<ret_t>);
    if (std::numeric_limits<ret_t>::max() - a >= b) {
        return static_cast<ret_t>(a) + b;
    } else {
        return std::nullopt;
    }
}

template <class T1, class T2, class... Ts>
constexpr std::optional<std::common_type_t<T1, T2, Ts...>> add(
    T1 a, T2 b, Ts... cs)
{
    if (const auto bcs = add(b, cs...)) {
        return add(a, *bcs);
    } else {
        return std::nullopt;
    }
}

template <class Ch, class Tr>
class sputn_engine
{
    const std::ctype<Ch>* facet_;
    std::exception_ptr ex_;

public:
    explicit sputn_engine(std::basic_ostream<Ch, Tr>& os) noexcept :
        facet_(nullptr), ex_(nullptr)
    {
        // noexcept-ness counts; we must throw exceptions only when the sentry
        // exists and beholds us
        try {
            facet_ = &std::use_facet<std::ctype<Ch>>(os.getloc());
        } catch (...) {
            ex_ = std::current_exception();
        }
    }

    bool operator()(std::basic_streambuf<Ch, Tr>* sb,
        const char* s, std::size_t n) const
    {
        if (ex_) {
            std::rethrow_exception(ex_);
        }
        for (std::size_t i = 0; i < n; ++i) {
            if (sb->sputc(facet_->widen(s[i])) == Tr::eof()) {
                return false;
            }
        }
        return true;
    }

    template <std::size_t N>
    bool operator()(std::basic_streambuf<Ch, Tr>* sb, const char(&s)[N]) const
    {
        // s shall be null terminated, so s[N - 1] is null
        assert(s[N - 1] == Ch());
        return (*this)(sb, s, N - 1);
    }
};

template <class Tr>
class sputn_engine<char, Tr>
{
public:
    explicit sputn_engine(std::basic_ostream<char, Tr>&) noexcept
    {}

    bool operator()(std::basic_streambuf<char, Tr>* sb,
        const char* s, std::size_t n) const
    {
        if constexpr (std::numeric_limits<std::size_t>::max() > unmax) {
            while (n > unmax) {
                if (sb->sputn(s, nmax) != nmax) {
                    return false;
                }
                s += unmax;
                n -= static_cast<std::size_t>(unmax);
                    // safe because n > unmax
            }
        }
        return sb->sputn(s, n) == static_cast<std::streamsize>(n);
    }

    template <std::size_t N>
    bool operator()(std::basic_streambuf<char, Tr>* sb,
        const char(&s)[N]) const
    {
        // s shall be null terminated, so s[N - 1] is null
        assert(s[N - 1] == char());
        return (*this)(sb, s, N - 1);
    }
};

} // end detail::ex

template <class Ch, class Tr>
std::basic_ostream<Ch, Tr>& operator<<(
    std::basic_ostream<Ch, Tr>& os, const text_error_info& i)
{
    using namespace detail::ex;

    const auto p = i.error().get_physical_position();

    if (!p) {
        return os << i.error().what();
    }

    // line
    char l[detail::ex::print_pos_min_n];
    const auto l_len = print_pos(l, p->first, i.get_base());

    // column
    char c[detail::ex::print_pos_min_n];
    const auto c_len = print_pos(c, p->second, i.get_base());

    // what
    const char* const w = i.error().what();
    const auto w_len = std::strlen(w);

    const auto n = add(w_len, l_len, c_len,
        ((w_len > 0) ?
            std::size(literals::and_line) :
            std::size(literals::text_error_at_line)),
        std::size(literals::column) - 2);    // 2 is for two EOSs

    sputn_engine sputn(os);

    return detail::formatted_output(os,
        (((!n || (*n > unmax))) ? nmax : static_cast<std::streamsize>(*n)),
            // If n is greater than the largest possible specifiable width,
            // then by right no padding should take place, so inhibiting
            // padding by specifying unmax as the length is reasonable
        [sputn, w, w_len, l = &l[0], l_len, c = &c[0], c_len]
        (auto* sb) {
            if (w_len > 0) {
                if (!(sputn(sb, w, w_len)
                   && sputn(sb, literals::and_line))) {
                    return false;
                }
            } else if (!sputn(sb, literals::text_error_at_line)) {
                return false;
            }
            return sputn(sb, l, l_len)
                && sputn(sb, literals::column)
                && sputn(sb, c, c_len);
        });
}

inline std::string to_string(const text_error_info& i)
{
    std::ostringstream s;
    s << i;
    return std::move(s).str();
}

inline std::wstring to_wstring(const text_error_info& i)
{
    std::wostringstream s;
    s << i;
    return std::move(s).str();
}

}

#endif
