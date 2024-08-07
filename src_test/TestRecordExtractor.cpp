/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include <gtest/gtest.h>

#include <commata/parse_csv.hpp>
#include <commata/record_extractor.hpp>

#include "BaseTest.hpp"
#include "fancy_allocator.hpp"
#include "tracking_allocator.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

using namespace commata;
using namespace commata::test;

namespace {

struct null_termination
{};

bool operator==(const char* str, null_termination)
{
    return *str == '\0';
}

struct null_terminated
{
    const char* str;

    const char* begin() const
    {
        return str;
    }

    null_termination end() const
    {
        return null_termination();
    }
};

} // end unnamed

struct TestRecordExtractorStringPred : BaseTest
{};

TEST_F(TestRecordExtractorStringPred, NullTerminatedEq)
{
    const null_terminated abc = { "ABC" };
    const auto eq =
        detail::record_extraction::make_eq<char, std::char_traits<char>>(abc);
    ASSERT_FALSE(eq("ABCD"));
    ASSERT_FALSE(eq("AB"));
    ASSERT_FALSE(eq("ABc"));
    ASSERT_TRUE (eq("ABC"));
}

struct TestRecordExtractor : BaseTestWithParam<std::size_t>
{};

TEST_P(TestRecordExtractor, LeftmostKey)
{
    const wchar_t* s = L"key_a,key_b,value_a,value_b\n"
                       LR"("ka1",kb1,va1,vb1)" L"\r\n"
                       L"ka2,kb2,va2,vb2\n"
                       LR"(ka1,kb3,vb3,"vb3")" L"\r";
    std::wstringbuf out;
    std::wstring key_a = L"key_a";
    parse_csv(s, make_record_extractor(out, key_a, L"ka1"), GetParam());
    ASSERT_EQ(L"key_a,key_b,value_a,value_b\n"
              L"\"ka1\",kb1,va1,vb1\n"
              L"ka1,kb3,vb3,\"vb3\"\n",
              std::move(out).str());
}

TEST_P(TestRecordExtractor, InnerKey)
{
    const char* s = "\r\n"
                    "\n"
                    "key_a,key_b,value_a,value_b\n"
                    "ka1,kb01,va1,vb1\n"
                    "ka2,kb12,va2,vb2\n"
                    R"(ka1,kb13,"vb)" "\n"
                    R"(3",vb3)";
    std::stringbuf out;
    parse_csv(s, make_record_extractor(out, "key_b",
        [](std::string_view s) {
            return s.substr(0, 3) == "kb1";
        }), GetParam());
    ASSERT_EQ(R"(key_a,key_b,value_a,value_b
ka2,kb12,va2,vb2
ka1,kb13,"vb
3",vb3
)",
              std::move(out).str());
}

TEST_P(TestRecordExtractor, NoSuchKey)
{
    const wchar_t* s = L"key_a,key_b,value_a,value_b\n"
                       L"ka1,kb01,va1,vb1\n"
                       L"ka2,kb12,va2,vb2\n";
    std::wstringbuf out;
    try {
        parse_csv(s,
            make_record_extractor(out, L"key_c", L"kc1"), GetParam());
        FAIL();
    } catch (const record_extraction_error& e) {
        ASSERT_TRUE(e.get_physical_position());
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
        make_record_extractor(out, "key_b", "k1"), GetParam()));
    ASSERT_EQ("key_a,key_b\n"
              "k0,k1,k2\n",
              std::move(out).str());
}

TEST_P(TestRecordExtractor, MoveCtor)
{
    // TableHandler requirements do not require that moved-from objects can be
    // used for parsing, but record_extractor provides the ability only if
    // the moved-from objects have not been used for parsing yet.
    // This is a test for that behaviour.

    const wchar_t* s = L",key_b,value_a,value_b\n"
                       LR"("ka1",kb1,va1,vb1)" L"\r\n"
                       L",kb2,va2,vb2\n"
                       LR"(ka1,kb3,vb3,"vb3")" L"\r";
        // field name/value preds are likely to be empty strings
        // after moved from, so we contain empty fields in the header and
        // a non-heder record
    std::wstringbuf out;
    std::wstring key_a = L"key_b";
    auto ex = make_record_extractor(out, key_a, L"kb3");
    auto ey = std::move(ex);

    try {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:26800)  // use of moved-from objects, elaborated
#endif
        parse_csv(s, std::move(ex), GetParam());
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        ASSERT_TRUE(out.str().empty());
    } catch (const record_extraction_error&) {
        // "no such field" is all right
    }

    parse_csv(s, std::move(ey), GetParam());
    ASSERT_STREQ(LR"(,key_b,value_a,value_b
ka1,kb3,vb3,"vb3"
)",
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
    const auto result = parse_csv(s, make_record_extractor(out,
        "key_a", "ka1"s, header, max_record_num), 2);
    ASSERT_EQ(max_record_num > 1, result);
    std::string expected;
    if (header == header_forwarding::yes) {
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
        testing::Values(header_forwarding::no, header_forwarding::yes),
        testing::Values(1, std::numeric_limits<std::size_t>::max())));

