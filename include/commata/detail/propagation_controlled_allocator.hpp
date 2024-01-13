/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_22665F19_C38E_420B_BA0A_34827245F1F5
#define COMMATA_GUARD_22665F19_C38E_420B_BA0A_34827245F1F5

#include <memory>
#include <type_traits>
#include <utility>

#include "member_like_base.hpp"

namespace commata::detail {

template <class A, bool Pocca, bool Pocma, bool Pocs>
class propagation_controlled_allocator :
    member_like_base<A>
{
    using base_traits_t = std::allocator_traits<A>;

public:
    using pointer = typename base_traits_t::pointer;
    using const_pointer = typename base_traits_t::const_pointer;
    using void_pointer = typename base_traits_t::void_pointer;
    using const_void_pointer = typename base_traits_t::const_void_pointer;
    using value_type = typename base_traits_t::value_type;
    using size_type = typename base_traits_t::size_type;
    using difference_type = typename base_traits_t::difference_type;

    template <class U>
    struct rebind
    {
        using other = propagation_controlled_allocator<
            typename base_traits_t::template rebind_alloc<U>,
            Pocca, Pocma, Pocs>;
    };

    propagation_controlled_allocator(const A& alloc)
        noexcept(std::is_nothrow_copy_constructible_v<A>) :
        member_like_base<A>(alloc)
    {}

    propagation_controlled_allocator(A&& alloc)
        noexcept(std::is_nothrow_move_constructible_v<A>) :
        member_like_base<A>(std::move(alloc))
    {}

    // To make rebound copies
    template <class B>
    propagation_controlled_allocator(
        const propagation_controlled_allocator<B, Pocca, Pocma, Pocs>& other)
        noexcept(std::is_nothrow_constructible_v<A, const B&>) :
        member_like_base<A>(other.base())
    {}

    // ditto
    template <class B>
    propagation_controlled_allocator(
        propagation_controlled_allocator<B, Pocca, Pocma, Pocs>&& other)
        noexcept(std::is_nothrow_constructible_v<A, B&&>) :
        member_like_base<A>(std::move(other.base()))
    {}

    // copy/move ctor/assignment ops are defaulted

    template <class... Args>
    [[nodiscard]] auto allocate(size_type n, Args&&... args)
    {
        return base_traits_t::allocate(base(), n, std::forward<Args>(args)...);
    }

    auto deallocate(pointer p, size_type n)
    {
        return base_traits_t::deallocate(base(), p, n);
    }

    size_type max_size() noexcept
        // this noexceptness is mandated in the spec of
        // std::allocator_traits<A>::max_size
    {
        return base_traits_t::max_size(base());
    }

    template <class T, class... Args>
    auto construct(T* p, Args&&... args)
    {
        return base_traits_t::construct(*this, p, std::forward<Args>(args)...);
    }

    template <class T>
    auto destroy(T* p)
    {
        return base_traits_t::destroy(base(), p);
    }

    propagation_controlled_allocator select_on_container_copy_construction()
        const noexcept(noexcept(
            base_traits_t::select_on_container_copy_construction(
                std::declval<const A&>())))
    {
        return propagation_controlled_allocator(
            base().select_on_container_copy_construction());
    }

    using propagate_on_container_copy_assignment = std::bool_constant<Pocca>;
    using propagate_on_container_move_assignment = std::bool_constant<Pocma>;
    using propagate_on_container_swap = std::bool_constant<Pocs>;
    using is_always_equal = typename base_traits_t::is_always_equal;

    decltype(auto) base() noexcept
    {
        return this->get();
    }

    decltype(auto) base() const noexcept
    {
        return this->get();
    }
};

template <class AllocatorL, class AllocatorR, bool... PosL, bool... PosR>
bool operator==(
    const propagation_controlled_allocator<AllocatorL, PosL...>& left,
    const propagation_controlled_allocator<AllocatorR, PosR...>& right)
    noexcept(noexcept(std::declval<const AllocatorL&>()
                   == std::declval<const AllocatorR&>()))
{
    return left.base() == right.base();
}

template <class AllocatorL, class AllocatorR, bool... PosL>
bool operator==(
    const propagation_controlled_allocator<AllocatorL, PosL...>& left,
    const AllocatorR& right)
    noexcept(noexcept(std::declval<const AllocatorL&>()
                   == std::declval<const AllocatorR&>()))
{
    return left.base() == right;
}

template <class AllocatorL, class AllocatorR, bool... PosR>
bool operator==(
    const AllocatorL& left,
    const propagation_controlled_allocator<AllocatorR, PosR...>& right)
    noexcept(noexcept(std::declval<const AllocatorL&>()
                   == std::declval<const AllocatorR&>()))
{
    return left == right.base();
}

template <class AllocatorL, class AllocatorR, bool... PosL, bool... PosR>
bool operator!=(
    const propagation_controlled_allocator<AllocatorL, PosL...>& left,
    const propagation_controlled_allocator<AllocatorR, PosR...>& right)
    noexcept(noexcept(std::declval<const AllocatorL&>()
                   != std::declval<const AllocatorR&>()))
{
    return left.base() != right.base();
}

template <class AllocatorL, class AllocatorR, bool... PosL>
bool operator!=(
    const propagation_controlled_allocator<AllocatorL, PosL...>& left,
    const AllocatorR& right)
    noexcept(noexcept(std::declval<const AllocatorL&>()
                   != std::declval<const AllocatorR&>()))
{
    return left.base() != right;
}

template <class AllocatorL, class AllocatorR, bool... PosR>
bool operator!=(
    const AllocatorL& left,
    const propagation_controlled_allocator<AllocatorR, PosR...>& right)
    noexcept(noexcept(std::declval<const AllocatorL&>()
                   != std::declval<const AllocatorR&>()))
{
    return left != right.base();
}

}

#endif
