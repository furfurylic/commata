/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <commata/parse_csv.hpp>
#include <commata/wrapper_handlers.hpp>

#include "BaseTest.hpp"
#include "fancy_allocator.hpp"
#include "logging_allocator.hpp"
#include "tracking_allocator.hpp"
#include "simple_transcriptor.hpp"

using namespace std::string_view_literals;

using namespace commata;
using namespace commata::test;

namespace {

static_assert(std::is_nothrow_copy_assignable_v<parse_error>);

static_assert(std::is_trivially_copyable_v<csv_source<streambuf_input<char>>>);

template <class Ch>
class test_collector
{
    std::vector<std::vector<std::basic_string<Ch>>>* field_values_;
    std::basic_string<Ch> field_value_;

public:
    using char_type = Ch;

    explicit test_collector(
        std::vector<std::vector<std::basic_string<Ch>>>& field_values) :
        field_values_(&field_values)
    {}

    void start_record(const Ch* /*record_begin*/)
    {
        field_values_->emplace_back();
    }

    void update(const Ch* first, const Ch* last)
    {
        field_value_.append(first, last);
    }

    void finalize(const Ch* first, const Ch* last)
    {
        field_value_.append(first, last);
        field_values_->back().emplace_back();
        field_values_->back().back().swap(field_value_);
            // field_value_ is cleared here
    }

    void end_record(const Ch* /*record_end*/)
    {}

protected:
    const std::basic_string<Ch>& field_value() const noexcept
    {
        return field_value_;
    }
};

struct test_collector_empty_line_aware : test_collector<char>
{
    using test_collector::test_collector;

    void empty_physical_line(const char* where)
    {
        const char underlines[] = "___";
        start_record(where);
        finalize(underlines, underlines + (sizeof underlines) - 1);
            // These arguments do not satisfy TableHandler's requirements
            // (args of finalize shall point a range in the start and end of
            // the record), but that is fine because we know the implementation
            // of test_collector::finalize
        end_record(where);
    }
};

struct test_collector_handle_exception : test_collector<char>
{
    using test_collector::test_collector;

    void handle_exception()
    {
        assert(std::current_exception());
        std::throw_with_nested(std::runtime_error(
            "Currenct field value: \"" + field_value() + '"'));
    }
};

template <class Ch>
class test_collector_uncopyable
{
    std::vector<std::vector<std::basic_string<Ch>>> field_values_;
    test_collector<Ch> base_;

public:
    using char_type = Ch;

    explicit test_collector_uncopyable() :
        base_(field_values_)
    {}

    test_collector_uncopyable(const test_collector_uncopyable&) = delete;
    test_collector_uncopyable(test_collector_uncopyable&&) = delete;

    void start_record(const Ch* record_begin)
    {
        base_.start_record(record_begin);
    }

    void update(const Ch* first, const Ch* last)
    {
        base_.update(first, last);
    }

    void finalize(const Ch* first, const Ch* last)
    {
        base_.finalize(first, last);
    }

    void end_record(const Ch* record_end)
    {
        base_.end_record(record_end);
    }

    const std::vector<std::vector<std::basic_string<Ch>>>& field_values() const
    {
        return field_values_;
    }
};

template <class Ch, class F>
class buffer_check_handler
{
    F f_;

public:
    using char_type = Ch;

    explicit buffer_check_handler(F f) : f_(f) {}

    void start_buffer(const Ch* buffer_begin, const Ch* buffer_end)
    {
        f_(buffer_begin);
        f_(buffer_end - 1);
    }

    void start_record(const Ch*) {}
    void update(const Ch*, const Ch*) {}
    void finalize(const Ch*, const Ch*) {}
    void end_record(const Ch*) {}
};

template <class Ch, class F>
buffer_check_handler<Ch, F> make_buffer_check_handler(F f)
{
    return buffer_check_handler<Ch, F>(f);
}

static_assert(std::is_nothrow_move_constructible_v<
    csv_source<streambuf_input<char>>::parser_type<test_collector<char>>>);

} // end unnamed

