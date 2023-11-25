/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_0DE728A7_BABE_4C3A_94B1_A05CB3D0C9E4
#define COMMATA_GUARD_0DE728A7_BABE_4C3A_94B1_A05CB3D0C9E4

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cwctype>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>

#include "member_like_base.hpp"
#include "replacement.hpp"
#include "text_error.hpp"
#include "typing_aid.hpp"
#include "write_ntmbs.hpp"

namespace commata {

class text_value_translation_error : public text_error
{
public:
    using text_error::text_error;
};

class text_value_invalid_format : public text_value_translation_error
{
public:
    using text_value_translation_error::text_value_translation_error;
};

class text_value_empty : public text_value_invalid_format
{
public:
    using text_value_invalid_format::text_value_invalid_format;
};

class text_value_out_of_range : public text_value_translation_error
{
public:
    using text_value_translation_error::text_value_translation_error;
};

struct invalid_format_t {};
struct out_of_range_t {};
struct empty_t {};

namespace detail::xlate {

template <class T>
struct numeric_type_traits;

template <>
struct numeric_type_traits<char>
{
    static constexpr std::string_view name = "char";
    using raw_type =
        std::conditional_t<std::is_signed_v<char>, long, unsigned long>;
};

template <>
struct numeric_type_traits<signed char>
{
    static constexpr std::string_view name = "signed char";
    using raw_type = long;
};

template <>
struct numeric_type_traits<unsigned char>
{
    static constexpr std::string_view name = "unsigned char";
    using raw_type = unsigned long;
};

template <>
struct numeric_type_traits<short>
{
    static constexpr std::string_view name = "short int";
    using raw_type = long;
};

template <>
struct numeric_type_traits<unsigned short>
{
    static constexpr std::string_view name = "unsigned short int";
    using raw_type = unsigned long;
};

template <>
struct numeric_type_traits<int>
{
    static constexpr std::string_view name = "int";
    using raw_type = long;
};

template <>
struct numeric_type_traits<unsigned>
{
    static constexpr std::string_view name = "unsigned int";
    using raw_type = unsigned long;
};

template <>
struct numeric_type_traits<long>
{
    static constexpr std::string_view name = "long int";
    static constexpr const auto strto = std::strtol;
    static constexpr const auto wcsto = std::wcstol;
};

template <>
struct numeric_type_traits<unsigned long>
{
    static constexpr std::string_view name = "unsigned long int";
    static constexpr const auto strto = std::strtoul;
    static constexpr const auto wcsto = std::wcstoul;
};

template <>
struct numeric_type_traits<long long>
{
    static constexpr std::string_view name = "long long int";
    static constexpr const auto strto = std::strtoll;
    static constexpr const auto wcsto = std::wcstoll;
};

template <>
struct numeric_type_traits<unsigned long long>
{
    static constexpr std::string_view name = "unsigned long long int";
    static constexpr const auto strto = std::strtoull;
    static constexpr const auto wcsto = std::wcstoull;
};

template <>
struct numeric_type_traits<float>
{
    static constexpr std::string_view name = "float";
    static constexpr const auto strto = std::strtof;
    static constexpr const auto wcsto = std::wcstof;
};

template <>
struct numeric_type_traits<double>
{
    static constexpr std::string_view name = "double";
    static constexpr const auto strto = std::strtod;
    static constexpr const auto wcsto = std::wcstod;
};

template <>
struct numeric_type_traits<long double>
{
    static constexpr std::string_view name = "long double";
    static constexpr const auto strto = std::strtold;
    static constexpr const auto wcsto = std::wcstold;
};

template <class T>
class raw_converter
{
public:
    template <class Ch, class H>
    auto operator()(const Ch* begin, const Ch* end, H h) const
    {
        Ch* middle;
        errno = 0;
        const auto r = engine(begin, &middle);
        using ret_t = std::optional<std::remove_const_t<decltype(r)>>;

        const auto has_postfix =
            std::any_of<const Ch*>(middle, end, [](Ch c) {
                return !is_space(c);
            });
        if (has_postfix) {
            // if a not-whitespace-extra-character found, it is NG
            return ret_t(h.invalid_format(begin, end));
        } else if (begin == middle) {
            // whitespace only
            return ret_t(h.empty());
        } else if (errno == ERANGE) {
            return ret_t(h.out_of_range(begin, end, erange(r)));
        } else {
            return ret_t(r);
        }
    }

private:
    auto engine(const char* s, char** e) const
    {
        if constexpr (std::is_floating_point_v<T>) {
            return numeric_type_traits<T>::strto(s, e);
        } else {
            return numeric_type_traits<T>::strto(s, e, 10);
        }
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        if constexpr (std::is_floating_point_v<T>) {
            return numeric_type_traits<T>::wcsto(s, e);
        } else {
            return numeric_type_traits<T>::wcsto(s, e, 10);
        }
    }

