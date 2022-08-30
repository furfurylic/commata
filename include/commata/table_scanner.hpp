/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_555AFEA0_404A_4BF3_AF38_4F653A0B76E6
#define COMMATA_GUARD_555AFEA0_404A_4BF3_AF38_4F653A0B76E6

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <clocale>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <iterator>
#include <limits>
#include <locale>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <vector>

#include "allocation_only_allocator.hpp"
#include "buffer_size.hpp"
#include "text_error.hpp"
#include "member_like_base.hpp"
#include "typing_aid.hpp"
#include "write_ntmbs.hpp"

namespace commata {

namespace detail::scanner {

struct typable
{
    virtual ~typable() {}
    virtual const std::type_info& get_type() const = 0;

    template <class T>
    const T* get_target() const noexcept
    {
        return (get_type() == typeid(T)) ?
            static_cast<const T*>(get_target_v()) : nullptr;
    }

    template <class T>
    T* get_target() noexcept
    {
        return (get_type() == typeid(T)) ?
            static_cast<T*>(get_target_v()) : nullptr;
    }

private:
    virtual const void* get_target_v() const = 0;
    virtual void* get_target_v() = 0;
};

class counting_header_field_scanner
{
    std::size_t remaining_header_records_;

public:
    counting_header_field_scanner(std::size_t header_record_count) :
        remaining_header_records_(header_record_count)
    {
        assert(header_record_count > 0);
    }

    template <class Ch, class TableScanner>
    bool operator()(std::size_t, const std::pair<Ch*, Ch*>* v, TableScanner&)
    {
        return v || (--remaining_header_records_ > 0);
    }
};

} // end detail::scanner

template <class Ch, class Tr = std::char_traits<Ch>,
          class Allocator = std::allocator<Ch>>
class basic_table_scanner
{
    using string_t = std::basic_string<Ch, Tr, Allocator>;

    struct field_scanner
    {
        virtual ~field_scanner() {}
        virtual void field_value(
            Ch* begin, Ch* end, basic_table_scanner& me) = 0;
        virtual void field_value(
            string_t&& value, basic_table_scanner& me) = 0;
    };

    struct header_field_scanner : field_scanner
    {
        virtual void so_much_for_header(basic_table_scanner& me) = 0;
    };

    template <class HeaderScanner>
    class typed_header_field_scanner :
        public header_field_scanner, detail::member_like_base<HeaderScanner>
    {
        using range_t = std::pair<Ch*, Ch*>;

    public:
        template <class T>
        explicit typed_header_field_scanner(T&& s) :
            detail::member_like_base<HeaderScanner>(std::forward<T>(s))
        {}

        typed_header_field_scanner(typed_header_field_scanner&&) = delete;

        void field_value(Ch* begin, Ch* end, basic_table_scanner& me) override
        {
            const range_t range(begin, end);
            if (!scanner()(me.j_, std::addressof(range), me)) {
                me.remove_header_field_scanner(false);
            }
        }

        void field_value(string_t&& value, basic_table_scanner& me) override
        {
            field_value(value.data(), value.data() + value.size(), me);
        }

        void so_much_for_header(basic_table_scanner& me) override
        {
            if (!scanner()(me.j_, static_cast<const range_t*>(nullptr), me)) {
                me.remove_header_field_scanner(true);
            }
        }

    private:
        HeaderScanner& scanner()
        {
            return this->get();
        }
    };

    struct body_field_scanner : field_scanner, detail::scanner::typable
    {
        virtual void field_skipped() = 0;
    };

    template <class FieldScanner>
    struct typed_body_field_scanner :
        body_field_scanner, private detail::member_like_base<FieldScanner>
    {
        template <class T>
        explicit typed_body_field_scanner(T&& s) :
            detail::member_like_base<FieldScanner>(std::forward<T>(s))
        {}

        typed_body_field_scanner(typed_body_field_scanner&&) = delete;

        void field_value(Ch* begin, Ch* end,
            [[maybe_unused]] basic_table_scanner& me) override
        {
            if constexpr (std::is_invocable_v<FieldScanner&, Ch*, Ch*>) {
                scanner()(begin, end);
            } else {
                scanner()(string_t(begin, end, me.get_allocator()));
            }
        }

        void field_value(string_t&& value, basic_table_scanner&) override
        {
            if constexpr (std::is_invocable_v<FieldScanner&, string_t&&>) {
                scanner()(std::move(value));
            } else {
                scanner()(value.data(), value.data() + value.size());
            }
        }

        void field_skipped() override
        {
            scanner()();
        }

        const std::type_info& get_type() const noexcept override
        {
            // Static types suffice for we do slicing on construction
            return typeid(FieldScanner);
        }

    private:
        const void* get_target_v() const noexcept override
        {
            return std::addressof(this->get());
        }

        void* get_target_v() noexcept override
        {
            return std::addressof(this->get());
        }

        decltype(auto) scanner() noexcept
        {
            return this->get();
        }
    };

    struct record_end_scanner : detail::scanner::typable
    {
        virtual bool end_record(basic_table_scanner& me) = 0;
    };

    template <class T>
    struct typed_record_end_scanner :
        record_end_scanner, private detail::member_like_base<T>
    {
        template <class U>
        explicit typed_record_end_scanner(U&& scanner) :
            detail::member_like_base<T>(std::forward<U>(scanner))
        {}

        typed_record_end_scanner(typed_record_end_scanner&&) = delete;

        bool end_record(basic_table_scanner& me) override
        {
            auto& f = this->get();
            if constexpr (std::is_invocable_v<T&, basic_table_scanner&>) {
                if constexpr (std::is_void_v<decltype(f(me))>) {
                    f(me);
                    return true;
                } else {
                    return f(me);
                }
            } else {
                if constexpr (std::is_void_v<decltype(f())>) {
                    f();
                    return true;
                } else {
                    return f();
                }
            }
        }

        const std::type_info& get_type() const noexcept override
        {
            return typeid(T);
        }

    private:
        const void* get_target_v() const noexcept override
        {
            return std::addressof(this->get());
        }

        void* get_target_v() noexcept override
        {
            return std::addressof(this->get());
        }
    };

