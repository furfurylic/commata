/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifdef _MSC_VER
#pragma warning(disable:4494)
#pragma warning(disable:4996)
#endif

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
#include "fancy_allocator.hpp"
#include "tracking_allocator.hpp"

using namespace commata;
using namespace commata::test;

namespace {

template <class Ch>
struct digits
{
    // With std::array, it seems we cannot make the compiler count the number
    // of the elements, so we employ good old arrays
    static const Ch all[];
};

template <>
const char digits<char>::all[] =
    { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

template <>
const wchar_t digits<wchar_t>::all[] =
    { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9' };

template <class Ch>
std::basic_string<Ch> plus1(std::basic_string<Ch> s, std::size_t i
    = static_cast<std::size_t>(-1))
{
    i = std::min(i, static_cast<std::size_t>(s.size() - 1));

    const Ch (&all)[sizeof(digits<Ch>::all) / sizeof(digits<Ch>::all[0])]
        = digits<Ch>::all;
    const auto all_end = all + sizeof(all) / sizeof(all[0]);

    for (;;) {
        const auto k = std::find(all, all_end, s[i]);
        if (k == all_end - 1) {
            s[i] = all[0];  // carrying occurs
            if (i == 0) {
                s.insert(s.begin(), all[1]);  // gcc 6.3.1 refuses s.cbegin()
                break;
            } else {
                --i;
            }
        } else {
            s[i] = k[1];    // for example, modify '3' to '4'
            break;
        }
    }

    return s;
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
{
protected:
    template <class T, class... Args>
    auto make_replace_if_conversion_failed_adding_0(Args&&... args)
    {
        return this->template
            make_replace_if_conversion_failed_adding_0_impl<T>(
                std::is_signed<T>(), std::forward<Args>(args)...);
    }

private:
    template <class T, class... Args>
    auto make_replace_if_conversion_failed_adding_0_impl(
        std::true_type, Args&&... args)
    {
        return replace_if_conversion_failed<T>(
            std::forward<Args>(args)..., static_cast<T>(0));
    }

    template <class T, class... Args>
    auto make_replace_if_conversion_failed_adding_0_impl(
        std::false_type, Args&&... args)
    {
        return replace_if_conversion_failed<T>(std::forward<Args>(args)...);
    }
};

TYPED_TEST_SUITE(TestFieldTranslatorForIntegralTypes, ChIntegrals);

TYPED_TEST(TestFieldTranslatorForIntegralTypes, Correct)
{
    using char_t = typename TypeParam::first_type;
    using value_t = typename TypeParam::second_type;

    const auto str = char_helper<char_t>::str;

    std::vector<value_t> values;

    basic_table_scanner<char_t> h;
    h.set_field_scanner(0, make_field_translator(values));

    ASSERT_NO_THROW(parse_csv(str(" 40\r\n63\t\n-10"), std::move(h)));
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
        [](auto t) { return char_helper<char_t>::to_string(t); };
    const auto widen = char_helper<char_t>::template widen<const char*>;

    constexpr auto maxx = std::numeric_limits<value_t>::max();
    const auto maxxPlus1 = plus1(to_string(maxx + 0));

    std::vector<value_t> values;

    basic_table_scanner<char_t> h;
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
        [](auto t) { return char_helper<char_t>::to_string(t); };
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
    constexpr auto maxx = std::numeric_limits<value_t>::max();
    const auto maxxPlus1 = plus1(to_string(maxx + 0));

    std::vector<value_t> values;

    basic_table_scanner<char_t> h;
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
        [](auto t) { return char_helper<char_t>::to_string(t); };

    string_t minn;
    string_t minnMinus1;
    if (std::is_signed<value_t>::value) {
        minn = to_string(std::numeric_limits<value_t>::min() + 0);
        minnMinus1 = ch('-') + plus1(minn.substr(1));
    } else {
        minn = ch('-') + to_string(std::numeric_limits<value_t>::max() + 0);
        minnMinus1 = ch('-') + plus1(plus1(minn.substr(1)));
    }
    constexpr auto maxx = std::numeric_limits<value_t>::max();
    const auto maxxPlus1 = plus1(to_string(maxx + 0));

    std::vector<value_t> values0;
    std::vector<value_t> values1;
    std::vector<value_t> values2;
    
    basic_table_scanner<char_t> h;
    h.set_field_scanner(0, make_field_translator(values0,
        fail_if_skipped(),
        replace_if_conversion_failed<value_t>(
            static_cast<value_t>(34))));
    h.set_field_scanner(1, make_field_translator(values1,
        replacement_fail,
        replace_if_conversion_failed<value_t>(
            replacement_fail, static_cast<value_t>(42))));
    h.set_field_scanner(2, make_field_translator(values2,
        fail_if_skipped(),
        this->template make_replace_if_conversion_failed_adding_0<value_t>(
            replacement_fail, replacement_fail,
            static_cast<value_t>(1))));

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

TYPED_TEST(TestFieldTranslatorForIntegralTypes, Copy)
{
    using char_t = typename TypeParam::first_type;
    using value_t = typename TypeParam::second_type;

    const auto str0 = char_helper<char_t>::str0;

    const auto empty = str0("");
    const auto ten = str0("10");
    const auto twenty = str0("20");

    std::vector<value_t> values;
    auto t = make_field_translator(values,
        static_cast<value_t>(1),
        replace_if_conversion_failed<value_t>(static_cast<value_t>(2)));
    auto u = t;
    t(ten.data(), ten.data() + (ten.size() - 1));
    u(empty.data(), empty.data());
    t();
    u(twenty.data(), twenty.data() + (twenty.size() - 1));

    const std::vector<value_t> expected = { 10, 2, 1, 20 };
    ASSERT_EQ(expected, values);
}

struct TestFieldTranslatorForIntegralRestriction : BaseTest
{};

TEST_F(TestFieldTranslatorForIntegralRestriction, Unsigned)
{
    // If unsigned short is as long as unsigned long,
    // this test will be somewhat absurd, but it does not seem likely

    std::string max = std::to_string(
        std::numeric_limits<unsigned short>::max());
    std::string maxp1 = plus1(max);

    std::stringstream s;
    s << max   << '\n'
      << maxp1 << '\n'
      << '-' << max   << '\n'
      << '-' << maxp1 << '\n';

    std::vector<unsigned short> values;

    basic_table_scanner<char> h;
    h.set_field_scanner(0, make_field_translator(values, fail_if_skipped(),
        replace_if_conversion_failed<unsigned short>(
            static_cast<unsigned short>(3),     // empty
            static_cast<unsigned short>(4),     // invalid
            static_cast<unsigned short>(2))));  // above max

    try {
        parse_csv(s, std::move(h));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(std::numeric_limits<unsigned short>::max(), values[0]);
    ASSERT_EQ(2, values[1]);
    ASSERT_EQ(1, values[2]);    // wrapped around
    ASSERT_EQ(2, values[3]);
}

template <class ChNum>
struct TestFieldTranslatorForFloatingPointTypes : BaseTest
{};

TYPED_TEST_SUITE(TestFieldTranslatorForFloatingPointTypes, ChFloatingPoints);

TYPED_TEST(TestFieldTranslatorForFloatingPointTypes, Correct)
{
    using char_t = typename TypeParam::first_type;
    using value_t = typename TypeParam::second_type;

    const auto str = char_helper<char_t>::str;

    std::vector<value_t> values;

    basic_table_scanner<char_t> h;
    h.set_field_scanner(0, make_field_translator(values));

    std::basic_string<char_t> s = str("6.02e23\t\r -5\n");
    try {
        parse_csv(s, std::move(h));
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

    constexpr auto maxx = std::numeric_limits<value_t>::max();
    string_t maxxBy10;
    {
        stringstream_t ss;
        ss << std::scientific << std::setprecision(50) << maxx << '0';
        maxxBy10 = ss.str();
    }

    std::vector<value_t> values;

    basic_table_scanner<char_t> h;
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

    constexpr auto minn = std::numeric_limits<value_t>::lowest();
    string_t minnBy10;
    {
        stringstream_t ss;
        ss << std::scientific << std::setprecision(50) << minn << '0';
        minnBy10 = ss.str();
    }

    std::vector<value_t> values;

    basic_table_scanner<char_t> h;
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

TYPED_TEST_SUITE(TestFieldTranslatorForStringTypes, Chs);

TYPED_TEST(TestFieldTranslatorForStringTypes, Correct)
{
    const auto str = char_helper<TypeParam>::str;

    std::deque<std::basic_string<TypeParam>> values;

    basic_table_scanner<TypeParam> h;
    h.set_field_scanner(0, make_field_translator(values));

    std::basic_string<TypeParam> s = str("ABC  \n\"xy\rz\"\n\"\"");

    try {
        parse_csv(s, std::move(h));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(3U, values.size());
    ASSERT_EQ(str("ABC  "), values[0]);
    ASSERT_EQ(str("xy\rz"), values[1]);
    ASSERT_TRUE(values[2].empty()) << values[2];
}

TYPED_TEST(TestFieldTranslatorForStringTypes, Copy)
{
    using char_t = TypeParam;
    using string_t = std::basic_string<TypeParam>;

    const auto str = char_helper<char_t>::str;

    std::vector<string_t> values;
    auto t = make_field_translator(values,
        replace_if_skipped<string_t>(str("1")));
    auto u = t;
    t(str("10"));
    u();
    t(str("20"));

    const std::vector<string_t> expected = { str("10"), str("1"), str("20") };
    ASSERT_EQ(expected, values);
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

template <class Ch>
struct TestLocaleBased : BaseTest
{};

TYPED_TEST_SUITE(TestLocaleBased, Chs);

TYPED_TEST(TestLocaleBased, FrenchStyle)
{
    const auto str0 = char_helper<TypeParam>::str0;

    std::vector<int> values0;
    std::deque<double> values1;

    std::locale loc(std::locale::classic(),
        new french_style_numpunct<TypeParam>);

    auto s0 = str0("100 000");
    auto s1 = str0("12 345 678,5");

    auto t = make_field_translator(values0, loc, replacement_ignore);
    auto u = make_field_translator(values1, loc, replacement_fail);

    t(&s0[0], &s0[0] + (s0.size() - 1));
    t();
    u(&s1[0], &s1[0] + (s1.size() - 1));
    ASSERT_THROW(u(), field_not_found);

    ASSERT_EQ(1U, values0.size());
    ASSERT_EQ(100000, values0.back());
    ASSERT_EQ(1U, values1.size());
    ASSERT_EQ(12345678.5, values1.back());
}

TYPED_TEST(TestLocaleBased, Copy)
{
    const auto str0 = char_helper<TypeParam>::str0;

    std::deque<double> values;
    const auto f = [&values](double a) {
        values.push_back(a);
    };

    std::locale loc(std::locale::classic(),
        new french_style_numpunct<TypeParam>);

    auto empty = str0("");
    auto s0 = str0("12 345 678,5");
    auto s1 = str0("-9 999");

    auto t = make_field_translator<double>(f, loc, 33.33,
        replace_if_conversion_failed<double>(777.77));
    auto u = t;

    t(&s0[0], &s0[0] + (s0.size() - 1));
    u();
    t(&empty[0], &empty[0]);
    u(&s1[0], &s1[0] + (s1.size() - 1));

    const std::deque<double> expected = { 12345678.5, 33.33, 777.77, -9999.0 };
    ASSERT_EQ(expected, values);
}

static_assert(
    std::uses_allocator<table_scanner, std::allocator<char>>::value, "");

template <class Ch>
struct TestTableScanner : BaseTest
{};

TYPED_TEST_SUITE(TestTableScanner, Chs);

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

    basic_table_scanner<TypeParam> h(1U);
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

    basic_table_scanner<TypeParam> h(1U);
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
             "Abbott";  // elaborately does not end with CR/lf_c
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

    basic_table_scanner<TypeParam> h(3U);
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

    basic_table_scanner<TypeParam> h;
    h.set_field_scanner(0, make_field_translator(
        values0, replace_if_skipped<string_t>()));
    h.set_field_scanner(1, make_field_translator(
        values1, replace_if_skipped<int>(50)));

    const auto scanner1 = h.template
        get_field_scanner<decltype(make_field_translator(
            values1, replace_if_skipped<int>()))>(1U);
    ASSERT_NE(scanner1, nullptr);
    ASSERT_EQ(50, *scanner1->get_skipping_handler()());

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

    basic_table_scanner<TypeParam> h;
    h.set_field_scanner(0, make_field_translator(
        values0, replace_if_skipped<int>(10)));
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

    basic_table_scanner<TypeParam> h(
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
    try {
        parse_csv(s, std::move(h));
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

    basic_table_scanner<TypeParam> h(
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

    ASSERT_NO_THROW(parse_csv(str("A\n1\n"), std::move(h)));
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
        const std::pair<Ch*, Ch*>* range, basic_table_scanner<Ch>& s)
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
    basic_table_scanner<TypeParam> scanner(std::move(h));
    auto s = str("AUD,AUD,EUR\r"
                 "JPY,USD,USD\r"
                 "80.0,0.9,1.3\r"
                 "82.1,0.91,1.35");
    parse_csv(s, std::move(scanner));

    std::vector<double> aud_jpy = { 80.0, 82.1 };
    ASSERT_EQ(aud_jpy, fx[str("AUDJPY")]);

    std::vector<double> aud_usd = { 0.9, 0.91 };
    ASSERT_EQ(aud_usd, fx[str("AUDUSD")]);

    std::vector<double> eur_usd = { 1.3, 1.35 };
    ASSERT_EQ(eur_usd, fx[str("EURUSD")]);
}

namespace {

template <class Ch>
class basic_table_scanner_wrapper
{
    basic_table_scanner<Ch> s_;

public:
    using char_type = Ch;

    basic_table_scanner_wrapper(basic_table_scanner<Ch>&& s) :
        s_(std::move(s))
    {}

    std::pair<Ch*, std::size_t> get_buffer()
    {
        return s_.get_buffer();
    }

    void release_buffer(const Ch* buffer)
    {
        s_.release_buffer(buffer);
    }

    void start_record(const Ch* record_begin)
    {
        s_.start_record(record_begin);
    }

    void end_record(const Ch* record_end)
    {
        s_.end_record(record_end);
    }

    void update(const Ch* first, const Ch* last)
    {
        if (s_.is_in_header()) {
            to_upper(first, last);
        }
        s_.update(first, last);
    }

    void finalize(const Ch* first, const Ch* last)
    {
        if (s_.is_in_header()) {
            to_upper(first, last);
        }
        s_.finalize(first, last);
    }

private:
    void to_upper(const Ch* first, const Ch* last) const;
};

template <>
void basic_table_scanner_wrapper<char>::
to_upper(const char* first, const char* last) const
{
    std::transform(first, last, const_cast<char*>(first), [](auto c) {
        return static_cast<char>(
            std::toupper(static_cast<int>(c)));
    });
}

template <>
void basic_table_scanner_wrapper<wchar_t>::
to_upper(const wchar_t* first, const wchar_t* last) const
{
    std::transform(first, last, const_cast<wchar_t*>(first), [](auto c) {
        return static_cast<wchar_t>(
            std::towupper(static_cast<std::wint_t>(c)));
    });
}

}

TYPED_TEST(TestTableScanner, IsInHeader)
{
    using string = std::basic_string<TypeParam>;

    const auto str = char_helper<TypeParam>::str;

    std::map<string, string> currency_map;
    std::pair<string, string> record;
    basic_table_scanner<TypeParam> scanner(
        [&str, &record](auto j, auto v, auto& t) {
            if (v) {
                const string field_name(v->first, v->second);
                if (field_name == str("COUNTRY")) {
                    t.set_field_scanner(j, make_field_translator<string>(
                        [&record](string&& s) {
                            record.first = std::move(s);
                        }));
                } else if (field_name == str("CURRENCY")) {
                    t.set_field_scanner(j, make_field_translator<string>(
                        [&record](string&& s) {
                            record.second = std::move(s);
                        }));
                }
                return true;
            } else {
                return false;
            }
        });
    scanner.set_record_end_scanner([&currency_map, &record] {
        currency_map.insert(std::move(record));
    });

    auto s = str("Country,Currency\r"
                 "Ukraine,Hryvnia\r"
                 "Estonia,Euro\r");
    parse_csv(s,
        basic_table_scanner_wrapper<TypeParam>(std::move(scanner)));

    ASSERT_EQ(2U, currency_map.size());
    ASSERT_EQ(str("Hryvnia"), currency_map[str("Ukraine")]);
    ASSERT_EQ(str("Euro"), currency_map[str("Estonia")]);
}

TYPED_TEST(TestTableScanner, BufferSize)
{
    using string_t = std::basic_string<TypeParam>;

    const auto str = char_helper<TypeParam>::str;

    std::vector<string_t> values0;
    std::vector<int> values1;

    for (std::size_t buffer_size : { 2U, 3U, 4U, 7U  }) {
        basic_table_scanner<TypeParam> h(0U, buffer_size);
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

    basic_table_scanner<TypeParam, Tr, Alloc> scanner(
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
        std::basic_string<TypeParam> s =
            str("ABCDEFGHIJKLMNOPQRSTUVWXYZ,"
                "abcdefghijklmnopqrstuvwxyz,"
                "12345678901234567890123456");
        parse_csv(s, std::move(scanner));
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

    basic_table_scanner<TypeParam> h1;
    auto t = make_field_translator(values);
    h1.set_field_scanner(0, std::move(t));

    basic_table_scanner<TypeParam> h2(std::move(h1));

    ASSERT_FALSE(h1.has_field_scanner(0));
    ASSERT_EQ(typeid(void), h1.get_field_scanner_type(0));
    ASSERT_EQ(nullptr, h1.template get_field_scanner<decltype(t)>(0));

    ASSERT_NE(nullptr, h2.template get_field_scanner<decltype(t)>(0));
}

namespace {

template <class Ch, class F>
class check_scanner
{
    F f_;

public:
    explicit check_scanner(F f) : f_(f) {}

    void operator()(Ch* b, Ch* e) { f_(b); f_(e - 1); }

    void operator()() {}
};

template <class Ch, class F>
check_scanner<Ch, F> make_check_scanner(F f)
{
    return check_scanner<Ch, F>(f);
}

}

TYPED_TEST(TestTableScanner, Fancy)
{
    const auto str = char_helper<TypeParam>::str;

    std::vector<std::pair<char*, char*>> allocated;
    tracking_allocator<fancy_allocator<TypeParam>> a(allocated);

    auto f = [&a](TypeParam* p) {
        if (!a.tracks(p)) {
            throw text_error("Not tracked");
        }
    };

    basic_table_scanner<TypeParam, std::char_traits<TypeParam>, decltype(a)>
        scanner(std::allocator_arg, a);
    auto g = make_check_scanner<TypeParam>(f);
    auto h = [] {};
    scanner.set_field_scanner(0, std::move(g));
    scanner.set_record_end_scanner(std::move(h));
    ASSERT_TRUE(a.tracks(scanner.template get_field_scanner<decltype(g)>(0)));
    ASSERT_TRUE(a.tracks_relax(scanner.template
        get_record_end_scanner<decltype(h)>()));

    ASSERT_NO_THROW(parse_csv(str("abc"), std::move(scanner)));
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

TYPED_TEST(TestTableScanner, Ignored)
{
    replace_if_conversion_failed<int> r(
        replacement_ignore, replacement_ignore);
    calc_average<int> a;
    basic_table_scanner<TypeParam> scanner;
    scanner.set_field_scanner(0, make_field_translator<int>(
        std::ref(a), fail_if_skipped(), std::move(r)));

    const auto str = char_helper<TypeParam>::str;

    try {
        parse_csv(str("100\nn/a\n\n200"), std::move(scanner));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(150, a.yield());
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
    table_scanner scanner(std::ref(header_scanner));

    header_scanner.index = 1;

    try {
        parse_csv("A,B\n100,200", std::move(scanner));
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
    table_scanner scanner;
    scanner.set_field_scanner(0, std::ref(field_scanner));

    ASSERT_EQ(typeid(std::reference_wrapper<decltype(field_scanner)>),
        scanner.get_field_scanner_type(0));

    try {
        parse_csv("100", std::move(scanner));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(1U, values1.size());
    ASSERT_EQ(100, values1[0]);
}

TEST_F(TestTableScannerReference, RecordEndScanner)
{
    std::vector<int> v;

    table_scanner scanner;
    scanner.set_field_scanner(0, make_field_translator(v));

    std::function<void()> record_end_scanner = [] {};
    scanner.set_record_end_scanner();                   // shall be nop
    scanner.set_record_end_scanner(record_end_scanner); // overwritten
    scanner.set_record_end_scanner(std::ref(record_end_scanner));

    record_end_scanner = [&v]() {
        v.push_back(-12345);
    };

    ASSERT_EQ(typeid(std::reference_wrapper<decltype(record_end_scanner)>),
        scanner.get_record_end_scanner_type());

    try {
        parse_csv("100\n200", std::move(scanner));
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

    // Copy construction
    const auto r2(r);
    ASSERT_NE(r2.get(), nullptr);
    ASSERT_STREQ("ABCDEFG", r2.get()->c_str());
    ASSERT_STREQ(r2.get()->c_str(), r2->c_str());
    ASSERT_EQ(std::string("ABCDEFG"), *r2);

    // Move construction
    const auto r3(std::move(r));
    ASSERT_NE(r3.get(), nullptr);
    ASSERT_STREQ("ABCDEFG", r3.get()->c_str());
    ASSERT_STREQ(r3.get()->c_str(), r3->c_str());
    ASSERT_EQ(std::string("ABCDEFG"), *r3);

    // Moved-from state: these are not specified in the spec
    ASSERT_EQ(nullptr, r.get());
    ASSERT_FALSE(static_cast<bool>(r));
}

TEST_F(TestReplacement, HeterogeneousConstruction)
{
    replacement<char> r('1');

    replacement<int> r1(r);
    ASSERT_NE(r1.get(), nullptr);
    ASSERT_EQ('1', *r1);

    replacement<int> r2(std::move(r));
    ASSERT_NE(r2.get(), nullptr);
    ASSERT_EQ('1', *r2);
}

TEST_F(TestReplacement, None)
{
    replacement<std::string> r1;
    ASSERT_EQ(nullptr, r1.get());

    replacement<std::string> r2(replacement_ignore);
    ASSERT_EQ(nullptr, r2.get());
}

TEST_F(TestReplacement, Assignment)
{
    replacement<std::string> rs[2];

    // Assignment from T to replacement with none
    {
        std::string s("ABCDEF");
        rs[1] = s;
        rs[0] = std::move(s);
    }
    for (std::size_t i : { 0, 1 }) {
        const auto& r = rs[i];
        ASSERT_NE(r.get(), nullptr) << i;
        ASSERT_STREQ("ABCDEF", r.get()->c_str()) << i;
        ASSERT_STREQ(r.get()->c_str(), r->c_str()) << i;
        ASSERT_EQ(std::string("ABCDEF"), *r) << i;
    }

    // Assignment from replacement with T to replacement with T
    {
        replacement<std::string> r(std::string("123456"));
        rs[1] = r;
        rs[0] = std::move(r);
        // Moved-from state: these are not specified in the spec
        ASSERT_EQ(nullptr, r.get());
        ASSERT_FALSE(static_cast<bool>(r));
    }
    for (std::size_t i : { 0, 1 }) {
        const auto& r = rs[i];
        ASSERT_NE(r.get(), nullptr) << i;
        ASSERT_STREQ("123456", r.get()->c_str()) << i;
        ASSERT_STREQ(r.get()->c_str(), r->c_str()) << i;
        ASSERT_EQ(std::string("123456"), *r) << i;
    }

    // Assignment from replacement with replacement with U (convertible to T)
    // to replacement with T
    {
        replacement<const char*> r("ijklmn");
        rs[1] = r;
        rs[0] = std::move(r);
        // Moved-from state: these are not specified in the spec
        ASSERT_EQ(nullptr, r.get());
        ASSERT_FALSE(static_cast<bool>(r));
    }
    for (std::size_t i : { 0, 1 }) {
        const auto& r = rs[i];
        ASSERT_NE(r.get(), nullptr) << i;
        ASSERT_STREQ("ijklmn", r.get()->c_str()) << i;
        ASSERT_STREQ(r.get()->c_str(), r->c_str()) << i;
        ASSERT_EQ(std::string("ijklmn"), *r) << i;
    }

    // Self-assignment
    rs[0] = std::move(rs[0]);
    // Strictly speaking, rs[0] can be placed in a moved-from state and there
    // are no guarantees on its state, but we offer harmless self-assignment
    ASSERT_NE(rs[0].get(), nullptr);
    ASSERT_STREQ("ijklmn", rs[0].get()->c_str());
    ASSERT_STREQ(rs[0].get()->c_str(), rs[0]->c_str());
    ASSERT_EQ(std::string("ijklmn"), *rs[0]);

    // Assignment from replacement with none to replacement with T
    {
        replacement<std::string> r;
        rs[1] = r;
        rs[0] = std::move(r);
    }
    for (std::size_t i : { 0, 1 }) {
        const auto& r = rs[i];
        ASSERT_EQ(nullptr, r.get()) << i;
    }
}

TEST_F(TestReplacement, Swap)
{
    using std::swap;

    replacement<int> rs[2];
    rs[0] = 100;

    static_assert(noexcept(swap(rs[0], rs[1])), "");

    swap(rs[0], rs[1]);
    ASSERT_EQ(nullptr, rs[0].get());
    ASSERT_NE(rs[1].get(), nullptr);
    ASSERT_EQ(100, *rs[1]);

    swap(rs[0], rs[1]);
    ASSERT_NE(rs[0].get(), nullptr);
    ASSERT_EQ(100, *rs[0]);
    ASSERT_EQ(nullptr, rs[1].get());

    rs[1] = 200;
    swap(rs[0], rs[1]);
    ASSERT_NE(rs[0].get(), nullptr);
    ASSERT_EQ(200, *rs[0]);
    ASSERT_NE(rs[1].get(), nullptr);
    ASSERT_EQ(100, *rs[1]);
}

struct TestReplaceIfSkipped : BaseTest
{};

TEST_F(TestReplaceIfSkipped, ActionInstallmentWithCtors)
{
    // default ctor
    {
        replace_if_skipped<std::string> r;
        ASSERT_STREQ("", r()->c_str());
    }

    // copy
    {
        replace_if_skipped<std::string> r(3, 'A');
        ASSERT_STREQ("AAA", r()->c_str());
    }

    // ignore
    {
        replace_if_skipped<std::string> r(replacement_ignore);
        ASSERT_TRUE(!r());
    }

    // fail
    {
        replace_if_skipped<std::string> r(replacement_fail);
        ASSERT_THROW(r(), field_not_found);
    }
}

TEST_F(TestReplaceIfSkipped, CopyCtor)
{
    // copy
    {
        const replace_if_skipped<std::string> r0("XYZ");
        replace_if_skipped<std::string> r(r0);
        ASSERT_STREQ("XYZ", r()->c_str());
    }

    // ignore
    {
        const replace_if_skipped<std::string> r0(replacement_ignore);
        replace_if_skipped<std::string> r(r0);
        ASSERT_TRUE(!r());
    }

    // fail
    {
        const replace_if_skipped<std::string> r0(replacement_fail);
        replace_if_skipped<std::string> r(r0);
        ASSERT_THROW(r(), field_not_found);
    }
}

TEST_F(TestReplaceIfSkipped, MoveCtor)
{
    // copy
    {
        replace_if_skipped<std::string> r0("XYZ");
        replace_if_skipped<std::string> r(std::move(r0));
        ASSERT_STREQ("XYZ", r()->c_str());
    }

    // ignore
    {
        replace_if_skipped<std::string> r0(replacement_ignore);
        replace_if_skipped<std::string> r(std::move(r0));
        ASSERT_TRUE(!r());
    }

    // fail
    {
        replace_if_skipped<std::string> r0(replacement_fail);
        replace_if_skipped<std::string> r(std::move(r0));
        ASSERT_THROW(r(), field_not_found);
    }
}

TEST_F(TestReplaceIfSkipped, CopyAssign)
{
    using r_t = replace_if_skipped<std::vector<int>>;

    // from copy
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(std::vector<int>{ -1, -2, -3 });

        const std::vector<int> v = { 10, 20, 30 };
        r_t r0(v);

        rs[0] = r0;
        rs[1] = r0;
        rs[2] = r0;
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_EQ(v, *rs[i]()) << i;
        }

        r0 = r0;
        ASSERT_EQ(v, *r0());
    }

    // from ignore
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(std::vector<int>{ -1, -2, -3 });

        r_t r0(replacement_ignore);

        rs[0] = r0;
        rs[1] = r0;
        rs[2] = r0;
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_TRUE(!rs[i]()) << i;
        }

        r0 = r0;
        ASSERT_TRUE(!r0());
    }

    // from fail
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(std::vector<int>{ -1, -2, -3 });

        r_t r0(replacement_fail);

        rs[0] = r0;
        rs[1] = r0;
        rs[2] = r0;
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_THROW(rs[i](), field_not_found) << i;
        }

        r0 = r0;
        ASSERT_THROW(r0(), field_not_found);
    }
}

TEST_F(TestReplaceIfSkipped, MoveAssign)
{
    using r_t = replace_if_skipped<std::vector<int>>;

    // from copy
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(std::vector<int>{ -1, -2, -3 });

        const std::vector<int> v = { 10, 20, 30 };

        rs[0] = r_t(v);
        rs[1] = r_t(v);
        rs[2] = r_t(v);
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_EQ(v, *rs[i]()) << i;
        }

        rs[0] = std::move(rs[0]);
        ASSERT_EQ(v, *rs[0]());
    }

    // from ignore
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(std::vector<int>{ -1, -2, -3 });

        rs[0] = r_t(replacement_ignore);
        rs[1] = r_t(replacement_ignore);
        rs[2] = r_t(replacement_ignore);
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_TRUE(!rs[i]()) << i;
        }

        rs[0] = std::move(rs[0]);
        ASSERT_TRUE(!rs[0]());
    }

    // from fail
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(std::vector<int>{ -1, -2, -3 });

        rs[0] = r_t(replacement_fail);
        rs[1] = r_t(replacement_fail);
        rs[2] = r_t(replacement_fail);
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_THROW(rs[i](), field_not_found) << i;
        }

        rs[0] = std::move(rs[0]);
        ASSERT_THROW(rs[0](), field_not_found);
    }
}

TEST_F(TestReplaceIfSkipped, Swap)
{
    std::vector<replace_if_skipped<std::string>> rs;
    rs.emplace_back("ABC");
    rs.emplace_back(replacement_ignore);
    rs.emplace_back(replacement_fail);
    rs.emplace_back("xyz");

    using std::swap;

    // copy vs ignore
    swap(rs[0], rs[1]);
    ASSERT_TRUE(!rs[0]());
    ASSERT_STREQ("ABC", rs[1]()->c_str());
    swap(rs[0], rs[1]);
    ASSERT_STREQ("ABC", rs[0]()->c_str());
    ASSERT_TRUE(!rs[1]());

    // ignore vs fail
    swap(rs[1], rs[2]);
    ASSERT_TRUE(!rs[2]());
    ASSERT_THROW(rs[1](), field_not_found);
    swap(rs[1], rs[2]);
    ASSERT_TRUE(!rs[1]());
    ASSERT_THROW(rs[2](), field_not_found);

    // fail vs copy
    swap(rs[2], rs[3]);
    ASSERT_STREQ("xyz", rs[2]()->c_str());
    ASSERT_THROW(rs[3](), field_not_found);
    swap(rs[2], rs[3]);
    ASSERT_STREQ("xyz", rs[3]()->c_str());
    ASSERT_THROW(rs[2](), field_not_found);

    // copy vs copy
    swap(rs[3], rs[0]);
    ASSERT_STREQ("ABC", rs[3]()->c_str());
    ASSERT_STREQ("xyz", rs[0]()->c_str());
    swap(rs[3], rs[0]);
    ASSERT_STREQ("xyz", rs[3]()->c_str());
    ASSERT_STREQ("ABC", rs[0]()->c_str());

    // swap with self
    swap(rs[0], rs[0]);
    ASSERT_STREQ("ABC", rs[0]()->c_str());
    swap(rs[1], rs[1]);
    ASSERT_TRUE(!rs[1]());
    swap(rs[2], rs[2]);
    ASSERT_THROW(rs[2](), field_not_found);
}

namespace replace_if_skipped_static_asserts {

struct a_t : std::allocator<int>
{
    using propagate_on_container_swap = std::true_type;
};

using ri_t = replace_if_skipped<int>;
using rv_t = replace_if_skipped<std::vector<int, a_t>>;
using std::swap;

static_assert(std::is_nothrow_copy_constructible<ri_t>::value, "");
static_assert(std::is_nothrow_move_constructible<ri_t>::value, "");
static_assert(std::is_nothrow_copy_assignable<ri_t>::value, "");
static_assert(std::is_nothrow_move_assignable<ri_t>::value, "");
static_assert(noexcept(
    swap(std::declval<ri_t&>(), std::declval<ri_t&>())), "");

static_assert(!std::is_nothrow_copy_constructible<rv_t>::value, "");
static_assert(std::is_nothrow_move_constructible<rv_t>::value, "");
static_assert(!std::is_nothrow_copy_assignable<rv_t>::value, "");
static_assert(std::is_nothrow_move_assignable<rv_t>::value, "");
static_assert(noexcept(
    swap(std::declval<rv_t&>(), std::declval<rv_t&>())), "");

}

template <class T>
struct TestReplaceIfConversionFailed : BaseTestWithParam<T>
{};

typedef testing::Types<double, std::string> ReplacedTypes;

TYPED_TEST_SUITE(TestReplaceIfConversionFailed, ReplacedTypes);

TYPED_TEST(TestReplaceIfConversionFailed, CtorsCopy)
{
    using r_t = replace_if_conversion_failed<TypeParam>;

    char d[] = "dummy";
    char* de = d + sizeof d - 1;

    const auto from_str = [](const char* s) {
        std::stringstream str;
        str << s;
        TypeParam num;
        str >> num;
        return num;
    };

    TypeParam num_1 = from_str("10");
    TypeParam num_2 = from_str("15");
    TypeParam num_3 = from_str("-35");
    TypeParam num_4 = from_str("55");

    std::deque<r_t> rs;
    /*0*/rs.emplace_back(num_1, num_2, num_3, num_4);
    /*1*/rs.emplace_back(rs[0]);
    /*2*/rs.emplace_back(std::move(r_t(rs[0])));

    for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
        const auto& r = rs[i];
        ASSERT_EQ(num_1, *r(empty_t())) << i;
        ASSERT_EQ(num_2, *r(invalid_format_t(), d, de)) << i;
        ASSERT_EQ(num_3, *r(out_of_range_t(), d, de, 1)) << i;
        ASSERT_EQ(num_4, *r(out_of_range_t(), d, de, -1)) << i;
        ASSERT_EQ(TypeParam(), *r(out_of_range_t(), d, de, 0)) << i;
    }
}

TYPED_TEST(TestReplaceIfConversionFailed, CtorsIgnore)
{
    using r_t = replace_if_conversion_failed<TypeParam>;

    char d[] = "dummy";
    char* de = d + sizeof d - 1;

    std::deque<r_t> rs;
    /*0*/rs.emplace_back(replacement_ignore, replacement_ignore,
            replacement_ignore, replacement_ignore, replacement_ignore);
    /*1*/rs.emplace_back(rs[0]);
    /*2*/rs.emplace_back(std::move(r_t(rs[0])));

    for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
        const auto& r = rs[i];
        ASSERT_TRUE(!r(empty_t())) << i;
        ASSERT_TRUE(!r(invalid_format_t(), d, de)) << i;
        ASSERT_TRUE(!r(out_of_range_t(), d, de, 1)) << i;
        ASSERT_TRUE(!r(out_of_range_t(), d, de, -1)) << i;
        ASSERT_TRUE(!r(out_of_range_t(), d, de, 0)) << i;
    }
}

TYPED_TEST(TestReplaceIfConversionFailed, CtorsFail)
{
    using r_t = replace_if_conversion_failed<TypeParam>;

    char d[] = "dummy";
    char* de = d + sizeof d - 1;

    std::deque<r_t> rs;
    /*0*/rs.emplace_back(replacement_fail, replacement_fail,
            replacement_fail, replacement_fail, replacement_fail);
    /*1*/rs.emplace_back(rs[0]);
    /*2*/rs.emplace_back(std::move(r_t(rs[0])));

    for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
        const auto& r = rs[i];
        ASSERT_THROW(r(empty_t()), field_empty);
        ASSERT_THROW(r(invalid_format_t(), d, de), field_invalid_format) << i;
        ASSERT_THROW(r(out_of_range_t(), d, de, 1), field_out_of_range) << i;
        ASSERT_THROW(r(out_of_range_t(), d, de, -1), field_out_of_range) << i;
        ASSERT_THROW(r(out_of_range_t(), d, de, 0), field_out_of_range) << i;
    }
}

TYPED_TEST(TestReplaceIfConversionFailed, CopyAssign)
{
    using r_t = replace_if_conversion_failed<TypeParam>;

    char d[] = "dummy";
    char* de = d + sizeof d - 1;

    const auto from_str = [](const char* s) {
        std::stringstream str;
        str << s;
        TypeParam num;
        str >> num;
        return num;
    };

    TypeParam num_1 = from_str("10");
    TypeParam num_2 = from_str("15");
    TypeParam num_3 = from_str("-35");
    TypeParam num_4 = from_str("55");
    TypeParam num_5 = from_str("-90");

    // from copy
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(num_3, num_4, num_5, num_1, num_2);

        r_t r0(num_1, num_2, num_3, num_4, num_5);

        rs[0] = r0;
        rs[1] = r0;
        rs[2] = r0;
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            const auto& r = rs[i];
            ASSERT_EQ(num_1, *r(empty_t())) << i;
            ASSERT_EQ(num_2, *r(invalid_format_t(), d, de)) << i;
            ASSERT_EQ(num_3, *r(out_of_range_t(), d, de, 1)) << i;
            ASSERT_EQ(num_4, *r(out_of_range_t(), d, de, -1)) << i;
            ASSERT_EQ(num_5, *r(out_of_range_t(), d, de, 0)) << i;
        }

        r0 = r0;
        ASSERT_EQ(num_1, *r0(empty_t()));
        ASSERT_EQ(num_2, *r0(invalid_format_t(), d, de));
        ASSERT_EQ(num_3, *r0(out_of_range_t(), d, de, 1));
        ASSERT_EQ(num_4, *r0(out_of_range_t(), d, de, -1));
        ASSERT_EQ(num_5, *r0(out_of_range_t(), d, de, 0));
    }

    // from ignore
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(num_3, num_4, num_5, num_1, num_2);

        r_t r0(replacement_ignore, replacement_ignore, replacement_ignore,
               replacement_ignore, replacement_ignore);

        rs[0] = r0;
        rs[1] = r0;
        rs[2] = r0;
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            const auto& r = rs[i];
            ASSERT_TRUE(!r(empty_t())) << i;
            ASSERT_TRUE(!r(invalid_format_t(), d, de)) << i;
            ASSERT_TRUE(!r(out_of_range_t(), d, de, 1)) << i;
            ASSERT_TRUE(!r(out_of_range_t(), d, de, -1)) << i;
            ASSERT_TRUE(!r(out_of_range_t(), d, de, 0)) << i;
        }

