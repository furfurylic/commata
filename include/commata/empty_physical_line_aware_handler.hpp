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
namespace detail { namespace empty_physical_line_aware {

template <class D, class Ch>
class handler_base
{
    D* d() noexcept
    {
        return static_cast<D*>(this);
    }

    void empty_physical_line_impl(const Ch* where, std::true_type)
        noexcept(noexcept(std::declval<D&>().start_record(where))
              && noexcept(std::declval<D&>().end_record(where)))
    {
        d()->start_record(where);
        d()->end_record(where);
    }

    auto empty_physical_line_impl(const Ch* where, std::false_type)
        noexcept(noexcept(std::declval<D&>().start_record(where))
              && noexcept(std::declval<D&>().end_record(where)))
    {
        return essay([t = d(), where] { return t->start_record(where); })
            && essay([t = d(), where] { return t->end_record(where); });
    }

    template <class F>
    static auto essay(F f) noexcept(noexcept(f()))
     -> std::enable_if_t<std::is_void<decltype(f())>::value, bool>
    {
        f();
        return true;
    }

    template <class F>
    static auto essay(F f) noexcept(noexcept(f()))
     -> std::enable_if_t<!std::is_void<decltype(f())>::value, bool>
    {
        return f();
    }

public:
    auto empty_physical_line(const Ch* where)
        noexcept(noexcept(std::declval<D&>().start_record(where))
              && noexcept(std::declval<D&>().end_record(where)))
    {
        return empty_physical_line_impl(where,
            std::integral_constant<bool,
                std::is_void<decltype(d()->start_record(where))>::value
             && std::is_void<decltype(d()->end_record(where))>::value>());
    }
};

template <class Handler>
class handler :
    public handler_decorator<Handler, handler<Handler>>,
    public handler_base<handler<Handler>, typename Handler::char_type>
{
    Handler handler_;

public:
    template <class A,
        std::enable_if_t<std::is_constructible<Handler, A>::value>* = nullptr>
    explicit handler(A&& h)
        noexcept(std::is_nothrow_constructible<Handler, A>::value) :
        handler_(std::forward<A>(h))
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

    using handler_base<handler<Handler>, typename Handler::char_type>::
        empty_physical_line;
};

template <class Handler>
auto make(Handler&& h)
    noexcept(std::is_nothrow_move_constructible<std::decay_t<Handler>>::value)
 -> std::enable_if_t<
        !is_std_reference_wrapper<std::decay_t<Handler>>::value
     && !has_empty_physical_line<std::decay_t<Handler>>::value,
        handler<std::decay_t<Handler>>>
{
    return handler<std::decay_t<Handler>>(std::forward<Handler>(h));
}

template <class Handler>
auto make(Handler&& h) noexcept
 -> std::enable_if_t<
        has_empty_physical_line<std::decay_t<Handler>>::value,
        std::decay_t<Handler>>
{
    return std::forward<Handler>(h);
}

template <class Handler>
auto make(std::reference_wrapper<Handler> h) noexcept
 -> std::enable_if_t<
        !has_empty_physical_line<Handler>::value,
        handler<detail::wrapper_handler<Handler>>>
{
    return make(wrapper_handler<Handler>(h.get()));
}

template <class Handler>
auto make(std::reference_wrapper<Handler> h) noexcept
 -> std::enable_if_t<
        has_empty_physical_line<Handler>::value,
        wrapper_handler<Handler>>
{
    return wrapper_handler<Handler>(h.get());
}

}} // end detail::empty_physical_line_aware

template <class Handler>
auto make_empty_physical_line_aware(Handler&& handler)
    noexcept(
        std::is_nothrow_constructible<std::decay_t<Handler>, Handler>::value)
 -> std::enable_if_t<
        !detail::is_std_reference_wrapper<std::decay_t<Handler>>::value,
        std::conditional_t<
            detail::has_empty_physical_line<std::decay_t<Handler>>::value,
            std::decay_t<Handler>,
            detail::empty_physical_line_aware::handler<std::decay_t<Handler>>>>
{
    return detail::empty_physical_line_aware::
        make(std::forward<Handler>(handler));
}

template <class Handler>
auto make_empty_physical_line_aware(
    std::reference_wrapper<Handler> handler) noexcept
 -> std::conditional_t<
        detail::has_empty_physical_line<Handler>::value,
        detail::wrapper_handler<Handler>,
        detail::empty_physical_line_aware::handler<
            detail::wrapper_handler<Handler>>>
{
    return make_empty_physical_line_aware(
        detail::wrapper_handler<Handler>(handler));
}

}

#endif