    int erange([[maybe_unused]] T v) const
    {
        if constexpr (std::is_floating_point_v<T>) {
            return (v > T()) ? 1 : (v < T()) ? -1 : 0;
        } else if constexpr (std::is_signed_v<T>) {
            return (v > T()) ? 1 : -1;
        } else {
            return 1;
        }
    }

    // To mimic space skipping by std::strtol and its comrades,
    // we have to refer current C locale
    static bool is_space(char c)
    {
        return std::isspace(static_cast<unsigned char>(c)) != 0;
    }

    // ditto
    static bool is_space(wchar_t c)
    {
        return std::iswspace(c) != 0;
    }
};

template <class T, class U>
struct restrained_converter
{
    template <class Ch, class H>
    std::optional<T> operator()(const Ch* begin, const Ch* end, H h) const
    {
        const auto result = raw_converter<U>()(begin, end, h);
        if (!result.has_value()) {
            return std::nullopt;
        }
        const auto r = *result;
        if constexpr (std::is_unsigned_v<T>) {
            if constexpr (sizeof(T) < sizeof(U)) {
                constexpr auto t_max = std::numeric_limits<T>::max();
                if (r > t_max) {
                    const auto s = static_cast<std::make_signed_t<U>>(r);
                    if (s < 0) {
                        // -t_max is the lowest number that can be wrapped
                        // around and then returned
                        const auto s_wrapped_around = s + t_max + 1;
                        if (s_wrapped_around > 0) {
                            return static_cast<T>(s_wrapped_around);
                        }
                    }
                    return h.out_of_range(begin, end, 1);
                }
            }
        } else {
            if (r < std::numeric_limits<T>::lowest()) {
                return h.out_of_range(begin, end, -1);
            } else if (std::numeric_limits<T>::max() < r) {
                return h.out_of_range(begin, end, 1);
            }
        }
        return static_cast<T>(r);
    }
};

// For types without corresponding "raw_type"
template <class T, class = void>
struct converter :
    raw_converter<T>
{};

// For types which have corresponding "raw_type"
template <class T>
struct converter<T, std::void_t<typename numeric_type_traits<T>::raw_type>> :
    restrained_converter<T, typename numeric_type_traits<T>::raw_type>
{};

} // end detail::xlate

struct fail_if_conversion_failed
{
    template <class T, class Ch>
    [[noreturn]]
    std::optional<T> operator()(invalid_format_t,
        const Ch* begin, const Ch* end, T* = nullptr) const
    try {
        using namespace std::string_view_literals;
        assert(*end == Ch());
        std::stringbuf s;
        if constexpr (
                std::is_same_v<Ch, char> || std::is_same_v<Ch, wchar_t>) {
            detail::write_ntmbs(&s, std::locale(), begin, end);
            sputn(s, ": cannot convert"sv);
        } else {
            sputn(s, "Cannot convert"sv);
        }
        write_name<T>(s, " to an instance of "sv);
        throw text_value_invalid_format(std::move(s).str());
    } catch (const text_value_invalid_format&) {
        throw;
    } catch (...) {
        std::throw_with_nested(text_value_invalid_format());
    }

    template <class T, class Ch>
    [[noreturn]]
    std::optional<T> operator()(out_of_range_t,
        const Ch* begin, const Ch* end, int, T* = nullptr) const
    try {
        using namespace std::string_view_literals;
        assert(*end == Ch());
        std::stringbuf s;
        if constexpr (
                std::is_same_v<Ch, char> || std::is_same_v<Ch, wchar_t>) {
            detail::write_ntmbs(&s, std::locale(), begin, end);
            sputn(s, ": out of range"sv);
        } else {
            sputn(s, "Out of range"sv);
        }
        write_name<T>(s, " of "sv);
        throw text_value_out_of_range(std::move(s).str());
    } catch (const text_value_out_of_range&) {
        throw;
    } catch (...) {
        std::throw_with_nested(text_value_invalid_format());
    }

