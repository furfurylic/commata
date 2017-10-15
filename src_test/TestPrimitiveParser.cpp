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

#include <furfurylic/commata/primitive_parser.hpp>

#include "BaseTest.hpp"

using namespace furfurylic::commata;

namespace {

template <class Ch>
class test_collector
{
    std::size_t buffer_size_;
    Ch* buffer_;
    std::vector<std::vector<std::basic_string<Ch>>>* all_field_values_;
    std::vector<std::basic_string<Ch>> current_field_values_;
    std::basic_string<Ch> field_value_;

public:
    test_collector(
        std::size_t buffer_size,
        std::vector<std::vector<std::basic_string<Ch>>>& field_values) :
        buffer_size_(buffer_size),
        buffer_(nullptr), all_field_values_(&field_values)
    {}

    ~test_collector()
    {
        delete [] buffer_;
    }

    void start_record(const Ch* /*row_begin*/)
    {}

    bool update(const Ch* first, const Ch* last)
    {
        field_value_.append(first, last);
        return true;
    }

    bool finalize(const Ch* first, const Ch* last)
    {
        field_value_.append(first, last);
        std::basic_string<Ch> current;
        current.swap(field_value_); // field_value_ is cleared here
        current_field_values_.push_back(std::move(current));
        return true;
    }

    void end_buffer(const Ch* /*buffer_end*/)
    {}

    void start_buffer(const Ch* /*buffer_begin*/)
    {}

    bool end_record(const Ch* /*end*/)
    {
        all_field_values_->push_back(std::move(current_field_values_));
        current_field_values_.clear();
        return true;
    }

    std::pair<Ch*, std::size_t> get_buffer()
    {
        if (!buffer_) {
            buffer_ = new Ch[buffer_size_]; // throw
        }
        return std::make_pair(buffer_, buffer_size_);
    }

    void release_buffer(const Ch* /*buffer*/) noexcept
    {}
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
    test_collector<char> collector(GetParam(), field_values);
    ASSERT_TRUE(parse(buf, collector));
    ASSERT_EQ(2U, field_values.size());
    std::vector<std::string> expected_row0 =
        { "", "col1", " col2 ", "col3", "" };
    ASSERT_EQ(expected_row0, field_values[0]);
    std::vector<std::string> expected_row1 =
        { " cell10 ", "", "cell\r\n12", "cell\"13\"", "" };
    ASSERT_EQ(expected_row1, field_values[1]);
}

TEST_P(TestPrimitiveParserBasics, Wide)
{
    std::wstring s = L"\n\r\rheader1,header2\r\r\n"
                     L"value1,value2\n";
    std::wstringbuf buf(s);
    std::vector<std::vector<std::wstring>> field_values;
    test_collector<wchar_t> collector(GetParam(), field_values);
    ASSERT_TRUE(parse(buf, collector));
    ASSERT_EQ(2U, field_values.size());
    std::vector<std::wstring> expected_row0 = { L"header1", L"header2" };
    ASSERT_EQ(expected_row0, field_values[0]);
    std::vector<std::wstring> expected_row1 = { L"value1", L"value2" };
    ASSERT_EQ(expected_row1, field_values[1]);
}

INSTANTIATE_TEST_CASE_P(,
    TestPrimitiveParserBasics, testing::Values(1, 10, 1024));

struct TestPrimitiveParserEndsWithoutLF :
    testing::TestWithParam<std::pair<const char*, const char*>>
{};

TEST_P(TestPrimitiveParserEndsWithoutLF, All)
{
    std::stringbuf buf(GetParam().first);
    std::vector<std::vector<std::string>> field_values;
    test_collector<char> collector(1024, field_values);
    ASSERT_TRUE(parse(buf, collector));
    ASSERT_EQ(1U, field_values.size());
    std::stringstream s;
    std::copy(field_values[0].cbegin(), field_values[0].cend(),
        std::ostream_iterator<std::string>(s, "/"));
    ASSERT_EQ(GetParam().second, s.str());
}

INSTANTIATE_TEST_CASE_P(, TestPrimitiveParserEndsWithoutLF,
    testing::Values(
        std::make_pair("ColA,ColB,ColC", "ColA/ColB/ColC/"),
        std::make_pair("ColA,ColB,\"ColC\"", "ColA/ColB/ColC/"),
        std::make_pair("ColA,ColB,", "ColA/ColB//")));

struct TestPrimitiveParserErrors :
    testing::TestWithParam<
        std::pair<const char*, std::pair<std::size_t, std::size_t>>>
{};

TEST_P(TestPrimitiveParserErrors, Errors)
{
    std::string s = GetParam().first;
    std::stringbuf buf(s);
    std::vector<std::vector<std::string>> field_values;

    // the buffer is shorter than one line
    test_collector<char> collector(4, field_values);

    try {
        parse(buf, collector);
        FAIL();
    } catch (const parse_error& e) {
        const auto pos = e.get_physical_position();
        ASSERT_TRUE(pos != nullptr);
        ASSERT_EQ(GetParam().second.first, pos->first);
        ASSERT_EQ(GetParam().second.second, pos->second);
    }
}

INSTANTIATE_TEST_CASE_P(, TestPrimitiveParserErrors,
    testing::Values(
        std::make_pair("col\"1\"", std::make_pair(0, 3)),
        std::make_pair("\"col1", std::make_pair(0, 5)),
        std::make_pair("\"col1\",\"", std::make_pair(0, 8)),
        std::make_pair("col1\r\n\n\"col2\"a", std::make_pair(2, 6))));
