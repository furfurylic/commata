/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_815C71AA_E8EC_4B62_98C0_03D99230C7BB
#define COMMATA_GUARD_815C71AA_E8EC_4B62_98C0_03D99230C7BB

#include <type_traits>
#include <utility>

#include "typing_aid.hpp"

namespace commata {
namespace detail {

template <class F, class = void>
class member_like_base;

template <class F>
class member_like_base<F,
    std::enable_if_t<!std::is_reference<F>::value
                  && std::is_final<F>::value>>
{
    F f_;

public:
    template <class... Args,
        std::enable_if_t<
            !std::is_base_of<member_like_base, first_t<Args...>>::value,
            std::nullptr_t> = nullptr>
    explicit member_like_base(Args&&... args)
        noexcept(std::is_nothrow_constructible<F, Args...>::value) :
        f_(std::forward<Args>(args)...)
    {}

    F& get() noexcept
    {
        return f_;
    }

    const F& get() const noexcept
    {
        return f_;
    }
};

template <class F>
class member_like_base<F,
    std::enable_if_t<!std::is_reference<F>::value
                  && !std::is_final<F>::value>> :
    F
{
public:
    template <class... Args,
        std::enable_if_t<
            !std::is_base_of<member_like_base, first_t<Args...>>::value,
            std::nullptr_t> = nullptr>
    explicit member_like_base(Args&&... args)
        noexcept(std::is_nothrow_constructible<F, Args...>::value) :
        F(std::forward<Args>(args)...)
    {}

    F& get() noexcept
    {
        return *this;
    }

    const F& get() const noexcept
    {
        return *this;
    }
};

template <class B, class M>
class base_member_pair
{
    struct pair : member_like_base<B>
    {
        M m;

        template <class BR, class MR>
        pair(BR&& base, MR&& member)
            noexcept(std::is_nothrow_constructible<B, BR>::value
                  && std::is_nothrow_constructible<M, MR>::value) :
            member_like_base<B>(std::forward<BR>(base)),
            m(std::forward<MR>(member))
        {}
    };

    pair p_;

public:
    template <class BR, class MR>
    base_member_pair(BR&& first, MR&& second)
        noexcept(std::is_nothrow_constructible<B, BR>::value
              && std::is_nothrow_constructible<M, MR>::value) :
        p_(std::forward<BR>(first), std::forward<MR>(second))
    {}

    B& base() noexcept
    {
        return p_.get();
    }

    const B& base() const noexcept
    {
        return p_.get();
    }

    M& member() noexcept
    {
        return p_.m;
    }

    const M& member() const noexcept
    {
        return p_.m;
    }
};

}}

#endif
