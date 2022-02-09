/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_4E257056_FA43_4ED4_BF21_5638E8C46B14
#define COMMATA_GUARD_4E257056_FA43_4ED4_BF21_5638E8C46B14

#include <algorithm>
#include <cstddef>
#include <memory>
#include <limits>

namespace commata { namespace detail {

template <class Allocator>
std::size_t sanitize_buffer_size(
    std::size_t buffer_size, Allocator alloc) noexcept
{
    constexpr auto buffer_size_max = std::numeric_limits<std::size_t>::max();
    if (buffer_size == 0U) {
        constexpr auto default_buffer_size =
            std::min(buffer_size_max, static_cast<std::size_t>(8192U));
        buffer_size = default_buffer_size;
    }
    const auto max_alloc0 = std::allocator_traits<Allocator>::max_size(alloc);
    const auto max_alloc = std::min(buffer_size_max, max_alloc0);
    return std::min(buffer_size, max_alloc);
}

}}

#endif