    using at_t = std::allocator_traits<Allocator>;
    using hfs_at_t =
        typename at_t::template rebind_traits<header_field_scanner>;
    using bfs_at_t =
        typename at_t::template rebind_traits<body_field_scanner>;
    using bfs_ptr_t = typename bfs_at_t::pointer;
    using bfs_ptr_p_t = typename std::pair<bfs_ptr_t, std::size_t>;
    using bfs_ptr_p_a_t = typename at_t::template rebind_alloc<bfs_ptr_p_t>;
    using scanners_a_t = detail::allocation_only_allocator<bfs_ptr_p_a_t>;
    using res_at_t = typename at_t::template rebind_traits<record_end_scanner>;

    std::size_t j_;
    std::size_t buffer_size_;
    typename at_t::pointer buffer_;
    const Ch* begin_;
    const Ch* end_;
    string_t value_;
    typename hfs_at_t::pointer header_field_scanner_;
    std::vector<bfs_ptr_p_t, scanners_a_t> scanners_;
    typename std::vector<bfs_ptr_p_t, scanners_a_t>::size_type sj_;
        // possibly active scanner: can't be an iterator
        // because it would be invalidated by header scanners
    typename res_at_t::pointer end_scanner_;

public:
    using char_type = Ch;
    using traits_type = Tr;
    using allocator_type = Allocator;
    using size_type = typename std::allocator_traits<Allocator>::size_type;

    explicit basic_table_scanner(
        std::size_t header_record_count = 0U,
        size_type buffer_size = 0U) :
        basic_table_scanner(std::allocator_arg, Allocator(),
            header_record_count, buffer_size)
    {}

    template <class HeaderFieldScanner,
        std::enable_if_t<!std::is_integral_v<HeaderFieldScanner>>* = nullptr>
    explicit basic_table_scanner(
        HeaderFieldScanner s,
        size_type buffer_size = 0U) :
        basic_table_scanner(
            std::allocator_arg, Allocator(), std::move(s), buffer_size)
    {}

    basic_table_scanner(
        std::allocator_arg_t, const Allocator& alloc,
        std::size_t header_record_count = 0U,
        size_type buffer_size = 0U) :
        buffer_size_(sanitize_buffer_size(buffer_size)), buffer_(),
        begin_(nullptr), value_(alloc),
        header_field_scanner_(
            (header_record_count > 0) ?
            allocate_construct<
                typed_header_field_scanner<
                    detail::scanner::counting_header_field_scanner>>(
                        header_record_count) :
            nullptr),
        scanners_(scanners_a_t(bfs_ptr_p_a_t(alloc))),
        end_scanner_(nullptr)
    {}

    template <class HeaderFieldScanner,
        std::enable_if_t<
            !std::is_integral_v<std::decay_t<HeaderFieldScanner>>>* = nullptr>
    basic_table_scanner(
        std::allocator_arg_t, const Allocator& alloc,
        HeaderFieldScanner&& s,
        size_type buffer_size = 0U) :
        buffer_size_(sanitize_buffer_size(buffer_size)),
        buffer_(), begin_(nullptr), value_(alloc),
        header_field_scanner_(allocate_construct<
            typed_header_field_scanner<std::decay_t<HeaderFieldScanner>>>(
                std::forward<HeaderFieldScanner>(s))),
        scanners_(scanners_a_t(bfs_ptr_p_a_t(alloc))), end_scanner_(nullptr)
    {}

    basic_table_scanner(basic_table_scanner&& other) noexcept :
        buffer_size_(other.buffer_size_),
        buffer_(std::exchange(other.buffer_, nullptr)),
        begin_(other.begin_), end_(other.end_),
        value_(std::move(other.value_)),
        header_field_scanner_(std::exchange(other.header_field_scanner_,
                                            nullptr)),
        scanners_(std::move(other.scanners_)),
        end_scanner_(std::exchange(other.end_scanner_, nullptr))
    {}

    ~basic_table_scanner()
    {
        auto a = get_allocator();
        if (buffer_) {
            at_t::deallocate(a, buffer_, buffer_size_);
        }
        if (header_field_scanner_) {
            destroy_deallocate(header_field_scanner_);
        }
        for (const auto& p : scanners_) {
            destroy_deallocate(p.first);
        }
        if (end_scanner_) {
            destroy_deallocate(end_scanner_);
        }
    }

    allocator_type get_allocator() const noexcept
    {
        return value_.get_allocator();
    }

    template <class FieldScanner = std::nullptr_t>
    void set_field_scanner(std::size_t j, FieldScanner&& s = FieldScanner())
    {
        do_set_field_scanner(j, std::forward<FieldScanner>(s));
    }

private:
    struct scanner_less
    {
        bool operator()(
            const bfs_ptr_p_t& left, std::size_t right) const noexcept
        {
            return left.second < right;
        }

        bool operator()(
            std::size_t left, const bfs_ptr_p_t& right) const noexcept
        {
            return left < right.second;
        }
    };

    template <class FieldScanner>
    auto do_set_field_scanner(std::size_t j, FieldScanner&& s)
     -> std::enable_if_t<
            !std::is_base_of_v<std::nullptr_t, std::decay_t<FieldScanner>>>
    {
        using scanner_t = typed_body_field_scanner<std::decay_t<FieldScanner>>;
        const auto it = std::lower_bound(
            scanners_.begin(), scanners_.end(), j, scanner_less());
        const auto p = allocate_construct<scanner_t>(
                            std::forward<FieldScanner>(s));         // throw
        if ((it != scanners_.end()) && (it->second == j)) {
            destroy_deallocate(it->first);
            it->first = p;
        } else {
            try {
                scanners_.emplace(it, p, j);                        // throw
            } catch (...) {
                destroy_deallocate(p);
                throw;
            }
        }
    }

