/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_0CB954D8_962A_4FA7_AE1E_25DF95DFFD36
#define COMMATA_GUARD_0CB954D8_962A_4FA7_AE1E_25DF95DFFD36

#include <functional>
#include <memory>
#include <type_traits>

#include "handler_decorator.hpp"
#include "typing_aid.hpp"

namespace commata {
namespace detail {

template <class D, class Ch>
class empty_physical_line_aware_handler_base
{
    D* d() noexcept
    {
        return static_cast<D*>(this);
    }

    template <class F>
    static bool essay(F f) noexcept(noexcept(f()))
    {
        if constexpr (std::is_void_v<decltype(f())>) {
            f();
            return true;
        } else {
            return f();
        }
    }

public:
    auto empty_physical_line(const Ch* where)
        noexcept(noexcept(std::declval<D&>().start_record(where))
              && noexcept(std::declval<D&>().end_record(where)))
    {
        if constexpr (std::is_void_v<decltype(d()->start_record(where))>
                   && std::is_void_v<decltype(d()->end_record(where))>) {
            d()->start_record(where);
            d()->end_record(where);
        } else {
            return essay([t = d(), where] { return t->start_record(where); })
                && essay([t = d(), where] { return t->end_record(where); });
        }
    }
};

template <class Handler>
class empty_physical_line_aware_handler :
    public detail::handler_decorator<
        Handler, empty_physical_line_aware_handler<Handler>>,
    public empty_physical_line_aware_handler_base<
            empty_physical_line_aware_handler<Handler>,
            typename Handler::char_type>
{
    Handler handler_;

public:
    template <class A,
        class = std::enable_if_t<std::is_constructible_v<Handler, A&&>>>
    explicit empty_physical_line_aware_handler(A&& handler)
        noexcept(std::is_nothrow_constructible_v<Handler, A&&>) :
        handler_(std::forward<A>(handler))
    {}

    // Defaulted move ctor is all right

    Handler& base() noexcept
    {
        return handler_;
    }

    const Handler& base() const noexcept
    {
        return handler_;
    }
};

template <class Handler>
class empty_physical_line_aware_handler<Handler&> :
    public detail::handler_decorator<
        Handler, empty_physical_line_aware_handler<Handler&>>,
    public empty_physical_line_aware_handler_base<
            empty_physical_line_aware_handler<Handler&>,
            typename Handler::char_type>
{
    Handler* handler_;

public:
    explicit empty_physical_line_aware_handler(Handler& handler) noexcept :
        handler_(std::addressof(handler))
    {}

    // Defaulted move ctor is all right

    Handler& base() noexcept
    {
        return *handler_;
    }

    const Handler& base() const noexcept
    {
        return *handler_;
    }
};

} // end namespace detail

template <class Handler>
auto make_empty_physical_line_aware(Handler&& handler)
    noexcept(std::is_nothrow_move_constructible_v<std::decay_t<Handler>>)
 -> std::enable_if_t<
        !detail::is_std_reference_wrapper<Handler>::value,
        detail::empty_physical_line_aware_handler<std::decay_t<Handler>>>
{
    return detail::empty_physical_line_aware_handler<
        std::decay_t<Handler>>(std::forward<Handler>(handler));
}

template <class Handler>
auto make_empty_physical_line_aware(
    const std::reference_wrapper<Handler>& handler) noexcept
 -> detail::empty_physical_line_aware_handler<Handler&>
{
    return detail::empty_physical_line_aware_handler<Handler&>(handler.get());
}

}

#endif
