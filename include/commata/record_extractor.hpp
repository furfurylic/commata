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
#include "write_ntmbs.hpp"

namespace commata {
namespace detail { namespace record_extraction {

template <class Ch, class Tr = std::char_traits<Ch>>
class range_eq
{
    const Ch* begin_;
    std::size_t len_;

public:
    template <class Allocator>
    explicit range_eq(const std::basic_string<Ch, Tr, Allocator>& s) noexcept :
        begin_(s.data()), len_(s.size())
    {}

    explicit range_eq(const Ch* s) :
        begin_(s), len_(Tr::length(s))
    {}

    bool operator()(const Ch* begin, const Ch* end) const noexcept
    {
        const std::size_t rlen = end - begin;
        return (len_ == rlen) && (Tr::compare(begin_, begin, rlen) == 0);
    }

    std::pair<const Ch*, const Ch*> get() const noexcept
    {
        return std::make_pair(begin_, begin_ + len_);
    }
};

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

    const std::basic_string<Ch, Tr, Allocator>& get() const noexcept
    {
        return s_;
    }
};

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
struct is_stream_writable :
    decltype(is_stream_writable_impl::check<Stream, T>(nullptr))
{};

template <class Ch, class T>
struct is_plain_field_name_pred :
    std::integral_constant<bool,
        std::is_pointer<T>::value
            // to match with function pointer types
     || std::is_convertible<T, bool (*)(const Ch*, const Ch*)>::value>
            // to match with no-capture closure types,
            // with gcc 7.3, whose objects are converted to function pointers
            // and again converted to bool values to produce dull "1" outputs
            // and generate dull -Waddress warnings;
            // this treatment is apparently not sufficient but seems to be
            // better than never
{};

template <class Ch, class T,
          std::enable_if_t<!(is_stream_writable<std::ostream, T>::value
                          || is_stream_writable<std::wostream, T>::value)
                        || is_plain_field_name_pred<Ch, T>::value,
                           std::nullptr_t> = nullptr>
void write_formatted_field_name_of(
    std::ostream&, const char*, const T&, const Ch*)
{}

template <class Ch, class T,
          std::enable_if_t<is_stream_writable<std::ostream, T>::value
                        && !is_plain_field_name_pred<Ch, T>::value,
                           std::nullptr_t> = nullptr>
void write_formatted_field_name_of(
    std::ostream& o, const char* prefix, const T& t, const Ch*)
{
    o.rdbuf()->sputn(prefix, std::strlen(prefix));
    o << t;
}

template <class Ch, class T,
          std::enable_if_t<!is_stream_writable<std::ostream, T>::value
                        && is_stream_writable<std::wostream, T>::value
                        && !is_plain_field_name_pred<Ch, T>::value,
                           std::nullptr_t> = nullptr>
void write_formatted_field_name_of(
    std::ostream& o, const char* prefix, const T& t, const Ch*)
{
    std::wstringstream wo;
    wo << t;
    o.rdbuf()->sputn(prefix, std::strlen(prefix));
    using it_t = std::istreambuf_iterator<wchar_t>;
    detail::write_ntmbs(o, it_t(wo), it_t());
}

template <class Tr>
void write_formatted_field_name_of(
    std::ostream& o, const char* prefix,
    const range_eq<wchar_t, Tr>& t, const wchar_t*)
{
    o.rdbuf()->sputn(prefix, std::strlen(prefix));
    detail::write_ntmbs(o, t.get().first, t.get().second);
}

template <class Tr, class Allocator>
void write_formatted_field_name_of(
    std::ostream& o, const char* prefix,
    const string_eq<wchar_t, Tr, Allocator>& t, const wchar_t*)
{
    o.rdbuf()->sputn(prefix, std::strlen(prefix));
    detail::write_ntmbs(o, t.get().cbegin(), t.get().cend());
}

struct hollow_field_name_pred
{
    template <class Ch>
    bool operator()(const Ch*, const Ch*) const noexcept
    {
        return true;
    }
};

}} // end detail::record_extraction

class record_extraction_error :
    public text_error
{
public:
    using text_error::text_error;
};

constexpr std::size_t record_extractor_npos = static_cast<std::size_t>(-1);

namespace detail { namespace record_extraction {

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

public:
    using char_type = Ch;
    using traits_type = Tr;
    using allocator_type = Allocator;

