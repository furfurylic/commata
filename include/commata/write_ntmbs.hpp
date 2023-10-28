/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_CC28B744_3844_4C86_9375_7FFB78261ABD
#define COMMATA_GUARD_CC28B744_3844_4C86_9375_7FFB78261ABD

#include <cstddef>
#include <cstdio>
#include <cwchar>
#include <iomanip>
#include <iterator>
#include <limits>
#include <new>
#include <optional>
#include <ostream>
#include <streambuf>
#include <string>
#include <type_traits>

namespace commata::detail {

namespace ntmbs {

template <class Ch>
constexpr std::size_t nchar()
{
    auto n = std::numeric_limits<std::make_unsigned_t<Ch>>::max();
    std::size_t i = 0;
    do {
        n >>= 1;
        ++i;
    } while (n > 0U);
    return (i + 7) / 8 * 2;
}

template <class Ch>
bool write_hexadecimal(std::streambuf* sb, Ch c)
{
    constexpr auto eof = std::char_traits<char>::eof();
    char hex[nchar<Ch>() + 1];
    const auto n = std::snprintf(hex, std::size(hex), "%x", c);
    if (sb->sputc('[') == eof) return false;
    if (n > 0) {
        if (sb->sputc('0') == eof) return false;
        if (sb->sputc('x') == eof) return false;
        for (std::size_t i = n; i < nchar<Ch>(); ++i) {
            if (sb->sputc('0') == eof) return false;
        }
        if (sb->sputn(hex, n) != n) return false;
    }
    return sb->sputc(']') != eof;
}

} // end ntmbs

template <class InputIterator, class InputIteratorEnd>
auto write_ntmbs(std::streambuf* sb, const std::locale&,
                 InputIterator begin, InputIteratorEnd end)
 -> std::enable_if_t<
        std::is_same_v<
            std::remove_const_t<
                typename std::iterator_traits<InputIterator>::value_type>,
            char>>
{
    // [begin, end] may be a NTMBS, so we cannot determine whether a char
    // is an unprintable one or not easily
    // (Of course we can first convert multibyte chars to wide chars,
    // but we know C's global locale is often unreliable)
    for (; !(begin == end); ++begin) {
        const auto c = *begin;
        if (c == '\0') {
            ntmbs::write_hexadecimal(sb, c);
        } else {
            sb->sputc(c);
        }
    }
}

template <class InputIterator, class InputIteratorEnd>
auto write_ntmbs(std::streambuf* sb, const std::locale& loc,
                 InputIterator begin, InputIteratorEnd end)
 -> std::enable_if_t<
        std::is_same_v<
            std::remove_const_t<
                typename std::iterator_traits<InputIterator>::value_type>,
            wchar_t>>
{
    // MB_CUR_MAX is not necessarily a constant;
    // we employ std::string for its no-allocation potential
    std::optional<std::string> s_store;
    char* s = nullptr;
    try {
        s_store.emplace();              // throw
        s_store->reserve(MB_CUR_MAX);   // throw
        s = s_store->data();
    } catch (...) {
        s_store.reset();
    }

    using ct_t = std::ctype<wchar_t>;
    const ct_t* facet = nullptr;
    try {
        facet = &std::use_facet<ct_t>(loc);
    } catch (...) {}

    std::mbstate_t state = {};
    for (; !(begin == end); ++begin) {
        const auto c = *begin;
        if (s && facet) {
            bool printable = false;
            try {
                printable = facet->is(ct_t::print, c);
            } catch (...) {}
            if (printable) {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif
                const auto n = std::wcrtomb(s, c, &state);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
                if ((n != static_cast<std::size_t>(-1)) && (n > 0)) {
                    sb->sputn(s, n);
                    continue;
                }
            }
        }
        ntmbs::write_hexadecimal(sb, c);
    }
}

// An unformatted output function (C++17 30.7.5.3)
template <class InputIterator, class InputIteratorEnd>
void write_ntmbs(std::ostream& os, InputIterator begin, InputIteratorEnd end)
{
    std::ostream::sentry s(os);                             // throw
    if (!s) {
        return;
    }
    try {
        write_ntmbs(os.rdbuf(), os.getloc(), begin, end);   // throw
    } catch (...) {
        // Set badbit without causing an std::ios::failure to be thrown
        // (C++17 30.7.5.3.1)
        try {
            os.setstate(std::ios_base::badbit);             // throw
        } catch (std::ios::failure&) {
        }
        // And then rethrow the exception if an exception is expected by
        // badbit (C++17 30.7.5.3.1)
        if ((os.exceptions() & std::ios_base::badbit) != 0) {
            throw;                                          // throw
        }
    }
}

}

#endif
