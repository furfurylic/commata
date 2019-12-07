/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_89204D85_0074_4F42_A210_A47FB5BD0B7C
#define COMMATA_GUARD_89204D85_0074_4F42_A210_A47FB5BD0B7C

namespace commata { namespace detail {

template <class Ch>
struct nul
{
    static Ch value;
};

template <class Ch>
Ch nul<Ch>::value = Ch();

}}

#endif