        r0 = r0;
        ASSERT_TRUE(!r0(empty_t()));
        ASSERT_TRUE(!r0(invalid_format_t(), d, de));
        ASSERT_TRUE(!r0(out_of_range_t(), d, de, 1));
        ASSERT_TRUE(!r0(out_of_range_t(), d, de, -1));
        ASSERT_TRUE(!r0(out_of_range_t(), d, de, 0));
    }

    // from fail
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(num_3, num_4, num_5, num_1, num_2);

        r_t r0(replacement_fail, replacement_fail, replacement_fail,
               replacement_fail, replacement_fail);

        rs[0] = r0;
        rs[1] = r0;
        rs[2] = r0;
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            const auto& r = rs[i];
            ASSERT_THROW(r(empty_t()), field_empty) << i;
            ASSERT_THROW(r(invalid_format_t(), d, de), field_invalid_format) << i;
            ASSERT_THROW(r(out_of_range_t(), d, de, 1), field_out_of_range) << i;
            ASSERT_THROW(r(out_of_range_t(), d, de, -1), field_out_of_range) << i;
            ASSERT_THROW(r(out_of_range_t(), d, de, 0), field_out_of_range) << i;
        }

        r0 = r0;
        ASSERT_THROW(r0(empty_t()), field_empty);
        ASSERT_THROW(r0(invalid_format_t(), d, de), field_invalid_format);
        ASSERT_THROW(r0(out_of_range_t(), d, de, 1), field_out_of_range);
        ASSERT_THROW(r0(out_of_range_t(), d, de, -1), field_out_of_range);
        ASSERT_THROW(r0(out_of_range_t(), d, de, 0), field_out_of_range);
    }
}

