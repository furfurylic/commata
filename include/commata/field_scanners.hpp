/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_A9C3A92A_F9E8_4582_B398_F0138CD813FB
#define COMMATA_GUARD_A9C3A92A_F9E8_4582_B398_F0138CD813FB

#include <cassert>
#include <locale>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "text_error.hpp"
#include "text_value_translation.hpp"
#include "detail/full_ebo.hpp"

namespace commata {

class field_not_found : public text_error
{
public:
    using text_error::text_error;
};

struct fail_if_skipped
{
    explicit fail_if_skipped(replacement_fail_t = replacement_fail)
    {}

    template <class T>
    [[noreturn]]
    T operator()(T* = nullptr) const
    {
        using namespace std::string_view_literals;
        throw field_not_found("This field did not appear in this record"sv);
    }
};

struct ignore_if_skipped
{
    explicit ignore_if_skipped(replacement_ignore_t = replacement_ignore)
    {}

    std::nullopt_t operator()() const
    {
        return std::nullopt;
    }
};

namespace detail::scanner::replace_if_skipped_impl {

template <class T>
class trivial_store
{
    alignas(T) char value_[sizeof(T)] = {};
    replace_mode mode_ = static_cast<replace_mode>(0);

public:
    template <class... Args>
    trivial_store(generic_args_t, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args&&...>) :
        mode_(replace_mode::replace)
    {
        emplace(std::forward<Args>(args)...);
    }

    explicit trivial_store(replace_mode mode) noexcept :
        mode_(mode)
    {}

    replace_mode mode() const noexcept
    {
        return mode_;
    }

    const T& value() const
    {
        assert(mode_ == replace_mode::replace);
        return *static_cast<const T*>(static_cast<const void*>(value_));
    }

protected:
    replace_mode& mode_ref() noexcept
    {
        return mode_;
    }

    template <class... Args>
    void emplace(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
    {
        ::new(value_) T(std::forward<Args>(args)...);
    }

    T& value()
    {
        assert(mode_ == replace_mode::replace);
        return *static_cast<T*>(static_cast<void*>(value_));
    }
};

template <class T>
class nontrivial_store : trivial_store<T>
{
public:
    using trivial_store<T>::trivial_store;

    nontrivial_store(const nontrivial_store& other)
        noexcept(std::is_nothrow_copy_constructible_v<T>) :
        trivial_store<T>(other.mode())
    {
        if (this->mode() == replace_mode::replace) {
            this->emplace(other.value());                       // throw
        }
    }

    nontrivial_store(nontrivial_store&& other)
        noexcept(std::is_nothrow_move_constructible_v<T>) :
        trivial_store<T>(other.mode())
    {
        if (this->mode() == replace_mode::replace) {
            this->emplace(std::move(other.value()));            // throw
        }
    }

    ~nontrivial_store()
    {
        if (this->mode() == replace_mode::replace) {
            this->value().~T();
        }
    }

    nontrivial_store& operator=(const nontrivial_store& other)
        noexcept(std::is_nothrow_copy_constructible_v<T>
              && std::is_nothrow_copy_assignable_v<T>)
    {
        assign(other);
        return *this;
    }

    nontrivial_store& operator=(nontrivial_store&& other)
        noexcept(std::is_nothrow_move_constructible_v<T>
              && std::is_nothrow_move_assignable_v<T>)
    {
        assign(std::move(other));
        return *this;
    }

    using trivial_store<T>::mode;
    using trivial_store<T>::value;

private:
    template <class Other>
    void assign(Other&& other)
    {
        using f_t = std::conditional_t<
            std::is_lvalue_reference_v<Other>, const T&, T>;
        if (this->mode() == replace_mode::replace) {
            if (other.mode() == replace_mode::replace) {
                if (this != std::addressof(other)) {
                    // Avoid self move assignment, which is unsafe at least
                    // for standard library types
                    this->value() =
                        std::forward<f_t>(other.value());       // throw
                }
                return;
            } else {
                this->value().~T();
            }
        } else if (other.mode() == replace_mode::replace) {
            this->emplace(std::forward<f_t>(other.value()));    // throw
        }
        this->mode_ref() = other.mode();
    }

public:
    void swap(nontrivial_store& other)
        noexcept(std::is_nothrow_swappable_v<T>
              && std::is_nothrow_move_constructible_v<T>)
    {
        using std::swap;
        if (this->mode() == replace_mode::replace) {
            if (other.mode() == replace_mode::replace) {
                swap(this->value(), other.value());             // throw
                return;
            } else {
                other.emplace(std::move(this->value()));        // throw
                this->value().~T();
            }
        } else if (other.mode() == replace_mode::replace) {
            this->emplace(std::move(other.value()));            // throw
            other.value().~T();
        }
        swap(this->mode_ref(), other.mode_ref());
    }
};

template <class T>
void swap(nontrivial_store<T>& left, nontrivial_store<T>& right)
    noexcept(noexcept(left.swap(right)))
{
    left.swap(right);
}

template <class T>
using store_t = std::conditional_t<std::is_trivially_copyable_v<T>,
    trivial_store<T>, nontrivial_store<T>>;

}

template <class T>
class replace_if_skipped
{
    using replace_mode = detail::replace_mode;

