/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_5A59E914_07DC_4541_95C2_8F99FBDBDE84
#define COMMATA_GUARD_5A59E914_07DC_4541_95C2_8F99FBDBDE84

#include <memory>
#include <type_traits>
#include <utility>

namespace commata { namespace detail {

// We made this class template to give nothrow-move-constructibility to
// std::vector, so we shall be able to remove this in C++17, in which it has
// a noexcept move ctor
template <class T, class = void>
class nothrow_move_constructible
{
    T* t_;

public:
    template <class AnyAllocator, class... Args>
    explicit nothrow_move_constructible(
        std::allocator_arg_t, const AnyAllocator& any_alloc, Args&&... args)
    {
        using at_t = typename std::allocator_traits<AnyAllocator>::
            template rebind_traits<T>;
        typename at_t::allocator_type alloc(any_alloc);
        const auto p = at_t::allocate(alloc, 1);
        t_ = std::addressof(*p);
        try {
            ::new(t_) T(std::forward<Args>(args)...);
        } catch (...) {
            at_t::deallocate(alloc, p, 1);
            throw;
        }
    }

    nothrow_move_constructible(nothrow_move_constructible&& other) noexcept :
        t_(other.t_)
    {
        other.t_ = nullptr;
    }

    ~nothrow_move_constructible()
    {
        assert(!t_);
    }

    explicit operator bool() const
    {
        return t_ != nullptr;
    }

    // Requires that the passed allocator is equal to the one passed on
    // construction of *this; throws nothing
    template <class AnyAllocator>
    void kill(const AnyAllocator& any_alloc)
    {
        assert(t_);
        using at_t = typename std::allocator_traits<AnyAllocator>::
            template rebind_traits<T>;
        typename at_t::allocator_type alloc(any_alloc);
        t_->~T();
        at_t::deallocate(alloc,
            std::pointer_traits<typename at_t::pointer>::pointer_to(*t_), 1);
#ifndef NDEBUG
        t_ = nullptr;
#endif
    }

    T& operator*()
    {
        assert(t_);
        return *t_;
    }

    const T& operator*() const
    {
        assert(t_);
        return *t_;
    }

    T* operator->()
    {
        assert(t_);
        return t_;
    }

    const T* operator->() const
    {
        assert(t_);
        return t_;
    }

    // Requires the passed allocator is equal to the one passed on construction
    // of *this and other; throws nothing
    template <class AnyAllocator>
    void assign(const AnyAllocator& any_alloc,
        nothrow_move_constructible&& other)
    {
        kill(any_alloc);
        t_ = other.t_;
        other.t_ = nullptr;
    }
};

template <class T>
class nothrow_move_constructible<T,
    std::enable_if_t<std::is_nothrow_move_constructible<T>::value>>
{
    T t_;

public:
    template <class AnyAllocator, class... Args>
    explicit nothrow_move_constructible(
        std::allocator_arg_t, const AnyAllocator&, Args&&... args) :
        t_(std::forward<Args>(args)...)
    {}

    nothrow_move_constructible(nothrow_move_constructible&& other) noexcept :
        t_(std::move(*other))
    {}

    ~nothrow_move_constructible()
    {}

    constexpr explicit operator bool() const
    {
        return true;
    }

    template <class AnyAllocator>
    void kill(const AnyAllocator&)
    {}

    T& operator*()
    {
        return t_;
    }

    const T& operator*() const
    {
        return t_;
    }

    T* operator->()
    {
        return std::addressof(t_);
    }

    const T* operator->() const
    {
        return std::addressof(t_);
    }

    // "assign" shall be called only when operator bool returns false,
    // which would not happen with this specialization
    template <class AnyAllocator>
    void assign(const AnyAllocator&, nothrow_move_constructible&&)
    {
        assert(false);
    }
};

}}

#endif
