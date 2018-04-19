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
#include <clocale>
#include <cstddef>
#include <cstdint>
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

#include "allocation_only_allocator.hpp"
#include "csv_error.hpp"
#include "member_like_base.hpp"
#include "typing_aid.hpp"

namespace furfurylic {
namespace commata {

namespace detail {

struct accepts_range_impl
{
    template <class T, class Ch>
    static auto check(T*) -> decltype(
        std::declval<T&>().field_value(
            static_cast<Ch*>(nullptr), static_cast<Ch*>(nullptr)),
        std::true_type());

    template <class T, class Ch>
    static auto check(...) -> std::false_type;
};

template <class T, class Ch>
struct accepts_range :
    decltype(accepts_range_impl::check<T, Ch>(nullptr))
{};

struct accepts_x_impl
{
    template <class T, class X>
    static auto check(T*) -> decltype(
        std::declval<T&>().field_value(std::declval<X>()),
        std::true_type());

    template <class T, class Ch>
    static auto check(...) -> std::false_type;
};

template <class T, class X>
struct accepts_x :
    decltype(accepts_x_impl::check<T, X>(nullptr))
{};

}

template <class Ch, class Tr = std::char_traits<Ch>,
          class Alloc = std::allocator<Ch>>
class csv_scanner :
    detail::member_like_base<Alloc>
{
    using string_t = std::basic_string<Ch, Tr, Alloc>;

    struct field_scanner
    {
        virtual ~field_scanner() {}
        virtual void field_value(Ch* begin, Ch* end, csv_scanner& me) = 0;
        virtual void field_value(string_t&& value, csv_scanner& me) = 0;
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
                me.destroy_deallocate(me.header_field_scanner_);
                me.header_field_scanner_ = nullptr;
            }
        }

        void field_value(string_t&& value, csv_scanner& me) override
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

        void field_value(Ch* begin, Ch* end, csv_scanner& me) override
        {
            field_value_r(
                typename detail::accepts_range<FieldScanner, Ch>(),
                begin, end, me);
        }

        void field_value(string_t&& value, csv_scanner& me) override
        {
            field_value_s(
                typename detail::accepts_x<FieldScanner, string_t>(),
                std::move(value), me);
        }

        void field_skipped() override
        {
            scanner_.field_skipped();
        }

        const std::type_info& get_type() const override
        {
            // should return the static type in which this instance was
            // created (not the dynamic type)
            return typeid(FieldScanner);
        }

    private:
        void field_value_r(std::true_type, Ch* begin, Ch* end, csv_scanner&)
        {
            scanner_.field_value(begin, end);
        }

        void field_value_r(std::false_type,
            Ch* begin, Ch* end, csv_scanner& me)
        {
            scanner_.field_value(string_t(begin, end, me.get_allocator()));
        }

        void field_value_s(std::true_type, string_t&& value, csv_scanner&)
        {
            scanner_.field_value(std::move(value));
        }

        void field_value_s(std::false_type, string_t&& value, csv_scanner&)
        {
            scanner_.field_value(
                &*value.begin(), &*value.begin() + value.size());
        }

        const void* get_target_v() const override
        {
            return &scanner_;
        }

        void* get_target_v() override
        {
            return &scanner_;
        }
    };

    using at_t = std::allocator_traits<Alloc>;
    using hfs_at_t =
        typename at_t::template rebind_traits<header_field_scanner>;
    using bfs_at_t =
        typename at_t::template rebind_traits<body_field_scanner>;
    using bfs_ptr_a_t =
        typename at_t::template rebind_alloc<typename bfs_at_t::pointer>;

    bool in_header_;
    std::size_t j_;
    std::size_t buffer_size_;
    typename at_t::pointer buffer_;
    const Ch* begin_;
    const Ch* end_;
    typename hfs_at_t::pointer header_field_scanner_;
    std::vector<typename bfs_at_t::pointer,
        detail::allocation_only_allocator<bfs_ptr_a_t>> scanners_;
    string_t fragmented_value_;

public:
    using char_type = Ch;
    using allocator_type = Alloc;
    using size_type = typename std::allocator_traits<Alloc>::size_type;