    template <class T>
    [[noreturn]]
    std::optional<T> operator()(empty_t, T* = nullptr) const
    try {
        using namespace std::string_view_literals;
        std::stringbuf s;
        sputn(s, "Cannot convert an empty string"sv);
        write_name<T>(s, " to an instance of "sv);
        throw text_value_empty(std::move(s).str());
    } catch (const text_value_empty&) {
        throw;
    } catch (...) {
        std::throw_with_nested(text_value_empty());
    }

private:
    template <class T>
    static void write_name(std::streambuf& sb, std::string_view prefix,
        decltype(detail::xlate::numeric_type_traits<T>::name)* = nullptr)
    {
        using namespace detail::xlate;
        sputn(sb, prefix);
        sputn(sb, detail::xlate::numeric_type_traits<T>::name);
    }

    template <class T, class... Args>
    static void write_name(std::streambuf&, Args&&...)
    {}

    template <class Ch, class Tr, class Tr2>
    static void sputn(std::basic_streambuf<Ch, Tr>& sb,
                      std::basic_string_view<Ch, Tr2> s)
    {
        using max_t = std::common_type_t<
            std::make_unsigned_t<std::streamsize>,
            typename std::basic_string_view<Ch, Tr2>::size_type>;
        constexpr max_t max =
            static_cast<max_t>(std::numeric_limits<std::streamsize>::max());

        while (s.size() > max) {
            sb.sputn(s.data(), max);
            s.remove_prefix(max);
        }
        sb.sputn(s.data(), s.size());
    }
};

struct ignore_if_conversion_failed
{
    template <class T, class Ch>
    std::optional<T> operator()(invalid_format_t,
        const Ch*, const Ch*, T* = nullptr) const
    {
        return std::nullopt;
    }

    template <class T, class Ch>
    std::optional<T> operator()(out_of_range_t,
        const Ch*, const Ch*, int, T* = nullptr) const
    {
        return std::nullopt;
    }

    template <class T>
    std::optional<T> operator()(empty_t, T* = nullptr) const
    {
        return std::nullopt;
    }
};

namespace detail::xlate::replace_if_conversion_failed_impl {

enum slot : unsigned {
    slot_empty = 0,
    slot_invalid_format = 1,
    slot_above_upper_limit = 2,
    slot_below_lower_limit = 3,
    slot_underflow = 4
};

struct copy_mode_t {};

template <class T, unsigned N>
struct trivial_store
{
private:
    alignas(T) char replacements_[N][sizeof(T)] = {};
    std::uint_fast8_t has_ = 0U;
    std::uint_fast8_t skips_ =0U;

protected:
    trivial_store(copy_mode_t, const trivial_store& other) noexcept :
        has_(other.has_), skips_(other.skips_)
    {}

public:
    using value_type = T;
    static constexpr unsigned size = N;

    template <class Head, class... Tails>
    trivial_store(generic_args_t, Head&& head, Tails&&... tails) :
        trivial_store(std::integral_constant<std::size_t, 0>(),
            std::forward<Head>(head), std::forward<Tails>(tails)...)
    {}

private:
    template <std::size_t Slot, class Head, class... Tails>
    trivial_store(std::integral_constant<std::size_t, Slot>,
                  Head&& head, Tails&&... tails) :
        trivial_store(std::integral_constant<std::size_t, Slot + 1>(),
            std::forward<Tails>(tails)...)
    {
        init<Slot>(std::forward<Head>(head));
    }

    trivial_store(std::integral_constant<std::size_t, N>) noexcept :
        has_(0), skips_(0)
    {}

    template <unsigned Slot, class U, std::enable_if_t<
        (!std::is_base_of_v<replacement_fail_t, std::decay_t<U>>)
     && (!std::is_base_of_v<replacement_ignore_t, std::decay_t<U>>)>*
        = nullptr>
    void init(U&& value)
        noexcept(std::is_nothrow_constructible_v<T, U&&>)
    {
        static_assert(Slot < N);
        assert(!has(Slot));
        assert(!skips(Slot));
        emplace(Slot, std::forward<U>(value));
        set_has(Slot);
    }

