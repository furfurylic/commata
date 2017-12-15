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
#include <functional>
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
        virtual void field_value(Ch* begin, Ch* end, csv_scanner& me) = 0;
        virtual void field_value(
            std::basic_string<Ch, Tr, Alloc>&& value, csv_scanner& me) = 0;
    };

    struct header_field_scanner : field_scanner
    {
        virtual void so_much_for_header(csv_scanner& me) = 0;
    };

    template <class HeaderScanner>
    class typed_header_field_scanner : public header_field_scanner
    {
        HeaderScanner scanner_;

        using range_t = std::pair<const Ch*, const Ch*>;

    public:
        explicit typed_header_field_scanner(HeaderScanner scanner) :
            scanner_(std::move(scanner))
        {}

        void field_value(Ch* begin, Ch* end, csv_scanner& me) override
        {
            const range_t range(begin, end);
            if (!scanner_(me.j_, &range, me)) {
                me.header_field_scanner_.reset();
            }
        }

        void field_value(
            std::basic_string<Ch, Tr, Alloc>&& value, csv_scanner& me) override
        {
            field_value(&value[0], &value[0] + value.size(), me);
        }

        void so_much_for_header(csv_scanner& me) override
        {
            scanner_(me.j_, static_cast<const range_t*>(nullptr), me);
        }
    };

    struct body_field_scanner : field_scanner
    {
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
    class typed_body_field_scanner : public body_field_scanner
    {
        FieldScanner scanner_;

    public:
        explicit typed_body_field_scanner(FieldScanner scanner) :
            scanner_(std::move(scanner))
        {}

        void field_value(Ch* begin, Ch* end, csv_scanner&) override
        {
            scanner_.field_value(begin, end);
        }

        void field_value(
            std::basic_string<Ch, Tr, Alloc>&& value, csv_scanner&) override
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

    bool in_header_;
    std::size_t j_;
    std::size_t buffer_size_;
    std::unique_ptr<Ch[]> buffer_;
    const Ch* begin_;
    const Ch* end_;
    std::unique_ptr<header_field_scanner> header_field_scanner_;
    std::vector<std::unique_ptr<body_field_scanner>> scanners_;
    std::basic_string<Ch, Tr, Alloc> fragmented_value_;

public:
    using char_type = Ch;

    explicit csv_scanner(
            bool has_header = false,
            std::size_t buffer_size = 8192) :
        in_header_(has_header), j_(0),
        buffer_size_(std::max(buffer_size, static_cast<std::size_t>(2U))),
        begin_(nullptr)
    {}

    template <
        class HeaderFieldScanner,
        class = std::enable_if_t<!std::is_integral<HeaderFieldScanner>::value>>
    explicit csv_scanner(
            HeaderFieldScanner s,
            std::size_t buffer_size = 8192) :
        in_header_(true), j_(0),
        buffer_size_(std::max(buffer_size, static_cast<std::size_t>(2U))),
        begin_(nullptr),
        header_field_scanner_(
            new typed_header_field_scanner<HeaderFieldScanner>(std::move(s)))
    {}

    csv_scanner(csv_scanner&&) = default;

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
        using scanner_t = typed_body_field_scanner<FieldScanner>;
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
        if (get_scanner() && (first != last)) {
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
        if (const auto scanner = get_scanner()) {
            if (begin_) {
                if (first != last) {
                    fragmented_value_.assign(begin_, end_);     // throw
                    fragmented_value_.append(first, last);      // throw
                    scanner->field_value(std::move(fragmented_value_), *this);
                    fragmented_value_.clear();
                } else {
                    *unconst(end_) = Ch();
                    scanner->field_value(
                        unconst(begin_), unconst(end_), *this);
                }
            } else if (!fragmented_value_.empty()) {
                fragmented_value_.append(first, last);          // throw
                scanner->field_value(std::move(fragmented_value_), *this);
                fragmented_value_.clear();
            } else {
                *unconst(last) = Ch();
                scanner->field_value(unconst(first), unconst(last), *this);
            }
            begin_ = nullptr;
        }
        ++j_;
        return true;
    }

    bool end_record(const Ch* /*record_end*/)
    {
        if (in_header_) {
            if (header_field_scanner_) {
                header_field_scanner_->so_much_for_header(*this);
                header_field_scanner_.reset();
            }
            in_header_ = false;
        } else {
            for (auto j = j_; j < scanners_.size(); ++j) {
                if (auto scanner = scanners_[j].get()) {
                    scanner->field_skipped();
                }
            }
        }
        j_ = 0;
        return true;
    }

private:
    field_scanner* get_scanner()
    {
        if (in_header_) {
            return header_field_scanner_.get();
        } else if (j_ < scanners_.size()) {
            return scanners_[j_].get();
        } else {
            return nullptr;
        }
    }

    Ch* unconst(const Ch* s) const
    {
        return buffer_.get() + (s - buffer_.get());
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
struct numeric_type_traits<char>
{
    static constexpr const char* name = "char";
    using raw_type =
        std::conditional_t<std::is_signed<char>::value, long, unsigned long>;
};

template <>
struct numeric_type_traits<signed char>
{
    static constexpr const char* name = "signed char";
    using raw_type = long;
};

template <>
struct numeric_type_traits<unsigned char>
{
    static constexpr const char* name = "unsigned char";
    using raw_type = unsigned long;
};

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

template <class D, class H>
class raw_convert_base :
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
        using result_t = std::decay_t<decltype(r)>;

        const auto has_postfix =
            std::find_if(static_cast<const Ch*>(middle), end, [](Ch c) {
                return !is_space(c);
            }) != end;
        if (begin == middle) {
            // no conversion could be performed
            if (has_postfix) {
                return static_cast<result_t>(eh().invalid_format(begin, end));
            } else {
                // whitespace only
                return static_cast<result_t>(eh().empty());
            }
        } else if (has_postfix) {
            // if an not-whitespace-extra-character found, it is NG
            return static_cast<result_t>(eh().invalid_format(begin, end));
        } else if (errno == ERANGE) {
            return static_cast<result_t>(eh().out_of_range(begin, end, r));
        } else {
            return r;
        }
    }

protected:
    H& eh()
    {
        return this->get();
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

template <class T, class H>
struct raw_convert;

template <class H>
struct raw_convert<long, H> :
    raw_convert_base<raw_convert<long, H>, H>
{
    using raw_convert_base<raw_convert<long, H>, H>
        ::raw_convert_base;

    auto engine(const char* s, char** e) const
    {
        return std::strtol(s, e, 10);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return std::wcstol(s, e, 10);
    }
};

template <class H>
struct raw_convert<unsigned long, H> :
    raw_convert_base<raw_convert<unsigned long, H>, H>
{
    using raw_convert_base<raw_convert<unsigned long, H>, H>
        ::raw_convert_base;

    auto engine(const char* s, char** e) const
    {
        return std::strtoul(s, e, 10);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return std::wcstoul(s, e, 10);
    }
};

template <class H>
struct raw_convert<long long, H> :
    raw_convert_base<raw_convert<long long, H>, H>
{
    using raw_convert_base<raw_convert<long long, H>, H>
        ::raw_convert_base;

    auto engine(const char* s, char** e) const
    {
        return std::strtoll(s, e, 10);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return std::wcstoll(s, e, 10);
    }
};

template <class H>
struct raw_convert<unsigned long long, H> :
    raw_convert_base<raw_convert<unsigned long long, H>, H>
{
    using raw_convert_base<raw_convert<unsigned long long, H>, H>
        ::raw_convert_base;

    auto engine(const char* s, char** e) const
    {
        return std::strtoull(s, e, 10);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return std::wcstoull(s, e, 10);
    }
};

template <class H>
struct raw_convert<float, H> :
    raw_convert_base<raw_convert<float, H>, H>
{
    using raw_convert_base<raw_convert<float, H>, H>
        ::raw_convert_base;

    auto engine(const char* s, char** e) const
    {
        return std::strtof(s, e);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return std::wcstof(s, e);
    }
};

template <class H>
struct raw_convert<double, H> :
    raw_convert_base<raw_convert<double, H>, H>
{
    using raw_convert_base<raw_convert<double, H>, H>
        ::raw_convert_base;

    auto engine(const char* s, char** e) const
    {
        return std::strtod(s, e);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return std::wcstod(s, e);
    }
};

template <class H>
struct raw_convert<long double, H> :
    raw_convert_base<raw_convert<long double, H>, H>
{
    using raw_convert_base<raw_convert<long double, H>, H>
        ::raw_convert_base;

    auto engine(const char* s, char** e) const
    {
        return std::strtold(s, e);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return std::wcstold(s, e);
    }
};

template <class T, class H, class = void>
struct convert :
    private raw_convert<T, H>
{
    using raw_convert<T, H>::raw_convert;

    template <class Ch>
    auto operator()(const Ch* begin, const Ch* end)
    {
        return this->convert_raw(begin, end);
    }
};

template <class T, class H, class U, class = void>
struct restrained_convert :
    private raw_convert<U, H>
{
    using raw_convert<U, H>::raw_convert;

    template <class Ch>
    T operator()(const Ch* begin, const Ch* end)
    {
        const auto result = this->convert_raw(begin, end);
        if (result < std::numeric_limits<T>::lowest()) {
            return this->eh().
                out_of_range(begin, end, std::numeric_limits<T>::lowest());
        } else if (std::numeric_limits<T>::max() < result) {
            return this->eh().
                out_of_range(begin, end, std::numeric_limits<T>::max());
        } else {
            return static_cast<T>(result);
        }
    }
};

template <class T, class H, class U>
struct restrained_convert<T, H, U,
        std::enable_if_t<std::is_unsigned<T>::value>> :
    private raw_convert<U, H>
{
    using raw_convert<U, H>::raw_convert;

    template <class Ch>
    T operator()(const Ch* begin, const Ch* end)
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
        return this->eh().out_of_range(
            begin, end, std::numeric_limits<T>::max());
    }
};

template <class>
using void_t = void;

template <class T, class H>
struct convert<T, H,
    void_t<typename numeric_type_traits<T>::raw_type>> :
    restrained_convert<T, H, typename numeric_type_traits<T>::raw_type>
{
    using restrained_convert<T, H, typename numeric_type_traits<T>::raw_type>
        ::restrained_convert;
};

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

template <class T>
struct fail_if_conversion_failed
{
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

    template <class Ch, class U>
    [[noreturn]]
    T out_of_range(const Ch* begin, const Ch* end, U /*proposed*/) const
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
    static constexpr const char* name = detail::numeric_type_traits<T>::name;

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

template <class Ch, class Tr>
struct fail_if_conversion_failed<std::basic_string<Ch, Tr>>
{};


template <class T>
class replace_if_conversion_failed
{
    enum replacement : std::int_fast8_t
    {
        replacement_empty = 0,
        replacement_invalid_format = 1,
        replacement_above_upper_limit = 2,
        replacement_below_lower_limit = 3
    };

    std::aligned_storage_t<sizeof(T), alignof(T)> replacements_[4];
    std::int_fast8_t has_;

public:
    template <
        class Empty = std::nullptr_t,
        class InvalidFormat = std::nullptr_t,
        class AboveUpperLimit = std::nullptr_t,
        class BelowLowerLimit = std::nullptr_t>
    explicit replace_if_conversion_failed(
        Empty on_empty = Empty(),
        InvalidFormat on_invalid_format = InvalidFormat(),
        AboveUpperLimit on_above_upper_limit = AboveUpperLimit(),
        BelowLowerLimit on_below_lower_limit = BelowLowerLimit()) :
        has_(0)
    {
        // This class template is designed for arithmetic types
        static_assert(
            std::is_trivially_copy_constructible<T>::value
         && std::is_trivially_destructible<T>::value, "");

        set(replacement_empty, on_empty);
        set(replacement_invalid_format, on_invalid_format);
        set(replacement_above_upper_limit, on_above_upper_limit);
        set(replacement_below_lower_limit, on_below_lower_limit);
    }

    template <class Ch>
    T invalid_format(const Ch* begin, const Ch* end) const
    {
        if (const auto p = get(replacement_invalid_format)) {
            return *p;
        } else {
            return fail_if_conversion_failed<T>().invalid_format(begin, end);
        }
    }

    template <class Ch, class U>
    T out_of_range(const Ch* begin, const Ch* end, U proposed) const
    {
        using limits_t = std::numeric_limits<T>;
        if (proposed >= limits_t::max()) {
            if (const auto p = get(replacement_above_upper_limit)) {
                return *p;
            }
        } else if (const auto p = get(replacement_below_lower_limit)) {
            return *p;
        }
        return fail_if_conversion_failed<T>()
            .out_of_range(begin, end, proposed);
    }

    T empty() const
    {
        if (const auto p = get(replacement_empty)) {
            return *p;
        } else {
            return fail_if_conversion_failed<T>().empty();
        }
    }

private:
    void set(replacement r, const T& value)
    {
        new (static_cast<void*>(&replacements_[r])) T(value);
        has_ |= 1 << r;
    }

    void set(replacement, std::nullptr_t)
    {}

    const T* get(replacement r) const
    {
        if (has_ & (1 << r)) {
            return static_cast<const T*>(
                static_cast<const void*>(&replacements_[r]));
        } else {
            return nullptr;
        }
    }
};

template <class Ch, class Tr>
struct replace_if_conversion_failed<std::basic_string<Ch, Tr>>
{};

template <class T, class OutputIterator,
    class SkippingHandler = fail_if_skipped<T>,
    class ConversionErrorHandler = fail_if_conversion_failed<T>>
class field_translator :
    detail::convert<T, ConversionErrorHandler>,
    public detail::skipping_handler_holder<SkippingHandler>
{
    OutputIterator out_;

public:
    explicit field_translator(
        OutputIterator out,
        SkippingHandler handle_skipping = SkippingHandler(),
        ConversionErrorHandler handle_conversion_error
            = ConversionErrorHandler()) :
        detail::convert<T, ConversionErrorHandler>(
            std::move(handle_conversion_error)),
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
    void field_value(std::basic_string<Ch, Tr, Alloc>&& value)
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

template <class Ch, class Tr, class Alloc, class OutputIterator,
          class SkippingHandler, class ConversionErrorHandler>
class field_translator<std::basic_string<Ch, Tr, Alloc>,
    OutputIterator, SkippingHandler, ConversionErrorHandler> :
    public detail::skipping_handler_holder<SkippingHandler>
{
    OutputIterator out_;

public:
    explicit field_translator(
        OutputIterator out,
        SkippingHandler handle_skipping = SkippingHandler(),
        ConversionErrorHandler = ConversionErrorHandler()) :
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
    class SkippingHandler = fail_if_skipped<T>,
    class ConversionErrorHandler = fail_if_conversion_failed<T>>
auto make_field_translator(
    OutputIterator out,
    SkippingHandler handle_skipping = SkippingHandler(),
    ConversionErrorHandler handle_conversion_error = ConversionErrorHandler())
{
    return field_translator<T, OutputIterator,
            SkippingHandler, ConversionErrorHandler>(
        std::move(out),
        std::move(handle_skipping), std::move(handle_conversion_error));
}

namespace detail {

template <
    class Container,
    class SkippingHandler,
    class ConversionErrorHandler,
    std::enable_if_t<
        !is_back_insertable<Container>::value>* = nullptr>
auto make_field_translator_c_impl(
    Container& values,
    SkippingHandler&& handle_skipping,
    ConversionErrorHandler&& handle_conversion_error)
{
    return make_field_translator<typename Container::value_type>(
        std::inserter(values, values.end()),
        std::forward<SkippingHandler>(handle_skipping),
        std::forward<ConversionErrorHandler>(handle_conversion_error));
}

template <
    class Container,
    class SkippingHandler,
    class ConversionErrorHandler,
    std::enable_if_t<
        is_back_insertable<Container>::value>* = nullptr>
auto make_field_translator_c_impl(
    Container& values,
    SkippingHandler&& handle_skipping,
    ConversionErrorHandler&& handle_conversion_error)
{
    return make_field_translator<typename Container::value_type>(
        std::back_inserter(values),
        std::forward<SkippingHandler>(handle_skipping),
        std::forward<ConversionErrorHandler>(handle_conversion_error));
}

}

template <
    class Container,
    class SkippingHandler =
        fail_if_skipped<typename Container::value_type>,
    class ConversionErrorHandler =
        fail_if_conversion_failed<typename Container::value_type>>
auto make_field_translator_c(
    Container& values,
    SkippingHandler handle_skipping = SkippingHandler(),
    ConversionErrorHandler handle_conversion_error = ConversionErrorHandler())
{
    return detail::make_field_translator_c_impl(
        values,
        std::move(handle_skipping), std::move(handle_conversion_error));
}

}}

#endif