    using generic_args_t = detail::generic_args_t;

    using store_t = detail::scanner::replace_if_skipped_impl::store_t<T>;
    store_t store_;

public:
    static_assert(
        !std::is_base_of_v<replacement_fail_t, std::remove_cv_t<T>>);
    static_assert(
        !std::is_base_of_v<replacement_ignore_t, std::remove_cv_t<T>>);

    using value_type = T;

    template <class U = T,
        std::enable_if_t<
            std::is_constructible_v<T, U>
         && !std::is_convertible_v<U&&, T>
         && !(std::is_base_of_v<replace_if_skipped, std::decay_t<U>>
           || std::is_base_of_v<replacement_fail_t, std::decay_t<U>>
           || std::is_base_of_v<replacement_ignore_t, std::decay_t<U>>)>*
                = nullptr>
    explicit replace_if_skipped(U&& u = std::decay_t<U>())
        noexcept(std::is_nothrow_constructible_v<T, U>) :
        store_(generic_args_t(), std::forward<U>(u))
    {}

    template <class U = T,
        std::enable_if_t<
            std::is_constructible_v<T, U>
         && std::is_convertible_v<U&&, T>
         && !(std::is_base_of_v<replace_if_skipped, std::decay_t<U>>
           || std::is_base_of_v<replacement_fail_t, std::decay_t<U>>
           || std::is_base_of_v<replacement_ignore_t, std::decay_t<U>>)>*
                = nullptr>
    replace_if_skipped(U&& u = std::decay_t<U>())
        noexcept(std::is_nothrow_constructible_v<T, U>) :
        store_(generic_args_t(), std::forward<U>(u))
    {}

    explicit replace_if_skipped(replacement_fail_t) noexcept :
        store_(detail::replace_mode::fail)
    {}

    explicit replace_if_skipped(replacement_ignore_t) noexcept :
        store_(detail::replace_mode::ignore)
    {}

    replace_if_skipped(const replace_if_skipped&) = default;
    replace_if_skipped(replace_if_skipped&&) = default;
    ~replace_if_skipped() = default;
    replace_if_skipped& operator=(const replace_if_skipped&) = default;
    replace_if_skipped& operator=(replace_if_skipped&&) = default;

    template <class U = T>
    auto operator()(U* = nullptr) const
     -> std::enable_if_t<std::is_convertible_v<const T&, U>, std::optional<U>>
    {
        switch (store_.mode()) {
        case replace_mode::replace:
            return store_.value();
        case replace_mode::fail:
            fail_if_skipped().operator()<U>();
            [[fallthrough]];
        default:
            return std::nullopt;
        }
    }

