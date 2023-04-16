/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_43DAA7B6_A9E7_4410_9210_CAADC6EA9D6F
#define COMMATA_GUARD_43DAA7B6_A9E7_4410_9210_CAADC6EA9D6F

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "allocation_only_allocator.hpp"
#include "buffer_size.hpp"
#include "empty_string.hpp"
#include "formatted_output.hpp"
#include "member_like_base.hpp"
#include "string_value.hpp"
#include "wrapper_handlers.hpp"

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

enum primitive_table_pull_handle : std::uint_fast8_t
{
    primitive_table_pull_handle_start_buffer        = 1,
    primitive_table_pull_handle_end_buffer          = 1 << 1,
    primitive_table_pull_handle_start_record        = 1 << 2,
    primitive_table_pull_handle_end_record          = 1 << 3,
    primitive_table_pull_handle_empty_physical_line = 1 << 4,
    primitive_table_pull_handle_update              = 1 << 5,
    primitive_table_pull_handle_finalize            = 1 << 6,
    primitive_table_pull_handle_all = static_cast<std::uint_fast8_t>(-1)
};

namespace detail { namespace pull {

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
struct has_get_physical_position :
    decltype(has_get_physical_position_impl::check<T>(nullptr))
{};

template <class Ch, class Allocator,
    std::underlying_type_t<primitive_table_pull_handle> Handle>
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
        typename at_t::template rebind_alloc<char_type*>>;

public:
    using state_queue_type =
        std::vector<state_queue_element_type, state_queue_a_t>;
    using data_queue_type = std::vector<char_type*, data_queue_a_t>;

private:
    base_member_pair<Allocator, std::size_t> alloc_;
    const typename at_t::pointer buffer_;
    state_queue_type sq_;
    data_queue_type dq_;
    std::size_t yield_location_;
    bool collects_data_;

public:
    explicit handler(
        std::allocator_arg_t, const Allocator& alloc,
        std::size_t buffer_size) :
        alloc_(alloc, detail::sanitize_buffer_size(buffer_size, alloc)),
        buffer_(at_t::allocate(alloc_.base(), alloc_.member())),
        yield_location_(0), collects_data_(true)
    {}

    handler(const handler& other) = delete;
    handler(handler&& other) = delete;

    ~handler()
    {
        at_t::deallocate(alloc_.base(), buffer_, alloc_.member());
    }

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

    std::pair<char_type*, std::size_t> get_buffer()
    {
        return { std::addressof(*buffer_), alloc_.member() };
    }

    void release_buffer(const char_type*) noexcept
    {}

    void start_buffer(const Ch* buffer_begin, const Ch* buffer_end)
    {
        start_buffer(buffer_begin, buffer_end,
            std::integral_constant<bool,
                (Handle & primitive_table_pull_handle_start_buffer) != 0>());
    }

private:
    void start_buffer(const Ch* buffer_begin, const Ch* buffer_end,
        std::true_type)
    {
        if (collects_data_) {
            sq_.emplace_back(primitive_table_pull_state::start_buffer, dn(2));
            dq_.push_back(uc(buffer_begin));
            dq_.push_back(uc(buffer_end));
        } else {
            sq_.emplace_back(primitive_table_pull_state::start_buffer, dn(0));
        }
    }

    void start_buffer(const Ch*, const Ch*, std::false_type) noexcept
    {}

public:
    void end_buffer(const Ch* buffer_end)
    {
        end_buffer(buffer_end,
            std::integral_constant<bool,
                (Handle & primitive_table_pull_handle_end_buffer) != 0>());
    }

private:
    void end_buffer(const Ch* buffer_end, std::true_type)
    {
        if (collects_data_) {
            sq_.emplace_back(primitive_table_pull_state::end_buffer, dn(1));
            dq_.push_back(uc(buffer_end));
        } else {
            sq_.emplace_back(primitive_table_pull_state::end_buffer, dn(0));
        }
    }

    void end_buffer(const Ch*, std::false_type) noexcept
    {}

public:
    void start_record(const char_type* record_begin)
    {
        start_record(record_begin,
            std::integral_constant<bool,
                (Handle & primitive_table_pull_handle_start_record) != 0>());
    }

private:
    void start_record(const char_type* record_begin, std::true_type)
    {
        if (collects_data_) {
            sq_.emplace_back(primitive_table_pull_state::start_record, dn(1));
            dq_.push_back(uc(record_begin));
        } else {
            sq_.emplace_back(primitive_table_pull_state::start_record, dn(0));
        }
    }

