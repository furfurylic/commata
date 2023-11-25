/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <deque>
#include <iomanip>
#include <ios>
#include <iostream>
#include <iterator>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <commata/field_scanners.hpp>

#include "BaseTest.hpp"

using namespace std::literals::string_literals;

using namespace commata;
using namespace commata::test;

namespace {

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

class from_str
{
    std::stringstream str_;

public:
    from_str(const char* s)
    {
        str_ << s;
    }

    template <class T>
    operator T() && noexcept
    {
        T num;
        str_ >> num;
        return num;
    }
};

} // end unnamed

template <class T>
struct TestReplaceIfConversionFailed : BaseTestWithParam<T>
{};

typedef testing::Types<double, std::string> ReplacedTypes;

TYPED_TEST_SUITE(TestReplaceIfConversionFailed, ReplacedTypes);

TYPED_TEST(TestReplaceIfConversionFailed, WithOneArgCtor)
{
    using r_t = replace_if_conversion_failed<TypeParam>;

    char d[] = "dummy";
    char* de = d + sizeof d - 1;

    TypeParam num_1 = from_str("10");

    std::deque<r_t> rs;
    /*0*/rs.emplace_back(num_1);
    /*1*/rs.emplace_back(replacement_ignore);
    /*2*/rs.emplace_back(replacement_fail);

    ASSERT_EQ(num_1, rs[0](empty_t()));
    ASSERT_EQ(num_1, rs[0](invalid_format_t(), d, de));
    ASSERT_EQ(num_1, rs[0](out_of_range_t(), d, de, 1));
    ASSERT_EQ(num_1, rs[0](out_of_range_t(), d, de, -1));
    ASSERT_EQ(num_1, rs[0](out_of_range_t(), d, de, 0));

    ASSERT_TRUE(!rs[1](empty_t()));
    ASSERT_TRUE(!rs[1](invalid_format_t(), d, de));
    ASSERT_TRUE(!rs[1](out_of_range_t(), d, de, 1));
    ASSERT_TRUE(!rs[1](out_of_range_t(), d, de, -1));
    ASSERT_TRUE(!rs[1](out_of_range_t(), d, de, 0));

    ASSERT_THROW(rs[2](empty_t()), text_value_empty);
    ASSERT_THROW(rs[2](invalid_format_t(), d, de), text_value_invalid_format);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, 1), text_value_out_of_range);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, -1), text_value_out_of_range);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, 0), text_value_out_of_range);
}

TYPED_TEST(TestReplaceIfConversionFailed, WithTwoArgCtor)
{
    using r_t = replace_if_conversion_failed<TypeParam>;

    char d[] = "dummy";
    char* de = d + sizeof d - 1;

    TypeParam num_1 = from_str("10");

    const r_t r(replacement_ignore, num_1);

    ASSERT_TRUE(!r(empty_t()));
    ASSERT_EQ(num_1, r(invalid_format_t(), d, de));
    ASSERT_EQ(num_1, r(out_of_range_t(), d, de, 1));
    ASSERT_EQ(num_1, r(out_of_range_t(), d, de, -1));
    ASSERT_EQ(num_1, r(out_of_range_t(), d, de, 0));
}

TYPED_TEST(TestReplaceIfConversionFailed, WithThreeArgCtor)
{
    using r_t = replace_if_conversion_failed<TypeParam>;

    char d[] = "dummy";
    char* de = d + sizeof d - 1;

    TypeParam num_1 = from_str("10");

    const r_t r(replacement_fail, replacement_ignore, num_1);

    ASSERT_THROW(r(empty_t()), text_value_empty);
    ASSERT_TRUE(!r(invalid_format_t(), d, de));
    ASSERT_EQ(num_1, r(out_of_range_t(), d, de, 1));
    ASSERT_EQ(num_1, r(out_of_range_t(), d, de, -1));
    ASSERT_EQ(num_1, r(out_of_range_t(), d, de, 0));
}

