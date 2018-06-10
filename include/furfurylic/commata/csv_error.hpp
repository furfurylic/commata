/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_GUARD_E8F031F6_10D8_4585_9012_CFADC2F95BA7
#define FURFURYLIC_GUARD_E8F031F6_10D8_4585_9012_CFADC2F95BA7

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <exception>
#include <limits>
#include <locale>
#include <memory>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <type_traits>
#include <utility>

#include "formatted_output.hpp"

namespace furfurylic {
namespace commata {
namespace detail {

template <class T>
struct npos_impl
{
    constexpr static T npos = static_cast<T>(-1);
};

// To define this in a header, npos_impl is a template
template <class T>
constexpr T npos_impl<T>::npos;

} // end namespace detail

class csv_error_info;

class csv_error :
    public std::exception, public detail::npos_impl<std::size_t>
{
    std::shared_ptr<std::string> what_;
    std::pair<std::size_t, std::size_t> physical_position_;

public:
    template <class T,
        class = std::enable_if_t<std::is_constructible<std::string, T>::value>>
    explicit csv_error(T&& what_arg) :
        what_(std::make_shared<std::string>(std::forward<T>(what_arg))),
        physical_position_(npos, npos)
    {}

    // Copy/move ctors and copy/move assignment ops are explicitly defined
    // so that they are noexcept

    csv_error(const csv_error& other) noexcept :
        std::exception(other),
        what_(other.what_),
        physical_position_(other.physical_position_)
        // According to C++14 20.3.2 (1), pair's copy ctor is noexcept
    {}

    csv_error(csv_error&& other) noexcept :
        std::exception(std::move(other)),
        what_(std::move(other.what_)),
        physical_position_(other.physical_position_)
        // ditto
    {}

    csv_error& operator=(const csv_error& other) noexcept
    {
        std::exception::operator=(other);
        what_ = other.what_;
        physical_position_ = other.physical_position_;
        // According to C++14 20.3.2 (1), pair's assignments are noexcept
        return *this;
    }

    csv_error& operator=(csv_error&& other) noexcept
    {
        std::exception::operator=(std::move(other));
        what_ = std::move(other.what_);
        physical_position_ = other.physical_position_;
        // ditto
        return *this;
    }

    const char* what() const noexcept override
    {
        return what_->c_str(); // std::string::c_str is noexcept
    }

    void set_physical_position(std::size_t row = npos, std::size_t col = npos)
    {
        physical_position_ = std::make_pair(row, col);
    }

    const std::pair<std::size_t, std::size_t>* get_physical_position() const
        noexcept
    {
        // std::make_pair<std::size_t, std::size_t> shall not throw exceptions
        return (physical_position_ != std::make_pair(npos, npos)) ?
            &physical_position_ : nullptr;
    }

    csv_error_info info() const noexcept;
};

namespace detail {

// Prints a non-negative integer value in the decimal system
// into a sufficient-length buffer
template <std::size_t N>
std::streamsize print_pos(char (&s)[N], std::size_t pos)
{
    const auto len = (pos != csv_error::npos) ?
        std::snprintf(s, N, "%zu", pos + 1) :
        std::snprintf(s, N, "n/a");
    assert((len > 0 ) && (static_cast<std::size_t>(len) < N));
    return static_cast<std::streamsize>(len);
}

template <class Tr>
std::nullptr_t getloc(std::basic_ostream<char, Tr>&)
{
    return nullptr;
}

template <class Ch, class Tr>
std::locale getloc(std::basic_ostream<Ch, Tr>& os)
{
    return os.getloc();
}

template <class Tr>
bool sputn(std::nullptr_t,
    std::basic_streambuf<char, Tr>* sb, const char* s, std::streamsize n)
{
    return sb->sputn(s, n) == n;
}

template <class Ch, class Tr>
bool sputn(const std::locale& loc,
    std::basic_streambuf<Ch, Tr>* sb, const char* s, std::streamsize n)
{
    const auto& facet = std::use_facet<std::ctype<Ch>>(loc);
    for (std::streamsize i = 0; i < n; ++i) {
        const auto c = s[i];
        if (sb->sputc(facet.widen(c)) == Tr::eof()) {
            return false;
        }
    }
    return true;
}

} // end namespace detail

class csv_error_info
{
    const csv_error* ex_;

public:
    explicit csv_error_info(const csv_error& ex) noexcept :
        ex_(&ex)
    {}

private:
    template <class Ch, class Tr>
    friend std::basic_ostream<Ch, Tr>& operator<<(
        std::basic_ostream<Ch, Tr>& os, const csv_error_info& i);
};

template <class Ch, class Tr>
std::basic_ostream<Ch, Tr>& operator<<(
    std::basic_ostream<Ch, Tr>& os, const csv_error_info& i)
{
    const auto w = i.ex_->what();
    if (const auto p = i.ex_->get_physical_position()) {
        char l[std::numeric_limits<std::size_t>::digits10 + 2];
        char c[sizeof(l)];
        const auto w_len = static_cast<std::streamsize>(std::strlen(w));
        const auto l_len = detail::print_pos(l, p->first);
        const auto c_len = detail::print_pos(c, p->second);
        const auto n = w_len + l_len + c_len + 15/* =7+8. See below */;
        const auto loc = detail::getloc(os);
        return detail::formatted_output(os, n,
            [w, w_len, l = &l[0], l_len, c = &c[0], c_len, &loc]
            (auto* sb) {
                return detail::sputn(loc, sb, w, w_len)
                    && detail::sputn(loc, sb, "; line ", 7)
                    && detail::sputn(loc, sb, l, l_len)
                    && detail::sputn(loc, sb, " column ", 8)
                    && detail::sputn(loc, sb, c, c_len);
            });
    } else {
        return os << w;
    }
}

inline csv_error_info csv_error::info() const noexcept
{
    return csv_error_info(*this);
}

inline std::string to_string(const csv_error_info& i)
{
    std::ostringstream s;
    s << i;
    return s.str();
}

inline std::wstring to_wstring(const csv_error_info& i)
{
    std::wostringstream s;
    s << i;
    return s.str();
}

}}

#endif