    void start_record(const char_type*, std::false_type)
    {}

public:
    void update(const char_type* first, const char_type* last)
    {
        update(first, last,
            std::integral_constant<bool,
                (Handle & primitive_table_pull_handle_update) != 0>());
    }

private:
    void update(const char_type* first, const char_type* last,
        std::true_type)
    {
        if (collects_data_) {
            sq_.emplace_back(primitive_table_pull_state::update, dn(2));
            dq_.push_back(uc(first));
            dq_.push_back(uc(last));
        } else {
            sq_.emplace_back(primitive_table_pull_state::update, dn(0));
        }
    }

    void update(const char_type*, const char_type*, std::false_type)
    {}

public:
    void finalize(const char_type* first, const char_type* last)
    {
        finalize(first, last,
            std::integral_constant<bool,
                (Handle & primitive_table_pull_handle_finalize) != 0>());
    }

private:
    void finalize(const char_type* first, const char_type* last,
        std::true_type)
    {
        if (collects_data_) {
            sq_.emplace_back(primitive_table_pull_state::finalize, dn(2));
            dq_.push_back(uc(first));
            dq_.push_back(uc(last));
        } else {
            sq_.emplace_back(primitive_table_pull_state::finalize, dn(0));
        }
    }

    void finalize(const char_type*, const char_type*, std::false_type)
    {}

public:
    void end_record(const char_type* record_end)
    {
        end_record(record_end,
            std::integral_constant<bool,
                (Handle & primitive_table_pull_handle_end_record) != 0>());
    }

private:
    void end_record(const char_type* record_end, std::true_type)
    {
        if (collects_data_) {
            sq_.emplace_back(primitive_table_pull_state::end_record, dn(1));
            dq_.push_back(uc(record_end));
        } else {
            sq_.emplace_back(primitive_table_pull_state::end_record, dn(0));
        }
    }

    void end_record(const char_type*, std::false_type)
    {}

public:
    void empty_physical_line(const char_type* where)
    {
        empty_physical_line(where,
            std::integral_constant<bool,
                (Handle
               & primitive_table_pull_handle_empty_physical_line) != 0>());
    }

private:
    void empty_physical_line(const char_type* where, std::true_type)
    {
        if (collects_data_) {
            sq_.emplace_back(
                primitive_table_pull_state::empty_physical_line, dn(1));
            dq_.push_back(uc(where));
        } else {
            sq_.emplace_back(
                primitive_table_pull_state::empty_physical_line, dn(0));
        }
    }

    void empty_physical_line(const char_type*, std::false_type)
    {}

public:
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

private:
    char_type* uc(const char_type* s) const noexcept
    {
        return buffer_ + (s - buffer_);
    }

    template <class N>
    static auto dn(N n)
    {
        return static_cast<typename state_queue_element_type::second_type>(n);
    }
};

}} // end detail::pull

template <class TableSource,
    class Allocator = std::allocator<typename TableSource::char_type>,
    std::underlying_type_t<primitive_table_pull_handle> Handle =
        primitive_table_pull_handle_all>
class primitive_table_pull
{
public:
    using char_type = typename TableSource::char_type;
    using allocator_type = Allocator;
    using size_type = std::size_t;

private:
    using at_t = std::allocator_traits<Allocator>;

    using handler_t = detail::pull::handler<char_type, Allocator, Handle>;
    using handler_a_t = detail::allocation_only_allocator<
        typename at_t::template rebind_alloc<handler_t>>;
    using handler_at_t = std::allocator_traits<handler_a_t>;
    using handler_p_t = typename handler_at_t::pointer;

    static_assert(std::is_same<
        decltype(std::declval<const TableSource&>()(
            std::declval<reference_handler<handler_t>>())),
        decltype(std::declval<TableSource>()(
            std::declval<reference_handler<handler_t>>()))>::value,
        "");

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
        detail::pull::has_get_physical_position<parser_t>::value;

    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    template <class TableSourceR,
        std::enable_if_t<
            std::is_base_of<TableSource, std::decay_t<TableSourceR>>::value
         && !std::is_base_of<primitive_table_pull,
                             std::decay_t<TableSourceR>>::value>* = nullptr>
    explicit primitive_table_pull(
        TableSourceR&& in, std::size_t buffer_size = 0) :
        primitive_table_pull(std::allocator_arg, Allocator(),
            std::forward<TableSourceR>(in), buffer_size)
    {}