// Precondition
static_assert(std::is_nothrow_move_constructible_v<test_collector<char>>);

// csv_source invocation
// ... with an rvalue of test_collector (move)
static_assert(std::is_nothrow_invocable_v<
    const csv_source<string_input<char>>&,
    test_collector<char>>);
static_assert(std::is_nothrow_invocable_v<
    csv_source<string_input<char>>,
    test_collector<char>>);
// ... with a reference-wrapped test_collector
static_assert(std::is_nothrow_invocable_v<
    const csv_source<string_input<char>>&,
    const std::reference_wrapper<test_collector<char>>&>);
static_assert(std::is_nothrow_invocable_v<
    csv_source<string_input<char>>,
    const std::reference_wrapper<test_collector<char>>&>);
// ... with a reference-wrapped test_collector and an allocator
static_assert(std::is_nothrow_invocable_v<
    const csv_source<string_input<char>>&,
    const std::reference_wrapper<test_collector<char>>&,
    std::size_t,
    tracking_allocator<std::allocator<char>>>);
static_assert(std::is_nothrow_invocable_v<
    csv_source<string_input<char>>,
    const std::reference_wrapper<test_collector<char>>&,
    std::size_t,
    tracking_allocator<std::allocator<char>>>);

// Parser's nothrow move constructibility
static_assert(std::is_nothrow_move_constructible_v<
    std::invoke_result_t<
        csv_source<string_input<char>>, test_collector<char>>>);

static_assert(
    std::is_same_v<
        typename csv_source<string_input<char>>::template
            parser_type<test_collector<char>>,
        typename csv_source<string_input<char>>::template
            parser_type<test_collector<char>, std::allocator<char>>>);
static_assert(
    std::is_same_v<
        std::invoke_result_t<const csv_source<string_input<wchar_t>>&,
            std::reference_wrapper<test_collector_uncopyable<wchar_t>>>,
        typename csv_source<string_input<wchar_t>>::template parser_type<
            reference_handler<test_collector_uncopyable<wchar_t>>,
            std::allocator<wchar_t>>>);

// On reference_handler
static_assert(
    std::is_nothrow_swappable_v<reference_handler<test_collector<char>>>);
static_assert(
    std::is_trivially_copyable_v<reference_handler<test_collector<char>>>);

struct TestParseCsvBasics :
    commata::test::BaseTestWithParam<std::size_t>
{};

TEST_P(TestParseCsvBasics, Narrow)
{
    std::string s = R"(,"col1", col2 ,col3,)" "\r\n"
                    "\n"
                    R"( cell10 ,,"cell)" "\r\n"
                    R"(12","cell""13""","")" "\n";
    std::stringbuf buf(s);
    std::vector<std::vector<std::string>> field_values;
    test_collector<char> collector(field_values);
    ASSERT_TRUE(parse_csv(buf, collector, GetParam()));
    ASSERT_EQ(2U, field_values.size());
    std::vector<std::string> expected_row0 =
        { "", "col1", " col2 ", "col3", "" };
    ASSERT_EQ(expected_row0, field_values[0]);
    std::vector<std::string> expected_row1 =
        { " cell10 ", "", "cell\r\n12", "cell\"13\"", "" };
    ASSERT_EQ(expected_row1, field_values[1]);
}

TEST_P(TestParseCsvBasics, Wide)
{
    std::wstring s = L"\n"
                     L"\r"
                     L"\r"
                     L"header1,header2\r"
                     L"\r\n"
                     L"value1,value2\n";
    std::vector<std::vector<std::wstring>> field_values;
    test_collector<wchar_t> collector(field_values);
    ASSERT_TRUE(parse_csv(make_char_input(std::wstringbuf(s)),
                          collector, GetParam()));
    ASSERT_EQ(2U, field_values.size());
    std::vector<std::wstring> expected_row0 = { L"header1", L"header2" };
    ASSERT_EQ(expected_row0, field_values[0]);
    std::vector<std::wstring> expected_row1 = { L"value1", L"value2" };
    ASSERT_EQ(expected_row1, field_values[1]);
}