    template <unsigned Slot>
    void init() noexcept(std::is_nothrow_default_constructible_v<T>)
    {
        static_assert(Slot < N);
        assert(!has(Slot));
        assert(!skips(Slot));
        emplace(Slot);
        set_has(Slot);
    }

    template <unsigned Slot>
    void init(replacement_fail_t) noexcept
    {
        static_assert(Slot < N);
        assert(!has(Slot));
        assert(!skips(Slot));
    }

    template <unsigned Slot>
    void init(replacement_ignore_t) noexcept
    {
        static_assert(Slot < N);
        assert(!has(Slot));
        assert(!skips(Slot));
        set_skips(Slot);
    }

public:
    std::pair<replace_mode, const T*> get(unsigned r) const noexcept
    {
        return get_g(this, r);
    }

private:
    template <class ThisType>
    static auto get_g(ThisType* me, unsigned r) noexcept
    {
        using t_t = std::conditional_t<std::is_const_v<ThisType>, const T, T>;
        using pair_t = std::pair<replace_mode, t_t*>;
        if (me->has(r)) {
            return pair_t(replace_mode::replace, std::addressof((*me)[r]));
        } else if (me->skips(r)) {
            return pair_t(replace_mode::ignore, nullptr);
        } else {
            return pair_t(replace_mode::fail, nullptr);
        }
    }

protected:
    T& operator[](unsigned r)
    {
        return *static_cast<T*>(static_cast<void*>(replacements_[r]));
    }

    const T& operator[](unsigned r) const
    {
        return *static_cast<const T*>(
            static_cast<const void*>(replacements_[r]));
    }

    template <class... Args>
    void emplace(unsigned r, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
    {
        ::new(replacements_[r]) T(std::forward<Args>(args)...);
    }

    bool has(unsigned r) const noexcept
    {
        return (has_ & (1 << r)) != 0;
    }

    void set_has(unsigned r) noexcept
    {
        has_ |= (1 << r);
    }

    void set_has(unsigned r, bool v) noexcept
    {
        if (v) {
            set_has(r);
        } else {
            has_ &= ~(1 << r);
        }
    }

    bool skips(unsigned r) const noexcept
    {
        return (skips_ & (1 << r)) != 0;
    }

    void set_skips(unsigned r) noexcept
    {
        skips_ |= (1 << r);
    }

    void set_skips(unsigned r, bool v) noexcept
    {
        if (v) {
            set_skips(r);
        } else {
            skips_ &= ~(1 << r);
        }
    }
};

template <class T, unsigned N>
struct nontrivial_store : trivial_store<T, N>
{
    using trivial_store<T, N>::trivial_store;

    nontrivial_store(const nontrivial_store& other)
        noexcept(std::is_nothrow_copy_constructible_v<T>) :
        trivial_store<T, N>(copy_mode_t(), other)
    {
        for (unsigned r = 0; r < N; ++r) {
            if (this->has(r)) {
                this->emplace(r, other[r]);
            }
        }
    }

    nontrivial_store(nontrivial_store&& other)
        noexcept(std::is_nothrow_move_constructible_v<T>) :
        trivial_store<T, N>(copy_mode_t(), other)
    {
        for (unsigned r = 0; r < N; ++r) {
            if (this->has(r)) {
                this->emplace(r, std::move(other[r]));
            }
        }
    }

