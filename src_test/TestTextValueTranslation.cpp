/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <iostream>
#include <iterator>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <commata/text_value_translation.hpp>

#include "BaseTest.hpp"

using namespace std::string_literals;

using namespace commata;
using namespace commata::test;

namespace {

using Chs = testing::Types<char, wchar_t>;

template <class Ch>
struct digit;

template <>
struct digit<char>
{
    static constexpr char ch0 = '0';
    static constexpr char ch1 = '1';
    static constexpr char ch2 = '2';
    static constexpr char ch3 = '3';
    static constexpr char ch4 = '4';
    static constexpr char ch5 = '5';
    static constexpr char ch6 = '6';
    static constexpr char ch7 = '7';
    static constexpr char ch8 = '8';
    static constexpr char ch9 = '9';
};

template <>
struct digit<wchar_t>
{
    static constexpr wchar_t ch0 = L'0';
    static constexpr wchar_t ch1 = L'1';
    static constexpr wchar_t ch2 = L'2';
    static constexpr wchar_t ch3 = L'3';
    static constexpr wchar_t ch4 = L'4';
    static constexpr wchar_t ch5 = L'5';
    static constexpr wchar_t ch6 = L'6';
    static constexpr wchar_t ch7 = L'7';
    static constexpr wchar_t ch8 = L'8';
    static constexpr wchar_t ch9 = L'9';
};

template <class Ch>
struct digits
{
    // With std::array, it seems we cannot make the compiler count the number
    // of the elements, so we employ good old arrays
    static constexpr Ch all[] = {
        digit<Ch>::ch0,
        digit<Ch>::ch1,
        digit<Ch>::ch2,
        digit<Ch>::ch3,
        digit<Ch>::ch4,
        digit<Ch>::ch5,
        digit<Ch>::ch6,
        digit<Ch>::ch7,
        digit<Ch>::ch8,
        digit<Ch>::ch9
    };