struct TestRecordExtractorIndexed : BaseTest
{};

TEST_F(TestRecordExtractorIndexed, Basics)
{
    const char* s = "\r\n"
                    "\n"
                    "key_a,key_b,value_a,value_b\n"
                    "ka1,kb01,va1,vb1\n"
                    "ka2,kb12,va2,vb2\n"
                    R"(ka1,kb13,"vb)" "\n"
                    R"(3",vb3)";
    std::stringbuf out;
    parse_csv(s, make_record_extractor(out, 1,
        [](std::string_view s) {
            return s.substr(0, 3) == "kb1";
        }), 1024);
    ASSERT_EQ(R"(key_a,key_b,value_a,value_b
ka2,kb12,va2,vb2
ka1,kb13,"vb
3",vb3
)",
              std::move(out).str());
}

TEST_F(TestRecordExtractorIndexed, FirstLineIncluded)
{
    const char* s = "assets,1100\n"
                    "lialibities,600\n"
                    "net assets,500\n";
    std::stringbuf out;
    parse_csv(s, make_record_extractor(out, 1,
        [](std::string_view s) {
            const int value = std::stoi(std::string(s));
            return value > 500;
        }, std::nullopt));
    ASSERT_EQ("assets,1100\nlialibities,600\n",
              std::move(out).str());
}

TEST_F(TestRecordExtractorIndexed, TooLargeTargetFieldIndex)
{
    std::stringbuf out;
    ASSERT_THROW(
        { auto x = make_record_extractor(out, record_extractor_npos, "ABC"); },
        std::out_of_range);
}

TEST_F(TestRecordExtractorIndexed, ConstRValueRefString)
{
    std::wstringbuf out;
    auto extractor = [&out] {
        const std::wstring s = L"star";
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:26478)  // move of const objects, elaborated
#endif
        return make_record_extractor(
            out, 0, std::move(s), header_forwarding::no);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
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
    bool operator()(std::string_view s) const
    {
        return s.substr(0, 3) == "kb1";
    }
};

} // end unnamed

struct TestRecordExtractorFinalPredicateForValue : BaseTest
{};

TEST_F(TestRecordExtractorFinalPredicateForValue, Basics)
{
    const char* s = "\r\n"
                    "\n"
                    "key_a,key_b,value_a,value_b\n"
                    "ka1,kb01,va1,vb1\n"
                    "ka2,kb12,va2,vb2\n"
                    R"(ka1,kb13,"vb)" "\n"
                    R"(3",vb3)";
    std::stringbuf out;
    parse_csv(s,
        make_record_extractor(out, 1, final_predicate_for_value()), 1024);
    ASSERT_EQ(R"(key_a,key_b,value_a,value_b
ka2,kb12,va2,vb2
ka1,kb13,"vb
3",vb3
)",
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

    const auto key_pred = [](auto s) {
        return s.substr(0, 3) == "typ";
    };
    const auto value_pred = [](auto s) {
        return (s == "brass") || (s == "woodwind");
    };

    auto ex = make_record_extractor(
        out, std::ref(key_pred), std::ref(value_pred));
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

    std::stringstream s;
    s << "instrument,type\n"
         "castanets,idiophone\n"
         "clarinet,woodwind\n";
    std::stringbuf out;

    auto ex = make_record_extractor(std::allocator_arg, alloc, out,
        "instrument", "clarinet"s);
    parse_csv(s, std::move(ex), 8U);
    ASSERT_GT(total, 0U);
}

TEST_F(TestRecordExtractorMiscellaneous, Fancy)
{
    std::size_t total = 0;
    tracking_allocator<fancy_allocator<wchar_t>> alloc(total);

    // Long names are required to make sure that std::string uses its
    // allocator
    std::wstringstream s;
    s << L"instrument,type\n"
      << L"castanets,idiophone\n"
      << L"clarinet,woodwind\n";
    std::wstringbuf out;

     auto ex = make_record_extractor(std::allocator_arg, alloc, out,
        L"instrument", std::wstring(L"clarinet"));
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

    explicit record_extractor_wrapper(RecordExtractor&& x) :
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

    auto x = make_record_extractor(out, "[instrument]", "clarinet");
    parse_csv(s, record_extractor_wrapper(std::move(x)));

    ASSERT_STREQ("[instrument],[type]\n"
                 "clarinet,woodwind\n", std::move(out).str().c_str());
}

TEST_F(TestRecordExtractorMiscellaneous, IsInHeaderIndexed)
{
    const char* s = "instrument,type\n"
                    "castanets,idiophone\n"
                    "clarinet,woodwind\n";
    std::stringbuf out;

    auto x = make_record_extractor(out, 1U, "woodwind");
    parse_csv(s, record_extractor_wrapper(std::move(x)));

    ASSERT_STREQ("[instrument],[type]\n"
                 "clarinet,woodwind\n", std::move(out).str().c_str());
}

