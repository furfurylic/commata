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
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "text_error.hpp"

#include "detail/allocation_only_allocator.hpp"
#include "detail/key_chars.hpp"
#include "detail/member_like_base.hpp"
#include "detail/typing_aid.hpp"
#include "detail/write_ntmbs.hpp"

namespace commata {
namespace detail::record_extraction {

template <class Ch, class Tr, class C>
class eq
{
    C c_;

public:
    explicit eq(C&& c) : c_(std::move(c))
    {}

    bool operator()(std::basic_string_view<Ch, Tr> s) const noexcept
    {
        auto       i  =   begin();
        const auto ie =   end();
        auto       j  = s.cbegin();
        const auto je = s.cend();
        if constexpr (std::is_same_v<decltype(i),
                                     std::remove_const_t<decltype(ie)>>) {
            return std::equal(i, ie, j, je,
                [](Ch l, Ch r) {
                    return Tr::eq(l, r);
                });
        } else {
            while (!((i == ie) || (j == je))) {
                if (!Tr::eq(*i, *j)) {
                    return false;
                }
                ++i;
                ++j;
            }
            return (i == ie) && (j == je);
        }
    }

    auto begin() const
    {
        if constexpr (std::is_pointer_v<C>) {
            return std::begin(*c_);
        } else {
            return std::begin(c_);
        }
    }

    auto end() const
    {
        if constexpr (std::is_pointer_v<C>) {
            return std::end(*c_);
        } else {
            return std::end(c_);
        }
    }
};

template <class Ch, class Tr, class C>
[[nodiscard]] auto make_eq(C&& c)
{
    if constexpr (std::is_reference_v<C>) {
        return eq<Ch, Tr, std::remove_reference_t<C>*>(std::addressof(c));
    } else {
        return eq<Ch, Tr, C>(std::forward<C>(c));
    }
}

template <class TrS, class Tr, class C>
std::basic_ostream<char, TrS>& operator<<(
    std::basic_ostream<char, TrS>& os, const eq<char, Tr, C>& eq)
{
    using begin_t = decltype(eq.begin());
    using end_t   = decltype(eq.end());
    typename std::iterator_traits<begin_t>::difference_type n;
    if constexpr (std::is_same_v<begin_t, end_t>) {
        n = std::distance(eq.begin(), eq.end());
    } else {
        n = 0;
        auto b = eq.begin();
        const auto e = eq.end();
        while (b != e) {
            ++n;
            ++b;
        }
    }
    return formatted_output(os, n, [&eq](auto* sb) {
        for (const auto c : eq) {
            if (sb->sputc(c) == Tr::eof()) {
                return false;
            }
        }
        return true;
    });
}

struct is_stream_writable_impl
{
    template <class Stream, class T>
    static auto check(T*) -> decltype(
        std::declval<Stream&>() << std::declval<const T&>(),
        std::true_type());

