/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_GUARD_555AFEA0_404A_4BF3_AF38_4F653A0B76E6
#define FURFURYLIC_GUARD_555AFEA0_404A_4BF3_AF38_4F653A0B76E6

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cwchar>
#include <cwctype>
#include <iterator>
#include <limits>
#include <locale>
#include <memory>
#include <sstream>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <vector>

#include "csv_error.hpp"
#include "member_like_base.hpp"

namespace furfurylic {
namespace commata {

template <class Ch, class Tr = std::char_traits<Ch>,
          class Alloc = std::allocator<Ch>>
class csv_scanner
{
    struct field_scanner
    {
        virtual ~field_scanner() {}
        virtual void field_value(const Ch* begin, const Ch* end) = 0;
        virtual void field_value(std::basic_string<Ch, Tr, Alloc>&& value) = 0;
        virtual void field_skipped() = 0;
        virtual const std::type_info& get_type() const = 0;

        template <class T>
        const T* get_target() const
        {
            return (get_type() == typeid(T)) ?
                static_cast<const T*>(get_target_v()) : nullptr;
        }

        template <class T>
        T* get_target()
        {
            return (get_type() == typeid(T)) ?
                static_cast<T*>(get_target_v()) : nullptr;
        }

    private:
        virtual const void* get_target_v() const = 0;
        virtual void* get_target_v() = 0;
    };

    template <class FieldScanner>
    class typed_field_scanner : public field_scanner
    {
        FieldScanner scanner_;

    public:
        explicit typed_field_scanner(FieldScanner scanner) :
            scanner_(std::move(scanner))
        {}

        void field_value(const Ch* begin, const Ch* end) override
        {
            scanner_.field_value(begin, end);
        }

        void field_value(std::basic_string<Ch, Tr, Alloc>&& value) override
        {
            scanner_.field_value(std::move(value));
        }

        void field_skipped() override
        {
            scanner_.field_skipped();
        }

        const std::type_info& get_type() const override
        {
            // Should return the static type in which this instance was
            // created (not the dynamic type).
            return typeid(FieldScanner);
        }

    private:
        const void* get_target_v() const override
        {
            return &scanner_;
        }

        void* get_target_v() override
        {
            return &scanner_;
        }
    };

    std::size_t i_;
    std::size_t j_;
    std::size_t buffer_size_;
    std::unique_ptr<Ch[]> buffer_;
    const Ch* begin_;
    const Ch* end_;
    std::vector<std::unique_ptr<field_scanner>> scanners_;
    std::basic_string<Ch, Tr, Alloc> fragmented_value_;

public:
    using char_type = Ch;

    explicit csv_scanner(std::size_t buffer_size = 8192) :
        i_(0), j_(0),
        buffer_size_(std::max(buffer_size, static_cast<std::size_t>(2U))),
        begin_(nullptr)
    {}

    template <class FieldScanner = std::nullptr_t>
    void set_field_scanner(std::size_t j, FieldScanner s = FieldScanner())
    {
        do_set_field_scanner(j, std::move(s));
    }

    const std::type_info& get_field_scanner_type(std::size_t j) const
    {
        if ((j < scanners_.size()) && scanners_[j]) {
            return scanners_[j]->get_type();
        } else {
            return typeid(void);
        }
    }

    bool has_field_scanner(std::size_t j) const
    {
        return (j < scanners_.size()) && scanners_[j];
    }

    template <class FieldScanner>
    const FieldScanner* get_field_scanner(std::size_t j) const
    {
        return get_field_scanner_g<FieldScanner>(*this, j);
    }

    template <class FieldScanner>
    FieldScanner* get_field_scanner(std::size_t j)
    {
        return get_field_scanner_g<FieldScanner>(*this, j);
    }

private:
    template <class FieldScanner>
    void do_set_field_scanner(std::size_t j, FieldScanner s)
    {
        using scanner_t = typed_field_scanner<FieldScanner>;
        auto p = std::make_unique<scanner_t>(std::move(s)); // throw
        if (j >= scanners_.size()) {
            scanners_.resize(j + 1);                        // throw
        }
        scanners_[j] = std::move(p);
    }

