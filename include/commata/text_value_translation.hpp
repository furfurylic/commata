/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_0DE728A7_BABE_4C3A_94B1_A05CB3D0C9E4
#define COMMATA_GUARD_0DE728A7_BABE_4C3A_94B1_A05CB3D0C9E4

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cwctype>
#include <optional>
#include <string_view>
#include <type_traits>

#include "text_error.hpp"

namespace commata {

class field_translation_error : public text_error
{
public:
    using text_error::text_error;
};

class field_invalid_format : public field_translation_error
{
public:
    using field_translation_error::field_translation_error;
};

class field_empty : public field_invalid_format
{
public:
    using field_invalid_format::field_invalid_format;
};

class field_out_of_range : public field_translation_error
{
public:
    using field_translation_error::field_translation_error;
};

struct replacement_ignore_t {};
inline constexpr replacement_ignore_t replacement_ignore{};

struct replacement_fail_t {};
inline constexpr replacement_fail_t replacement_fail{};

struct invalid_format_t {};
struct out_of_range_t {};
struct empty_t {};

namespace detail {

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

struct is_convertible_numeric_type_impl
{
    template <class T>
    static auto check(T* = nullptr)
     -> decltype(numeric_type_traits<T>::name, std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

template <class T>
constexpr bool is_convertible_numeric_type_v =
    decltype(is_convertible_numeric_type_impl::check<T>(nullptr))();

namespace xlate {

// D must derive from raw_converter_base<D, H> (CRTP)
// and must have member functions "engine" for const char* and const wchar_t*
template <class D, class H>
class raw_converter_base :
    member_like_base<H>
{
public:
    using member_like_base<H>::member_like_base;

    template <class Ch>
    auto convert_raw(const Ch* begin, const Ch* end)
    {
        Ch* middle;
        errno = 0;
        const auto r = static_cast<const D*>(this)->engine(begin, &middle);
        using ret_t = std::optional<std::remove_const_t<decltype(r)>>;

        const auto has_postfix =
            std::any_of<const Ch*>(middle, end, [](Ch c) {
                return !is_space(c);
            });
        if (has_postfix) {
            // if a not-whitespace-extra-character found, it is NG
            return ret_t(this->get().invalid_format(begin, end));
        } else if (begin == middle) {
            // whitespace only
            return ret_t(this->get().empty());
        } else if (errno == ERANGE) {
            return ret_t(this->get().out_of_range(begin, end,
                static_cast<const D*>(this)->erange(r)));
        } else {
            return ret_t(r);
        }
    }

    decltype(auto) get_conversion_error_handler() noexcept
    {
        return this->get().get();
    }

    decltype(auto) get_conversion_error_handler() const noexcept
    {
        return this->get().get();
    }

private:
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

template <class T, class H, class = void>
struct raw_converter;

// For integral types
template <class T, class H>
struct raw_converter<T, H, std::enable_if_t<std::is_integral_v<T>,
        std::void_t<decltype(numeric_type_traits<T>::strto)>>> :
    raw_converter_base<raw_converter<T, H>, H>
{
    using raw_converter_base<raw_converter<T, H>, H>
        ::raw_converter_base;
    using raw_converter_base<raw_converter<T, H>, H>
        ::get_conversion_error_handler;

    auto engine(const char* s, char** e) const
    {
        return numeric_type_traits<T>::strto(s, e, 10);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return numeric_type_traits<T>::wcsto(s, e, 10);
    }

    int erange([[maybe_unused]] T v) const
    {
        if constexpr (std::is_signed_v<T>) {
            return (v > T()) ? 1 : -1;
        } else {
            return 1;
        }
    }
};

// For floating-point types
template <class T, class H>
struct raw_converter<T, H, std::enable_if_t<std::is_floating_point_v<T>,
        std::void_t<decltype(numeric_type_traits<T>::strto)>>> :
    raw_converter_base<raw_converter<T, H>, H>
{
    using raw_converter_base<raw_converter<T, H>, H>
        ::raw_converter_base;
    using raw_converter_base<raw_converter<T, H>, H>
        ::get_conversion_error_handler;

    auto engine(const char* s, char** e) const
    {
        return numeric_type_traits<T>::strto(s, e);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return numeric_type_traits<T>::wcsto(s, e);
    }

    int erange(T v) const
    {
        return (v > T()) ? 1 : (v < T()) ? -1 : 0;
    }
};

template <class T>
struct conversion_error_facade
{
    template <class H, class Ch>
    static std::optional<T> invalid_format(
        H& h,const Ch* begin, const Ch* end)
    {
        if constexpr (std::is_invocable_v<H,
                invalid_format_t, const Ch*, const Ch*>) {
            return h(invalid_format_t(), begin, end);
        } else {
            return h(invalid_format_t(), begin, end, static_cast<T*>(nullptr));
        }
    }

    template <class H, class Ch>
    static std::optional<T> out_of_range(
        H& h, const Ch* begin, const Ch* end, int sign)
    {
        if constexpr (std::is_invocable_v<H,
                out_of_range_t, const Ch*, const Ch*, int>) {
            return h(out_of_range_t(), begin, end, sign);
        } else {
            return h(out_of_range_t(),
                     begin, end, sign, static_cast<T*>(nullptr));
        }
    }

    template <class H>
    static std::optional<T> empty(H& h)
    {
        if constexpr (std::is_invocable_v<H, empty_t>) {
            return h(empty_t());
        } else {
            return h(empty_t(), static_cast<T*>(nullptr));
        }
    }
};

template <class T, class H>
struct typed_conversion_error_handler :
    private member_like_base<H>
{
    using member_like_base<H>::member_like_base;
    using member_like_base<H>::get;

    template <class Ch>
    std::optional<T> invalid_format(const Ch* begin, const Ch* end) const
    {
        return conversion_error_facade<T>::
            invalid_format(this->get(), begin, end);
    }

    template <class Ch>
    std::optional<T> out_of_range(
        const Ch* begin, const Ch* end, int sign) const
    {
        return conversion_error_facade<T>::
            out_of_range(this->get(), begin, end, sign);
    }

    std::optional<T> empty() const
    {
        return conversion_error_facade<T>::empty(this->get());
    }
};

} // end xlate

// For types without corresponding "raw_type"
template <class T, class H, class = void>
struct converter :
    private xlate::raw_converter<T,
        xlate::typed_conversion_error_handler<T, H>>
{
    template <class K,
        std::enable_if_t<!std::is_base_of_v<converter, std::decay_t<K>>>*
            = nullptr>
    converter(K&& h) :
        xlate::raw_converter<T,
            xlate::typed_conversion_error_handler<T, H>>(
                xlate::typed_conversion_error_handler<T, H>(
                    std::forward<K>(h)))
    {}

    using xlate::raw_converter<T,
        xlate::typed_conversion_error_handler<T, H>>::
            get_conversion_error_handler;

    template <class Ch>
    auto convert(const Ch* begin, const Ch* end)
    {
        return this->convert_raw(begin, end);
    }
};

namespace xlate {

template <class T, class H, class U, class = void>
struct restrained_converter :
    private raw_converter<U, H>
{
    static_assert(!std::is_floating_point_v<T>);

    using raw_converter<U, H>::raw_converter;
    using raw_converter<U, H>::get_conversion_error_handler;

    template <class Ch>
    std::optional<T> convert(const Ch* begin, const Ch* end)
    {
        const auto result = this->convert_raw(begin, end);
        if (!result.has_value()) {
            return std::optional<T>();
        }
        const auto r = *result;
        if (r < std::numeric_limits<T>::lowest()) {
            return conversion_error_facade<T>::out_of_range(
                this->get_conversion_error_handler(), begin, end, -1);
        } else if (std::numeric_limits<T>::max() < r) {
            return conversion_error_facade<T>::out_of_range(
                this->get_conversion_error_handler(), begin, end, 1);
        } else {
            return std::optional<T>(static_cast<T>(r));
        }
    }
};

template <class T, class H, class U>
struct restrained_converter<T, H, U,
        std::enable_if_t<std::is_unsigned_v<T>>> :
    private raw_converter<U, H>
{
    static_assert(!std::is_floating_point_v<T>);

    using raw_converter<U, H>::raw_converter;
    using raw_converter<U, H>::get_conversion_error_handler;

    template <class Ch>
    std::optional<T> convert(const Ch* begin, const Ch* end)
    {
        const auto result = this->convert_raw(begin, end);
        if (!result.has_value()) {
            return std::nullopt;
        }
        const auto r = *result;
        if constexpr (sizeof(T) < sizeof(U)) {
            constexpr auto t_max = std::numeric_limits<T>::max();
            if (r <= t_max) {
                return static_cast<T>(r);
            } else {
                const auto s = static_cast<std::make_signed_t<U>>(r);
                if (s < 0) {
                    // -t_max is the lowest number that can be wrapped around
                    // and then returned
                    const auto s_wrapped_around = s + t_max + 1;
                    if (s_wrapped_around > 0) {
                        return static_cast<T>(s_wrapped_around);
                    }
                }
            }
            return conversion_error_facade<T>::out_of_range(
                this->get_conversion_error_handler(), begin, end, 1);
        } else {
            return static_cast<T>(r);
        }
    }
};

} // end xlate

// For types which have corresponding "raw_type"
template <class T, class H>
struct converter<T, H,
                 std::void_t<typename numeric_type_traits<T>::raw_type>> :
    xlate::restrained_converter<T,
        xlate::typed_conversion_error_handler<T, H>,
        typename numeric_type_traits<T>::raw_type>
{
    using xlate::restrained_converter<T,
        xlate::typed_conversion_error_handler<T, H>,
        typename numeric_type_traits<T>::raw_type>::restrained_converter;
};

} // end detail

}

#endif
