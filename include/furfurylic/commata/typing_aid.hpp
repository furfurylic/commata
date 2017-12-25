/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_GUARD_9F3EFB02_5C6D_449D_A895_01323928E6BC
#define FURFURYLIC_GUARD_9F3EFB02_5C6D_449D_A895_01323928E6BC

#include <string>
#include <type_traits>

namespace furfurylic {
namespace commata {
namespace detail {

template <class T>
struct is_std_string :
    std::false_type
{};

template <class... Args>
struct is_std_string<std::basic_string<Args...>> :
    std::true_type
{};

}}}

#endif
