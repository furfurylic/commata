/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_F6071085_9476_4724_95ED_E1DA376E837C
#define COMMATA_GUARD_F6071085_9476_4724_95ED_E1DA376E837C

#include <memory>
#include <type_traits>
#include <utility>

#include "member_like_base.hpp"

namespace commata::detail {

namespace allocation_only {

template <class A, class = void>
struct reference_forwarded
{
    using type = typename A::value_type&;
};

template <class A>
struct reference_forwarded<A, typename A::reference>
{
    using type = typename A::reference;
};

template <class A, class = void>
struct const_reference_forwarded
{
    using type = const typename A::value_type&;
};

template <class A>
struct const_reference_forwarded<A, typename A::const_reference>
{
    using type = typename A::const_reference;
};

} // end allocation_only

template <class A>
class allocation_only_allocator :
    member_like_base<A>
{
    using base_traits_t = typename std::allocator_traits<A>;

public:
    using pointer = typename base_traits_t::pointer;
    using const_pointer = typename base_traits_t::const_pointer;
    using void_pointer = typename base_traits_t::void_pointer;
    using const_void_pointer = typename base_traits_t::const_void_pointer;
    using value_type = typename base_traits_t::value_type;
    using size_type = typename base_traits_t::size_type;
    using difference_type = typename base_traits_t::difference_type;

    // These types are not required by the C++14 standard, but
    // std::basic_string which comes with gcc 7.3.1 seems to do
    using reference =
        typename allocation_only::reference_forwarded<A>::type;
    using const_reference =
        typename allocation_only::const_reference_forwarded<A>::type;

    template <class U>
    struct rebind
    {
        using other = allocation_only_allocator<
            typename base_traits_t::template rebind_alloc<U>>;
    };

    // Default-constructibility of an allocator is not mandated by the C++14
    // standard, but std::basic_string which comes with gcc 7.3.1 requires it
    allocation_only_allocator() = default;

    // To make wrappers
    explicit allocation_only_allocator(const A& other)
            noexcept(std::is_nothrow_copy_constructible_v<A>):
        member_like_base<A>(other)
    {}

    // ditto
    explicit allocation_only_allocator(A&& other)
            noexcept(std::is_nothrow_move_constructible_v<A>) :
        member_like_base<A>(std::move(other))
    {}

    // To make rebound copies
    template <class B>
    explicit allocation_only_allocator(
        const allocation_only_allocator<B>& other)
            noexcept(std::is_nothrow_constructible_v<A, const B&>) :
        member_like_base<A>(other.base())
    {}

    // ditto
    template <class B>
    explicit allocation_only_allocator(
        allocation_only_allocator<B>&& other)
            noexcept(std::is_nothrow_constructible_v<A, B&&>) :
        member_like_base<A>(std::move(other.base()))
    {}

    // copy/move ctor/assignment ops are defaulted

    template <class... Args>
    auto allocate(size_type n, Args&&... args)
    {
        return base_traits_t::allocate(base(), n, std::forward<Args>(args)...);
    }

    auto deallocate(pointer p, size_type n)
    {
        return base_traits_t::deallocate(base(), p, n);
    }

    size_type max_size() const noexcept
        // this noexceptness is mandated in the spec of
        // std::allocator_traits<A>::max_size
    {
        return base_traits_t::max_size(base());
    }

    template <class T, class... Args>
    void construct(T* p, Args&&... args)
    {
        ::new(p) T(std::forward<Args>(args)...);
    }

    template <class T>
    void destroy(T* p)
    {
        destroy(p, std::is_trivially_destructible<T>());
    }

    allocation_only_allocator select_on_container_copy_construction() const
        noexcept(noexcept(base_traits_t::select_on_container_copy_construction(
                            std::declval<const A&>())))
    {
        return base_traits_t::select_on_container_copy_construction(base());
    }

    using propagate_on_container_copy_assignment =
        typename base_traits_t::propagate_on_container_copy_assignment;
    using propagate_on_container_move_assignment =
        typename base_traits_t::propagate_on_container_move_assignment;
    using propagate_on_container_swap =
        typename base_traits_t::propagate_on_container_swap;
    using is_always_equal = typename base_traits_t::is_always_equal;

    decltype(auto) base() noexcept
    {
        return this->get();
    }

    decltype(auto) base() const noexcept
    {
        return this->get();
    }

private:
    template <class T>
    void destroy(T*, std::true_type)
    {}

    template <class T>
    void destroy(T* p, std::false_type)
    {
        p->~T();
    }
};

template <class AllocatorL, class AllocatorR>
bool operator==(
    const allocation_only_allocator<AllocatorL>& left,
    const allocation_only_allocator<AllocatorR>& right)
    noexcept(noexcept(std::declval<const AllocatorL&>()
                   == std::declval<const AllocatorR&>()))
{
    return left.base() == right.base();
}

template <class AllocatorL, class AllocatorR>
bool operator==(
    const allocation_only_allocator<AllocatorL>& left,
    const AllocatorR& right)
    noexcept(noexcept(std::declval<const AllocatorL&>()
                   == std::declval<const AllocatorR&>()))
{
    return left.base() == right;
}

template <class AllocatorL, class AllocatorR>
bool operator==(
    const AllocatorL& left,
    const allocation_only_allocator<AllocatorR>& right)
    noexcept(noexcept(std::declval<const AllocatorL&>()
                   == std::declval<const AllocatorR&>()))
{
    return left == right.base();
}

template <class AllocatorL, class AllocatorR>
bool operator!=(
    const allocation_only_allocator<AllocatorL>& left,
    const allocation_only_allocator<AllocatorR>& right)
    noexcept(noexcept(std::declval<const AllocatorL&>()
                   != std::declval<const AllocatorR&>()))
{
    return left.base() != right.base();
}

template <class AllocatorL, class AllocatorR>
bool operator!=(
    const allocation_only_allocator<AllocatorL>& left,
    const AllocatorR& right)
    noexcept(noexcept(std::declval<const AllocatorL&>()
                   != std::declval<const AllocatorR&>()))
{
    return left.base() != right;
}

template <class AllocatorL, class AllocatorR>
bool operator!=(
    const AllocatorL& left,
    const allocation_only_allocator<AllocatorR>& right)
    noexcept(noexcept(std::declval<const AllocatorL&>()
                   != std::declval<const AllocatorR&>()))
{
    return left != right.base();
}

}

#endif