    template <class FieldNamePredR, class FieldValuePredR,
        std::enable_if_t<!std::is_integral<FieldNamePredR>::value>* = nullptr>
    impl(
            std::allocator_arg_t, const Allocator& alloc,
            std::basic_streambuf<Ch, Tr>* out,
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
            std::basic_streambuf<Ch, Tr>* out,
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
        std::basic_streambuf<Ch, Tr>* out,
        FieldNamePredR&& field_name_pred, FieldValuePredR&& field_value_pred,
        std::size_t target_field_index, bool has_header,
        bool includes_header, std::size_t max_record_num) :
        record_num_to_include_(max_record_num),
        target_field_index_(target_field_index), field_index_(0), out_(out),
        nf_(std::forward<FieldNamePredR>(field_name_pred),
            std::basic_string<Ch, Tr, alloc_t>(alloc_t(alloc))),
        vr_(std::forward<FieldValuePredR>(field_value_pred),
            std::basic_string<Ch, Tr, alloc_t>(alloc_t(alloc))),
        header_mode_(has_header ?
                        includes_header ?
                            record_mode::include : record_mode::exclude :
                        record_mode::unknown),
        record_mode_(record_mode::exclude)
    {}

public:
    impl(impl&& other)
            noexcept(
                std::is_nothrow_move_constructible<FieldNamePred>::value
             && std::is_nothrow_move_constructible<FieldValuePred>::value) :
        record_num_to_include_(other.record_num_to_include_),
        target_field_index_(other.target_field_index_),
        field_index_(other.field_index_),
        current_begin_(other.current_begin_),
        out_(std::exchange(other.out_, nullptr)),
        nf_(std::move(other.nf_)), vr_(std::move(other.vr_)),
        header_mode_(other.header_mode_), record_mode_(other.record_mode_)
    {}

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
        if ((is_in_header() && (target_field_index_ == record_extractor_npos))
         || (field_index_ == target_field_index_)) {
            field_buffer().append(first, last);
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
        const char* const what_core = "No matching field";
        try {
            std::ostringstream what;
            what << what_core;
            write_formatted_field_name_of(what, " for ", nf_.base(),
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

}} // end detail::record_extraction

enum header_forwarding : unsigned
{
    header_forwarding_yes = 1,
    header_forwarding_no = 2
};

template <class FieldNamePred, class FieldValuePred, class Ch,
    class Tr = std::char_traits<Ch>, class Allocator = std::allocator<Ch>>
class record_extractor :
    public detail::record_extraction::impl<
        FieldNamePred, FieldValuePred, Ch, Tr, Allocator>
{
    using base = detail::record_extraction::impl<
        FieldNamePred, FieldValuePred, Ch, Tr, Allocator>;

public:
    template <class FieldNamePredR, class FieldValuePredR,
        std::enable_if_t<
            std::is_constructible<FieldNamePred, FieldNamePredR&&>::value
         && std::is_constructible<FieldValuePred, FieldValuePredR&&>::value>*
        = nullptr>
    record_extractor(
        std::basic_streambuf<Ch, Tr>* out,
        FieldNamePredR&& field_name_pred, FieldValuePredR&& field_value_pred,
        header_forwarding header = header_forwarding_yes,
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
        std::basic_streambuf<Ch, Tr>* out,
        FieldNamePredR&& field_name_pred, FieldValuePredR&& field_value_pred,
        header_forwarding header = header_forwarding_yes,
        std::size_t max_record_num = 0) :
        base(std::allocator_arg, alloc, out,
            std::forward<FieldNamePredR>(field_name_pred),
            std::forward<FieldValuePredR>(field_value_pred),
            (header == header_forwarding_yes), max_record_num)
    {}

    record_extractor(record_extractor&&) = default;
    ~record_extractor() = default;
};

template <class FieldValuePred, class Ch,
    class Tr = std::char_traits<Ch>, class Allocator = std::allocator<Ch>>
class record_extractor_with_indexed_key :
    public detail::record_extraction::impl<
        detail::record_extraction::hollow_field_name_pred, FieldValuePred,
        Ch, Tr, Allocator>
{
    using base = detail::record_extraction::impl<
        detail::record_extraction::hollow_field_name_pred, FieldValuePred,
        Ch, Tr, Allocator>;

public:
    template <class FieldValuePredR,
        std::enable_if_t<
            std::is_constructible<FieldValuePred, FieldValuePredR&&>::value>*
        = nullptr>
    record_extractor_with_indexed_key(
        std::basic_streambuf<Ch, Tr>* out,
        std::size_t target_field_index, FieldValuePredR&& field_value_pred,
        std::underlying_type_t<header_forwarding> header
            = header_forwarding_yes,
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
        std::basic_streambuf<Ch, Tr>* out,
        std::size_t target_field_index, FieldValuePredR&& field_value_pred,
        std::underlying_type_t<header_forwarding> header
            = header_forwarding_yes,
        std::size_t max_record_num = 0) :
        base(
            std::allocator_arg, alloc, out,
            sanitize_target_field_index(target_field_index),
            std::forward<FieldValuePredR>(field_value_pred),
            (header != 0U), (header == header_forwarding_yes), max_record_num)
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
            throw std::out_of_range(str.str());
        }
    }
};

namespace detail { namespace record_extraction {

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

template <class Ch, class Tr, class Allocator, class OtherAllocator>
auto make_string_pred(
    std::basic_string<Ch, Tr, OtherAllocator>&& s, const Allocator&) noexcept
{
    return string_eq<Ch, Tr, OtherAllocator>(std::move(s));
}

template <class Ch, class Tr, class Allocator, class OtherAllocator>
auto make_string_pred(const std::basic_string<Ch, Tr, OtherAllocator>&& s,
                      const Allocator& a)
{
    return string_eq<Ch, Tr, Allocator>(
        std::basic_string<Ch, Tr, Allocator>(s.cbegin(), s.cend(), a));
}

template <class Ch, class Tr, class OtherAllocator, class Allocator>
auto make_string_pred(const std::basic_string<Ch, Tr, OtherAllocator>& s,
                      const Allocator&) noexcept
{
    return range_eq<Ch, Tr>(s);
}

template <class Ch, class Tr, class Allocator>
auto make_string_pred(const Ch* s, const Allocator&) noexcept
{
    return range_eq<Ch, Tr>(s);
}

template <class Ch, class Tr, class Allocator, class T>
auto make_string_pred(T&& s, const Allocator& a)
 -> std::enable_if_t<
        !detail::is_std_string_of_ch_tr<
            std::remove_const_t<std::remove_reference_t<T>>, Ch, Tr>::value
     && !std::is_same<
            std::remove_const_t<std::remove_reference_t<T>>, Ch*>::value
     && has_const_iterator<std::remove_reference_t<T>>::value,
        string_eq<Ch, Tr, Allocator>>
{
    return string_eq<Ch, Tr, Allocator>(
        std::basic_string<Ch, Tr, Allocator>(s.cbegin(), s.cend(), a));
}

// Watch out for the return type, which is not string_eq
template <class Ch, class Tr, class Allocator, class T>
auto make_string_pred(T&& s, const Allocator&)
 -> std::enable_if_t<
        !detail::is_std_string_of_ch_tr<
            std::remove_const_t<std::remove_reference_t<T>>, Ch, Tr>::value
     && !std::is_same<
            std::remove_const_t<std::remove_reference_t<T>>, Ch*>::value
     && !has_const_iterator<std::remove_reference_t<T>>::value,
        T&&>
{
    // This "not uses-allocator construction" is intentional because
    // what we would like to do here is a mere move/copy by forwarding
    return std::forward<T>(s);
}

struct is_string_pred_impl
{
    template <class T, class Ch>
    static auto check(T*) ->
        decltype(
            std::declval<bool&>() = std::declval<T>()(
                std::declval<const Ch*>(), std::declval<const Ch*>()),
            std::true_type());