    explicit csv_scanner(
        bool has_header = false,
        size_type buffer_size = default_buffer_size) :
        csv_scanner(std::allocator_arg, Alloc(), has_header, buffer_size)
    {}

    template <
        class HeaderFieldScanner,
        class = std::enable_if_t<!std::is_integral<HeaderFieldScanner>::value>>
    explicit csv_scanner(
        HeaderFieldScanner s,
        size_type buffer_size = default_buffer_size) :
        csv_scanner(std::allocator_arg, Alloc(), std::move(s), buffer_size)
    {}

    csv_scanner(
        std::allocator_arg_t, const Alloc& alloc,
        bool has_header = false,
        size_type buffer_size = default_buffer_size) :
        detail::member_like_base<Alloc>(alloc),
        in_header_(has_header), j_(0),
        buffer_size_(sanitize_buffer_size(buffer_size)),
        buffer_(), begin_(nullptr),
        header_field_scanner_(),
        scanners_(detail::allocation_only_allocator<bfs_ptr_a_t>(
            bfs_ptr_a_t(this->get()))),
        fragmented_value_(alloc)
    {}

    template <
        class HeaderFieldScanner,
        class = std::enable_if_t<!std::is_integral<HeaderFieldScanner>::value>>
    csv_scanner(
        std::allocator_arg_t, const Alloc& alloc,
        HeaderFieldScanner s,
        size_type buffer_size = default_buffer_size) :
        detail::member_like_base<Alloc>(alloc),
        in_header_(true), j_(0),
        buffer_size_(sanitize_buffer_size(buffer_size)),
        buffer_(), begin_(nullptr),
        header_field_scanner_(allocate_construct<
            typed_header_field_scanner<HeaderFieldScanner>>(std::move(s))),
        scanners_(detail::allocation_only_allocator<bfs_ptr_a_t>(
            bfs_ptr_a_t(this->get()))),
        fragmented_value_(alloc)
    {}

    csv_scanner(csv_scanner&& other) noexcept(noexcept(
        std::is_nothrow_move_constructible<decltype(scanners_)>::value
     && std::is_nothrow_move_constructible<string_t>::value)) :
        detail::member_like_base<Alloc>(std::move(other)),
        in_header_(other.in_header_), j_(other.j_),
        buffer_size_(other.buffer_size_),
        buffer_(other.buffer_), begin_(other.begin_), end_(other.end_),
        header_field_scanner_(other.header_field_scanner_),
        scanners_(std::move(other.scanners_)),
        fragmented_value_(std::move(other.fragmented_value_))
    {
        other.buffer_ = nullptr;
        other.header_field_scanner_ = nullptr;
    }

    ~csv_scanner()
    {
        if (buffer_) {
            at_t::deallocate(this->get(), buffer_, buffer_size_);
        }
        if (header_field_scanner_) {
            destroy_deallocate(header_field_scanner_);
        }
        for (const auto p : scanners_) {
            if (p) {
                destroy_deallocate(p);
            }
        }
    }

    allocator_type get_allocator() const noexcept
    {
        return this->get();
    }

    template <class FieldScanner = std::nullptr_t>
    void set_field_scanner(std::size_t j, FieldScanner s = FieldScanner())
    {
        do_set_field_scanner(j, std::move(s));
    }

private:
    template <class FieldScanner>
    void do_set_field_scanner(std::size_t j, FieldScanner s)
    {
        using scanner_t = typed_body_field_scanner<FieldScanner>;
        if (j >= scanners_.size()) {
            scanners_.resize(j + 1);                            // throw
        } else if (const auto scanner = scanners_[j]) {
            destroy_deallocate(scanner);
        }
        scanners_[j] =
            allocate_construct<scanner_t>(std::move(s));        // throw
    }

