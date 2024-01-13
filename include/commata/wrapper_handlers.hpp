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

#include "detail/handler_decorator.hpp"
#include "detail/typing_aid.hpp"

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

// The ctor reference_handler(Handler&) is the best match for non-const lvalue
// of reference_handler<T> and defeats the copy deduction candidate, so this
// deduction guide is needed
template <class Handler>
reference_handler(reference_handler<Handler>) -> reference_handler<Handler>;

template <class Handler>
reference_handler<Handler> wrap_ref(Handler& handler) noexcept
{
    return reference_handler(handler);
}

template <class Handler>
reference_handler<Handler> wrap_ref(std::reference_wrapper<Handler> handler)
    noexcept
{
    return reference_handler(handler.get());
}

template <class Handler>
reference_handler<Handler> wrap_ref(reference_handler<Handler> handler)
    noexcept
{
    return reference_handler(handler);
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

    explicit empty_physical_line_aware_handler(const Handler& handler)
        noexcept(std::is_nothrow_copy_constructible_v<Handler>) :
        handler_(handler)
    {}

    explicit empty_physical_line_aware_handler(Handler&& handler)
        noexcept(std::is_nothrow_move_constructible_v<Handler>) :
        handler_(std::move(handler))
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

    auto empty_physical_line(char_type* where)
     -> std::conditional_t<
            std::is_void_v<decltype(
                std::declval<empty_physical_line_aware_handler&>().
                    start_record(where))>
         && std::is_void_v<decltype(
                std::declval<empty_physical_line_aware_handler&>().
                    end_record(where))>,
            void, bool>
    {
        if constexpr (std::is_void_v<decltype(this->start_record(where))>
                   && std::is_void_v<decltype(this->end_record(where))>) {
            this->start_record(where);
            this->end_record(where);
        } else {
            return detail::invoke_returning_bool(
                        [this, where] { return this->start_record(where); })
                && detail::invoke_returning_bool(
                        [this, where] { return this->end_record(where); });
        }
    }

    void swap(empty_physical_line_aware_handler& other)
        noexcept(std::is_nothrow_swappable_v<Handler>)
    {
        // Avoid self-moving in std::swap
        if (this != std::addressof(other)) {
            using std::swap;
            base().swap(other.base());
        }
    }
};

template <class Handler>
auto swap(
    empty_physical_line_aware_handler<Handler>& left,
    empty_physical_line_aware_handler<Handler>& right)
    noexcept(noexcept(left.swap(right)))
 -> std::enable_if_t<std::is_swappable_v<Handler>>
{
    left.swap(right);
}

template <class Handler>
auto make_empty_physical_line_aware(Handler&& handler)
    noexcept(
        std::is_nothrow_constructible_v<std::decay_t<Handler>, Handler&&>)
 -> std::enable_if_t<
        !detail::is_std_reference_wrapper_v<std::decay_t<Handler>>,
        std::conditional_t<
            detail::has_empty_physical_line_v<std::decay_t<Handler>>,
            Handler&&,
            empty_physical_line_aware_handler<std::decay_t<Handler>>>>
{
    if constexpr (detail::has_empty_physical_line_v<std::decay_t<Handler>>) {
        return std::forward<Handler>(handler);
    } else {
        return empty_physical_line_aware_handler(
                    std::forward<Handler>(handler));
    }
}

template <class Handler>
auto make_empty_physical_line_aware(
    std::reference_wrapper<Handler> handler) noexcept
 -> std::conditional_t<
        detail::has_empty_physical_line_v<Handler>,
        reference_handler<Handler>,
        empty_physical_line_aware_handler<reference_handler<Handler>>>
{
    return make_empty_physical_line_aware(wrap_ref(handler));
}

}

#endif