    template <class Stream, class T>
    static auto check(...) -> std::false_type;
};

template <class Stream, class T>
constexpr bool is_stream_writable_v =
    decltype(is_stream_writable_impl::check<Stream, T>(nullptr))();

template <class Ch, class Tr, class T>
constexpr bool is_plain_field_name_pred_v =
    std::is_pointer_v<T>
        // to match with function pointer types
 || std::is_convertible_v<T, bool (*)(std::basic_string_view<Ch, Tr>)>;
        // to match with no-capture closure types,
        // with gcc 7.3, whose objects are converted to function pointers
        // and again converted to bool values to produce dull "1" outputs
        // and generate dull -Waddress warnings;
        // this treatment is apparently not sufficient but seems to be
        // better than never

template <class Tr, class Ch, class T,
          std::enable_if_t<!(is_stream_writable_v<std::ostream, T>
                          || is_stream_writable_v<std::wostream, T>)
                        || is_plain_field_name_pred_v<Ch, Tr, T>,
                           std::nullptr_t> = nullptr>
void write_formatted_field_name_of(
    std::ostream&, std::string_view, const T&, const Ch*)
{}

template <class Tr, class Ch, class T,
          std::enable_if_t<is_stream_writable_v<std::ostream, T>
                        && !is_plain_field_name_pred_v<Ch, Tr, T>,
                           std::nullptr_t> = nullptr>
void write_formatted_field_name_of(
    std::ostream& o, std::string_view prefix, const T& t, const Ch*)
{
    o.write(prefix.data(), prefix.size());
    o << t;
}

template <class Tr, class Ch, class T,
          std::enable_if_t<!is_stream_writable_v<std::ostream, T>
                        && is_stream_writable_v<std::wostream, T>
                        && !is_plain_field_name_pred_v<Ch, Tr, T>,
                           std::nullptr_t> = nullptr>
void write_formatted_field_name_of(
    std::ostream& o, std::string_view prefix, const T& t, const Ch*)
{
    std::wstringstream wo;
    wo << t;
    o.write(prefix.data(), prefix.size());
    using it_t = std::istreambuf_iterator<wchar_t>;
    detail::write_ntmbs(o, it_t(wo), it_t());
}

template <class Tr, class... Args>
void write_formatted_field_name_of(
    std::ostream& o, std::string_view prefix,
    const eq<wchar_t, Tr, Args...>& t, const wchar_t*)
{
    o.write(prefix.data(), prefix.size());
    detail::write_ntmbs(o, t.begin(), t.end());
}

struct hollow_field_name_pred
{
    template <class Ch, class Tr>
    bool operator()(std::basic_string_view<Ch, Tr>) const noexcept
    {
        return true;
    }
};

} // end detail::record_extraction

class record_extraction_error :
    public text_error
{
public:
    using text_error::text_error;
};

constexpr std::size_t record_extractor_npos = static_cast<std::size_t>(-1);

namespace detail::record_extraction {

template <class FieldNamePred, class FieldValuePred,
    class Ch, class Tr, class Allocator>
class impl
{
    using alloc_t = detail::allocation_only_allocator<Allocator>;

    enum class record_mode : std::int_fast8_t
    {
        unknown,
        include,
        exclude
    };

    std::size_t record_num_to_include_;
    std::size_t target_field_index_;

    std::size_t field_index_;
    const Ch* current_begin_;   // current records's begin if not the buffer
                                // switched, current buffer's begin otherwise
    std::basic_streambuf<Ch, Tr>* out_;

    detail::base_member_pair<
        FieldNamePred,
        std::vector<Ch, alloc_t>/*field_buffer*/> nf_;
    detail::base_member_pair<
        FieldValuePred,
        std::vector<Ch, alloc_t>/*record_buffer*/> vr_;
                                // populated only after the buffer switched in
                                // a unknown (included or not) record and
                                // shall not overlap with interval
                                // [current_begin_, +inf)

    record_mode header_mode_;
    record_mode record_mode_;

public:
    using char_type = const Ch;
    using traits_type = Tr;
    using allocator_type = Allocator;

    template <class FieldNamePredR, class FieldValuePredR,
        std::enable_if_t<!std::is_integral_v<FieldNamePredR>>* = nullptr>
    impl(
            std::allocator_arg_t, const Allocator& alloc,
            std::basic_streambuf<Ch, Tr>& out,
            FieldNamePredR&& field_name_pred,
            FieldValuePredR&& field_value_pred,
            bool includes_header, std::size_t max_record_num) :
        impl(
            std::allocator_arg, alloc, out,
            std::forward<FieldNamePredR>(field_name_pred),
            std::forward<FieldValuePredR>(field_value_pred),
            record_extractor_npos, true, includes_header, max_record_num)
    {}

