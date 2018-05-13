/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_GUARD_815C71AA_E8EC_4B62_98C0_03D99230C7BB
#define FURFURYLIC_GUARD_815C71AA_E8EC_4B62_98C0_03D99230C7BB

#include <type_traits>
#include <utility>

namespace furfurylic {
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
    {}

    explicit member_like_base(const F& f) :
        f_(f)
    {}

    explicit member_like_base(F&& f) :
        f_(std::move(f))
    {}

    F& get()
    {
        return f_;
    }

    const F& get() const
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
    {}

    explicit member_like_base(const F& f) :
        F(f)
    {}

    explicit member_like_base(F&& f) :
        F(std::move(f))
    {}

    F& get()
    {
        return *this;
    }

    const F& get() const
    {
        return *this;
    }
};

}}}

#endif