    template <class T, class Ch>
    static auto check(...) -> std::false_type;
};

template <class T, class Ch>
struct is_string_pred : decltype(is_string_pred_impl::check<T, Ch>(nullptr))
{};

}} // end detail::record_extraction

template <class FieldNamePred, class FieldValuePred,
    class Ch, class Tr, class Allocator, class... Appendices>
auto make_record_extractor(
    std::allocator_arg_t, const Allocator& alloc,
    std::basic_streambuf<Ch, Tr>* out,
    FieldNamePred&& field_name_pred, FieldValuePred&& field_value_pred,
    Appendices&&... appendices)
 -> std::enable_if_t<
        detail::record_extraction::is_string_pred<std::decay_t<
            decltype(detail::record_extraction::make_string_pred<Ch, Tr>(
                std::declval<FieldNamePred>(),
                std::declval<Allocator>()))>, Ch>::value
     && detail::record_extraction::is_string_pred<std::decay_t<
            decltype(detail::record_extraction::make_string_pred<Ch, Tr>(
                std::declval<FieldValuePred>(),
                std::declval<Allocator>()))>, Ch>::value,
        record_extractor<
            std::decay_t<
                decltype(detail::record_extraction::make_string_pred<Ch, Tr>(
                    std::declval<FieldNamePred>(),
                    std::declval<Allocator>()))>,
            std::decay_t<
                decltype(detail::record_extraction::make_string_pred<Ch, Tr>(
                    std::declval<FieldValuePred>(),
                    std::declval<Allocator>()))>,
            Ch, Tr, Allocator>>
{
    auto fnp = detail::record_extraction::make_string_pred<Ch, Tr>(
        std::forward<FieldNamePred>(field_name_pred), alloc);
    auto fvp = detail::record_extraction::make_string_pred<Ch, Tr>(
        std::forward<FieldValuePred>(field_value_pred), alloc);
    return record_extractor<decltype(fnp), decltype(fvp), Ch, Tr, Allocator>(
        std::allocator_arg, alloc, out,
        std::move(fnp), std::move(fvp),
        std::forward<Appendices>(appendices)...);
}

template <class FieldValuePred,
    class Ch, class Tr, class Allocator, class... Appendices>
auto make_record_extractor(
    std::allocator_arg_t, const Allocator& alloc,
    std::basic_streambuf<Ch, Tr>* out,
    std::size_t target_field_index, FieldValuePred&& field_value_pred,
    Appendices&&... appendices)
 -> std::enable_if_t<
        detail::record_extraction::is_string_pred<std::decay_t<
            decltype(detail::record_extraction::make_string_pred<Ch, Tr>(
                std::declval<FieldValuePred>(),
                std::declval<Allocator>()))>, Ch>::value,
        record_extractor_with_indexed_key<std::decay_t<
            decltype(detail::record_extraction::make_string_pred<Ch, Tr>(
                std::declval<FieldValuePred>(),
                std::declval<Allocator>()))>,
            Ch, Tr, Allocator>>
{
    auto fvp = detail::record_extraction::make_string_pred<Ch, Tr>(
        std::forward<FieldValuePred>(field_value_pred), alloc);
    return record_extractor_with_indexed_key<decltype(fvp), Ch, Tr, Allocator>(
        std::allocator_arg, alloc, out,
        static_cast<std::size_t>(target_field_index), std::move(fvp),
        std::forward<Appendices>(appendices)...);
}

template <class Ch, class Tr, class Allocator, class... Appendices>
auto make_record_extractor(
    std::allocator_arg_t, const Allocator& alloc,
    std::basic_ostream<Ch, Tr>& out, Appendices&&... appendices)
 -> decltype(make_record_extractor(
        std::allocator_arg, alloc, out.rdbuf(),
        std::forward<Appendices>(appendices)...))
{
    return make_record_extractor(
        std::allocator_arg, alloc, out.rdbuf(),
        std::forward<Appendices>(appendices)...);
}

template <class Ch, class Tr, class... Appendices>
auto make_record_extractor(
    std::basic_streambuf<Ch, Tr>* out, Appendices&&... appendices)
 -> decltype(make_record_extractor(std::allocator_arg, std::allocator<Ch>(),
        out, std::forward<Appendices>(appendices)...))
{
    return make_record_extractor(std::allocator_arg, std::allocator<Ch>(),
        out, std::forward<Appendices>(appendices)...);
}

template <class Ch, class Tr, class... Appendices>
auto make_record_extractor(
    std::basic_ostream<Ch, Tr>& out, Appendices&&... appendices)
 -> decltype(make_record_extractor(std::allocator_arg, std::allocator<Ch>(),
        out, std::forward<Appendices>(appendices)...))
{
    return make_record_extractor(std::allocator_arg, std::allocator<Ch>(),
        out, std::forward<Appendices>(appendices)...);
}

}

#endif