    void swap(replace_if_skipped& other)
        noexcept(std::is_nothrow_swappable_v<store_t>)
    {
        // See comments on replace_if_conversion_failed::swap
        // (But it seems that valgrind does not report 'memcpy for overlapping
        // buffers' even with Clang7 even if this self-check is absent)
        if (this != std::addressof(other)) {
            using std::swap;
            swap(store_, other.store_);
        }
    }
};

template <class T>
auto swap(replace_if_skipped<T>& left, replace_if_skipped<T>& right)
    noexcept(noexcept(left.swap(right)))
 -> std::enable_if_t<std::is_swappable_v<T>>
{
    left.swap(right);
}

template <class T,
    std::enable_if_t<
        !std::is_same_v<
            detail::replaced_type_not_found_t,
            detail::replaced_type_from_t<T>>,
        std::nullptr_t> = nullptr>
replace_if_skipped(T) -> replace_if_skipped<detail::replaced_type_from_t<T>>;

namespace detail::scanner {

template <class T, class = void>
struct is_output_iterator : std::false_type
{};

template <class T>
struct is_output_iterator<T,
    std::enable_if_t<
        std::is_base_of_v<
            std::output_iterator_tag,
            typename std::iterator_traits<T>::iterator_category>
     || std::is_base_of_v<
            std::forward_iterator_tag,
            typename std::iterator_traits<T>::iterator_category>>> :
    std::true_type
{};

template <class T>
constexpr bool is_output_iterator_v = is_output_iterator<T>::value;

template <class T, class H, class F>
void field_skipped_impl(H h, [[maybe_unused]] F f)
{
    auto r = invoke_typing_as<T>(h);
    using r_t = decltype(r);
    static_assert(std::is_convertible_v<r_t, std::optional<T>>);
    if constexpr (std::is_convertible_v<r_t, T>) {
        f(std::move(r));
    } else if constexpr (is_std_optional_v<r_t>) {
        if (r) {
            f(std::move(*r));
        }
    } else if constexpr (!std::is_same_v<std::nullopt_t, r_t>) {
        std::optional<T> r2(std::move(r));
        if (r2) {
            f(std::move(*r2));
        }
    }
}

template <class T, class Sink, class SkippingHandler>
class translator :
    member_like_base<SkippingHandler>
{
    Sink sink_;

public:
    template <class SinkR, class SkippingHandlerR>
    translator(SinkR&& sink, SkippingHandlerR&& handle_skipping) :
        member_like_base<SkippingHandler>(
            std::forward<SkippingHandlerR>(handle_skipping)),
        sink_(std::forward<SinkR>(sink))
    {}

    const SkippingHandler& get_skipping_handler() const noexcept
    {
        return this->get();
    }

    SkippingHandler& get_skipping_handler() noexcept
    {
        return this->get();
    }

    void field_skipped()
    {
        field_skipped_impl<T>(std::ref(get_skipping_handler()),
            [this](auto&& t) {
                this->put(std::forward<decltype(t)>(t));
            });
    }

public:
    template <class U>
    void put(U&& value)
    {
        if constexpr (is_output_iterator_v<Sink>) {
            *sink_ = std::forward<U>(value);
            ++sink_;
        } else {
            sink_(std::forward<U>(value));
        }
    }

    template <class U>
    void put(std::optional<U>&& value)
    {
        if (value.has_value()) {
            if constexpr (is_output_iterator_v<Sink>) {
                *sink_ = std::move(*value);
                ++sink_;
            } else {
                sink_(std::move(*value));
            }
        }
    }
};

} // end detail::scanner

template <class T, class Sink,
    class SkippingHandler = fail_if_skipped,
    class ConversionErrorHandler = fail_if_conversion_failed>
class arithmetic_field_translator
{
    using translator_t = detail::scanner::translator<T, Sink, SkippingHandler>;

    detail::base_member_pair<ConversionErrorHandler, translator_t> ct_;

public:
    using value_type = T;
    using skipping_handler_type = SkippingHandler;
    using conversion_error_handler_type = ConversionErrorHandler;

    template <class SinkR, class SkippingHandlerR = SkippingHandler,
              class ConversionErrorHandlerR = ConversionErrorHandler,
              std::enable_if_t<
                !std::is_base_of_v<
                    arithmetic_field_translator, std::decay_t<SinkR>>>*
                        = nullptr>
    explicit arithmetic_field_translator(
        SinkR&& sink,
        SkippingHandlerR&& handle_skipping =
            std::decay_t<SkippingHandlerR>(),
        ConversionErrorHandlerR&& handle_conversion_error =
            std::decay_t<ConversionErrorHandlerR>()) :
        ct_(std::forward<ConversionErrorHandlerR>(handle_conversion_error),
            translator_t(std::forward<SinkR>(sink),
                         std::forward<SkippingHandlerR>(handle_skipping)))
    {}

    arithmetic_field_translator(const arithmetic_field_translator&) = default;
    arithmetic_field_translator(arithmetic_field_translator&&) = default;
    ~arithmetic_field_translator() = default;

    const SkippingHandler& get_skipping_handler() const noexcept
    {
        return ct_.member().get_skipping_handler();
    }

    SkippingHandler& get_skipping_handler() noexcept
    {
        return ct_.member().get_skipping_handler();
    }

    void operator()()
    {
        ct_.member().field_skipped();
    }

    ConversionErrorHandler& get_conversion_error_handler() noexcept
    {
        return ct_.base();
    }

