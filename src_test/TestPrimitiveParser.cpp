/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <furfurylic/commata/primitive_parser.hpp>

#include "BaseTest.hpp"

using namespace furfurylic::commata;

namespace {

template <class Ch>
class test_collector
{
    std::vector<std::vector<std::basic_string<Ch>>>* all_field_values_;
    std::vector<std::basic_string<Ch>> current_field_values_;
    std::basic_string<Ch> field_value_;

public:
    explicit test_collector(std::vector<std::vector<std::basic_string<Ch>>>& field_values) :
        all_field_values_(&field_values)
    {
        std::basic_stringstream<Ch> s;
        s << "dummy";
        current_field_values_.push_back(s.str());
    }

    void start_record(const Ch* /*row_begin*/)
    {
        current_field_values_ = std::vector<std::basic_string<Ch>>();
    }

    bool update(const Ch* first, const Ch* last)
    {
        field_value_.append(first, last);
        return true;
    }

    bool finalize(const Ch* first, const Ch* last)
    {
        field_value_.append(first, last);
        current_field_values_.push_back(std::move(field_value_));
        return true;
    }

    void end_buffer(const Ch* /*buffer_end*/)
    {}

    void start_buffer(const Ch* /*buffer_begin*/)
    {}

    bool end_record(const Ch* /*end*/)
    {
        all_field_values_->push_back(std::move(current_field_values_));
        return true;
    }
};

}

struct TestPrimitiveParserBasics :
    furfurylic::test::BaseTestWithParam<std::size_t>
{};

TEST_P(TestPrimitiveParserBasics, Narrow)
{
    std::string s = ",\"col1\", col2 ,col3,\r\n\n"
                    " cell10 ,,\"cell\r\n12\",\"cell\"\"13\"\"\",\"\"\n";
    std::stringbuf buf(s);
    std::vector<std::vector<std::string>> field_values;
    test_collector<char> collector(field_values);
    ASSERT_TRUE(parse(buf, GetParam(), collector));
    ASSERT_EQ(2U, field_values.size());
    std::vector<std::string> expected_row0 = { "", "col1", " col2 ", "col3", "" };
    ASSERT_EQ(expected_row0, field_values[0]);
    std::vector<std::string> expected_row1 = { " cell10 ", "", "cell\r\n12", "cell\"13\"", "" };
    ASSERT_EQ(expected_row1, field_values[1]);
}

TEST_P(TestPrimitiveParserBasics, Wide)
{
    std::wstring s = L"\n\r\rheader1,header2\r\r\n"
                     L"value1,value2\n";
    std::wstringbuf buf(s);
    std::vector<std::vector<std::wstring>> field_values;
    test_collector<wchar_t> collector(field_values);
    ASSERT_TRUE(parse(buf, GetParam(), collector));
    ASSERT_EQ(2U, field_values.size());
    std::vector<std::wstring> expected_row0 = { L"header1", L"header2" };
    ASSERT_EQ(expected_row0, field_values[0]);
    std::vector<std::wstring> expected_row1 = { L"value1", L"value2" };
    ASSERT_EQ(expected_row1, field_values[1]);
}

INSTANTIATE_TEST_CASE_P(, TestPrimitiveParserBasics, testing::Values(1, 10, 1024));

struct TestPrimitiveParserEndsWithoutLF :
    testing::TestWithParam<std::pair<const char*, const char*>>
{};

TEST_P(TestPrimitiveParserEndsWithoutLF, All)
{
    std::stringbuf buf(GetParam().first);
    std::vector<std::vector<std::string>> field_values;
    test_collector<char> collector(field_values);
    ASSERT_TRUE(parse(buf, 1024, collector));
    ASSERT_EQ(1U, field_values.size());
    std::stringstream s;
    std::copy(field_values[0].cbegin(), field_values[0].cend(),
        std::ostream_iterator<std::string>(s, "/"));
    ASSERT_EQ(s.str(), GetParam().second);
}

INSTANTIATE_TEST_CASE_P(, TestPrimitiveParserEndsWithoutLF,
    testing::Values(
        std::make_pair("ColA,ColB,ColC", "ColA/ColB/ColC/"),
        std::make_pair("ColA,ColB,\"ColC\"", "ColA/ColB/ColC/"),
        std::make_pair("ColA,ColB,", "ColA/ColB//")));

struct TestPrimitiveParserErrors :
    testing::TestWithParam<std::pair<const char*, std::pair<std::size_t, std::size_t>>>
{};

TEST_P(TestPrimitiveParserErrors, Errors)
{
    std::string s = GetParam().first;
    std::stringbuf buf(s);
    std::vector<std::vector<std::string>> field_values;
    test_collector<char> collector(field_values);
    try {
        parse(buf, 4, collector);   // shorter than one line
        FAIL();
    } catch (const parse_error& e) {
        if (GetParam().second.first == parse_error::npos) {
            ASSERT_TRUE(e.get_physical_position() == nullptr);
        } else {
            const auto pos = e.get_physical_position();
            ASSERT_TRUE(pos != nullptr);
            ASSERT_EQ(GetParam().second.first, pos->first);
            ASSERT_EQ(GetParam().second.second, pos->second);
        }
    }
}

INSTANTIATE_TEST_CASE_P(, TestPrimitiveParserErrors,
    testing::Values(
        std::make_pair("col\"1\"", std::make_pair(0, 3)),
        std::make_pair("\"col1", std::make_pair(parse_error::npos, parse_error::npos)),
        std::make_pair("\"col1\",\"", std::make_pair(parse_error::npos, parse_error::npos)),
        std::make_pair("col1\r\n\n\"col2\"a", std::make_pair(2, 6))));