    void do_set_field_scanner(std::size_t j, std::nullptr_t)
    {
        const auto it = std::lower_bound(
            scanners_.begin(), scanners_.end(), j, scanner_less());
        if ((it != scanners_.end()) && (it->second == j)) {
            destroy_deallocate(it->first);
            scanners_.erase(it);
        }
    }

public:
    const std::type_info& get_field_scanner_type(std::size_t j) const noexcept
    {
        const auto it = std::lower_bound(
            scanners_.cbegin(), scanners_.cend(), j, scanner_less());
        if ((it != scanners_.cend()) && (it->second == j)) {
            return it->first->get_type();
        }
        return typeid(void);
    }

    bool has_field_scanner(std::size_t j) const noexcept
    {
        return std::binary_search(
            scanners_.cbegin(), scanners_.cend(), j, scanner_less());
    }

    template <class FieldScanner>
    const FieldScanner* get_field_scanner(std::size_t j) const noexcept
    {
        return get_field_scanner_g<FieldScanner>(*this, j);
    }

    template <class FieldScanner>
    FieldScanner* get_field_scanner(std::size_t j) noexcept
    {
        return get_field_scanner_g<FieldScanner>(*this, j);
    }

private:
    template <class FieldScanner, class ThisType>
    static std::conditional_t<
        std::is_const_v<std::remove_reference_t<ThisType>>,
        const FieldScanner, FieldScanner>*
    get_field_scanner_g(ThisType& me, std::size_t j) noexcept
    {
        const auto it = std::lower_bound(me.scanners_.begin(),
            me.scanners_.end(), j, scanner_less());
        if ((it != me.scanners_.end()) && (it->second == j)) {
            return it->first->template get_target<FieldScanner>();
        }
        return nullptr;
    }

public:
    template <class RecordEndScanner = std::nullptr_t>
    void set_record_end_scanner(RecordEndScanner&& s = RecordEndScanner())
    {
        do_set_record_end_scanner(std::forward<RecordEndScanner>(s));
    }

private:
    template <class RecordEndScanner>
    auto do_set_record_end_scanner(RecordEndScanner&& s)
     -> std::enable_if_t<
            !std::is_base_of_v<std::nullptr_t, std::decay_t<RecordEndScanner>>>
    {
        using scanner_t = typed_record_end_scanner<
            std::decay_t<RecordEndScanner>>;
        const auto p = allocate_construct<scanner_t>(
            std::forward<RecordEndScanner>(s)); // throw
        if (end_scanner_) {
            destroy_deallocate(end_scanner_);
        }
        end_scanner_ = p;
    }

    void do_set_record_end_scanner(std::nullptr_t)
    {
        if (end_scanner_) {
            destroy_deallocate(end_scanner_);
            end_scanner_ = nullptr;
        }
    }

public:
    const std::type_info& get_record_end_scanner_type() const noexcept
    {
        if (end_scanner_) {
            return end_scanner_->get_type();
        } else {
            return typeid(void);
        }
    }

    bool has_record_end_scanner() const noexcept
    {
        return end_scanner_ != nullptr;
    }

    template <class RecordEndScanner>
    const RecordEndScanner* get_record_end_scanner() const noexcept
    {
        return get_record_end_scanner_g<RecordEndScanner>(*this);
    }

    template <class RecordEndScanner>
    RecordEndScanner* get_record_end_scanner() noexcept
    {
        return get_record_end_scanner_g<RecordEndScanner>(*this);
    }

private:
    template <class RecordEndScanner, class ThisType>
    static auto get_record_end_scanner_g(ThisType& me)
    {
        return me.has_record_end_scanner() ?
            me.end_scanner_->template get_target<RecordEndScanner>() :
            nullptr;
    }

public:
    [[nodiscard]] std::pair<Ch*, std::size_t> get_buffer()
    {
        if (!buffer_) {
            auto a = get_allocator();
            buffer_ = at_t::allocate(a, buffer_size_);  // throw
        } else if (begin_) {
            value_.assign(begin_, end_);                // throw
            begin_ = nullptr;
        }
        return { true_buffer(), static_cast<std::size_t>(buffer_size_ - 1) };
        // We'd like to push buffer_[buffer_size_] with '\0' on EOF
        // so we tell the driver that the buffer size is smaller by one
    }

    void release_buffer(const Ch*)
    {}

    void start_record(const Ch* /*record_begin*/)
    {
        sj_ = 0;
        j_ = 0;
    }

    void update(const Ch* first, const Ch* last)
    {
        if (get_scanner().first) {
            if (begin_) {
                assert(value_.empty());
                value_.reserve((end_ - begin_) + (last - first));   // throw
                value_.assign(begin_, end_);
                value_.append(first, last);
                begin_ = nullptr;
            } else if (!value_.empty()) {
                value_.append(first, last);                         // throw
            } else {
                begin_ = first;
                end_ = last;
            }
        }
    }

    void finalize(const Ch* first, const Ch* last)
    {
        if (const auto scanner = get_scanner(); scanner.first) {
            if (begin_) {
                if (first != last) {
                    value_.
                        reserve((end_ - begin_) + (last - first));  // throw
                    value_.assign(begin_, end_);
                    value_.append(first, last);
                    scanner.first->field_value(std::move(value_), *this);
                    value_.clear();
                } else {
                    *uc(end_) = Ch();
                    scanner.first->field_value(uc(begin_), uc(end_), *this);
                }
                begin_ = nullptr;
            } else if (!value_.empty()) {
                value_.append(first, last);                         // throw
                scanner.first->field_value(std::move(value_), *this);
                value_.clear();
            } else {
                *uc(last) = Ch();
                scanner.first->field_value(uc(first), uc(last), *this);
            }
            if (scanner.second) {
                ++sj_;
            }
        }
        ++j_;
    }

