/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_CC28B744_3844_4C86_9375_7FFB78261ABD
#define COMMATA_GUARD_CC28B744_3844_4C86_9375_7FFB78261ABD

#include <iomanip>
#include <limits>
#include <locale>
#include <ostream>
#include <streambuf>
#include <type_traits>

namespace commata { namespace detail {

namespace narrow {

inline std::ostream& set_hex(std::ostream& os)
{
    return os << std::showbase << std::hex << std::setfill('0');
}

template <class Ch>
static std::ostream& setw_char(std::ostream& os)
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127)
#endif
    constexpr auto m = std::numeric_limits<std::make_unsigned_t<Ch>>::max();
    if (m <= 0xff) {
        os << std::setw(2);
    } else if (m <= 0xffff) {
        os << std::setw(4);
    } else if (m <= 0xffffffff) {
        os << std::setw(8);
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    return os;
}

} // end narrow

template <class Ch>
void write_narrow(std::ostream& os, const Ch* begin, const Ch* end)
{
    using namespace narrow;

    const auto& facet =
        std::use_facet<std::ctype<Ch>>(std::locale());  // current global
    while (begin != end) {
        const auto c = *begin;
        if (c == Ch()) {
            os << '[' << set_hex << setw_char<Ch> << 0 << ']';
        } else if (!facet.is(std::ctype<Ch>::print, c)) {
            os << '[' << set_hex << setw_char<Ch> << (c + 0) << ']';
        } else {
            const auto d = facet.narrow(c, '\0');
            if (d == '\0') {
                os << '[' << set_hex << setw_char<Ch> << (c + 0) << ']';
            } else {
                os.rdbuf()->sputc(d);
            }
        }
        ++begin;
    }
}

inline void write_narrow(std::ostream& os, const char* begin, const char* end)
{
    using namespace narrow;

    // [begin, end] may be a NTMBS, so we cannot determine whether a char
    // is an unprintable one or not easily
    while (begin != end) {
        const auto c = *begin;
        if (c == '\0') {
            os << '[' << set_hex << setw_char<char> << 0 << ']';
        } else {
            os.rdbuf()->sputc(c);
        }
        ++begin;
    }
}

}}

#endif
