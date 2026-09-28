/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <cstddef>
#include <deque>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <commata/text_value_translation.hpp>

#include "BaseTest.hpp"

using namespace std::string_view_literals;

using namespace commata;
using namespace commata::test;

namespace {

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

namespace {

using ReplacedTypes = testing::Types<double, std::string>;

struct B
{};

struct D : B
{};

struct E
{
    explicit E(const B&)
    {}
};

static_assert(std::is_convertible_v<D, replace_if_conversion_failed<B>>);
static_assert(!std::is_convertible_v<B, replace_if_conversion_failed<E>>);

static_assert(
    std::is_convertible_v<
        replacement_fail_t,
        replace_if_conversion_failed<std::string>>);
static_assert(
    !std::is_convertible_v<
        std::string_view,
        replace_if_conversion_failed<std::string>>);
static_assert(
    std::is_constructible_v<
        replace_if_conversion_failed<std::string>,
        std::string_view>);

} // end unnamed

TYPED_TEST_SUITE(TestReplaceIfConversionFailed, ReplacedTypes, );

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
}

TEST(TestReplaceIfSkippedConvertible, All)
{
    replace_if_conversion_failed<std::string> r(
        replacement_fail, replacement_ignore, "ABC");

    // Empty: fail
    ASSERT_THROW(r(empty_t(), static_cast<std::string_view*>(nullptr)),
        text_value_empty);

    // InvalidFormat: ignore
    {
        const char range[] = "XXX";
        ASSERT_FALSE(r(invalid_format_t(),
            range, range + 3, static_cast<std::string_view*>(nullptr)));
    }

    // AllOutOfRange: copy: "ABC"
    {
        const char range[] = "XXX";
        const wchar_t wrange[] = L"XXX";
        const auto a1 = r(out_of_range_t(), range, range + 3, 1,
            static_cast<std::string_view*>(nullptr));
        const auto a2 = r(out_of_range_t(), wrange, wrange + 3, 1,
            static_cast<std::string_view*>(nullptr));
        const auto l1 = r(out_of_range_t(), range, range + 3, -1,
            static_cast<std::string_view*>(nullptr));
        const auto l2 = r(out_of_range_t(), wrange, wrange + 3, -1,
            static_cast<std::string_view*>(nullptr));
        const auto u1 = r(out_of_range_t(), range, range + 3, 0,
            static_cast<std::string_view*>(nullptr));
        const auto u2 = r(out_of_range_t(), wrange, wrange + 3, 0,
            static_cast<std::string_view*>(nullptr));
        ASSERT_EQ("ABC"sv, a1);
        ASSERT_EQ("ABC"sv, l1);
        ASSERT_EQ("ABC"sv, u1);
        ASSERT_EQ(a1->data(), a2->data());  // views to an identical string
        ASSERT_EQ(l1->data(), l2->data());  // ditto
        ASSERT_EQ(u1->data(), u2->data());  // ditto
    }

    // Not convertible
    static_assert(!std::is_invocable_v<
        replace_if_conversion_failed<std::string>, empty_t, std::wstring*>);
    static_assert(!std::is_invocable_v<
        replace_if_conversion_failed<std::string>, invalid_format_t,
        const char*, const char*, std::wstring*>);
    static_assert(!std::is_invocable_v<
        replace_if_conversion_failed<std::string>, out_of_range_t,
        const char*, const char*, int, std::wstring*>);
}

namespace {

// Just to check compilation
[[maybe_unused]] void use_replace_if_conversion_failed_deduction_guide()
{
    replace_if_conversion_failed r(replacement_fail, 0, 10L,
                                   replacement_ignore);
    static_assert(std::is_same_v<replace_if_conversion_failed<long>,
                                 decltype(r)>);

    // The code below should not compile by a "no viable deduction guide" error
    // replace_if_conversion_failed r2(replacement_fail, 0, std::string("XY"));
}

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

} // end unnamed
