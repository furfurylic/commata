/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_7BD1D77E_5440_472C_9FD5_204E09DEE54D
#define COMMATA_GUARD_7BD1D77E_5440_472C_9FD5_204E09DEE54D

#include <cassert>
#include <memory>

namespace commata::detail {

// Allocates memory with a rebound copy of the specified allocator
// and constructs an object with no allocator
template <class T, class Allocator, class... Args>
[[nodiscard]] auto allocate_construct_g(const Allocator& alloc, Args&&... args)
{
    using t_at_t = typename std::allocator_traits<Allocator>::template
        rebind_traits<T>;
    typename t_at_t::allocator_type a(alloc);
    const auto p = t_at_t::allocate(a, 1);                          // throw
    try {
        ::new(std::addressof(*p)) T(std::forward<Args>(args)...);   // throw
    } catch (...) {
        a.deallocate(p, 1);
        throw;
    }
    return p;
}

// Destructs an object with no allocator
// and deallocates memory with a rebound copy of the specified allocator
template <class Allocator, class P>
void destroy_deallocate_g(const Allocator& alloc, P p)
{
    assert(p);
    using v_t = typename std::pointer_traits<P>::element_type;
    p->~v_t();
    using t_at_t = typename std::allocator_traits<Allocator>::template
        rebind_traits<v_t>;
    typename t_at_t::allocator_type a(alloc);
    t_at_t::deallocate(a, p, 1);
}

}

#endif
