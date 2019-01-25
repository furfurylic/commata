/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_D53E08F9_CF1C_4762_BF77_1A6FB68C6A96
#define COMMATA_GUARD_D53E08F9_CF1C_4762_BF77_1A6FB68C6A96

#include <ios>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <ostream>
#include <streambuf>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "allocation_only_allocator.hpp"
#include "text_error.hpp"
#include "key_chars.hpp"
#include "member_like_base.hpp"
#include "typing_aid.hpp"

namespace commata {
namespace detail {

template <class T>
class field_name_of_impl
{
    const char* prefix_;
    const T* t_;

public:
    field_name_of_impl(const char* prefix, const T& t) :
        prefix_(prefix), t_(&t)
    {}

    friend std::ostream& operator<<(
        std::ostream& o, const field_name_of_impl& t)
    {
        return o << t.prefix_ << *t.t_;
    }
};

template <class... Args>
auto field_name_of(Args...)
{
    return "";
}

template <class T>
auto field_name_of(const char* prefix, const T& t,
    decltype(&(std::declval<std::ostream&>() << t)) = nullptr)
{
    return field_name_of_impl<T>(prefix, t);
}

template <class FieldNamePred>
struct field_name_pred_base :
    member_like_base<FieldNamePred>
{
    using member_like_base<FieldNamePred>::member_like_base;
};

template <class FieldValuePred>
struct field_value_pred_base :
    member_like_base<FieldValuePred>
{
    using member_like_base<FieldValuePred>::member_like_base;
};

template <class Ch>
struct hollow_field_name_pred
{
    bool operator()(const Ch*, const Ch*) const
    {
        return true;
    }
};

} // end namespace detail

class record_extraction_error :
    public text_error
{
public:
    using text_error::text_error;
};

template <class FieldNamePred, class FieldValuePred,
          class Ch, class Tr, class Allocator>
class record_extractor;

template <class FieldValuePred,
          class Ch, class Tr, class Allocator>
class record_extractor_with_indexed_key;

namespace detail {

template <class FieldNamePred, class FieldValuePred,
    class Ch, class Tr, class Allocator>
class record_extractor_impl :
    field_name_pred_base<FieldNamePred>,
    field_value_pred_base<FieldValuePred>
{
    using field_name_pred_t = field_name_pred_base<FieldNamePred>;
    using field_value_pred_t = field_value_pred_base<FieldValuePred>;
    using alloc_t = detail::allocation_only_allocator<Allocator>;

    enum class record_mode : std::int_fast8_t
    {
        unknown,
        include,
        exclude
    };

    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    record_mode header_mode_;
    record_mode record_mode_;

    std::size_t record_num_to_include_;
    std::size_t target_field_index_;

    std::size_t field_index_;
    const Ch* current_begin_;   // current records's begin if not the buffer
                                // switched, current buffer's begin otherwise
    std::basic_streambuf<Ch, Tr>* out_;

    // We use string instead of vector for its nothrow-movability, but in
    // C++17 we shall have nothrow-movable vector, so then we shall be able to
    // adopt vector
    std::basic_string<Ch, Tr, alloc_t> field_buffer_;
    std::basic_string<Ch, Tr, alloc_t> record_buffer_;
                                // populated only after the buffer switched in
                                // a unknown (included or not) record and
                                // shall not overlap with interval
                                // [current_begin_, +inf)

    friend class record_extractor<
        FieldNamePred, FieldValuePred, Ch, Tr, Allocator>;
    friend class record_extractor_with_indexed_key<
        FieldValuePred, Ch, Tr, Allocator>;

public:
    using char_type = Ch;
    using traits_type = Tr;
    using allocator_type = Allocator;

    record_extractor_impl(
        std::allocator_arg_t, const Allocator& alloc,
        std::basic_streambuf<Ch, Tr>* out,
        FieldNamePred field_name_pred, FieldValuePred field_value_pred,
        bool includes_header, std::size_t max_record_num) :
        field_name_pred_t(std::move(field_name_pred)),
        field_value_pred_t(std::move(field_value_pred)),
        header_mode_(includes_header ?
            record_mode::include : record_mode::exclude),
        record_mode_(record_mode::exclude),
        record_num_to_include_(max_record_num), target_field_index_(npos),
        field_index_(0), out_(out),
        field_buffer_(alloc_t(alloc)), record_buffer_(alloc_t(alloc))
    {}

    record_extractor_impl(record_extractor_impl&&) noexcept = default;
    ~record_extractor_impl() = default;

    // Move-assignment shall be deleted because basic_string's propagation of
    // the allocator in C++14 is apocryphal (it does not seem able to be
    // noexcept unconditionally)

    allocator_type get_allocator() const noexcept
    {
        return field_buffer_.get_allocator().base();
    }

    void start_buffer(const Ch* buffer_begin, const Ch* /*buffer_end*/)
    {
        current_begin_ = buffer_begin;
    }

    void end_buffer(const Ch* buffer_end)
    {
        switch (record_mode_) {
        case record_mode::include:
            flush_current(buffer_end);
            break;
        case record_mode::unknown:
            record_buffer_.append(current_begin_, buffer_end);
            break;
        default:
            break;
        }
    }

    void start_record(const Ch* record_begin)
    {
        current_begin_ = record_begin;
        record_mode_ = header_yet() ? header_mode_ : record_mode::unknown;
        field_index_ = 0;
        assert(record_buffer_.empty());
    }

    void update(const Ch* first, const Ch* last)
    {
        if ((header_yet() && (target_field_index_ == npos))
         || (field_index_ == target_field_index_)) {
            field_buffer_.append(first, last);
        }
    }

    void finalize(const Ch* first, const Ch* last)
    {
        using namespace std::placeholders;
        if (header_yet()) {
            if ((target_field_index_ == npos)
             && with_field_buffer_appended(first, last, std::bind(
                    std::ref(field_name_pred_t::get()), _1, _2))) {
                target_field_index_ = field_index_;
            }
            ++field_index_;
            if (field_index_ >= npos) {
                throw no_matching_field();
            }
        } else {
            if ((record_mode_ == record_mode::unknown)
             && (field_index_ == target_field_index_)) {
                if (with_field_buffer_appended(first, last, std::bind(
                        std::ref(field_value_pred_t::get()), _1, _2))) {
                    include();
                } else {
                    exclude();
                }
            }
            ++field_index_;
            if (field_index_ >= npos) {
                exclude();
            }
        }
    }

    bool end_record(const Ch* record_end)
    {
        if (header_yet()) {
            if (target_field_index_ == npos) {
                throw no_matching_field();
            }
            flush_record(record_end);
            if (record_num_to_include_ == 0) {
                return false;
            }
            header_mode_ = record_mode::unknown;
        } else if (flush_record(record_end)) {
            if (record_num_to_include_ == 1) {
                return false;
            }
            --record_num_to_include_;
        }
        return true;
    }

private:
    bool header_yet() const noexcept
    {
        return header_mode_ != record_mode::unknown;
    }

    template <class F>
    auto with_field_buffer_appended(const Ch* first, const Ch* last, F f)
    {
        if (field_buffer_.empty()) {
            return f(first, last);
        } else {
            field_buffer_.append(first, last);
            const auto r = f(
                &field_buffer_[0], &field_buffer_[0] + field_buffer_.size());
            field_buffer_.clear();
            return r;
        }
    }

    record_extraction_error no_matching_field() const
    {
        std::ostringstream what;
        what << "No matching field"
             << field_name_of(" for ", field_name_pred_t::get());
        return record_extraction_error(what.str());
    }

    void include()
    {
        flush_record_buffer();
        record_mode_ = record_mode::include;
    }

    void exclude()
    {
        record_mode_ = record_mode::exclude;
        record_buffer_.clear();
    }

    bool flush_record(const Ch* record_end)
    {
        switch (record_mode_) {
        case record_mode::include:
            flush_record_buffer();
            flush_current(record_end);
            flush_lf();
            record_mode_ = record_mode::exclude;    // to prevent end_buffer
                                                    // from doing anything
            return true;
        case record_mode::exclude:
            assert(record_buffer_.empty());
            return false;
        case record_mode::unknown:
            assert(!header_yet());
            record_mode_ = record_mode::exclude;    // no such a far field
            record_buffer_.clear();
            return false;
        default:
            assert(false);
            return false;
        }
    }

    void flush_record_buffer()
    {
        if (!record_buffer_.empty()) {
            out_->sputn(record_buffer_.data(), record_buffer_.size());
            record_buffer_.clear();
        }
    }

    void flush_current(const Ch* end)
    {
        assert(record_buffer_.empty());
        out_->sputn(current_begin_, end - current_begin_);
    }

    void flush_lf()
    {
        out_->sputc(key_chars<Ch>::LF);
    }
};

} // end namespace detail

template <class FieldNamePred, class FieldValuePred, class Ch,
    class Tr = std::char_traits<Ch>, class Allocator = std::allocator<Ch>>
class record_extractor :
    public detail::record_extractor_impl<
        FieldNamePred, FieldValuePred, Ch, Tr, Allocator>
{
    using base = detail::record_extractor_impl<
        FieldNamePred, FieldValuePred, Ch, Tr, Allocator>;

public:
    record_extractor(
        std::basic_streambuf<Ch, Tr>* out,
        FieldNamePred field_name_pred, FieldValuePred field_value_pred,
        bool includes_header = true, std::size_t max_record_num = base::npos) :
        record_extractor(
            std::allocator_arg, Allocator(), out, field_name_pred,
            field_value_pred, includes_header, max_record_num)
    {}

    record_extractor(
        std::allocator_arg_t, const Allocator& alloc,
        std::basic_streambuf<Ch, Tr>* out,
        FieldNamePred field_name_pred, FieldValuePred field_value_pred,
        bool includes_header = true, std::size_t max_record_num = base::npos) :
        base(std::allocator_arg, alloc, out, std::move(field_name_pred),
            std::move(field_value_pred), includes_header, max_record_num)
    {}

    record_extractor(record_extractor&&) = default;
    ~record_extractor() = default;
};

template <class FieldValuePred, class Ch,
    class Tr = std::char_traits<Ch>, class Allocator = std::allocator<Ch>>
class record_extractor_with_indexed_key :
    public detail::record_extractor_impl<
        detail::hollow_field_name_pred<Ch>, FieldValuePred, Ch, Tr, Allocator>
{
    using base = detail::record_extractor_impl<
        detail::hollow_field_name_pred<Ch>, FieldValuePred, Ch, Tr, Allocator>;

public:
    record_extractor_with_indexed_key(
        std::basic_streambuf<Ch, Tr>* out,
        std::size_t target_field_index, FieldValuePred field_value_pred,
        bool includes_header = true, std::size_t max_record_num = base::npos) :
        record_extractor_with_indexed_key(
            std::allocator_arg, Allocator(), out, target_field_index,
            field_value_pred, includes_header, max_record_num)
    {}

    record_extractor_with_indexed_key(
        std::allocator_arg_t, const Allocator& alloc,
        std::basic_streambuf<Ch, Tr>* out,
        std::size_t target_field_index, FieldValuePred field_value_pred,
        bool includes_header = true, std::size_t max_record_num = base::npos) :
        base(std::allocator_arg, alloc, out,
            detail::hollow_field_name_pred<Ch>(), std::move(field_value_pred),
            includes_header, max_record_num)
    {
        this->target_field_index_ = target_field_index;
    }

    record_extractor_with_indexed_key(
        record_extractor_with_indexed_key&&) = default;
    ~record_extractor_with_indexed_key() = default;
};

namespace detail {

template <class Ch, class Tr, class Allocator>
class string_eq;

template <class Ch, class Tr, class Allocator>
std::basic_ostream<Ch, Tr>& operator<<(
    std::basic_ostream<Ch, Tr>& os, const string_eq<Ch, Tr, Allocator>& o);

template <class Ch, class Tr, class Allocator>
class string_eq
{
    std::basic_string<Ch, Tr, Allocator> s_;

public:
    explicit string_eq(std::basic_string<Ch, Tr, Allocator>&& s) :
        s_(std::move(s))
    {}