    ~nontrivial_store()
    {
        for (unsigned r = 0; r < N; ++r) {
            if (this->has(r)) {
                (*this)[r].~T();
            }
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
        if (this == std::addressof(other)) {
            return; // see comments in replace_if_skipped
        }
        using f_t = std::conditional_t<
            std::is_lvalue_reference_v<Other>, const T&, T>;
        for (unsigned r = 0; r < N; ++r) {
            if (this->has(r)) {
                if (other.has(r)) {
                    (*this)[r] = std::forward<f_t>(other[r]);
                    continue;
                } else {
                    (*this)[r].~T();
                }
            } else if (other.has(r)) {
                this->emplace(r, std::forward<f_t>(other[r]));
            }
            this->set_has(r, other.has(r));
            this->set_skips(r, other.skips(r));
        }
    }

public:
    void swap(nontrivial_store& other)
        noexcept(std::is_nothrow_swappable_v<T>
              && std::is_nothrow_constructible_v<T>)
    {
        for (unsigned r = 0; r < N; ++r) {
            if (this->has(r)) {
                if (other.has(r)) {
                    using std::swap;
                    swap((*this)[r], other[r]);
                    continue;
                } else {
                    other.emplace(r, std::move((*this)[r]));
                    (*this)[r].~T();
                }
            } else if (other.has(r)) {
                this->emplace(r, std::move(other[r]));
                other[r].~T();
            }

            {
                const bool t = this->has(r);
                this->set_has(r, other.has(r));
                other.set_has(r, t);
            }
            {
                const bool t = this->skips(r);
                this->set_skips(r, other.skips(r));
                other.set_skips(r, t);
            }
        }
    }
};

template <class T, unsigned N>
void swap(nontrivial_store<T, N>& left, nontrivial_store<T, N>& right)
    noexcept(noexcept(left.swap(right)))
{
    left.swap(right);
}

template <class T, class A>
constexpr bool is_acceptable_arg_v =
    std::is_base_of_v<replacement_fail_t, std::decay_t<A>>
 || std::is_base_of_v<replacement_ignore_t, std::decay_t<A>>
 || std::is_constructible_v<T, A>;

template <class T, class A>
constexpr bool is_nothrow_arg_v =
    std::is_base_of_v<replacement_fail_t, std::decay_t<A>>
 || std::is_base_of_v<replacement_ignore_t, std::decay_t<A>>
 || std::is_nothrow_constructible_v<T, A>;

template <class T>
constexpr unsigned base_n =
    std::is_integral_v<T> ? std::is_signed_v<T> ? 4 : 3 : 5;

template <class T, unsigned N>
struct base;

template <class T>
struct base<T, 0>
{
protected:
    using store_t = std::conditional_t<std::is_trivially_copyable_v<T>,
        trivial_store<T, base_n<T>>, nontrivial_store<T, base_n<T>>>;

private:
    store_t store_;

protected:
    template <class... As>
    base(generic_args_t, As&&... as) :
        store_(generic_args_t(), std::forward<As>(as)...)
    {}

    const store_t& store() const
    {
        return store_;
    }

    store_t& store()
    {
        return store_;
    }
};

template <class T>
struct base<T, 3> : base<T, 0>
{
    template <class All = T,
        std::enable_if_t<is_acceptable_arg_v<T, const All&>>* = nullptr>
    explicit base(const All& for_all = All()) :
        base(for_all, for_all, for_all)
    {}

    template <class Empty, class AllButEmpty,
        std::enable_if_t<
            is_acceptable_arg_v<T, Empty&&>
         && is_acceptable_arg_v<T, const AllButEmpty&>>* = nullptr>
    base(Empty&& on_empty, const AllButEmpty& for_all_but_empty) :
        base(std::forward<Empty>(on_empty),
             for_all_but_empty, for_all_but_empty)
    {}

    template <class Empty, class InvalidFormat, class AboveUpperLimit,
        std::enable_if_t<
            is_acceptable_arg_v<T, Empty&&>
         && is_acceptable_arg_v<T, InvalidFormat&&>
         && is_acceptable_arg_v<T, AboveUpperLimit&&>>* = nullptr>
    explicit base(
        Empty&& on_empty,
        InvalidFormat&& on_invalid_format,
        AboveUpperLimit&& on_above_upper_limit)
            noexcept(std::is_nothrow_default_constructible_v<T>
                  && is_nothrow_arg_v<T, Empty>
                  && is_nothrow_arg_v<T, InvalidFormat>
                  && is_nothrow_arg_v<T, AboveUpperLimit>) :
        base<T, 0>(
            generic_args_t(),
            std::forward<Empty>(on_empty),
            std::forward<InvalidFormat>(on_invalid_format),
            std::forward<AboveUpperLimit>(on_above_upper_limit))
    {}
};

template <class T>
struct base<T, 4> : base<T, 0>
{
    template <class All = T,
        std::enable_if_t<is_acceptable_arg_v<T, const All&>>* = nullptr>
    explicit base(const All& for_all = All()) :
        base(for_all, for_all, for_all, for_all)
    {}

    template <class Empty, class AllButEmpty,
        std::enable_if_t<
            is_acceptable_arg_v<T, Empty&&>
         && is_acceptable_arg_v<T, const AllButEmpty&>>* = nullptr>
    explicit base(Empty&& on_empty, const AllButEmpty& for_all_but_empty) :
        base(std::forward<Empty>(on_empty),
             for_all_but_empty, for_all_but_empty, for_all_but_empty)
    {}