    bool end_record(const Ch* /*record_end*/)
    {
        if (header_field_scanner_) {
            header_field_scanner_->so_much_for_header(*this);
        } else {
            for (auto i = sj_, ie = scanners_.size(); i != ie; ++i) {
                scanners_[i].first->field_skipped();
            }
            if (end_scanner_) {
                return end_scanner_->end_record(*this);
            }
        }
        return true;
    }

    bool is_in_header() const noexcept
    {
        return header_field_scanner_;
    }

private:
    std::size_t sanitize_buffer_size(std::size_t buffer_size) noexcept
    {
        return std::max(
            static_cast<std::size_t>(2U),
            detail::sanitize_buffer_size(buffer_size, get_allocator()));
    }

    template <class T, class... Args>
    [[nodiscard]] auto allocate_construct(Args&&... args)
    {
        using t_alloc_traits_t =
            typename at_t::template rebind_traits<T>;
        typename t_alloc_traits_t::allocator_type a(get_allocator());
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

    template <class P>
    void destroy_deallocate(P p)
    {
        assert(p);
        using v_t = typename std::pointer_traits<P>::element_type;
        p->~v_t();
        using t_at_t = typename at_t::template rebind_traits<v_t>;
        typename t_at_t::allocator_type a(get_allocator());
        t_at_t::deallocate(a, p, 1);
    }

    Ch* true_buffer() const noexcept
    {
        assert(buffer_);
        return std::addressof(*buffer_);
    }

    std::pair<field_scanner*, bool> get_scanner()
    {
        if (header_field_scanner_) {
            return { std::addressof(*header_field_scanner_), false };
        } else if ((sj_ < scanners_.size()) && (j_ == scanners_[sj_].second)) {
            return { std::addressof(*scanners_[sj_].first), true };
        } else {
            return { nullptr, false };
        }
    }

    Ch* uc(const Ch* s) const
    {
        const auto tb = true_buffer();
        return tb + (s - tb);
    }

    void remove_header_field_scanner(bool at_record_end)
    {
        if (at_record_end) {
            destroy_deallocate(header_field_scanner_);
            header_field_scanner_ = nullptr;
        } else {
            // If removal of the header field scanner occurs in the midst of a
            // record, we must replace the scanner with a "padder" so that
            // no body field scanners fire on the remaining header fields
            decltype(header_field_scanner_) padder =
                allocate_construct<typed_header_field_scanner<
                    detail::scanner::counting_header_field_scanner>>(1);
                        // throw
            destroy_deallocate(header_field_scanner_);
            header_field_scanner_ = padder;
        }
    }
};

using table_scanner = basic_table_scanner<char>;
using wtable_scanner = basic_table_scanner<wchar_t>;

class field_translation_error : public text_error
{
public:
    using text_error::text_error;
};

class field_not_found : public field_translation_error
{
public:
    using field_translation_error::field_translation_error;
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
constexpr replacement_ignore_t replacement_ignore = replacement_ignore_t{};

struct replacement_fail_t {};
constexpr replacement_fail_t replacement_fail = replacement_fail_t{};

struct invalid_format_t {};
struct out_of_range_t {};
struct empty_t {};

namespace detail::scanner {

template <class T>
struct numeric_type_traits;

template <>
struct numeric_type_traits<char>
{
    static constexpr const char* name = "char";
    using raw_type =
        std::conditional_t<std::is_signed_v<char>, long, unsigned long>;
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
    static constexpr const float huge = HUGE_VALF;
};

template <>
struct numeric_type_traits<double>
{
    static constexpr const char* name = "double";
    static constexpr const auto strto = std::strtod;
    static constexpr const auto wcsto = std::wcstod;
    static constexpr const double huge = HUGE_VAL;
};

template <>
struct numeric_type_traits<long double>
{
    static constexpr const char* name = "long double";
    static constexpr const auto strto = std::strtold;
    static constexpr const auto wcsto = std::wcstold;
    static constexpr const long double huge = HUGE_VALL;
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
            std::find_if<const Ch*>(middle, end, [](Ch c) {
                return !is_space(c);
            }) != end;
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

    int erange(T v) const
    {
        if constexpr (std::is_signed_v<T>) {
            if (v == std::numeric_limits<T>::max()) {
                return 1;
            } else if (v == std::numeric_limits<T>::min()) {
                return -1;
            } else {
                return 0;
            }
        } else {
            if (v == std::numeric_limits<T>::max()) {
                return 1;
            } else {
                return -1;
            }
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
        if (v == numeric_type_traits<T>::huge) {
            return 1;
        } else if (v == -numeric_type_traits<T>::huge) {
            return -1;
        } else {
            return 0;
        }
    }
};

template <class Ch>
struct has_simple_invalid_format_impl
{
    template <class F>
    static auto check(F*) -> decltype(
        std::declval<F&>()(std::declval<invalid_format_t>(),
            std::declval<const Ch*>(), std::declval<const Ch*>()),
        std::true_type());

    template <class F>
    static auto check(...) -> std::false_type;
};

template <class F, class Ch>
constexpr bool has_simple_invalid_format_v =
    decltype(has_simple_invalid_format_impl<Ch>::template check<F>(nullptr))();

template <class Ch>
struct has_simple_out_of_range_impl
{
    template <class F>
    static auto check(F*) -> decltype(
        std::declval<F&>()(std::declval<out_of_range_t>(),
            std::declval<const Ch*>(), std::declval<const Ch*>(),
            std::declval<int>()),
        std::true_type());

