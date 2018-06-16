/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include <furfurylic/commata/text_error.hpp>

using namespace furfurylic::commata;

static_assert(
    std::is_nothrow_move_constructible<text_error>::value,
    "text_error shall be nothrow-move-constructible");
static_assert(
    std::is_nothrow_copy_constructible<text_error>::value,
    "text_error shall be nothrow-copy-constructible");
static_assert(
    std::is_nothrow_move_assignable<text_error>::value,
    "text_error shall be nothrow-move-assignable");
static_assert(
    std::is_nothrow_copy_assignable<text_error>::value,
    "text_error shall be nothrow-copy-assignable");

TEST(TestTextError, Ctors)
{
    const char* message = "Some error occurred";

    text_error e1(message);
    ASSERT_STREQ(message, e1.what());

    auto e2(e1);
    auto e3(std::move(e1));
    ASSERT_STREQ(message, e2.what());
    ASSERT_STREQ(message, e3.what());
}

TEST(TestTextError, AssignmentOps)
{
    const char* message1 = "Some error occurred";
    const char* message2 = "One more error occurred";
    const char* message3 = "No message";

    text_error e1(message1);
    text_error e2(message2);
    text_error e3(message3);

    e2 = e1;
    e3 = std::move(e1);
    ASSERT_STREQ(message1, e2.what());
    ASSERT_STREQ(message1, e3.what());
}

TEST(TestTextError, Info)
{
    text_error e("Some error occurred");
    {
        std::stringstream s;
        s << e.info();
        ASSERT_EQ(e.what(), s.str());
    }

    e.set_physical_position(12345);
    {
        std::stringstream s;
        s << e.info();
        std::string i = s.str();
        ASSERT_NE(s.str(), e.what());
        const auto lpos = i.find(
            std::to_string(e.get_physical_position()->first + 1));
        const auto cpos = i.find("n/a");
        ASSERT_NE(lpos, std::string::npos);
        ASSERT_NE(cpos, std::string::npos);
        ASSERT_GT(cpos, lpos);
    }

    // info()'s string contains positions
    e.set_physical_position(text_error::npos - 1, text_error::npos - 2);
    std::string is;
    {
        std::stringstream s;
        s << e.info();
        is = s.str();
    }
    ASSERT_NE(is, e.what());
    {
        const auto lpos = is.find(
            std::to_string(e.get_physical_position()->first + 1));
        const auto cpos = is.find(
            std::to_string(e.get_physical_position()->second + 1));
        ASSERT_NE(lpos, std::string::npos);
        ASSERT_NE(cpos, std::string::npos);
        ASSERT_GT(cpos, lpos);
    }
    ASSERT_EQ(is, to_string(e.info()));

    // info()'s string is aligned to right
    std::string is2;
    {
        std::stringstream s;
        s << std::setw(is.size() + 20) << std::setfill('_') << e.info();
        is2 = s.str();
    }
    ASSERT_EQ(is.size() + 20, is2.size());
    ASSERT_EQ(std::string(20, '_'), is2.substr(0, 20));
    ASSERT_EQ(is, is2.substr(20));

    // info()'s string is aligned to left
    std::string is3;
    {
        std::stringstream s;
        s << std::left;
        s << std::setw(is.size() + 10) << std::setfill('#') << e.info();
        is3 = s.str();
    }
    ASSERT_EQ(is.size() + 10, is3.size());
    ASSERT_EQ(is, is3.substr(0, is.size()));
    ASSERT_EQ(std::string(10, '#'), is3.substr(is.size()));

    // info() can be written into wide streams
    std::wstring isw;
    {
        std::wstringstream s;
        s << is.c_str();
        isw = s.str();
    }
    std::wstring is4;
    {
        std::wstringstream s;
        s << e.info();
        is4 = s.str();
    }
    ASSERT_EQ(isw, is4);
    ASSERT_EQ(isw, to_wstring(e.info()));
}
