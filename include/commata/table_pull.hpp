/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_43DAA7B6_A9E7_4410_9210_CAADC6EA9D6F
#define COMMATA_GUARD_43DAA7B6_A9E7_4410_9210_CAADC6EA9D6F

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "wrapper_handlers.hpp"

#include "detail/allocation_only_allocator.hpp"
#include "detail/member_like_base.hpp"

namespace commata {

enum class primitive_table_pull_state : std::uint_fast8_t
{
    eof,
    before_parse,
    start_record,
    end_record,
    update,
    finalize,
    empty_physical_line,
    start_buffer,
    end_buffer
};

enum class primitive_table_pull_handle : std::uint_fast8_t
{
    start_buffer        = 1,
    end_buffer          = 1 << 1,
    start_record        = 1 << 2,
    end_record          = 1 << 3,
    empty_physical_line = 1 << 4,
    update              = 1 << 5,
    finalize            = 1 << 6,
    all = static_cast<std::uint_fast8_t>(-1)
};

constexpr inline primitive_table_pull_handle operator|(
    primitive_table_pull_handle left, primitive_table_pull_handle right)
    noexcept
{
    using u_t = std::underlying_type_t<primitive_table_pull_handle>;
    return primitive_table_pull_handle(
        static_cast<u_t>(left) | static_cast<u_t>(right));
}

constexpr inline primitive_table_pull_handle& operator|=(
    primitive_table_pull_handle& left, primitive_table_pull_handle right)
    noexcept
{
    return left = left | right;
}

constexpr inline primitive_table_pull_handle operator&(
    primitive_table_pull_handle left, primitive_table_pull_handle right)
    noexcept
{
    using u_t = std::underlying_type_t<primitive_table_pull_handle>;
    return primitive_table_pull_handle(
        static_cast<u_t>(left) & static_cast<u_t>(right));
}

constexpr inline primitive_table_pull_handle& operator&=(
    primitive_table_pull_handle& left, primitive_table_pull_handle right)
    noexcept
{
    return left = left & right;
}

constexpr inline primitive_table_pull_handle operator^(
    primitive_table_pull_handle left, primitive_table_pull_handle right)
    noexcept
{
    using u_t = std::underlying_type_t<primitive_table_pull_handle>;
    return primitive_table_pull_handle(
        static_cast<u_t>(left) ^ static_cast<u_t>(right));
}

constexpr inline primitive_table_pull_handle& operator^=(
    primitive_table_pull_handle& left, primitive_table_pull_handle right)
    noexcept
{
    return left = left ^ right;
}

constexpr inline primitive_table_pull_handle operator~(
    primitive_table_pull_handle handle) noexcept
{
    using u_t = std::underlying_type_t<primitive_table_pull_handle>;
    return primitive_table_pull_handle(~static_cast<u_t>(handle));
}

namespace detail::pull {

struct has_get_physical_position_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<std::pair<std::size_t, std::size_t>&>() =
            std::declval<const T&>().get_physical_position(),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

template <class T>
constexpr bool has_get_physical_position_v =
    decltype(has_get_physical_position_impl::check<T>(nullptr))();

struct has_nothrow_get_physical_position_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        noexcept(std::declval<std::pair<std::size_t, std::size_t>&>() =
            std::declval<const T&>().get_physical_position()),
        std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

template <class T>
constexpr bool has_nothrow_get_physical_position_v =
    decltype(has_nothrow_get_physical_position_impl::check<T>(nullptr))();

template <class Ch, class Allocator, primitive_table_pull_handle Handle>
class handler
{
public:
    using char_type = Ch;
    using state_queue_element_type =
        std::pair<primitive_table_pull_state, std::uint_fast8_t>;

private:
    using at_t = std::allocator_traits<Allocator>;

