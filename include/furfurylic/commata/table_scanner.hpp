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
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <iomanip>
#include <iterator>
#include <limits>
#include <locale>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <vector>

#include "allocation_only_allocator.hpp"
#include "text_error.hpp"
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

// We made this class template to give nothrow-move-constructibility to
// std::vector, so we shall be able to remove this in C++17, in which it has
// a noexcept move ctor
template <class T, class = void>
class nothrow_move_constructible
{
    T* t_;

public:
    template <class AnyAllocator, class... Args>
    explicit nothrow_move_constructible(
        std::allocator_arg_t, const AnyAllocator& any_alloc, Args&&... args)
    {
        using at_t = typename std::allocator_traits<AnyAllocator>::
            template rebind_traits<T>;
        typename at_t::allocator_type alloc(any_alloc);
        const auto p = at_t::allocate(alloc, 1);
        t_ = std::addressof(*p);
        try {
            ::new(t_) T(std::forward<Args>(args)...);
        } catch (...) {
            at_t::deallocate(alloc, p, 1);
            throw;
        }
    }

    nothrow_move_constructible(nothrow_move_constructible&& other) noexcept :
        t_(other.t_)
    {
        other.t_ = nullptr;
    }

    ~nothrow_move_constructible()
    {
        assert(!t_);
    }

    explicit operator bool() const
    {
        return t_ != nullptr;
    }

    template <class AnyAllocator>
    void kill(const AnyAllocator& any_alloc)
    {
        assert(t_);
        using at_t = typename std::allocator_traits<AnyAllocator>::
            template rebind_traits<T>;
        typename at_t::allocator_type alloc(any_alloc);
        t_->~T();
        at_t::deallocate(alloc,
            std::pointer_traits<typename at_t::pointer>::pointer_to(*t_), 1);
#ifndef NDEBUG
        t_ = nullptr;
#endif
    }

    T& operator*()
    {
        assert(t_);
        return *t_;
    }

    const T& operator*() const
    {
        assert(t_);
        return *t_;
    }

    T* operator->()
    {
        assert(t_);
        return t_;
    }

    const T* operator->() const
    {
        assert(t_);
        return t_;
    }

    template <class AnyAllocator>
    void assign(const AnyAllocator& any_alloc,
        nothrow_move_constructible&& other) noexcept
    {
        kill(any_alloc);
        t_ = other.t_;
        other.t_ = nullptr;
    }
};

template <class T>
class nothrow_move_constructible<T,
    std::enable_if_t<std::is_nothrow_move_constructible<T>::value>>
{
    std::aligned_storage_t<sizeof(T), alignof(T)> t_;

public:
    template <class AnyAllocator, class... Args>
    explicit nothrow_move_constructible(
        std::allocator_arg_t, const AnyAllocator&, Args&&... args)
    {
        ::new(&*(*this)) T(std::forward<Args>(args)...);
    }

    nothrow_move_constructible(nothrow_move_constructible&& other) noexcept :
        t_(std::move(other.t_))
    {
        ::new(&*(*this)) T(std::move(*other));
        other->clear(); // inhibit overkill
    }

    ~nothrow_move_constructible()
    {
        (*this)->~T();
    }

    constexpr explicit operator bool() const
    {
        return true;
    }

    template <class AnyAllocator>
    void kill(const AnyAllocator&)
    {}

    T& operator*()
    {
        return *reinterpret_cast<T*>(&t_);
    }

    const T& operator*() const
    {
        return *reinterpret_cast<const T*>(&t_);
    }

    T* operator->()
    {
        return &*(*this);
    }

    const T* operator->() const
    {
        return &*(*this);
    }

    template <class AnyAllocator>
    void assign(const AnyAllocator&,
        nothrow_move_constructible&& other) noexcept
    {
        (*this)->~T();
        ::new(&*(*this)) T(std::move(*other));
        other->clear(); // inhibit overkill
    }
};

} // end namespace detail

template <class Ch, class Tr = std::char_traits<Ch>,
          class Allocator = std::allocator<Ch>>
