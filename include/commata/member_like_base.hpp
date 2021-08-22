/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_815C71AA_E8EC_4B62_98C0_03D99230C7BB
#define COMMATA_GUARD_815C71AA_E8EC_4B62_98C0_03D99230C7BB

#include <type_traits>
#include <utility>

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
    member_like_base()
        noexcept(std::is_nothrow_default_constructible<F>::value)
    {}

    explicit member_like_base(const F& f)
        noexcept(std::is_nothrow_copy_constructible<F>::value) :
        f_(f)
    {}

    explicit member_like_base(F&& f)
        noexcept(std::is_nothrow_move_constructible<F>::value) :
        f_(std::move(f))
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
    member_like_base()
        noexcept(std::is_nothrow_default_constructible<F>::value)
    {}

    explicit member_like_base(const F& f)
        noexcept(std::is_nothrow_copy_constructible<F>::value) :
        F(f)
    {}

    explicit member_like_base(F&& f)
        noexcept(std::is_nothrow_move_constructible<F>::value) :
        F(std::move(f))
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

        template <class BR, class MR,
            class = std::enable_if_t<
                std::is_constructible<B, BR>::value
             && std::is_constructible<M, MR>::value>>
        pair(BR&& base, MR&& member)
            noexcept(std::is_nothrow_constructible<B, BR>::value
                  && std::is_nothrow_constructible<M, MR>::value) :
            member_like_base<B>(std::forward<BR>(base)),
            m(std::forward<MR>(member))
        {}
    };

    pair p_;

public:
    template <class BR, class MR,
        std::enable_if_t<
            std::is_constructible<B, BR>::value
         && std::is_constructible<M, MR>::value,
            std::nullptr_t> = nullptr>
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