    template <class FieldValuePredR>
    impl(
            std::allocator_arg_t, const Allocator& alloc,
            std::basic_streambuf<Ch, Tr>& out,
            std::size_t target_field_index, FieldValuePredR&& field_value_pred,
            bool has_header,
            bool includes_header, std::size_t max_record_num) :
        impl(
            std::allocator_arg, alloc, out,
            FieldNamePred(),
            std::forward<FieldValuePredR>(field_value_pred),
            target_field_index, has_header, includes_header, max_record_num)
    {}

private:
    template <class FieldNamePredR, class FieldValuePredR>
    impl(
        std::allocator_arg_t, const Allocator& alloc,
        std::basic_streambuf<Ch, Tr>& out,
        FieldNamePredR&& field_name_pred, FieldValuePredR&& field_value_pred,
        std::size_t target_field_index, bool has_header,
        bool includes_header, std::size_t max_record_num) :
        record_num_to_include_(max_record_num),
        target_field_index_(target_field_index), field_index_(0),
        current_begin_(nullptr), out_(std::addressof(out)),
        nf_(std::forward<FieldNamePredR>(field_name_pred),
            std::vector<Ch, alloc_t>(alloc_t(alloc))),
        vr_(std::forward<FieldValuePredR>(field_value_pred),
            std::vector<Ch, alloc_t>(alloc_t(alloc))),
        header_mode_(has_header ?
                        includes_header ?
                            record_mode::include : record_mode::exclude :
                        record_mode::unknown),
        record_mode_(record_mode::exclude)
    {}

public:
    impl(impl&& other)
            noexcept(
                std::is_nothrow_move_constructible_v<FieldNamePred>
             && std::is_nothrow_move_constructible_v<FieldValuePred>) :
        record_num_to_include_(other.record_num_to_include_),
        target_field_index_(other.target_field_index_),
        field_index_(other.field_index_),
        current_begin_(other.current_begin_),
        out_(std::exchange(other.out_, nullptr)),
        nf_(std::move(other.nf_)), vr_(std::move(other.vr_)),
        header_mode_(other.header_mode_), record_mode_(other.record_mode_)
    {}

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
            record_buffer().insert(
                record_buffer().cend(), current_begin_, buffer_end);
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
        if ((is_in_header() && (target_field_index_ == record_extractor_npos))
         || (field_index_ == target_field_index_)) {
            field_buffer().insert(field_buffer().cend(), first, last);
        }
    }

    void finalize(const Ch* first, const Ch* last)
    {
        if (is_in_header()) {
            if ((target_field_index_ == record_extractor_npos)
             && with_field_buffer_appended(
                    first, last, std::ref(nf_.base()))) {
                target_field_index_ = field_index_;
            }
            ++field_index_;
            if (field_index_ >= record_extractor_npos) {
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
            if (field_index_ >= record_extractor_npos) {
                exclude();
            }
        }
    }

    bool end_record(const Ch* record_end)
    {
        if (is_in_header()) {
            if (target_field_index_ == record_extractor_npos) {
                throw no_matching_field();
            }
            flush_record(record_end);
            header_mode_ = record_mode::unknown;
        } else if (flush_record(record_end)) {
            if (record_num_to_include_ > 0) {
                if (record_num_to_include_ == 1) {
                    return false;
                }
                --record_num_to_include_;
            }
        }
        return true;
    }

    bool is_in_header() const noexcept
    {
        return header_mode_ != record_mode::unknown;
    }

private:
    decltype(auto) field_buffer()
    {
        return nf_.member();
    }

    decltype(auto) record_buffer()
    {
        return vr_.member();
    }

    template <class F>
    auto with_field_buffer_appended(const Ch* first, const Ch* last, F f)
    {
        auto& b = field_buffer();
        if (b.empty()) {
            return f(std::basic_string_view<Ch, Tr>(first, last - first));
        } else {
            b.insert(b.cend(), first, last);
            const auto r = f(std::basic_string_view<Ch, Tr>(
            	                b.data(), b.size()));
            b.clear();
            return r;
        }
    }

    record_extraction_error no_matching_field() const
    {
        using namespace std::string_view_literals;
        constexpr auto what_core = "No matching field"sv;
        try {
            std::ostringstream what;
            what << what_core;
            write_formatted_field_name_of<Tr>(what, " for "sv, nf_.base(),
                static_cast<const Ch*>(nullptr));
            return record_extraction_error(std::move(what).str());
        } catch (...) {
            return record_extraction_error(what_core);
        }
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
        if (out_) {
            assert(record_buffer().empty());
            out_->sputn(current_begin_, end - current_begin_);
        }
    }

    void flush_lf()
    {
        if (out_) {
            out_->sputc(key_chars<Ch>::lf_c);
        }
    }
};

template <class Ch, class Tr>
struct is_string_pred_impl
{
    template <class T>
    static auto check(T*) -> decltype(
        std::declval<bool&>() = std::declval<T&>()(
            std::declval<std::basic_string_view<Ch, Tr>&>()),
        std::true_type());