    template <class F>
    static auto check(...) -> std::false_type;
};

template <class F, class Ch>
constexpr bool has_simple_out_of_range_v =
    decltype(has_simple_out_of_range_impl<Ch>::template check<F>(nullptr))();

struct has_simple_empty_impl
{
    template <class F>
    static auto check(F*) -> decltype(
        std::declval<F&>()(std::declval<empty_t>()),
        std::true_type());

    template <class F>
    static auto check(...) -> std::false_type;
};

template <class F>
constexpr bool has_simple_empty_v =
    decltype(has_simple_empty_impl::check<F>(nullptr))();

template <class T>
struct conversion_error_facade
{
    template <class H, class Ch>
    static std::optional<T> invalid_format(
        H& h,const Ch* begin, const Ch* end)
    {
        if constexpr (has_simple_invalid_format_v<H, Ch>) {
            return h(invalid_format_t(), begin, end);
        } else {
            return h(invalid_format_t(), begin, end, static_cast<T*>(nullptr));
        }
    }

    template <class H, class Ch>
    static std::optional<T> out_of_range(
        H& h, const Ch* begin, const Ch* end, int sign)
    {
        if constexpr (has_simple_out_of_range_v<H, Ch>) {
            return h(out_of_range_t(), begin, end, sign);
        } else {
            return h(out_of_range_t(),
                     begin, end, sign, static_cast<T*>(nullptr));
        }
    }

    template <class H>
    static std::optional<T> empty(H& h)
    {
        if constexpr (has_simple_empty_v<H>) {
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

// For types without corresponding "raw_type"
template <class T, class H, class = void>
struct converter :
    private raw_converter<T, typed_conversion_error_handler<T, H>>
{
    template <class K,
        std::enable_if_t<!std::is_base_of_v<converter, std::decay_t<K>>>*
            = nullptr>
    converter(K&& h) :
        raw_converter<T, typed_conversion_error_handler<T, H>>(
            typed_conversion_error_handler<T, H>(std::forward<K>(h)))
    {}

    using raw_converter<T, typed_conversion_error_handler<T, H>>::
        get_conversion_error_handler;

    template <class Ch>
    auto convert(const Ch* begin, const Ch* end)
    {
        return this->convert_raw(begin, end);
    }
};

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
            return std::optional<T>();
        }
        const auto r = *result;
        if constexpr (sizeof(T) < sizeof(U)) {
            constexpr auto t_max = std::numeric_limits<T>::max();
            if (r <= t_max) {
                return { static_cast<T>(r) };
            } else {
                const auto s = static_cast<std::make_signed_t<U>>(r);
                if (s < 0) {
                    // -t_max is the lowest number that can be wrapped around
                    // and then returned
                    const auto s_wrapped_around = s + t_max + 1;
                    if (s_wrapped_around > 0) {
                        return { static_cast<T>(s_wrapped_around) };
                    }
                }
            }
            return conversion_error_facade<T>::out_of_range(
                this->get_conversion_error_handler(), begin, end, 1);
        } else {
            return { static_cast<T>(r) };
        }
    }
};

// For types which have corresponding "raw_type"
template <class T, class H>
struct converter<T, H,
                 std::void_t<typename numeric_type_traits<T>::raw_type>> :
    restrained_converter<T, typed_conversion_error_handler<T, H>,
        typename numeric_type_traits<T>::raw_type>
{
    using restrained_converter<T, typed_conversion_error_handler<T, H>,
        typename numeric_type_traits<T>::raw_type>::restrained_converter;
};

} // end detail::scanner

struct fail_if_skipped
{
    explicit fail_if_skipped(replacement_fail_t = replacement_fail)
    {}

    template <class T>
    [[noreturn]]
    std::optional<T> operator()(T* = nullptr) const
    {
        throw field_not_found("This field did not appear in this record");
    }
};

namespace detail::scanner { 

enum class replace_mode
{
    replace,
    fail,
    ignore
};

struct generic_args_t {};

namespace replace_if_skipped_impl {

template <class T>
class trivial_store
{
    alignas(T) char value_[sizeof(T)];
    replace_mode mode_;

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
    using replace_mode = detail::scanner::replace_mode;

    using generic_args_t = detail::scanner::generic_args_t;

    using store_t = detail::scanner::replace_if_skipped_impl::store_t<T>;
    store_t store_;

public:
    template <class... Args,
        std::enable_if_t<
            std::is_constructible_v<T, Args&&...>
         && !((sizeof...(Args) == 1)
           && (std::is_base_of_v<replace_if_skipped,
                detail::first_t<std::decay_t<Args>...>>
            || std::is_base_of_v<replacement_ignore_t,
                detail::first_t<std::decay_t<Args>...>>
            || std::is_base_of_v<replacement_fail_t,
                detail::first_t<std::decay_t<Args>...>>))>* = nullptr>
    explicit replace_if_skipped(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args&&...>) :
        store_(generic_args_t(), std::forward<Args>(args)...)
    {}

    explicit replace_if_skipped(replacement_fail_t) noexcept :
        store_(detail::scanner::replace_mode::fail)
    {}

    explicit replace_if_skipped(replacement_ignore_t) noexcept :
        store_(detail::scanner::replace_mode::ignore)
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
        using std::swap;
        swap(store_, other.store_);
    }
};

template <class T>
auto swap(replace_if_skipped<T>& left, replace_if_skipped<T>& right)
    noexcept(noexcept(left.swap(right)))
 -> std::enable_if_t<std::is_swappable_v<T>>
{
    left.swap(right);
}

namespace detail::scanner {

struct replaced_type_impossible_t
{};

struct replaced_type_not_found_t
{};

template <class... Ts>
struct replaced_type_from_impl;

template <class Found>
struct replaced_type_from_impl<Found>
{
    using type = std::conditional_t<
        std::is_same_v<replaced_type_impossible_t, Found>,
        replaced_type_not_found_t, Found>;
};

// In C++20, we can use std::type_identity instead
template <class T>
struct identity
{
    using type = T;
};

struct has_common_type_impl
{
    template <class... Ts>
    static std::true_type check(typename std::common_type<Ts...>::type*);