    void do_set_field_scanner(std::size_t j, std::nullptr_t)
    {
        if (j < scanners_.size()) {
            scanners_[j].reset();
        }
    }

    template <class FieldScanner, class ThisType>
    static auto get_field_scanner_g(ThisType& me, std::size_t j)
    {
        return me.has_field_scanner(j) ?
            me.scanners_[j]->template get_target<FieldScanner>() :
            nullptr;
    }

public:
    std::pair<Ch*, std::size_t> get_buffer()
    {
        if (!buffer_) {
            buffer_.reset(new Ch[buffer_size_]);    // throw
        } else if (begin_) {
            fragmented_value_.assign(begin_, end_); // throw
            begin_ = nullptr;
        }
        return { buffer_.get(), buffer_size_ - 1 }; // throw
        // We'd like to push buffer_[buffer_size_] with '\0'
        // so we tell the driver the buffer size is smaller by one
    }

    void release_buffer(const Ch*)
    {}

    void start_record(const Ch* /*record_begin*/)
    {}

    bool update(const Ch* first, const Ch* last)
    {
        if ((j_ < scanners_.size()) && scanners_[j_] && (first != last)) {
            if (begin_) {
                fragmented_value_.assign(begin_, end_);     // throw
                fragmented_value_.append(first, last);      // throw
                begin_ = nullptr;
            } else if (!fragmented_value_.empty()) {
                fragmented_value_.append(first, last);      // throw
            } else {
                begin_ = first;
                end_ = last;
            }
        }
        return true;
    }

    bool finalize(const Ch* first, const Ch* last)
    {
        if ((j_ < scanners_.size())) {
            if (const auto scanner = scanners_[j_].get()) {
                if (begin_) {
                    if (first != last) {
                        fragmented_value_.assign(begin_, end_);     // throw
                        fragmented_value_.append(first, last);      // throw
                        scanner->field_value(std::move(fragmented_value_));
                        fragmented_value_.clear();
                    } else {
                        buffer_[end_ - buffer_.get()] = Ch();
                        scanner->field_value(begin_, end_);
                    }
                } else if (!fragmented_value_.empty()) {
                    fragmented_value_.append(first, last);          // throw
                    scanner->field_value(std::move(fragmented_value_));
                    fragmented_value_.clear();
                } else {
                    buffer_[last - buffer_.get()] = Ch();
                    scanner->field_value(first, last);
                }
                begin_ = nullptr;
            }
        }
        ++j_;
        return true;
    }

    bool end_record(const Ch* /*record_end*/)
    {
        for (auto j = j_; j < scanners_.size(); ++j) {
            if (auto scanner = scanners_[j].get()) {
                scanner->field_skipped();
            }
        }
        ++i_;
        j_ = 0;
        return true;
    }
};

class field_conversion_error : public csv_error
{
public:
    using csv_error::csv_error;
};

class field_not_found : public field_conversion_error
{
public:
    using field_conversion_error::field_conversion_error;
};

class field_invalid_format : public field_conversion_error
{
public:
    using field_conversion_error::field_conversion_error;
};

class field_empty : public field_invalid_format
{
public:
    using field_invalid_format::field_invalid_format;
};

class field_out_of_range : public field_conversion_error
{
public:
    using field_conversion_error::field_conversion_error;
};

namespace detail {

template <class T>
struct numeric_type_traits;

template <>
struct numeric_type_traits<short>
{
    static constexpr const char* name = "short int";
    using raw_type = long;
};

template <>
struct numeric_type_traits<unsigned short>
{
    static constexpr const char* name = "unsigned short int";
    using raw_type = unsigned long;
};

template <>
struct numeric_type_traits<int>
{
    static constexpr const char* name = "int";
    using raw_type = long;
};

template <>
struct numeric_type_traits<unsigned>
{
    static constexpr const char* name = "unsigned int";
    using raw_type = unsigned long;
};

template <>
struct numeric_type_traits<long>
{
    static constexpr const char* name = "long int";
};

template <>
struct numeric_type_traits<unsigned long>
{
    static constexpr const char* name = "unsigned long int";
};

template <>
struct numeric_type_traits<long long>
{
    static constexpr const char* name = "long long int";
};

template <>
struct numeric_type_traits<unsigned long long>
{
    static constexpr const char* name = "unsigned long long int";
};

template <>
struct numeric_type_traits<float>
{
    static constexpr const char* name = "float";
};

template <>
struct numeric_type_traits<double>
{
    static constexpr const char* name = "double";
};

template <>
struct numeric_type_traits<long double>
{
    static constexpr const char* name = "long double";
};

template <class T>
struct handle_conversion_error
{
    using range_type = T;