TEST_P(TestParseCsvBasics, EmptyLineAware)
{
    std::wstring s = L"\n"
                     L"\r"
                     L"\r"
                     L"x1,x2\r"
                     L"\"\"\r\n"    // not an empty line
                     L"y1,y2\n";
    std::vector<std::vector<std::wstring>> field_values;
    ASSERT_TRUE(parse_csv(s, make_empty_physical_line_aware(
        test_collector<wchar_t>(field_values)), GetParam()));
    ASSERT_EQ(6U, field_values.size());
    ASSERT_TRUE(field_values[0].empty());
    ASSERT_TRUE(field_values[1].empty());
    ASSERT_TRUE(field_values[2].empty());
    std::vector<std::wstring> expected_row3 = { L"x1", L"x2" };
    ASSERT_EQ(expected_row3, field_values[3]);
    std::vector<std::wstring> expected_row4 = { L"" };
    ASSERT_EQ(expected_row4, field_values[4]);
    std::vector<std::wstring> expected_row5 = { L"y1", L"y2" };
    ASSERT_EQ(expected_row5, field_values[5]);
}

TEST_P(TestParseCsvBasics, AlreadyEmptyPhysicalLineAware)
{
    std::string s = "\n"
                    "ABC";
    std::vector<std::vector<std::string>> field_values;
    ASSERT_TRUE(parse_csv(s, make_empty_physical_line_aware(
        test_collector_empty_line_aware(field_values)), GetParam()));
    ASSERT_EQ(2U, field_values.size());
    ASSERT_EQ(1U, field_values[0].size());
    ASSERT_STREQ("___", field_values[0][0].c_str());
    ASSERT_EQ(1U, field_values[1].size());
    ASSERT_STREQ("ABC", field_values[1][0].c_str());
}

TEST_P(TestParseCsvBasics, CrCrLf)
{
    std::string s = "AB\r\r\nCD\r\r\r\nEF\r\r\r\r\n";
    std::vector<std::vector<std::string>> field_values;
    ASSERT_TRUE(parse_csv(s,
        test_collector_empty_line_aware(field_values), GetParam()));
    ASSERT_EQ(3U, field_values.size());
    ASSERT_EQ(1U, field_values[0].size());
    ASSERT_STREQ("AB", field_values[0][0].c_str());
    ASSERT_EQ(1U, field_values[1].size());
    ASSERT_STREQ("CD", field_values[1][0].c_str());
    ASSERT_EQ(1U, field_values[2].size());
    ASSERT_STREQ("EF", field_values[2][0].c_str());
}

TEST_P(TestParseCsvBasics, EvadeCopying)
{
    std::vector<std::size_t> allocations;
    logging_allocator<char> a(allocations);
    std::ostringstream transcript;
    ASSERT_TRUE(parse_csv("Name,Mass\nEarth,1\n\nMoon,0.0123",
        simple_transcriptor<const char>(transcript), GetParam(), a));
    const std::string s = std::move(transcript).str();

    // start_buffer and end_buffer are never reported except the begin and the
    // end no matter what is the buffer size
    ASSERT_EQ(std::string_view(s),
        "<{(Name)(Mass)}{(Earth)(1)}*{(Moon)(0.0123)}>"sv);

    // No allocation for buffers do not occur
    ASSERT_TRUE(allocations.empty());
}

TEST_P(TestParseCsvBasics, EvadeCopyingWhenNonconstVersionsExist)
{
    std::vector<std::size_t> allocations;
    logging_allocator<wchar_t> a(allocations);
    std::wostringstream transcript;
    ASSERT_TRUE(parse_csv(L"Name,Mass\nEarth,1\n\nMoon,0.0123",
        simple_transcriptor_with_nonconst_interface<const wchar_t>(transcript),
        GetParam(), a));
    const std::wstring s = std::move(transcript).str();

    // Now the handler have nonconst versions, but its char_type is
    // unchangedly const char, so const versions shall be called without any
    // buffer allocation
    ASSERT_EQ(std::wstring_view(s),
        L"<{(Name)(Mass)}{(Earth)(1)}*{(Moon)(0.0123)}>"sv);

    // No allocation for buffers do not occur
    ASSERT_TRUE(allocations.empty());
}

