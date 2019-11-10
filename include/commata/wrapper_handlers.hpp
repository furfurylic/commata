/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_0CB954D8_962A_4FA7_AE1E_25DF95DFFD36
#define COMMATA_GUARD_0CB954D8_962A_4FA7_AE1E_25DF95DFFD36

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "handler_decorator.hpp"
#include "typing_aid.hpp"

namespace commata {

template <class Handler>
class reference_handler :
    public detail::handler_decorator<Handler, reference_handler<Handler>>
{
    Handler* handler_;

public:
    reference_handler(Handler& handler) noexcept :
        handler_(std::addressof(handler))
    {}

    reference_handler(Handler&& handler) = delete;

    reference_handler(const reference_handler&) = default;
    reference_handler& operator=(const reference_handler&) = default;

    Handler& base() const noexcept
    {
        return *handler_;
    }
};

template <class Handler>
reference_handler<Handler> wrap_ref(Handler& handler) noexcept
{
    return reference_handler<Handler>(handler);
}

template <class Handler>
reference_handler<Handler> wrap_ref(std::reference_wrapper<Handler> handler)
    noexcept
{
    return wrap_ref(handler.get());
}

template <class Handler>
reference_handler<Handler> wrap_ref(reference_handler<Handler> handler)
    noexcept
{
    return wrap_ref(handler.base());
}

template <class Handler>
class empty_physical_line_aware_handler :
    public detail::handler_decorator<
                Handler, empty_physical_line_aware_handler<Handler>>
{
    Handler handler_;

public:
    using char_type = typename Handler::char_type;
    using handler_type = Handler;

    template <class... Args,
        std::enable_if_t<
            (sizeof...(Args) != 1)
         || !std::is_base_of_v<
                empty_physical_line_aware_handler,
                detail::first_t<Args...>>>* = nullptr>
    explicit empty_physical_line_aware_handler(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<Handler, Args&&...>) :
        handler_(std::forward<Args>(args)...)
    {}

    empty_physical_line_aware_handler(
        const empty_physical_line_aware_handler&) = default;
    empty_physical_line_aware_handler(
        empty_physical_line_aware_handler&&) = default;
    ~empty_physical_line_aware_handler() = default;
    empty_physical_line_aware_handler& operator=(
        const empty_physical_line_aware_handler&) = default;
    empty_physical_line_aware_handler& operator=(
        empty_physical_line_aware_handler&&) = default;

    Handler& base() noexcept
    {
        return handler_;
    }

    const Handler& base() const noexcept
    {
        return handler_;
    }

    auto empty_physical_line(const char_type* where)
        noexcept(noexcept(std::declval<empty_physical_line_aware_handler&>().
                            start_record(where))
              && noexcept(std::declval<empty_physical_line_aware_handler&>().
                            end_record(where)))
    {
        return empty_physical_line_impl(where,
            std::bool_constant<
                std::is_void_v<decltype(this->start_record(where))>
             && std::is_void_v<decltype(this->end_record(where))>>());
    }

    void swap(empty_physical_line_aware_handler& other)
        noexcept(std::is_nothrow_swappable_v<Handler>)
    {
        using std::swap;
        base().swap(other.base());
    }

private:
    void empty_physical_line_impl(const char_type* where, std::true_type)
        noexcept(noexcept(std::declval<empty_physical_line_aware_handler&>().
                            start_record(where))
              && noexcept(std::declval<empty_physical_line_aware_handler&>().
                            end_record(where)))
    {
        this->start_record(where);
        this->end_record(where);
    }

    auto empty_physical_line_impl(const char_type* where, std::false_type)
        noexcept(noexcept(std::declval<empty_physical_line_aware_handler&>().
                            start_record(where))
              && noexcept(std::declval<empty_physical_line_aware_handler&>().
                            end_record(where)))
    {
        return essay([this, where] { return this->start_record(where); })
            && essay([this, where] { return this->end_record(where); });
    }

    template <class F>
    static auto essay(F f) noexcept(noexcept(f()))
     -> std::enable_if_t<std::is_void_v<decltype(f())>, bool>
    {
        f();
        return true;
    }

    template <class F>
    static auto essay(F f) noexcept(noexcept(f()))
     -> std::enable_if_t<!std::is_void_v<decltype(f())>, bool>
    {
        return f();
    }
};

template <class Handler>
void swap(
    empty_physical_line_aware_handler<Handler>& left,
    empty_physical_line_aware_handler<Handler>& right)
    noexcept(noexcept(left.swap(right)))
{
    return left.swap(right);
}

namespace detail::empty_physical_line_aware {

template <class Handler>
auto make(Handler&& h)
    noexcept(std::is_nothrow_move_constructible_v<std::decay_t<Handler>>)
 -> std::enable_if_t<
        !has_empty_physical_line<std::decay_t<Handler>>::value,
        empty_physical_line_aware_handler<std::decay_t<Handler>>>
{
    return empty_physical_line_aware_handler<std::decay_t<Handler>>(
            std::forward<Handler>(h));
}

template <class Handler>
auto make(Handler&& h) noexcept
 -> std::enable_if_t<
        has_empty_physical_line<std::decay_t<Handler>>::value,
        std::decay_t<Handler>>
{
    return std::forward<Handler>(h);
}

} // end detail::empty_physical_line_aware

template <class Handler>
auto make_empty_physical_line_aware(Handler&& handler)
    noexcept(
        std::is_nothrow_constructible_v<std::decay_t<Handler>, Handler>)
 -> std::enable_if_t<
        !detail::is_std_reference_wrapper<std::decay_t<Handler>>::value,
        std::conditional_t<
            detail::has_empty_physical_line<std::decay_t<Handler>>::value,
            std::decay_t<Handler>,
            empty_physical_line_aware_handler<std::decay_t<Handler>>>>
{
    return detail::empty_physical_line_aware::
        make(std::forward<Handler>(handler));
}

template <class Handler>
auto make_empty_physical_line_aware(
    std::reference_wrapper<Handler> handler) noexcept
 -> std::conditional_t<
        detail::has_empty_physical_line<Handler>::value,
        reference_handler<Handler>,
        empty_physical_line_aware_handler<reference_handler<Handler>>>
{
    return make_empty_physical_line_aware(wrap_ref(handler));
}

}

#endif