    template <class... Ts>
    static std::false_type check(...);
};

template <class... Ts>
constexpr bool has_common_type_v =
    decltype(has_common_type_impl::check<Ts...>(nullptr))::value;

template <class Found, class Head, class... Tails>
struct replaced_type_from_impl<Found, Head, Tails...>
{
    using type = typename replaced_type_from_impl<
        typename std::conditional<
            std::is_base_of_v<replacement_fail_t, Head>
         || std::is_base_of_v<replacement_ignore_t, Head>,
            identity<identity<Found>>,
            typename std::conditional<
                std::is_same_v<Found, replaced_type_not_found_t>,
                identity<Head>,
                std::conditional_t<
                    has_common_type_v<Found, Head>,
                    std::common_type<Found, Head>,
                    identity<replaced_type_impossible_t>>>>::type::type::type,
        Tails...>::type;
};

template <class... Ts>
using replaced_type_from_t =
    typename replaced_type_from_impl<replaced_type_not_found_t, Ts...>::type;

} // end detail::scanner

template <class T,
    std::enable_if_t<
        !std::is_same_v<
            detail::scanner::replaced_type_not_found_t,
            detail::scanner::replaced_type_from_t<T>>,
        std::nullptr_t> = nullptr>
replace_if_skipped(T)
 -> replace_if_skipped<detail::scanner::replaced_type_from_t<T>>;

struct fail_if_conversion_failed
{
    template <class T, class Ch>
    [[noreturn]]
    std::optional<T> operator()(invalid_format_t,
        const Ch* begin, const Ch* end, T* = nullptr) const
    {
        assert(*end == Ch());
        std::ostringstream s;
        detail::write_ntmbs(s, begin, end);
        s << ": cannot convert";
        write_name<T>(s, " to an instance of ");
        throw field_invalid_format(std::move(s).str());
    }

    template <class T, class Ch>
    [[noreturn]]
    std::optional<T> operator()(out_of_range_t,
        const Ch* begin, const Ch* end, int, T* = nullptr) const
    {
        assert(*end == Ch());
        std::ostringstream s;
        detail::write_ntmbs(s, begin, end);
        s << ": out of range";
        write_name<T>(s, " of ");
        throw field_out_of_range(std::move(s).str());
    }

    template <class T>
    [[noreturn]]
    std::optional<T> operator()(empty_t, T* = nullptr) const
    {
        std::ostringstream s;
        s << "Cannot convert an empty string";
        write_name<T>(s, " to an instance of ");
        throw field_empty(std::move(s).str());
    }

private:
    template <class T>
    static std::ostream& write_name(std::ostream& os, const char* prefix,
        decltype(detail::scanner::numeric_type_traits<T>::name)* = nullptr)
    {
        return os << prefix << detail::scanner::numeric_type_traits<T>::name;
    }

    template <class T>
    static std::ostream& write_name(std::ostream& os, ...)
    {
        return os;
    }
};

namespace detail::scanner::replace_if_conversion_failed_impl {

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
    alignas(T) char replacements_[N][sizeof(T)];
    std::uint_fast8_t has_;
    std::uint_fast8_t skips_;

protected:
    trivial_store(copy_mode_t, const trivial_store& other) noexcept :
        has_(other.has_), skips_(other.skips_)
    {}

public:
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