    using state_queue_a_t = allocation_only_allocator<
        typename at_t::template rebind_alloc<state_queue_element_type>>;
    using data_queue_a_t = allocation_only_allocator<
        typename at_t::template rebind_alloc<Ch*>>;

public:
    using state_queue_type =
        std::vector<state_queue_element_type, state_queue_a_t>;
    using data_queue_type = std::vector<char_type*, data_queue_a_t>;

private:
    state_queue_type sq_;
    data_queue_type dq_;
    std::size_t yield_location_;
    bool collects_data_;

public:
    handler(std::allocator_arg_t, const Allocator& alloc) :
        sq_(state_queue_a_t(alloc)), dq_(data_queue_a_t(alloc)),
        yield_location_(0), collects_data_(true)
    {}

    handler(const handler& other) = delete;
    handler(handler&& other) = delete;

    ~handler() = default;

    const state_queue_type& state_queue() const noexcept
    {
        return sq_;
    }

    state_queue_type& state_queue() noexcept
    {
        return sq_;
    }

    const data_queue_type& data_queue() const noexcept
    {
        return dq_;
    }

    data_queue_type& data_queue() noexcept
    {
        return dq_;
    }

    bool is_discarding_data() const noexcept
    {
        return !collects_data_;
    }

    handler& set_discarding_data(bool b = true) noexcept
    {
        collects_data_ = !b;
        return *this;
    }

    void start_buffer(
        [[maybe_unused]] char_type* buffer_begin,
        [[maybe_unused]] char_type* buffer_end)
    {
        if constexpr (handles(primitive_table_pull_handle::start_buffer)) {
            if (collects_data_) {
                sq_.emplace_back(
                    primitive_table_pull_state::start_buffer, dn(2));
                dq_.push_back(buffer_begin);
                dq_.push_back(buffer_end);
            } else {
                sq_.emplace_back(
                    primitive_table_pull_state::start_buffer, dn(0));
            }
        }
    }

    void end_buffer([[maybe_unused]] char_type* buffer_end)
    {
        if constexpr (handles(primitive_table_pull_handle::end_buffer)) {
            if (collects_data_) {
                sq_.emplace_back(
                    primitive_table_pull_state::end_buffer, dn(1));
                dq_.push_back(buffer_end);
            } else {
                sq_.emplace_back(
                    primitive_table_pull_state::end_buffer, dn(0));
            }
        }
    }

    void start_record([[maybe_unused]] char_type* record_begin)
    {
        if constexpr (handles(primitive_table_pull_handle::start_record)) {
            if (collects_data_) {
                sq_.emplace_back(
                    primitive_table_pull_state::start_record, dn(1));
                dq_.push_back(record_begin);
            } else {
                sq_.emplace_back(
                    primitive_table_pull_state::start_record, dn(0));
            }
        }
    }

    void update(
        [[maybe_unused]] char_type* first,
        [[maybe_unused]] char_type* last)
    {
        if constexpr (handles(primitive_table_pull_handle::update)) {
            if (collects_data_) {
                sq_.emplace_back(primitive_table_pull_state::update, dn(2));
                dq_.push_back(first);
                dq_.push_back(last);
            } else {
                sq_.emplace_back(primitive_table_pull_state::update, dn(0));
            }
        }
    }

    void finalize(
        [[maybe_unused]] char_type* first,
        [[maybe_unused]] char_type* last)
    {
        if constexpr (handles(primitive_table_pull_handle::finalize)) {
            if (collects_data_) {
                sq_.emplace_back(primitive_table_pull_state::finalize, dn(2));
                dq_.push_back(first);
                dq_.push_back(last);
            } else {
                sq_.emplace_back(primitive_table_pull_state::finalize, dn(0));
            }
        }
    }

    void end_record([[maybe_unused]] char_type* record_end)
    {
        if constexpr (handles(primitive_table_pull_handle::end_record)) {
            if (collects_data_) {
                sq_.emplace_back(
                    primitive_table_pull_state::end_record, dn(1));
                dq_.push_back(record_end);
            } else {
                sq_.emplace_back(
                    primitive_table_pull_state::end_record, dn(0));
            }
        }
    }

