/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_555AFEA0_404A_4BF3_AF38_4F653A0B76E6
#define COMMATA_GUARD_555AFEA0_404A_4BF3_AF38_4F653A0B76E6

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <vector>

#include "detail/allocate_deallocate.hpp"
#include "detail/allocation_only_allocator.hpp"
#include "detail/buffer_size.hpp"
#include "detail/full_ebo.hpp"
#include "detail/member_like_base.hpp"
#include "detail/typing_aid.hpp"

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
    bool operator()(std::size_t,
        std::optional<std::pair<Ch*, Ch*>> v, TableScanner&)
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
        virtual bool accepts_const() const noexcept = 0;
        virtual void field_value(
            Ch* begin, Ch* end, basic_table_scanner& me) = 0;
        virtual void field_value(
            const Ch* begin, const Ch* end, basic_table_scanner& me) = 0;
        virtual void field_value(
            string_t&& value, basic_table_scanner& me) = 0;
    };

    struct header_field_scanner : field_scanner
    {
        virtual void so_much_for_header(basic_table_scanner& me) = 0;
    };

    template <class HeaderScanner>
    class COMMATA_FULL_EBO typed_header_field_scanner :
        public header_field_scanner, detail::member_like_base<HeaderScanner>
    {
        static constexpr bool const_ready = std::is_invocable_v<HeaderScanner&,
            std::size_t, std::optional<std::pair<const Ch*, const Ch*>>,
                basic_table_scanner&>;

    public:
        template <class T>
        explicit typed_header_field_scanner(T&& s) :
            detail::member_like_base<HeaderScanner>(std::forward<T>(s))
        {}

        typed_header_field_scanner(typed_header_field_scanner&&) = delete;

        bool accepts_const() const noexcept override
        {
            return const_ready;
        }

        void field_value(Ch* begin, Ch* end, basic_table_scanner& me) override
        {
            field_value_impl(begin, end, me);
        }

        void field_value([[maybe_unused]] const Ch* begin,
                         [[maybe_unused]] const Ch* end,
                         basic_table_scanner& me) override
        {
            // Even when FieldScanner does not accept const params, this will
            // be instantiated, which will never be called at run-time...
            assert(accepts_const());
            if constexpr (const_ready) {
                field_value_impl(begin, end, me);
            }
        }

        void field_value(string_t&& value, basic_table_scanner& me) override
        {
            field_value(value.data(), value.data() + value.size(), me);
        }

        void so_much_for_header(basic_table_scanner& me) override
        {
            if constexpr (const_ready) {
                call(std::optional<std::pair<const Ch*, const Ch*>>(), me,
                    true);
            } else {
                call(std::optional<std::pair<Ch*, Ch*>>(), me, true);
            }
        }

    private:
        template <class C>
        void field_value_impl(C* begin, C* end, basic_table_scanner& me)
        {
            call(std::optional(std::pair(begin, end)), me, false);
        }

        template <class C>
        void call(std::optional<std::pair<C*, C*>> opt_range,
            basic_table_scanner& me, bool at_record_end)
        {
            if (!scanner()(me.j_, std::move(opt_range), me)) {
                me.remove_header_field_scanner(at_record_end);
            }
        }

        HeaderScanner& scanner()
        {
            return this->get();
        }
    };

    struct COMMATA_FULL_EBO body_field_scanner :
        field_scanner, detail::scanner::typable
    {
        virtual void field_skipped() = 0;
    };

    template <class FieldScanner>
    struct COMMATA_FULL_EBO typed_body_field_scanner :
        body_field_scanner, private detail::member_like_base<FieldScanner>
    {
        template <class T>
        explicit typed_body_field_scanner(T&& s) :
            detail::member_like_base<FieldScanner>(std::forward<T>(s))
        {}

        typed_body_field_scanner(typed_body_field_scanner&&) = delete;

        bool accepts_const() const noexcept override
        {
            return std::is_invocable_v<FieldScanner&, const Ch*, const Ch*>;
        }

        void field_value(Ch* begin, Ch* end, basic_table_scanner& me) override
        {
            field_value_impl(scanner(), begin, end, me);
        }

        void field_value(const Ch* begin, const Ch* end,
                basic_table_scanner& me) override
        {
            assert(accepts_const());
            field_value_impl(scanner(), begin, end, me);
        }

        void field_value(string_t&& value,
                [[maybe_unused]] basic_table_scanner& me) override
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
        template <class C>
        static void field_value_impl(FieldScanner& scanner,
            [[maybe_unused]] C* begin, [[maybe_unused]] C* end,
            [[maybe_unused]] basic_table_scanner& me)
        {
            // Even when FieldScanner accepts const params only, this will be
            // instantiated with C being non-const, which will never be called
            // at run-time...
            if constexpr (std::is_invocable_v<FieldScanner&, C*, C*>) {
                scanner(begin, end);
            } else if constexpr(std::is_invocable_v<FieldScanner&, string_t>) {
                scanner(string_t(begin, end, me.get_allocator()));
            } else {
                // ...so this branching is needed
                assert(false);
            }
        }

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
    struct COMMATA_FULL_EBO typed_record_end_scanner :
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
                return detail::invoke_returning_bool(f, me);
            } else {
                return detail::invoke_returning_bool(f);
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
    const Ch* buffer_;
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
    using char_type = const Ch;
    using traits_type = Tr;
    using allocator_type = Allocator;
    using size_type = typename std::allocator_traits<Allocator>::size_type;

    explicit basic_table_scanner(std::size_t header_record_count = 0U) :
        basic_table_scanner(std::allocator_arg, Allocator(),
            header_record_count)
    {}

    template <class HeaderFieldScanner,
        std::enable_if_t<!std::is_arithmetic_v<HeaderFieldScanner>>* = nullptr>
    explicit basic_table_scanner(HeaderFieldScanner&& s) :
        basic_table_scanner(
            std::allocator_arg, Allocator(),
            std::forward<HeaderFieldScanner>(s))
    {}

    basic_table_scanner(
        std::allocator_arg_t, const Allocator& alloc,
        std::size_t header_record_count = 0U) :
        buffer_(nullptr), begin_(nullptr), value_(alloc),
        header_field_scanner_(
            (header_record_count > 0) ?
            allocate_construct<
                typed_header_field_scanner<
                    detail::scanner::counting_header_field_scanner>>(
                        header_record_count) :
            nullptr),
        scanners_(scanners_a_t(bfs_ptr_p_a_t(alloc))), end_scanner_(nullptr)
    {}

    template <class HeaderFieldScanner,
        std::enable_if_t<!std::is_arithmetic_v<
            std::decay_t<HeaderFieldScanner>>>* = nullptr>
    basic_table_scanner(
        std::allocator_arg_t, const Allocator& alloc, HeaderFieldScanner&& s) :
        buffer_(nullptr), begin_(nullptr), value_(alloc),
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
    void start_buffer(const Ch* begin, const Ch*)
    {
        buffer_ = begin;
    }

    void end_buffer(const Ch*)
    {
        if (begin_) {
            value_.assign(begin_, end_);                // throw
            begin_ = nullptr;
        }
    }

    void start_record(const Ch* /*record_begin*/)
    {
        sj_ = 0;
        j_ = 0;
    }

    void update(Ch* first, Ch* last)
    {
        update_impl(first, last);
    }

    void update(const Ch* first, const Ch* last)
    {
        update_impl(first, last);
    }

    void finalize(Ch* first, Ch* last)
    {
        finalize_impl(first, last);
    }

    void finalize(const Ch* first, const Ch* last)
    {
        finalize_impl(first, last);
    }

private:
    template <class C>
    void update_impl(C* first, C* last)
    {
        if (const auto scanner = get_scanner(); scanner.first) {
            if constexpr (std::is_const_v<C>) {
                if (!scanner.first->accepts_const()) {
                    value_.append(first, last);                     // throw
                    return;
                }
            }
            if (!value_.empty()) {
                value_.append(first, last);                         // throw
            } else if (begin_) {
                value_.reserve((end_ - begin_) + (last - first));   // throw
                value_.assign(begin_, end_);
                value_.append(first, last);
                begin_ = nullptr;
            } else {
                begin_ = first;
                end_ = last;
            }
        }
    }

    template <class C>
    void finalize_impl(C* first, C* last)
    {
        if (const auto scanner = get_scanner(); scanner.first) {
            // Another function is introduced to avoid employing goto
            finalize_core(first, last, *scanner.first);
            if (scanner.second) {
                ++sj_;
            }
        }
        ++j_;
    }

    template <class C>
    void finalize_core(C* first, C* last, field_scanner& scanner)
    {
        if constexpr (std::is_const_v<C>) {
            if (!scanner.accepts_const()) {
                finalize_core_with_value(first, last, scanner);     // throw
                return;
            }
        }
        if (!value_.empty()) {
            finalize_core_with_value(first, last, scanner);         // throw
        } else if (begin_) {
            if (first != last) {
                value_.reserve((end_ - begin_) + (last - first));   // throw
                value_.assign(begin_, end_);
                finalize_core_with_value(first, last, scanner);     // throw
            } else if constexpr (std::is_const_v<C>) {
                scanner.field_value(begin_, end_, *this);
            } else {
                // Here we know it is the non-const version that is being
                // called, so const_casts below are safe
                using D = std::remove_const_t<C>;
                D* const begin = const_cast<D*>(begin_);
                D* const end = const_cast<D*>(end_);
                scanner.field_value(begin, end, *this);
            }
            begin_ = nullptr;
        } else {
            scanner.field_value(first, last, *this);
        }
    }

    template <class C>
    void finalize_core_with_value(C* first, C* last, field_scanner& scanner)
    {
        value_.append(first, last);                                 // throw
        scanner.field_value(std::move(value_), *this);
        value_.clear();
    }

public:
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
    template <class T, class... Args>
    [[nodiscard]] auto allocate_construct(Args&&... args)
    {
        return detail::allocate_construct_g<T>(
            get_allocator(), std::forward<Args>(args)...);  // throw
    }

    template <class P>
    void destroy_deallocate(P p)
    {
        detail::destroy_deallocate_g(get_allocator(), p);
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

}

#endif