TYPED_TEST(TestReplaceIfConversionFailed, MoveAssign)
{
    using r_t = replace_if_conversion_failed<TypeParam>;

    char d[] = "dummy";
    char* de = d + sizeof d - 1;

    const auto from_str = [](const char* s) {
        std::stringstream str;
        str << s;
        TypeParam num;
        str >> num;
        return num;
    };

    TypeParam num_1 = from_str("10");
    TypeParam num_2 = from_str("15");
    TypeParam num_3 = from_str("-35");
    TypeParam num_4 = from_str("55");
    TypeParam num_5 = from_str("-90");

    // from copy
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(num_3, num_4, num_5, num_1, num_2);

        r_t r0(num_1, num_2, num_3, num_4, num_5);

        rs[0] = r_t(r0);
        rs[1] = r_t(r0);
        rs[2] = r_t(r0);
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            const auto& r = rs[i];
            ASSERT_EQ(num_1, *r(empty_t())) << i;
            ASSERT_EQ(num_2, *r(invalid_format_t(), d, de)) << i;
            ASSERT_EQ(num_3, *r(out_of_range_t(), d, de, 1)) << i;
            ASSERT_EQ(num_4, *r(out_of_range_t(), d, de, -1)) << i;
            ASSERT_EQ(num_5, *r(out_of_range_t(), d, de, 0)) << i;
        }

        r0 = std::move(r0);
        ASSERT_EQ(num_1, *r0(empty_t()));
        ASSERT_EQ(num_2, *r0(invalid_format_t(), d, de));
        ASSERT_EQ(num_3, *r0(out_of_range_t(), d, de, 1));
        ASSERT_EQ(num_4, *r0(out_of_range_t(), d, de, -1));
        ASSERT_EQ(num_5, *r0(out_of_range_t(), d, de, 0));
    }

    // from ignore
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(num_3, num_4, num_5, num_1, num_2);

        r_t r0(replacement_ignore, replacement_ignore, replacement_ignore,
               replacement_ignore, replacement_ignore);

        rs[0] = r_t(r0);
        rs[1] = r_t(r0);
        rs[2] = r_t(r0);
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            const auto& r = rs[i];
            ASSERT_TRUE(!r(empty_t())) << i;
            ASSERT_TRUE(!r(invalid_format_t(), d, de)) << i;
            ASSERT_TRUE(!r(out_of_range_t(), d, de, 1)) << i;
            ASSERT_TRUE(!r(out_of_range_t(), d, de, -1)) << i;
            ASSERT_TRUE(!r(out_of_range_t(), d, de, 0)) << i;
        }

        r0 = std::move(r0);
        ASSERT_TRUE(!r0(empty_t()));
        ASSERT_TRUE(!r0(invalid_format_t(), d, de));
        ASSERT_TRUE(!r0(out_of_range_t(), d, de, 1));
        ASSERT_TRUE(!r0(out_of_range_t(), d, de, -1));
        ASSERT_TRUE(!r0(out_of_range_t(), d, de, 0));
    }

    // from fail
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(num_3, num_4, num_5, num_1, num_2);

        r_t r0(replacement_fail, replacement_fail, replacement_fail,
               replacement_fail, replacement_fail);

        rs[0] = r_t(r0);
        rs[1] = r_t(r0);
        rs[2] = r_t(r0);
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            const auto& r = rs[i];
            ASSERT_THROW(r(empty_t()), field_empty) << i;
            ASSERT_THROW(r(invalid_format_t(), d, de), field_invalid_format) << i;
            ASSERT_THROW(r(out_of_range_t(), d, de, 1), field_out_of_range) << i;
            ASSERT_THROW(r(out_of_range_t(), d, de, -1), field_out_of_range) << i;
            ASSERT_THROW(r(out_of_range_t(), d, de, 0), field_out_of_range) << i;
        }

        r0 = std::move(r0);
        ASSERT_THROW(r0(empty_t()), field_empty);
        ASSERT_THROW(r0(invalid_format_t(), d, de), field_invalid_format);
        ASSERT_THROW(r0(out_of_range_t(), d, de, 1), field_out_of_range);
        ASSERT_THROW(r0(out_of_range_t(), d, de, -1), field_out_of_range);
        ASSERT_THROW(r0(out_of_range_t(), d, de, 0), field_out_of_range);
    }
}