    void do_set_field_scanner(std::size_t j, std::nullptr_t)
    {
        if (j < scanners_.size()) {
            auto& scanner = scanners_[j];
            destroy_deallocate(scanner);
            scanner = nullptr;
        }
    }

public:
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
            buffer_ = at_t::allocate(
                this->get(), buffer_size_);         // throw
        } else if (begin_) {
            fragmented_value_.assign(begin_, end_); // throw
            begin_ = nullptr;
        }
        return { true_buffer(), static_cast<std::size_t>(buffer_size_ - 1) };
        // We'd like to push buffer_[buffer_size_] with '\0' on EOF
        // so we tell the driver that the buffer size is smaller by one
    }

    void release_buffer(const Ch*)
    {}

    void start_record(const Ch* /*record_begin*/)
    {}

    bool update(const Ch* first, const Ch* last)
    {
        if (get_scanner() && (first != last)) {
            if (begin_) {
                assert(fragmented_value_.empty());
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
                destroy_deallocate(header_field_scanner_);
                header_field_scanner_ = nullptr;
            }
            in_header_ = false;
        } else {
            for (auto j = j_; j < scanners_.size(); ++j) {
                if (auto scanner = scanners_[j]) {
                    scanner->field_skipped();
                }
            }
        }
        j_ = 0;
        return true;
    }

private:
    static constexpr size_type default_buffer_size =
        std::min(std::numeric_limits<size_type>::max(),
            static_cast<size_type>(8192U));

    size_type sanitize_buffer_size(size_type buffer_size)
    {
        return std::min(
            std::max(buffer_size, static_cast<size_type>(2U)),
            at_t::max_size(this->get()));
    }

    template <class T, class... Args>
    auto allocate_construct(Args&&... args)
    {
        using t_alloc_traits_t =
            typename at_t::template rebind_traits<T>;
        typename t_alloc_traits_t::allocator_type a(this->get());
        const auto p = t_alloc_traits_t::allocate(a, 1);    // throw
        try {
            ::new(std::addressof(*p))
                T(std::forward<Args>(args)...);             // throw
        } catch (...) {
            a.deallocate(p, 1);
            throw;
        }
        return p;
    }

    template <class T>
    void destroy_deallocate(T* p)
    {
        assert(p);
        using t_at_t = typename at_t::template rebind_traits<T>;
        typename t_at_t::allocator_type a(this->get());
        std::addressof(*p)->~T();
        t_at_t::deallocate(a, p, 1);
    }

    Ch* true_buffer() const noexcept
    {
        assert(buffer_);
        return std::addressof(*buffer_);
    }

    field_scanner* get_scanner()
    {
        const auto true_addressof = [](auto p) {
            return p ? std::addressof(*p) : nullptr;
        };
        if (in_header_) {
            return true_addressof(header_field_scanner_);
        } else if (j_ < scanners_.size()) {
            return true_addressof(scanners_[j_]);
        } else {
            return nullptr;
        }
    }

    Ch* unconst(const Ch* s) const
    {
        const auto tb = true_buffer();
        return tb + (s - tb);
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
    static constexpr const auto strto = std::strtol;
    static constexpr const auto wcsto = std::wcstol;
};

template <>
struct numeric_type_traits<unsigned long>
{
    static constexpr const char* name = "unsigned long int";
    static constexpr const auto strto = std::strtoul;
    static constexpr const auto wcsto = std::wcstoul;
};

template <>
struct numeric_type_traits<long long>
{
    static constexpr const char* name = "long long int";
    static constexpr const auto strto = std::strtoll;
    static constexpr const auto wcsto = std::wcstoll;
};

template <>
struct numeric_type_traits<unsigned long long>
{
    static constexpr const char* name = "unsigned long long int";
    static constexpr const auto strto = std::strtoull;
    static constexpr const auto wcsto = std::wcstoull;
};

template <>
struct numeric_type_traits<float>
{
    static constexpr const char* name = "float";
    static constexpr const auto strto = std::strtof;
    static constexpr const auto wcsto = std::wcstof;
};

template <>
struct numeric_type_traits<double>
{
    static constexpr const char* name = "double";
    static constexpr const auto strto = std::strtod;
    static constexpr const auto wcsto = std::wcstod;
};