    template <class TableSourceR,
        std::enable_if_t<
            std::is_base_of<TableSource, std::decay_t<TableSourceR>>::value>*
        = nullptr>
    primitive_table_pull(std::allocator_arg_t, const Allocator& alloc,
        TableSourceR&& in, std::size_t buffer_size = 0) :
        i_sq_(0), i_dq_(0),
        handler_(create_handler(alloc,
            std::allocator_arg, alloc, buffer_size)),
        sq_(&handler_->state_queue()), dq_(&handler_->data_queue()),
        ap_(alloc,
            std::forward<TableSourceR>(in)(
                reference_handler<handler_t>(*handler_)))
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
            sq_->clear();
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

    char_type* operator[](size_type i)
    {
        assert(i < data_size());
        return (*dq_)[i_dq_ + i];
    }

    const char_type* operator[](size_type i) const
    {
        assert(i < data_size());
        return (*dq_)[i_dq_ + i];
    }

    char_type* at(size_type i)
    {
        return at_impl(this, i);
    }

    const char_type* at(size_type i) const
    {
        return at_impl(this, i);
    }

private:
    template <class ThisType>
    static auto at_impl(ThisType* me, size_type i)
    {
        const auto ds = me->data_size();
        if (i < ds) {
            return (*me)[i];
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
        return ((Handle & (primitive_table_pull_handle_start_buffer
                         | primitive_table_pull_handle_update
                         | primitive_table_pull_handle_finalize)) != 0) ?  2 :
               ((Handle & (primitive_table_pull_handle_end_buffer
                         | primitive_table_pull_handle_empty_physical_line
                         | primitive_table_pull_handle_start_record
                         | primitive_table_pull_handle_end_record)) != 0) ? 1 :
               0;
    }

    std::pair<std::size_t, std::size_t> get_physical_position() const
        noexcept((!detail::pull::has_get_physical_position<parser_t>::value)
              || noexcept(std::declval<const parser_t&>()
                            .get_physical_position()))
    {
        return get_physical_position_impl(
            detail::pull::has_get_physical_position<parser_t>());
    }

private:
     template <class... Args>
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

    std::pair<std::size_t, std::size_t> get_physical_position_impl(
        std::true_type) const noexcept(
            noexcept(std::declval<const parser_t&>().get_physical_position()))
    {
        return ap_.member().get_physical_position();
    }

    std::pair<std::size_t, std::size_t> get_physical_position_impl(
        std::false_type) const noexcept
    {
        return { npos, npos };
    }
};

template <class TableSource, class Allocator,
    std::underlying_type_t<primitive_table_pull_handle> Handle>
typename primitive_table_pull<TableSource, Allocator, Handle>::handler_t::
            state_queue_type
    primitive_table_pull<TableSource, Allocator, Handle>::sq_moved_from
 = { std::make_pair(
        primitive_table_pull_state::eof,
        static_cast<typename primitive_table_pull<TableSource, Allocator,
            Handle>::handler_t::state_queue_element_type::second_type>(0)) };

template <class TableSource, class Allocator,
    std::underlying_type_t<primitive_table_pull_handle> Handle>
typename primitive_table_pull<TableSource, Allocator, Handle>::handler_t::
            data_queue_type
    primitive_table_pull<TableSource, Allocator, Handle>::dq_moved_from = {};

enum class table_pull_state : std::uint_fast8_t
{
    eof,
    before_parse,
    field,
    record_end
};

template <class TableSource,
    class Allocator = std::allocator<typename TableSource::char_type>>
class table_pull
{
public:
    using char_type = typename TableSource::char_type;
    using traits_type = typename TableSource::traits_type;
    using allocator_type = Allocator;

private:
    using primitive_t = primitive_table_pull<TableSource, allocator_type,
        (primitive_table_pull_handle_end_buffer
       | primitive_table_pull_handle_end_record
       | primitive_table_pull_handle_empty_physical_line
       | primitive_table_pull_handle_update
       | primitive_table_pull_handle_finalize)>;

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

    table_pull_state last_state_;
    std::pair<char_type*, char_type*> last_;
    std::basic_string<char_type, traits_type, Allocator> value_;

