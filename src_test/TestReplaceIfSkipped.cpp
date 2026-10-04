/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <commata/field_scanners.hpp>

#include "BaseTest.hpp"
#include "trivials.hpp"

using namespace std::string_view_literals;
using namespace commata;
using namespace commata::test;

namespace {

using ReplacedTypes = testing::Types<int, std::string>;

struct B
{};

struct D : B
{};

struct E
{
    explicit E(const B&)
    {}
};

static_assert(std::is_convertible_v<D, replace_if_skipped<B>>);
static_assert(!std::is_convertible_v<B, replace_if_skipped<E>>);

static_assert(std::is_trivially_destructible_v<replace_if_skipped<int>>);
static_assert(
    !std::is_trivially_destructible_v<replace_if_skipped<std::string>>);
static_assert(
    std::is_destructible_v<replace_if_skipped<std::string>>);

static_assert(std::is_trivially_move_assignable_v<replace_if_skipped<int>>);
static_assert(
    !std::is_trivially_move_assignable_v<replace_if_skipped<std::string>>);
static_assert(
    std::is_nothrow_move_assignable_v<replace_if_skipped<std::string>>);

static_assert(std::is_trivially_copy_assignable_v<replace_if_skipped<int>>);
static_assert(
    !std::is_trivially_copy_assignable_v<replace_if_skipped<std::string>>);
static_assert(
    !std::is_nothrow_copy_assignable_v<replace_if_skipped<std::string>>);
static_assert(
    std::is_copy_assignable_v<replace_if_skipped<std::string>>);

static_assert(
std::is_trivially_move_constructible_v<replace_if_skipped<int>>);
static_assert(
    !std::is_trivially_move_constructible_v<replace_if_skipped<std::string>>);
static_assert(
    std::is_nothrow_move_constructible_v<replace_if_skipped<std::string>>);

static_assert(
    std::is_trivially_copy_constructible_v<replace_if_skipped<int>>);
static_assert(
    !std::is_trivially_copy_constructible_v<replace_if_skipped<std::string>>);
static_assert(
    !std::is_nothrow_copy_constructible_v<replace_if_skipped<std::string>>);
static_assert(
    std::is_copy_constructible_v<replace_if_skipped<std::string>>);

} // end unnamed

struct TestReplaceIfSkipped : BaseTest
{};

template <class T>
struct TestReplaceIfSkippedTyped : BaseTest
{};

TYPED_TEST_SUITE(TestReplaceIfSkippedTyped, ReplacedTypes, );

TYPED_TEST(TestReplaceIfSkippedTyped, ActionInstallmentWithCtors)
{
    using r_t = replace_if_skipped<TypeParam>;

    // default ctor
    {
        r_t r;
        ASSERT_EQ(TypeParam(), r());
    }

    // copy
    {
        const TypeParam val = from_str("300"sv);
        r_t r(val);
        ASSERT_EQ(val, r());
    }

    // ignore
    {
        r_t r(replacement_ignore);
        ASSERT_TRUE(!r());
    }

    // fail
    {
        r_t r(replacement_fail);
        ASSERT_THROW(r(), field_not_found);
    }
}

TYPED_TEST(TestReplaceIfSkippedTyped, CopyCtor)
{
    using r_t = replace_if_skipped<TypeParam>;

    // copy
    {
        const TypeParam val = from_str("-10.0"sv);
        const r_t r0(val);
        r_t r(r0);
        ASSERT_EQ(val, r());
    }

    // ignore
    {
        const r_t r0(replacement_ignore);
        r_t r(r0);
        ASSERT_TRUE(!r());
    }

    // fail
    {
        const r_t r0(replacement_fail);
        r_t r(r0);
        ASSERT_THROW(r(), field_not_found);
    }
}

TYPED_TEST(TestReplaceIfSkippedTyped, MoveCtor)
{
    using r_t = replace_if_skipped<TypeParam>;

    // copy
    {
        const TypeParam val = from_str("123.45"sv);
        r_t r0(val);
        r_t r(std::move(r0));
        ASSERT_EQ(val, r());
    }

    // ignore
    {
        r_t r0(replacement_ignore);
        r_t r(std::move(r0));
        ASSERT_TRUE(!r());
    }

    // fail
    {
        r_t r0(replacement_fail);
        r_t r(std::move(r0));
        ASSERT_THROW(r(), field_not_found);
    }
}

TYPED_TEST(TestReplaceIfSkippedTyped, CopyAssign)
{
    using r_t = replace_if_skipped<TypeParam>;

    const TypeParam val = from_str("6.02e23"sv);

    // from copy
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(val);

        const TypeParam v = from_str("1.38e-23"sv);
        r_t r0(v);

        rs[0] = r0;
        rs[1] = r0;
        rs[2] = r0;
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_EQ(v, *rs[i]()) << i;
        }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
        r0 = r0;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
        ASSERT_EQ(v, *r0());
    }

    // from ignore
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(val);

        r_t r0(replacement_ignore);

        rs[0] = r0;
        rs[1] = r0;
        rs[2] = r0;
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_TRUE(!rs[i]()) << i;
        }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
        r0 = r0;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
        ASSERT_TRUE(!r0());
    }

    // from fail
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(val);

        r_t r0(replacement_fail);

        rs[0] = r0;
        rs[1] = r0;
        rs[2] = r0;
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_THROW(rs[i](), field_not_found) << i;
        }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
        r0 = r0;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
        ASSERT_THROW(r0(), field_not_found);
    }
}

