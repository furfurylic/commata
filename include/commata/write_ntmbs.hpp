/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_CC28B744_3844_4C86_9375_7FFB78261ABD
#define COMMATA_GUARD_CC28B744_3844_4C86_9375_7FFB78261ABD

#include <cstddef>
#include <cwchar>
#include <iomanip>
#include <iterator>
#include <limits>
#include <new>
#include <ostream>
#include <streambuf>
#include <string>
#include <type_traits>

namespace commata { namespace detail {

namespace ntmbs {

class string_buffer
{
    alignas(std::string) char store_[sizeof(std::string)];
    char* s_;

public:
    explicit string_buffer(std::size_t size) :
        s_(nullptr)
    {
        try {
            ::new(store_) std::string;      // throw
            const auto p = reinterpret_cast<std::string*>(store_);
            try {
                p->reserve(size);           // throw
                s_ = &(*p)[0];
            } catch (...) {
                reinterpret_cast<std::string*>(store_)->~basic_string();
            }
        } catch (...) {}
    }

    ~string_buffer()
    {
        if (s_) {
            reinterpret_cast<std::string*>(store_)->~basic_string();
        }
    }

    char* operator()()
    {
        return s_;
    }
};

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

inline std::ostream& set_hex(std::ostream& os)
{
    return os << std::noshowbase << std::nouppercase
              << std::hex << std::right << std::setfill('0');
}

template <class Ch>
std::ostream& setw_char(std::ostream& os)
{
    return os << std::setw(nchar<Ch>());
}

class flags
{
    std::ostream* os_;
    std::ios_base::fmtflags flags_;
    char fill_;

public:
    explicit flags(std::ostream& os) :
        os_(&os), flags_(os.flags()), fill_(os.fill())
    {}

    ~flags()
    {
        try {   // just in case
            os_->setf(flags_);
            *os_ << std::setw(0) << std::setfill(fill_);
        } catch (...) {}
    }
};

} // end ntmbs

template <class InputIterator, class InputIteratorEnd>
auto write_ntmbs(std::ostream& os, InputIterator begin, InputIteratorEnd end)
 -> std::enable_if_t<
        std::is_same<
            std::remove_const_t<
                typename std::iterator_traits<InputIterator>::value_type>,
            wchar_t>::value>
{
    using namespace ntmbs;

    // MB_CUR_MAX is not necessarily a constant;
    // we employ std::string for its no-allocation potential
    string_buffer s(MB_CUR_MAX);

    using ct_t = std::ctype<wchar_t>;
    const ct_t* facet = nullptr;
    try {
        facet = &std::use_facet<ct_t>(os.getloc());
    } catch (...) {}

    std::mbstate_t state = {};
    const flags f(os);
    for (; begin != end; ++begin) {
        const auto c = *begin;
        if (s() && facet) {
            bool printable = false;
            try {
                printable = facet->is(ct_t::print, c);
            } catch (...) {}
            if (printable) {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif
                const auto n = std::wcrtomb(s(), c, &state);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
                if (n > 0) {
                    os.rdbuf()->sputn(s(), n);
                    continue;
                }
            }
        }
        // We assume this is a rare case, so flags are set lazily
        os << std::setw(0)
           << "[0x" << set_hex << setw_char<wchar_t> << (c + 0) << ']';
    }
}

template <class InputIterator, class InputIteratorEnd>
auto write_ntmbs(std::ostream& os, InputIterator begin, InputIteratorEnd end)
 -> std::enable_if_t<
        std::is_same<
            std::remove_const_t<
                typename std::iterator_traits<InputIterator>::value_type>,
            char>::value>
{
    using namespace ntmbs;

    // [begin, end] may be a NTMBS, so we cannot determine whether a char
    // is an unprintable one or not easily
    // (Of course we can first convert multibyte chars to wide chars,
    // but we know C's global locale is often unreliable)
    const flags f(os);
    for (; begin != end; ++begin) {
        const auto c = *begin;
        if (c == '\0') {
            // We assume this is a rare case, so flags are set lazily
            os << std::setw(0)
               << "[0x" << set_hex << setw_char<char> << 0 << ']';
        } else {
            os.rdbuf()->sputc(c);
        }
    }
}

}}

#endif
