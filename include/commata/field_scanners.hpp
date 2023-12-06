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
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "text_error.hpp"
#include "text_value_translation.hpp"

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
    std::nullopt_t operator()(T* = nullptr) const
    {
        throw field_not_found("This field did not appear in this record");
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

namespace detail::scanner {

namespace replace_if_skipped_impl {

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
    replace_mode& mode() noexcept
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
class nontrivial_store : public trivial_store<T>
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
        this->mode() = other.mode();
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
        swap(this->mode(), other.mode());
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

}}

template <class T>
class replace_if_skipped
{
    using replace_mode = detail::replace_mode;

    using generic_args_t = detail::generic_args_t;

    using store_t = detail::scanner::replace_if_skipped_impl::store_t<T>;
    store_t store_;

public:
    using value_type = T;

    explicit replace_if_skipped(const T& t)
        noexcept(std::is_nothrow_copy_constructible_v<T>) :
        store_(generic_args_t(), t)
    {}

    explicit replace_if_skipped(T&& t = T())
        noexcept(std::is_nothrow_move_constructible_v<T>) :
        store_(generic_args_t(), std::move(t))
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

    std::optional<T> operator()() const
    {
        switch (store_.mode()) {
        case replace_mode::replace:
            return std::optional<T>(store_.value());
        case replace_mode::fail:
            fail_if_skipped().operator()<T>();
            [[fallthrough]];
        default:
            return std::optional<T>();
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

#if _MSC_VER
#pragma warning(push)
#pragma warning(disable:4702)
#endif
    void field_skipped()
    {
        std::optional<T> r = invoke_typing_as<T>(get_skipping_handler());
        if (r) {
            put(std::move(*r));
        }
    }
#if _MSC_VER
#pragma warning(pop)
#endif

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
    void operator()(const Ch* begin, const Ch* end)
    {
        assert(*end == Ch());
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
    void operator()(Ch* begin, Ch* end)
    {
        assert(*end == Ch());   // shall be null-terminated
        const auto new_end = remove_(begin, end);
        *new_end = Ch();
        return out_(begin, new_end);
    }
};

template <class Sink, class Ch,
    class Tr = std::char_traits<Ch>, class Allocator = std::allocator<Ch>,
    class SkippingHandler = fail_if_skipped>
class string_field_translator
{
public:
    using value_type = std::basic_string<Ch, Tr, Allocator>;
    using allocator_type = Allocator;
    using skipping_handler_type = SkippingHandler;

private:
    using translator_t =
        detail::scanner::translator<value_type, Sink, SkippingHandler>;

    detail::base_member_pair<Allocator, translator_t> at_;

public:
    template <class SinkR, class SkippingHandlerR = SkippingHandler,
              std::enable_if_t<
                !std::is_base_of_v<
                    string_field_translator, std::decay_t<SinkR>>>* = nullptr>
    explicit string_field_translator(
        SinkR&& sink,
        SkippingHandlerR&& handle_skipping =
            std::decay_t<SkippingHandlerR>()) :
        at_(Allocator(),
            translator_t(std::forward<SinkR>(sink),
                         std::forward<SkippingHandlerR>(handle_skipping)))
    {}

    template <class SinkR, class SkippingHandlerR = SkippingHandler>
    string_field_translator(
        std::allocator_arg_t, const Allocator& alloc, SinkR&& sink,
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

    void operator()(const Ch* begin, const Ch* end)
    {
        at_.member().put(std::basic_string<Ch, Tr, Allocator>(
            begin, end, get_allocator()));
    }

    void operator()(std::basic_string<Ch, Tr, Allocator>&& value)
    {
        if constexpr (
                !std::allocator_traits<Allocator>::is_always_equal::value) {
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

template <class Sink, class Ch, class Tr = std::char_traits<Ch>,
    class SkippingHandler = fail_if_skipped>
class string_view_field_translator :
    detail::member_like_base<
        detail::scanner::translator<
            std::basic_string_view<Ch, Tr>, Sink, SkippingHandler>>
{
public:
    using value_type = std::basic_string_view<Ch, Tr>;
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

    void operator()(const Ch* begin, const Ch* end)
    {
        this->get().put(std::basic_string_view<Ch, Tr>(begin, end - begin));
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

template <class T, class Sink, class S, class C,
    std::enable_if_t<
        !detail::is_std_string_v<T>
     && !detail::is_std_string_view_v<T>
     && !std::is_base_of_v<std::locale, std::decay_t<S>>,
        std::nullptr_t> = nullptr>
arithmetic_field_translator<T, Sink,
        skipping_handler_t<T, S>, conversion_error_handler_t<T, C>>
    make_field_translator_na(Sink, S&&, C&&);

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

template <class T, class Sink, class S, class C,
    std::enable_if_t<
        !detail::is_std_string_v<T>
     && !detail::is_std_string_view_v<T>, std::nullptr_t> = nullptr>
locale_based_arithmetic_field_translator<T, Sink,
        skipping_handler_t<T, S>, conversion_error_handler_t<T, C>>
    make_field_translator_na(Sink, std::locale, S&&, C&&);

// string_field_translator
template <class T, class Sink,
    std::enable_if_t<detail::is_std_string_v<T>, std::nullptr_t> = nullptr>
string_field_translator<Sink, typename T::value_type, typename T::traits_type,
        typename T::allocator_type>
    make_field_translator_na(Sink);

template <class T, class Sink, class S,
    std::enable_if_t<detail::is_std_string_v<T>, std::nullptr_t> = nullptr>
string_field_translator<Sink, typename T::value_type, typename T::traits_type,
        typename T::allocator_type, skipping_handler_t<T, S>>
    make_field_translator_na(Sink, S&&);

// string_view_field_translator
template <class T, class Sink,
    std::enable_if_t<
        detail::is_std_string_view_v<T>, std::nullptr_t> = nullptr>
string_view_field_translator<Sink, typename T::value_type,
        typename T::traits_type>
    make_field_translator_na(Sink);

template <class T, class Sink, class S,
    std::enable_if_t<
        detail::is_std_string_view_v<T>, std::nullptr_t> = nullptr>
string_view_field_translator<Sink, typename T::value_type,
        typename T::traits_type, skipping_handler_t<T, S>>
    make_field_translator_na(Sink, S&&);

} // end detail::scanner

template <class T, class Sink, class... Appendices>
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

template <class T, class Allocator, class Sink, class... Appendices>
auto make_field_translator(std::allocator_arg_t, const Allocator& alloc,
    Sink&& sink, Appendices&&... appendices)
 -> std::enable_if_t<
        detail::is_std_string_v<T>
     && std::is_same_v<Allocator, typename T::allocator_type>
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

template <class Container, class = void>
struct back_insert_iterator;

template <class Container>
struct back_insert_iterator<Container,
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
struct back_insert_iterator<Container,
    std::enable_if_t<is_insertable_v<Container>
                  && !is_back_insertable_v<Container>>>
{
    using type = std::insert_iterator<Container>;

    static type from(Container& c)
    {
        return std::inserter(c, c.end());
    }
};

template <class Container>
using back_insert_iterator_t = typename back_insert_iterator<Container>::type;

} // end detail::scanner

template <class Container, class... Appendices>
auto make_field_translator(Container& values, Appendices&&... appendices)
 -> std::enable_if_t<
        (is_default_translatable_arithmetic_type_v<
            typename Container::value_type>
      || detail::is_std_string_v<typename Container::value_type>)
     && (detail::scanner::is_back_insertable_v<Container>
      || detail::scanner::is_insertable_v<Container>),
        decltype(
            detail::scanner::make_field_translator_na<
                    typename Container::value_type>(
                std::declval<detail::scanner::
                    back_insert_iterator_t<Container>>(),
                std::forward<Appendices>(appendices)...))>
{
    return make_field_translator<typename Container::value_type>(
        detail::scanner::back_insert_iterator<Container>::from(values),
        std::forward<Appendices>(appendices)...);
}

template <class Allocator, class Container, class... Appendices>
auto make_field_translator(std::allocator_arg_t, const Allocator& alloc,
    Container& values, Appendices&&... appendices)
 -> std::enable_if_t<
        detail::is_std_string_v<typename Container::value_type>
     && (detail::scanner::is_back_insertable_v<Container>
      || detail::scanner::is_insertable_v<Container>),
        string_field_translator<
            detail::scanner::back_insert_iterator_t<Container>,
            typename Container::value_type::value_type,
            typename Container::value_type::traits_type,
            Allocator, Appendices...>>
{
    return make_field_translator<typename Container::value_type>(
        std::allocator_arg, alloc,
        detail::scanner::back_insert_iterator<Container>::from(values),
        std::forward<Appendices>(appendices)...);
}

}

#endif