class table_scanner :
    detail::member_like_base<Allocator>
{
    using string_t = std::basic_string<Ch, Tr, Allocator>;

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

    struct field_scanner
    {
        virtual ~field_scanner() {}
        virtual void field_value(Ch* begin, Ch* end, table_scanner& me) = 0;
        virtual void field_value(string_t&& value, table_scanner& me) = 0;
    };

    struct header_field_scanner : field_scanner
    {
        virtual void so_much_for_header(table_scanner& me) = 0;
    };

    template <class HeaderScanner>
    class typed_header_field_scanner : public header_field_scanner
    {
        HeaderScanner scanner_;

        using range_t = std::pair<Ch*, Ch*>;

    public:
        explicit typed_header_field_scanner(HeaderScanner s) :
            scanner_(std::move(s))
        {}

        void field_value(Ch* begin, Ch* end, table_scanner& me) override
        {
            const range_t range(begin, end);
            if (!scanner_(me.j_, &range, me)) {
                me.remove_header_field_scanner();
            }
        }

        void field_value(string_t&& value, table_scanner& me) override
        {
            field_value(&value[0], &value[0] + value.size(), me);
        }

        void so_much_for_header(table_scanner& me) override
        {
            if (!scanner_(me.j_, static_cast<const range_t*>(nullptr), me)) {
                me.remove_header_field_scanner();
            }
        }
    };

    struct body_field_scanner : field_scanner, typable
    {
        virtual void field_skipped() = 0;
    };

    template <class FieldScanner>
    class typed_body_field_scanner : public body_field_scanner
    {
        FieldScanner scanner_;

    public:
        explicit typed_body_field_scanner(FieldScanner s) :
            scanner_(std::move(s))
        {}

        void field_value(Ch* begin, Ch* end, table_scanner& me) override
        {
            field_value_r(
                typename detail::accepts_range<
                    std::remove_reference_t<decltype(scanner())>, Ch>(),
                begin, end, me);
        }

        void field_value(string_t&& value, table_scanner& me) override
        {
            field_value_s(
                typename detail::accepts_x<
                    std::remove_reference_t<decltype(scanner())>, string_t>(),
                std::move(value), me);
        }

        void field_skipped() override
        {
            scanner().field_skipped();
        }

        const std::type_info& get_type() const noexcept override
        {
            // Static types suffice for we do slicing on construction
            return typeid(FieldScanner);
        }

    private:
        void field_value_r(std::true_type, Ch* begin, Ch* end, table_scanner&)
        {
            scanner().field_value(begin, end);
        }

        void field_value_r(std::false_type,
            Ch* begin, Ch* end, table_scanner& me)
        {
            scanner().field_value(string_t(begin, end, me.get_allocator()));
        }

        void field_value_s(std::true_type, string_t&& value, table_scanner&)
        {
            scanner().field_value(std::move(value));
        }

        void field_value_s(std::false_type, string_t&& value, table_scanner&)
        {
            scanner().field_value(
                &*value.begin(), &*value.begin() + value.size());
        }

        const void* get_target_v() const noexcept override
        {
            return &scanner_;
        }

        void* get_target_v() noexcept override
        {
            return &scanner_;
        }

        decltype(auto) scanner() noexcept
        {
            return scanner_impl(
                detail::is_std_reference_wrapper<FieldScanner>());
        }

        decltype(auto) scanner_impl(std::false_type) noexcept
        {
            return static_cast<FieldScanner&>(scanner_);
        }

        decltype(auto) scanner_impl(std::true_type) noexcept
        {
            return static_cast<typename FieldScanner::type&>(scanner_.get());
        }
    };

    struct record_end_scanner : typable
    {
        virtual void end_record() = 0;
    };

    template <class T>
    class typed_record_end_scanner : public record_end_scanner
    {
        T scanner_;

    public:
        explicit typed_record_end_scanner(T scanner) :
            scanner_(std::move(scanner))
        {}

        void end_record() override
        {
            scanner_();
        }

        const std::type_info& get_type() const noexcept override
        {
            return typeid(T);
        }

    private:
        const void* get_target_v() const noexcept override
        {
            return &scanner_;
        }

        void* get_target_v() noexcept override
        {
            return &scanner_;
        }
    };

    using at_t = std::allocator_traits<Allocator>;
    using hfs_at_t =
        typename at_t::template rebind_traits<header_field_scanner>;
    using bfs_at_t =
        typename at_t::template rebind_traits<body_field_scanner>;
    using bfs_ptr_t = typename bfs_at_t::pointer;
    using bfs_ptr_a_t = typename at_t::template rebind_alloc<bfs_ptr_t>;
    using scanners_a_t = detail::allocation_only_allocator<bfs_ptr_a_t>;
    using res_at_t =
        typename at_t::template rebind_traits<record_end_scanner>;

    std::size_t remaining_header_records_;
    std::size_t j_;
    std::size_t buffer_size_;
    typename at_t::pointer buffer_;
    const Ch* begin_;
    const Ch* end_;
    typename hfs_at_t::pointer header_field_scanner_;
    detail::nothrow_move_constructible<
        std::vector<bfs_ptr_t, scanners_a_t>> scanners_;
    typename res_at_t::pointer end_scanner_;
    string_t fragmented_value_;

public:
    using char_type = Ch;
    using traits_type = Tr;
    using allocator_type = Allocator;
    using size_type = typename std::allocator_traits<Allocator>::size_type;

    explicit table_scanner(
        std::size_t header_record_count = 0U,
        size_type buffer_size = 0U) :
        table_scanner(std::allocator_arg, Allocator(),
            header_record_count, buffer_size)
    {}

    template <
        class HeaderFieldScanner,
        class = std::enable_if_t<!std::is_integral<HeaderFieldScanner>::value>>
    explicit table_scanner(
        HeaderFieldScanner s,
        size_type buffer_size = 0U) :
        table_scanner(
            std::allocator_arg, Allocator(), std::move(s), buffer_size)
    {}

    table_scanner(
        std::allocator_arg_t, const Allocator& alloc,
        std::size_t header_record_count = 0U,
        size_type buffer_size = 0U) :
        detail::member_like_base<Allocator>(alloc),
        remaining_header_records_(header_record_count), j_(0),
        buffer_size_(sanitize_buffer_size(buffer_size)),
        buffer_(), begin_(nullptr),
        header_field_scanner_(),
        scanners_(make_scanners()),
        end_scanner_(nullptr), fragmented_value_(alloc)
    {}

    template <
        class HeaderFieldScanner,
        class = std::enable_if_t<!std::is_integral<HeaderFieldScanner>::value>>
    table_scanner(
        std::allocator_arg_t, const Allocator& alloc,
        HeaderFieldScanner s,
        size_type buffer_size = 0U) :
        detail::member_like_base<Allocator>(alloc),
        remaining_header_records_(0U), j_(0),
        buffer_size_(sanitize_buffer_size(buffer_size)),
        buffer_(), begin_(nullptr),
        header_field_scanner_(allocate_construct<
            typed_header_field_scanner<HeaderFieldScanner>>(std::move(s))),
        scanners_(make_scanners()),
        end_scanner_(nullptr), fragmented_value_(alloc)
    {}

    table_scanner(table_scanner&& other) noexcept :
        detail::member_like_base<Allocator>(std::move(other)),
        remaining_header_records_(other.remaining_header_records_),
        j_(other.j_), buffer_size_(other.buffer_size_),
        buffer_(other.buffer_), begin_(other.begin_), end_(other.end_),
        header_field_scanner_(other.header_field_scanner_),
        scanners_(std::move(other.scanners_)),
        end_scanner_(other.end_scanner_),
        fragmented_value_(std::move(other.fragmented_value_))
    {
        other.buffer_ = nullptr;
        other.header_field_scanner_ = nullptr;
        other.end_scanner_ = nullptr;
    }

    ~table_scanner()
    {
        if (buffer_) {
            at_t::deallocate(this->get(), buffer_, buffer_size_);
        }
        if (header_field_scanner_) {
            destroy_deallocate(header_field_scanner_);
        }
        if (scanners_) {
            for (const auto p : *scanners_) {
                if (p) {
                    destroy_deallocate(p);
                }
            }
            scanners_.kill(this->get());
        }
        if (end_scanner_) {
            destroy_deallocate(end_scanner_);
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
        if (!scanners_) {
            scanners_.assign(this->get(), make_scanners());         // throw
            scanners_->resize(j + 1);                               // throw
        } else if (j >= scanners_->size()) {
            scanners_->resize(j + 1);                               // throw
        }
        const auto p = allocate_construct<scanner_t>(std::move(s)); // throw
        if (const auto scanner = (*scanners_)[j]) {
            destroy_deallocate(scanner);
        }
        (*scanners_)[j] = p;
    }

    void do_set_field_scanner(std::size_t j, std::nullptr_t)
    {
        if (scanners_ && (j < scanners_->size())) {
            auto& scanner = (*scanners_)[j];
            destroy_deallocate(scanner);
            scanner = nullptr;
        }
    }

public:
    const std::type_info& get_field_scanner_type(std::size_t j) const noexcept
    {
        if (scanners_ && (j < scanners_->size()) && (*scanners_)[j]) {
            return (*scanners_)[j]->get_type();
        } else {
            return typeid(void);
        }
    }

    bool has_field_scanner(std::size_t j) const noexcept
    {
        return scanners_ && (j < scanners_->size()) && (*scanners_)[j];
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
    static auto get_field_scanner_g(ThisType& me, std::size_t j) noexcept
    {
        return me.has_field_scanner(j) ?
            (*me.scanners_)[j]->template get_target<FieldScanner>() :
            nullptr;
    }

public:
    template <class RecordEndScanner = std::nullptr_t>
    void set_record_end_scanner(RecordEndScanner s = RecordEndScanner())
    {
        do_set_record_end_scanner(std::move(s));
    }

private:
    template <class RecordEndScanner>
    void do_set_record_end_scanner(RecordEndScanner s)
    {
        using scanner_t = typed_record_end_scanner<RecordEndScanner>;
        end_scanner_ = allocate_construct<scanner_t>(std::move(s)); // throw
    }

    void do_set_record_end_scanner(std::nullptr_t)
    {
        destroy_deallocate(end_scanner_);
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
                fragmented_value_.
                    reserve((end_ - begin_) + (last - first));  // throw
                fragmented_value_.assign(begin_, end_);
                fragmented_value_.append(first, last);
                begin_ = nullptr;
            } else if (!fragmented_value_.empty()) {
                fragmented_value_.append(first, last);          // throw
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
                    fragmented_value_.
                        reserve((end_ - begin_) + (last - first));  // throw
                    fragmented_value_.assign(begin_, end_);
                    fragmented_value_.append(first, last);
                    scanner->field_value(std::move(fragmented_value_), *this);
                    fragmented_value_.clear();
                } else {
                    *uc(end_) = Ch();
                    scanner->field_value(uc(begin_), uc(end_), *this);
                }
                begin_ = nullptr;
            } else if (!fragmented_value_.empty()) {
                fragmented_value_.append(first, last);              // throw
                scanner->field_value(std::move(fragmented_value_), *this);
                fragmented_value_.clear();
            } else {
                *uc(last) = Ch();
                scanner->field_value(uc(first), uc(last), *this);
            }
        }
        ++j_;
        return true;
    }

    bool end_record(const Ch* /*record_end*/)
    {
        if (header_field_scanner_) {
            header_field_scanner_->so_much_for_header(*this);
        } else if (remaining_header_records_ > 0) {
            --remaining_header_records_;
        } else {
            if (scanners_) {
                for (auto j = j_; j < scanners_->size(); ++j) {
                    if (auto scanner = (*scanners_)[j]) {
                        scanner->field_skipped();
                    }
                }
            }
            if (end_scanner_) {
                end_scanner_->end_record();
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
        if (buffer_size == 0U) {
            buffer_size = default_buffer_size;
        }
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

    auto make_scanners()
    {
        return decltype(scanners_)(std::allocator_arg,
            this->get(), scanners_a_t(bfs_ptr_a_t(this->get())));
    }

    Ch* true_buffer() const noexcept
    {
        assert(buffer_);
        return std::addressof(*buffer_);
    }

    field_scanner* get_scanner()
    {
        if (header_field_scanner_) {
            return std::addressof(*header_field_scanner_);
        } else if ((remaining_header_records_ == 0U)
                && scanners_ && (j_ < scanners_->size())) {
            if (const auto p = (*scanners_)[j_]) {
                return std::addressof(*p);
            }
        }
        return nullptr;
    }

    Ch* uc(const Ch* s) const
    {
        const auto tb = true_buffer();
        return tb + (s - tb);
    }

    void remove_header_field_scanner()
    {
        destroy_deallocate(header_field_scanner_);
        header_field_scanner_ = nullptr;
    }
};

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
        using result_t = std::remove_const_t<decltype(r)>;

        const auto has_postfix =
            std::find_if<const Ch*>(middle, end, [](Ch c) {
                return !is_space(c);
            }) != end;
        if (has_postfix) {
            // if a not-whitespace-extra-character found, it is NG
            return static_cast<result_t>(get_conversion_error_handler().
                invalid_format(begin, end));
        } else if (begin == middle) {
            // whitespace only
            return static_cast<result_t>(get_conversion_error_handler().
                empty());
        } else if (errno == ERANGE) {
            using limits_t =
                std::numeric_limits<std::remove_const_t<decltype(r)>>;
            const int s = (r == limits_t::max()) ? 1 :
                          (r == limits_t::lowest()) ? -1 : 0;
            return static_cast<result_t>(get_conversion_error_handler().
                out_of_range(begin, end, s));
        } else {
            return r;
        }
    }

    H& get_conversion_error_handler() noexcept
    {
        return this->get();
    }

    const H& get_conversion_error_handler() const noexcept
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
struct raw_converter;

// For integral types
template <class T, class H>
struct raw_converter<T, H, std::enable_if_t<std::is_integral<T>::value,
        void_t<decltype(numeric_type_traits<T>::strto)>>> :
    raw_converter_base<raw_converter<T, H>, H>
{
    using raw_converter_base<raw_converter<T, H>, H>
        ::raw_converter_base;

    auto engine(const char* s, char** e) const
    {
        return numeric_type_traits<T>::strto(s, e, 10);
    }

    auto engine(const wchar_t* s, wchar_t** e) const
    {
        return numeric_type_traits<T>::wcsto(s, e, 10);
    }
};

// For floating-point types
template <class T, class H>
struct raw_converter<T, H, std::enable_if_t<std::is_floating_point<T>::value,
        void_t<decltype(numeric_type_traits<T>::strto)>>> :
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
};

