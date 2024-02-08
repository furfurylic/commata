/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_7F9A9721_67A5_4D9A_A749_6C9316AAC80A
#define COMMATA_GUARD_7F9A9721_67A5_4D9A_A749_6C9316AAC80A

#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "../char_input.hpp"
#include "../wrapper_handlers.hpp"

#include "buffer_control.hpp"
#include "typing_aid.hpp"

namespace commata::detail {

template <class CharInput,
          template <class CharInputT, class HandlerT> class Parser>
class base_source
{
    CharInput in_;

public:
    using input_type = CharInput;
    using char_type = typename CharInput::char_type;
    using traits_type = typename CharInput::traits_type;

    explicit base_source(const CharInput& input) noexcept(
            std::is_nothrow_copy_constructible_v<CharInput>) :
        in_(input)
    {}

    explicit base_source(CharInput&& input) noexcept(
            std::is_nothrow_move_constructible_v<CharInput>) :
        in_(std::move(input))
    {}

    base_source(const base_source&) = default;
    base_source(base_source&&) = default;
    ~base_source() = default;
    base_source& operator=(const base_source&) = default;
    base_source& operator=(base_source&&) = default;

private:
    template <class Handler>
    class without_allocator
    {
        static_assert(!std::is_reference_v<Handler>);

    public:
        static constexpr bool enabled =
            !detail::is_std_reference_wrapper_v<Handler>
         && detail::is_with_buffer_control_v<Handler>;

        using full_fledged_handler_t =
            std::conditional_t<
                detail::is_full_fledged_v<Handler>,
                Handler,
                detail::full_fledged_handler<
                    Handler, detail::thru_buffer_control>>;

        using ret_t = Parser<CharInput, full_fledged_handler_t>;

        template <class HandlerR, class CharInputR>
        static auto invoke(HandlerR&& handler, CharInputR&& in)
        {
            static_assert(std::is_same_v<Handler, std::decay_t<HandlerR>>);
            static_assert(
                std::is_same_v<
                    std::remove_const_t<typename Handler::char_type>,
                    typename traits_type::char_type>,
                "std::decay_t<Handler>::char_type and traits_type::char_type "
                "are inconsistent; they shall be the same type expect that "
                "the former may be const-qualified");
            return ret_t(
                    std::forward<CharInputR>(in),
                    full_fledged_handler_t(std::forward<Handler>(handler)));
        }
    };

    template <class Handler, class Allocator>
    class with_allocator
    {
        static_assert(!std::is_reference_v<Handler>);

        using buffer_engine_t = detail::default_buffer_control<Allocator>;

        using full_fledged_handler_t =
            detail::full_fledged_handler<Handler, buffer_engine_t>;

    public:
        static constexpr bool enabled =
            !detail::is_std_reference_wrapper_v<Handler>
         && detail::is_without_buffer_control_v<Handler>;

        using ret_t = Parser<CharInput, full_fledged_handler_t>;

        template <class HandlerR, class CharInputR>
        static auto invoke(HandlerR&& handler,
            std::size_t buffer_size, const Allocator& alloc, CharInputR&& in)
        {
            static_assert(
                std::is_same_v<
                    std::remove_const_t<typename Handler::char_type>,
                    typename traits_type::char_type>,
                "std::decay_t<Handler>::char_type and traits_type::char_type "
                "are inconsistent; they shall be the same type expect that "
                "the former may be const-qualified");
            static_assert(
                std::is_same_v<
                    std::remove_const_t<typename Handler::char_type>,
                    typename std::allocator_traits<Allocator>::value_type>,
                "std::decay_t<Handler>::char_type and "
                "std::allocator_traits<Allocator>::value_type are "
                "inconsistent; they shall be the same type expect that "
                "the former may be const-qualified");
            return ret_t(
                    std::forward<CharInputR>(in),
                    full_fledged_handler_t(
                        std::forward<HandlerR>(handler),
                        buffer_engine_t(buffer_size, alloc)));
        }
    };

public:
    template <class Handler, class Allocator = void>
    using parser_type = std::conditional_t<
            without_allocator<Handler>::enabled,
            typename without_allocator<Handler>::ret_t,
            typename with_allocator<
                Handler,
                std::conditional_t<
                    std::is_same_v<Allocator, void>,
                    std::allocator<char_type>,
                    Allocator>>::ret_t>;