    template <class Ch>
    [[noreturn]]
    T invalid_format(const Ch* begin, const Ch* end) const
    {
        // TODO: OK? (are there any other zeros?)
        assert(*end == Ch());
        std::ostringstream s;
        narrow(s, begin, end);
        s << ": cannot convert to an instance of " << name;
        throw field_invalid_format(s.str());
    }

    template <class Ch>
    [[noreturn]]
    T out_of_range(const Ch* begin, const Ch* end, T /*proposed*/) const
    {
        assert(*end == Ch());
        std::ostringstream s;
        narrow(s, begin, end);
        s << ": out of range of " << name;
        throw field_out_of_range(s.str());
    }

    [[noreturn]]
    T empty() const
    {
        std::ostringstream s;
        s << "Cannot convert an empty string to an instance of " << name;
        throw field_empty(s.str());
    }

private:
    static constexpr const char* name = numeric_type_traits<T>::name;

    template <class Ch>
    static void narrow(std::ostream& os, const Ch* begin, const Ch* end)
    {
        const auto& facet =
            std::use_facet<std::ctype<Ch>>(std::locale::classic());
        std::transform(begin, end, std::ostreambuf_iterator<char>(os),
            [&facet](Ch c) {
                return facet.narrow(c, '?');
            });
    }