    const ConversionErrorHandler& get_conversion_error_handler() const noexcept
    {
        return ct_.base();
    }

    template <class Ch>
    auto operator()(Ch* begin, Ch* end)
     -> std::enable_if_t<std::is_same_v<Ch, char>
                      || std::is_same_v<Ch, wchar_t>>
    {
        *end = Ch();
        auto converted = to_arithmetic<std::optional<T>>(
            arithmetic_convertible<Ch>{ begin, end }, ct_.base());
        if (converted) {
            ct_.member().put(*converted);
        }
    }

private:
    template <class Ch>
    struct arithmetic_convertible
    {
        const Ch* begin;
        const Ch* end;

        const Ch* c_str() const noexcept
        {
            return begin;
        }

        std::size_t size() const noexcept
        {
            return std::size_t(end - begin);
        }
    };
};

template <class T, class Sink,
    class SkippingHandler = fail_if_skipped,
    class ConversionErrorHandler = fail_if_conversion_failed>
class locale_based_arithmetic_field_translator
{
    numpunct_replacer_to_c remove_;
    arithmetic_field_translator<
        T, Sink, SkippingHandler, ConversionErrorHandler> out_;

public:
    using value_type = T;
    using skipping_handler_type = SkippingHandler;
    using conversion_error_handler_type = ConversionErrorHandler;

    template <class SinkR, class SkippingHandlerR = SkippingHandler,
              class ConversionErrorHandlerR = ConversionErrorHandler,
              std::enable_if_t<
                !std::is_base_of_v<
                    locale_based_arithmetic_field_translator,
                    std::decay_t<SinkR>>>* = nullptr>
    locale_based_arithmetic_field_translator(
        SinkR&& sink, const std::locale& loc,
        SkippingHandlerR&& handle_skipping =
            std::decay_t<SkippingHandlerR>(),
        ConversionErrorHandlerR&& handle_conversion_error =
            std::decay_t<ConversionErrorHandlerR>()) :
        remove_(loc),
        out_(std::forward<SinkR>(sink),
             std::forward<SkippingHandlerR>(handle_skipping),
             std::forward<ConversionErrorHandlerR>(handle_conversion_error))
    {}

    locale_based_arithmetic_field_translator(
        const locale_based_arithmetic_field_translator&) = default;
    locale_based_arithmetic_field_translator(
        locale_based_arithmetic_field_translator&&) = default;
    ~locale_based_arithmetic_field_translator() = default;

    const SkippingHandler& get_skipping_handler() const noexcept
    {
        return out_.get_skipping_handler();
    }

    SkippingHandler& get_skipping_handler() noexcept
    {
        return out_.get_skipping_handler();
    }

    void operator()()
    {
        out_();
    }

    ConversionErrorHandler& get_conversion_error_handler() noexcept
    {
        return out_.get_conversion_error_handler();
    }

    const ConversionErrorHandler& get_conversion_error_handler() const noexcept
    {
        return out_.get_conversion_error_handler();
    }

    template <class Ch>
    auto operator()(Ch* begin, Ch* end)
     -> std::enable_if_t<std::is_same_v<Ch, char>
                      || std::is_same_v<Ch, wchar_t>>
    {
        return out_(begin, remove_(begin, end));
    }
};

template <class T, class Sink, class SkippingHandler = fail_if_skipped>
class string_field_translator
{
public:
    static_assert(detail::is_std_string_v<T>);

    using value_type = T;
    using allocator_type = typename T::allocator_type;
    using skipping_handler_type = SkippingHandler;

private:
    using translator_t =
        detail::scanner::translator<value_type, Sink, SkippingHandler>;

    detail::base_member_pair<allocator_type, translator_t> at_;

public:
    template <class SinkR, class SkippingHandlerR = SkippingHandler,
              std::enable_if_t<
                !std::is_base_of_v<
                    string_field_translator, std::decay_t<SinkR>>>* = nullptr>
    explicit string_field_translator(
        SinkR&& sink,
        SkippingHandlerR&& handle_skipping =
            std::decay_t<SkippingHandlerR>()) :
        at_(allocator_type(),
            translator_t(std::forward<SinkR>(sink),
                         std::forward<SkippingHandlerR>(handle_skipping)))
    {}

    template <class SinkR, class SkippingHandlerR = SkippingHandler>
    string_field_translator(std::allocator_arg_t,
            const typename T::allocator_type& alloc, SinkR&& sink,
            SkippingHandlerR&& handle_skipping = SkippingHandler()) :
        at_(alloc,
            translator_t(std::forward<SinkR>(sink),
                         std::forward<SkippingHandlerR>(handle_skipping)))
    {}

