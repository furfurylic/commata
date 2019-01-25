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

namespace commata {
namespace detail {

template <class A, bool POCCA, bool POCMA, bool POCS>
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
            POCCA, POCMA, POCS>;
    };

    explicit propagation_controlled_allocator(const A& alloc) :
          member_like_base<A>(alloc)
    {}

    template <class U>
    explicit propagation_controlled_allocator(
        const propagation_controlled_allocator<U, POCCA, POCMA, POCS>& other) :
        member_like_base<A>(A(other.base()))
    {}

    template <class U>
    explicit propagation_controlled_allocator(
        propagation_controlled_allocator<U, POCCA, POCMA, POCS>&& other) :
        member_like_base<A>(A(std::move(other.base())))
    {}

    // copy/move ctor/assignment op are defaulted

    template <class... Args>
    auto allocate(Args... args)
    {
        return this->get().allocate(std::forward<Args>(args)...);
    }

    template <class... Args>
    auto deallocate(Args... args)
    {
        return this->get().deallocate(std::forward<Args>(args)...);
    }

    size_type max_size()
    {
        return this->get().max_size();
    }

    bool operator==(const propagation_controlled_allocator& other) const
    {
        return this->get() == other.get();
    }

    template <class U>
    bool operator==(const propagation_controlled_allocator<
        U, POCCA, POCMA, POCS>& other) const
    {
        return *this ==
            typename propagation_controlled_allocator<U, POCCA, POCMA, POCS>::
                rebind::other(other);
    }

    template <class U>
    bool operator!=(const propagation_controlled_allocator<
        U, POCCA, POCMA, POCS>& other) const
    {
        return !(*this == other);
    }

    template <class T, class... Args>
    auto construct(T* p, Args&&... args)
    {
        return base_traits_t::construct(*this, p, std::forward<Args>(args)...);
    }

    template <class T>
    auto destroy(T* p)
    {
        return base_traits_t::destroy(*this, p);
    }

    propagation_controlled_allocator select_on_container_copy_construction()
        const
    {
        return propagation_controlled_allocator(
            this->get().select_on_container_copy_construction());
    }

    using propagate_on_container_copy_assignment =
        std::integral_constant<bool, POCCA>;
    using propagate_on_container_move_assignment =
        std::integral_constant<bool, POCMA>;
    using propagate_on_container_swap = std::integral_constant<bool, POCS>;

    decltype(auto) base()
    {
        return this->get();
    }

    decltype(auto) base() const
    {
        return this->get();
    }
};

}}

#endif
