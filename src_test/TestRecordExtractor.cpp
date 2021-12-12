/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifdef _MSC_VER
#pragma warning(disable:4494)
#endif

#include <limits>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

#include <gtest/gtest.h>

#include <commata/parse_csv.hpp>
#include <commata/record_extractor.hpp>

#include "BaseTest.hpp"
#include "fancy_allocator.hpp"
#include "tracking_allocator.hpp"

using namespace commata;
using namespace commata::test;

struct TestRecordExtractor : BaseTestWithParam<std::size_t>
{};

TEST_P(TestRecordExtractor, LeftmostKey)
{
    const wchar_t* s = L"key_a,key_b,value_a,value_b\n"
                       L"\"ka1\",kb1,va1,vb1\r\n"
                       L"ka2,kb2,va2,vb2\n"
                       L"ka1,kb3,vb3,\"vb3\"\r";
    std::wstringbuf out;
    std::wstring key_a = L"key_a";
    parse_csv(s, make_record_extractor(&out, key_a, L"ka1"), GetParam());
    ASSERT_EQ(L"key_a,key_b,value_a,value_b\n"
              L"\"ka1\",kb1,va1,vb1\n"
              L"ka1,kb3,vb3,\"vb3\"\n",
              std::move(out).str());
}

TEST_P(TestRecordExtractor, InnerKey)
{
    const char* s = "\r\n\n"
                    "key_a,key_b,value_a,value_b\n"
                    "ka1,kb01,va1,vb1\n"
                    "ka2,kb12,va2,vb2\n"
                    "ka1,kb13,\"vb\n3\",vb3";
    std::ostringstream out;
    parse_csv(s, make_record_extractor(out, "key_b",
        [](const char* first ,const char* last) {
            return std::string(first, last).substr(0, 3) == "kb1";
        }), GetParam());
    ASSERT_EQ("key_a,key_b,value_a,value_b\n"
              "ka2,kb12,va2,vb2\n"
              "ka1,kb13,\"vb\n3\",vb3\n",
              std::move(out).str());
}

TEST_P(TestRecordExtractor, NoSuchKey)
{
    const wchar_t* s = L"key_a,key_b,value_a,value_b\n"
                       L"ka1,kb01,va1,vb1\n"
                       L"ka2,kb12,va2,vb2\n";
    std::wstringbuf out;
    try {
        parse_csv(s, make_record_extractor(&out, L"key_c", L"kc1"), GetParam());
        FAIL();
    } catch (const record_extraction_error& e) {
        ASSERT_NE(e.get_physical_position(), nullptr);
        ASSERT_EQ(0U, e.get_physical_position()->first);
        std::string message(e.what());
        ASSERT_TRUE(message.find("key_c") != std::string::npos) << message;
    }
}

TEST_P(TestRecordExtractor, NoSuchField)
{
    const char* s = "key_a,key_b\r"
                    "k1\r"
                    "k0,k1,k2\r";
    std::stringbuf out;
    ASSERT_TRUE(parse_csv(s,
        make_record_extractor(&out, "key_b", "k1"), GetParam()));
    ASSERT_EQ("key_a,key_b\n"
              "k0,k1,k2\n",
              std::move(out).str());
}

TEST_P(TestRecordExtractor, MoveCtor)
{
    // TableHandler requirements do not require that moved-from objects can be
    // used for parsing, but record_extractor provides the ability only if
    // the moved-from objects have not been used parsed yet.
    // This is a test for that behaviour.

    const wchar_t* s = L",key_b,value_a,value_b\n"
                       L"\"ka1\",kb1,va1,vb1\r\n"
                       L",kb2,va2,vb2\n"
                       L"ka1,kb3,vb3,\"vb3\"\r";
        // field name/value preds are likely to be empty strings
        // after moved from, so we contain empty fields in the header and
        // a non-heder record
    std::wstringbuf out;
    std::wstring key_a = L"key_b";
    auto ex = make_record_extractor(&out, key_a, L"kb3");
    auto ey = std::move(ex);

    try {
        parse_csv(s, std::move(ex), GetParam());
        ASSERT_TRUE(out.str().empty());
    } catch (const record_extraction_error&) {
        // "no such field" is all right
    }

    parse_csv(s, std::move(ey), GetParam());
    ASSERT_STREQ(
        L",key_b,value_a,value_b\n"
        L"ka1,kb3,vb3,\"vb3\"\n",
        std::move(out).str().c_str());
}

INSTANTIATE_TEST_SUITE_P(, TestRecordExtractor, testing::Values(1, 10, 1024));

struct TestRecordExtractorLimit : commata::test::BaseTestWithParam<
                                    std::tuple<header_forwarding, std::size_t>>
{};