    string_field_translator(const string_field_translator&) = default;
    string_field_translator(string_field_translator&&) = default;
    ~string_field_translator() = default;

    allocator_type get_allocator() const noexcept
    {
        return at_.base();
    }

    const SkippingHandler& get_skipping_handler() const noexcept
    {
        return at_.member().get_skipping_handler();
    }

    SkippingHandler& get_skipping_handler() noexcept
    {
        return at_.member().get_skipping_handler();
    }

    void operator()()
    {
        at_.member().field_skipped();
    }

    void operator()(
        const typename T::value_type* begin, const typename T::value_type* end)
    {
        at_.member().put(T(begin, end, get_allocator()));
    }

    void operator()(T&& value)
    {
        if constexpr (!std::allocator_traits<allocator_type>::
                        is_always_equal::value) {
            if (value.get_allocator() != get_allocator()) {
                // We don't try to construct string with move(value) and
                // get_allocator() because some standard libs lack that ctor
                // and to do so doesn't seem necessarily cheap
                (*this)(value.data(), value.data() + value.size());
                return;
            }
        }
        at_.member().put(std::move(value));
    }
};

template <class T, class Sink, class SkippingHandler = fail_if_skipped>
class string_view_field_translator :
    detail::member_like_base<
        detail::scanner::translator<T, Sink, SkippingHandler>>
{
public:
    static_assert(detail::is_std_string_view_v<T>);

    using value_type = T;
    using skipping_handler_type = SkippingHandler;

private:
    using translator_t =
        detail::scanner::translator<value_type, Sink, SkippingHandler>;
    using base_t = detail::member_like_base<translator_t>;

public:
    template <class SinkR, class SkippingHandlerR = SkippingHandler,
              std::enable_if_t<
                !std::is_base_of_v<
                    string_view_field_translator, std::decay_t<SinkR>>,
                std::nullptr_t> = nullptr>
    explicit string_view_field_translator(
        SinkR&& sink,
        SkippingHandlerR&& handle_skipping =
            std::decay_t<SkippingHandlerR>()) :
        base_t(std::forward<SinkR>(sink),
               std::forward<SkippingHandlerR>(handle_skipping))
    {}

    string_view_field_translator(const string_view_field_translator&)
        = default;
    string_view_field_translator(string_view_field_translator&&) = default;
    ~string_view_field_translator() = default;

    const SkippingHandler& get_skipping_handler() const noexcept
    {
        return this->get().get_skipping_handler();
    }

    SkippingHandler& get_skipping_handler() noexcept
    {
        return this->get().get_skipping_handler();
    }

    void operator()()
    {
        this->get().field_skipped();
    }

    void operator()(
        const typename T::value_type* begin, const typename T::value_type* end)
    {
        this->get().put(T(begin, end - begin));
    }
};

template <class Container, class SkippingHandler = fail_if_skipped>
class COMMATA_FULL_EBO string_field_inserter :
    detail::member_like_base<SkippingHandler>,
    detail::member_like_base<typename Container::value_type::allocator_type>
{
    // On copying and moving, the skipping handler is more likely to throw an
    // exception than the allocator; thus the skipping handler should be copied
    // or moved first, so it is the first base class

public:
    using string_type = typename Container::value_type;
    using char_type = typename string_type::value_type;
    using traits_type = typename string_type::traits_type;
    using allocator_type = typename string_type::allocator_type;
    using value_type = std::basic_string_view<char_type, traits_type>;
    using skipping_handler_type = SkippingHandler;

private:
    static_assert(detail::is_std_string_v<string_type>);

    Container* c_;

public:
    template <class SkippingHandlerR = SkippingHandler>
    explicit string_field_inserter(
        Container& container,
        SkippingHandlerR&& handle_skipping =
            std::decay_t<SkippingHandlerR>()) :
        detail::member_like_base<SkippingHandler>(
            std::forward<SkippingHandlerR>(handle_skipping)),
        c_(std::addressof(container))
    {}

    template <class SkippingHandlerR = SkippingHandler>
    string_field_inserter(
        std::allocator_arg_t, const allocator_type& alloc,
        Container& container,
        SkippingHandlerR&& handle_skipping =
            std::decay_t<SkippingHandlerR>()) :
        detail::member_like_base<SkippingHandler>(
            std::forward<SkippingHandlerR>(handle_skipping)),
        detail::member_like_base<allocator_type>(alloc),
        c_(std::addressof(container))
    {}

    string_field_inserter(const string_field_inserter&) = default;
    string_field_inserter(string_field_inserter&&)= default;
    ~string_field_inserter() = default;

    allocator_type get_allocator() const noexcept
    {
        return this->detail::member_like_base<allocator_type>::get();
    }

    const SkippingHandler& get_skipping_handler() const noexcept
    {
        return this->detail::member_like_base<SkippingHandler>::get();
    }

    SkippingHandler& get_skipping_handler() noexcept
    {
        return this->detail::member_like_base<SkippingHandler>::get();
    }

    void operator()()
    {
        detail::scanner::field_skipped_impl<value_type>(
            std::ref(get_skipping_handler()),
            [this](auto&& v) {
                emplace(*c_, std::forward<decltype(v)>(v));
            });
    }

    void operator()(const char_type* begin, const char_type* end)
    {
        emplace(*c_, value_type(begin, end - begin));
    }

    void operator()(string_type&& value)
    {
        if constexpr (
            !std::allocator_traits<allocator_type>::is_always_equal::value) {
            if (value.get_allocator() != get_allocator()) {
                // We don't try to construct string with move(value) and
                // get_allocator() because some standard libs lack that ctor
                // and to do so doesn't seem necessarily cheap
                (*this)(value.data(), value.data() + value.size());
                return;
            }
        }
        emplace(*c_, std::move(value));
    }

private:
    template <class Container2,
        decltype(std::declval<Container2&>().push_back(
            std::declval<string_type>()), 1)* = nullptr>
    void emplace(Container2& c, string_type&& value)
    {
        assert(value.get_allocator() == get_allocator());
        c.push_back(std::move(value));
    }

    template <class Container2, class Allocator1,
        decltype(std::declval<Container2&>().insert(
            std::declval<string_type>()), 1U)* = nullptr>
    void emplace(Container2& c, string_type&& value)
    {
        assert(value.get_allocator() == get_allocator());
        c.insert(std::move(value));
    }

    template <class Container2,
        decltype(std::declval<Container2&>().emplace_back(
            std::declval<value_type>()), 1)* = nullptr>
    void emplace(Container2& c, value_type v)
    {
        c.emplace_back(v, get_allocator());
    }

    template <class Container2,
        decltype(std::declval<Container2&>().emplace(
            std::declval<value_type>()), 1U)* = nullptr>
    void emplace(Container2& c, value_type v)
    {
        c.emplace(v, get_allocator());
    }

    template <class Comp, class Allocator2>
    auto emplace(
        std::set<string_type, Comp, Allocator2>& c, value_type v)
     -> std::enable_if_t<
            std::is_invocable_r_v<bool, Comp, value_type, string_type>
         && std::is_invocable_r_v<bool, Comp, string_type, value_type>,
            std::void_t<typename Comp::is_transparent>>
    {
        if (const auto i = c.lower_bound(v);
                (i == c.end()) || c.key_comp()(v, *i)) {
            c.emplace_hint(i, v, get_allocator());
        }
    }
};

namespace detail::scanner {

// Make a cv-unqualified SkippingHandler type for the target type T and the
// type U specified in place of SkippingHandler
template <class T, class U>
using skipping_handler_t =
    std::conditional_t<
        std::is_base_of_v<replacement_fail_t, std::decay_t<U>>,
        fail_if_skipped,
        std::conditional_t<
            std::is_base_of_v<replacement_ignore_t, std::decay_t<U>>,
            ignore_if_skipped,
            std::conditional_t<
                std::is_constructible_v<replace_if_skipped<T>, U>
             && !std::is_base_of_v<replace_if_skipped<T>, std::decay_t<U>>,
                replace_if_skipped<T>,
                std::decay_t<U>>>>;

// Make a cv-unqualified ConversionErrorHandler type for the target type T and
// the type U specified in place of ConversionErrorHandler
template <class T, class U>
using conversion_error_handler_t =
    std::conditional_t<
        std::is_base_of_v<replacement_fail_t, std::decay_t<U>>,
        fail_if_conversion_failed,
        std::conditional_t<
            std::is_base_of_v<replacement_ignore_t, std::decay_t<U>>,
            ignore_if_conversion_failed,
            std::conditional_t<
                std::is_constructible_v<replace_if_conversion_failed<T>, U>
             && !std::is_base_of_v<replace_if_conversion_failed<T>,
                                   std::decay_t<U>>,
                replace_if_conversion_failed<T>,
                std::decay_t<U>>>>;

// arithmetic_field_translator
template <class T, class Sink,
    std::enable_if_t<
        !detail::is_std_string_v<T>
     && !detail::is_std_string_view_v<T>, std::nullptr_t> = nullptr>
arithmetic_field_translator<T, Sink>
    make_field_translator_na(Sink);

template <class T, class Sink, class S,
    std::enable_if_t<
        !detail::is_std_string_v<T>
     && !detail::is_std_string_view_v<T>
     && !std::is_base_of_v<std::locale, std::decay_t<S>>,
        std::nullptr_t> = nullptr>
arithmetic_field_translator<T, Sink, skipping_handler_t<T, S>>
    make_field_translator_na(Sink, S&&);

template <class T, class Sink, class S, class C, class... As,
    std::enable_if_t<
        !detail::is_std_string_v<T>
     && !detail::is_std_string_view_v<T>
     && !std::is_base_of_v<std::locale, std::decay_t<S>>,
        std::nullptr_t> = nullptr>
arithmetic_field_translator<T, Sink,
        skipping_handler_t<T, S>, conversion_error_handler_t<T, C>>
    make_field_translator_na(Sink, S&&, C&&, As&&...);

// locale_based_arithmetic_field_translator
template <class T, class Sink,
    std::enable_if_t<
        !detail::is_std_string_v<T>
     && !detail::is_std_string_view_v<T>, std::nullptr_t> = nullptr>
locale_based_arithmetic_field_translator<T, Sink>
    make_field_translator_na(Sink, std::locale);

template <class T, class Sink, class S,
    std::enable_if_t<
        !detail::is_std_string_v<T>
     && !detail::is_std_string_view_v<T>, std::nullptr_t> = nullptr>
locale_based_arithmetic_field_translator<T, Sink, skipping_handler_t<T, S>>
    make_field_translator_na(Sink, std::locale, S&&);

template <class T, class Sink, class S, class C, class... As,
    std::enable_if_t<
        !detail::is_std_string_v<T>
     && !detail::is_std_string_view_v<T>, std::nullptr_t> = nullptr>
locale_based_arithmetic_field_translator<T, Sink,
        skipping_handler_t<T, S>, conversion_error_handler_t<T, C>>
    make_field_translator_na(Sink, std::locale, S&&, C&&, As&&...);

// string_field_translator
template <class T, class Sink,
    std::enable_if_t<detail::is_std_string_v<T>, std::nullptr_t> = nullptr>
string_field_translator<T, Sink>
    make_field_translator_na(Sink);

template <class T, class Sink, class S, class... As,
    std::enable_if_t<detail::is_std_string_v<T>, std::nullptr_t> = nullptr>
string_field_translator<T, Sink, skipping_handler_t<T, S>>
    make_field_translator_na(Sink, S&&, As&&...);

// string_view_field_translator
template <class T, class Sink,
    std::enable_if_t<
        detail::is_std_string_view_v<T>, std::nullptr_t> = nullptr>
string_view_field_translator<T, Sink>
    make_field_translator_na(Sink);

template <class T, class Sink, class S, class... As,
    std::enable_if_t<
        detail::is_std_string_view_v<T>, std::nullptr_t> = nullptr>
string_view_field_translator<T, Sink, skipping_handler_t<T, S>>
    make_field_translator_na(Sink, S&&, As&&...);

} // end detail::scanner

template <class T, class Sink, class... Appendices>
[[nodiscard]]
auto make_field_translator(Sink&& sink, Appendices&&... appendices)
 -> std::enable_if_t<
        (is_default_translatable_arithmetic_type_v<T>
      || detail::is_std_string_v<T>
      || detail::is_std_string_view_v<T>)
     && (detail::scanner::is_output_iterator_v<std::decay_t<Sink>>
      || std::is_invocable_v<std::decay_t<Sink>&, T>),
        decltype(detail::scanner::make_field_translator_na<T>(
                    std::forward<Sink>(sink),
                    std::forward<Appendices>(appendices)...))>
{
    using t = decltype(detail::scanner::make_field_translator_na<T>(
        std::forward<Sink>(sink), std::forward<Appendices>(appendices)...));
    return t(std::forward<Sink>(sink),
             std::forward<Appendices>(appendices)...);
}

template <class T, class Sink, class... Appendices>
[[nodiscard]] auto make_field_translator(std::allocator_arg_t,
        const typename T::allocator_type& alloc, Sink&& sink,
        Appendices&&... appendices)
 -> std::enable_if_t<
        detail::is_std_string_v<T>
     && (detail::scanner::is_output_iterator_v<std::decay_t<Sink>>
      || std::is_invocable_v<std::decay_t<Sink>&, T>),
        decltype(detail::scanner::make_field_translator_na<T>(
                    std::forward<Sink>(sink),
                    std::forward<Appendices>(appendices)...))>
{
    using t = decltype(detail::scanner::make_field_translator_na<T>(
        std::forward<Sink>(sink), std::forward<Appendices>(appendices)...));
    return t(std::allocator_arg, alloc,
        std::forward<Sink>(sink), std::forward<Appendices>(appendices)...);
}

namespace detail::scanner {

struct is_back_insertable_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<T&>().push_back(std::declval<typename T::value_type>()),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

template <class T>
constexpr bool is_back_insertable_v =
    decltype(is_back_insertable_impl::check<T>(nullptr))();

struct is_insertable_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<T&>().insert(std::declval<T&>().end(),
            std::declval<typename T::value_type>()),
        std::true_type());