TEST_P(TestParseCsvBasics, PrefersNonconstWhenIndirect)
{
    std::vector<std::size_t> allocations;
    std::ostringstream transcript;
    ASSERT_TRUE(parse_csv(
        std::istringstream("Name,Mass\nEarth,1\n\nMoon,0.0123"),    // indirect
        simple_transcriptor_with_nonconst_interface<const char>(
            transcript, true),
        GetParam()));
    const std::string s = std::move(transcript).str();

    ASSERT_EQ(std::string_view(s),
        "{{((Name))((Mass))}}{{((Earth))((1))}}?{{((Moon))((0.0123))}}"sv);
}

namespace {

class aborting_handler
{
    std::string last_value_;
    std::string value_;

public:
    using char_type = char;

    bool start_record(const char* /*record_begin*/)
    {
        const bool ret = (last_value_ != "ABORT start_record");
        last_value_.clear();
        return ret;
    }

    bool end_record(const char* /*record_end*/)
    {
        return last_value_ != "ABORT end_record";
    }

    bool empty_physical_line(const char* /*where*/)
    {
        return last_value_ != "ABORT empty_physical_line";
    }

    void update(const char* first, const char* last)
    {
        value_.append(first, last);
    }

    bool finalize(const char* first, const char* last)
    {
        update(first, last);
        const bool ret = (value_ != "ABORT \"finalize\"");
        last_value_ = std::move(value_);
        value_.clear();
        return ret;
    }
};

}

TEST_P(TestParseCsvBasics, ParsePointGood)
{
    const auto l = "ABCD,EFGH,\"IJKL\"\r\n"sv;
    const auto r =make_csv_source(l)(aborting_handler())();
    ASSERT_EQ(l.size(), get_parse_point(r));
}

TEST_P(TestParseCsvBasics, ParsePointAbortStartRecord)
{
    const auto l = "ABCD,EFGH,\"ABORT start_record\"\r\n"sv;
    const auto r =make_csv_source(std::string(l) + "\"IJKL\"")
        (aborting_handler())();
    ASSERT_EQ(l.size(), get_parse_point(r));
}

TEST_P(TestParseCsvBasics, ParsePointAbortEndRecord)
{
    const auto l = "ABCD,EFGH,\"ABORT end_record\""sv;
    const auto r =make_csv_source(std::string(l) + "\r\r\n")
        (aborting_handler())();
    ASSERT_EQ(l.size(), get_parse_point(r));
}

TEST_P(TestParseCsvBasics, ParsePointAbortEmptyPhysicalLine)
{
    const auto l = "ABCD,EFGH,\"ABORT empty_physical_line\"\n"sv;
    const auto r =make_csv_source(std::string(l) + "\nXYZ\n")
        (aborting_handler())();
    ASSERT_EQ(l.size(), get_parse_point(r));
}

TEST_P(TestParseCsvBasics, ParsePointAbortFinalize)
{
    const auto l = "ABCD,EFGH,\"ABORT \"\"finalize\"\"\""sv;
    const auto r =make_csv_source(std::string(l) + "\r\n")
        (aborting_handler())();
    ASSERT_EQ(l.size(), get_parse_point(r));
}

INSTANTIATE_TEST_SUITE_P(,
    TestParseCsvBasics, testing::Values(1, 10, 1024));

struct TestParseCsvReference : commata::test::BaseTest
{};