    void empty_physical_line([[maybe_unused]] char_type* where)
    {
        if constexpr (
                handles(primitive_table_pull_handle::empty_physical_line)) {
            if (collects_data_) {
                sq_.emplace_back(
                    primitive_table_pull_state::empty_physical_line, dn(1));
                dq_.push_back(where);
            } else {
                sq_.emplace_back(
                    primitive_table_pull_state::empty_physical_line, dn(0));
            }
        }
    }

    bool yield(std::size_t location) noexcept
    {
        if ((location != static_cast<std::size_t>(-1)) && sq_.empty()) {
            return false;
        } else {
            yield_location_ = location;
            return true;
        }
    }

    std::size_t yield_location() const noexcept
    {
        return yield_location_;
    }

    static constexpr bool handles(primitive_table_pull_handle handle) noexcept
    {
        return (Handle & handle) != ~primitive_table_pull_handle::all;
    }

private:
    template <class N>
    static auto dn(N n)
    {
        return static_cast<typename state_queue_element_type::second_type>(n);
    }
};

template <class D, class Ch, class S>
struct primitive_table_pull_base_const
{
    const Ch* operator[](S i) const
    {
        return static_cast<const D*>(this)->access_impl(i);
    }

    const Ch* at(S i) const
    {
        return static_cast<const D*>(this)->at_impl(i);
    }
};

template <class D, class Ch, class S>
struct primitive_table_pull_base_nonconst :
    primitive_table_pull_base_const<D, Ch, S>
{
    using primitive_table_pull_base_const<D, Ch, S>::operator[];
    using primitive_table_pull_base_const<D, Ch, S>::at;

    Ch* operator[](S i)
    {
        return const_cast<Ch*>(std::as_const(*this)[i]);
    }

    Ch* at(S i)
    {
        return const_cast<Ch*>(std::as_const(*this).at(i));
    }
};

struct reads_direct_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<bool&>() = T::reads_direct::value,
        typename T::reads_direct());

    template <class T>
    static auto check(...) -> std::false_type;
};

template <class T>
constexpr bool reads_direct_v =
    decltype(reads_direct_impl::check<T>(nullptr))::value;

template <class TableSource, primitive_table_pull_handle Handle,
          class Allocator, bool const_qualified>
constexpr bool reads_direct_with_handler = reads_direct_v<
    typename TableSource::template parser_type<reference_handler<handler<
        std::conditional_t<const_qualified,
            const typename TableSource::char_type,
            typename TableSource::char_type>,
        Allocator, Handle>>>>;

} // end detail::pull

template <class TableSource,
    primitive_table_pull_handle Handle = primitive_table_pull_handle::all,
    class Allocator = std::allocator<typename TableSource::char_type>>