    template <class Empty, class InvalidFormat, class AllOutOfRange,
        std::enable_if_t<
            is_acceptable_arg_v<T, Empty&&>
         && is_acceptable_arg_v<T, InvalidFormat&&>
         && is_acceptable_arg_v<T, const AllOutOfRange&>>* = nullptr>
    base(Empty&& on_empty, InvalidFormat&& on_invalid_format,
            const AllOutOfRange& for_all_out_of_range) :
        base(std::forward<Empty>(on_empty),
             std::forward<InvalidFormat>(on_invalid_format),
             for_all_out_of_range, for_all_out_of_range)
    {}

    template <class Empty, class InvalidFormat,
        class AboveUpperLimit, class BelowLowerLimit,
        std::enable_if_t<
            is_acceptable_arg_v<T, Empty&&>
         && is_acceptable_arg_v<T, InvalidFormat&&>
         && is_acceptable_arg_v<T, AboveUpperLimit&&>
         && is_acceptable_arg_v<T, BelowLowerLimit&&>>* = nullptr>
    base(
        Empty&& on_empty,
        InvalidFormat&& on_invalid_format,
        AboveUpperLimit&& on_above_upper_limit,
        BelowLowerLimit&& on_below_lower_limit)
            noexcept(std::is_nothrow_default_constructible_v<T>
                  && is_nothrow_arg_v<T, Empty&&>
                  && is_nothrow_arg_v<T, InvalidFormat&&>
                  && is_nothrow_arg_v<T, AboveUpperLimit&&>
                  && is_nothrow_arg_v<T, BelowLowerLimit&&>) :
        base<T, 0>(
            generic_args_t(),
            std::forward<Empty>(on_empty),
            std::forward<InvalidFormat>(on_invalid_format),
            std::forward<AboveUpperLimit>(on_above_upper_limit),
            std::forward<BelowLowerLimit>(on_below_lower_limit))
    {}
};

template <class T>
struct base<T, 5> : base<T, 0>
{
    template <class All = T,
        std::enable_if_t<is_acceptable_arg_v<T, const All&>>* = nullptr>
    explicit base(const All& for_all = All()) :
        base(for_all, for_all, for_all, for_all, for_all)
    {}

    template <class Empty, class AllButEmpty,
        std::enable_if_t<
            is_acceptable_arg_v<T, Empty&&>
         && is_acceptable_arg_v<T, const AllButEmpty&>>* = nullptr>
    explicit base(Empty&& on_empty, const AllButEmpty& for_all_but_empty) :
        base(std::forward<Empty>(on_empty), for_all_but_empty,
            for_all_but_empty, for_all_but_empty, for_all_but_empty)
    {}

    template <class Empty, class InvalidFormat, class AllOutOfRange,
        std::enable_if_t<
            is_acceptable_arg_v<T, Empty&&>
         && is_acceptable_arg_v<T, InvalidFormat&&>
         && is_acceptable_arg_v<T, const AllOutOfRange&>>* = nullptr>
    base(Empty&& on_empty, InvalidFormat&& on_invalid_format,
            const AllOutOfRange& for_all_out_of_range) :
        base(std::forward<Empty>(on_empty),
             std::forward<InvalidFormat>(on_invalid_format),
             for_all_out_of_range, for_all_out_of_range, for_all_out_of_range)
    {}

