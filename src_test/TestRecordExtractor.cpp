/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <sstream>
#include <tuple>

#include <gtest/gtest.h>

#include <furfurylic/commata/primitive_parser.hpp>
#include <furfurylic/commata/record_extractor.hpp>

#include "BaseTest.hpp"

using namespace furfurylic::commata;

struct TestRecordExtractor :
    furfurylic::test::BaseTestWithParam<std::size_t>
{};

TEST_P(TestRecordExtractor, LeftmostKey)
{
    const char* s = "key_a,key_b,value_a,value_b\n"
                    "\"ka1\",kb1,va1,vb1\r\n"
                    "ka2,kb2,va2,vb2\n"
                    "ka1,kb3,vb3,\"vb3\"\r";
    std::stringbuf in(s);
    std::stringbuf out;
    parse(in, GetParam(), make_record_extractor(out, "key_a", "ka1"));
    ASSERT_EQ("key_a,key_b,value_a,value_b\n"
              "\"ka1\",kb1,va1,vb1\n"
              "ka1,kb3,vb3,\"vb3\"\n",
              out.str());
}

TEST_P(TestRecordExtractor, InnerKey)
{
    const char* s = "\r\n\n"
                    "key_a,key_b,value_a,value_b\n"
                    "ka1,kb01,va1,vb1\n"
                    "ka2,kb12,va2,vb2\n"
                    "ka1,kb13,\"vb\n3\",vb3";
    std::stringbuf in(s);
    std::stringbuf out;
    parse(in, GetParam(), make_record_extractor(out, "key_b",
        [](const char* first ,const char* last) {
            return std::string(first, last).substr(0, 3) == "kb1";
        }));
    ASSERT_EQ("key_a,key_b,value_a,value_b\n"
              "ka2,kb12,va2,vb2\n"
              "ka1,kb13,\"vb\n3\",vb3\n",
              out.str());
}

TEST_P(TestRecordExtractor, NoSuchKey)
{
    const wchar_t* s = L"key_a,key_b,value_a,value_b\n"
                       L"ka1,kb01,va1,vb1\n"
                       L"ka2,kb12,va2,vb2\n";
    std::wstringbuf in(s);
    std::wstringbuf out;
    try {
        parse(in, GetParam(), make_record_extractor(out, L"key_c", L"kc1"));
        FAIL();
    } catch (const record_extraction_error& e) {
        ASSERT_NE(e.get_physical_position(), nullptr);
        ASSERT_EQ(0U, e.get_physical_position()->first);
    }
}

INSTANTIATE_TEST_CASE_P(, TestRecordExtractor, testing::Values(1, 10, 1024));

struct TestRecordExtractorLimit :
    testing::TestWithParam<std::tuple<bool, std::size_t>>
{};

TEST_P(TestRecordExtractorLimit, Basics)
{
    const auto includes_header = std::get<0>(GetParam());
    const auto max_record_num = std::get<1>(GetParam());
    const char* s = "key_a,key_b,value_a,value_b\n"
                    "ka1,kb1,va1,vb1\r"
                    "ka2,kb2,va2,vb2\n"
                    "ka1,kb3,vb3,vb3\n";
    std::stringbuf in(s);
    std::stringbuf out;
    const auto result = parse(in, 2, make_record_extractor(
        out, "key_a", "ka1", includes_header, max_record_num));
    ASSERT_EQ(max_record_num > 1, result);
    std::string expected;
    if (includes_header) {
        expected += "key_a,key_b,value_a,value_b\n";
    }
    expected += "ka1,kb1,va1,vb1\n";
    if (max_record_num > 1) {
        expected += "ka1,kb3,vb3,vb3\n";
    }
    ASSERT_EQ(expected, out.str());
}

INSTANTIATE_TEST_CASE_P(, TestRecordExtractorLimit,
    testing::Combine(
        testing::Bool(),
        testing::Values(1, std::numeric_limits<std::size_t>::max())));