    template <class>
    static auto check(...) -> std::false_type;
};

// Why this is not defined as std::is_invocable_r_v<bool, T&,
// const std::basic_string_view<Ch, Tr>&> is that we'd like to exclude member
// pointers (they do not any harm, but we'd like to keep the interface simple)
template <class T, class Ch, class Tr>
constexpr bool is_string_pred_v = decltype(is_string_pred_impl<Ch, Tr>::
                                    template check<T>(nullptr))::value;

} // end detail::record_extraction

enum class header_forwarding : std::uint_fast8_t
{
    yes, no
};

template <class FieldNamePred, class FieldValuePred, class Ch,
    class Tr = std::char_traits<Ch>, class Allocator = std::allocator<Ch>>
class record_extractor :
    public detail::record_extraction::impl<
        FieldNamePred, FieldValuePred, Ch, Tr, Allocator>
{
    static_assert(detail::record_extraction::
                    is_string_pred_v<FieldNamePred, Ch, Tr>,
        "FieldNamePred of record_extractor is not a valid invocable type");
    static_assert(detail::record_extraction::
                    is_string_pred_v<FieldValuePred, Ch, Tr>,
        "FieldValuePred of record_extractor is not a valid invocable type");

    using base = detail::record_extraction::impl<
        FieldNamePred, FieldValuePred, Ch, Tr, Allocator>;

public:
    template <class FieldNamePredR, class FieldValuePredR,
        std::enable_if_t<
            std::is_constructible<FieldNamePred, FieldNamePredR&&>::value
         && std::is_constructible<FieldValuePred, FieldValuePredR&&>::value>*
        = nullptr>
    record_extractor(
        std::basic_streambuf<Ch, Tr>& out,
        FieldNamePredR&& field_name_pred, FieldValuePredR&& field_value_pred,
        header_forwarding header = header_forwarding::yes,
        std::size_t max_record_num = 0) :
        record_extractor(
            std::allocator_arg, Allocator(), out,
            std::forward<FieldNamePredR>(field_name_pred),
            std::forward<FieldValuePredR>(field_value_pred),
            header, max_record_num)
    {}

    template <class FieldNamePredR, class FieldValuePredR,
        std::enable_if_t<
            std::is_constructible<FieldNamePred, FieldNamePredR&&>::value
         && std::is_constructible<FieldValuePred, FieldValuePredR&&>::value>*
        = nullptr>
    record_extractor(
        std::allocator_arg_t, const Allocator& alloc,
        std::basic_streambuf<Ch, Tr>& out,
        FieldNamePredR&& field_name_pred, FieldValuePredR&& field_value_pred,
        header_forwarding header = header_forwarding::yes,
        std::size_t max_record_num = 0) :
        base(std::allocator_arg, alloc, out,
            std::forward<FieldNamePredR>(field_name_pred),
            std::forward<FieldValuePredR>(field_value_pred),
            (header == header_forwarding::yes), max_record_num)
    {}

    record_extractor(record_extractor&&) = default;
    ~record_extractor() = default;
};

template <class FieldNamePred, class FieldValuePred, class Ch, class Tr,
          class... Args>
record_extractor(std::basic_streambuf<Ch, Tr>&, FieldNamePred, FieldValuePred,
                 Args...)
 -> record_extractor<FieldNamePred, FieldValuePred, Ch, Tr,
                     std::allocator<Ch>>;

template <class FieldNamePred, class FieldValuePred, class Ch, class Tr,
          class Allocator, class... Args>
record_extractor(std::allocator_arg_t, Allocator,
    std::basic_streambuf<Ch, Tr>&, FieldNamePred, FieldValuePred, Args...)
 -> record_extractor<FieldNamePred, FieldValuePred, Ch, Tr, Allocator>;

template <class FieldValuePred, class Ch,
    class Tr = std::char_traits<Ch>, class Allocator = std::allocator<Ch>>
class record_extractor_with_indexed_key :
    public detail::record_extraction::impl<
        detail::record_extraction::hollow_field_name_pred, FieldValuePred,
        Ch, Tr, Allocator>
{
    static_assert(detail::record_extraction::
                    is_string_pred_v<FieldValuePred, Ch, Tr>,
        "FieldValuePred of record_extractor_with_indexed_key "
        "is not a valid invocable type");

    using base = detail::record_extraction::impl<
        detail::record_extraction::hollow_field_name_pred, FieldValuePred,
        Ch, Tr, Allocator>;

public:
    template <class FieldValuePredR,
        std::enable_if_t<
            std::is_constructible<FieldValuePred, FieldValuePredR&&>::value>*
        = nullptr>
    record_extractor_with_indexed_key(
        std::basic_streambuf<Ch, Tr>& out,
        std::size_t target_field_index, FieldValuePredR&& field_value_pred,
        std::optional<header_forwarding> header = header_forwarding::yes,
        std::size_t max_record_num = 0) :
        record_extractor_with_indexed_key(
            std::allocator_arg, Allocator(), out, target_field_index,
            std::forward<FieldValuePredR>(field_value_pred),
            header, max_record_num)
    {}

    template <class FieldValuePredR,
        std::enable_if_t<
            std::is_constructible<FieldValuePred, FieldValuePredR&&>::value>*
        = nullptr>
    record_extractor_with_indexed_key(
        std::allocator_arg_t, const Allocator& alloc,
        std::basic_streambuf<Ch, Tr>& out,
        std::size_t target_field_index, FieldValuePredR&& field_value_pred,
        std::optional<header_forwarding> header = header_forwarding::yes,
        std::size_t max_record_num = 0) :
        base(
            std::allocator_arg, alloc, out,
            sanitize_target_field_index(target_field_index),
            std::forward<FieldValuePredR>(field_value_pred),
            (header.has_value()),
            (header.has_value() && (*header == header_forwarding::yes)),
            max_record_num)
    {}

    record_extractor_with_indexed_key(
        record_extractor_with_indexed_key&&) = default;
    ~record_extractor_with_indexed_key() = default;

private:
    static std::size_t sanitize_target_field_index(std::size_t i)
    {
        if (i < record_extractor_npos) {
            return i;
        } else {
            std::ostringstream str;
            str << "Target field index too large: " << i;
            throw std::out_of_range(std::move(str).str());
        }
    }
};

template <class FieldValuePred, class Ch, class Tr, class... Args>
record_extractor_with_indexed_key(std::basic_streambuf<Ch, Tr>&, std::size_t,
    FieldValuePred, Args...)
 -> record_extractor_with_indexed_key<FieldValuePred, Ch, Tr,
                                      std::allocator<Ch>>;

template <class FieldValuePred, class Ch, class Tr, class Allocator,
          class... Args>
record_extractor_with_indexed_key(std::allocator_arg_t, Allocator,
    std::basic_streambuf<Ch, Tr>&, std::size_t, FieldValuePred, Args...)
 -> record_extractor_with_indexed_key<FieldValuePred, Ch, Tr, Allocator>;

namespace detail::record_extraction {

template <class Ch, class Tr, class T>
decltype(auto) make_string_pred(T&& s)
{
    if constexpr (std::is_convertible_v<T, const Ch*>) {
        const Ch* const str = std::forward<T>(s);
        return make_eq<Ch, Tr>(std::basic_string_view<Ch, Tr>(str));
    } else if constexpr (is_range_accessible_v<
                std::remove_reference_t<T>, Ch, std::forward_iterator_tag>) {
        return make_eq<Ch, Tr>(std::forward<T>(s));
    } else {
        return std::forward<T>(s);
    }
}

} // end detail::record_extraction

template <class FieldNamePred, class FieldValuePred,
    class Ch, class Tr, class Allocator, class... Appendices>
[[nodiscard]] auto make_record_extractor(
    std::allocator_arg_t, const Allocator& alloc,
    std::basic_streambuf<Ch, Tr>& out,
    FieldNamePred&& field_name_pred, FieldValuePred&& field_value_pred,
    Appendices&&... appendices)
 -> std::enable_if_t<
        detail::record_extraction::is_string_pred_v<std::decay_t<
            decltype(detail::record_extraction::make_string_pred<Ch, Tr>(
                std::declval<FieldNamePred>()))>, Ch, Tr>
     && detail::record_extraction::is_string_pred_v<std::decay_t<
            decltype(detail::record_extraction::make_string_pred<Ch, Tr>(
                std::declval<FieldValuePred>()))>, Ch, Tr>,
        record_extractor<
            std::decay_t<
                decltype(detail::record_extraction::make_string_pred<Ch, Tr>(
                    std::declval<FieldNamePred>()))>,
            std::decay_t<
                decltype(detail::record_extraction::make_string_pred<Ch, Tr>(
                    std::declval<FieldValuePred>()))>,
            Ch, Tr, Allocator>>
{
    auto fnp = detail::record_extraction::make_string_pred<Ch, Tr>(
        std::forward<FieldNamePred>(field_name_pred));
    auto fvp = detail::record_extraction::make_string_pred<Ch, Tr>(
        std::forward<FieldValuePred>(field_value_pred));
    return record_extractor(
        std::allocator_arg, alloc, out,
        std::move(fnp), std::move(fvp),
        std::forward<Appendices>(appendices)...);
}

template <class FieldValuePred,
    class Ch, class Tr, class Allocator, class... Appendices>
[[nodiscard]] auto make_record_extractor(
    std::allocator_arg_t, const Allocator& alloc,
    std::basic_streambuf<Ch, Tr>& out,
    std::size_t target_field_index, FieldValuePred&& field_value_pred,
    Appendices&&... appendices)
 -> std::enable_if_t<
        detail::record_extraction::is_string_pred_v<std::decay_t<
            decltype(detail::record_extraction::make_string_pred<Ch, Tr>(
                std::declval<FieldValuePred>()))>, Ch, Tr>,
        record_extractor_with_indexed_key<std::decay_t<
            decltype(detail::record_extraction::make_string_pred<Ch, Tr>(
                std::declval<FieldValuePred>()))>,
            Ch, Tr, Allocator>>
{
    auto fvp = detail::record_extraction::make_string_pred<Ch, Tr>(
        std::forward<FieldValuePred>(field_value_pred));
    return record_extractor_with_indexed_key(
        std::allocator_arg, alloc, out,
        static_cast<std::size_t>(target_field_index), std::move(fvp),
        std::forward<Appendices>(appendices)...);
}

template <class Ch, class Tr, class... Appendices>
[[nodiscard]] auto make_record_extractor(
    std::basic_streambuf<Ch, Tr>& out, Appendices&&... appendices)
 -> decltype(make_record_extractor(std::allocator_arg, std::allocator<Ch>(),
        out, std::forward<Appendices>(appendices)...))
{
    return make_record_extractor(std::allocator_arg, std::allocator<Ch>(),
        out, std::forward<Appendices>(appendices)...);
}

}

#endif