TYPED_TEST(TestReplaceIfConversionFailed, Swap)
{
    using r_t = replace_if_conversion_failed<TypeParam>;

    char d[] = "dummy";
    char* de = d + sizeof d - 1;

    const auto from_str = [](const char* s) {
        std::stringstream str;
        str << s;
        TypeParam num;
        str >> num;
        return num;
    };

    TypeParam num_1 = from_str("10");
    TypeParam num_2 = from_str("15");
    TypeParam num_3 = from_str("-35");
    TypeParam num_4 = from_str("55");
    TypeParam num_5 = from_str("-90");

    std::vector<r_t> rs;
    rs.emplace_back(num_1, num_2, num_3, num_4, num_5);
    rs.emplace_back(replacement_ignore, replacement_ignore, replacement_ignore,
                    replacement_ignore, replacement_ignore);
    rs.emplace_back(replacement_fail, replacement_fail, replacement_fail,
                    replacement_fail, replacement_fail);
    rs.emplace_back(num_3, num_4, num_5, num_1, num_2);

    using std::swap;

    // copy vs ignore
    swap(rs[0], rs[1]);
    ASSERT_TRUE(!rs[0](empty_t()));
    ASSERT_TRUE(!rs[0](invalid_format_t(), d, de));
    ASSERT_TRUE(!rs[0](out_of_range_t(), d, de, 1));
    ASSERT_TRUE(!rs[0](out_of_range_t(), d, de, -1));
    ASSERT_TRUE(!rs[0](out_of_range_t(), d, de, 0));
    ASSERT_EQ(num_1, *rs[1](empty_t()));
    ASSERT_EQ(num_2, *rs[1](invalid_format_t(), d, de));
    ASSERT_EQ(num_3, *rs[1](out_of_range_t(), d, de, 1));
    ASSERT_EQ(num_4, *rs[1](out_of_range_t(), d, de, -1));
    ASSERT_EQ(num_5, *rs[1](out_of_range_t(), d, de, 0));
    swap(rs[0], rs[1]);
    ASSERT_EQ(num_1, *rs[0](empty_t()));
    ASSERT_EQ(num_2, *rs[0](invalid_format_t(), d, de));
    ASSERT_EQ(num_3, *rs[0](out_of_range_t(), d, de, 1));
    ASSERT_EQ(num_4, *rs[0](out_of_range_t(), d, de, -1));
    ASSERT_EQ(num_5, *rs[0](out_of_range_t(), d, de, 0));
    ASSERT_TRUE(!rs[1](empty_t()));
    ASSERT_TRUE(!rs[1](invalid_format_t(), d, de));
    ASSERT_TRUE(!rs[1](out_of_range_t(), d, de, 1));
    ASSERT_TRUE(!rs[1](out_of_range_t(), d, de, -1));
    ASSERT_TRUE(!rs[1](out_of_range_t(), d, de, 0));

    // ignore vs fail
    swap(rs[1], rs[2]);
    ASSERT_THROW(rs[1](empty_t()), field_empty);
    ASSERT_THROW(rs[1](invalid_format_t(), d, de), field_invalid_format);
    ASSERT_THROW(rs[1](out_of_range_t(), d, de, 1), field_out_of_range);
    ASSERT_THROW(rs[1](out_of_range_t(), d, de, -1), field_out_of_range);
    ASSERT_THROW(rs[1](out_of_range_t(), d, de, 0), field_out_of_range);
    ASSERT_TRUE(!rs[2](empty_t()));
    ASSERT_TRUE(!rs[2](invalid_format_t(), d, de));
    ASSERT_TRUE(!rs[2](out_of_range_t(), d, de, 1));
    ASSERT_TRUE(!rs[2](out_of_range_t(), d, de, -1));
    ASSERT_TRUE(!rs[2](out_of_range_t(), d, de, 0));
    swap(rs[1], rs[2]);
    ASSERT_TRUE(!rs[1](empty_t()));
    ASSERT_TRUE(!rs[1](invalid_format_t(), d, de));
    ASSERT_TRUE(!rs[1](out_of_range_t(), d, de, 1));
    ASSERT_TRUE(!rs[1](out_of_range_t(), d, de, -1));
    ASSERT_TRUE(!rs[1](out_of_range_t(), d, de, 0));
    ASSERT_THROW(rs[2](empty_t()), field_empty);
    ASSERT_THROW(rs[2](invalid_format_t(), d, de), field_invalid_format);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, 1), field_out_of_range);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, -1), field_out_of_range);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, 0), field_out_of_range);

    // fail vs copy
    swap(rs[2], rs[3]);
    ASSERT_EQ(num_3, *rs[2](empty_t()));
    ASSERT_EQ(num_4, *rs[2](invalid_format_t(), d, de));
    ASSERT_EQ(num_5, *rs[2](out_of_range_t(), d, de, 1));
    ASSERT_EQ(num_1, *rs[2](out_of_range_t(), d, de, -1));
    ASSERT_EQ(num_2, *rs[2](out_of_range_t(), d, de, 0));
    ASSERT_THROW(rs[3](empty_t()), field_empty);
    ASSERT_THROW(rs[3](invalid_format_t(), d, de), field_invalid_format);
    ASSERT_THROW(rs[3](out_of_range_t(), d, de, 1), field_out_of_range);
    ASSERT_THROW(rs[3](out_of_range_t(), d, de, -1), field_out_of_range);
    ASSERT_THROW(rs[3](out_of_range_t(), d, de, 0), field_out_of_range);
    swap(rs[2], rs[3]);
    ASSERT_THROW(rs[2](empty_t()), field_empty);
    ASSERT_THROW(rs[2](invalid_format_t(), d, de), field_invalid_format);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, 1), field_out_of_range);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, -1), field_out_of_range);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, 0), field_out_of_range);
    ASSERT_EQ(num_3, *rs[3](empty_t()));
    ASSERT_EQ(num_4, *rs[3](invalid_format_t(), d, de));
    ASSERT_EQ(num_5, *rs[3](out_of_range_t(), d, de, 1));
    ASSERT_EQ(num_1, *rs[3](out_of_range_t(), d, de, -1));
    ASSERT_EQ(num_2, *rs[3](out_of_range_t(), d, de, 0));

    // copy vs copy
    swap(rs[3], rs[0]);
    ASSERT_EQ(num_1, *rs[3](empty_t()));
    ASSERT_EQ(num_2, *rs[3](invalid_format_t(), d, de));
    ASSERT_EQ(num_3, *rs[3](out_of_range_t(), d, de, 1));
    ASSERT_EQ(num_4, *rs[3](out_of_range_t(), d, de, -1));
    ASSERT_EQ(num_5, *rs[3](out_of_range_t(), d, de, 0));
    ASSERT_EQ(num_3, *rs[0](empty_t()));
    ASSERT_EQ(num_4, *rs[0](invalid_format_t(), d, de));
    ASSERT_EQ(num_5, *rs[0](out_of_range_t(), d, de, 1));
    ASSERT_EQ(num_1, *rs[0](out_of_range_t(), d, de, -1));
    ASSERT_EQ(num_2, *rs[0](out_of_range_t(), d, de, 0));
    swap(rs[3], rs[0]);
    ASSERT_EQ(num_3, *rs[3](empty_t()));
    ASSERT_EQ(num_4, *rs[3](invalid_format_t(), d, de));
    ASSERT_EQ(num_5, *rs[3](out_of_range_t(), d, de, 1));
    ASSERT_EQ(num_1, *rs[3](out_of_range_t(), d, de, -1));
    ASSERT_EQ(num_2, *rs[3](out_of_range_t(), d, de, 0));
    ASSERT_EQ(num_1, *rs[0](empty_t()));
    ASSERT_EQ(num_2, *rs[0](invalid_format_t(), d, de));
    ASSERT_EQ(num_3, *rs[0](out_of_range_t(), d, de, 1));
    ASSERT_EQ(num_4, *rs[0](out_of_range_t(), d, de, -1));
    ASSERT_EQ(num_5, *rs[0](out_of_range_t(), d, de, 0));

    // swap with self
    swap(rs[0], rs[0]);
    ASSERT_EQ(num_1, *rs[0](empty_t()));
    ASSERT_EQ(num_2, *rs[0](invalid_format_t(), d, de));
    ASSERT_EQ(num_3, *rs[0](out_of_range_t(), d, de, 1));
    ASSERT_EQ(num_4, *rs[0](out_of_range_t(), d, de, -1));
    ASSERT_EQ(num_5, *rs[0](out_of_range_t(), d, de, 0));
    swap(rs[1], rs[1]);
    ASSERT_TRUE(!rs[1](empty_t()));
    ASSERT_TRUE(!rs[1](invalid_format_t(), d, de));
    ASSERT_TRUE(!rs[1](out_of_range_t(), d, de, 1));
    ASSERT_TRUE(!rs[1](out_of_range_t(), d, de, -1));
    ASSERT_TRUE(!rs[1](out_of_range_t(), d, de, 0));
    swap(rs[2], rs[2]);
    ASSERT_THROW(rs[2](empty_t()), field_empty);
    ASSERT_THROW(rs[2](invalid_format_t(), d, de), field_invalid_format);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, 1), field_out_of_range);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, -1), field_out_of_range);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, 0), field_out_of_range);
}

namespace replace_if_conversion_failed_static_asserts {

struct a_t : std::allocator<int>
{
    using propagate_on_container_swap = std::true_type;
};

using ri_t = replace_if_conversion_failed<int>;
using rv_t = replace_if_conversion_failed<std::vector<int, a_t>>;
using std::swap;

static_assert(std::is_nothrow_copy_constructible<ri_t>::value, "");
static_assert(std::is_nothrow_move_constructible<ri_t>::value, "");
static_assert(std::is_nothrow_copy_assignable<ri_t>::value, "");
static_assert(std::is_nothrow_move_assignable<ri_t>::value, "");
static_assert(noexcept(
    swap(std::declval<ri_t&>(), std::declval<ri_t&>())), "");

static_assert(!std::is_nothrow_copy_constructible<rv_t>::value, "");
static_assert(std::is_nothrow_move_constructible<rv_t>::value, "");
static_assert(!std::is_nothrow_copy_assignable<rv_t>::value, "");
static_assert(std::is_nothrow_move_assignable<rv_t>::value, "");
static_assert(noexcept(
    swap(std::declval<rv_t&>(), std::declval<rv_t&>())), "");

}