TEST_F(TestParseCsvReference, Reference)
{
    const char s[] = "A,B\n\nZ";
    test_collector_uncopyable<char> collector;
    ASSERT_TRUE(parse_csv(s, (sizeof s) - 2, std::ref(collector)));
    ASSERT_EQ(1U, collector.field_values().size());
    ASSERT_EQ(2U, collector.field_values()[0].size());
    ASSERT_EQ("A", collector.field_values()[0][0]);
    ASSERT_EQ("B", collector.field_values()[0][1]);
}

TEST_F(TestParseCsvReference, EmptyLineAware)
{
    std::string s = "A,B\r\rC,D";
    test_collector_uncopyable<char> collector;
    auto sink = make_empty_physical_line_aware(std::ref(collector));
    ASSERT_TRUE(parse_csv(std::istringstream(s), std::move(sink)));
    ASSERT_EQ(3U, collector.field_values().size());
    ASSERT_EQ(2U, collector.field_values()[0].size());
    ASSERT_EQ("A", collector.field_values()[0][0]);
    ASSERT_EQ("B", collector.field_values()[0][1]);
    ASSERT_TRUE(collector.field_values()[1].empty());
    ASSERT_EQ(2U, collector.field_values()[2].size());
    ASSERT_EQ("C", collector.field_values()[2][0]);
    ASSERT_EQ("D", collector.field_values()[2][1]);
}

TEST_F(TestParseCsvReference, AlreadyEmptyPhysicalLineAware)
{
    std::string s = "\n"
                    "ABC";
    std::vector<std::vector<std::string>> field_values;
    test_collector_empty_line_aware collector(field_values);
    auto sink = make_empty_physical_line_aware(std::ref(collector));
    ASSERT_TRUE(parse_csv(std::istringstream(s), std::move(sink)));
    ASSERT_EQ(2U, field_values.size());
    ASSERT_EQ(1U, field_values[0].size());
    ASSERT_STREQ("___", field_values[0][0].c_str());
    ASSERT_EQ(1U, field_values[1].size());
    ASSERT_STREQ("ABC", field_values[1][0].c_str());
}

template <class Ch>
struct TestParseCsvFancy : BaseTest
{};

using Chs = testing::Types<char, wchar_t>;
TYPED_TEST_SUITE(TestParseCsvFancy, Chs);

TYPED_TEST(TestParseCsvFancy, Basics)
{
    using char_t = TypeParam;

    const auto str = char_helper<char_t>::str;
    const auto s = str("ABC,DEF,GHI,JKL\n123,456,789,0ab");
    std::basic_stringbuf<char_t> in(s);

    std::vector<std::pair<char*, char*>> allocated;
    tracking_allocator<fancy_allocator<char_t>> a(allocated);
    const auto f = [&a](const char_t* p) {
        if (!a.tracks(p)) {
            throw text_error("Not tracked");
        }
    };
    ASSERT_NO_THROW(parse_csv(in, make_buffer_check_handler<char_t>(f), 0, a));
}

struct TestParseCsvEndsWithoutLF :
    commata::test::BaseTestWithParam<std::pair<const char*, const char*>>
{};

TEST_P(TestParseCsvEndsWithoutLF, All)
{
    std::vector<std::vector<std::string>> field_values;
    test_collector<char> collector(field_values);
    ASSERT_TRUE(
        parse_csv(std::string_view(GetParam().first), collector, 1024));
    ASSERT_EQ(1U, field_values.size());
    std::ostringstream s;
    std::copy(field_values[0].cbegin(), field_values[0].cend(),
        std::ostream_iterator<std::string>(s, "/"));
    ASSERT_EQ(GetParam().second, std::move(s).str());
}

INSTANTIATE_TEST_SUITE_P(, TestParseCsvEndsWithoutLF,
    testing::Values(
        std::make_pair("ColA,ColB,ColC", "ColA/ColB/ColC/"),
        std::make_pair("ColA,ColB,\"ColC\"", "ColA/ColB/ColC/"),
        std::make_pair("ColA,ColB,", "ColA/ColB//")));

struct TestParseCsvErrors :
    commata::test::BaseTestWithParam<
        std::pair<const char*, std::pair<std::size_t, std::size_t>>>
{};