    template <class T>
    static auto check(...)->std::false_type;
};

template <class T>
struct is_insertable :
    decltype(is_insertable_impl::check<T>(nullptr))
{};

template <class T>
constexpr bool is_insertable_v = is_insertable<T>::value;

template <class T>
constexpr bool is_any_insertable_v =
    is_back_insertable_v<T> || is_insertable_v<T>;

template <class Container, class = void>
struct any_insert_iterator;

template <class Container>
struct any_insert_iterator<Container,
    std::enable_if_t<is_insertable_v<Container>
                  && is_back_insertable_v<Container>>>
{
    using type = std::back_insert_iterator<Container>;

    static type from(Container& c)
    {
        return std::back_inserter(c);
    }
};

template <class Container>
struct any_insert_iterator<Container,
    std::enable_if_t<is_insertable_v<Container>
                  && !is_back_insertable_v<Container>>>
{
    using type = std::insert_iterator<Container>;

    static type from(Container& c)
    {
        return std::inserter(c, c.end());
    }
};

template <class Container, class... Appendices>
struct arithmetic_populator
{
    using type = decltype(
        make_field_translator_na<typename Container::value_type>(
            std::declval<typename any_insert_iterator<Container>::type>(),
            std::declval<Appendices>()...));
};

template <class Container, class... Appendices>
struct string_populator
{
    using type = string_field_inserter<
        Container,
        skipping_handler_t<
            std::basic_string_view<
                typename Container::value_type::value_type,
                typename Container::value_type::traits_type>,
            Appendices>...>;
};

} // end detail::scanner

