/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <commata/empty_physical_line_aware_handler.hpp>
#include <commata/parse_csv.hpp>

#include "BaseTest.hpp"

using namespace commata;

namespace {

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
};

template <class Ch>
class test_collector2
{
    std::vector<std::vector<std::basic_string<Ch>>> field_values_;
    test_collector<Ch> base_;

public:
    using char_type = Ch;

    explicit test_collector2() :
        base_(field_values_)
    {}

    test_collector2(const test_collector2&) = delete;
    test_collector2(test_collector2&&) = delete;

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

}

struct TestParseCsvBasics :
    commata::test::BaseTestWithParam<std::size_t>
{};

TEST_P(TestParseCsvBasics, Narrow)
{
    std::string s = ",\"col1\", col2 ,col3,\r\n\n"
                    " cell10 ,,\"cell\r\n12\",\"cell\"\"13\"\"\",\"\"\n";
    std::stringbuf buf(s);
    std::vector<std::vector<std::string>> field_values;
    test_collector<char> collector(field_values);
    ASSERT_TRUE(parse_csv(&buf, collector, GetParam()));
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
    std::wstring s = L"\n\r\rheader1,header2\r\r\n"
                     L"value1,value2\n";
    std::wstringbuf buf(s);
    std::vector<std::vector<std::wstring>> field_values;
    test_collector<wchar_t> collector(field_values);
    ASSERT_TRUE(parse_csv(&buf, collector, GetParam()));
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
    std::wstringbuf buf(s);
    std::vector<std::vector<std::wstring>> field_values;
    ASSERT_TRUE(parse_csv(&buf, make_empty_physical_line_aware(
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

INSTANTIATE_TEST_SUITE_P(,
    TestParseCsvBasics, testing::Values(1, 10, 1024));

struct TestParseCsvReference : commata::test::BaseTest
{};

TEST_F(TestParseCsvReference, Reference)
{
    std::string s = "A,B\n\n";
    std::stringbuf buf(s);
    test_collector2<char> collector;
    ASSERT_TRUE(parse_csv(&buf, std::ref(collector)));
    ASSERT_EQ(1U, collector.field_values().size());
    ASSERT_EQ(2U, collector.field_values()[0].size());
    ASSERT_EQ("A", collector.field_values()[0][0]);
    ASSERT_EQ("B", collector.field_values()[0][1]);
}

TEST_F(TestParseCsvReference, EmptyLineAware)
{
    std::string s = "A,B\r\rC,D";
    std::stringbuf buf(s);
    test_collector2<char> collector;
    auto sink = make_empty_physical_line_aware(std::ref(collector));
    ASSERT_TRUE(parse_csv(&buf, std::move(sink)));
    ASSERT_EQ(3U, collector.field_values().size());
    ASSERT_EQ(2U, collector.field_values()[0].size());
    ASSERT_EQ("A", collector.field_values()[0][0]);
    ASSERT_EQ("B", collector.field_values()[0][1]);
    ASSERT_TRUE(collector.field_values()[1].empty());
    ASSERT_EQ(2U, collector.field_values()[2].size());
    ASSERT_EQ("C", collector.field_values()[2][0]);
    ASSERT_EQ("D", collector.field_values()[2][1]);
}

struct TestParseCsvEndsWithoutLF :
    testing::TestWithParam<std::pair<const char*, const char*>>
{};

TEST_P(TestParseCsvEndsWithoutLF, All)
{
    std::stringbuf buf(GetParam().first);
    std::vector<std::vector<std::string>> field_values;
    test_collector<char> collector(field_values);
    ASSERT_TRUE(parse_csv(&buf, collector, 1024));
    ASSERT_EQ(1U, field_values.size());
    std::stringstream s;
    std::copy(field_values[0].cbegin(), field_values[0].cend(),
        std::ostream_iterator<std::string>(s, "/"));
    ASSERT_EQ(GetParam().second, s.str());
}

INSTANTIATE_TEST_SUITE_P(, TestParseCsvEndsWithoutLF,
    testing::Values(
        std::make_pair("ColA,ColB,ColC", "ColA/ColB/ColC/"),
        std::make_pair("ColA,ColB,\"ColC\"", "ColA/ColB/ColC/"),
        std::make_pair("ColA,ColB,", "ColA/ColB//")));

struct TestParseCsvErrors :
    testing::TestWithParam<
        std::pair<const char*, std::pair<std::size_t, std::size_t>>>
{};

TEST_P(TestParseCsvErrors, Errors)
{
    std::string s = GetParam().first;
    std::stringbuf buf(s);
    std::vector<std::vector<std::string>> field_values;

    test_collector<char> collector(field_values);

    try {
        // the buffer is shorter than one line
        parse_csv(&buf, collector, 4);
        FAIL();
    } catch (const parse_error& e) {
        const auto pos = e.get_physical_position();
        ASSERT_TRUE(pos != nullptr);
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
