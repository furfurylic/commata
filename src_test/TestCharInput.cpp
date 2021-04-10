/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#include <memory>
#include <string>

#include <gtest/gtest.h>

#include <commata/char_input.hpp>

#include "BaseTest.hpp"

using namespace commata;
using namespace commata::test;

namespace {

struct TestCharInput : BaseTest
{};

TEST_F(TestCharInput, OwnedStringCopy)
{
    auto i = std::make_unique<owned_string_input<char>>(std::string("ABC"));
    auto j = *i;

    std::vector<char> b(6);

    {
        const auto len = (*i)(b.data(), 4);
        ASSERT_EQ(3, len);
        ASSERT_EQ("ABC", std::string(b.data(), 3));
    }
    i.reset();

    {
        const auto len = j(b.data() + 3, 4);
        ASSERT_EQ(3, len);
        ASSERT_EQ("ABCABC", std::string(b.data(), 6));
    }
}

}
