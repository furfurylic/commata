/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_D53E08F9_CF1C_4762_BF77_1A6FB68C6A96
#define COMMATA_GUARD_D53E08F9_CF1C_4762_BF77_1A6FB68C6A96

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <ostream>
#include <streambuf>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "allocation_only_allocator.hpp"
#include "key_chars.hpp"
#include "member_like_base.hpp"
#include "text_error.hpp"
#include "typing_aid.hpp"

namespace commata {
namespace detail {

template <class T>
class format_field_name_of_impl
{
    const char* prefix_;
    const T* t_;

public:
    format_field_name_of_impl(const char* prefix, const T& t) noexcept :
        prefix_(prefix), t_(std::addressof(t))
    {}

    void operator()(std::ostream& o) const
    {
        o << prefix_ << *t_;
    }
};

template <class... Args>
auto format_field_name_of(Args...) noexcept
{
    return [](std::ostream&) {};
}

template <class T>
auto format_field_name_of(const char* prefix, const T& t,
    decltype(&(std::declval<std::ostream&>() << t)) = nullptr) noexcept
{
    return format_field_name_of_impl<T>(prefix, t);
}

template <class Ch>
struct hollow_field_name_pred
{
    bool operator()(const Ch*, const Ch*) const noexcept
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
class record_extractor_impl
{
    using alloc_t = detail::allocation_only_allocator<Allocator>;

    enum class record_mode : std::int_fast8_t
    {
        unknown,
        include,
        exclude
    };

    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    std::size_t record_num_to_include_;
    std::size_t target_field_index_;

    std::size_t field_index_;
    const Ch* current_begin_;   // current records's begin if not the buffer
                                // switched, current buffer's begin otherwise
    std::basic_streambuf<Ch, Tr>* out_;

    detail::base_member_pair<
        FieldNamePred,
        std::basic_string<Ch, Tr, alloc_t>/*field_buffer*/> nf_;
    detail::base_member_pair<
        FieldValuePred,
        std::basic_string<Ch, Tr, alloc_t>/*record_buffer*/> vr_;
                                // populated only after the buffer switched in
                                // a unknown (included or not) record and
                                // shall not overlap with interval
                                // [current_begin_, +inf)

    record_mode header_mode_;
    record_mode record_mode_;

    friend class record_extractor<
        FieldNamePred, FieldValuePred, Ch, Tr, Allocator>;
    friend class record_extractor_with_indexed_key<
        FieldValuePred, Ch, Tr, Allocator>;

public:
    using char_type = Ch;
    using traits_type = Tr;
    using allocator_type = Allocator;

    template <class FieldNamePredR, class FieldValuePredR,
        std::enable_if_t<
            !std::is_integral<FieldNamePredR>::value,
            std::nullptr_t> = nullptr>
    record_extractor_impl(
            std::allocator_arg_t, const Allocator& alloc,
            std::basic_streambuf<Ch, Tr>* out,
            FieldNamePredR&& field_name_pred,
            FieldValuePredR&& field_value_pred,
            bool includes_header, std::size_t max_record_num) :
        record_extractor_impl(
            std::allocator_arg, alloc, out,
            std::forward<FieldNamePredR>(field_name_pred),
            std::forward<FieldValuePredR>(field_value_pred),
            npos, includes_header, max_record_num)
    {}

    template <class FieldValuePredR>
    record_extractor_impl(
            std::allocator_arg_t, const Allocator& alloc,
            std::basic_streambuf<Ch, Tr>* out,
            std::size_t target_field_index, FieldValuePredR&& field_value_pred,
            bool includes_header, std::size_t max_record_num) :
        record_extractor_impl(
            std::allocator_arg, alloc, out,
            FieldNamePred(),
            std::forward<FieldValuePredR>(field_value_pred),
            target_field_index, includes_header, max_record_num)
    {}

private:
    template <class FieldNamePredR, class FieldValuePredR>
    record_extractor_impl(
        std::allocator_arg_t, const Allocator& alloc,
        std::basic_streambuf<Ch, Tr>* out,
        FieldNamePredR&& field_name_pred, FieldValuePredR&& field_value_pred,
        std::size_t target_field_index,
        bool includes_header, std::size_t max_record_num) :
        record_num_to_include_(max_record_num),
        target_field_index_(target_field_index), field_index_(0), out_(out),
        nf_(std::forward<FieldNamePredR>(field_name_pred),
            std::basic_string<Ch, Tr, alloc_t>(alloc_t(alloc))),
        vr_(std::forward<FieldValuePredR>(field_value_pred),
            std::basic_string<Ch, Tr, alloc_t>(alloc_t(alloc))),
        header_mode_(includes_header ?
            record_mode::include : record_mode::exclude),
        record_mode_(record_mode::exclude)
    {}

public:
    record_extractor_impl(record_extractor_impl&& other)
            noexcept(
                std::is_nothrow_move_constructible<FieldNamePred>::value
             && std::is_nothrow_move_constructible<FieldValuePred>::value) :
        record_num_to_include_(other.record_num_to_include_),
        target_field_index_(other.target_field_index_),
        field_index_(other.field_index_),
        current_begin_(other.current_begin_), out_(other.out_),
        nf_(std::move(other.nf_)), vr_(std::move(other.vr_)),
        header_mode_(other.header_mode_), record_mode_(other.record_mode_)
    {
        other.out_ = nullptr;
    }

