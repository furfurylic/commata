/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <locale>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <commata/write_ntmbs.hpp>

#include "BaseTest.hpp"

using namespace commata::detail;
using namespace commata::test;

struct TestWriteNTMBS : BaseTest
{};

TEST_F(TestWriteNTMBS, NarrowChar)
{
    // Frankly speaking, we don't like to rely on ntmbs::nchar's sanity, but
    // this test will be far more lengthy and redundant if we don't trust it
    const std::string zeros(ntmbs::nchar<char>(), '0');

    std::ostringstream os;
    os.imbue(std::locale::classic());

    std::vector<char> s = { 'A', 'B', '\0', 'C' };
    write_ntmbs(os, s.cbegin(), s.cend());
    s.push_back('\0');

    ASSERT_EQ("AB[0x" + zeros + "]C", std::move(os).str());
}

TEST_F(TestWriteNTMBS, WideChar)
{
    // ditto
    const std::string zeros(ntmbs::nchar<wchar_t>(), '0');

    std::ostringstream os;
    os.imbue(std::locale::classic());

    std::vector<wchar_t> s = { L'A', L'B', L'\0', L'C' };
    write_ntmbs(os, s.cbegin(), s.cend());

    ASSERT_EQ("AB[0x" + zeros + "]C", std::move(os).str());
}