    static void narrow(std::ostream& os, const char* begin, const char*
#ifndef NDEBUG
        end
#endif
        )
    {
        assert(*end == '\0');
        os << begin;
    }
};

template <class D>
struct raw_convert_base
{
    template <class Ch>
    auto convert_raw(const Ch* begin, const Ch* end) const
    {
        Ch* middle;
        errno = 0;
        const auto r = static_cast<const D*>(this)->engine(begin, &middle);
        using result_t = std::decay_t<decltype(r)>;

        const auto has_postfix =
            std::find_if(static_cast<const Ch*>(middle), end, [](Ch c) {
                return !is_space(c);
            }) != end;
        if (begin == middle) {
            // no conversion could be performed
            if (has_postfix) {
                return static_cast<result_t>(
                    static_cast<const D*>(this)->invalid_format(begin, end));
            } else {
                // whitespace only
                return static_cast<result_t>(
                    static_cast<const D*>(this)->empty());
            }
        } else if (has_postfix) {
            // if an not-whitespace-extra-character found, it is NG
            return static_cast<result_t>(
                static_cast<const D*>(this)->invalid_format(begin, end));
        } else if (errno == ERANGE) {
            using range_t = typename D::range_type;
            using limits_t = std::numeric_limits<range_t>;
            auto saturated_result = (limits_t::min() > r) ?
                limits_t::min() : r;
            saturated_result = (limits_t::max() < r) ?
                limits_t::max() : r;
            return static_cast<result_t>(
                static_cast<const D*>(this)->out_of_range(
                    begin, end, static_cast<range_t>(saturated_result)));
        } else {
            return r;
        }
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

template <class T, class U = T>
struct raw_convert;

template <class U>
struct raw_convert<long, U> :
    raw_convert_base<raw_convert<long, U>>,
    handle_conversion_error<U>
{
    auto engine(const char* s, char** e) const
    {
        return std::strtol(s, e, 10);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return std::wcstol(s, e, 10);
    }
};

template <class U>
struct raw_convert<unsigned long, U> :
    raw_convert_base<raw_convert<unsigned long, U>>,
    handle_conversion_error<U>
{
    auto engine(const char* s, char** e) const
    {
        return std::strtoul(s, e, 10);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return std::wcstoul(s, e, 10);
    }
};

template <class U>
struct raw_convert<long long, U> :
    raw_convert_base<raw_convert<long long, U>>,
    handle_conversion_error<U>
{
    auto engine(const char* s, char** e) const
    {
        return std::strtoll(s, e, 10);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return std::wcstoll(s, e, 10);
    }
};

template <class U>
struct raw_convert<unsigned long long, U> :
    raw_convert_base<raw_convert<unsigned long long, U>>,
    handle_conversion_error<U>
{
    auto engine(const char* s, char** e) const
    {
        return std::strtoull(s, e, 10);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return std::wcstoull(s, e, 10);
    }
};

template <class U>
struct raw_convert<float, U> :
    raw_convert_base<raw_convert<float, U>>,
    handle_conversion_error<U>
{
    auto engine(const char* s, char** e) const
    {
        return std::strtof(s, e);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return std::wcstof(s, e);
    }
};

template <class U>
struct raw_convert<double, U> :
    raw_convert_base<raw_convert<double, U>>,
    handle_conversion_error<U>
{
    auto engine(const char* s, char** e) const
    {
        return std::strtod(s, e);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return std::wcstod(s, e);
    }
};

template <class U>
struct raw_convert<long double, U> :
    raw_convert_base<raw_convert<long double, U>>,
    handle_conversion_error<U>
{
    auto engine(const char* s, char** e) const
    {
        return std::strtold(s, e);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return std::wcstold(s, e);
    }
};

template <class T, class = void>
struct convert :
    private raw_convert<T>
{
    template <class Ch>
    auto operator()(const Ch* begin, const Ch* end) const
    {
        return this->convert_raw(begin, end);
    }
};

template <class T, class U, class = void>
struct restrained_convert :
    private raw_convert<U, T>
{
    template <class Ch>
    T operator()(const Ch* begin, const Ch* end) const
    {
        const auto result = this->convert_raw(begin, end);
        if (result < std::numeric_limits<T>::lowest()) {
            return this->
                out_of_range(begin, end, std::numeric_limits<T>::lowest());
        } else if (std::numeric_limits<T>::max() < result) {
            return this->
                out_of_range(begin, end, std::numeric_limits<T>::max());
        } else {
            return static_cast<T>(result);
        }
    }
};

template <class T, class U>
struct restrained_convert<T, U, std::enable_if_t<std::is_unsigned<T>::value>> :
    private raw_convert<U, T>
{
    template <class Ch>
    T operator()(const Ch* begin, const Ch* end) const
    {
        const auto result = this->convert_raw(begin, end);
        if (result <= std::numeric_limits<T>::max()) {
            return static_cast<T>(result);
        } else {
            const auto s = static_cast<std::make_signed_t<U>>(result);
            if ((s < 0)
             && (static_cast<U>(-s) <= std::numeric_limits<T>::max())) {
                return static_cast<T>(s);
            }
        }
        return this->out_of_range(begin, end, std::numeric_limits<T>::max());
    }
};

template <class>
using void_t = void;

template <class T>
struct convert<T, void_t<typename numeric_type_traits<T>::raw_type>> :
    restrained_convert<T, typename numeric_type_traits<T>::raw_type>
{};

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
struct is_back_insertable :
    decltype(is_back_insertable_impl::check<T>(nullptr))
{};

template <class SkippingHandler>
struct skipping_handler_holder :
    private member_like_base<SkippingHandler>
{
    using member_like_base<SkippingHandler>::member_like_base;

    const SkippingHandler& get_skipping_handler() const
    {
        return this->get();
    }

    SkippingHandler& get_skipping_handler()
    {
        return this->get();
    }
};

}

template <class T>
struct fail_if_skipped
{
    [[noreturn]]
    const T& skipped() const
    {
        throw field_not_found("This field did not appear in this record");
    }
};

template <class T>
class default_if_skipped
{
    T default_value_;

public:
    explicit default_if_skipped(T default_value = T()) :
        default_value_(std::move(default_value))
    {}