    std::size_t i_;
    std::size_t j_;

public:
    using value_type = const char_type;
    using reference = value_type&;
    using const_reference = reference;
    using pointer = value_type*;
    using const_pointer = pointer;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = value_type*;
    using const_iterator = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr bool physical_position_available =
        primitive_t::physical_position_available;

    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    template <class TableSourceR,
        std::enable_if_t<
            std::is_base_of<TableSource, std::decay_t<TableSourceR>>::value
         && !std::is_base_of<table_pull, std::decay_t<TableSourceR>>::value>*
        = nullptr>
    explicit table_pull(TableSourceR&& in, std::size_t buffer_size = 0) :
        table_pull(std::allocator_arg, Allocator(),
            std::forward<TableSourceR>(in), buffer_size)
    {}

    template <class TableSourceR,
        std::enable_if_t<
            std::is_base_of<TableSource, std::decay_t<TableSourceR>>::value>*
        = nullptr>
    table_pull(std::allocator_arg_t, const Allocator& alloc,
        TableSourceR&& in, std::size_t buffer_size = 0) :
        p_(std::allocator_arg, alloc, std::forward<TableSourceR>(in),
            ((buffer_size > 1) ? buffer_size : 2)),
        empty_physical_line_aware_(false),
        last_state_(table_pull_state::before_parse), last_(empty_string()),
        value_(alloc), i_(0), j_(0)
    {}

    table_pull(table_pull&& other) noexcept :
        p_(std::move(other.p_)),
        empty_physical_line_aware_(other.empty_physical_line_aware_),
        last_state_(std::exchange(other.last_state_, table_pull_state::eof)),
        last_(std::exchange(other.last_, empty_string())),
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
        return last_state_;
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

        last_ = empty_string();
        value_.clear();
        switch (last_state_) {
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
            next_field();                                           // throw
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
                    last_state_ = table_pull_state::field;
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
                    // fall through
                case primitive_table_pull_state::end_record:
                    last_state_ = table_pull_state::record_end;
                    return *this;
                case primitive_table_pull_state::eof:
                    last_state_ = table_pull_state::eof;
                    return *this;
                case primitive_table_pull_state::end_buffer:
                default:
                    break;
                }
            }
        } catch (...) {
            last_state_ = table_pull_state::eof;
            throw;
        }
    }

private:
    void next_field()
    {
        assert(*this);
        try {
            for (;;) {
                switch (p_().state()) {                             // throw
                case primitive_table_pull_state::update:
                    do_update(p_[0], p_[1]);                        // throw
                    break;
                case primitive_table_pull_state::finalize:
                    do_update(p_[0], p_[1]);                        // throw
                    if (value_.empty()) {
                        *last_.second = char_type();
                    } else {
                        value_.push_back(char_type());              // throw
                        last_.first = &value_[0];
                        last_.second = last_.first + value_.size() - 1;
                    }
                    last_state_ = table_pull_state::field;
                    return;
                case primitive_table_pull_state::empty_physical_line:
                    if (!empty_physical_line_aware_) {
                        break;
                    }
                    // fall through
                case primitive_table_pull_state::end_record:
                    last_state_ = table_pull_state::record_end;
                    last_ = empty_string();
                    return;
                case primitive_table_pull_state::end_buffer:
                    if (last_.first != empty_string().first) {
                        value_.append(last_.first, last_.second);   // throw
                        last_.first = empty_string().first;
                    }
                    break;
                case primitive_table_pull_state::eof:
                    last_state_ = table_pull_state::eof;
                    last_ = empty_string();
                    return;
                default:
                    break;
                }
            }
        } catch (...) {
            last_state_ = table_pull_state::eof;
            last_ = empty_string();
            throw;
        }
    }

