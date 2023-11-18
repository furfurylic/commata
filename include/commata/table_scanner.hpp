/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_555AFEA0_404A_4BF3_AF38_4F653A0B76E6
#define COMMATA_GUARD_555AFEA0_404A_4BF3_AF38_4F653A0B76E6

#include <algorithm>
#include <cassert>
#include <clocale>
#include <cstddef>
#include <cwchar>
#include <locale>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <vector>

#include "allocation_only_allocator.hpp"
#include "buffer_size.hpp"
#include "replacement.hpp"
#include "text_error.hpp"
#include "member_like_base.hpp"
#include "text_value_translation.hpp"
#include "typing_aid.hpp"

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

    std::size_t j_ = 0;
    Ch* buffer_;
    Ch* begin_;
    Ch* end_;
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

    explicit basic_table_scanner(std::size_t header_record_count = 0U) :
        basic_table_scanner(std::allocator_arg, Allocator(),
            header_record_count)
    {}

    template <class HeaderFieldScanner,
        std::enable_if_t<!std::is_integral_v<HeaderFieldScanner>>* = nullptr>
    explicit basic_table_scanner(HeaderFieldScanner&& s) :
        basic_table_scanner(
            std::allocator_arg, Allocator(),
            std::forward<HeaderFieldScanner>(s))
    {}

    basic_table_scanner(
        std::allocator_arg_t, const Allocator& alloc,
        std::size_t header_record_count = 0U) :
        buffer_(),
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
        std::allocator_arg_t, const Allocator& alloc, HeaderFieldScanner&& s) :
        buffer_(), begin_(nullptr), value_(alloc),
        header_field_scanner_(allocate_construct<
            typed_header_field_scanner<std::decay_t<HeaderFieldScanner>>>(
                std::forward<HeaderFieldScanner>(s))),
        scanners_(scanners_a_t(bfs_ptr_p_a_t(alloc))), end_scanner_(nullptr)
    {}

    basic_table_scanner(basic_table_scanner&& other) noexcept :
        buffer_(other.buffer_),
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
    void start_buffer(Ch* begin, Ch*)
    {
        buffer_ = begin;
    }

    void end_buffer(Ch*)
    {
        if (begin_) {
            value_.assign(begin_, end_);                // throw
            begin_ = nullptr;
        }
    }

    void start_record(Ch* /*record_begin*/)
    {
        sj_ = 0;
        j_ = 0;
    }

    void update(Ch* first, Ch* last)
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

    void finalize(Ch* first, Ch* last)
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
                    *end_ = Ch();
                    scanner.first->field_value(begin_, end_, *this);
                }
                begin_ = nullptr;
            } else if (!value_.empty()) {
                value_.append(first, last);                         // throw
                scanner.first->field_value(std::move(value_), *this);
                value_.clear();
            } else {
                *last = Ch();
                scanner.first->field_value(first, last, *this);
            }
            if (scanner.second) {
                ++sj_;
            }
        }
        ++j_;
    }

    bool end_record(Ch* /*record_end*/)
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

class field_not_found : public text_value_translation_error
{
public:
    using text_value_translation_error::text_value_translation_error;
};

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
    using converter_t = detail::converter<T, ConversionErrorHandler>;
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
    wchar_t decimal_point_ = L'\0';     // of specified loc in the ctor
    wchar_t thousands_sep_ = L'\0';     // ditto
    wchar_t decimal_point_c_ = L'\0';   // of C's global to mimic
                                        // std::strtol and its comrades
    wchar_t thousands_sep_c_ = L'\0';   // ditto
    bool mimics_ = false;

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
        (detail::is_convertible_numeric_type_v<T>
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
        (detail::is_convertible_numeric_type_v<typename Container::value_type>
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