    const T& skipped() const
    {
        return default_value_;
    }
};

template <class T, class OutputIterator,
    class SkippingHandler = fail_if_skipped<T>>
class field_translator :
    detail::convert<T>,
    public detail::skipping_handler_holder<SkippingHandler>
{
    OutputIterator out_;

public:
    explicit field_translator(
        OutputIterator out,
        SkippingHandler handle_skipping = SkippingHandler()) :
        detail::skipping_handler_holder<SkippingHandler>(
            std::move(handle_skipping)),
        out_(std::move(out))
    {}

    template <class Ch>
    void field_value(const Ch* begin, const Ch* end)
    {
        assert(*end == Ch());   // shall be null-teminated
        *out_ = (*this)(begin, end);
        ++out_;
    }

    template <class Ch, class Tr, class Alloc>
    void field_value(const std::basic_string<Ch, Tr, Alloc>& value)
    {
        field_value(value.c_str(), value.c_str() + value.size());
    }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4702)
#endif
    void field_skipped()
    {
        *out_ = this->get_skipping_handler().skipped();
        ++out_;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    }
};

template <class Ch, class Tr, class Alloc,
          class OutputIterator, class SkippingHandler>
class field_translator<
    std::basic_string<Ch, Tr, Alloc>, OutputIterator, SkippingHandler> :
    public detail::skipping_handler_holder<SkippingHandler>
{
    OutputIterator out_;

public:
    explicit field_translator(
        OutputIterator out,
        SkippingHandler handle_skipping = SkippingHandler()) :
        detail::skipping_handler_holder<SkippingHandler>(
            std::move(handle_skipping)),
        out_(std::move(out))
    {}

    void field_value(const Ch* begin, const Ch* end)
    {
        field_value(std::basic_string<Ch, Tr, Alloc>(begin, end));
    }

    void field_value(std::basic_string<Ch, Tr, Alloc>&& value)
    {
        *out_ = std::move(value);
        ++out_;
    }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4702)
#endif
    void field_skipped()
    {
        *out_ = this->get_skipping_handler().skipped();
        ++out_;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    }
};

template <
    class T, class OutputIterator,
    class SkippingHandler = fail_if_skipped<T>>
auto make_field_translator(
    OutputIterator out,
    SkippingHandler handle_skipping = SkippingHandler())
{
    return field_translator<T, OutputIterator, SkippingHandler>(
        std::move(out), std::move(handle_skipping));
}

namespace detail {

template <
    class Container,
    class SkippingHandler,
    std::enable_if_t<
        !is_back_insertable<Container>::value>* = nullptr>
auto make_field_translator_c_impl(
    Container& values, SkippingHandler&& handle_skipping)
{
    return make_field_translator<typename Container::value_type>(
        std::inserter(values, values.end()),
        std::forward<SkippingHandler>(handle_skipping));
}

template <
    class Container,
    class SkippingHandler,
    std::enable_if_t<
        is_back_insertable<Container>::value>* = nullptr>
auto make_field_translator_c_impl(
    Container& values, SkippingHandler&& handle_skipping)
{
    return make_field_translator<typename Container::value_type>(
        std::back_inserter(values),
        std::forward<SkippingHandler>(handle_skipping));
}

}

template <
    class Container,
    class SkippingHandler =
        fail_if_skipped<typename Container::value_type>>
auto make_field_translator_c(
    Container& values,
    SkippingHandler handle_skipping = SkippingHandler())
{
    return detail::make_field_translator_c_impl(
        values, std::move(handle_skipping));
}

}}

#endif