TYPED_TEST(TestReplaceIfConversionFailed, CtorsCopy)
{
    using r_t = replace_if_conversion_failed<TypeParam>;

    char d[] = "dummy";
    char* de = d + sizeof d - 1;

    TypeParam num_1 = from_str("10");
    TypeParam num_2 = from_str("15");
    TypeParam num_3 = from_str("-35");
    TypeParam num_4 = from_str("55");

    std::deque<r_t> rs;
    /*0*/rs.emplace_back(num_1, num_2, num_3, num_4, replacement_ignore);
    /*1*/rs.emplace_back(rs[0]);
    /*2*/rs.emplace_back(std::move(r_t(rs[0])));

    for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
        const auto& r = rs[i];
        ASSERT_EQ(num_1, *r(empty_t())) << i;
        ASSERT_EQ(num_2, *r(invalid_format_t(), d, de)) << i;
        ASSERT_EQ(num_3, *r(out_of_range_t(), d, de, 1)) << i;
        ASSERT_EQ(num_4, *r(out_of_range_t(), d, de, -1)) << i;
        ASSERT_TRUE(!r(out_of_range_t(), d, de, 0)) << i;
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
        ASSERT_THROW(r(empty_t()), text_value_empty);
        ASSERT_THROW(r(invalid_format_t(), d, de),
                     text_value_invalid_format) << i;
        ASSERT_THROW(r(out_of_range_t(), d, de, 1),
                     text_value_out_of_range) << i;
        ASSERT_THROW(r(out_of_range_t(), d, de, -1),
                     text_value_out_of_range) << i;
        ASSERT_THROW(r(out_of_range_t(), d, de, 0),
                     text_value_out_of_range) << i;
    }
}

TYPED_TEST(TestReplaceIfConversionFailed, CopyAssign)
{
    using r_t = replace_if_conversion_failed<TypeParam>;

    char d[] = "dummy";
    char* de = d + sizeof d - 1;

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

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
        r0 = r0;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
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

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
        r0 = r0;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
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
            ASSERT_THROW(r(empty_t()), text_value_empty) << i;
            ASSERT_THROW(r(invalid_format_t(), d, de),
                         text_value_invalid_format) << i;
            ASSERT_THROW(r(out_of_range_t(), d, de, 1),
                         text_value_out_of_range) << i;
            ASSERT_THROW(r(out_of_range_t(), d, de, -1),
                         text_value_out_of_range) << i;
            ASSERT_THROW(r(out_of_range_t(), d, de, 0),
                         text_value_out_of_range) << i;
        }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
        r0 = r0;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
        ASSERT_THROW(r0(empty_t()), text_value_empty);
        ASSERT_THROW(r0(invalid_format_t(), d, de), text_value_invalid_format);
        ASSERT_THROW(r0(out_of_range_t(), d, de, 1), text_value_out_of_range);
        ASSERT_THROW(r0(out_of_range_t(), d, de, -1), text_value_out_of_range);
        ASSERT_THROW(r0(out_of_range_t(), d, de, 0), text_value_out_of_range);
    }
}

TYPED_TEST(TestReplaceIfConversionFailed, MoveAssign)
{
    using r_t = replace_if_conversion_failed<TypeParam>;

    char d[] = "dummy";
    char* de = d + sizeof d - 1;

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
            ASSERT_THROW(r(empty_t()), text_value_empty) << i;
            ASSERT_THROW(r(invalid_format_t(), d, de),
                         text_value_invalid_format) << i;
            ASSERT_THROW(r(out_of_range_t(), d, de, 1),
                         text_value_out_of_range) << i;
            ASSERT_THROW(r(out_of_range_t(), d, de, -1),
                         text_value_out_of_range) << i;
            ASSERT_THROW(r(out_of_range_t(), d, de, 0),
                         text_value_out_of_range) << i;
        }

        r0 = std::move(r0);
        ASSERT_THROW(r0(empty_t()), text_value_empty);
        ASSERT_THROW(r0(invalid_format_t(), d, de), text_value_invalid_format);
        ASSERT_THROW(r0(out_of_range_t(), d, de, 1), text_value_out_of_range);
        ASSERT_THROW(r0(out_of_range_t(), d, de, -1), text_value_out_of_range);
        ASSERT_THROW(r0(out_of_range_t(), d, de, 0), text_value_out_of_range);
    }
}