TEST_P(TestRecordExtractorLimit, Basics)
{
    const auto header = std::get<0>(GetParam());
    const auto max_record_num = std::get<1>(GetParam());
    const char* s = "key_a,key_b,value_a,value_b\n"
                    "ka1,kb1,va1,vb1\r"
                    "ka2,kb2,va2,vb2\n"
                    "ka1,kb3,vb3,vb3\n";
    std::stringbuf out;
    const auto result = parse_csv(s, make_record_extractor(&out,
        "key_a", std::string("ka1"), header, max_record_num), 2);
    ASSERT_EQ(max_record_num > 1, result);
    std::string expected;
    if (header == header_forwarding_yes) {
        expected += "key_a,key_b,value_a,value_b\n";
    }
    expected += "ka1,kb1,va1,vb1\n";
    if (max_record_num > 1) {
        expected += "ka1,kb3,vb3,vb3\n";
    }
    ASSERT_EQ(expected, std::move(out).str());
}

INSTANTIATE_TEST_SUITE_P(, TestRecordExtractorLimit,
    testing::Combine(
        testing::Values(header_forwarding_no, header_forwarding_yes),
        testing::Values(1, std::numeric_limits<std::size_t>::max())));

struct TestRecordExtractorIndexed : BaseTest
{};

TEST_F(TestRecordExtractorIndexed, Basics)
{
    const char* s = "\r\n\n"
                    "key_a,key_b,value_a,value_b\n"
                    "ka1,kb01,va1,vb1\n"
                    "ka2,kb12,va2,vb2\n"
                    "ka1,kb13,\"vb\n3\",vb3";
    std::ostringstream out;
    parse_csv(s, make_record_extractor(out, 1,
        [](const char* first ,const char* last) {
            return std::string(first, last).substr(0, 3) == "kb1";
        }), 1024);
    ASSERT_EQ("key_a,key_b,value_a,value_b\n"
              "ka2,kb12,va2,vb2\n"
              "ka1,kb13,\"vb\n3\",vb3\n",
              std::move(out).str());
}

TEST_F(TestRecordExtractorIndexed, FirstLineIncluded)
{
    const char* s = "assets,1100\n"
                    "lialibities,600\n"
                    "net assets,500\n";
    std::ostringstream out;
    parse_csv(s, make_record_extractor(out, 1,
        [](const char* first, const char* last) {
            const int value = std::stoi(std::string(first, last));
            return value > 500;
        }, 0U));
    ASSERT_EQ("assets,1100\nlialibities,600\n",
              std::move(out).str());
}

TEST_F(TestRecordExtractorIndexed, TooLargeTargetFieldIndex)
{
    std::stringbuf out;
    ASSERT_THROW(
        make_record_extractor(&out, record_extractor_npos, "ABC"),
        std::out_of_range);
}

TEST_F(TestRecordExtractorIndexed, ConstRValueRefString)
{
    std::wstringbuf out;
    auto extractor = [&out] {
        const std::wstring s = L"star";
        return make_record_extractor(
            &out, 0, std::move(s), header_forwarding_no);
    }();
    // If the range of the const rvalue of s has been grabbed in the lambda,
    // extractor will have a dangling range here;
    // below we'd like to assert that it is not the case

    const wchar_t* s = L"category,example\n"
                       L"fish,crucian\n"
                       L"star,alnilam\n"
                       L"vegetable,brassica\n";
    parse_csv(s, std::move(extractor));
    ASSERT_EQ(L"star,alnilam\n", std::move(out).str());
}

namespace {

struct final_predicate_for_value final
{
    bool operator()(const char* first, const char* last) const
    {
        return std::string(first, last).substr(0, 3) == "kb1";
    }
};

}

struct TestRecordExtractorFinalPredicateForValue : BaseTest
{};

TEST_F(TestRecordExtractorFinalPredicateForValue, Basics)
{
    const char* s = "\r\n\n"
                    "key_a,key_b,value_a,value_b\n"
                    "ka1,kb01,va1,vb1\n"
                    "ka2,kb12,va2,vb2\n"
                    "ka1,kb13,\"vb\n3\",vb3";
    std::stringbuf out;
    parse_csv(s,
        make_record_extractor(&out, 1, final_predicate_for_value()), 1024);
    ASSERT_EQ("key_a,key_b,value_a,value_b\n"
              "ka2,kb12,va2,vb2\n"
              "ka1,kb13,\"vb\n3\",vb3\n",
              std::move(out).str());
}

struct TestRecordExtractorMiscellaneous : BaseTest
{};

