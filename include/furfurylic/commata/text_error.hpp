/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_GUARD_E8F031F6_10D8_4585_9012_CFADC2F95BA7
#define FURFURYLIC_GUARD_E8F031F6_10D8_4585_9012_CFADC2F95BA7

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
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

class text_error;

class text_error_info
{
    const text_error* ex_;

public:
    explicit text_error_info(const text_error& ex) noexcept :
        ex_(&ex)
    {}

    const text_error& error() const noexcept
    {
        return *ex_;
    }
};

class text_error :
    public std::exception, public detail::npos_impl<std::size_t>
{
    std::shared_ptr<std::string> what_;
    std::pair<std::size_t, std::size_t> physical_position_;

public:
    template <class T,
        class = std::enable_if_t<std::is_constructible<std::string, T>::value>>
    explicit text_error(T&& what_arg) :
        what_(std::make_shared<std::string>(std::forward<T>(what_arg))),
        physical_position_(npos, npos)
    {}

    // Copy/move ctors and copy/move assignment ops are explicitly defined
    // so that they are noexcept

    text_error(const text_error& other) noexcept :
        std::exception(other),
        what_(other.what_),
        physical_position_(other.physical_position_)
        // According to C++14 20.3.2 (1), pair's copy ctor is noexcept
    {}

    text_error(text_error&& other) noexcept :
        std::exception(std::move(other)),
        what_(std::move(other.what_)),
        physical_position_(other.physical_position_)
        // ditto
    {}

    text_error& operator=(const text_error& other) noexcept
    {
        std::exception::operator=(other);
        what_ = other.what_;
        physical_position_ = other.physical_position_;
        // According to C++14 20.3.2 (1), pair's assignments are noexcept
        return *this;
    }

    text_error& operator=(text_error&& other) noexcept
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

    void set_physical_position(std::size_t line = npos, std::size_t col = npos)
    {
        physical_position_ = std::make_pair(line, col);
    }

    const std::pair<std::size_t, std::size_t>* get_physical_position() const
        noexcept
    {
        // std::make_pair<std::size_t, std::size_t> shall not throw exceptions
        return (physical_position_ != std::make_pair(npos, npos)) ?
            &physical_position_ : nullptr;
    }

    text_error_info info() const noexcept
    {
        return text_error_info(*this);
    }
};

namespace detail {

// Prints a non-negative integer value in the decimal system
// into a sufficient-length buffer
template <std::size_t N>
std::streamsize print_pos(char (&s)[N], std::size_t pos)
{
    const auto len = (pos != text_error::npos) ?
        std::snprintf(s, N, "%zu", pos + 1) :
        std::snprintf(s, N, "n/a");
    assert((len > 0 ) && (static_cast<std::size_t>(len) < N));
    return static_cast<std::streamsize>(len);
}

template <std::size_t N>
std::streamsize print_pos(wchar_t (&s)[N], std::size_t pos)
{
    const auto len = (pos != text_error::npos) ?
        std::swprintf(s, N, L"%zu", pos + 1) :
        std::swprintf(s, N, L"n/a");
    assert((len > 0 ) && (static_cast<std::size_t>(len) < N));
    return static_cast<std::streamsize>(len);
}

} // end namespace detail

template <class Tr>
std::basic_ostream<char, Tr>& operator<<(
    std::basic_ostream<char, Tr>& os, const text_error_info& i)
{
    if (const auto p = i.error().get_physical_position()) {
        // line
        char l[std::numeric_limits<std::size_t>::digits10 + 2];
        const auto l_len = detail::print_pos(l, p->first);

        // column
        char c[sizeof(l)];
        const auto c_len = detail::print_pos(c, p->second);

        // what
        const auto w = i.error().what();
        const auto w_len = static_cast<std::streamsize>(std::strlen(w));

        const auto n = w_len + l_len + c_len + ((w_len > 0) ? 15 : 27);

        return detail::formatted_output(os, n,
            [w, w_len, l = &l[0], l_len, c = &c[0], c_len]
            (auto* sb) {
                if (w_len > 0) {
                    if ((sb->sputn(w, w_len) != w_len)
                     || (sb->sputn("; line ", 7) != 7)) {
                        return false;
                    }
                } else if (sb->sputn("Text error at line ", 19) != 19) {
                    return false;
                }
                return (sb->sputn(l, l_len) == l_len)
                    && (sb->sputn(" column ", 8) == 8)
                    && (sb->sputn(c, c_len) == c_len);
            });

    } else {
        return os << i.error().what();
    }
}

template <class Tr>
std::basic_ostream<wchar_t, Tr>& operator<<(
    std::basic_ostream<wchar_t, Tr>& os, const text_error_info& i)
{
    // Count the wide characters in what, which may be an NTMBS
    auto w_raw = i.error().what();
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif
    auto w_len = static_cast<std::streamsize>(std::mbstowcs(0, w_raw, 0));
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    if (w_len == -1) {
        // Conversion failed
        w_raw = "";
        w_len = 0;
    }

    if (const auto p = i.error().get_physical_position()) {
        // line
        wchar_t l[std::numeric_limits<std::size_t>::digits10 + 2];
        const auto l_len = detail::print_pos(l, p->first);

        // column
        wchar_t c[sizeof(l)];
        const auto c_len = detail::print_pos(c, p->second);

        const auto n = w_len + l_len + c_len + ((w_len > 0) ? 15 : 27);

        return detail::formatted_output(os, n,
            [w_raw, w_len, l = &l[0], l_len, c = &c[0], c_len]
            (auto* sb) {
                if (w_len > 0) {
                    std::unique_ptr<wchar_t[]> w(
                        new wchar_t[w_len + 1]);                    // throw
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif
                    std::mbstowcs(w.get(), w_raw, w_len);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
                    w[w_len] = L'\0';   // maybe not required
                    if ((sb->sputn(w.get(), w_len) != w_len)
                     || (sb->sputn(L"; line ", 7) != 7)) {
                        return false;
                    }
                } else if (sb->sputn(L"Text error at line ", 19) != 19) {
                    return false;
                }
                return (sb->sputn(l, l_len) == l_len)
                    && (sb->sputn(L" column ", 8) == 8)
                    && (sb->sputn(c, c_len) == c_len);
            });

    } else if (w_len > 0) {
        return detail::formatted_output(os, w_len,
            [w_raw, &w_len]
            (auto* sb) {
                std::unique_ptr<wchar_t[]> w(
                    new wchar_t[w_len + 1]);                        // throw
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif
                std::mbstowcs(w.get(), w_raw, w_len);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
                w[w_len] = L'\0';   // maybe not required
                return sb->sputn(w.get(), w_len) == w_len;
            });

    } else {
        return os;
    }
}

inline std::string to_string(const text_error_info& i)
{
    std::ostringstream s;
    s << i;
    return s.str();
}

inline std::wstring to_wstring(const text_error_info& i)
{
    std::wostringstream s;
    s << i;
    return s.str();
}

}}

#endif