TYPED_TEST(TestReplaceIfConversionFailed, Swap)
{
    using r_t = replace_if_conversion_failed<TypeParam>;

    char d[] = "dummy";
    char* de = d + sizeof d - 1;

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
    ASSERT_THROW(rs[1](empty_t()), text_value_empty);
    ASSERT_THROW(rs[1](invalid_format_t(), d, de), text_value_invalid_format);
    ASSERT_THROW(rs[1](out_of_range_t(), d, de, 1), text_value_out_of_range);
    ASSERT_THROW(rs[1](out_of_range_t(), d, de, -1), text_value_out_of_range);
    ASSERT_THROW(rs[1](out_of_range_t(), d, de, 0), text_value_out_of_range);
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
    ASSERT_THROW(rs[2](empty_t()), text_value_empty);
    ASSERT_THROW(rs[2](invalid_format_t(), d, de), text_value_invalid_format);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, 1), text_value_out_of_range);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, -1), text_value_out_of_range);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, 0), text_value_out_of_range);

    // fail vs copy
    swap(rs[2], rs[3]);
    ASSERT_EQ(num_3, *rs[2](empty_t()));
    ASSERT_EQ(num_4, *rs[2](invalid_format_t(), d, de));
    ASSERT_EQ(num_5, *rs[2](out_of_range_t(), d, de, 1));
    ASSERT_EQ(num_1, *rs[2](out_of_range_t(), d, de, -1));
    ASSERT_EQ(num_2, *rs[2](out_of_range_t(), d, de, 0));
    ASSERT_THROW(rs[3](empty_t()), text_value_empty);
    ASSERT_THROW(rs[3](invalid_format_t(), d, de), text_value_invalid_format);
    ASSERT_THROW(rs[3](out_of_range_t(), d, de, 1), text_value_out_of_range);
    ASSERT_THROW(rs[3](out_of_range_t(), d, de, -1), text_value_out_of_range);
    ASSERT_THROW(rs[3](out_of_range_t(), d, de, 0), text_value_out_of_range);
    swap(rs[2], rs[3]);
    ASSERT_THROW(rs[2](empty_t()), text_value_empty);
    ASSERT_THROW(rs[2](invalid_format_t(), d, de), text_value_invalid_format);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, 1), text_value_out_of_range);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, -1), text_value_out_of_range);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, 0), text_value_out_of_range);
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
    ASSERT_THROW(rs[2](empty_t()), text_value_empty);
    ASSERT_THROW(rs[2](invalid_format_t(), d, de), text_value_invalid_format);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, 1), text_value_out_of_range);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, -1), text_value_out_of_range);
    ASSERT_THROW(rs[2](out_of_range_t(), d, de, 0), text_value_out_of_range);
}

TYPED_TEST(TestReplaceIfConversionFailed, DeductionGuides)
{
    const auto from_str = [](const char* s) {
        std::stringstream str;
        str << s;
        TypeParam num;
        str >> num;
        return num;
    };

    TypeParam num_1 = from_str("10");
    TypeParam num_2 = from_str("-0.5");

    const char s[] = "";

    replace_if_conversion_failed r(num_1, TypeParam(),
            replacement_fail, replacement_ignore, num_2);
    static_assert(
        std::is_same_v<replace_if_conversion_failed<TypeParam>, decltype(r)>);
    ASSERT_EQ(num_1, *r(empty_t()));
    ASSERT_EQ(TypeParam(), *r(invalid_format_t(), s, s));
    ASSERT_THROW(r(out_of_range_t(), s, s, 1), text_value_out_of_range);
    ASSERT_TRUE(!r(out_of_range_t(), s, s, -1));
    ASSERT_EQ(num_2, *r(out_of_range_t(), s, s, 0));

    replace_if_conversion_failed r2(replacement_fail, 0, 10l,
                                    replacement_ignore);
    static_assert(std::is_same_v<replace_if_conversion_failed<long>,
                  decltype(r2)>);

    // The code below should not compile by a "no viable deduction guide" error
    // replace_if_conversion_failed r3(replacement_fail, 0, std::string("XY"));
}

namespace replace_if_conversion_failed_static_asserts {

using ri_t = replace_if_conversion_failed<int>;
using rv_t = replace_if_conversion_failed<std::vector<int>>;

static_assert(std::is_nothrow_copy_constructible_v<ri_t>);
static_assert(std::is_nothrow_move_constructible_v<ri_t>);
static_assert(std::is_nothrow_copy_assignable_v<ri_t>);
static_assert(std::is_nothrow_move_assignable_v<ri_t>);
static_assert(std::is_nothrow_swappable_v<ri_t>);

static_assert(!std::is_nothrow_copy_constructible_v<rv_t>);
static_assert(std::is_nothrow_move_constructible_v<rv_t>);
static_assert(!std::is_nothrow_copy_assignable_v<rv_t>);
static_assert(std::is_nothrow_move_assignable_v<rv_t>);
static_assert(std::is_nothrow_swappable_v<rv_t>);

static_assert(std::is_trivially_copyable<ri_t>::value);

static_assert(!std::is_constructible<replace_if_conversion_failed<int>,
    int, int, int, int, int>::value);
static_assert(!std::is_constructible<replace_if_conversion_failed<unsigned>,
    unsigned, replacement_fail_t, replacement_ignore_t, long>::value);

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
struct TestToArithmeticIntegrals : BaseTest
{};

TYPED_TEST_SUITE(TestToArithmeticIntegrals, ChIntegrals);

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

TYPED_TEST_SUITE(TestToArithmeticFloatingPoints, ChFloatingPoints);

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
