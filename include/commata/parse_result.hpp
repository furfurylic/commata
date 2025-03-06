/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_790E88FB_FBFE_410B_B392_5C22F0D892E3
#define COMMATA_GUARD_790E88FB_FBFE_410B_B392_5C22F0D892E3

#include <cstddef>

namespace commata {

class parse_result
{
    bool good_;
    std::size_t parse_point_;

public:
    parse_result(bool good, std::size_t offset) noexcept :
        good_(good), parse_point_(offset)
    {}

    parse_result(const parse_result& other) noexcept = default;
    parse_result& operator=(const parse_result& other) noexcept = default;

    explicit operator bool() const noexcept
    {
        return good_;
    }

    std::size_t get_parse_point() const noexcept
    {
        return parse_point_;
    }
};

inline std::size_t get_parse_point(const parse_result& r) noexcept
{
    return r.get_parse_point();
}

}

#endif
