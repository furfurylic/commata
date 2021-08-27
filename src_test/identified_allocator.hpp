/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_3486BCC7_6E1A_41C9_9FAA_F6A71268A80F
#define FURFURYLIC_3486BCC7_6E1A_41C9_9FAA_F6A71268A80F

#include <cstddef>
#include <type_traits>

namespace commata { namespace test {

template <class T, bool Pocca, bool Pocma, bool Pocs, bool Iae = false>
class identified_allocator
{
    std::size_t id_;

public:
    template <class U>
    struct rebind
    {
        using other = identified_allocator<U, Pocca, Pocma, Pocs, Iae>;
    };

    using value_type = T;

    identified_allocator() noexcept :
        identified_allocator(static_cast<std::size_t>(-1))
    {}

    explicit identified_allocator(std::size_t id) noexcept :
        id_(id)
    {}

    // To make a rebound copy
    template <class U>
    explicit identified_allocator(
            const identified_allocator<U, Pocca, Pocma, Pocs, Iae>& other)
        noexcept :
        id_(other.id())
    {}

    // Copy/move ops are defaulted

    auto allocate(std::size_t n)
    {
        return reinterpret_cast<T*>(new char[sizeof(T) * n]);   // throw
    }

    void deallocate(T* p, std::size_t) noexcept
    {
        delete [] reinterpret_cast<char*>(p);
    }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100)
#endif
    template <class U>
    bool operator==(const identified_allocator<
        U, Pocca, Pocma, Pocs, Iae>& other) const noexcept
    {
        return Iae || (id() == other.id());
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    template <class U>
    bool operator!=(const identified_allocator<
        U, Pocca, Pocma, Pocs, Iae>& other) const noexcept
    {
        return !(*this == other);
    }

    using propagate_on_container_copy_assignment =
        std::integral_constant<bool, Pocca>;
    using propagate_on_container_move_assignment =
        std::integral_constant<bool, Pocma>;
    using propagate_on_container_swap =
        std::integral_constant<bool, Pocs>;

    std::size_t id() const noexcept
    {
        return id_;
    }
};

}}

#endif