public:
    table_pull& skip_record(std::size_t n = 0)
    {
        if (!*this) {
            return *this;
        }

        last_ = empty_string();
        value_.clear();

        if (last_state_ == table_pull_state::record_end) {
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
                    last_state_ = table_pull_state::field;
                    ++j_;
                    break;
                case primitive_table_pull_state::empty_physical_line:
                    if (!empty_physical_line_aware_) {
                        break;
                    }
                    // fall through
                case primitive_table_pull_state::end_record:
                    last_state_ = table_pull_state::record_end;
                    if (n == 0) {
                        return *this;
                    } else {
                        ++i_;
                        j_ = 0;
                        --n;
                        break;
                    }
                case primitive_table_pull_state::eof:
                    last_state_ = table_pull_state::eof;
                    return *this;
                case primitive_table_pull_state::end_buffer:
                default:
                    break;
                }
            }
        } catch (...) {
            last_state_ = table_pull_state::eof;
            throw;
        }
    }

    const_iterator begin() const noexcept
    {
        return cbegin();
    }

    const_iterator end() const noexcept
    {
        return cend();
    }

    const_iterator cbegin() const noexcept
    {
        return last_.first;
    }

    const_iterator cend() const noexcept
    {
        return last_.second;
    }

    const_reverse_iterator rbegin() const noexcept
    {
        return crbegin();
    }

    const_reverse_iterator rend() const noexcept
    {
        return crend();
    }

    const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(cend());
    }

    const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(cbegin());
    }

    const_reference operator[](size_type pos) const
    {
        // pos == size() is OK, IAW std::basic_string
        return cbegin()[pos];
    }

    const_reference at(size_type pos) const
    {
        // pos == size() is NG, IAW std::basic_string
        if (pos >= size()) {
            std::ostringstream s;
            s << pos << " is too large for the current string value, "
                        "whose size is " << size();
            throw std::out_of_range(s.str());
        }
        return (*this)[pos];
    }

    const_reference front() const
    {
        assert(!empty());
        return *cbegin();
    }

    const_reference back() const
    {
        assert(!empty());
        return *(cend() - 1);
    }

    const_pointer c_str() const noexcept
    {
        return cbegin();
    }

    const_pointer data() const noexcept
    {
        return cbegin();
    }

    bool empty() const noexcept
    {
        return cbegin() == cend();
    }

    size_type size() const noexcept
    {
        return cend() - cbegin();
    }

    size_type max_size() const noexcept
    {
        return std::numeric_limits<size_type>::max();
    }

    template <class OtherTr = std::char_traits<char_type>,
        class OtherAllocator = std::allocator<char_type>>
    explicit operator
    std::basic_string<char_type, OtherTr, OtherAllocator>() const
    {
        return { cbegin(), cend() };
    }

private:
    void do_update(char_type* first, char_type* last)
    {
        if (!value_.empty()) {
            value_.append(first, last);                             // throw
        } else if (last_.first != empty_string().first) {
            TableSource::traits_type::move(last_.second, first, last - first);
            last_.second += last - first;
        } else {
            last_.first = first;
            last_.second = last;
        }
    }

    static std::pair<char_type*, char_type*>
    empty_string() noexcept
    {
        return std::make_pair(
            &detail::nul<char_type>::value, &detail::nul<char_type>::value);
    }
};

template <class TableSourceL, class TableSourceR,
          class AllocatorL, class AllocatorR>
auto operator==(
    const table_pull<TableSourceL, AllocatorL>& left,
    const table_pull<TableSourceR, AllocatorR>& right) noexcept
 -> std::enable_if_t<
        std::is_same<
            typename TableSourceL::char_type,
            typename TableSourceR::char_type>::value
     && std::is_same<
            typename TableSourceL::traits_type,
            typename TableSourceR::traits_type>::value, bool>
{
    return detail::string_value_eq(left, right);
}

template <class TableSource, class Allocator, class Right>
auto operator==(
    const table_pull<TableSource, Allocator>& left,
    const Right& right)
    noexcept(noexcept(detail::string_value_eq(left, right)))
 -> std::enable_if_t<detail::is_comparable_with_string_value<
        typename TableSource::char_type,
        typename TableSource::traits_type, Right>::value, bool>
{
    return detail::string_value_eq(left, right);
}

template <class TableSource, class Allocator, class Left>
auto operator==(
    const Left& left,
    const table_pull<TableSource, Allocator>& right)
    noexcept(noexcept(detail::string_value_eq(left, right)))
 -> std::enable_if_t<detail::is_comparable_with_string_value<
        typename TableSource::char_type,
        typename TableSource::traits_type, Left>::value, bool>
{
    return detail::string_value_eq(left, right);
}

template <class TableSourceL, class TableSourceR,
          class AllocatorL, class AllocatorR>
