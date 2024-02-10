/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_FD30DB2D_6977_4238_A98D_658A7563C717
#define COMMATA_GUARD_FD30DB2D_6977_4238_A98D_658A7563C717

#include <cstddef>
#include <memory>
#include <new>
#include <vector>

namespace commata::test {

template <class T>
class logging_allocator
{
    std::vector<std::size_t>* allocations_;

public:
    using value_type = T;
    using size_type = std::size_t;

    logging_allocator() noexcept : allocations_(nullptr)
    {}

    explicit logging_allocator(std::vector<std::size_t>& allocations)
        noexcept :
        allocations_(std::addressof(allocations))
    {}

    template <class U>
    logging_allocator(const logging_allocator<U>& other) noexcept :
        allocations_(other.allocations_)
    {}

    T* allocate(std::size_t n)
    {
        if (allocations_) {
            if (allocations_->size() == allocations_->max_size()) {
                throw std::bad_alloc();
            }
            allocations_->reserve(allocations_->size() + 1);
        }
        const std::size_t nn = sizeof(T) * n;
        const auto p = static_cast<T*>(static_cast<void*>(new char[nn]));
        if (allocations_) {
            allocations_->push_back(nn);
        }
        return p;
    }

    void deallocate(T* p, std::size_t)
    {
        delete [] static_cast<char*>(static_cast<void*>(p));
    }

    template <class U>
    bool operator==(const logging_allocator<U>&) const
    {
        return true;
    }

    template <class U>
    bool operator!=(const logging_allocator<U>&) const
    {
        return false;
    }
};

}

#endif