    static constexpr bool is_sorted = all[0] <= all[1]
                                   && all[1] <= all[2]
                                   && all[2] <= all[3]
                                   && all[3] <= all[4]
                                   && all[4] <= all[5]
                                   && all[5] <= all[6]
                                   && all[6] <= all[7]
                                   && all[7] <= all[8]
                                   && all[8] <= all[9];
};

template <class Ch>
std::basic_string<Ch> plus1(std::basic_string<Ch> str, std::size_t i
    = static_cast<std::size_t>(-1))
{
    auto s = std::move(str);

    i = std::min(i, static_cast<std::size_t>(s.size() - 1));

    const Ch (&all)[std::size(digits<Ch>::all)] = digits<Ch>::all;
    const auto all_end = all + std::size(digits<Ch>::all);

    for (;;) {
        const Ch* k;
        if constexpr (digits<Ch>::is_sorted) {
            k = std::lower_bound(all, all_end, s[i]);
            if ((k != all_end) && (*k != s[i])) {
                k = all_end;
            }
        } else {
            k = std::find(all, all_end, s[i]);
        }
        if (k == all_end - 1) {
            s[i] = all[0];  // carrying occurs
            if (i == 0) {
                s.insert(s.begin(), all[1]);  // gcc 7.3.1 refuses s.cbegin()
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

using ChIntegrals = testing::Types<
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
>;

using ChFloatingPoints = testing::Types<
    std::pair<char, float>,
    std::pair<char, double>,
    std::pair<char, long double>,
    std::pair<wchar_t, float>,
    std::pair<wchar_t, double>,
    std::pair<wchar_t, long double>
>;

} // end unnamed

template <class ChNum>
struct TestToArithmeticIntegrals : BaseTest
{};

TYPED_TEST_SUITE(TestToArithmeticIntegrals, ChIntegrals, );

TYPED_TEST(TestToArithmeticIntegrals, Correct)
{
    using char_t = typename TypeParam::first_type;
    using value_t = typename TypeParam::second_type;
    using opt_t = std::optional<value_t>;

    const auto str = char_helper<char_t>::str;

    ASSERT_EQ(value_t(40), to_arithmetic<value_t>(str(" 40")));
    ASSERT_EQ(value_t(63), to_arithmetic<value_t>(str("63")));
    ASSERT_EQ(value_t(-10), to_arithmetic<value_t>(str("-10 ")));
    ASSERT_EQ(opt_t(value_t(100)), to_arithmetic<opt_t>(str("100")));
}

TYPED_TEST(TestToArithmeticIntegrals, UpperLimit)
{
    using char_t = typename TypeParam::first_type;
    using value_t = typename TypeParam::second_type;
    using string_t = std::basic_string<char_t>;
    using stringstream_t = std::basic_stringstream<char_t>;
    using opt_t = std::optional<value_t>;

    const auto to_string =
        [](auto t) { return char_helper<char_t>::to_string(t); };
    const auto widen = char_helper<char_t>::template widen<const char*>;

    constexpr auto maxx = std::numeric_limits<value_t>::max();
    const auto maxx_plus1 = plus1(to_string(maxx + 0));
        // "+ 0" is to prevent a single (w)char(_t) from being written

    // maxx
    {
        stringstream_t s;
        s << (maxx + 0);
        ASSERT_EQ(maxx, to_arithmetic<value_t>(s.str()));
    }

    // maxx_plus1
    try {
        to_arithmetic<value_t>(maxx_plus1);
        FAIL();
    } catch (const text_value_out_of_range& e) {
        const string_t message = widen(e.what());
        ASSERT_TRUE(message.find(maxx_plus1) != string_t::npos) << message;
    }
    ASSERT_FALSE(to_arithmetic<opt_t>(maxx_plus1).has_value());
}

 TYPED_TEST(TestToArithmeticIntegrals, LowerLimit)
 {
     using char_t = typename TypeParam::first_type;
     using value_t = typename TypeParam::second_type;
     using string_t = std::basic_string<char_t>;
    using opt_t = std::optional<value_t>;

     const auto ch = char_helper<char_t>::ch;
     const auto to_string =
         [](auto t) { return char_helper<char_t>::to_string(t); };
     const auto widen = char_helper<char_t>::template widen<const char*>;

     string_t minn;
     string_t minn_minus1;
     if constexpr (std::is_signed_v<value_t>) {
         minn = to_string(std::numeric_limits<value_t>::min() + 0);
         minn_minus1 = ch('-') + plus1(minn.substr(1));
     } else {
         minn = ch('-') + to_string(std::numeric_limits<value_t>::max() + 0);
         minn_minus1 = ch('-') + plus1(plus1(minn.substr(1)));
     }
     constexpr auto maxx = std::numeric_limits<value_t>::max();
     const auto maxxPlus1 = plus1(to_string(maxx + 0));

    // minn
    ASSERT_NO_THROW(to_arithmetic<value_t>(minn));

    // minn_minus1
    try {
        to_arithmetic<value_t>(minn_minus1);
        FAIL();
    } catch (const text_value_out_of_range& e) {
        const string_t message = widen(e.what());
        ASSERT_TRUE(message.find(minn_minus1) != string_t::npos) << message;
    }
    ASSERT_FALSE(to_arithmetic<opt_t>(minn_minus1).has_value());
}

TYPED_TEST(TestToArithmeticIntegrals, Replacement)
{
    using char_t = typename TypeParam::first_type;
    using value_t = typename TypeParam::second_type;
    using string_t = std::basic_string<char_t>;
    using opt_t = std::optional<value_t>;

    const auto ch = char_helper<char_t>::ch;
    const auto str = char_helper<char_t>::str;
    const auto to_string =
        [](auto t) { return char_helper<char_t>::to_string(t); };

    string_t minn;
    string_t minn_minus1;
    if constexpr (std::is_signed_v<value_t>) {
        minn = to_string(std::numeric_limits<value_t>::min() + 0);
        minn_minus1 = ch('-') + plus1(minn.substr(1));
    } else {
        minn = ch('-') + to_string(std::numeric_limits<value_t>::max() + 0);
        minn_minus1 = ch('-') + plus1(plus1(minn.substr(1)));
    }
    constexpr auto maxx = std::numeric_limits<value_t>::max();
    const auto maxx_plus1 = plus1(to_string(maxx + 0));

    {
        replace_if_conversion_failed<value_t> h(value_t(34));
        ASSERT_NO_THROW(to_arithmetic<value_t>(str("-5"), h));
        ASSERT_EQ(value_t(34), to_arithmetic<value_t>(str(""), h));
        ASSERT_EQ(opt_t(value_t(34)), to_arithmetic<opt_t>(str(""), h));
    }
    {
        replace_if_conversion_failed<value_t> h(replacement_fail, value_t(42));
        ASSERT_EQ(value_t(42), to_arithmetic<value_t>(str("x"), h));
        ASSERT_THROW(to_arithmetic<value_t>(str(""), h), text_value_empty);
        ASSERT_THROW(to_arithmetic<opt_t>(str(""), h), text_value_empty);
    }
    if constexpr (std::is_signed_v<value_t>) {
        replace_if_conversion_failed<value_t> h(
            replacement_fail, replacement_fail, value_t(1), value_t(0));
        ASSERT_EQ(value_t(1), to_arithmetic<value_t>(maxx_plus1, h));
        ASSERT_EQ(value_t(0), to_arithmetic<value_t>(minn_minus1, h));
    } else {
        replace_if_conversion_failed<value_t> h(
            replacement_fail, replacement_fail, value_t(1));
        ASSERT_EQ(value_t(1), to_arithmetic<value_t>(maxx_plus1, h));
        ASSERT_EQ(value_t(1), to_arithmetic<value_t>(minn_minus1, h));
    }
}

struct TestToArithmeticIntegralsRestricted : BaseTest
{};

TEST_F(TestToArithmeticIntegralsRestricted, Unsigned)
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

    replace_if_conversion_failed<unsigned short> h(
        static_cast<unsigned short>(3),     // empty
        static_cast<unsigned short>(4),     // invalid
        static_cast<unsigned short>(2));    // above max

    const auto f = [&h](const std::string& s) {
            return to_arithmetic<unsigned short>(s, h);
        };
    ASSERT_EQ(std::numeric_limits<unsigned short>::max(), f(max));
    ASSERT_EQ(2, f(maxp1));
    ASSERT_EQ(1, f('-' + max));     // wrapped around
    ASSERT_EQ(2, f('-' + maxp1));
}

template <class ChNum>
struct TestToArithmeticFloatingPoints : BaseTest
{};

TYPED_TEST_SUITE(TestToArithmeticFloatingPoints, ChFloatingPoints, );

TYPED_TEST(TestToArithmeticFloatingPoints, Correct)
{
    using char_t = typename TypeParam::first_type;
    using value_t = typename TypeParam::second_type;
    using string_t = std::basic_string<char_t>;
    using opt_t = std::optional<value_t>;

    const auto str = char_helper<char_t>::str;

    const std::vector<string_t> sources = { str("6.02e23\t"), str(" -5") };
    const std::vector<string_t> expressions = { str("6.02e23"), str("-5") };
    for (std::size_t i = 0, ie = sources.size(); i < ie; ++i) {
        std::basic_stringstream<char_t> ss;
        ss << expressions[i];
        value_t value;
        ss >> value;
        ASSERT_EQ(value, to_arithmetic<value_t>(sources[i]));
        ASSERT_EQ(opt_t(value), to_arithmetic<opt_t>(sources[i]));
    }
}

TYPED_TEST(TestToArithmeticFloatingPoints, UpperLimit)
{
    using char_t = typename TypeParam::first_type;
    using value_t = typename TypeParam::second_type;
    using string_t = std::basic_string<char_t>;
    using stringstream_t = std::basic_stringstream<char_t>;
    using opt_t = std::optional<value_t>;

    const auto widen = char_helper<char_t>::template widen<const char*>;

    constexpr auto maxx = std::numeric_limits<value_t>::max();
    if (!std::isdigit(static_cast<unsigned char>(std::to_string(maxx)[0]))) {
        // There is a pathological envirionment where maxx is not finite and,
        // in addition, "if (!std::isfinite(maxx))" does not do correct
        // branching. Sigh...
        std::cerr << "Pathological environment. Skipping this test."
                  << std::endl;
        return;
    }

    string_t maxx_by_10;
    {
        stringstream_t ss;
        ss << std::scientific << std::setprecision(50) << maxx << '0';
        maxx_by_10 = std::move(ss).str();
    }

    // maxx
    {
        stringstream_t ss;
        ss << std::scientific << std::setprecision(50) << maxx;
        ASSERT_EQ(maxx, to_arithmetic<value_t>(std::move(ss).str()));
    }

    // maxx_by_10
    try {
        to_arithmetic<value_t>(maxx_by_10);
    } catch (const text_value_out_of_range& e) {
        const string_t message = widen(e.what());
        ASSERT_TRUE(message.find(maxx_by_10) != string_t::npos) << message;
    }
    ASSERT_FALSE(to_arithmetic<opt_t>(maxx_by_10).has_value());
}

TYPED_TEST(TestToArithmeticFloatingPoints, LowerLimit)
{
    using char_t = typename TypeParam::first_type;
    using value_t = typename TypeParam::second_type;
    using string_t = std::basic_string<char_t>;
    using stringstream_t = std::basic_stringstream<char_t>;
    using opt_t = std::optional<value_t>;

    const auto widen = char_helper<char_t>::template widen<const char*>;

    constexpr auto minn = std::numeric_limits<value_t>::lowest();
    if (!std::isdigit(static_cast<unsigned char>(std::to_string(minn)[1]))) {
        std::cerr << "Pathological environment. Skipping this test."
                  << std::endl;
        return;
    }

    string_t minn_by_10;
    {
        stringstream_t ss;
        ss << std::scientific << std::setprecision(50) << minn << '0';
        minn_by_10 = std::move(ss).str();
    }

    // minn
    {
        stringstream_t ss;
        ss << std::scientific << std::setprecision(50) << minn;
        ASSERT_EQ(minn, to_arithmetic<value_t>(std::move(ss).str()));
    }

    // minn_by_10
    try {
        to_arithmetic<value_t>(minn_by_10);
    } catch (const text_value_out_of_range& e) {
        const string_t message = widen(e.what());
        ASSERT_TRUE(message.find(minn_by_10) != string_t::npos) << message;
    }
    ASSERT_FALSE(to_arithmetic<opt_t>(minn_by_10).has_value());
}

struct TestToArithmeticMiscellaneous : BaseTest
{};

TEST_F(TestToArithmeticMiscellaneous, Const)
{
    ASSERT_EQ(42, to_arithmetic<const int>("42"s));
    try {
        to_arithmetic<const int>("42x"s);
        FAIL();
    } catch (const text_value_invalid_format& e) {
        ASSERT_NE(std::string_view(e.what()).find("int"),
                  std::string_view::npos) << e.what();
    }
    ASSERT_EQ(55, to_arithmetic<int>(""s,
        replace_if_conversion_failed<const int>(55)));
}

namespace {

struct rvalue_handler
{
    int operator()(invalid_format_t) &
    {
        return 1;
    }

    int operator()(invalid_format_t) &&
    {
        return 100;
    }

    int operator()(out_of_range_t) &
    {
        return 2;
    }

    int operator()(out_of_range_t) &&
    {
        return 200;
    }

    int operator()(empty_t) &
    {
        return 3;
    }

    std::optional<int> operator()(empty_t) &&
    {
        return 300;
    }
};

}

TEST_F(TestToArithmeticMiscellaneous, Rvalue)
{
    ASSERT_EQ(100, to_arithmetic<int>("42x"s, rvalue_handler()));
}

TEST_F(TestToArithmeticMiscellaneous, ReferenceWrapper)
{
    rvalue_handler h;
    ASSERT_EQ(1, to_arithmetic<int>("42x"s, std::ref(h)));
}

template <class T>
struct TestNumPunctReplacerToC : BaseTestWithParam<T>
{};

TYPED_TEST_SUITE(TestNumPunctReplacerToC, Chs, );

TYPED_TEST(TestNumPunctReplacerToC, Usual)
{
    using char_t = TypeParam;
    using string_t = std::basic_string<char_t>;

    const auto str = char_helper<char_t>::str;

    numpunct_replacer_to_c engine(std::locale(std::locale::classic(),
        new french_style_numpunct<char_t>));

    // in place
    {
        string_t s = str("-98 765,25");
        s.erase(engine(s.begin(), s.end()), s.end());
        const double d = std::stod(s);
        ASSERT_EQ(-98765.25, d) << s;
    }

    // copy
    {
        string_t s = str("-98 765,25");
        string_t t;
        engine(s.cbegin(), s.cend(), std::back_inserter(t));
        const double d = std::stod(t);
        ASSERT_EQ(-98765.25, d) << t;
    }
}

TYPED_TEST(TestNumPunctReplacerToC, Nop)
{
    using char_t = TypeParam;
    using string_t = std::basic_string<char_t>;

    const auto str = char_helper<char_t>::str;

    numpunct_replacer_to_c engine(std::locale(std::locale::classic(),
        new french_style_numpunct<char_t>));

    // in place
    {
        string_t s = str("12345");
        s.erase(engine(s.begin(), s.end()), s.end());
        const double d = std::stod(s);
        ASSERT_EQ(12345.0, d) << s;
    }

    // copy
    {
        string_t s = str("12345");
        string_t t;
        engine(s.cbegin(), s.cend(), std::back_inserter(t));
        const double d = std::stod(t);
        ASSERT_EQ(12345.0, d) << t;
    }
}
