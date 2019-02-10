/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cwchar>
#include <deque>
#include <iomanip>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <commata/empty_physical_line_aware_handler.hpp>
#include <commata/parse_csv.hpp>
#include <commata/text_error.hpp>
#include <commata/table_scanner.hpp>

#include "BaseTest.hpp"
#include "tracking_allocator.hpp"

using namespace commata;
using namespace commata::test;

namespace {

template <class Ch>
struct digits
{
    static const Ch all[];
};

template <>
const char digits<char>::all[] = "0123456789";

template <>
const wchar_t digits<wchar_t>::all[] = L"0123456789";

template <class Ch>
std::basic_string<Ch> plus1(std::basic_string<Ch> s, std::size_t i
    = static_cast<std::size_t>(-1))
{
    const auto j = std::min(i, static_cast<std::size_t>(s.size() - 1));
    const Ch (&all)[sizeof(digits<Ch>::all) / sizeof(digits<Ch>::all[0])]
        = digits<Ch>::all;
    const auto all_end = all + sizeof(all) / sizeof(all[0]) - 1;
    const auto k = std::find(all, all_end, s[j]);
    if (k == all_end - 1) {
        s[j] = all[0];
        if (j == 0) {
            s.insert(s.begin(), all[1]);  // gcc 6.3.1 refuses s.cbegin()
            return s;
        } else {
            return plus1(std::move(s), j - 1);
        }
    } else {
        s[j] = k[1];
        return s;
    }
}

}

typedef testing::Types<char, wchar_t> Chs;

typedef testing::Types<
    std::pair<char, char>,
    std::pair<char, signed char>,
    std::pair<char, unsigned char>,
    std::pair<char, short>,
    std::pair<char, unsigned short>,
    std::pair<char, int>,
    std::pair<char, unsigned>,
    std::pair<char, long>,
    std::pair<char, unsigned long>,
    std::pair<char, long long>,
    std::pair<char, unsigned long long>,
    std::pair<wchar_t, char>,
    std::pair<wchar_t, signed char>,
    std::pair<wchar_t, unsigned char>,
    std::pair<wchar_t, short>,
    std::pair<wchar_t, unsigned short>,
    std::pair<wchar_t, int>,
    std::pair<wchar_t, unsigned>,
    std::pair<wchar_t, long>,
    std::pair<wchar_t, unsigned long>,
    std::pair<wchar_t, long long>,
    std::pair<wchar_t, unsigned long long>
> ChIntegrals;

typedef testing::Types<
    std::pair<char, float>,
    std::pair<char, double>,
    std::pair<char, long double>,
    std::pair<wchar_t, float>,
    std::pair<wchar_t, double>,
    std::pair<wchar_t, long double>
> ChFloatingPoints;

template <class ChNum>
struct TestFieldTranslatorForIntegralTypes : BaseTest
{};

TYPED_TEST_CASE(TestFieldTranslatorForIntegralTypes, ChIntegrals);

TYPED_TEST(TestFieldTranslatorForIntegralTypes, Correct)
{
    using char_t = typename TypeParam::first_type;
    using value_t = typename TypeParam::second_type;

    const auto str = char_helper<char_t>::str;

    std::vector<value_t> values;

    table_scanner<char_t> h;
    h.set_field_scanner(0, make_field_translator(values));

    std::basic_stringbuf<char_t> buf(str(" 40\r\n63\t\n-10"));
    ASSERT_NO_THROW(parse_csv(&buf, std::move(h)));
    ASSERT_EQ(3U, values.size());
    ASSERT_EQ(static_cast<value_t>(40), values[0]);
    ASSERT_EQ(static_cast<value_t>(63), values[1]);
    ASSERT_EQ(static_cast<value_t>(-10), values[2]);
}

TYPED_TEST(TestFieldTranslatorForIntegralTypes, UpperLimit)
{
    using char_t = typename TypeParam::first_type;
    using value_t = typename TypeParam::second_type;
    using string_t = std::basic_string<char_t>;
    using stringstream_t = std::basic_stringstream<char_t>;

    const auto to_string =
        [](auto t) { return char_helper<char_t>::template to_string(t); };
    const auto widen = char_helper<char_t>::template widen<const char*>;

    const auto maxx = std::numeric_limits<value_t>::max();
    const auto maxxPlus1 = plus1(to_string(maxx + 0));

    std::vector<value_t> values;

    table_scanner<char_t> h;
    h.set_field_scanner(0, make_field_translator(values));

    stringstream_t s;
    s << (maxx + 0) << "\r\n"
      << maxxPlus1;

    try {
        parse_csv(s, std::move(h));
        FAIL();
    } catch (const field_out_of_range& e) {
        ASSERT_NE(e.get_physical_position(), nullptr);
        ASSERT_EQ(1U, e.get_physical_position()->first);
        const string_t message = widen(e.what());
        ASSERT_TRUE(message.find(maxxPlus1) != string_t::npos)
            << message;
    }
}