// For types without corresponding "raw_type"
template <class T, class H, class = void>
struct converter :
    private raw_converter<T, H>
{
    using raw_converter<T, H>::raw_converter;
    using raw_converter<T, H>::get_conversion_error_handler;

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
    using raw_converter<U, H>::raw_converter;
    using raw_converter<U, H>::get_conversion_error_handler;

    template <class Ch>
    T convert(const Ch* begin, const Ch* end)
    {
        const auto result = this->convert_raw(begin, end);
        if (result < std::numeric_limits<T>::lowest()) {
            return this->get_conversion_error_handler().
                out_of_range(begin, end, -1);
        } else if (std::numeric_limits<T>::max() < result) {
            return this->get_conversion_error_handler().
                out_of_range(begin, end, 1);
        } else {
            return static_cast<T>(result);
        }
    }
};

template <class T, class H, class U>
struct restrained_converter<T, H, U,
        std::enable_if_t<std::is_unsigned<T>::value>> :
    private raw_converter<U, H>
{
    using raw_converter<U, H>::raw_converter;
    using raw_converter<U, H>::get_conversion_error_handler;

    template <class Ch>
    T convert(const Ch* begin, const Ch* end)
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
        return this->get_conversion_error_handler().out_of_range(
            begin, end, 1);
    }
};