    template <class Handler>
    auto operator()(Handler&& handler) const&
        noexcept(
            std::is_nothrow_constructible_v<std::decay_t<Handler>, Handler&&>
         && std::is_nothrow_copy_constructible_v<CharInput>)
     -> std::enable_if_t<without_allocator<std::decay_t<Handler>>::enabled,
            parser_type<std::decay_t<Handler>>>
    {
        return without_allocator<std::decay_t<Handler>>::invoke(
            std::forward<Handler>(handler), in_);
    }

    template <class Handler>
    auto operator()(Handler&& handler) &&
        noexcept(
            std::is_nothrow_constructible_v<std::decay_t<Handler>, Handler&&>
         && std::is_nothrow_move_constructible_v<CharInput>)
     -> std::enable_if_t<without_allocator<std::decay_t<Handler>>::enabled,
            parser_type<std::decay_t<Handler>>>
    {
        return without_allocator<std::decay_t<Handler>>::invoke(
            std::forward<Handler>(handler), std::move(in_));
    }

    template <class Handler, class Allocator = std::allocator<char_type>>
    auto operator()(Handler&& handler, std::size_t buffer_size = 0,
            const Allocator& alloc = Allocator()) const&
        noexcept(
            std::is_nothrow_constructible_v<std::decay_t<Handler>, Handler&&>
         && std::is_nothrow_copy_constructible_v<CharInput>
         && std::is_nothrow_copy_constructible_v<Allocator>)
     -> std::enable_if_t<
            with_allocator<std::decay_t<Handler>, Allocator>::enabled,
            parser_type<std::decay_t<Handler>, Allocator>>
    {
        return with_allocator<std::decay_t<Handler>, Allocator>::invoke(
            std::forward<Handler>(handler), buffer_size, alloc, in_);
    }

    template <class Handler, class Allocator = std::allocator<char_type>>
    auto operator()(Handler&& handler,
        std::size_t buffer_size = 0, const Allocator& alloc = Allocator()) &&
        noexcept(
            std::is_nothrow_constructible_v<std::decay_t<Handler>, Handler&&>
         && std::is_nothrow_move_constructible_v<CharInput>
         && std::is_nothrow_copy_constructible_v<Allocator>)
     -> std::enable_if_t<
            with_allocator<std::decay_t<Handler>, Allocator>::enabled,
            parser_type<std::decay_t<Handler>, Allocator>>
    {
        return with_allocator<std::decay_t<Handler>, Allocator>::invoke(
            std::forward<Handler>(handler),
            buffer_size, alloc, std::move(in_));
    }

    template <class Handler, class... Args>
    auto operator()(std::reference_wrapper<Handler> handler,
            Args&&... args) const&
        noexcept(std::is_nothrow_invocable_v<const base_source&,
            reference_handler<Handler>, Args...>)
     -> decltype((*this)(reference_handler(
            handler.get()), std::forward<Args>(args)...))
    {
        return (*this)(wrap_ref(handler.get()), std::forward<Args>(args)...);
    }

    template <class Handler, class... Args>
    auto operator()(std::reference_wrapper<Handler> handler,
            Args&&... args) &&
        noexcept(std::is_nothrow_invocable_v<base_source,
            reference_handler<Handler>, Args...>)
     -> decltype(std::move(*this)(reference_handler(
            handler.get()), std::forward<Args>(args)...))
    {
        return std::move(*this)(wrap_ref(handler.get()),
                                std::forward<Args>(args)...);
    }

    void swap(base_source& other)
        noexcept(std::is_nothrow_swappable_v<CharInput>)
    {
        if (this != std::addressof(other)) {
            using std::swap;
            swap(in_, other.in_);
        }
    }
};

struct are_make_char_input_args_impl
{
    template <class... Args>
    static auto check(std::void_t<Args...>*) -> decltype(
        make_char_input(std::declval<Args>()...),
        std::true_type());

    template <class...>
    static auto check(...) -> std::false_type;
};

template <class... Args>
constexpr bool are_make_char_input_args_v =
    decltype(are_make_char_input_args_impl::check<Args...>(nullptr))();

}

#endif