    template <std::size_t Slot>
    trivial_store(std::integral_constant<std::size_t, Slot>) :
        trivial_store(std::integral_constant<std::size_t, Slot + 1>())
    {
        init<Slot>();
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
    base(detail::scanner::generic_args_t, As&&... as) :
        store_(detail::scanner::generic_args_t(), std::forward<As>(as)...)
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
    using base<T, 0>::base;

    template <class Empty = T, class InvalidFormat = T,
        class AboveUpperLimit = T,
        std::enable_if_t<
            is_acceptable_arg_v<T, Empty&&>
         && is_acceptable_arg_v<T, InvalidFormat&&>
         && is_acceptable_arg_v<T, AboveUpperLimit&&>>* = nullptr>
    explicit base(
        Empty&& on_empty = Empty(),
        InvalidFormat&& on_invalid_format = InvalidFormat(),
        AboveUpperLimit&& on_above_upper_limit = AboveUpperLimit())
            noexcept(std::is_nothrow_default_constructible_v<T>
                  && is_nothrow_arg_v<T, Empty>
                  && is_nothrow_arg_v<T, InvalidFormat>
                  && is_nothrow_arg_v<T, AboveUpperLimit>) :
        base<T, 0>(
            detail::scanner::generic_args_t(),
            std::forward<Empty>(on_empty),
            std::forward<InvalidFormat>(on_invalid_format),
            std::forward<AboveUpperLimit>(on_above_upper_limit))
    {}
};

template <class T>
struct base<T, 4> : base<T, 3>
{
    using base<T, 3>::base;

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
        base<T, 3>(
            detail::scanner::generic_args_t(),
            std::forward<Empty>(on_empty),
            std::forward<InvalidFormat>(on_invalid_format),
            std::forward<AboveUpperLimit>(on_above_upper_limit),
            std::forward<BelowLowerLimit>(on_below_lower_limit))
    {}
};

template <class T>
struct base<T, 5> : base<T, std::is_default_constructible<T>::value ? 4 : 0>
{
    using base<T, std::is_default_constructible<T>::value ? 4 : 0>::base;

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
        base<T, std::is_default_constructible<T>::value ? 4 : 0>(
            detail::scanner::generic_args_t(),
            std::forward<Empty>(on_empty),
            std::forward<InvalidFormat>(on_invalid_format),
            std::forward<AboveUpperLimit>(on_above_upper_limit),
            std::forward<BelowLowerLimit>(on_below_lower_limit),
            std::forward<Underflow>(on_underflow))
    {}
};

template <class T>
using base_t = base<T, base_n<T>>;

}

template <class T>
class replace_if_conversion_failed :
    public detail::scanner::replace_if_conversion_failed_impl::base_t<T>
{
    using replace_mode = detail::scanner::replace_mode;

    static constexpr unsigned slot_empty =
        detail::scanner::replace_if_conversion_failed_impl::slot_empty;
    static constexpr unsigned slot_invalid_format =
        detail::scanner::replace_if_conversion_failed_impl::
            slot_invalid_format;
    static constexpr unsigned slot_above_upper_limit =
        detail::scanner::replace_if_conversion_failed_impl::
            slot_above_upper_limit;
    static constexpr unsigned slot_below_lower_limit =
        detail::scanner::replace_if_conversion_failed_impl::
            slot_below_lower_limit;
    static constexpr unsigned slot_underflow =
        detail::scanner::replace_if_conversion_failed_impl::slot_underflow;

private:
    template <class... As>
    replace_if_conversion_failed(detail::scanner::generic_args_t, As&&...)
        = delete;

public:
    // VS2015 refuses to compile "base_t<T>" here
    using detail::scanner::replace_if_conversion_failed_impl::base<T,
        detail::scanner::replace_if_conversion_failed_impl::base_n<T>>::base;

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
        using std::swap;
        swap(this->store(), other.store());
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
            detail::scanner::replaced_type_not_found_t,
            detail::scanner::replaced_type_from_t<Ts...>>>* = nullptr>
replace_if_conversion_failed(Ts...)
 -> replace_if_conversion_failed<detail::scanner::replaced_type_from_t<Ts...>>;

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
        std::optional<T> r;
        if constexpr (std::is_invocable_v<SkippingHandler&>) {
            r = get_skipping_handler()();
        } else {
            r = get_skipping_handler()(static_cast<T*>(nullptr));
        }
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
    using converter_t = detail::scanner::converter<T, ConversionErrorHandler>;
    using translator_t = detail::scanner::translator<T, Sink, SkippingHandler>;

    detail::base_member_pair<converter_t, translator_t> ct_;

public:
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
        ct_(converter_t(std::forward<ConversionErrorHandlerR>(
                            handle_conversion_error)),
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
        return ct_.base().get_conversion_error_handler();
    }

    const ConversionErrorHandler& get_conversion_error_handler() const noexcept
    {
        return ct_.base().get_conversion_error_handler();
    }

    template <class Ch>
    void operator()(const Ch* begin, const Ch* end)
    {
        assert(*end == Ch());
        ct_.member().put(ct_.base().convert(begin, end));
    }
};

template <class T, class Sink,
    class SkippingHandler = fail_if_skipped,
    class ConversionErrorHandler = fail_if_conversion_failed>
class locale_based_arithmetic_field_translator
{
    std::locale loc_;
    arithmetic_field_translator<
        T, Sink, SkippingHandler, ConversionErrorHandler> out_;

    // These are initialized after parsing has started
    wchar_t decimal_point_;     // of specified loc in the ctor
    wchar_t thousands_sep_;     // ditto
    wchar_t decimal_point_c_;   // of C's global
                                // to mimic std::strtol and its comrades
    wchar_t thousands_sep_c_;   // ditto
    bool mimics_;

public:
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
        loc_(loc),
        out_(std::forward<SinkR>(sink),
             std::forward<SkippingHandlerR>(handle_skipping),
             std::forward<ConversionErrorHandlerR>(handle_conversion_error)),
        decimal_point_c_(), mimics_()
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
        if (decimal_point_c_ == wchar_t()) {
            decimal_point_c_ = widen(*std::localeconv()->decimal_point, Ch());
            thousands_sep_c_ = widen(*std::localeconv()->thousands_sep, Ch());

            const auto& facet = std::use_facet<std::numpunct<Ch>>(loc_);
            thousands_sep_ = facet.grouping().empty() ?
                Ch() : facet.thousands_sep();
            decimal_point_ = facet.decimal_point();

            mimics_ = (decimal_point_ != decimal_point_c_)
                   || ((thousands_sep_ != Ch()) // means needs to take care of
                                                // separators in field values
                    && (thousands_sep_ != thousands_sep_c_));
        }
        assert(*end == Ch());   // shall be null-terminated
        if (mimics_) {
            bool decimal_point_appeared = false;
            Ch* head = begin;
            for (Ch* b = begin; b != end; ++b) {
                Ch c = *b;
                assert(c != Ch());
                if (c == static_cast<Ch>(decimal_point_)) {
                    if (!decimal_point_appeared) {
                        c = static_cast<Ch>(decimal_point_c_);
                        decimal_point_appeared = true;
                    }
                } else if (c == static_cast<Ch>(thousands_sep_)) {
                    continue;
                }
                *head = c;
                ++head;
            }
            *head = Ch();
            out_(begin, head);
        } else {
            out_(begin, end);
        }
    }

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

template <class Sink, class Ch,
    class Tr = std::char_traits<Ch>, class Allocator = std::allocator<Ch>,
    class SkippingHandler = fail_if_skipped>
class string_field_translator
{
    using translator_t =
        detail::scanner::translator<std::basic_string<Ch, Tr, Allocator>,
        Sink, SkippingHandler>;

