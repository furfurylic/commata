/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_GUARD_DF869B02_BDA5_4CA3_9D83_8BFF19B6ECE5
#define FURFURYLIC_GUARD_DF869B02_BDA5_4CA3_9D83_8BFF19B6ECE5

#include "text_error.hpp"

namespace furfurylic {
namespace commata {

class parse_error :
    public text_error
{
public:
    using text_error::text_error;
};

}}

#endif