auto operator!=(
    const table_pull<TableSourceL, AllocatorL>& left,
    const table_pull<TableSourceR, AllocatorR>& right) noexcept
 -> std::enable_if_t<
        std::is_same<
            typename TableSourceL::char_type,
            typename TableSourceR::char_type>::value
     && std::is_same<
            typename TableSourceL::traits_type,
            typename TableSourceR::traits_type>::value, bool>
{
    return !(left == right);
}

template <class TableSource, class Allocator, class Right>
auto operator!=(
    const table_pull<TableSource, Allocator>& left,
    const Right& right)
    noexcept(noexcept(!(left == right)))
 -> std::enable_if_t<detail::is_comparable_with_string_value<
        typename TableSource::char_type,
        typename TableSource::traits_type, Right>::value, bool>
{
    return !(left == right);
}

template <class TableSource, class Allocator, class Left>
auto operator!=(
    const Left& left,
    const table_pull<TableSource, Allocator>& right)
    noexcept(noexcept(!(left == right)))
 -> std::enable_if_t<detail::is_comparable_with_string_value<
        typename TableSource::char_type,
        typename TableSource::traits_type, Left>::value, bool>
{
    return !(left == right);
}

template <class TableSourceL, class TableSourceR,
          class AllocatorL, class AllocatorR>
auto operator<(
    const table_pull<TableSourceL, AllocatorL>& left,
    const table_pull<TableSourceR, AllocatorR>& right) noexcept
 -> std::enable_if_t<
        std::is_same<
            typename TableSourceL::char_type,
            typename TableSourceR::char_type>::value
     && std::is_same<
            typename TableSourceL::traits_type,
            typename TableSourceR::traits_type>::value, bool>
{
    return detail::string_value_lt(left, right);
}

template <class TableSource, class Allocator, class Right>
auto operator<(
    const table_pull<TableSource, Allocator>& left,
    const Right& right)
    noexcept(noexcept(detail::string_value_lt(left, right)))
 -> std::enable_if_t<detail::is_comparable_with_string_value<
        typename TableSource::char_type,
        typename TableSource::traits_type, Right>::value, bool>
{
    return detail::string_value_lt(left, right);
}

template <class TableSource, class Allocator, class Left>
auto operator<(
    const Left& left,
    const table_pull<TableSource, Allocator>& right)
    noexcept(noexcept(detail::string_value_lt(left, right)))
 -> std::enable_if_t<detail::is_comparable_with_string_value<
        typename TableSource::char_type,
        typename TableSource::traits_type, Left>::value, bool>
{
    return detail::string_value_lt(left, right);
}

template <class TableSourceL, class TableSourceR,
          class AllocatorL, class AllocatorR>
auto operator>(
    const table_pull<TableSourceL, AllocatorL>& left,
    const table_pull<TableSourceR, AllocatorR>& right) noexcept
 -> std::enable_if_t<
        std::is_same<
            typename TableSourceL::char_type,
            typename TableSourceR::char_type>::value
     && std::is_same<
            typename TableSourceL::traits_type,
            typename TableSourceR::traits_type>::value, bool>
{
    return right < left;
}

template <class TableSource, class Allocator, class Right>
auto operator>(
    const table_pull<TableSource, Allocator>& left,
    const Right& right)
    noexcept(noexcept(right < left))
 -> std::enable_if_t<detail::is_comparable_with_string_value<
        typename TableSource::char_type,
        typename TableSource::traits_type, Right>::value, bool>
{
    return right < left;
}

template <class TableSource, class Allocator, class Left>
auto operator>(
    const Left& left,
    const table_pull<TableSource, Allocator>& right)
    noexcept(noexcept(right < left))
 -> std::enable_if_t<detail::is_comparable_with_string_value<
        typename TableSource::char_type,
        typename TableSource::traits_type, Left>::value, bool>
{
    return right < left;
}

template <class TableSourceL, class TableSourceR,
          class AllocatorL, class AllocatorR>
auto operator<=(
    const table_pull<TableSourceL, AllocatorL>& left,
    const table_pull<TableSourceR, AllocatorR>& right) noexcept
 -> std::enable_if_t<
        std::is_same<
            typename TableSourceL::char_type,
            typename TableSourceR::char_type>::value
     && std::is_same<
            typename TableSourceL::traits_type,
            typename TableSourceR::traits_type>::value, bool>
{
    return !(right < left);
}