    template <class Empty, class InvalidFormat,
        class AboveUpperLimit, class BelowLowerLimit,
        class Underflow,
        std::enable_if_t<
            is_acceptable_arg_v<T, Empty>
         && is_acceptable_arg_v<T, InvalidFormat>
         && is_acceptable_arg_v<T, AboveUpperLimit>
         && is_acceptable_arg_v<T, BelowLowerLimit>
         && is_acceptable_arg_v<T, Underflow>>* = nullptr>
    base(
        Empty&& on_empty,
        InvalidFormat&& on_invalid_format,
        AboveUpperLimit&& on_above_upper_limit,
        BelowLowerLimit&& on_below_lower_limit,
        Underflow&& on_underflow)
            noexcept(is_nothrow_arg_v<T, Empty>
                  && is_nothrow_arg_v<T, InvalidFormat>
                  && is_nothrow_arg_v<T, AboveUpperLimit>
                  && is_nothrow_arg_v<T, BelowLowerLimit>
                  && is_nothrow_arg_v<T, Underflow>) :
        base<T, 0>(
            generic_args_t(),
            std::forward<Empty>(on_empty),
            std::forward<InvalidFormat>(on_invalid_format),
            std::forward<AboveUpperLimit>(on_above_upper_limit),
            std::forward<BelowLowerLimit>(on_below_lower_limit),
            std::forward<Underflow>(on_underflow))
    {}
};

template <class T>
using base_t = base<T, base_n<T>>;

} // detail::xlate::replace_if_conversion_failed_impl

template <class T>
class replace_if_conversion_failed :
    public detail::xlate::replace_if_conversion_failed_impl::base_t<T>
{
    using replace_mode = detail::replace_mode;

    static constexpr unsigned slot_empty =
        detail::xlate::replace_if_conversion_failed_impl::slot_empty;
    static constexpr unsigned slot_invalid_format =
        detail::xlate::replace_if_conversion_failed_impl::
            slot_invalid_format;
    static constexpr unsigned slot_above_upper_limit =
        detail::xlate::replace_if_conversion_failed_impl::
            slot_above_upper_limit;
    static constexpr unsigned slot_below_lower_limit =
        detail::xlate::replace_if_conversion_failed_impl::
            slot_below_lower_limit;
    static constexpr unsigned slot_underflow =
        detail::xlate::replace_if_conversion_failed_impl::slot_underflow;

private:
    template <class... As>
    replace_if_conversion_failed(detail::generic_args_t, As&&...) = delete;

public:
    using value_type = T;
    static constexpr std::size_t size = detail::xlate::
        replace_if_conversion_failed_impl::base_t<T>::store_t::size;

    // VS2019/2022 refuses to compile "base_t<T>" here
    using detail::xlate::replace_if_conversion_failed_impl::base<T,
        detail::xlate::replace_if_conversion_failed_impl::base_n<T>>::
        base;

    replace_if_conversion_failed(
        const replace_if_conversion_failed&) = default;
    replace_if_conversion_failed(
        replace_if_conversion_failed&&) = default;
    ~replace_if_conversion_failed() = default;
    replace_if_conversion_failed& operator=(
        const replace_if_conversion_failed&) = default;
    replace_if_conversion_failed& operator=(
        replace_if_conversion_failed&&) = default;

    template <class Ch>
    std::optional<T> operator()(invalid_format_t,
        const Ch* begin, const Ch* end) const
    {
        return unwrap(this->store().get(slot_invalid_format),
            [begin, end]() {
                fail_if_conversion_failed()(invalid_format_t(),
                    begin, end, static_cast<T*>(nullptr));
            });
    }

    template <class Ch>
    std::optional<T> operator()(out_of_range_t,
        const Ch* begin, const Ch* end, int sign) const
    {
        const auto fail = [begin, end, sign]() {
            fail_if_conversion_failed()(out_of_range_t(),
                begin, end, sign, static_cast<T*>(nullptr));
        };
        if (sign > 0) {
            return unwrap(this->store().get(slot_above_upper_limit), fail);
        } else if (sign < 0) {
            return unwrap(this->store().get(slot_below_lower_limit), fail);
        } else {
            return unwrap(this->store().get(slot_underflow), fail);
        }
    }

    std::optional<T> operator()(empty_t) const
    {
        return unwrap(this->store().get(slot_empty),
            []() {
                fail_if_conversion_failed()(empty_t(),
                    static_cast<T*>(nullptr));
            });
    }

    void swap(replace_if_conversion_failed& other)
        noexcept(std::is_nothrow_swappable_v<T>
              && std::is_nothrow_constructible_v<T>)
    {
        // There seem to be pairs of a compiler and a standard library
        // implementation (at least Clang 7) whose std::swap is not self-safe
        // std::swap, which inherently employs move construction, which is not
        // required to be self-safe
        // (Clang7 seems to employ memcpy for move construction of trivially-
        // copyable types, which itself is perfectly correct, and which is not,
        // however, suitable for std::swap without self-check)
        if (this != std::addressof(other)) {
            using std::swap;
            swap(this->store(), other.store());
        }
    }

private:
    template <class Fail>
    auto unwrap(const std::pair<replace_mode, const T*>& p, Fail fail) const
    {
        switch (p.first) {
        case replace_mode::replace:
            return std::optional<T>(*p.second);
        case replace_mode::ignore:
            return std::optional<T>();
        case replace_mode::fail:
        default:
            break;
        }
        fail();
        assert(false);
        return std::optional<T>();
    }
};

template <class T>
auto swap(
    replace_if_conversion_failed<T>& left,
    replace_if_conversion_failed<T>& right)
    noexcept(noexcept(left.swap(right)))
 -> std::enable_if_t<std::is_swappable_v<T>>
{
    left.swap(right);
}

template <class... Ts,
    std::enable_if_t<
        !std::is_same_v<
            detail::replaced_type_not_found_t,
            detail::replaced_type_from_t<Ts...>>>* = nullptr>
replace_if_conversion_failed(Ts...)
 -> replace_if_conversion_failed<detail::replaced_type_from_t<Ts...>>;

namespace detail::xlate {

template <class A>
auto do_c_str(const A& a) -> decltype(a.c_str())
{
    return a.c_str();
}

template <class A>
auto do_c_str(const A& a, ...) -> decltype(a->c_str())
{
    return a->c_str();
}

template <class A>
auto do_size(const A& a) -> decltype(a.size())
{
    return a.size();
}

template <class A>
auto do_size(const A& a, ...) -> decltype(a->size())
{
    return a->size();
}

template <class T, class H>
class typed_conversion_error_handler
{
    std::reference_wrapper<H> handler_;

public:
    explicit typed_conversion_error_handler(H& handler) :
        handler_(handler)
    {}