template <>
struct numeric_type_traits<long double>
{
    static constexpr const char* name = "long double";
    static constexpr const auto strto = std::strtold;
    static constexpr const auto wcsto = std::wcstold;
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
        using result_t = std::remove_const_t<decltype(r)>;

        const auto has_postfix =
            std::find_if<const Ch*>(middle, end, [](Ch c) {
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
            // if a not-whitespace-extra-character found, it is NG
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

template <class>
using void_t = void;

template <class T, class H, class = void>
struct raw_convert;

template <class T, class H>
struct raw_convert<T, H, std::enable_if_t<std::is_integral<T>::value,
        void_t<decltype(numeric_type_traits<T>::strto)>>> :
    raw_convert_base<raw_convert<T, H>, H>
{
    using raw_convert_base<raw_convert<T, H>, H>
        ::raw_convert_base;

    auto engine(const char* s, char** e) const
    {
        return numeric_type_traits<T>::strto(s, e, 10);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return numeric_type_traits<T>::wcsto(s, e, 10);
    }
};

template <class T, class H>
struct raw_convert<T, H, std::enable_if_t<std::is_floating_point<T>::value,
        void_t<decltype(numeric_type_traits<T>::strto)>>> :
    raw_convert_base<raw_convert<T, H>, H>
{
    using raw_convert_base<raw_convert<T, H>, H>
        ::raw_convert_base;

    auto engine(const char* s, char** e) const
    {
        return numeric_type_traits<T>::strto(s, e);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return numeric_type_traits<T>::wcsto(s, e);
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

template <class T, class H>
struct convert<T, H, void_t<typename numeric_type_traits<T>::raw_type>> :
    restrained_convert<T, H, typename numeric_type_traits<T>::raw_type>
{
    using restrained_convert<T, H, typename numeric_type_traits<T>::raw_type>
        ::restrained_convert;
};

} // end namespace detail

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
            std::use_facet<std::ctype<Ch>>(std::locale());  // current global
        std::transform(begin, end, std::ostreambuf_iterator<char>(os),
            [&facet](Ch c) {
                // map NUL to '?' to prevent exception::what()
                // from ending at that NUL
                return (c == Ch()) ? '?' : facet.narrow(c, '?');
            });
    }

    // Overloaded to avoid unneeded access to the global locale
    static void narrow(std::ostream& os, const char* begin, const char* end)
    {
        std::transform(begin, end, std::ostreambuf_iterator<char>(os),
            [](char c) {
                // ditto
                return (c == char()) ? '?' : c;
            });
    }
};

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

namespace detail {

template <class OutputIterator, class SkippingHandler>
class translator :
    member_like_base<SkippingHandler>
{
    OutputIterator out_;

public:
    translator(OutputIterator out, SkippingHandler handle_skipping) :
        member_like_base<SkippingHandler>(std::move(handle_skipping)),
        out_(std::move(out))
    {}

    const SkippingHandler& get_skipping_handler() const
    {
        return this->get();
    }

    SkippingHandler& get_skipping_handler()
    {
        return this->get();
    }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4702)
#endif
    void field_skipped()
    {
        put(get_skipping_handler().skipped());
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    }

    template <class T>
    void put(T&& value)
    {
        *out_ = std::forward<T>(value);
        ++out_;
    }
};

} // end namespace detail

template <class T, class OutputIterator,
    class SkippingHandler = fail_if_skipped<T>,
    class ConversionErrorHandler = fail_if_conversion_failed<T>>
class numeric_field_translator :
    detail::convert<T, ConversionErrorHandler>,
    detail::translator<OutputIterator, SkippingHandler>
{
public:
    explicit numeric_field_translator(
        OutputIterator out,
        SkippingHandler handle_skipping = SkippingHandler(),
        ConversionErrorHandler handle_conversion_error
            = ConversionErrorHandler()) :
        detail::convert<T, ConversionErrorHandler>(
            std::move(handle_conversion_error)),
        detail::translator<OutputIterator, SkippingHandler>(
            std::move(out), std::move(handle_skipping))
    {}

    template <class Ch>
    void field_value(const Ch* begin, const Ch* end)
    {
        assert(*end == Ch());   // shall be null-teminated
        this->put((*this)(begin, end));
    }

    using detail::translator<OutputIterator, SkippingHandler>::
        get_skipping_handler;
    using detail::translator<OutputIterator, SkippingHandler>::field_skipped;
};

template <class T, class OutputIterator, class Ch,
    class SkippingHandler = fail_if_skipped<T>,
    class ConversionErrorHandler = fail_if_conversion_failed<T>>
class locale_based_numeric_field_translator :
    detail::convert<T, ConversionErrorHandler>,
    detail::translator<OutputIterator, SkippingHandler>
{
    Ch thousands_sep_;      // of specified loc in the ctor
    Ch decimal_point_;      // of specified loc in the ctor
    Ch decimal_point_c_;    // of C's global
                            // to mimic std::strtol and its comrades;
                            // initialized after parsing has started

public:
    locale_based_numeric_field_translator(
        OutputIterator out, const std::locale& loc,
        SkippingHandler handle_skipping = SkippingHandler(),
        ConversionErrorHandler handle_conversion_error
            = ConversionErrorHandler()) :
        detail::convert<T, ConversionErrorHandler>(
            std::move(handle_conversion_error)),
        detail::translator<OutputIterator, SkippingHandler>(
            std::move(out), std::move(handle_skipping)),
        decimal_point_c_(Ch())
    {
        const auto& facet = std::use_facet<std::numpunct<Ch>>(loc);
        thousands_sep_ = facet.grouping().empty() ?
            Ch() : facet.thousands_sep();
        decimal_point_ = facet.decimal_point();
    }

    void field_value(Ch* begin, Ch* end)
    {
        if (decimal_point_c_ == Ch()) {
            decimal_point_c_ = widen(*std::localeconv()->decimal_point, Ch());
        }
        assert(*end == Ch());   // shall be null-terminated
        bool decimal_point_appeared = false;
        Ch* head = begin;
        for (Ch* b = begin; b != end; ++b) {
            Ch c = *b;
            assert(c != Ch());
            if (c == decimal_point_) {
                if (!decimal_point_appeared) {
                    c = decimal_point_c_;
                    decimal_point_appeared = true;
                }
            } else if (c == thousands_sep_) {
                continue;
            }
            *head = c;
            ++head;
        }
        *head = Ch();
        this->put((*this)(begin, head));
    }

    using detail::translator<OutputIterator, SkippingHandler>::
        get_skipping_handler;
    using detail::translator<OutputIterator, SkippingHandler>::field_skipped;

private:
    static char widen(char c, char)
    {
        return c;
    }

    static wchar_t widen(char c, wchar_t)
    {
        return std::btowc(static_cast<unsigned char>(c));
    }
};

template <class OutputIterator, class Ch,
    class Tr = std::char_traits<Ch>, class Alloc = std::allocator<Ch>,
    class SkippingHandler = fail_if_skipped<std::basic_string<Ch, Tr, Alloc>>>
class string_field_translator :
    detail::member_like_base<Alloc>,
    detail::translator<OutputIterator, SkippingHandler>
{
public:
    using allocator_type = Alloc;

    explicit string_field_translator(
        OutputIterator out,
        SkippingHandler handle_skipping = SkippingHandler()) :
        string_field_translator(
            std::allocator_arg, Alloc(), out, std::move(handle_skipping))
    {}

    string_field_translator(
        std::allocator_arg_t, const Alloc& alloc, OutputIterator out,
        SkippingHandler handle_skipping = SkippingHandler()) :
        detail::member_like_base<Alloc>(alloc),
        detail::translator<OutputIterator, SkippingHandler>(
            std::move(out), std::move(handle_skipping))
    {}

    allocator_type get_allocator() const
    {
        return detail::member_like_base<Alloc>::get();
    }

    void field_value(Ch* begin, Ch* end)
    {
        this->put(std::basic_string<Ch, Tr, Alloc>(
            begin, end, get_allocator()));
    }

    void field_value(std::basic_string<Ch, Tr, Alloc>&& value)
    {
        if (value.get_allocator() == get_allocator()) {
            this->put(std::move(value));
        } else {
            this->field_value(&value[0], &value[0] + value.size());
        }
    }

    using detail::translator<OutputIterator, SkippingHandler>::
        get_skipping_handler;
    using detail::translator<OutputIterator, SkippingHandler>::field_skipped;
};

namespace detail {

template <class... Ts>
struct first;

template <>
struct first<>
{
    using type = void;
};

template <class Head, class... Tail>
struct first<Head, Tail...>
{
    using type = Head;
};

template <class... Ts>
using first_t = typename first<Ts...>::type;

} // end namespace detail

template <class T, class OutputIterator, class... Appendices,
    std::enable_if_t<
        !detail::is_std_string<T>::value
     && !std::is_same<
            std::decay_t<detail::first_t<Appendices...>>,
            std::locale>::value,
        std::nullptr_t> = nullptr>
auto make_field_translator(OutputIterator out, Appendices&&... appendices)
{
    return numeric_field_translator<T, OutputIterator, Appendices...>(
        std::move(out), std::forward<Appendices>(appendices)...);
}

template <class T, class Ch, class OutputIterator, class... Appendices,
    std::enable_if_t<!detail::is_std_string<T>::value, std::nullptr_t>
        = nullptr>
auto make_field_translator(
    OutputIterator out, const std::locale& loc, Ch, Appendices&&... appendices)
{
    return locale_based_numeric_field_translator<
                T, OutputIterator, Ch, Appendices...>(
            std::move(out), loc, std::forward<Appendices>(appendices)...);
}

template <class T, class OutputIterator, class... Appendices,
    std::enable_if_t<detail::is_std_string<T>::value, std::nullptr_t>
        = nullptr>
auto make_field_translator(OutputIterator out, Appendices&&... appendices)
{
    return string_field_translator<OutputIterator,
            typename T::value_type, typename T::traits_type,
            typename T::allocator_type, Appendices...>(
        std::move(out), std::forward<Appendices>(appendices)...);
}

template <class T, class Allocator, class OutputIterator, class... Appendices,
    std::enable_if_t<detail::is_std_string<T>::value, std::nullptr_t>
        = nullptr>
auto make_field_translator(std::allocator_arg_t, const Allocator& alloc,
        OutputIterator out, Appendices&&... appendices)
{
    return string_field_translator<OutputIterator,
            typename T::value_type, typename T::traits_type,
            Allocator, Appendices...>(
        std::allocator_arg, alloc,
        std::move(out), std::forward<Appendices>(appendices)...);
}

namespace detail {

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

template <class Container>
auto make_back_insert_iterator(Container& c, std::true_type)
{
    return std::back_inserter(c);
}

template <class Container>
auto make_back_insert_iterator(Container& c, std::false_type)
{
    return std::inserter(c, c.end());
}

} // end namespace detail

template <class Container, class... Appendices,
    std::enable_if_t<
        !std::is_same<std::allocator_arg_t, std::decay_t<Container>>::value,
        std::nullptr_t> = nullptr>
auto make_field_translator_c(Container& values, Appendices&&... appendices)
{
    return make_field_translator<typename Container::value_type>(
        detail::make_back_insert_iterator(
            values, detail::is_back_insertable<Container>()),
        std::forward<Appendices>(appendices)...);
}

template <class Allocator, class Container, class... Appendices>
auto make_field_translator_c(std::allocator_arg_t, const Allocator& alloc,
    Container& values, Appendices&&... appendices)
{
    return make_field_translator<typename Container::value_type>(
        std::allocator_arg, alloc,
        detail::make_back_insert_iterator(
            values, detail::is_back_insertable<Container>()),
        std::forward<Appendices>(appendices)...);
}

}}

#endif