TEST_F(TestRecordExtractorMiscellaneous, DeductionGuide)
{
    const char* s = "instrument,type\n"
                    "castanets,idiophone\n"
                    "clarinet,woodwind\n"
                    "triangle,idiophone\n";

    const auto is_type =
        [](std::string_view s) {
            return s == "type";
        };
    const auto is_woodwind =
        [](std::string_view s) {
            return s == "woodwind";
        };

    {
        std::stringbuf out;
        parse_csv(s, record_extractor(out, is_type, is_woodwind));
        ASSERT_STREQ("instrument,type\n"
                     "clarinet,woodwind\n", std::move(out).str().c_str());
    }

    {
        std::stringbuf out;
        parse_csv(s, record_extractor(out,
            is_type, std::not_fn(is_woodwind), header_forwarding::no, 1));
        ASSERT_STREQ("castanets,idiophone\n", std::move(out).str().c_str());
    }

    {
        std::size_t total = 0;
        tracking_allocator<std::allocator<char>> a(total);
        std::stringbuf out;
        parse_csv(std::stringbuf(s), record_extractor(std::allocator_arg, a,
            out, is_type, is_woodwind), 5);
        ASSERT_STREQ("instrument,type\n"
                     "clarinet,woodwind\n", std::move(out).str().c_str());
        ASSERT_GT(total, 0U);
    }

    {
        std::size_t total = 0;
        tracking_allocator<std::allocator<char>> a(total);
        std::stringbuf out;
        parse_csv(std::stringbuf(s), record_extractor(std::allocator_arg, a,
            out, is_type, std::not_fn(is_woodwind), header_forwarding::no, 1),
            5);
        ASSERT_STREQ("castanets,idiophone\n", std::move(out).str().c_str());
        ASSERT_GT(total, 0U);
    }
}

TEST_F(TestRecordExtractorMiscellaneous, DeductionGuideIndexed)
{
    const char* s = "instrument,type\n"
                    "castanets,idiophone\n"
                    "clarinet,woodwind\n"
                    "triangle,idiophone\n";

    const auto is_woodwind =
        [](std::string_view s) {
            return s == "woodwind";
        };

    {
        std::stringbuf out;
        parse_csv(s, record_extractor_with_indexed_key(
            out, 1, is_woodwind));
        ASSERT_STREQ("instrument,type\n"
                     "clarinet,woodwind\n", std::move(out).str().c_str());
    }

    {
        std::stringbuf out;
        parse_csv(s, record_extractor_with_indexed_key(
            out, 1, std::not_fn(is_woodwind), header_forwarding::no, 1));
        ASSERT_STREQ("castanets,idiophone\n", std::move(out).str().c_str());
    }

    {
        std::size_t total = 0;
        tracking_allocator<std::allocator<char>> a(total);
        std::stringbuf out;
        parse_csv(std::stringbuf(s), record_extractor_with_indexed_key(
            std::allocator_arg, a, out, 1, is_woodwind), 5);
        ASSERT_STREQ("instrument,type\n"
                     "clarinet,woodwind\n", std::move(out).str().c_str());
        ASSERT_GT(total, 0U);
    }

    {
        std::size_t total = 0;
        tracking_allocator<std::allocator<char>> a(total);
        std::stringbuf out;
        parse_csv(std::stringbuf(s), record_extractor_with_indexed_key(
            std::allocator_arg, a,
            out, 1, std::not_fn(is_woodwind), header_forwarding::no, 1), 5);
        ASSERT_STREQ("castanets,idiophone\n", std::move(out).str().c_str());
        ASSERT_GT(total, 0U);
    }
}

TEST_F(TestRecordExtractorMiscellaneous, EvadeCopying)
{
    const wchar_t* s = L"instrument,type\n"
                       L"castanets,idiophone\n"
                       L"clarinet,woodwind\n"
                       L"triangle,idiophone\n";

    std::size_t total = 0;
    tracking_allocator<std::allocator<wchar_t>> a(total);
    std::wstringbuf out;

    // Freakingly, invoking Visual Studio 2019/2022's allocator-taking vector's
    // ctor and the move ctor from it seem to allocate some amount of memory.
    // So we must reset total to 0 before the parsing takes place.
    auto parser = make_csv_source(s)(make_record_extractor(
        std::allocator_arg, a, out, L"type", L"idiophone"sv), 1);
    total = 0;
    parser();

    ASSERT_STREQ(L"instrument,type\n"
                 L"castanets,idiophone\n"
                 L"triangle,idiophone\n", std::move(out).str().c_str());
    ASSERT_EQ(0U, total);
}

TEST_F(TestRecordExtractorMiscellaneous, EvadeCopyingIndexed)
{
    const char* s = "instrument,type\n"
                    "castanets,idiophone\n"
                    "clarinet,woodwind\n"
                    "triangle,idiophone\n";

    std::size_t total = 0;
    tracking_allocator<std::allocator<char>> a(total);
    std::stringbuf out;

    // ditto
    auto parser = make_csv_source(s)(make_record_extractor(
        std::allocator_arg, a, out, 1, "woodwind"sv), 1);
    total = 0;
    parser();

    ASSERT_STREQ("instrument,type\n"
                 "clarinet,woodwind\n", std::move(out).str().c_str());
    ASSERT_EQ(0U, total);
}