template <class Container, class... Appendices>
[[nodiscard]]
auto make_field_translator(Container& values, Appendices&&... appendices)
 -> typename std::enable_if_t<
        (is_default_translatable_arithmetic_type_v<
            typename Container::value_type>
      || detail::is_std_string_v<typename Container::value_type>)
     && detail::scanner::is_any_insertable_v<Container>,
        std::conditional_t<
            detail::is_std_string_v<typename Container::value_type>,
            detail::scanner::string_populator<Container, Appendices...>,
            detail::scanner::arithmetic_populator<Container, Appendices...>
        >>::type
{
    using value_t = typename Container::value_type;
    if constexpr (detail::is_std_string_v<value_t>) {
        using ret_t = typename detail::scanner::
            string_populator<Container, Appendices...>::type;
        return ret_t(values, std::forward<Appendices>(appendices)...);
    } else {
        return make_field_translator<value_t>(
            detail::scanner::any_insert_iterator<Container>::from(values),
            std::forward<Appendices>(appendices)...);
    }
}

template <class Container, class... Appendices>
[[nodiscard]] auto make_field_translator(std::allocator_arg_t,
    const typename Container::value_type::allocator_type& alloc,
    Container& values, Appendices&&... appendices)
 -> std::enable_if_t<
        detail::is_std_string_v<typename Container::value_type>
     && detail::scanner::is_any_insertable_v<Container>,
        typename detail::scanner::
            string_populator<Container, Appendices...>::type>
{
    using ret_t = typename detail::scanner::
        string_populator<Container, Appendices...>::type;
    return ret_t(std::allocator_arg, alloc,
        values, std::forward<Appendices>(appendices)...);
}

}

#endif
