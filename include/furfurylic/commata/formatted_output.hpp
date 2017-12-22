/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_GUARD_5BAA6AA2_1785_4F1B_AB40_3FC95A78EB2B
#define FURFURYLIC_GUARD_5BAA6AA2_1785_4F1B_AB40_3FC95A78EB2B

#include <ios>
#include <ostream>
#include <streambuf>

namespace furfurylic {
namespace commata {
namespace detail {

template <class Ch, class Tr, class F>
std::basic_ostream<Ch, Tr>& formatted_output(
    std::basic_ostream<Ch, Tr>& os, std::streamsize n, F put_obj)
{
    const auto pad = [&os, n] {
        const auto w = os.width();
        if ((w > 0) && (static_cast<std::ios_base::streampos>(n) < w)) {
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

    const typename std::basic_ostream<Ch, Tr>::sentry s(os);
    if (!s) {
        os.setstate(std::ios_base::failbit);    // throw
    } else {
        try {
            if ((os.flags() & std::ios_base::adjustfield)
                    != std::ios_base::left) {
                if (!(pad() && put())) {
                    os.setstate(std::ios_base::failbit);    // throw
                }
            } else {
                if (!(put() && pad())) {
                    os.setstate(std::ios_base::failbit);    // throw
                }
            }
            os.width(0);
        } catch (...) {
            os.setstate(std::ios_base::badbit); // throw
        }
    }
    return os;
}

}}}

#endif
