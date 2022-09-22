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

#include "formatted_output.hpp"
#include "typing_aid.hpp"

namespace commata {

class text_error_info;

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

    const std::pair<std::size_t, std::size_t>* get_physical_position() const
        noexcept
    {
        return (pos_ != std::make_pair(npos, npos)) ? &pos_ : nullptr;
    }

    text_error_info info(std::size_t base = 1U) const noexcept;
};

class text_error_info
{
    const text_error* ex_;
    std::size_t base_;

public:
    text_error_info(const text_error& ex, std::size_t base) noexcept :
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

inline text_error_info text_error::info(std::size_t base) const noexcept
{
    return text_error_info(*this, base);
}

namespace detail::ex {

using length_t = std::common_type_t<std::make_unsigned_t<std::streamsize>,
                                    std::size_t>;

constexpr std::streamsize nmax = std::numeric_limits<std::streamsize>::max();
constexpr length_t unmax = static_cast<length_t>(nmax);

// Prints a non-negative integer value in the decimal system
// into a sufficient-length buffer
template <std::size_t N>
std::size_t print_pos(char (&s)[N], std::size_t pos, std::size_t base)
{
    const auto len = (pos != text_error::npos)
                  && (text_error::npos - base >= pos) ?
        std::snprintf(s, N, "%zu", pos + base) :
        std::snprintf(s, N, "n/a");
    assert((len > 0 ) && (static_cast<std::size_t>(len) < N));
    return static_cast<std::size_t>(len);
}

template <class T>
std::optional<T> add(T a, T b)
{
    if (std::numeric_limits<T>::max() - a >= b) {
        return a + b;
    } else {
        return std::nullopt;
    }
}

template <class T, class... Ts>
std::optional<T> add(T a, T b, Ts... cs)
{
    if (const auto bcs = add<T>(b, cs...)) {
        return add<T>(a, *bcs);
    } else {
        return std::nullopt;
    }
}

struct literals
{
    constexpr static char and_line           [] = "; line ";
    constexpr static char text_error_at_line [] = "Text error at line ";
    constexpr static char column             [] = " column ";
};

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

template <class Ch, class Tr>
sputn_engine(std::basic_ostream<Ch, Tr>&) -> sputn_engine<Ch, Tr>;

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
    char l[std::numeric_limits<std::size_t>::digits10 + 2];
    const auto l_len = print_pos(l, p->first, i.get_base());

    // column
    char c[std::size(l)];
    const auto c_len = print_pos(c, p->second, i.get_base());

    // what
    const char* const w = i.error().what();
    const auto w_len = std::strlen(w);

    auto n = add<length_t>(w_len, l_len, c_len,
        ((w_len > 0) ?
            std::size(literals::and_line) :
            std::size(literals::text_error_at_line)),
        std::size(literals::column) - 2);    // 2 is for two EOSs
    if (!n || (*n > unmax)) {
        // more than largest possible padding length, when 'no padding'
        // does the trick
        n = 0;
    }

    sputn_engine sputn(os);

    return detail::formatted_output(os, static_cast<std::streamsize>(*n),
        [sputn, w, w_len, l = &l[0], l_len, c = &c[0], c_len]
        (auto* sb) {
            if (w_len > 0) {
                if (!sputn(sb, w, w_len)
                 || !sputn(sb, literals::and_line)) {
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