class primitive_table_pull :
    public std::conditional_t<
        detail::pull::reads_direct_with_handler<
            TableSource, Handle, Allocator, false>,
        detail::pull::primitive_table_pull_base_nonconst<
            primitive_table_pull<TableSource, Handle, Allocator>,
            typename TableSource::char_type, std::size_t>,
        std::conditional_t<
            detail::pull::reads_direct_with_handler<
                TableSource, Handle, Allocator, true>,
                detail::pull::primitive_table_pull_base_const<
                    primitive_table_pull<TableSource, Handle, Allocator>,
                    typename TableSource::char_type, std::size_t>,
                detail::pull::primitive_table_pull_base_nonconst<
                    primitive_table_pull<TableSource, Handle, Allocator>,
                    typename TableSource::char_type, std::size_t>>>
{
public:
    using char_type = std::conditional_t<
        detail::pull::reads_direct_with_handler<
            TableSource, Handle, Allocator, false>,
        typename TableSource::char_type,
        std::conditional_t<
            detail::pull::reads_direct_with_handler<
                TableSource, Handle, Allocator, true>,
            const typename TableSource::char_type,
            typename TableSource::char_type>>;
    using allocator_type = Allocator;
    using size_type = std::size_t;

private:
    // To get get() called; only one of them should suffice but we just can't
    // get over the esotericism of friend decls
    friend struct detail::pull::primitive_table_pull_base_const<
        primitive_table_pull, typename TableSource::char_type, std::size_t>;
    friend struct detail::pull::primitive_table_pull_base_nonconst<
        primitive_table_pull, typename TableSource::char_type, std::size_t>;

    using at_t = std::allocator_traits<Allocator>;

    using handler_t = detail::pull::handler<char_type, Allocator, Handle>;
    using handler_a_t = detail::allocation_only_allocator<
        typename at_t::template rebind_alloc<handler_t>>;
    using handler_at_t = std::allocator_traits<handler_a_t>;
    using handler_p_t = typename handler_at_t::pointer;

    static_assert(std::is_same_v<
        decltype(std::declval<const TableSource&>()(
            std::declval<reference_handler<handler_t>>())),
        decltype(std::declval<TableSource>()(
            std::declval<reference_handler<handler_t>>()))>);

    using parser_t = typename TableSource::template parser_type<
        reference_handler<handler_t>>;

    std::size_t i_sq_;
    std::size_t i_dq_;
    handler_p_t handler_;
    typename handler_t::state_queue_type* sq_;
    typename handler_t::data_queue_type* dq_;
    detail::base_member_pair<allocator_type, parser_t> ap_;

    static typename handler_t::state_queue_type sq_moved_from;
    static typename handler_t::data_queue_type dq_moved_from;

public:
    static constexpr bool physical_position_available =
        detail::pull::has_get_physical_position_v<parser_t>;

    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    template <class TableSourceR,
        std::enable_if_t<
            std::is_base_of_v<TableSource, std::decay_t<TableSourceR>>
         && !std::is_base_of_v<primitive_table_pull,
                               std::decay_t<TableSourceR>>>* = nullptr>
    explicit primitive_table_pull(
        TableSourceR&& in, std::size_t buffer_size = 0) :
        primitive_table_pull(std::allocator_arg, Allocator(),
            std::forward<TableSourceR>(in), buffer_size)
    {}

    template <class TableSourceR,
        std::enable_if_t<
            std::is_base_of_v<TableSource, std::decay_t<TableSourceR>>>*
        = nullptr>
    primitive_table_pull(std::allocator_arg_t, const Allocator& alloc,
        TableSourceR&& in, std::size_t buffer_size = 0) :
        i_sq_(0), i_dq_(0),
        handler_(create_handler(alloc, std::allocator_arg, alloc)),
        sq_(&handler_->state_queue()), dq_(&handler_->data_queue()),
        ap_(alloc, std::forward<TableSourceR>(in)(
                            wrap_ref(*handler_), buffer_size, alloc))
    {
        sq_->emplace_back(
            primitive_table_pull_state::before_parse,
            static_cast<typename handler_t::
                    state_queue_element_type::second_type>(0));
    }

    primitive_table_pull(primitive_table_pull&& other) noexcept :
        i_sq_(std::exchange(other.i_sq_, 0)),
        i_dq_(std::exchange(other.i_dq_, 0)),
        handler_(std::exchange(other.handler_, nullptr)),
        sq_(&handler_->state_queue()), dq_(&handler_->data_queue()),
        ap_(std::move(other.ap_))
    {
        other.sq_ = &sq_moved_from;
        other.dq_ = &dq_moved_from;
    }

    ~primitive_table_pull()
    {
        if (handler_) {
            destroy_handler(handler_);
        }
    }

    allocator_type get_allocator() const noexcept
    {
        return ap_.base();
    }

    bool is_discarding_data() const noexcept
    {
        return handler_ && handler_->is_discarding_data();
    }

    primitive_table_pull& set_discarding_data(bool b = true) noexcept
    {
        if (handler_) {
            handler_->set_discarding_data(b);
        }
        return *this;
    }

    primitive_table_pull_state state() const noexcept
    {
        assert(sq_->size() > i_sq_);
        return (*sq_)[i_sq_].first;
    }

    explicit operator bool() const noexcept
    {
        const auto s = state();
        return s != primitive_table_pull_state::eof;
    }

    primitive_table_pull& operator()()
    {
        assert(!sq_->empty());
        const auto dsize = (*sq_)[i_sq_].second;

        if (dsize > 0) {
            i_dq_ += dsize;
            if (i_dq_ == dq_->size()) {
                dq_->clear();
                i_dq_ = 0;
            }
        }

        ++i_sq_;
        if (i_sq_ == sq_->size()) {
            if (sq_ != std::addressof(sq_moved_from)) {
                sq_->clear();
            }
            i_sq_ = 0;
        }

        if (sq_->empty()) {
            ap_.member()();
            if (sq_->empty()) {
                sq_->emplace_back(
                    primitive_table_pull_state::eof,
                    static_cast<typename handler_t::state_queue_element_type::
                        second_type>(0));
            }
        }
        return *this;
    }

private:
    const char_type* access_impl(size_type i) const
    {
        assert(i < data_size());
        return (*dq_)[i_dq_ + i];
    }

    const char_type* at_impl(size_type i) const
    {
        const auto ds = data_size();
        if (i < ds) {
            return access_impl(i);
        } else {
            std::ostringstream what;
            what << "Too large suffix " << i
                 << ": its maximum value is " << (ds - 1);
            throw std::out_of_range(std::move(what).str());
        }
    }

public:
    size_type data_size() const noexcept
    {
        return (*sq_)[i_sq_].second;
    }

    size_type max_data_size() const noexcept
    {
        return handler_t::handles(
                    primitive_table_pull_handle::start_buffer
                  | primitive_table_pull_handle::update
                  | primitive_table_pull_handle::finalize) ?  2 :
               handler_t::handles(
                    primitive_table_pull_handle::end_buffer
                  | primitive_table_pull_handle::empty_physical_line
                  | primitive_table_pull_handle::start_record
                  | primitive_table_pull_handle::end_record) ? 1 :
               0;
    }

    std::pair<std::size_t, std::size_t> get_physical_position() const
        noexcept((!physical_position_available)
              || detail::pull::has_nothrow_get_physical_position_v<parser_t>)
    {
        if constexpr (physical_position_available) {
            return ap_.member().get_physical_position();
        } else {
            return { npos, npos };
        }
    }

private:
    template <class... Args>
    [[nodiscard]]
    static auto create_handler(const Allocator& alloc, Args&&... args)
    {
        handler_a_t a(alloc);
        const auto p = handler_at_t::allocate(a, 1);
        ::new(std::addressof(*p)) handler_t(std::forward<Args>(args)...);
        return p;
    }

    void destroy_handler(handler_p_t p)
    {
        p->~handler_t();
        handler_a_t a(get_allocator());
        handler_at_t::deallocate(a, p, 1);
    }
};