    // Move-assignment shall be deleted because basic_string's propagation of
    // the allocator in C++14 is apocryphal (it does not seem able to be
    // noexcept unconditionally)

    allocator_type get_allocator() const noexcept
    {
        return field_buffer().get_allocator().base();
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
            record_buffer().append(current_begin_, buffer_end);
            break;
        default:
            break;
        }
    }

    void start_record(const Ch* record_begin)
    {
        current_begin_ = record_begin;
        record_mode_ = is_in_header() ? header_mode_ : record_mode::unknown;
        field_index_ = 0;
        assert(record_buffer().empty());
    }

    void update(const Ch* first, const Ch* last)
    {
        if ((is_in_header() && (target_field_index_ == npos))
         || (field_index_ == target_field_index_)) {
            field_buffer().append(first, last);
        }
    }

    void finalize(const Ch* first, const Ch* last)
    {
        using namespace std::placeholders;
        if (is_in_header()) {
            if ((target_field_index_ == npos)
             && with_field_buffer_appended(
                    first, last, std::ref(nf_.base()))) {
                target_field_index_ = field_index_;
            }
            ++field_index_;
            if (field_index_ >= npos) {
                throw no_matching_field();
            }
        } else {
            if ((record_mode_ == record_mode::unknown)
             && (field_index_ == target_field_index_)) {
                if (with_field_buffer_appended(
                        first, last, std::ref(vr_.base()))) {
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
        if (is_in_header()) {
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

    bool is_in_header() const noexcept
    {
        return header_mode_ != record_mode::unknown;
    }

private:
    std::basic_string<Ch, Tr, alloc_t>& field_buffer() noexcept
    {
        return nf_.member();
    }

    std::basic_string<Ch, Tr, alloc_t>& record_buffer() noexcept
    {
        return vr_.member();
    }

    template <class F>
    auto with_field_buffer_appended(const Ch* first, const Ch* last, F f)
    {
        auto& b = field_buffer();
        if (b.empty()) {
            return f(first, last);
        } else {
            b.append(first, last);
            const auto r = f(b.data(), b.data() + b.size());
            b.clear();
            return r;
        }
    }

    record_extraction_error no_matching_field() const
    {
        std::ostringstream what;
        what << "No matching field";
        format_field_name_of(" for ", nf_.base())(what);
        return record_extraction_error(what.str());
    }

    void include()
    {
        flush_record_buffer();
        record_mode_ = record_mode::include;
    }

    void exclude() noexcept
    {
        record_mode_ = record_mode::exclude;
        record_buffer().clear();
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
            assert(record_buffer().empty());
            return false;
        case record_mode::unknown:
            assert(!is_in_header());
            record_mode_ = record_mode::exclude;    // no such a far field
            record_buffer().clear();
            return false;
        default:
            assert(false);
            return false;
        }
    }

    void flush_record_buffer()
    {
        if (out_ && !record_buffer().empty()) {
            out_->sputn(record_buffer().data(), record_buffer().size());
            record_buffer().clear();
        }
    }

    void flush_current(const Ch* end)
    {
        assert(record_buffer().empty());
        if (out_) {
            out_->sputn(current_begin_, end - current_begin_);
        }
    }

    void flush_lf()
    {
        if (out_) {
            out_->sputc(key_chars<Ch>::LF);
        }
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
    template <class FieldNamePredR, class FieldValuePredR>
    record_extractor(
        std::basic_streambuf<Ch, Tr>* out,
        FieldNamePredR&& field_name_pred, FieldValuePredR&& field_value_pred,
        bool includes_header = true, std::size_t max_record_num = base::npos) :
        record_extractor(
            std::allocator_arg, Allocator(), out,
            std::forward<FieldNamePredR>(field_name_pred),
            std::forward<FieldValuePredR>(field_value_pred),
            includes_header, max_record_num)
    {}

    template <class FieldNamePredR, class FieldValuePredR>
    record_extractor(
        std::allocator_arg_t, const Allocator& alloc,
        std::basic_streambuf<Ch, Tr>* out,
        FieldNamePredR&& field_name_pred, FieldValuePredR&& field_value_pred,
        bool includes_header = true, std::size_t max_record_num = base::npos) :
        base(std::allocator_arg, alloc, out,
            std::forward<FieldNamePredR>(field_name_pred),
            std::forward<FieldValuePredR>(field_value_pred),
            includes_header, max_record_num)
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
    template <class FieldValuePredR>
    record_extractor_with_indexed_key(
        std::basic_streambuf<Ch, Tr>* out,
        std::size_t target_field_index, FieldValuePredR&& field_value_pred,
        bool includes_header = true, std::size_t max_record_num = base::npos) :
        record_extractor_with_indexed_key(
            std::allocator_arg, Allocator(), out, target_field_index,
            std::forward<FieldValuePredR>(field_value_pred),
            includes_header, max_record_num)
    {}

    template <class FieldValuePredR>
    record_extractor_with_indexed_key(
        std::allocator_arg_t, const Allocator& alloc,
        std::basic_streambuf<Ch, Tr>* out,
        std::size_t target_field_index, FieldValuePredR&& field_value_pred,
        bool includes_header = true, std::size_t max_record_num = base::npos) :
        base(std::allocator_arg, alloc, out,
            target_field_index,
            std::forward<FieldValuePredR>(field_value_pred),
            includes_header, max_record_num)
    {}

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
    explicit string_eq(std::basic_string<Ch, Tr, Allocator>&& s) noexcept :
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

struct has_const_iterator_impl
{
    template <class T>
    static auto check(T*) ->
        decltype(std::declval<typename T::const_iterator>(), std::true_type());

    template <class T>
    static auto check(...) -> std::false_type;
};

template <class T>
struct has_const_iterator :
    decltype(has_const_iterator_impl::check<T>(nullptr))
{};

template <class Ch, class Tr, class Allocator>
auto make_string_pred(
    std::basic_string<Ch, Tr, Allocator>&& s, const Allocator&) noexcept
{
    return string_eq<Ch, Tr, Allocator>(
        std::basic_string<Ch, Tr, Allocator>(std::move(s)));
}

template <class Ch, class Tr, class Allocator, class T>
auto make_string_pred(T&& s, const Allocator& a)
 -> std::enable_if_t<
        std::is_constructible<
            std::basic_string<Ch, Tr, Allocator>,
            T&&,
            const Allocator&>::value,
        string_eq<Ch, Tr, Allocator>>
{
    return string_eq<Ch, Tr, Allocator>(
        std::basic_string<Ch, Tr, Allocator>(std::forward<T>(s), a));
}

template <class Ch, class Tr, class Allocator, class T>
auto make_string_pred(T&& s, const Allocator& a)
 -> std::enable_if_t<
        !std::is_constructible<
            std::basic_string<Ch, Tr, Allocator>,
            T&&,
            const Allocator&>::value
     && has_const_iterator<std::remove_reference_t<T>>::value,
        string_eq<Ch, Tr, Allocator>>
{
    return string_eq<Ch, Tr, Allocator>(
        std::basic_string<Ch, Tr, Allocator>(
            std::forward<T>(s).cbegin(), std::forward<T>(s).cend(), a));
}

// Watch out for the return type, which is not string_eq
template <class Ch, class Tr, class Allocator, class T>
auto make_string_pred(T&& s, const Allocator&)
 -> std::enable_if_t<
        !std::is_constructible<
            std::basic_string<Ch, Tr, Allocator>,
            T&&,
            const Allocator&>::value
     && !has_const_iterator<std::remove_reference_t<T>>::value,
        T&&>
{
    // This "not uses-allocator construction" is intentional because
    // what we would like to do here is a mere move/copy by forwarding
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
