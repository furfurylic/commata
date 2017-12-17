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
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <furfurylic/commata/primitive_parser.hpp>
#include <furfurylic/commata/csv_error.hpp>
#include <furfurylic/commata/csv_scanner.hpp>

#include "BaseTest.hpp"

using namespace furfurylic::commata;
using namespace furfurylic::test;

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
            return std::move(s);
        } else {
            return plus1(std::move(s), j - 1);
        }
    } else {
        s[j] = k[1];
        return std::move(s);
    }
}

}

typedef testing::Types<char, wchar_t> Chs;

typedef testing::Types<
    std::pair<char, short>,
    std::pair<char, unsigned short>,
    std::pair<char, int>,
    std::pair<char, unsigned int>,
    std::pair<char, long>,
    std::pair<char, unsigned long>,
    std::pair<char, long long>,
    std::pair<char, unsigned long long>,
    std::pair<wchar_t, short>,
    std::pair<wchar_t, unsigned short>,
    std::pair<wchar_t, int>,
    std::pair<wchar_t, unsigned int>,
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

    csv_scanner<char_t> h;
    h.set_field_scanner(0, make_field_translator_c(values));

    std::basic_stringbuf<char_t> buf(str(" 200\r\n123\t\n-10"));
    ASSERT_NO_THROW(parse(&buf, std::move(h)));
    ASSERT_EQ(3U, values.size());
    ASSERT_EQ(static_cast<value_t>(200), values[0]);
    ASSERT_EQ(static_cast<value_t>(123), values[1]);
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
    const auto maxxPlus1 = plus1(to_string(maxx));

    std::vector<value_t> values;

    csv_scanner<char_t> h;
    h.set_field_scanner(0, make_field_translator_c(values));

    stringstream_t s;
    s << maxx << "\r\n"
      << maxxPlus1;

    try {
        parse(s, std::move(h));
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
        minn = to_string(std::numeric_limits<value_t>::min());
        minnMinus1 = ch('-') + plus1(minn.substr(1));
    } else {
        minn = ch('-') + to_string(std::numeric_limits<value_t>::max());
        minnMinus1 = ch('-') + plus1(plus1(minn.substr(1)));
    }
    const auto maxx = std::numeric_limits<value_t>::max();
    const auto maxxPlus1 = plus1(to_string(maxx));

    std::vector<value_t> values;

    csv_scanner<char_t> h;
    h.set_field_scanner(0, make_field_translator_c(values));

    stringstream_t s;
    s << minn << "\r\n"
      << minnMinus1;

    try {
        parse(s, std::move(h));
        FAIL() << values[0] << ',' << values[1];
    } catch (const field_out_of_range& e) {
        ASSERT_NE(e.get_physical_position(), nullptr);
        ASSERT_EQ(1U, e.get_physical_position()->first);
        const string_t message = widen(e.what());
        ASSERT_TRUE(message.find(minnMinus1) != string_t::npos)
            << message;
    }
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

    csv_scanner<char_t> h;
    h.set_field_scanner(0, make_field_translator_c(values));

    std::basic_string<char_t> s = str("6.02e23\t\r -5\n");
    std::basic_stringbuf<char_t> buf(s);
    try {
        parse(&buf, std::move(h));
    } catch (const csv_error& e) {
        FAIL() << e.what();
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

    csv_scanner<char_t> h;
    h.set_field_scanner(0, make_field_translator_c(values));

    stringstream_t s;
    s << std::scientific << std::setprecision(50) << maxx << "\n"
      << maxxBy10;

    try {
        parse(s, std::move(h));
        FAIL() << values[1];
    } catch (const csv_error& e) {
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

    const auto minn = std::numeric_limits<value_t>::lowest();
    string_t minnBy10;
    {
        stringstream_t ss;
        ss << std::scientific << std::setprecision(50) << minn << '0';
        minnBy10 = ss.str();
    }

    std::vector<value_t> values;

    csv_scanner<char_t> h;
    h.set_field_scanner(0, make_field_translator_c(values));

    stringstream_t s;
    s << std::scientific << std::setprecision(50) << minn << "\n"
      << minnBy10;

    try {
        parse(s, std::move(h));
        FAIL() << values[1];
    } catch (const csv_error& e) {
        ASSERT_NE(e.get_physical_position(), nullptr);
        ASSERT_EQ(1U, e.get_physical_position()->first);
        string_t message;
        {
            stringstream_t ss;
            ss << e.what();
            message = ss.str();
        }
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

    csv_scanner<TypeParam> h;
    h.set_field_scanner(0, make_field_translator_c(values));

    std::basic_string<TypeParam> s = str("ABC  \n\"xy\rz\"\n\"\"");
    std::basic_stringbuf<TypeParam> buf(s);

    try {
        parse(&buf, std::move(h));
    } catch (const csv_error& e) {
        FAIL() << e.what();
    }

    ASSERT_EQ(3U, values.size());
    ASSERT_EQ(str("ABC  "), values[0]);
    ASSERT_EQ(str("xy\rz"), values[1]);
    ASSERT_TRUE(values[2].empty()) << values[2];
}

template <class Ch>
struct TestCsvScanner : BaseTest
{};

TYPED_TEST_CASE(TestCsvScanner, Chs);

TYPED_TEST(TestCsvScanner, Basics)
{
    using string_t = std::basic_string<TypeParam>;

    const auto str = char_helper<TypeParam>::str;

    std::deque<long> values0;
    std::vector<double> values21;
    std::deque<double> values22;
    std::list<string_t> values3;
    std::set<unsigned short> values4;

    csv_scanner<TypeParam> h;
    h.set_field_scanner(0,
        make_field_translator<long>(std::front_inserter(values0)));
    h.set_field_scanner(2, make_field_translator_c(values22));
    h.set_field_scanner(2);
    h.set_field_scanner(2, make_field_translator_c(values21));
    h.set_field_scanner(5, nullptr);
    h.set_field_scanner(4, make_field_translator_c(values4));
    h.set_field_scanner(3, make_field_translator_c(values3));

    std::basic_string<TypeParam> s =
        str("50,__, 101.2 ,XYZ,  200\n"
            "-3,__,3.00e9,\"\"\"ab\"\"\rc\",200\n");
    std::basic_stringbuf<TypeParam> buf(s);
    ASSERT_NO_THROW(parse(&buf, std::move(h)));

    const std::deque<long> expected0 = { -3, 50 };
    const std::vector<double> expected21 = { 101.2, 3.00e9 };
    const std::list<string_t> expected3 = { str("XYZ"), str("\"ab\"\rc") };
    const std::set<unsigned short> expected4 = { 200 };
    ASSERT_EQ(expected0, values0);
    ASSERT_EQ(expected21, values21);
    ASSERT_TRUE(values22.empty());
    ASSERT_EQ(expected3, values3);
    ASSERT_EQ(expected4, values4);
}

TYPED_TEST(TestCsvScanner, SkippedWithNoErrors)
{
    using string_t = std::basic_string<TypeParam>;

    const auto str = char_helper<TypeParam>::str;

    std::deque<string_t> values0;
    std::deque<int> values1;

    csv_scanner<TypeParam> h;
    h.set_field_scanner(0, make_field_translator_c(
        values0, default_if_skipped<string_t>()));
    h.set_field_scanner(1, make_field_translator_c(
        values1, default_if_skipped<int>(50)));

    std::basic_stringstream<TypeParam> s;
    s << "XYZ,20\n"
         "\n"
         "A";
    ASSERT_NO_THROW(parse(s, make_empty_physical_row_aware(std::move(h))));

    const std::deque<string_t> expected0 =
        { str("XYZ"), string_t(), str("A") };
    const std::deque<int> expected1 = { 20, 50, 50 };
    ASSERT_EQ(expected0, values0);
    ASSERT_EQ(expected1, values1);
}

TYPED_TEST(TestCsvScanner, SkippedWithErrors)
{
    std::deque<int> values0;
    std::deque<int> values1;

    csv_scanner<TypeParam> h;
    h.set_field_scanner(0, make_field_translator_c(
        values0, default_if_skipped<int>(10)));
    h.set_field_scanner(1, make_field_translator_c(values1));

    std::basic_stringstream<TypeParam> s;
    s << "10,20\n"
        "-5";
    try {
        parse(s, std::move(h));
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