// For types which have corresponding "raw_type"
template <class T, class H>
struct converter<T, H, void_t<typename numeric_type_traits<T>::raw_type>> :
    restrained_converter<T, H, typename numeric_type_traits<T>::raw_type>
{
    using restrained_converter<T, H, typename numeric_type_traits<T>::raw_type>
        ::restrained_converter;
};

} // end namespace detail

template <class T>
struct fail_if_skipped
{
    [[noreturn]]
    T operator()() const
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

    T operator()() const
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
        s << ": cannot convert";
        write_name<T>(s, " to an instance of ");
        throw field_invalid_format(s.str());
    }

    template <class Ch>
    [[noreturn]]
    T out_of_range(const Ch* begin, const Ch* end, int /*sign*/) const
    {
        assert(*end == Ch());
        std::ostringstream s;
        narrow(s, begin, end);
        s << ": out of range";
        write_name<T>(s, " of ");
        throw field_out_of_range(s.str());
    }

    [[noreturn]]
    T empty() const
    {
        std::ostringstream s;
        s << "Cannot convert an empty string";
        write_name<T>(s, " to an instance of ");
        throw field_empty(s.str());
    }

private:
    template <class U>
    static std::ostream& write_name(std::ostream& os, const char* prefix,
        decltype(detail::numeric_type_traits<U>::name)* = nullptr)
    {
        return os << prefix << detail::numeric_type_traits<T>::name;
    }

    template <class U>
    static std::ostream& write_name(std::ostream& os, ...)
    {
        return os;
    }

    template <class Ch>
    static void narrow(std::ostream& os, const Ch* begin, const Ch* end)
    {
        const auto& facet =
            std::use_facet<std::ctype<Ch>>(std::locale());  // current global
        while (begin != end) {
            const auto c = *begin;
            if (c == Ch()) {
                os << '[' << set_hex << setw_char<Ch> << 0 << ']';
            } else if (!facet.is(std::ctype<Ch>::print, c)) {
                os << '[' << set_hex << setw_char<Ch> << (c + 0) << ']';
            } else {
                const auto d = facet.narrow(c, '\0');
                if (d == '\0') {
                    os << '[' << set_hex << setw_char<Ch> << (c + 0) << ']';
                } else {
                    os.rdbuf()->sputc(d);
                }
            }
            ++begin;
        }
    }

    static void narrow(std::ostream& os, const char* begin, const char* end)
    {
        // [begin, end] may be a NTMBS, so we cannot determine whether a char
        // is an unprintable one or not easily
        while (begin != end) {
            const auto c = *begin;
            if (c == '\0') {
                os << '[' << set_hex << setw_char<char> << 0 << ']';
            } else {
                os.rdbuf()->sputc(c);
            }
            ++begin;
        }
    }

    static std::ostream& set_hex(std::ostream& os)
    {
        return os << std::showbase << std::hex << std::setfill('0');
    }

    template <class Ch>
    static std::ostream& setw_char(std::ostream& os)
    {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127)
