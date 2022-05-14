/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_815C71AA_E8EC_4B62_98C0_03D99230C7BB
#define COMMATA_GUARD_815C71AA_E8EC_4B62_98C0_03D99230C7BB

#include <type_traits>
#include <utility>

#include "typing_aid.hpp"

namespace commata::detail {

template <class F, class = void>
class member_like_base;

template <class F>
class member_like_base<F,
    std::enable_if_t<!std::is_reference_v<F> && std::is_final_v<F>>>
{
    F f_;

public:
    template <class... Args,
        std::enable_if_t<
            !std::is_base_of_v<
                member_like_base,
                first_t<std::decay_t<Args>...>>>* = nullptr>
    explicit member_like_base(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<F, Args&&...>) :
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
    std::enable_if_t<!std::is_reference_v<F> && !std::is_final_v<F>>> :
    F
{
public:
    template <class... Args,
        std::enable_if_t<
            !std::is_base_of_v<
                member_like_base,
                first_t<std::decay_t<Args>...>>>* = nullptr>
    explicit member_like_base(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<F, Args&&...>) :
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

        template <class C, class N>
        pair(C&& base, N&& member)
            noexcept(std::is_nothrow_constructible_v<B, C&&>
                  && std::is_nothrow_constructible_v<M, N&&>) :
            member_like_base<B>(std::forward<C>(base)),
            m(std::forward<N>(member))
        {}
    };

    pair p_;

public:
    template <class C, class N>
    base_member_pair(C&& first, N&& second)
        noexcept(std::is_nothrow_constructible_v<B&&, C&&>
              && std::is_nothrow_constructible_v<M&&, N&&>) :
        p_(std::forward<C>(first), std::forward<N>(second))
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

}

#endif