    template <class Ch>
    std::optional<T> invalid_format(const Ch* begin, const Ch* end) const
    {
        if constexpr (std::is_invocable_v<H,
                invalid_format_t, const Ch*, const Ch*>) {
            return handler_(invalid_format_t(), begin, end);
        } else {
            return handler_(invalid_format_t(), begin, end,
                            static_cast<T*>(nullptr));
        }
    }

    template <class Ch>
    std::optional<T> out_of_range(
        const Ch* begin, const Ch* end, int sign) const
    {
        if constexpr (std::is_invocable_v<H,
                out_of_range_t, const Ch*, const Ch*, int>) {
            return handler_(out_of_range_t(), begin, end, sign);
        } else {
            return handler_(out_of_range_t(), begin, end, sign,
                            static_cast<T*>(nullptr));
        }
    }

    std::optional<T> empty() const
    {
        if constexpr (std::is_invocable_v<H, empty_t>) {
            return handler_(empty_t());
        } else {
            return handler_(empty_t(), static_cast<T*>(nullptr));
        }
    }
};

template <class T, class A, class H>
auto do_convert(const A& a, H&& h)
{
    const auto* const c_str = do_c_str(a);
    const auto size = do_size(a);
    return converter<T>()(c_str, c_str + size,
        typed_conversion_error_handler<T, std::remove_reference_t<H>>(h));
}

struct is_default_translatable_arithmetic_type_impl
{
    template <class T>
    static auto check(T* = nullptr) -> decltype(
        numeric_type_traits<std::remove_cv_t<T>>::name, std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

} // end detail::xlate

template <class T>
struct is_default_translatable_arithmetic_type : decltype(detail::xlate::
    is_default_translatable_arithmetic_type_impl::check<T>(nullptr))
{};

template <class T>
inline constexpr bool is_default_translatable_arithmetic_type_v =
    is_default_translatable_arithmetic_type<T>::value;

template <class T, class ConversionErrorHandler, class A>
T to_arithmetic(const A& a, ConversionErrorHandler&& handler)
{
    if constexpr (detail::is_std_optional_v<T>) {
        return detail::xlate::do_convert<typename T::value_type>(a, handler);
    } else if constexpr (std::is_same_v<fail_if_conversion_failed,
                                    std::decay_t<ConversionErrorHandler>>) {
        // We know that an empty optional would never be returned when handler
        // is a fail_if_conversion_failed
        return *detail::xlate::do_convert<T>(a, handler);
    } else {
        return detail::xlate::do_convert<T>(a, handler).value();
    }
}

template <class T, class A>
T to_arithmetic(const A& a)
{
    if constexpr (detail::is_std_optional_v<T>) {
        using U = typename T::value_type;
        return detail::xlate::do_convert<U>(a, ignore_if_conversion_failed());
    } else {
        return *detail::xlate::do_convert<T>(a, fail_if_conversion_failed());
    }
}

}

#endif