#endif
        constexpr auto m =
            std::numeric_limits<std::make_unsigned_t<Ch>>::max();
        if (m <= 0xff) {
            os << std::setw(2);
        } else if (m <= 0xffff) {
            os << std::setw(4);
        } else if (m <= 0xffffffff) {
            os << std::setw(8);
        }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        return os;
    }
};

template <class T>
class replace_if_conversion_failed
{
    static constexpr unsigned replacement_empty = 0;
    static constexpr unsigned replacement_invalid_format = 1;
    static constexpr unsigned replacement_above_upper_limit = 2;
    static constexpr unsigned replacement_below_lower_limit = 3;
    static constexpr unsigned replacement_underflow = 4;
    static constexpr unsigned replacement_n = 5;

    std::aligned_storage_t<sizeof(T), alignof(T)> replacements_[replacement_n];
    std::int_fast8_t has_;

private:
    template <class U>
    using is_affordable =
        std::integral_constant<
            bool,
            std::is_constructible<T, U>::value
         || std::is_same<U, std::nullptr_t>::value>;

public:
    template <
        class Empty = std::nullptr_t,
        class InvalidFormat = std::nullptr_t,
        class AboveUpperLimit = std::nullptr_t,
        class BelowLowerLimit = std::nullptr_t,
        class Underflow = std::nullptr_t,
        std::enable_if_t<
            is_affordable<Empty>::value
         && is_affordable<InvalidFormat>::value
         && is_affordable<AboveUpperLimit>::value
         && is_affordable<BelowLowerLimit>::value
         && is_affordable<Underflow>::value,
            std::nullptr_t> = nullptr>
    explicit replace_if_conversion_failed(
        Empty on_empty = Empty(),
        InvalidFormat on_invalid_format = InvalidFormat(),
        AboveUpperLimit on_above_upper_limit = AboveUpperLimit(),
        BelowLowerLimit on_below_lower_limit = BelowLowerLimit(),
        Underflow on_underflow = Underflow()) noexcept :
        has_(0)
    {
        set(replacement_empty, on_empty);
        set(replacement_invalid_format, on_invalid_format);
        set(replacement_above_upper_limit, on_above_upper_limit);
        set(replacement_below_lower_limit, on_below_lower_limit);
        set(replacement_underflow, on_underflow);
    }

    replace_if_conversion_failed(const replace_if_conversion_failed& other) :
        replace_if_conversion_failed(
            other, std::is_trivially_copyable<T>())
    {}

