/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_GUARD_D53E08F9_CF1C_4762_BF77_1A6FB68C6A96
#define FURFURYLIC_GUARD_D53E08F9_CF1C_4762_BF77_1A6FB68C6A96

#include <ios>
#include <cstddef>
#include <cstdint>
#include <streambuf>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "csv_error.hpp"
#include "key_chars.hpp"

namespace furfurylic {
namespace commata {

class record_extraction_error :
    public csv_error
{
public:
    explicit record_extraction_error(std::string what_arg) :
        csv_error(std::move(what_arg))
    {}
};

template <class FieldNamePred, class FieldValuePred,
    class Ch, class Tr = std::char_traits<Ch>>
class record_extractor
{
    enum class record_mode : std::int_fast8_t
    {
        unknown,
        include,
        exclude
    };

    static const std::size_t npos = static_cast<std::size_t>(-1);

    FieldNamePred field_name_pred_;
    FieldValuePred field_value_pred_;

    record_mode header_mode_;
    record_mode record_mode_;

    std::size_t record_num_to_include_;
    std::size_t target_field_index_;

    std::size_t field_index_;
    const Ch* record_begin_;
    std::basic_streambuf<Ch, Tr>* out_;
    std::vector<Ch> field_buffer_;
    std::vector<Ch> record_buffer_;

public:
    record_extractor(
        std::basic_streambuf<Ch, Tr>& out,
        FieldNamePred field_name_pred, FieldValuePred field_value_pred,
        bool includes_header, std::size_t max_record_num) :
        field_name_pred_(std::move(field_name_pred)),
        field_value_pred_(std::move(field_value_pred)),
        header_mode_(includes_header ?
            record_mode::include : record_mode::exclude),
        record_num_to_include_(max_record_num), target_field_index_(npos),
        field_index_(0), out_(&out)
    {}

    record_extractor(record_extractor&&) = default;
    record_extractor& operator=(record_extractor&&) = default;

    void start_buffer(const Ch* buffer_begin)
    {
        record_begin_ = buffer_begin;
    }

    void end_buffer(const Ch* buffer_end)
    {
        if (record_mode_ != record_mode::exclude) {
            record_buffer_.insert(record_buffer_.cend(), record_begin_, buffer_end);
        }
    }

    void start_record(const Ch* record_begin)
    {
        record_begin_ = record_begin;
        record_mode_ = header_yet() ? header_mode_ : record_mode::unknown;
        field_index_ = 0;
        record_buffer_.clear();
    }

    bool update(const Ch* first, const Ch* last)
    {
        if ((header_yet() && (target_field_index_ == npos))
         || (field_index_ == target_field_index_)) {
            field_buffer_.insert(field_buffer_.cend(), first, last);
        }
        return true;
    }

    bool finalize(const Ch* first, const Ch* last)
    {
        if (header_yet()) {
            if ((target_field_index_ == npos)
             && with_field_buffer_appended(first, last, field_name_pred_)) {
                target_field_index_ = field_index_;
            }
        } else if (field_index_ == target_field_index_) {
            if (with_field_buffer_appended(first, last, field_value_pred_)) {
                record_mode_ = record_mode::include;
            } else {
                record_mode_ = record_mode::exclude;
                record_buffer_.clear();
            }
        }
        ++field_index_;
        return true;
    }

    bool end_record(const Ch* record_end)
    {
        if (header_yet() && (target_field_index_ == npos)) {
            throw record_extraction_error("No matching field found");
        }
        if (record_mode_ == record_mode::include) {
            if (!record_buffer_.empty()) {
                out_->sputn(record_buffer_.data(), record_buffer_.size());
            }
            out_->sputn(record_begin_, record_end - record_begin_);
            out_->sputc(detail::key_chars<Ch>::LF);
            if (!header_yet()
             && (record_num_to_include_ < static_cast<std::size_t>(-1))) {
                if (record_num_to_include_ == 1) {
                    return false;
                }
                --record_num_to_include_;
            }
            record_mode_ = record_mode::exclude; // to prevent end_buffer
                                                 // from doing anything
        }
        header_mode_ = record_mode::unknown;
        return true;
    }

private:
    bool header_yet() const
    {
        return header_mode_ != record_mode::unknown;
    }

    template <class F>
    auto with_field_buffer_appended(const Ch* first, const Ch* last, F& f)
      -> decltype(f(nullptr, nullptr))
    {
        if (field_buffer_.empty()) {
            return f(first, last);
        } else {
            field_buffer_.insert(field_buffer_.cend(), first, last);
            const auto r = f(
                field_buffer_.data(),
                field_buffer_.data() + field_buffer_.size());
            field_buffer_.clear();
            return r;
        }
    }
};

namespace detail {

template <class Ch, class Tr>
class string_eq
{
    std::basic_string<Ch, Tr> s_;

public:
    explicit string_eq(std::basic_string<Ch, Tr>&& s) :
        s_(std::move(s))
    {}

    bool operator()(const Ch* begin, const Ch* end) const
    {
        const auto rlen = static_cast<decltype(s_.size())>(end - begin);
        return (s_.size() == rlen)
            && (Tr::compare(s_.data(), begin, rlen) == 0);
    }
};

template <class Ch, class Tr, class T>
auto forward_as_string_pred(T&& t)
  -> typename std::enable_if<
        std::is_constructible<std::basic_string<Ch, Tr>, T&&>::value,
        string_eq<Ch, Tr>>::type
{
    return string_eq<Ch, Tr>(std::forward<T>(t));
}

template <class Ch, class Tr, class T>
auto forward_as_string_pred(T&& t)
  -> typename std::enable_if<
        !std::is_constructible<std::basic_string<Ch, Tr>, T&&>::value,
        T&&>::type
{
    return std::forward<T>(t);
}

template <class Ch, class Tr, class T>
using string_pred =
    typename std::conditional<
        std::is_constructible<std::basic_string<Ch, Tr>, T&&>::value,
        string_eq<Ch, Tr>,
        typename std::remove_reference<T>::type>::type;

template <class FieldNamePredF, class FieldValuePredF, class Ch, class Tr>
using record_extractor_from =
    record_extractor<
        string_pred<Ch, Tr, FieldNamePredF>,
        string_pred<Ch, Tr, FieldValuePredF>,
        Ch, Tr>;

} // end namespace detail

template <class FieldNamePredF, class FieldValuePredF, class Ch, class Tr>
detail::record_extractor_from<FieldNamePredF, FieldValuePredF, Ch, Tr>
make_record_extractor(
    std::basic_streambuf<Ch, Tr>& out,
    FieldNamePredF&& field_name_pred,
    FieldValuePredF&& field_value_pred,
    bool includes_header = true,
    std::size_t max_record_num = static_cast<std::size_t>(-1))
{
    return detail::record_extractor_from<
            FieldNamePredF, FieldValuePredF, Ch, Tr>(
        out,
        detail::forward_as_string_pred<Ch, Tr>(
            std::forward<FieldNamePredF>(field_name_pred)),
        detail::forward_as_string_pred<Ch, Tr>(
            std::forward<FieldValuePredF>(field_value_pred)),
        includes_header, max_record_num);
}

}}

#endif
