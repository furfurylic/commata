/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_5BAA6AA2_1785_4F1B_AB40_3FC95A78EB2B
#define COMMATA_GUARD_5BAA6AA2_1785_4F1B_AB40_3FC95A78EB2B

#include <ios>
#include <ostream>
#include <streambuf>

namespace commata {
namespace detail {

template <class Ch, class Tr, class F>
std::basic_ostream<Ch, Tr>& formatted_output(
    std::basic_ostream<Ch, Tr>& os, std::streamsize n, F put_obj)
{
    const typename std::basic_ostream<Ch, Tr>::sentry s(os);        // throw
    auto w = os.width(0);

    const auto pad = [&os, n, w] {
        if (w > n) {
            const auto sb = os.rdbuf();
            const auto f = os.fill();
            const auto pad = w - n;
            for (decltype(pad + 1) i = 0; i < pad; ++i) {
                if (sb->sputc(f) == Tr::eof()) {
                    return false;
                }
            }
        }
        return true;
    };
    const auto put = [&os, &put_obj] {
        return put_obj(os.rdbuf());
    };

    bool failed = true;
    if (s) {
        try  {
            failed = !(((os.flags() & std::ios_base::adjustfield)
                    == std::ios_base::left) ?
                put() && pad() : pad() && put());                   // throw
        } catch (...) {
            if (os.bad()) {
                throw;                                              // throw
            } else {
                // Set badbit, possibly rethrowing the original exception
                try {
                    os.setstate(std::ios_base::badbit);             // throw
                } catch (...) {
                }
                if ((os.exceptions() & std::ios_base::badbit) != 0) {
                    throw;                                          // throw
                }
                failed = false;
            }
        }
    }
    if (failed) {
        os.setstate(std::ios_base::failbit);                        // throw
    }
    return os;
}

}}

#endif