    replace_if_conversion_failed(replace_if_conversion_failed&& other)
        noexcept(std::is_nothrow_move_constructible<T>::value) :
        replace_if_conversion_failed(
            std::move(other), std::is_trivially_copyable<T>())
    {}

private:
    replace_if_conversion_failed(const replace_if_conversion_failed& other,
        std::true_type) noexcept :
        has_(other.has_)
    {
        std::memcpy(&replacements_, &other.replacements_,
            sizeof replacements_);
    }

    replace_if_conversion_failed(const replace_if_conversion_failed& other,
        std::false_type) :
        has_(0)
    {
        for (unsigned i = 0; i < replacement_n; ++i) {
            if (const auto p = other.get(i)) {
                set(i, *p);
            }
        }
    }

    replace_if_conversion_failed(replace_if_conversion_failed&& other,
        std::false_type)
        noexcept(std::is_nothrow_move_constructible<T>::value) :
        has_(0)
    {
        for (unsigned i = 0; i < replacement_n; ++i) {
            if (const auto p = other.get(i)) {
                set(i, std::move(*p));  // ?
                other.reset(i);
            }
        }
    }

public:
    ~replace_if_conversion_failed()
    {
        destroy(std::is_trivially_destructible<T>());
    }

private:
    void destroy(std::true_type)
    {}

    void destroy(std::false_type)
    {
        for (unsigned i = 0; i < replacement_n; ++i) {
            if (const auto p = get(i)) {
                destroy(i);
            }
        }
    }

public:
    replace_if_conversion_failed& operator=(
        const replace_if_conversion_failed& other)
    {
        assign(other, std::is_trivially_copyable<T>());
        return *this;
    }

    replace_if_conversion_failed& operator=(
        replace_if_conversion_failed&& other)
        noexcept(std::is_nothrow_move_constructible<T>::value)
    {
        assign(std::move(other), std::is_trivially_copyable<T>());
        return *this;
    }

private:
    void assign(const replace_if_conversion_failed& other,
        std::true_type) noexcept
    {
        std::memcpy(&replacements_, &other.replacements_,
            sizeof replacements_);
        has_ = other.has_;
    }

    void assign(const replace_if_conversion_failed& other, std::false_type)
    {
        for (unsigned i = 0; i < replacement_n; ++i) {
            const auto p = other.get(i);
            if (const auto q = get(i)) {
                if (p) {
                    *q = *p;
                } else {
                    reset(i);
                }
            } else if (p) {
                set(i, *p);
            }
        }
    }

    void assign(replace_if_conversion_failed&& other, std::false_type)
        noexcept(std::is_nothrow_move_constructible<T>::value)
    {
        for (unsigned i = 0; i < replacement_n; ++i) {
            const auto p = other.get(i);
            if (const auto q = get(i)) {
                if (p) {
                    *q = std::move(*p);
                    other.reset(i);
                } else {
                    reset(i);
                }
            } else if (p) {
                set(i, std::move(*p));
                other.reset(i);
            }
        }
    }

public:
    template <class Ch>
    T invalid_format(const Ch* begin, const Ch* end) const
    {
        if (const auto p = get(replacement_invalid_format)) {
            return *p;
        } else {
            return fail_if_conversion_failed<T>().invalid_format(begin, end);
        }
    }

    template <class Ch>
    T out_of_range(const Ch* begin, const Ch* end, int sign) const
    {
        if (sign > 0) {
            if (const auto p = get(replacement_above_upper_limit)) {
                return *p;
            }
        } else if (sign < 0) {
            if (const auto p = get(replacement_below_lower_limit)) {
                return *p;
            }
        } else if (const auto p = get(replacement_underflow)) {
            return *p;
        }
        return fail_if_conversion_failed<T>().out_of_range(begin, end, sign);
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
    template <class U>
    void set(unsigned r, U&& value)
    {
        ::new(&replacements_[r]) T(std::forward<U>(value));
        has_ |= 1 << r;
    }

    void set(unsigned, std::nullptr_t) noexcept
    {}

    void reset(unsigned r) noexcept
    {
        destroy(r);
        has_ &= ~(1 << r);
    }

    void destroy(unsigned r) noexcept
    {
        reinterpret_cast<T*>(&replacements_[r])->~T();
    }

    const T* get(unsigned r) const noexcept
    {
        if (has_ & (1 << r)) {
            return reinterpret_cast<const T*>(&replacements_[r]);
        } else {
            return nullptr;
        }
    }

    T* get(unsigned r) noexcept
    {
        if (has_ & (1 << r)) {
            return reinterpret_cast<T*>(&replacements_[r]);
        } else {
            return nullptr;
        }
    }
};

namespace detail {

template <class T, class = void>
struct is_output_iterator : std::false_type
{};

template <class T>
struct is_output_iterator<T,
    std::enable_if_t<
        std::is_base_of<
            std::output_iterator_tag,
            typename std::iterator_traits<T>::iterator_category>::value
     || std::is_base_of<
            std::forward_iterator_tag,
            typename std::iterator_traits<T>::iterator_category>::value>> :
    std::true_type
{};

template <class Sink, class SkippingHandler>
class translator :
    member_like_base<SkippingHandler>
{
    Sink sink_;

public:
    translator(Sink&& sink, SkippingHandler&& handle_skipping) :
        member_like_base<SkippingHandler>(std::move(handle_skipping)),
        sink_(std::move(sink))
    {}

    const SkippingHandler& get_skipping_handler() const noexcept
    {
        return this->get();
    }

    SkippingHandler& get_skipping_handler() noexcept
    {
        return this->get();
    }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4702)
#endif
    void field_skipped()
    {
        put(get_skipping_handler()());
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    }

protected:
    template <class T>
    void put(T&& value)
    {
        do_put(std::forward<T>(value), is_output_iterator<Sink>());
    }

private:
    template <class T>
    void do_put(T&& value, std::true_type)
    {
        *sink_ = std::forward<T>(value);
        ++sink_;
    }