TEST_P(TestParseCsvErrors, Errors)
{
    std::string s = GetParam().first;
    std::vector<std::vector<std::string>> field_values;

    test_collector<char> collector(field_values);

    try {
        // the buffer is shorter than one line
        parse_csv(std::move(s), collector, 4);
        FAIL();
    } catch (const parse_error& e) {
        const auto pos = e.get_physical_position();
        ASSERT_TRUE(pos.has_value());
        ASSERT_EQ(GetParam().second.first, pos->first);
        ASSERT_EQ(GetParam().second.second, pos->second);
    }
}

INSTANTIATE_TEST_SUITE_P(, TestParseCsvErrors,
    testing::Values(
        std::make_pair("col\"1\"", std::make_pair(0, 3)),
        std::make_pair("\"col1", std::make_pair(0, 5)),
        std::make_pair("\"col1\",\"", std::make_pair(0, 8)),
        std::make_pair("col1\r\n\n\"col2\"a", std::make_pair(2, 6))));

struct TestParseCsvHandleException : commata::test::BaseTest
{};

TEST_F(TestParseCsvHandleException, All)
{
    std::vector<std::vector<std::string>> field_values;

    try {
        parse_csv("A,B,C\nX,\"YZ"sv,
            test_collector_handle_exception(field_values));
        FAIL();
    } catch (const std::runtime_error& e) {
        ASSERT_EQ(2U, field_values.size());
        ASSERT_EQ(1U, field_values.back().size());
        ASSERT_TRUE(
            std::string_view(e.what()).find("YZ"sv) != std::string_view::npos)
            << e.what();
        ASSERT_THROW(std::rethrow_if_nested(e), parse_error);
    }
}

namespace {

template <class Ch>
struct full_fledged
{
    Ch c;

    using char_type = Ch;
    [[nodiscard]] std::pair<Ch*, std::size_t> get_buffer()
        { return std::make_pair(&c, 1); }
    void release_buffer(const Ch*) {}
    void start_buffer(const Ch*, const Ch*) {}
    void end_buffer(const Ch*) {}
    void start_record(const Ch*) {}
    void end_record(const Ch*) {}
    void empty_physical_line(const Ch*) {}
    void update(const Ch*, const Ch*) {}
    void finalize(const Ch*, const Ch*) {}
};

} // end unnamed

struct TestCsvSource : commata::test::BaseTest
{};

// We'd like to confirm only that this function compiles
TEST_F(TestCsvSource, AcceptFullFledged)
{
    std::basic_stringbuf<char> in1("abc");
    make_csv_source(in1)(full_fledged<char>())();

    make_csv_source(L"def")(full_fledged<wchar_t>())();
}

TEST_F(TestCsvSource, Assign)
{
    auto abc = make_csv_source("ABC");
    auto xyz = make_csv_source("");

    static_assert(std::is_nothrow_move_constructible_v<decltype(abc)>);
    static_assert(std::is_nothrow_move_assignable_v<decltype(abc)>);

    xyz = std::move(abc);

    std::vector<std::vector<std::string>> field_values;
    test_collector<char> collector(field_values);
    xyz(std::move(collector))();

    ASSERT_EQ(1U, field_values.size());
    ASSERT_EQ(1U, field_values[0].size());
    ASSERT_STREQ("ABC", field_values[0][0].c_str());
}

TEST_F(TestCsvSource, Swap)
{
    auto abc = make_csv_source("ABC");
    csv_source xyz(string_input("XYZ"));

    static_assert(std::is_nothrow_swappable_v<decltype(abc)>);
    swap(abc, xyz);

    std::vector<std::vector<std::string>> field_values;
    test_collector<char> collector(field_values);
    abc(std::move(collector))();

    ASSERT_EQ(1U, field_values.size());
    ASSERT_EQ(1U, field_values[0].size());
    ASSERT_STREQ("XYZ", field_values[0][0].c_str());
}
