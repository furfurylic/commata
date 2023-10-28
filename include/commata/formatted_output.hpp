/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_5BAA6AA2_1785_4F1B_AB40_3FC95A78EB2B
#define COMMATA_GUARD_5BAA6AA2_1785_4F1B_AB40_3FC95A78EB2B

#include <ios>
#include <ostream>
#include <streambuf>

namespace commata::detail {

template <class Ch, class Tr, class F>
std::basic_ostream<Ch, Tr>& formatted_output(
    std::basic_ostream<Ch, Tr>& os, std::streamsize n, F put_obj)
{
    const typename std::basic_ostream<Ch, Tr>::sentry s(os);    // throw
    if (s) {
        bool sets_failbit = true;
        try  {
            const auto pad = [&os, n, w = os.width()] {
                if (w > n) {
                    const auto sb = os.rdbuf();
                    const auto f = os.fill();
                    for (std::streamsize i = 0, ie = w - n; i < ie; ++i) {
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
            sets_failbit = !(((os.flags() & std::ios_base::adjustfield)
                           == std::ios_base::left) ?
                put() && pad() : pad() && put());               // throw
        } catch (...) {
            // Set badbit without causing an std::ios::failure to be thrown
            // (C++17 30.7.5.2.1)
            try {
                os.setstate(std::ios_base::badbit);             // throw
            } catch (std::ios::failure&) {
            }
            // And then rethrow the exception if an exception is expected by
            // badbit (C++17 30.7.5.2.1)
            if ((os.exceptions() & std::ios_base::badbit) != 0) {
                throw;                                          // throw
            }
            sets_failbit = false;
        }
        // According to C++17 30.7.5.2.1, setting failbit *does not seem*
        // required when the sentry is not sound
        if (sets_failbit) {
            os.setstate(std::ios_base::failbit);                // throw
        }
        // According to C++17 30.7.5.2.1, setting width to 0 is not required in
        // any way when the sentry is not sound
        os.width(0);
    }
    return os;
}

}

#endif
