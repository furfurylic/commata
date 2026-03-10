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
    using c_at_t = typename std::allocator_traits<Allocator>::template
        rebind_traits<char>;
    typename c_at_t::allocator_type a(alloc);
    const auto p = c_at_t::allocate(a, sizeof(T));                  // throw
    try {
        ::new(static_cast<T*>(static_cast<void*>(&*p)))
            T(std::forward<Args>(args)...);                         // throw
    } catch (...) {
        a.deallocate(p, sizeof(T));
        throw;
    }

    using t_at_t = typename std::allocator_traits<Allocator>::template
        rebind_traits<T>;
    using t_p_t = typename t_at_t::pointer;
    return std::pointer_traits<t_p_t>::pointer_to(
                *static_cast<T*>(static_cast<void*>(&*p)));
}

template <class Allocator, class P>
void destroy_deallocate_g_impl(
    const Allocator& alloc, P p, std::size_t size_of)
{
    assert(p);
    using v_t = typename std::pointer_traits<P>::element_type;
    p->~v_t();
    using c_at_t = typename std::allocator_traits<Allocator>::template
        rebind_traits<char>;
    using c_p_t = typename c_at_t::pointer;
    typename c_at_t::allocator_type a(alloc);
    c_at_t::deallocate(
        a,
        std::pointer_traits<c_p_t>::pointer_to(
            *static_cast<char*>(static_cast<void*>(std::addressof(*p)))),
        size_of);
}

// Destructs an object with no allocator
// and deallocates memory with a rebound copy of the specified allocator
// (The deallocation size is obtained with sizeof(*p),
// so *p's dynamic type and its static type shall be the same type)
template <class Allocator, class P>
void destroy_deallocate_g_static(const Allocator& alloc, P p)
{
    using v_t = typename std::pointer_traits<P>::element_type;
    assert(p && (typeid(*p) == typeid(v_t)));

    destroy_deallocate_g_impl(alloc, p, sizeof(*p));
}

// Destructs an object with no allocator
// and deallocates memory with a rebound copy of the specified allocator
// (The deallocation size is obtained with p->size_of())
template <class Allocator, class P>
void destroy_deallocate_g_dynamic(const Allocator& alloc, P p)
{
    assert(p);
    destroy_deallocate_g_impl(alloc, p, p->size_of());
}

}

#endif