TEST_F(TestRecordExtractorMiscellaneous, Reference)
{
    const char* s = "instrument,type\n"
                    "viola,string\n"
                    "tuba,brass\n"
                    "clarinet,woodwind\n"
                    "koto,string";
    std::stringbuf out;

    const auto key_pred = [](auto begin, auto end) {
        return ((end - begin) >= 3) && (std::strncmp(begin, "typ", 3) == 0);
    };
    const auto value_pred = [](auto begin, auto end) {
        const std::string value(begin, end);
        return (value == "brass") || (value == "woodwind");
    };

    auto ex = make_record_extractor(
        &out, std::ref(key_pred), std::ref(value_pred));
    parse_csv(s, std::move(ex));
    ASSERT_EQ("instrument,type\n"
              "tuba,brass\n"
              "clarinet,woodwind\n",
              std::move(out).str());
}

TEST_F(TestRecordExtractorMiscellaneous, Allocator)
{
    std::size_t total = 0;
    tracking_allocator<std::allocator<char>> alloc(total);

    // Long names are required to make sure that std::string uses its
    // allocator
    const char* s = "instrument_______,type\n"
                    "castanets________,idiophone\n"
                    "clarinet_________,woodwind\n";
    std::stringbuf out;

    auto ex = make_record_extractor(std::allocator_arg, alloc, &out,
        "instrument_______", std::string("clarinet_________"));
    parse_csv(s, std::move(ex), 8U);
    ASSERT_GT(total, 0U);
}

TEST_F(TestRecordExtractorMiscellaneous, Fancy)
{
    std::size_t total = 0;
    tracking_allocator<fancy_allocator<wchar_t>> alloc(total);

    // ditto
    const wchar_t* s = L"instrument_______,type\n"
                       L"castanets________,idiophone\n"
                       L"clarinet_________,woodwind\n";
    std::wstringbuf out;

     auto ex = make_record_extractor(std::allocator_arg, alloc, &out,
        L"instrument_______", std::wstring(L"clarinet_________"));
    parse_csv(s, std::move(ex), 8U);
    ASSERT_GT(total, 0U);
}

namespace {

template <class RecordExtractor>
class record_extractor_wrapper
{
    RecordExtractor x_;
    bool in_header_field_value_;
    const char* buffer_end_;

public:
    using char_type = char;

    record_extractor_wrapper(RecordExtractor&& x) :
        x_(std::move(x)), in_header_field_value_(false), buffer_end_(nullptr)
    {}

    void start_buffer(const char* buffer_begin, const char* buffer_end)
    {
        x_.start_buffer(buffer_begin, buffer_end);
    }

    void end_buffer(const char* buffer_end)
    {
        x_.end_buffer(buffer_end);
        buffer_end_ = buffer_end;
    }

    void start_record(const char* record_begin)
    {
        x_.start_record(record_begin);
    }

    void end_record(const char* record_end)
    {
        x_.end_record(record_end);
    }

    void update(const char* first, const char* last)
    {
        if (x_.is_in_header() && !in_header_field_value_) {
            insert(first, '[');
            in_header_field_value_ = true;
        }
        x_.update(first, last);
    }

    void finalize(const char* first, const char* last)
    {
        if (x_.is_in_header()) {
            if (!in_header_field_value_) {
                insert(first, '[');
                in_header_field_value_ = false;
            }
            x_.update(first, last);
            insert(last, ']');
            x_.finalize(last, last);
        } else {
            x_.finalize(first, last);
        }
    }

private:
    void insert(const char* before, char c)
    {
        char s[2];
        s[0] = c;
        x_.end_buffer(before);
        x_.start_buffer(s, s + 1);
        x_.update(s, s + 1);
        x_.end_buffer(s + 1);
        x_.start_buffer(before, buffer_end_);
    }
};

}

TEST_F(TestRecordExtractorMiscellaneous, IsInHeader)
{
    const char* s = "instrument,type\n"
                    "castanets,idiophone\n"
                    "clarinet,woodwind\n";
    std::stringbuf out;

    auto x = make_record_extractor(&out, "[instrument]", "clarinet");
    parse_csv(s, record_extractor_wrapper<decltype(x)>(std::move(x)));

    ASSERT_STREQ("[instrument],[type]\n"
                 "clarinet,woodwind\n", std::move(out).str().c_str());
}

TEST_F(TestRecordExtractorMiscellaneous, IsInHeaderIndexed)
{
    const char* s = "instrument,type\n"
                    "castanets,idiophone\n"
                    "clarinet,woodwind\n";
    std::stringbuf out;

    auto x = make_record_extractor(&out, 1U, "woodwind");
    parse_csv(s, record_extractor_wrapper<decltype(x)>(std::move(x)));

    ASSERT_STREQ("[instrument],[type]\n"
                 "clarinet,woodwind\n", std::move(out).str().c_str());
}
