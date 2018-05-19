/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_GUARD_F6071085_9476_4724_95ED_E1DA376E837C
#define FURFURYLIC_GUARD_F6071085_9476_4724_95ED_E1DA376E837C

#include <memory>
#include <type_traits>
#include <utility>

#include "member_like_base.hpp"

namespace furfurylic {
namespace commata {
namespace detail {

template <class Allocator>
class allocation_only_allocator :
    member_like_base<Allocator>
{
    using base_traits = typename std::allocator_traits<Allocator>;

    // To rebind
    template <class Allocator2>
    friend class allocation_only_allocator;

public:
    using pointer = typename base_traits::pointer;
    using const_pointer = typename base_traits::const_pointer;
    using void_pointer = typename base_traits::void_pointer;
    using const_void_pointer = typename base_traits::const_void_pointer;
    using value_type = typename base_traits::value_type;
    using size_type = typename base_traits::size_type;
    using difference_type = typename base_traits::difference_type;

    template <class U>
    struct rebind
    {
        using other = allocation_only_allocator<
            typename base_traits::template rebind_alloc<U>>;
    };

    // To make wrappers
    explicit allocation_only_allocator(const Allocator& other) :
        member_like_base<Allocator>(other)
    {}

    // ditto
    explicit allocation_only_allocator(Allocator&& other) :
        member_like_base<Allocator>(std::move(other))
    {}

    // To make rebound copies
    template <class Allocator2>
    explicit allocation_only_allocator(
        const allocation_only_allocator<Allocator2>& other) :
        member_like_base<Allocator>(Allocator(other.get()))
    {}

    template <class... Args>
    auto allocate(size_type n, Args&&... args)
    {
        return base_traits::allocate(base(),
            n, std::forward<Args>(args)...);
    }

    auto deallocate(pointer p, size_type n) noexcept
    {
        return base_traits::deallocate(base(), p, n);
    }

    auto max_size() noexcept
    {
        return base_traits::max_size(base());
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

    auto select_on_container_copy_construction() noexcept
    {
        return base_traits::select_on_container_copy_construction(base());
    }

    using propagate_on_container_copy_assignment =
        typename base_traits::propagate_on_container_copy_assignment;
    using propagate_on_container_move_assignment =
        typename base_traits::propagate_on_container_move_assignment;
    using propagate_on_container_swap =
        typename base_traits::propagate_on_container_swap;

    decltype(auto) base()
    {
        return this->get();
    }

    decltype(auto) base() const
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

template <class Allocator1, class Allocator2>
inline bool operator==(
    const allocation_only_allocator<Allocator1>& left,
    const allocation_only_allocator<Allocator2>& right)
{
    return left.base() == right.base();
}

template <class Allocator1, class Allocator2>
inline bool operator!=(
    const allocation_only_allocator<Allocator1>& left,
    const allocation_only_allocator<Allocator2>& right)
{
    return !(left == right);
}

}}}

#endif