template <class TableSource, class... Args>
primitive_table_pull(TableSource, Args...)
    -> primitive_table_pull<TableSource>;

template <class TableSource, class Allocator, class... Args>
primitive_table_pull(std::allocator_arg_t, Allocator, TableSource, Args...)
    -> primitive_table_pull<
            TableSource, primitive_table_pull_handle::all, Allocator>;

template <class TableSource, primitive_table_pull_handle Handle,
            class Allocator>
typename primitive_table_pull<TableSource, Handle, Allocator>::
        handler_t::state_queue_type
primitive_table_pull<TableSource, Handle, Allocator>::
    sq_moved_from = { std::make_pair(
        primitive_table_pull_state::eof,
        typename primitive_table_pull<TableSource, Handle, Allocator>::
            handler_t::state_queue_element_type::second_type(0)) };

template <class TableSource, primitive_table_pull_handle Handle,
            class Allocator>
typename primitive_table_pull<TableSource, Handle, Allocator>::
        handler_t::data_queue_type
primitive_table_pull<TableSource, Handle, Allocator>::
    dq_moved_from = {};

enum class table_pull_state : std::uint_fast8_t
{
    eof,
    before_parse,
    field,
    record_end
};

namespace detail::pull {

template <class D, class Ch, class Tr>
struct table_pull_base_evading_copy
{};

template <class D, class Ch, class Tr>
struct table_pull_base_not_evading_copy
{
    const Ch* c_str() const noexcept
    {
        return static_cast<const D&>(*this)->data();
    }

