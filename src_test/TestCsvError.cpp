/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include <furfurylic/commata/csv_error.hpp>

using namespace furfurylic::commata;

static_assert(
    std::is_nothrow_move_constructible<csv_error>::value,
    "csv_error shall be nothrow-move-constructible");

static_assert(
    std::is_nothrow_copy_constructible<csv_error>::value,
    "csv_error shall be nothrow-copy-constructible");

static_assert(
    std::is_nothrow_move_assignable<csv_error>::value,
    "csv_error shall be nothrow-move-assignable");

static_assert(
    std::is_nothrow_copy_assignable<csv_error>::value,
    "csv_error shall be nothrow-copy-assignable");

TEST(TestCsvError, Ctors)
{
    const char* message = "Some error occurred";

    csv_error e1(message);
    ASSERT_STREQ(message, e1.what());

    auto e2(e1);
    auto e3(std::move(e1));
    ASSERT_STREQ(message, e2.what());
    ASSERT_STREQ(message, e3.what());
}

TEST(TestCsvError, AssignmentOps)
{
    const char* message1 = "Some error occurred";
    const char* message2 = "One more error occurred";
    const char* message3 = "No message";

    csv_error e1(message1);
    csv_error e2(message2);
    csv_error e3(message3);

    e2 = e1;
    e3 = std::move(e1);
    ASSERT_STREQ(message1, e2.what());
    ASSERT_STREQ(message1, e3.what());
}