TYPED_TEST(TestFieldTranslatorForIntegralTypes, LowerLimit)
{
    using char_t = typename TypeParam::first_type;
    using value_t = typename TypeParam::second_type;
    using string_t = std::basic_string<char_t>;
    using stringstream_t = std::basic_stringstream<char_t>;

    const auto ch = char_helper<char_t>::ch;
    const auto to_string =
        [](auto t) { return char_helper<char_t>::template to_string(t); };
    const auto widen = char_helper<char_t>::template widen<const char*>;

    string_t minn;
    string_t minnMinus1;
    if (std::is_signed<value_t>::value) {
        minn = to_string(std::numeric_limits<value_t>::min() + 0);
        minnMinus1 = ch('-') + plus1(minn.substr(1));
    } else {
        minn = ch('-') + to_string(std::numeric_limits<value_t>::max() + 0);
        minnMinus1 = ch('-') + plus1(plus1(minn.substr(1)));
    }
    const auto maxx = std::numeric_limits<value_t>::max();
    const auto maxxPlus1 = plus1(to_string(maxx + 0));

    std::vector<value_t> values;

    table_scanner<char_t> h;
    h.set_field_scanner(0, make_field_translator(values));

    stringstream_t s;
    s << minn << "\r\n"
      << minnMinus1;

    try {
        parse_csv(s, std::move(h));
        FAIL() << values[0] << ',' << values[1];
    } catch (const field_out_of_range& e) {
        ASSERT_NE(e.get_physical_position(), nullptr);
        ASSERT_EQ(1U, e.get_physical_position()->first);
        const string_t message = widen(e.what());
        ASSERT_TRUE(message.find(minnMinus1) != string_t::npos)
            << message;
    }
}

TYPED_TEST(TestFieldTranslatorForIntegralTypes, Replacement)
{
    using char_t = typename TypeParam::first_type;
    using value_t = typename TypeParam::second_type;
    using string_t = std::basic_string<char_t>;
    using stringstream_t = std::basic_stringstream<char_t>;

    const auto ch = char_helper<char_t>::ch;
    const auto to_string =
        [](auto t) { return char_helper<char_t>::template to_string(t); };

    string_t minn;
    string_t minnMinus1;
    if (std::is_signed<value_t>::value) {
        minn = to_string(std::numeric_limits<value_t>::min() + 0);
        minnMinus1 = ch('-') + plus1(minn.substr(1));
    } else {
        minn = ch('-') + to_string(std::numeric_limits<value_t>::max() + 0);
        minnMinus1 = ch('-') + plus1(plus1(minn.substr(1)));
    }
    const auto maxx = std::numeric_limits<value_t>::max();
    const auto maxxPlus1 = plus1(to_string(maxx + 0));

    std::vector<value_t> values0;
    std::vector<value_t> values1;
    std::vector<value_t> values2;
    
    table_scanner<char_t> h;
    h.set_field_scanner(0, make_field_translator(values0,
        fail_if_skipped(),
        replace_if_conversion_failed<value_t>(
            static_cast<value_t>(34))));
    h.set_field_scanner(1, make_field_translator(values1,
        fail_if_skipped(),
        replace_if_conversion_failed<value_t>(
            replacement_fail, static_cast<value_t>(42))));
    h.set_field_scanner(2, make_field_translator(values2,
        fail_if_skipped(),
        replace_if_conversion_failed<value_t>(
            replacement_fail, replacement_fail, static_cast<value_t>(1), static_cast<value_t>(0))));

    stringstream_t s;
    s << "-5,x," << maxxPlus1 << '\n'
      << ",3," << minnMinus1;

    try {
        parse_csv(s, std::move(h));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }
    ASSERT_EQ(2U, values0.size());
    ASSERT_EQ(2U, values1.size());
    ASSERT_EQ(2U, values2.size());
    ASSERT_EQ(static_cast<value_t>(-5), values0[0]);
    ASSERT_EQ(static_cast<value_t>(34), values0[1]);
    ASSERT_EQ(static_cast<value_t>(42), values1[0]);
    ASSERT_EQ(static_cast<value_t>(3), values1[1]);
    ASSERT_EQ(static_cast<value_t>(1), values2[0]);
    if (std::is_signed<value_t>::value) {
        ASSERT_EQ(static_cast<value_t>(0), values2[1]);
    } else {
        ASSERT_EQ(static_cast<value_t>(1), values2[1]);
    }
}

struct TestFieldTranslatorForChar : BaseTest
{};