    template <class F>
    D& rewrite(F f)
    {
        auto& view = const_cast<std::basic_string_view<Ch, Tr>&>(
                                                *static_cast<D&>(*this));
        auto* const begin = const_cast<Ch*>(view.data());
        auto* const end = begin + view.size();
        auto* const new_end = f(begin, end);
        if (new_end < end) {
            *new_end = Ch();
            view.remove_suffix(end - new_end);
        }
        return static_cast<D&>(*this);
    }
};

template <class TableSource, class Allocator>
using primitive_for_t = primitive_table_pull<
        TableSource,
        (primitive_table_pull_handle::end_buffer
       | primitive_table_pull_handle::end_record
       | primitive_table_pull_handle::empty_physical_line
       | primitive_table_pull_handle::update
       | primitive_table_pull_handle::finalize),
        Allocator>;

} // end detail::pull

template <class TableSource,
    class Allocator = std::allocator<typename TableSource::char_type>>
class table_pull :
    public std::conditional_t<
        std::is_const_v<typename detail::pull::primitive_for_t<
                                        TableSource, Allocator>::char_type>,
        detail::pull::table_pull_base_evading_copy<
            table_pull<TableSource, Allocator>,
            typename TableSource::char_type,
            typename TableSource::traits_type>,
        detail::pull::table_pull_base_not_evading_copy<
            table_pull<TableSource, Allocator>,
            typename TableSource::char_type,
            typename TableSource::traits_type>>
{
public:
    using char_type = typename TableSource::char_type;
    using traits_type = typename TableSource::traits_type;
    using allocator_type = Allocator;
    using view_type = std::basic_string_view<char_type, traits_type>;

private:
    using primitive_t = detail::pull::primitive_for_t<TableSource, Allocator>;
    using buffer_char_t = typename primitive_t::char_type;

    struct reset_discarding_data
    {
        void operator()(primitive_t* p) const
        {
            p->set_discarding_data(false);
        }
    };

    using temporarily_discard =
        std::unique_ptr<primitive_t, reset_discarding_data>;

private:
    primitive_t p_;
    bool empty_physical_line_aware_;

    table_pull_state state_;

    // current string value, arranged contiguously, followed by a null
    // character, and maintained also when value_ is in use
    view_type view_;
    // current string value, as a null terminated one, used only when it
    // cannot reside in current buffer; empty when not used
    std::vector<char_type, Allocator> value_;

    // num of "end record" events encountered
    std::size_t i_;
    // num of "finalize" events encountered in current record
    std::size_t j_;

public:
    static constexpr bool physical_position_available =
        primitive_t::physical_position_available;

    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    template <class TableSourceR,
        std::enable_if_t<
            std::is_base_of_v<TableSource, std::decay_t<TableSourceR>>
         && !std::is_base_of_v<table_pull, std::decay_t<TableSourceR>>>*
        = nullptr>
    explicit table_pull(TableSourceR&& in, std::size_t buffer_size = 0) :
        table_pull(std::allocator_arg, Allocator(),
            std::forward<TableSourceR>(in), buffer_size)
    {}

    template <class TableSourceR,
        std::enable_if_t<
            std::is_base_of_v<TableSource, std::decay_t<TableSourceR>>>*
        = nullptr>
    table_pull(std::allocator_arg_t, const Allocator& alloc,
        TableSourceR&& in, std::size_t buffer_size = 0) :
        p_(std::allocator_arg, alloc, std::forward<TableSourceR>(in),
            buffer_size),
        empty_physical_line_aware_(false),
        state_(table_pull_state::before_parse), view_(),
        value_(alloc), i_(0), j_(0)
    {}

    table_pull(table_pull&& other) noexcept :
        p_(std::move(other.p_)),
        empty_physical_line_aware_(other.empty_physical_line_aware_),
        state_(std::exchange(other.state_, table_pull_state::eof)),
        view_(std::exchange(other.view_, view_type())),
        value_(std::move(other.value_)),
        i_(std::exchange(other.i_, 0)),
        j_(std::exchange(other.j_, 0))
    {}

    ~table_pull() = default;

    allocator_type get_allocator() const noexcept
    {
        return value_.get_allocator();
    }

    bool is_empty_physical_line_aware() const noexcept
    {
        return empty_physical_line_aware_;
    }

    table_pull& set_empty_physical_line_aware(bool b = true) noexcept
    {
        empty_physical_line_aware_ = b;
        return *this;
    }

    table_pull_state state() const noexcept
    {
        return state_;
    }

    explicit operator bool() const noexcept
    {
        return state() != table_pull_state::eof;
    }

    std::pair<std::size_t, std::size_t> get_position() const noexcept
    {
        return std::make_pair(i_, j_);
    }

    std::pair<std::size_t, std::size_t> get_physical_position() const
        noexcept(noexcept(p_.get_physical_position()))
    {
        return p_.get_physical_position();
    }

    table_pull& operator()(std::size_t n = 0)
    {
        if (!*this) {
            return *this;
        }

        view_ = view_type();
        value_.clear();
        switch (state_) {
        case table_pull_state::field:
            ++j_;
            break;
        case table_pull_state::record_end:
            ++i_;
            j_ = 0;
            break;
        default:
            break;
        }

        if (n < 1) {
            try {
                next_field();                                       // throw
            } catch (...) {
                state_ = table_pull_state::eof;
                throw;
            }
            return *this;
        }

        p_.set_discarding_data(true);
        temporarily_discard d(&p_);
        try {
            for (;;) {
                switch (p_().state()) {                             // throw
                case primitive_table_pull_state::update:
                    break;
                case primitive_table_pull_state::finalize:
                    ++j_;
                    if (n == 1) {
                        d.reset();
                        next_field();                               // throw
                        return *this;
                    }
                    --n;
                    break;
                case primitive_table_pull_state::empty_physical_line:
                    if (!empty_physical_line_aware_) {
                        break;
                    }
                    [[fallthrough]];
                case primitive_table_pull_state::end_record:
                    state_ = table_pull_state::record_end;
                    return *this;
                case primitive_table_pull_state::eof:
                    state_ = table_pull_state::eof;
                    return *this;
                case primitive_table_pull_state::end_buffer:
                default:
                    break;
                }
            }
        } catch (...) {
            state_ = table_pull_state::eof;
            throw;
        }
    }

private:
    void next_field()
    {
        assert(*this);
        for (;;) {
            switch (p_().state()) {                             // throw
            case primitive_table_pull_state::update:
                do_update(p_[0], p_[1]);                        // throw
                break;
            case primitive_table_pull_state::finalize:
                do_update(p_[0], p_[1]);                        // throw
                if (value_.empty()) {
                    if constexpr (!std::is_const_v<char_type>) {
                        const_cast<char_type*>(view_.data())[view_.size()]
                            = char_type();
                    }
                } else {
                    if constexpr (std::is_const_v<char_type>) {
                        view_ = view_type(value_.data(), value_.size());
                    } else {
                        value_.push_back(char_type());          // throw
                        view_ = view_type(value_.data(), value_.size() - 1);
                    }
                }
                state_ = table_pull_state::field;
                return;
            case primitive_table_pull_state::empty_physical_line:
                if (!empty_physical_line_aware_) {
                    break;
                }
                [[fallthrough]];
            case primitive_table_pull_state::end_record:
                state_ = table_pull_state::record_end;
                view_ = view_type();
                return;
            case primitive_table_pull_state::end_buffer:
                if (!view_.empty()) {
                    value_.insert(value_.cend(),
                        view_.cbegin(), view_.cend());          // throw
                    view_ = view_type();
                }
                break;
            case primitive_table_pull_state::eof:
                state_ = table_pull_state::eof;
                view_ = view_type();
                return;
            default:
                break;
            }
        }
    }

    void do_update(buffer_char_t* first, buffer_char_t* last)
    {
        if (!value_.empty()) {
            value_.insert(value_.cend(), first, last);              // throw
        } else if (!view_.empty()) {
            const auto len = last - first;
            traits_type::move(
                const_cast<char_type*>(view_.data() + view_.size()),
                first, len);
            view_ = view_type(view_.data(), view_.size() + len);
        } else {
            view_ = view_type(first, last - first);
        }
    }

public:
    table_pull& skip_record(std::size_t n = 0)
    {
        if (!*this) {
            return *this;
        }

        view_ = view_type();
        value_.clear();

        if (state_ == table_pull_state::record_end) {
            ++i_;
            j_ = 0;
        }

        p_.set_discarding_data(true);
        temporarily_discard d(&p_);
        try {
            for (;;) {
                switch (p_().state()) {                             // throw
                case primitive_table_pull_state::update:
                    break;
                case primitive_table_pull_state::finalize:
                    state_ = table_pull_state::field;
                    ++j_;
                    break;
                case primitive_table_pull_state::empty_physical_line:
                    if (!empty_physical_line_aware_) {
                        break;
                    }
                    [[fallthrough]];
                case primitive_table_pull_state::end_record:
                    state_ = table_pull_state::record_end;
                    if (n == 0) {
                        return *this;
                    } else {
                        ++i_;
                        j_ = 0;
                        --n;
                        break;
                    }
                case primitive_table_pull_state::eof:
                    state_ = table_pull_state::eof;
                    return *this;
                case primitive_table_pull_state::end_buffer:
                default:
                    break;
                }
            }
        } catch (...) {
            state_ = table_pull_state::eof;
            throw;
        }
    }

    const view_type& operator*() const noexcept
    {
        return view_;
    }

    const view_type* operator->() const noexcept
    {
        return &view_;
    }
};