TYPED_TEST(TestReplaceIfSkippedTyped, MoveAssign)
{
    using r_t = replace_if_skipped<TypeParam>;

    const TypeParam val = from_str("0.01"sv);

    // from copy
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(val);

        const TypeParam v = from_str("-0.2"sv);

        rs[0] = r_t(v);
        rs[1] = r_t(v);
        rs[2] = r_t(v);
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_EQ(v, *rs[i]()) << i;
        }
    }

    // from ignore
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(val);

        rs[0] = r_t(replacement_ignore);
        rs[1] = r_t(replacement_ignore);
        rs[2] = r_t(replacement_ignore);
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_TRUE(!rs[i]()) << i;
        }
    }

    // from fail
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(val);

        rs[0] = r_t(replacement_fail);
        rs[1] = r_t(replacement_fail);
        rs[2] = r_t(replacement_fail);
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_THROW(rs[i](), field_not_found) << i;
        }
    }
}

TYPED_TEST(TestReplaceIfSkippedTyped, Swap)
{
    using r_t = replace_if_skipped<TypeParam>;

    const TypeParam v1 = from_str("3.142"sv);
    const TypeParam v2 = from_str("2.718"sv);

    std::vector<r_t> rs;
    rs.emplace_back(v1);
    rs.emplace_back(replacement_ignore);
    rs.emplace_back(replacement_fail);
    rs.emplace_back(v2);

    using std::swap;

    // copy vs ignore
    swap(rs[0], rs[1]);
    ASSERT_TRUE(!rs[0]());
    ASSERT_EQ(v1, rs[1]());
    swap(rs[0], rs[1]);
    ASSERT_EQ(v1, rs[0]());
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
    ASSERT_EQ(v2, rs[2]());
    ASSERT_THROW(rs[3](), field_not_found);
    swap(rs[2], rs[3]);
    ASSERT_EQ(v2, rs[3]());
    ASSERT_THROW(rs[2](), field_not_found);

    // copy vs copy
    swap(rs[3], rs[0]);
    ASSERT_EQ(v1, rs[3]());
    ASSERT_EQ(v2, rs[0]());
    swap(rs[3], rs[0]);
    ASSERT_EQ(v2, rs[3]());
    ASSERT_EQ(v1, rs[0]());

    // swap with self
    swap(rs[0], rs[0]);
    ASSERT_EQ(v1, rs[0]());
    swap(rs[1], rs[1]);
    ASSERT_TRUE(!rs[1]());
    swap(rs[2], rs[2]);
    ASSERT_THROW(rs[2](), field_not_found);
}

TEST_F(TestReplaceIfSkipped, Convertible)
{
    std::vector<replace_if_skipped<std::string>> rs;
    rs.emplace_back("ABC");
    rs.emplace_back(replacement_ignore);
    rs.emplace_back(replacement_fail);

    // 0: copy: "ABC"
    {
        const auto o1 = rs[0](static_cast<std::string_view*>(nullptr));
        const auto o2 = rs[0](static_cast<std::string_view*>(nullptr));
        ASSERT_EQ("ABC"sv, o1);
        ASSERT_EQ(o1->data(), o2->data());  // views to an identical string
    }

    // 1: ignore
    ASSERT_FALSE(rs[1](static_cast<std::string_view*>(nullptr)));

    // 2: fail
    ASSERT_THROW(rs[2](static_cast<std::string_view*>(nullptr)),
        field_not_found);

    // Not convertible
    static_assert(!std::is_invocable_v<replace_if_skipped<std::string>,
                                       std::wstring*>);
}

TEST_F(TestReplaceIfSkipped, DeductionGuides)
{
    replace_if_skipped r1(10);
    static_assert(std::is_same_v<replace_if_skipped<int>, decltype(r1)>);
    ASSERT_TRUE(r1().has_value());
    ASSERT_EQ(10, *r1());

    const std::string s("skipped");
    replace_if_skipped r2(s);
    static_assert(
        std::is_same_v<replace_if_skipped<std::string>, decltype(r2)>);
    ASSERT_TRUE(r2().has_value());
    ASSERT_STREQ("skipped", r2()->c_str());
}

namespace {

using ri_t = replace_if_skipped<int>;
using rv_t = replace_if_skipped<std::vector<int>>;

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

static_assert(std::is_trivially_copy_constructible_v<
    replace_if_skipped<trivially_copy_constructible>>);
static_assert(std::is_trivially_move_constructible_v<
    replace_if_skipped<trivially_move_constructible>>);
static_assert(std::is_trivially_copy_assignable_v<
    replace_if_skipped<trivially_copy_assignable>>);
static_assert(std::is_trivially_move_assignable_v<
    replace_if_skipped<trivially_move_assignable>>);

} // end unnamed
