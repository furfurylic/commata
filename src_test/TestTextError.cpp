/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <commata/text_error.hpp>

#include "BaseTest.hpp"
#include "tracking_allocator.hpp"

using namespace commata;
using namespace commata::test;

static_assert(std::is_nothrow_move_constructible_v<text_error_info>);
static_assert(std::is_nothrow_copy_constructible_v<text_error_info>);
static_assert(std::is_nothrow_move_assignable_v<text_error_info>);
static_assert(std::is_nothrow_copy_assignable_v<text_error_info>);
static_assert(std::is_trivially_copyable_v<text_error_info>);
static_assert(std::is_trivially_destructible_v<text_error_info>);

static_assert(std::is_nothrow_move_constructible_v<text_error>);
static_assert(std::is_nothrow_copy_constructible_v<text_error>);
static_assert(std::is_nothrow_move_assignable_v<text_error>);
static_assert(std::is_nothrow_copy_assignable_v<text_error>);

struct TestTextError : BaseTest
{};

TEST_F(TestTextError, DefaultCtor)
{
    text_error e;
    ASSERT_STREQ("", e.what());
    ASSERT_FALSE(e.get_physical_position());
}

TEST_F(TestTextError, Ctors)
{
    const char* message = "Some error occurred";

    text_error e1(message);
    ASSERT_STREQ(message, e1.what());
    ASSERT_FALSE(e1.get_physical_position());

    e1.set_physical_position(123, 456);

    auto e2(e1);
    ASSERT_STREQ(message, e2.what());
    ASSERT_TRUE(e2.get_physical_position());
    ASSERT_EQ(123, e2.get_physical_position()->first);
    ASSERT_EQ(456, e2.get_physical_position()->second);

    auto e3(std::move(e1));
    ASSERT_STREQ(message, e3.what());
    ASSERT_TRUE(e3.get_physical_position());
    ASSERT_EQ(123, e3.get_physical_position()->first);
    ASSERT_EQ(456, e3.get_physical_position()->second);

    text_error e4{std::string_view(message)};   // most vexing parse
    ASSERT_STREQ(message, e4.what());
    ASSERT_FALSE(e4.get_physical_position());
}

TEST_F(TestTextError, AssignmentOps)
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

TEST_F(TestTextError, Info)
{
    text_error e("Some error occurred");
    {
        std::ostringstream s;
        s << e.info();
        ASSERT_EQ(e.what(), std::move(s).str());
    }

    e.set_physical_position(12345);
    {
        std::ostringstream s;
        s << e.info();
        std::string i = std::move(s).str();
        ASSERT_NE(i, e.what());
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
        std::ostringstream s;
        s << e.info();
        is = std::move(s).str();
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
        std::ostringstream s;
        s << std::setw(is.size() + 20) << std::setfill('_') << e.info();
        is2 = std::move(s).str();
    }
    ASSERT_EQ(is.size() + 20, is2.size());
    ASSERT_EQ(std::string(20, '_'), is2.substr(0, 20));
    ASSERT_EQ(is, is2.substr(20));

    // info()'s string is aligned to left
    std::string is3;
    {
        std::ostringstream s;
        s << std::left;
        s << std::setw(is.size() + 10) << std::setfill('#') << e.info();
        is3 = std::move(s).str();
    }
    ASSERT_EQ(is.size() + 10, is3.size());
    ASSERT_EQ(is, is3.substr(0, is.size()));
    ASSERT_EQ(std::string(10, '#'), is3.substr(is.size()));

    // info() can be written into wide streams
    std::wstring isw;
    {
        std::wostringstream s;
        s << is.c_str();
        isw = std::move(s).str();
    }
    std::wstring is4;
    {
        std::wostringstream s;
        s << e.info();
        is4 = std::move(s).str();
    }
    ASSERT_EQ(isw, is4);
    ASSERT_EQ(isw, to_wstring(e.info()));
}

TEST_F(TestTextError, Allocators)
{
    std::vector<std::pair<char*, char*>> allocated;
    tracking_allocator<std::allocator<char>> a(allocated);

    const char* message = "A certain very lengthy message that tells "
                          "the details of the text error";

    using traits_t = std::char_traits<char>;
    using string_t = std::basic_string<char, traits_t, decltype(a)>;
    text_error e1(string_t(message, a));
    ASSERT_STREQ(message, e1.what());
    ASSERT_TRUE(a.tracks(e1.what()));

    auto e2(e1);
    auto e3(std::move(e1));
    ASSERT_TRUE(a.tracks(e2.what()));
    ASSERT_TRUE(a.tracks(e3.what()));
}