    template <class T>
    void do_put(T&& value, std::false_type)
    {
        sink_(std::forward<T>(value));
    }
};

} // end namespace detail

template <class T, class Sink,
    class SkippingHandler = fail_if_skipped<T>,
    class ConversionErrorHandler = fail_if_conversion_failed<T>>
class arithmetic_field_translator :
    detail::converter<T, ConversionErrorHandler>,
    detail::translator<Sink, SkippingHandler>
{
public:
    explicit arithmetic_field_translator(
        Sink sink,
        SkippingHandler handle_skipping = SkippingHandler(),
        ConversionErrorHandler handle_conversion_error
            = ConversionErrorHandler()) :
        detail::converter<T, ConversionErrorHandler>(
            std::move(handle_conversion_error)),
        detail::translator<Sink, SkippingHandler>(
            std::move(sink), std::move(handle_skipping))
    {}

    arithmetic_field_translator(const arithmetic_field_translator&) = default;
    arithmetic_field_translator(arithmetic_field_translator&&) = default;
    ~arithmetic_field_translator() = default;
    arithmetic_field_translator& operator=(const arithmetic_field_translator&)
        = default;
    arithmetic_field_translator& operator=(arithmetic_field_translator&&)
        = default;

    using detail::translator<Sink, SkippingHandler>::get_skipping_handler;
    using detail::translator<Sink, SkippingHandler>::field_skipped;
    using detail::converter<T, ConversionErrorHandler>::
        get_conversion_error_handler;

    template <class Ch>
    void field_value(const Ch* begin, const Ch* end)
    {
        assert(*end == Ch());
        this->put(this->convert(begin, end));
    }
};

template <class T, class Sink,
    class SkippingHandler = fail_if_skipped<T>,
    class ConversionErrorHandler = fail_if_conversion_failed<T>>