    detail::base_member_pair<Allocator, translator_t> at_;

public:
    using allocator_type = Allocator;

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
    using translator_t = detail::scanner::translator<
        std::basic_string_view<Ch, Tr>, Sink, SkippingHandler>;
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

template <class T, class Sink, class... As,
    std::enable_if_t<!detail::is_std_string_v<T>, std::nullptr_t> = nullptr>
arithmetic_field_translator<T, Sink, fail_if_skipped, As...>
    make_field_translator_na(Sink, replacement_fail_t, As...);

template <class T, class Sink, class ReplaceIfSkipped, class... As,
    std::enable_if_t<
        !detail::is_std_string_v<T>
     && !detail::is_std_string_view_v<T>
     && !std::is_base_of_v<std::locale, std::decay_t<ReplaceIfSkipped>>
     && !std::is_base_of_v<replacement_fail_t,std::decay_t<ReplaceIfSkipped>>
     && std::is_constructible_v<replace_if_skipped<T>, ReplaceIfSkipped&&>,
        std::nullptr_t> = nullptr>
arithmetic_field_translator<T, Sink, replace_if_skipped<T>, As...>
    make_field_translator_na(Sink, ReplaceIfSkipped&&, As...);

template <class T, class Sink, class... As,
    std::enable_if_t<
        !detail::is_std_string_v<T>
     && !detail::is_std_string_view_v<T>
     && ((sizeof...(As) == 0)
      || !(std::is_base_of_v<std::locale, std::decay_t<detail::first_t<As...>>>
        || std::is_constructible_v<replace_if_skipped<T>,
                                   detail::first_t<As...>>)),
        std::nullptr_t> = nullptr>
arithmetic_field_translator<T, Sink, std::decay_t<As>...>
    make_field_translator_na(Sink, As&&...);

template <class T, class Sink, class... As,
    std::enable_if_t<
        !detail::is_std_string_v<T>,
        std::nullptr_t> = nullptr>
locale_based_arithmetic_field_translator<T, Sink, fail_if_skipped, As...>
    make_field_translator_na(Sink, std::locale, replacement_fail_t, As...);

template <class T, class Sink, class ReplaceIfSkipped, class... As,
    std::enable_if_t<
        !detail::is_std_string_v<T>
     && !std::is_base_of_v<replacement_fail_t, std::decay_t<ReplaceIfSkipped>>
     && std::is_constructible_v<replace_if_skipped<T>, ReplaceIfSkipped&&>,
        std::nullptr_t> = nullptr>
locale_based_arithmetic_field_translator<T, Sink, replace_if_skipped<T>, As...>
    make_field_translator_na(Sink, std::locale, ReplaceIfSkipped&&, As...);

template <class T, class Sink, class... As,
    std::enable_if_t<
        !detail::is_std_string_v<T>
     && ((sizeof...(As) == 0)
      || !std::is_constructible_v<replace_if_skipped<T>,
                                  detail::first_t<As...>>),
        std::nullptr_t> = nullptr>
locale_based_arithmetic_field_translator<T, Sink, std::decay_t<As>...>
    make_field_translator_na(Sink, std::locale, As&&... as);

template <class T, class Sink,
    std::enable_if_t<detail::is_std_string_v<T>, std::nullptr_t>
        = nullptr>
string_field_translator<Sink,
        typename T::value_type, typename T::traits_type,
        typename T::allocator_type, fail_if_skipped>
    make_field_translator_na(Sink, replacement_fail_t);

template <class T, class Sink, class ReplaceIfSkipped,
    std::enable_if_t<
        detail::is_std_string_v<T>
     && !std::is_base_of_v<replacement_fail_t, std::decay_t<ReplaceIfSkipped>>
     && std::is_constructible_v<replace_if_skipped<T>, ReplaceIfSkipped&&>,
        std::nullptr_t> = nullptr>
string_field_translator<Sink,
        typename T::value_type, typename T::traits_type,
        typename T::allocator_type, replace_if_skipped<T>>
    make_field_translator_na(Sink, ReplaceIfSkipped&&);

template <class T, class Sink, class... As,
    std::enable_if_t<
        detail::is_std_string_v<T>
     && ((sizeof...(As) == 0)
      || !std::is_constructible_v<replace_if_skipped<T>,
                                  detail::first_t<As...>>),
        std::nullptr_t> = nullptr>
string_field_translator<Sink,
        typename T::value_type, typename T::traits_type,
        typename T::allocator_type, std::decay_t<As>...>
    make_field_translator_na(Sink, As&&...);

template <class T, class Sink,
    std::enable_if_t<detail::is_std_string_view_v<T>, std::nullptr_t>
        = nullptr>
string_view_field_translator<Sink,
        typename T::value_type, typename T::traits_type, fail_if_skipped>
    make_field_translator_na(Sink, replacement_fail_t);

template <class T, class Sink, class ReplaceIfSkipped,
    std::enable_if_t<
        detail::is_std_string_view_v<T>
     && !std::is_base_of_v<replacement_fail_t, std::decay_t<ReplaceIfSkipped>>
     && std::is_constructible_v<replace_if_skipped<T>, ReplaceIfSkipped&&>,
        std::nullptr_t> = nullptr>
string_view_field_translator<Sink,
        typename T::value_type, typename T::traits_type, replace_if_skipped<T>>
    make_field_translator_na(Sink, ReplaceIfSkipped&&);

template <class T, class Sink, class... As,
    std::enable_if_t<
        detail::is_std_string_view_v<T>
     && ((sizeof...(As) == 0)
      || !std::is_constructible_v<replace_if_skipped<T>,
                                  detail::first_t<As...>>),
        std::nullptr_t> = nullptr>
string_view_field_translator<Sink,
        typename T::value_type, typename T::traits_type, std::decay_t<As>...>
    make_field_translator_na(Sink, As&&...);

} // end detail::scanner

template <class T, class Sink, class... Appendices>
auto make_field_translator(Sink&& sink, Appendices&&... appendices)
 -> std::enable_if_t<
        (detail::scanner::is_convertible_numeric_type_v<T>
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
        (detail::scanner::is_convertible_numeric_type_v<
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