TEST_F(TestFieldTranslatorForChar, Correct)
{
    std::vector<int_least8_t> values0;      // maybe signed char
    std::vector<uint_least8_t> values1;     // maybe unsigned char
    std::deque<char> values2;

    table_scanner<char> h;
    h.set_field_scanner(0, make_field_translator(values0));
    h.set_field_scanner(1, make_field_translator(values1));
    h.set_field_scanner(2, make_field_translator(values2));

    std::basic_stringbuf<char> buf("-120,250,-5");
    try {
        parse_csv(&buf, std::move(h));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }
    ASSERT_EQ(1U, values0.size());
    ASSERT_EQ(1U, values1.size());
    ASSERT_EQ(1U, values2.size());
    ASSERT_EQ(-120, values0.front());
    ASSERT_EQ(250U, values1.front());
    ASSERT_EQ(static_cast<char>(-5), values2.front());
}

template <class ChNum>
struct TestFieldTranslatorForFloatingPointTypes : BaseTest
{};

TYPED_TEST_CASE(TestFieldTranslatorForFloatingPointTypes, ChFloatingPoints);

TYPED_TEST(TestFieldTranslatorForFloatingPointTypes, Correct)
{
    using char_t = typename TypeParam::first_type;
    using value_t = typename TypeParam::second_type;

    const auto str = char_helper<char_t>::str;

    std::vector<value_t> values;

    table_scanner<char_t> h;
    h.set_field_scanner(0, make_field_translator(values));

    std::basic_string<char_t> s = str("6.02e23\t\r -5\n");
    std::basic_stringbuf<char_t> buf(s);
    try {
        parse_csv(&buf, std::move(h));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(2U, values.size());

    const std::vector<std::basic_string<char_t>> expressions =
        { str("6.02e23"), str("-5") };
    for (decltype(values.size()) i = 0; i < 2U; ++i) {
        std::basic_stringstream<char_t> ss;
        ss << expressions[i];
        value_t value;
        ss >> value;
        ASSERT_EQ(value, values[i]);
    }
}

TYPED_TEST(TestFieldTranslatorForFloatingPointTypes, UpperLimit)
{
    using char_t = typename TypeParam::first_type;
    using value_t = typename TypeParam::second_type;
    using string_t = std::basic_string<char_t>;
    using stringstream_t = std::basic_stringstream<char_t>;

    const auto widen = char_helper<char_t>::template widen<const char*>;

    const auto maxx = std::numeric_limits<value_t>::max();
    string_t maxxBy10;
    {
        stringstream_t ss;
        ss << std::scientific << std::setprecision(50) << maxx << '0';
        maxxBy10 = ss.str();
    }

    std::vector<value_t> values;

    table_scanner<char_t> h;
    h.set_field_scanner(0, make_field_translator(values));

    stringstream_t s;
    s << std::scientific << std::setprecision(50) << maxx << "\n"
      << maxxBy10;

    try {
        parse_csv(s, std::move(h));
        FAIL() << values[1];
    } catch (const text_error& e) {
        ASSERT_NE(e.get_physical_position(), nullptr);
        ASSERT_EQ(1U, e.get_physical_position()->first);
        const string_t message = widen(e.what());
        ASSERT_TRUE(message.find(maxxBy10) != string_t::npos)
            << message;
    }
}

TYPED_TEST(TestFieldTranslatorForFloatingPointTypes, LowerLimit)
{
    using char_t = typename TypeParam::first_type;
    using value_t = typename TypeParam::second_type;
    using string_t = std::basic_string<char_t>;
    using stringstream_t = std::basic_stringstream<char_t>;

    const auto widen = char_helper<char_t>::template widen<const char*>;

    const auto minn = std::numeric_limits<value_t>::lowest();
    string_t minnBy10;
    {
        stringstream_t ss;
        ss << std::scientific << std::setprecision(50) << minn << '0';
        minnBy10 = ss.str();
    }

    std::vector<value_t> values;

    table_scanner<char_t> h;
    h.set_field_scanner(0, make_field_translator(values));

    stringstream_t s;
    s << std::scientific << std::setprecision(50) << minn << "\n"
      << minnBy10;

    try {
        parse_csv(s, std::move(h));
        FAIL() << values[1];
    } catch (const text_error& e) {
        ASSERT_NE(e.get_physical_position(), nullptr);
        ASSERT_EQ(1U, e.get_physical_position()->first);
        const string_t message = widen(e.what());
        ASSERT_TRUE(message.find(minnBy10) != string_t::npos)
            << message;
    }
}

template <class Ch>
struct TestFieldTranslatorForStringTypes : BaseTest
{};

TYPED_TEST_CASE(TestFieldTranslatorForStringTypes, Chs);

TYPED_TEST(TestFieldTranslatorForStringTypes, Correct)
{
    const auto str = char_helper<TypeParam>::str;

    std::deque<std::basic_string<TypeParam>> values;

    table_scanner<TypeParam> h;
    h.set_field_scanner(0, make_field_translator(values));

    std::basic_string<TypeParam> s = str("ABC  \n\"xy\rz\"\n\"\"");
    std::basic_stringbuf<TypeParam> buf(s);

    try {
        parse_csv(&buf, std::move(h));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(3U, values.size());
    ASSERT_EQ(str("ABC  "), values[0]);
    ASSERT_EQ(str("xy\rz"), values[1]);
    ASSERT_TRUE(values[2].empty()) << values[2];
}

static_assert(
    std::uses_allocator<table_scanner<char>, std::allocator<char>>::value, "");

template <class Ch>
struct TestTableScanner : BaseTest
{};

TYPED_TEST_CASE(TestTableScanner, Chs);

TYPED_TEST(TestTableScanner, Indexed)
{
    using string_t = std::basic_string<TypeParam>;

    const auto ch = char_helper<TypeParam>::ch;
    const auto str = char_helper<TypeParam>::str;

    std::deque<long> values0;
    std::vector<double> values21;
    std::deque<double> values22;
    std::list<string_t> values3;
    std::set<unsigned short> values4;
    int values5[2];
    string_t values6;

    table_scanner<TypeParam> h(1U);
    h.set_field_scanner(0,
        make_field_translator<long>(std::front_inserter(values0)));
    h.set_field_scanner(2, make_field_translator(values22));
    h.set_field_scanner(2);
    h.set_field_scanner(2, make_field_translator(values22));  // overridden
    h.set_field_scanner(2, make_field_translator(values21));
    h.set_field_scanner(5, nullptr);
    h.set_field_scanner(4, make_field_translator(values4));
    h.set_field_scanner(3, make_field_translator(values3));
    h.set_field_scanner(5, make_field_translator<int>(values5));
    h.set_field_scanner(6, make_field_translator<string_t>(
        [&values6, ch](string_t&& s){
            values6 += ch('[');
            values6 += std::move(s);
            values6 += ch(']');
        }));

    ASSERT_EQ(
        typeid(make_field_translator(values21)),
        h.get_field_scanner_type(2));
    ASSERT_EQ(typeid(void), h.get_field_scanner_type(1));
    ASSERT_EQ(typeid(void), h.get_field_scanner_type(100));

    ASSERT_NE(nullptr,
        h.template get_field_scanner<
            decltype(make_field_translator(values3))>(3));
    ASSERT_EQ(nullptr,
        h.template get_field_scanner<
        decltype(make_field_translator(values4))>(3));
    ASSERT_EQ(nullptr, h.template get_field_scanner<void>(1));
    ASSERT_EQ(nullptr, h.template get_field_scanner<void>(100));

    std::basic_stringstream<TypeParam> s;
    s << "F0,F1,F2,F3,F4,F5,F6\r"
         "50,__, 101.2 ,XYZ,  200,1,fixa\n"
         "-3,__,3.00e9,\"\"\"ab\"\"\rc\",200,2,tive\n";
    try {
        parse_csv(s, std::move(h));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    const std::deque<long> expected0 = { -3, 50 };
    const std::vector<double> expected21 = { 101.2, 3.00e9 };
    const std::list<string_t> expected3 = { str("XYZ"), str("\"ab\"\rc") };
    const std::set<unsigned short> expected4 = { 200 };
    ASSERT_EQ(expected0, values0);
    ASSERT_EQ(expected21, values21);
    ASSERT_TRUE(values22.empty());
    ASSERT_EQ(expected3, values3);
    ASSERT_EQ(expected4, values4);
    ASSERT_EQ(1, values5[0]);
    ASSERT_EQ(2, values5[1]);
    ASSERT_EQ(str("[fixa][tive]"), values6);
}

TYPED_TEST(TestTableScanner, RecordEndScanner)
{
    const auto str = char_helper<TypeParam>::str;

    std::vector<std::basic_string<TypeParam>> v;

    table_scanner<TypeParam> h(1U);
    h.set_field_scanner(0, make_field_translator(v));

    ASSERT_FALSE(h.has_record_end_scanner());
    auto f = [&v, str] { v.push_back(str("*")); };
    h.set_record_end_scanner(f);
    ASSERT_TRUE(h.has_record_end_scanner());
    ASSERT_EQ(typeid(f), h.get_record_end_scanner_type());
    ASSERT_NE(nullptr, h.template get_record_end_scanner<decltype(f)>());
    ASSERT_EQ(nullptr, h.template get_record_end_scanner<int>());

    try {
        std::basic_stringstream<TypeParam> s;
        s << "Word\r"
             "\"aban\ndon\"\n"
             "Abbott";  // elaborately does not end with CR/LF
        parse_csv(s, std::move(h));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    std::vector<std::basic_string<TypeParam>> expected;
    expected.push_back(str("aban\ndon"));
    expected.push_back(str("*"));
    expected.push_back(str("Abbott"));
    expected.push_back(str("*"));
    ASSERT_EQ(expected, v);
}

TYPED_TEST(TestTableScanner, MultilinedHeader)
{
    std::deque<long> values;

    table_scanner<TypeParam> h(3U);
    h.set_field_scanner(0, make_field_translator(values));

    std::basic_stringstream<TypeParam> s;
    s << "H1\r"
         "H2\n"
         "H3\n"
         "12345";
    try {
        parse_csv(s, std::move(h));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    const std::deque<long> expected = { 12345 };
    ASSERT_EQ(expected, values);
}

TYPED_TEST(TestTableScanner, SkippedWithNoErrors)
{
    using string_t = std::basic_string<TypeParam>;

    const auto str = char_helper<TypeParam>::str;

    std::deque<string_t> values0;
    std::deque<int> values1;

    table_scanner<TypeParam> h;
    h.set_field_scanner(0, make_field_translator(
        values0, default_if_skipped<string_t>()));
    h.set_field_scanner(1, make_field_translator(
        values1, default_if_skipped<int>(50)));

    const auto scanner1 = h.template
        get_field_scanner<decltype(make_field_translator(
            values1, default_if_skipped<int>()))>(1U);
    ASSERT_NE(scanner1, nullptr);
    ASSERT_EQ(50, scanner1->get_skipping_handler()());

    std::basic_stringstream<TypeParam> s;
    s << "XYZ,20\n"
         "\n"
         "A";
    try {
        parse_csv(s, make_empty_physical_line_aware(std::move(h)));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    const std::deque<string_t> expected0 =
        { str("XYZ"), string_t(), str("A") };
    const std::deque<int> expected1 = { 20, 50, 50 };
    ASSERT_EQ(expected0, values0);
    ASSERT_EQ(expected1, values1);
}

TYPED_TEST(TestTableScanner, SkippedWithErrors)
{
    std::deque<int> values0;
    std::deque<int> values1;

    table_scanner<TypeParam> h;
    h.set_field_scanner(0, make_field_translator(
        values0, default_if_skipped<int>(10)));
    h.set_field_scanner(1, make_field_translator(values1));

    std::basic_stringstream<TypeParam> s;
    s << "10,20\n"
        "-5";
    try {
        parse_csv(s, std::move(h));
        FAIL();
    } catch (const field_not_found& e) {
        ASSERT_NE(e.get_physical_position(), nullptr);
        ASSERT_EQ(1U, e.get_physical_position()->first);
    }

    const std::deque<int> expected0 = { 10, -5 };
    const std::deque<int> expected1 = { 20 };
    ASSERT_EQ(expected0, values0);
    ASSERT_EQ(expected1, values1);
}

TYPED_TEST(TestTableScanner, HeaderScan)
{
    const auto str = char_helper<TypeParam>::str;

    std::vector<unsigned> ids;
    std::vector<short> values1;

    table_scanner<TypeParam> h(
        [&ids, &values1, str, this]
        (std::size_t j, const auto* range, auto& f) {
            const std::basic_string<TypeParam>
                field_name(range->first, range->second);
            if (field_name == str("ID")) {
                f.set_field_scanner(j, make_field_translator(ids));
                return true;
            } else if (field_name == str("Value1")) {
                f.set_field_scanner(j, make_field_translator(values1));
                return false;
            } else {
                return true;
            }
        });

    auto s = str("ID,Value0,Value1,Value1\n"
                 "1,ABC,123,xyz\n");
    std::basic_stringbuf<TypeParam> buf(s);
    try {
        parse_csv(&buf, std::move(h));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(1U, ids.size());
    ASSERT_EQ(1U, values1.size());

    ASSERT_EQ(1U, ids[0]);
    ASSERT_EQ(123, values1[0]);
}

TYPED_TEST(TestTableScanner, HeaderScanToTheEnd)
{
    const auto str = char_helper<TypeParam>::str;

    table_scanner<TypeParam> h(
        [](std::size_t j, const auto* range, auto&) {
            if (j == 1) {
                if (range) {
                    throw std::runtime_error("Header's end with a range");
                }
                return false;   // cease scanning
            } else if (!range) {
                throw std::runtime_error("Not a header's end without a range");
            } else {
                return true;    // scan more
            }
    });

    auto s = str("A\n1\n");
    std::basic_stringbuf<TypeParam> buf(s);
    ASSERT_NO_THROW(parse_csv(&buf, std::move(h)));
}

namespace {

template <class Ch>
class two_lined_fx_header_scanner
{
    std::map<std::basic_string<Ch>, std::vector<double>>* fx_;

    std::size_t i_ = 0;
    std::map<std::size_t, std::basic_string<Ch>> first_ccys_;

public:
    explicit two_lined_fx_header_scanner(
        std::map<std::basic_string<Ch>, std::vector<double>>& fx) :
        fx_(&fx)
    {}

    bool operator()(std::size_t j,
        const std::pair<Ch*, Ch*>* range, table_scanner<Ch>& s)
    {
        if (i_ == 0) {
            if (range) {
                first_ccys_[j].assign(range->first, range->second);
            } else {
                ++i_;
            }
        } else {
            if (range) {
                std::basic_string<Ch> name(std::move(first_ccys_[j]));
                name.append(range->first, range->second);
                s.set_field_scanner(j,
                    make_field_translator((*fx_)[std::move(name)]));
            } else {
                return false;
            }
        }
        return true;
    }
};

}

TYPED_TEST(TestTableScanner, MultilinedHeaderScan)
{
    const auto str = char_helper<TypeParam>::str;

    std::map<std::basic_string<TypeParam>, std::vector<double>> fx;
    two_lined_fx_header_scanner<TypeParam> h(fx);
    table_scanner<TypeParam> scanner(std::move(h));
    auto s = str("AUD,AUD,EUR\r"
                 "JPY,USD,USD\r"
                 "80.0,0.9,1.3\r"
                 "82.1,0.91,1.35");
    std::basic_stringbuf<TypeParam> buf(s);
    parse_csv(&buf, std::move(scanner));

    std::vector<double> aud_jpy = { 80.0, 82.1 };
    ASSERT_EQ(aud_jpy, fx[str("AUDJPY")]);

    std::vector<double> aud_usd = { 0.9, 0.91 };
    ASSERT_EQ(aud_usd, fx[str("AUDUSD")]);

    std::vector<double> eur_usd = { 1.3, 1.35 };
    ASSERT_EQ(eur_usd, fx[str("EURUSD")]);
}

namespace {

template <class Ch>
struct french_style_numpunct : std::numpunct<Ch>
{
    explicit french_style_numpunct(std::size_t r = 0) :
        std::numpunct<Ch>(r)
    {}

protected:
    Ch do_decimal_point() const override
    {
        return char_helper<Ch>::ch(',');
    }

    Ch do_thousands_sep() const override
    {
        return char_helper<Ch>::ch(' ');
    }

    std::string do_grouping() const override
    {
        return "\003";
    }
};

}

TYPED_TEST(TestTableScanner, LocaleBased)
{
    const auto str = char_helper<TypeParam>::str;

    std::vector<int> values0;
    std::deque<double> values1;

    table_scanner<TypeParam> h;
    std::locale loc(std::locale::classic(),
        new french_style_numpunct<TypeParam>);
    h.set_field_scanner(0, make_field_translator(values0, loc));
    h.set_field_scanner(1, make_field_translator<double>(
        std::front_inserter(values1), loc));

    std::basic_stringbuf<TypeParam> buf(str("100 000,\"12 345 678,5\""));

    try {
        parse_csv(&buf, std::move(h));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }
    ASSERT_EQ(100000, values0[0]);
    ASSERT_EQ(12345678.5, values1[0]);
}

TYPED_TEST(TestTableScanner, BufferSize)
{
    using string_t = std::basic_string<TypeParam>;

    const auto str = char_helper<TypeParam>::str;

    std::vector<string_t> values0;
    std::vector<int> values1;

    for (std::size_t buffer_size : { 2U, 3U, 4U, 7U  }) {
        table_scanner<TypeParam> h(0U, buffer_size);
        h.set_field_scanner(0, make_field_translator(values0));
        h.set_field_scanner(1, make_field_translator(values1));

        std::basic_stringbuf<TypeParam> buf;
        const auto line = str("ABC,123\n");
        for (std::size_t i = 0; i < 50; ++i) {
            buf.sputn(line.data(), line.size());
        }

        try {
            parse_csv(&buf, std::move(h));
        } catch (const text_error& e) {
            FAIL() << e.info() << "\nbuffer_size=" << buffer_size;
        }

        ASSERT_EQ(50U, values0.size());
        ASSERT_EQ(50U, values1.size());
        for (std::size_t i = 0; i < 50; ++i) {
            ASSERT_EQ(str("ABC"), values0[i])
                << "buffer_size=" << buffer_size << " i=" << i;
            ASSERT_EQ(123, values1[i])
                << "buffer_size=" << buffer_size << " i=" << i;
        }
        values0.clear();
        values1.clear();
    }
}

TYPED_TEST(TestTableScanner, Allocators)
{
    using Alloc = tracking_allocator<std::allocator<TypeParam>>;
    using Tr = std::char_traits<TypeParam>;
    using String = std::basic_string<TypeParam, Tr, Alloc>;

    const auto str = char_helper<TypeParam>::str;

    std::vector<std::pair<char*, char*>> allocated0;
    std::size_t total0 = 0U;
    Alloc a0(allocated0, total0);

    std::vector<std::pair<char*, char*>> allocated2;
    std::size_t total2 = 0U;
    Alloc a2(allocated2, total2);

    table_scanner<TypeParam, Tr, Alloc> scanner(
        std::allocator_arg, a0, 0U, 20U);   // make underflow happen

    // The same allocator as the scanner
    std::vector<String> v0;
    auto f0 = make_field_translator(
        std::allocator_arg, scanner.get_allocator(), v0);
    scanner.set_field_scanner(0, std::move(f0));

    // A different allocator from the scanner
    std::vector<std::basic_string<TypeParam>> v1;
    auto f1 = make_field_translator(v1);
    scanner.set_field_scanner(1, std::move(f1));

    // A different allocator from the scanner, but types are same
    std::vector<String> v2;
    auto f2 = make_field_translator(std::allocator_arg, a2, v2);
    scanner.set_field_scanner(2, std::move(f2));

    // Field scanners are stored into the memories allocated by a0
    ASSERT_TRUE(a0.tracks(
        scanner.template get_field_scanner<decltype(f0)>(0)));

    // A lengthy field is required to make sure String uses an allocator
    try {
        std::basic_stringbuf<TypeParam>
            buf(str("ABCDEFGHIJKLMNOPQRSTUVWXYZ,"
                    "abcdefghijklmnopqrstuvwxyz,"
                    "12345678901234567890123456"));
        parse_csv(&buf, std::move(scanner));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_GT(a0.total(), (80U + 26U) * sizeof(TypeParam));
        // at least allocated all buffers and v0[0]

    ASSERT_EQ(a0, v0[0].get_allocator());
    ASSERT_TRUE(a0.tracks(&v0[0][0]));

    ASSERT_FALSE(a0.tracks(&v1[0][0]));
    ASSERT_FALSE(a0.tracks(&v2[0][0]));

    ASSERT_EQ(a2, v2[0].get_allocator());
    ASSERT_TRUE(a2.tracks(&v2[0][0]));
}

TYPED_TEST(TestTableScanner, MovedFromState)
{
    std::vector<int> values;

    table_scanner<TypeParam> h1;
    auto t = make_field_translator(values);
    h1.set_field_scanner(0, std::move(t));

    table_scanner<TypeParam> h2(std::move(h1));

    ASSERT_FALSE(h1.has_field_scanner(0));
    ASSERT_EQ(typeid(void), h1.get_field_scanner_type(0));
    ASSERT_EQ(nullptr, h1.template get_field_scanner<decltype(t)>(0));

    ASSERT_NE(nullptr, h2.template get_field_scanner<decltype(t)>(0));
}

namespace {

struct stateful_header_scanner
{
    std::size_t index = 0;
    std::vector<int>* values = nullptr;

    template <class Ch, class T>
    bool operator()(std::size_t j, const std::pair<Ch*, Ch*>*, T& t) const
    {
        if (j == index) {
            t.set_field_scanner(j, make_field_translator(*values));
            return false;
        } else {
            return true;
        }
    }
};

}

struct TestTableScannerReference : BaseTest
{};

TEST_F(TestTableScannerReference, HeaderScanner)
{
    std::vector<int> values;
    stateful_header_scanner header_scanner;
    header_scanner.values = &values;
    table_scanner<char> scanner(std::ref(header_scanner));

    header_scanner.index = 1;

    try {
        std::stringbuf buf("A,B\n100,200");
        parse_csv(&buf, std::move(scanner));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(1U, values.size());
    ASSERT_EQ(200, values[0]);
}

TEST_F(TestTableScannerReference, FieldScanner)
{
    std::vector<int> values1;
    auto field_scanner = make_field_translator(values1);
    table_scanner<char> scanner;
    scanner.set_field_scanner(0, std::ref(field_scanner));

    ASSERT_EQ(typeid(std::reference_wrapper<decltype(field_scanner)>),
        scanner.get_field_scanner_type(0));

    try {
        std::stringbuf buf("100");
        parse_csv(&buf, std::move(scanner));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(1U, values1.size());
    ASSERT_EQ(100, values1[0]);
}

TEST_F(TestTableScannerReference, RecordEndScanner)
{
    std::vector<int> v;

    table_scanner<char> scanner;
    scanner.set_field_scanner(0, make_field_translator(v));

    std::function<void()> record_end_scanner = [] {};
    scanner.set_record_end_scanner(std::ref(record_end_scanner));

    record_end_scanner = [&v]() {
        v.push_back(-12345);
    };

    ASSERT_EQ(typeid(std::reference_wrapper<decltype(record_end_scanner)>),
        scanner.get_record_end_scanner_type());

    try {
        std::stringbuf buf("100\n200");
        parse_csv(&buf, std::move(scanner));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(4U, v.size());
    ASSERT_EQ(   100, v[0]);
    ASSERT_EQ(-12345, v[1]);
    ASSERT_EQ(   200, v[2]);
    ASSERT_EQ(-12345, v[3]);
}

struct TestReplacement : BaseTest
{};

TEST_F(TestReplacement, Basics)
{
    replacement<std::string> r(std::string("ABCDEFG"));
    ASSERT_NE(r.get(), nullptr);
    ASSERT_STREQ("ABCDEFG", r.get()->c_str());
    ASSERT_STREQ(r.get()->c_str(), r->c_str());
    ASSERT_EQ(std::string("ABCDEFG"), *r);

    const auto r2(std::move(r));
    ASSERT_NE(r2.get(), nullptr);
    ASSERT_STREQ("ABCDEFG", r2.get()->c_str());
    ASSERT_STREQ(r2.get()->c_str(), r2->c_str());
    ASSERT_EQ(std::string("ABCDEFG"), *r2);
}

TEST_F(TestReplacement, None)
{
    replacement<std::string> r;
    ASSERT_EQ(nullptr, r.get());
}

struct TestReplaceIfConversionFailed : BaseTest
{};

TEST_F(TestReplaceIfConversionFailed, NonArithmeticNoThrowMoveConstructible)
{
    using r_t = replace_if_conversion_failed<std::string>;
    static_assert(std::is_nothrow_move_constructible<r_t>::value, "");

    r_t r("E", replacement_fail, "U", replacement_fail, "X");
    std::vector<r_t> rs;
    rs.push_back(r);
    rs.push_back(std::move(r));
    for (const auto& r2 : rs) {
        const char d[] = "dummy";
        const auto de = d + sizeof d - 1;
        ASSERT_STREQ("E", r2.empty()->c_str());
        ASSERT_THROW(r2.invalid_format(d, de), field_invalid_format);
        ASSERT_STREQ("U", r2.out_of_range(d, de, 1)->c_str());
        ASSERT_THROW(r2.out_of_range(d, de, -1), field_out_of_range);
        ASSERT_STREQ("X", r2.out_of_range(d, de, 0)->c_str());
    }
}

namespace {

class char_holder
{
    char* n_;

public:
    explicit char_holder(char n) : n_(new char(n))
    {}

    char_holder(const char_holder& o) : n_(new char(*o.n_))
    {}

    ~char_holder()
    {
        delete n_;
    }

    char operator()() const
    {
        return *n_;
    }
};

}

TEST_F(TestReplaceIfConversionFailed, NonArithmeticNonNoThrowMoveConstructible)
{
    using r_t = replace_if_conversion_failed<char_holder>;
    static_assert(std::is_nothrow_move_constructible<r_t>::value, "");

    r_t r(replacement_fail, char_holder('I'), replacement_fail, char_holder('B'), replacement_fail);
    std::vector<r_t> rs;
    rs.push_back(r);
    rs.push_back(std::move(r));
    for (const auto& r2 : rs) {
        const char d[] = "dummy";
        const auto de = d + sizeof d - 1;
        ASSERT_THROW(r2.empty(), field_empty);
        ASSERT_EQ('I', (*r2.invalid_format(d, de))());
        ASSERT_THROW(r2.out_of_range(d, de, 1), field_out_of_range);
        ASSERT_EQ('B', (*r2.out_of_range(d, de, -1))());
        ASSERT_THROW(r2.out_of_range(d, de, 0), field_out_of_range);
    }
}

namespace {

template <class T>
class calc_average
{
    T n_ = 0;
    T sum_ = T();

public:
    void operator()(T n)
    {
        ++n_;
        sum_ += n;
    }

    T yield() const
    {
        return sum_ / n_;
    }
};

}

TEST_F(TestReplaceIfConversionFailed, Ignored)
{
    replace_if_conversion_failed<int> r(replacement_ignore, replacement_ignore);
    calc_average<int> a;
    table_scanner<char> scanner;
    scanner.set_field_scanner(0,
        make_field_translator<int>(std::ref(a), fail_if_skipped(), std::move(r)));

    try {
        std::stringbuf buf("100\nn/a\n\n200");
        parse_csv(&buf, std::move(scanner));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(150, a.yield());
}