class locale_based_arithmetic_field_translator :
    detail::converter<T, ConversionErrorHandler>,
    detail::translator<Sink, SkippingHandler>
{
    std::locale loc_;

    // These are initialized after parsing has started
    wchar_t thousands_sep_;     // of specified loc in the ctor
    wchar_t decimal_point_;     // of specified loc in the ctor
    wchar_t decimal_point_c_;   // of C's global
                                // to mimic std::strtol and its comrades

public:
    locale_based_arithmetic_field_translator(
        Sink sink, const std::locale& loc,
        SkippingHandler handle_skipping = SkippingHandler(),
        ConversionErrorHandler handle_conversion_error
            = ConversionErrorHandler()) :
        detail::converter<T, ConversionErrorHandler>(
            std::move(handle_conversion_error)),
        detail::translator<Sink, SkippingHandler>(
            std::move(sink), std::move(handle_skipping)),
        loc_(loc), decimal_point_c_()
    {}

    locale_based_arithmetic_field_translator(
        const locale_based_arithmetic_field_translator&) = default;
    locale_based_arithmetic_field_translator(
        locale_based_arithmetic_field_translator&&) = default;
    ~locale_based_arithmetic_field_translator() = default;
    locale_based_arithmetic_field_translator& operator=(
        const locale_based_arithmetic_field_translator&) = default;
    locale_based_arithmetic_field_translator& operator=(
        locale_based_arithmetic_field_translator&&) = default;

    using detail::translator<Sink, SkippingHandler>::get_skipping_handler;
    using detail::translator<Sink, SkippingHandler>::field_skipped;
    using detail::converter<T, ConversionErrorHandler>::
        get_conversion_error_handler;

    template <class Ch>
    void field_value(Ch* begin, Ch* end)
    {
        if (decimal_point_c_ == wchar_t()) {
            decimal_point_c_ = widen(*std::localeconv()->decimal_point, Ch());

            const auto& facet = std::use_facet<std::numpunct<Ch>>(loc_);
            thousands_sep_ = facet.grouping().empty() ?
                Ch() : facet.thousands_sep();
            decimal_point_ = facet.decimal_point();
        }
        assert(*end == Ch());   // shall be null-terminated
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
        this->put(this->convert(begin, head));
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
    class SkippingHandler =
        fail_if_skipped<std::basic_string<Ch, Tr, Allocator>>>
class string_field_translator :
    detail::member_like_base<Allocator>,
    detail::translator<Sink, SkippingHandler>
{
public:
    using allocator_type = Allocator;

    explicit string_field_translator(
        Sink sink,
        SkippingHandler handle_skipping = SkippingHandler()) :
        detail::translator<Sink, SkippingHandler>(
            std::move(sink), std::move(handle_skipping))
    {}

    string_field_translator(
        std::allocator_arg_t, const Allocator& alloc, Sink sink,
        SkippingHandler handle_skipping = SkippingHandler()) :
        detail::member_like_base<Allocator>(alloc),
        detail::translator<Sink, SkippingHandler>(
            std::move(sink), std::move(handle_skipping))
    {}

    string_field_translator(const string_field_translator&) = default;
    string_field_translator(string_field_translator&&) = default;
    ~string_field_translator() = default;
    string_field_translator& operator=(const string_field_translator&)
        = default;
    string_field_translator& operator=(string_field_translator&&)
        = default;

    allocator_type get_allocator() const noexcept
    {
        return detail::member_like_base<Allocator>::get();
    }

    using detail::translator<Sink, SkippingHandler>::get_skipping_handler;
    using detail::translator<Sink, SkippingHandler>::field_skipped;

    void field_value(const Ch* begin, const Ch* end)
    {
        this->put(std::basic_string<Ch, Tr, Allocator>(
            begin, end, get_allocator()));
    }

    void field_value(std::basic_string<Ch, Tr, Allocator>&& value)
    {
        // std::basic_string which comes with gcc 7.3.1 does not seem to have
        // "move-with-specified-allocator" ctor
        if (value.get_allocator() == get_allocator()) {
            this->put(std::move(value));
        } else {
            this->field_value(value.c_str(), value.c_str() + value.size());
        }
    }
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

template <class T, class Sink, class... Appendices,
    std::enable_if_t<
        !detail::is_std_string<T>::value
     && !std::is_base_of<
            std::locale, std::decay_t<detail::first_t<Appendices...>>>::value,
        std::nullptr_t> = nullptr>
auto make_field_translator_na(Sink sink, Appendices&&... appendices)
{
    return arithmetic_field_translator<T, Sink, Appendices...>(
        std::move(sink), std::forward<Appendices>(appendices)...);
}

template <class T, class Sink, class... Appendices,
    std::enable_if_t<!detail::is_std_string<T>::value, std::nullptr_t>
        = nullptr>
auto make_field_translator_na(
    Sink sink, const std::locale& loc, Appendices&&... appendices)
{
    return locale_based_arithmetic_field_translator<
                T, Sink, Appendices...>(
            std::move(sink), loc, std::forward<Appendices>(appendices)...);
}

template <class T, class Sink, class... Appendices,
    std::enable_if_t<detail::is_std_string<T>::value, std::nullptr_t>
        = nullptr>
auto make_field_translator_na(Sink sink, Appendices&&... appendices)
{
    return string_field_translator<Sink,
        typename T::value_type, typename T::traits_type,
        typename T::allocator_type, Appendices...>(
            std::move(sink), std::forward<Appendices>(appendices)...);
}

template <class A>
struct is_callable_impl
{
    template <class F>
    static auto check(F*) -> decltype(
        std::declval<F>()(std::declval<A>()),
        std::true_type());

    template <class F>
    static auto check(...) -> std::false_type;
};

template <class F, class A>
struct is_callable :
    decltype(is_callable_impl<A>::template check<F>(nullptr))
{};

} // end namespace detail

template <class T, class Sink, class... Appendices,
    std::enable_if_t<
        detail::is_output_iterator<Sink>::value
     || detail::is_callable<Sink, T>::value,
        std::nullptr_t> = nullptr>
auto make_field_translator(Sink sink, Appendices&&... appendices)
{
    return detail::make_field_translator_na<T>(
        std::move(sink), std::forward<Appendices>(appendices)...);
}

template <class T, class Allocator, class Sink, class... Appendices,
    std::enable_if_t<
        detail::is_output_iterator<Sink>::value
     || detail::is_callable<Sink, T>::value,
        std::nullptr_t> = nullptr>
auto make_field_translator(std::allocator_arg_t, const Allocator& alloc,
    Sink sink, Appendices&&... appendices)
{
    return string_field_translator<Sink,
            typename T::value_type, typename T::traits_type,
            Allocator, Appendices...>(
        std::allocator_arg, alloc,
        std::move(sink), std::forward<Appendices>(appendices)...);
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
    std::enable_if_t<detail::is_back_insertable<Container>::value
                  || detail::is_insertable<Container>::value, std::nullptr_t>*
        = nullptr>
auto make_field_translator(Container& values, Appendices&&... appendices)
{
    return make_field_translator<typename Container::value_type>(
        detail::make_back_insert_iterator(
            values, detail::is_back_insertable<Container>()),
        std::forward<Appendices>(appendices)...);
}

template <class Allocator, class Container, class... Appendices,
    std::enable_if_t<detail::is_back_insertable<Container>::value
                  || detail::is_insertable<Container>::value, std::nullptr_t>*
        = nullptr>
auto make_field_translator(std::allocator_arg_t, const Allocator& alloc,
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