template <class TableSource, class Allocator, class Right>
auto operator<=(
    const table_pull<TableSource, Allocator>& left,
    const Right& right) 
    noexcept(noexcept(!(right < left)))
 -> std::enable_if_t<detail::is_comparable_with_string_value<
        typename TableSource::char_type,
        typename TableSource::traits_type, Right>::value, bool>
{
    return !(right < left);
}

template <class TableSource, class Allocator, class Left>
auto operator<=(
    const Left& left,
    const table_pull<TableSource, Allocator>& right)
    noexcept(noexcept(!(right < left)))
 -> std::enable_if_t<detail::is_comparable_with_string_value<
        typename TableSource::char_type,
        typename TableSource::traits_type, Left>::value, bool>
{
    return !(right < left);
}

template <class TableSourceL, class TableSourceR,
          class AllocatorL, class AllocatorR>
auto operator>=(
    const table_pull<TableSourceL, AllocatorL>& left,
    const table_pull<TableSourceR, AllocatorR>& right) noexcept
 -> std::enable_if_t<
        std::is_same<
            typename TableSourceL::char_type,
            typename TableSourceR::char_type>::value
     && std::is_same<
            typename TableSourceL::traits_type,
            typename TableSourceR::traits_type>::value, bool>
{
    return !(left < right);
}

template <class TableSource, class Allocator, class Right>
auto operator>=(
    const table_pull<TableSource, Allocator>& left,
    const Right& right)
    noexcept(noexcept(!(left < right)))
 -> std::enable_if_t<detail::is_comparable_with_string_value<
        typename TableSource::char_type,
        typename TableSource::traits_type, Right>::value, bool>
{
    return !(left < right);
}

template <class TableSource, class Allocator, class Left>
auto operator>=(
    const Left& left,
    const table_pull<TableSource, Allocator>& right)
    noexcept(noexcept(!(left < right)))
 -> std::enable_if_t<detail::is_comparable_with_string_value<
        typename TableSource::char_type,
        typename TableSource::traits_type, Left>::value, bool>
{
    return !(left < right);
}

template <class TableSource, class Allocator, class OtherAllocator>
auto operator+=(
    std::basic_string<typename TableSource::char_type,
        typename TableSource::traits_type, OtherAllocator>& left,
    const table_pull<TableSource, Allocator>& right)
 -> decltype(left)
{
    return detail::string_value_plus_assign(left, right);
}

template <class TableSource, class Allocator>
auto operator<<(
    std::basic_ostream<typename TableSource::char_type,
                       typename TableSource::traits_type>& os,
    const table_pull<TableSource, Allocator>& p)
 -> decltype(os)
{
    // In C++17, this function will be able to be implemented in terms of
    // string_view's operator<<
    const auto n = static_cast<std::streamsize>(p.size());
    return detail::formatted_output(
        os, n,
        [b = p.cbegin(), n](auto* sb) {
            return sb->sputn(b, n) == n;
        });
}

template <class TableSource, class Allocator,
    class OtherAllocator = std::allocator<typename TableSource::char_type>>
std::basic_string<typename TableSource::char_type,
                  std::char_traits<typename TableSource::char_type>,
                  OtherAllocator> to_string(
    const table_pull<TableSource, Allocator>& p,
    const OtherAllocator& alloc = OtherAllocator())
{
    return { p.cbegin(), p.cend(), alloc };
}

template <class TableSource, class... Appendices>
auto make_table_pull(TableSource&& in, Appendices&&... appendices)
 -> std::enable_if_t<
        std::is_constructible<table_pull<std::decay_t<TableSource>>,
                              TableSource&&, Appendices&&...>::value,
        table_pull<std::decay_t<TableSource>>>
{
    return table_pull<std::decay_t<TableSource>>(
        std::forward<TableSource>(in),
        std::forward<Appendices>(appendices)...);
}

template <class TableSource, class Allocator, class... Appendices>
auto make_table_pull(std::allocator_arg_t, const Allocator& alloc,
    TableSource&& in, Appendices&&... appendices)
 -> std::enable_if_t<
        std::is_constructible<
            table_pull<std::decay_t<TableSource>, Allocator>,
            std::allocator_arg_t, const Allocator&,
            TableSource&&, Appendices&&...>::value,
        table_pull<std::decay_t<TableSource>, Allocator>>
{
    return table_pull<std::decay_t<TableSource>, Allocator>(
        std::allocator_arg, alloc, std::forward<TableSource>(in),
        std::forward<Appendices>(appendices)...);
}

}

#endif