template <class TableSource, class... Args>
table_pull(TableSource, Args...) -> table_pull<TableSource>;

template <class TableSource, class Allocator, class... Args>
table_pull(std::allocator_arg_t, Allocator, TableSource, Args...)
    -> table_pull<TableSource, Allocator>;

template <class TableSource, class... Appendices>
[[nodiscard]]
auto make_table_pull(TableSource&& in, Appendices&&... appendices)
 -> std::enable_if_t<
        std::is_constructible_v<table_pull<std::decay_t<TableSource>>,
                                TableSource&&, Appendices&&...>,
        table_pull<std::decay_t<TableSource>>>
{
    return table_pull<std::decay_t<TableSource>>(
        std::forward<TableSource>(in),
        std::forward<Appendices>(appendices)...);
}

template <class TableSource, class Allocator, class... Appendices>
[[nodiscard]] auto make_table_pull(std::allocator_arg_t,
    const Allocator& alloc, TableSource&& in, Appendices&&... appendices)
 -> std::enable_if_t<
        std::is_constructible_v<
            table_pull<std::decay_t<TableSource>, Allocator>,
            std::allocator_arg_t, const Allocator&,
            TableSource&&, Appendices&&...>,
        table_pull<std::decay_t<TableSource>, Allocator>>
{
    return table_pull<std::decay_t<TableSource>, Allocator>(
        std::allocator_arg, alloc, std::forward<TableSource>(in),
        std::forward<Appendices>(appendices)...);
}

}

#endif
