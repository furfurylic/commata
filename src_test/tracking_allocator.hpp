/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_F2499770_B34D_40F0_9D61_D86483E76760
#define FURFURYLIC_F2499770_B34D_40F0_9D61_D86483E76760

#include <cassert>
#include <memory>
#include <utility>
#include <unordered_map>
#include <vector>

namespace commata { namespace test {

template <class BaseAllocator>
class tracking_allocator :
    public BaseAllocator
{
    std::vector<std::pair<char*, char*>>* allocated_;
    typename std::allocator_traits<BaseAllocator>::size_type* total_;

    template <class U>
    friend class tracking_allocator;

public:
    using base_traits = typename std::allocator_traits<BaseAllocator>;

    template <class U>
    struct rebind
    {
        using other =
            tracking_allocator<typename base_traits::template rebind_alloc<U>>;
    };

    // C++14 standard does not mandate default-constructibility of allocators
    // but some implementations require it
    tracking_allocator() noexcept :
        allocated_(nullptr), total_(nullptr)
    {}

    // C++14 standard says no constructors of an allocator shall exit via an
    // exception, so users must supply spaces by themselves for tracking info
    tracking_allocator(
        std::vector<std::pair<char*, char*>>& allocated,
        typename base_traits::size_type& total,
        const BaseAllocator& base = BaseAllocator()) noexcept :
        BaseAllocator(base), allocated_(&allocated), total_(&total)
    {}

    explicit tracking_allocator(
        std::vector<std::pair<char*, char*>>& allocated,
        const BaseAllocator& base = BaseAllocator()) noexcept :
        BaseAllocator(base), allocated_(&allocated), total_(nullptr)
    {}

    tracking_allocator(
        typename base_traits::size_type& total,
        const BaseAllocator& base = BaseAllocator()) noexcept :
        BaseAllocator(base), allocated_(nullptr), total_(&total)
    {}

    tracking_allocator(const tracking_allocator& other) noexcept = default;

    template <class OtherBaseAllocator>
    explicit tracking_allocator(
        const tracking_allocator<OtherBaseAllocator>& other) noexcept :
        BaseAllocator(other),
        allocated_(other.allocated_), total_(other.total_)
    {}

    // C++14 standard does not seem to mandate copy-assignability but
    // Visual Studo 2015's scoped_allocator_adaptor seems to need it
    template <class OtherBaseAllocator>
    tracking_allocator& operator=(
        const tracking_allocator<OtherBaseAllocator>& other)
        noexcept(std::is_nothrow_assignable<
            BaseAllocator&, const OtherBaseAllocator&>::value)
    {
        static_cast<BaseAllocator&>(*this) = other;
        allocated_ = other.allocated_;
        total_ = other.total_;
        return *this;
    }

    // To achieve symmetry
    template <class OtherBaseAllocator>
    tracking_allocator& operator=(
        tracking_allocator<OtherBaseAllocator>&& other)
        noexcept(std::is_nothrow_assignable<
            BaseAllocator&, OtherBaseAllocator>::value)
    {
        static_cast<BaseAllocator&>(*this) = std::move(other);
        allocated_ = other.allocated_;
        total_ = other.total_;
        return *this;
    }

    auto allocate(typename base_traits::size_type n)
    {
        if (allocated_) {
            allocated_->emplace_back();                         // throw
        }
        try {
            const auto p = base_traits::allocate(*this, n);     // throw
            char* const f = static_cast<char*>(true_addressof(p));
            const auto l = f + n * sizeof(typename base_traits::value_type);
            if (allocated_) {
                allocated_->back() = std::make_pair(f, l);
            }
            if (total_) {
                *total_ += l - f;
            }
            return p;
        } catch (...) {
            allocated_->pop_back();
            throw;
        }
    }

    void deallocate(
        typename base_traits::pointer p,
        typename base_traits::size_type n) noexcept
    {
        if (allocated_) {
            const auto i = std::find_if(
                allocated_->cbegin(), allocated_->cend(),
                [p](auto be) {
                    return be.first == tracking_allocator::true_addressof(p);
                        // "tracking_allocator::" is to satiate GCC 6.3.1
                });
            assert(i != allocated_->cend());
            allocated_->erase(i);
        }
        BaseAllocator::deallocate(p, n);
    }

    template <class OtherBaseAllocator>
    bool operator==(
        const tracking_allocator<OtherBaseAllocator>& other) const noexcept
    {
        return (static_cast<const BaseAllocator&>(*this) == other)
            && (allocated_ == other.allocated_)
            && (total_ == other.total_);
    }

    bool tracks(const void* p) const noexcept
    {
        if (allocated_) {
            for (const auto& be : *allocated_) {
                if ((be.first <= p) && (p < be.second)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool tracks_relax(const void* p) const noexcept
    {
#ifdef _MSC_VER
        // Visual Studio seems to return a pointer to the EBOed base type
        // that points JUST AFTER the object, so we should test a relaxed
        // manner
        if (allocated_) {
            for (const auto& be : *allocated_) {
                if ((be.first <= p) && (p <= be.second)) {
                    return true;
                }
            }
        }
        return false;
#else
        return tracks(p);
#endif
    }

    typename base_traits::size_type total() const noexcept
    {
        return total_ ? *total_ : static_cast<decltype(*total_ + 0)>(-1);
    }

private:
    static void* true_addressof(typename base_traits::pointer p) noexcept
    {
        return static_cast<void*>(std::addressof(*p));
    }
};

template <class LeftBaseAllocator, class RightBaseAllocator>
bool operator!=(
    const tracking_allocator<LeftBaseAllocator>& left,
    const tracking_allocator<RightBaseAllocator>& right) noexcept
{
    return !(left == right);
}

}}

#endif