    bool operator()(const Ch* begin, const Ch* end) const noexcept
    {
        const auto rlen = static_cast<decltype(s_.size())>(end - begin);
        return (s_.size() == rlen)
            && (Tr::compare(s_.data(), begin, rlen) == 0);
    }

    friend std::basic_ostream<Ch, Tr>& operator<< <Ch, Tr, Allocator>(
        std::basic_ostream<Ch, Tr>&, const string_eq<Ch, Tr, Allocator>&);
};

template <class Ch, class Tr, class Allocator>
std::basic_ostream<Ch, Tr>& operator<<(
    std::basic_ostream<Ch, Tr>& os, const string_eq<Ch, Tr, Allocator>& o)
{
    return os << o.s_;
}

template <class Ch, class Tr, class Allocator, class OtherAllocator>
string_eq<Ch, Tr, OtherAllocator> make_string_pred(
    std::basic_string<Ch, Tr, OtherAllocator>&& s, const Allocator&)
{
    return string_eq<Ch, Tr, OtherAllocator>(std::move(s));
}

template <class Ch, class Tr, class Allocator, class T>
auto make_string_pred(const T& s, const Allocator& a)
 -> std::enable_if_t<
        std::is_constructible<std::basic_string<Ch, Tr, Allocator>,
        const T&, const Allocator&>::value, string_eq<Ch, Tr, Allocator>>
{
    return string_eq<Ch, Tr, Allocator>(
        std::basic_string<Ch, Tr, Allocator>(s, a));
}

template <class Ch, class Tr, class Allocator, class T>
auto make_string_pred(T&& s, const Allocator&)
 -> std::enable_if_t<
        !std::is_constructible<std::basic_string<Ch, Tr, Allocator>,
        const T&, const Allocator&>::value, T&&>
{
    return std::forward<T>(s);
}

template <class TargetFieldIndex, class FieldValuePred,
    class Ch, class Tr, class Allocator, class... Appendices>
auto make_record_extractor_impl(
    std::true_type,
    std::allocator_arg_t, const Allocator& alloc,
    std::basic_streambuf<Ch, Tr>* out,
    TargetFieldIndex target_field_index, FieldValuePred&& field_value_pred,
    Appendices&&... appendices)
{
    auto fvp = detail::make_string_pred<Ch, Tr, Allocator>(
        std::forward<FieldValuePred>(field_value_pred), alloc);
    return record_extractor_with_indexed_key<decltype(fvp), Ch, Tr, Allocator>(
        std::allocator_arg, alloc, out,
        static_cast<std::size_t>(target_field_index), std::move(fvp),
        std::forward<Appendices>(appendices)...);
}

template <class FieldNamePred, class FieldValuePred,
    class Ch, class Tr, class Allocator, class... Appendices>
auto make_record_extractor_impl(
    std::false_type,
    std::allocator_arg_t, const Allocator& alloc,
    std::basic_streambuf<Ch, Tr>* out,
    FieldNamePred&& field_name_pred, FieldValuePred&& field_value_pred,
    Appendices&&... appendices)
{
    auto fnp = detail::make_string_pred<Ch, Tr, Allocator>(
        std::forward<FieldNamePred>(field_name_pred), alloc);
    auto fvp = detail::make_string_pred<Ch, Tr, Allocator>(
        std::forward<FieldValuePred>(field_value_pred), alloc);
    return record_extractor<decltype(fnp), decltype(fvp), Ch, Tr, Allocator>(
        std::allocator_arg, alloc, out,
        std::move(fnp), std::move(fvp),
        std::forward<Appendices>(appendices)...);
}

} // end namespace detail

template <class FieldNamePred, class FieldValuePred,
    class Ch, class Tr, class Allocator, class... Appendices>
auto make_record_extractor(
    std::allocator_arg_t, const Allocator& alloc,
    std::basic_streambuf<Ch, Tr>* out,
    FieldNamePred&& field_name_pred, FieldValuePred&& field_value_pred,
    Appendices&&... appendices)
{
    return detail::make_record_extractor_impl(
        std::is_integral<std::decay_t<FieldNamePred>>(),
        std::allocator_arg, alloc, out,
        std::forward<FieldNamePred>(field_name_pred),
        std::forward<FieldValuePred>(field_value_pred),
        std::forward<Appendices>(appendices)...);
}

template <class Ch, class Tr, class Allocator, class... Appendices>
auto make_record_extractor(
    std::allocator_arg_t, const Allocator& alloc,
    std::basic_ostream<Ch, Tr>& out, Appendices&&... appendices)
{
    return make_record_extractor(
        std::allocator_arg, alloc, out.rdbuf(),
        std::forward<Appendices>(appendices)...);
}

template <class Ch, class Tr, class... Appendices>
auto make_record_extractor(
    std::basic_streambuf<Ch, Tr>* out, Appendices&&... appendices)
{
    return make_record_extractor(std::allocator_arg, std::allocator<Ch>(),
        out, std::forward<Appendices>(appendices)...);
}

template <class Ch, class Tr, class... Appendices>
auto make_record_extractor(
    std::basic_ostream<Ch, Tr>& out, Appendices&&... appendices)
{
    return make_record_extractor(std::allocator_arg, std::allocator<Ch>(),
        out, std::forward<Appendices>(appendices)...);
}

}

#endif
