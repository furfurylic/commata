/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_CA224F66_0D7D_4D06_8FB7_D49E3E24E061
#define COMMATA_GUARD_CA224F66_0D7D_4D06_8FB7_D49E3E24E061

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>

namespace commata::test {

template <class T>
class fancy_ptr
{
    T* p_;

    template <class U>
    friend class fancy_ptr;

public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = fancy_ptr;
    using const_pointer = fancy_ptr<const T>;

    fancy_ptr() : p_(nullptr) {}
    fancy_ptr(std::nullptr_t) : fancy_ptr() {}  // intentionally not explicit
    fancy_ptr(const fancy_ptr&) noexcept = default;
    fancy_ptr& operator=(const fancy_ptr&) noexcept = default;

#ifndef __GNUC__
private:
#endif
    fancy_ptr(T* p) : p_(p) {}

public:
    template <class U>
    fancy_ptr(const fancy_ptr<U>& other) noexcept : p_(other.p_) {}
    template <class U>
    fancy_ptr& operator=(const fancy_ptr<U>& other) noexcept
    { p_ = other.p_; return *this; }

    T& operator*() const noexcept { return *p_; }
    T* operator->() const noexcept { return p_; }   // shamefully not so fancy
    fancy_ptr& operator++() noexcept { ++p_; return *this; }
    fancy_ptr operator++(int) noexcept { auto q = *this; ++(*this); return q; }
    fancy_ptr& operator--() noexcept { --p_; return *this; }
    fancy_ptr operator--(int) noexcept { auto q = *this; --(*this); return q; }
    T& operator[](difference_type n) const noexcept { return p_[n]; }
    explicit operator bool() const noexcept { return p_ != nullptr; }
#ifdef __GNUC__
    operator T*() const noexcept { return p_; }
#endif

    static fancy_ptr pointer_to(T& t) noexcept
    { return fancy_ptr(std::addressof(t)); }

    fancy_ptr& operator+=(difference_type n) noexcept { p_ += n; return *this; }
    fancy_ptr& operator-=(difference_type n) noexcept { p_ -= n; return *this; }
};

template <class T>
fancy_ptr<T> operator+(const fancy_ptr<T>& p,
    typename fancy_ptr<T>::difference_type n) noexcept
{ auto q = p; return q += n; }

template <class T>
fancy_ptr<T> operator+(typename fancy_ptr<T>::difference_type n,
    const fancy_ptr<T>& p) noexcept
{ return p + n; }

template <class T>
fancy_ptr<T> operator-(const fancy_ptr<T>& p,
    typename fancy_ptr<T>::difference_type n) noexcept
{ auto q = p; return q -= n; }

template <class T, class U>
typename fancy_ptr<T>::difference_type operator-(
    const fancy_ptr<T>& l, const fancy_ptr<U>& r) noexcept
{ return std::addressof(*l) - std::addressof(*r); }

template <class T, class U>
bool operator==(const fancy_ptr<T>& l, const fancy_ptr<U>& r) noexcept
{ return std::addressof(*l) == std::addressof(*r); }

template <class T>
bool operator==(const fancy_ptr<T>& l, std::nullptr_t) noexcept
{ return std::addressof(*l) == nullptr; }

template <class T>
bool operator==(std::nullptr_t, const fancy_ptr<T>& r) noexcept
{ return r == nullptr; }

template <class T, class U>
bool operator!=(const fancy_ptr<T>& l, const fancy_ptr<U>& r) noexcept
{ return !(l == r); }

template <class T>
bool operator!=(const fancy_ptr<T>& l, std::nullptr_t r) noexcept
{ return !(l == r); }

template <class T>
bool operator!=(std::nullptr_t l, fancy_ptr<T>& r) noexcept
{ return !(l == r); }

template <class T, class U>
bool operator<(const fancy_ptr<T>& l, const fancy_ptr<U>& r) noexcept
{ return std::addressof(*l) < std::addressof(*r); }

template <class T>
bool operator<(const fancy_ptr<T>&, std::nullptr_t) noexcept
{ return false; }

template <class T>
bool operator<(std::nullptr_t, const fancy_ptr<T>& r) noexcept
{ return r != nullptr; }

template <class T, class U>
bool operator>(const fancy_ptr<T>& l, const fancy_ptr<U>& r) noexcept
{ return r < l; }

template <class T>
bool operator>(const fancy_ptr<T>& l, std::nullptr_t r) noexcept
{ return r < l; }

template <class T>
bool operator>(std::nullptr_t l, const fancy_ptr<T>& r) noexcept
{ return r < l; }

template <class T, class U>
bool operator<=(const fancy_ptr<T>& l, const fancy_ptr<U>& r) noexcept
{ return !(r < l); }

template <class T>
bool operator<=(const fancy_ptr<T>& l, std::nullptr_t r) noexcept
{ return !(r < l); }

template <class T>
bool operator<=(std::nullptr_t l, const fancy_ptr<T>& r) noexcept
{ return !(r < l); }

template <class T, class U>
bool operator>=(const fancy_ptr<T>& l, const fancy_ptr<U>& r) noexcept
{ return !(r > l); }

template <class T>
bool operator>=(const fancy_ptr<T>& l, std::nullptr_t r) noexcept
{ return !(r > l); }

template <class T>
bool operator>=(std::nullptr_t l, const fancy_ptr<T>& r) noexcept
{ return !(r > l); }

template <class T>
struct fancy_allocator
{
    using pointer = fancy_ptr<T>;
    using const_pointer = fancy_ptr<const T>;
    using reference = T&;
    using const_reference = const T&;
    using value_type = T;
    using difference_type =
        typename std::pointer_traits<pointer>::difference_type;
    using size_type = std::make_unsigned_t<difference_type>;

    fancy_allocator() noexcept = default;
    fancy_allocator(const fancy_allocator&) noexcept = default;
    fancy_allocator& operator=(const fancy_allocator&) noexcept = default;

    template <class U>
    fancy_allocator(const fancy_allocator<U>&) noexcept {}

    [[nodiscard]] pointer allocate(size_type n)
    {
        const auto p = ::operator new(n * sizeof(T));
        return fancy_ptr<T>::pointer_to(*static_cast<T*>(p));
    }

    void deallocate(pointer p, size_type)
    {
        ::operator delete(std::addressof(*p));
    }

    friend constexpr bool operator==(
        const fancy_allocator&, const fancy_allocator&)
    { return true; }

    friend constexpr bool operator!=(
        const fancy_allocator&, const fancy_allocator&)
    { return false; }

    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
};

}

#endif
