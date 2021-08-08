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
#include <limits>
#include <memory>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <type_traits>
#include <utility>

#include "formatted_output.hpp"
#include "typing_aid.hpp"

namespace commata {
namespace detail { namespace ex {

template <class T>
struct npos_impl
{
    constexpr static T npos = static_cast<T>(-1);
};

// To define this in a header, npos_impl is a template
template <class T>
constexpr T npos_impl<T>::npos;

}} // end detail::ex

class text_error_info;

class text_error :
    public std::exception, public detail::ex::npos_impl<std::size_t>
{
    struct what_holder
    {
        virtual ~what_holder() {}
        virtual const char* what() const noexcept = 0;
    };

    template <class S>
    class string_holder : public what_holder
    {
        S s_;

    public:
        template <class T,
            class = std::enable_if_t<std::is_constructible<S, T>::value>>
        string_holder(T&& s) : s_(std::forward<T>(s))
        {}

        ~string_holder() = default;
        string_holder(const string_holder&) = delete;

        const char* what() const noexcept override
        {
            return s_.c_str();
                // std::basic_string<Ch, Tr, Allocator>::c_str is noexcept
        }
    };

    std::shared_ptr<what_holder> what_;
    std::pair<std::size_t, std::size_t> physical_position_;

public:
    text_error() noexcept :
        physical_position_(npos, npos)
    {}

    template <class T,
        std::enable_if_t<
            detail::is_std_string_of_ch<std::decay_t<T>, char>::value
         || std::is_constructible<std::string, T>::value,
            std::nullptr_t> = nullptr>
    explicit text_error(T&& what_arg) :
        what_(create_what(
            detail::is_std_string_of_ch<std::decay_t<T>, char>(),
            std::forward<T>(what_arg))),
        physical_position_(npos, npos)
    {}

private:
    template <class T>
    static auto create_what(std::true_type, T&& what_arg)
    {
        return std::allocate_shared<string_holder<std::decay_t<T>>>(
            what_arg.get_allocator(), std::forward<T>(what_arg));
    }

    template <class T>
    static auto create_what(std::false_type, T&& what_arg)
    {
        return std::make_shared<string_holder<std::string>>(
            std::forward<T>(what_arg));
    }

public:
    text_error(const text_error& other) = default;
    text_error(text_error&& other) = default;

    text_error& operator=(const text_error& other) noexcept
    {
        std::exception::operator=(other);
        what_ = other.what_;
        physical_position_ = other.physical_position_;
        // According to C++14 20.3.2 (1), pair's assignments do not throw
        // but are not declared as noexcept
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
        return what_ ? what_->what() : "";
    }

    text_error& set_physical_position(
        std::size_t line = npos, std::size_t col = npos) noexcept
    {
        physical_position_ = std::make_pair(line, col);
        return *this;
    }

    const std::pair<std::size_t, std::size_t>* get_physical_position() const
        noexcept
    {
        return (physical_position_ != std::make_pair(npos, npos)) ?
            &physical_position_ : nullptr;
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

namespace detail { namespace ex {

// Prints a non-negative integer value in the decimal system
// into a sufficient-length buffer
template <std::size_t N>
std::streamsize print_pos(char (&s)[N], std::size_t pos, std::size_t base)
{
    const auto len = (pos != text_error::npos)
                  && (text_error::npos - base >= pos) ?
        std::snprintf(s, N, "%zu", pos + base) :
        std::snprintf(s, N, "n/a");
    assert((len > 0 ) && (static_cast<std::size_t>(len) < N));
    return static_cast<std::streamsize>(len);
}

template <std::size_t N>
std::streamsize print_pos(wchar_t (&s)[N], std::size_t pos, std::size_t base)
{
    const auto len = (pos != text_error::npos)
                  && (text_error::npos - base >= pos) ?
        std::swprintf(s, N, L"%zu", pos + base) :
        std::swprintf(s, N, L"n/a");
    assert((len > 0 ) && (static_cast<std::size_t>(len) < N));
    return static_cast<std::streamsize>(len);
}

template <class Ch, class Tr, std::size_t N>
bool sputn(std::basic_streambuf<Ch, Tr>* sb, const Ch(&s)[N])
{
    // s shall be null terminated, so s[N - 1] is null
    return sb->sputn(s, N - 1) == N - 1;
}

template <class Ch, class Tr>
bool sputn(std::basic_streambuf<Ch, Tr>* sb, const Ch* s, std::streamsize n)
{
    return sb->sputn(s, n) == n;
}

}} // end detail::ex

template <class Tr>
std::basic_ostream<char, Tr>& operator<<(
    std::basic_ostream<char, Tr>& os, const text_error_info& i)
{
    using namespace detail::ex;

    if (const auto p = i.error().get_physical_position()) {
        // line
        char l[std::numeric_limits<std::size_t>::digits10 + 2];
        const auto l_len = print_pos(l, p->first, i.get_base());

        // column
        char c[sizeof(l)];
        const auto c_len = print_pos(c, p->second, i.get_base());

        // what
        const auto w = i.error().what();
        const auto w_len = static_cast<std::streamsize>(std::strlen(w));

        const auto n = w_len + l_len + c_len + ((w_len > 0) ? 15 : 27);

        return detail::formatted_output(os, n,
            [w, w_len, l = &l[0], l_len, c = &c[0], c_len]
            (auto* sb) {
                if (w_len > 0) {
                    if (!sputn(sb, w, w_len)
                     || !sputn(sb, "; line ")) {
                        return false;
                    }
                } else if (!sputn(sb, "Text error at line ")) {
                    return false;
                }
                return sputn(sb, l, l_len)
                    && sputn(sb, " column ")
                    && sputn(sb, c, c_len);
            });

    } else {
        return os << i.error().what();
    }
}

template <class Tr>
std::basic_ostream<wchar_t, Tr>& operator<<(
    std::basic_ostream<wchar_t, Tr>& os, const text_error_info& i)
{
    using namespace detail::ex;

    if (const auto p = i.error().get_physical_position()) {
        // line
        wchar_t l[std::numeric_limits<std::size_t>::digits10 + 2];
        const auto l_len = print_pos(l, p->first, i.get_base());

        // column
        wchar_t c[sizeof(l) / sizeof(wchar_t)];
        const auto c_len = print_pos(c, p->second, i.get_base());

        // what
        const auto w_raw = i.error().what();
        const auto w_len = static_cast<std::streamsize>(std::strlen(w_raw));

        const auto n = w_len + l_len + c_len + ((w_len > 0) ? 15 : 27);

        return detail::formatted_output(os, n,
            [&os, w_raw, w_len, l = &l[0], l_len, c = &c[0], c_len]
            (auto* sb) {
                if (w_len > 0) {
                    for (std::streamsize j = 0; j < w_len; ++j) {
                        if (sb->sputc(os.widen(w_raw[j])) == Tr::eof()) {
                            return false;
                        }
                    }
                    if (!sputn(sb, L"; line ")) {
                        return false;
                    }
                } else if (!sputn(sb, L"Text error at line ")) {
                    return false;
                }
                return sputn(sb, l, l_len)
                    && sputn(sb, L" column ")
                    && sputn(sb, c, c_len);
            });

    } else {
        return os << i.error().what();
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

}

#endif
