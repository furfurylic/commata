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

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

using namespace commata;
using namespace commata::test;

namespace {

struct TestCharInput : BaseTest
{};

TEST_F(TestCharInput, MakeFromStreambufPtr)
{
    std::wstringbuf buf(L"XYZ");
    streambuf_input<wchar_t> in = make_char_input(&buf);
    std::wstring out(5, L' ');
    ASSERT_EQ(3, in(out.data(), 5));
    ASSERT_EQ(L"XYZ  ", out);
}

TEST_F(TestCharInput, MakeFromIStreamLvalueRef)
{
    std::istringstream buf("XYZ");
    streambuf_input<char> in = make_char_input(buf);
    std::string out(5, ' ');
    ASSERT_EQ(3, in(out.data(), 5));
    ASSERT_EQ("XYZ  ", out);
}

TEST_F(TestCharInput, MakeFromStreambufRvalueRef)
{
    std::stringbuf buf("XYZ");
    owned_streambuf_input<std::stringbuf> in = make_char_input(std::move(buf));
    std::string out(5, ' ');
    ASSERT_EQ(3, in(out.data(), 5));
    ASSERT_EQ("XYZ  ", out);
}

TEST_F(TestCharInput, MakeFromIStreamRvalueRef)
{
    std::istringstream buf("XYZ");
    owned_istream_input<std::istringstream> in =
        make_char_input(std::move(buf));
    std::string out(5, ' ');
    ASSERT_EQ(3, in(out.data(), 5));
    ASSERT_EQ("XYZ  ", out);
}

TEST_F(TestCharInput, MakeFromCharPtr)
{
    const wchar_t* const str= L"XYZ";
    string_input<wchar_t> in = make_char_input(str);
    std::wstring out(5, L' ');
    ASSERT_EQ(3, in(out.data(), 5));
    ASSERT_EQ(L"XYZ  ", out);
}

TEST_F(TestCharInput, MakeFromCharPtrAndSize)
{
    const wchar_t* const str= L"XYZABC";
    string_input<wchar_t> in = make_char_input(str, 4);
    std::wstring out(5, L' ');
    ASSERT_EQ(4, in(out.data(), 4));
    ASSERT_EQ(L"XYZA ", out);
}

TEST_F(TestCharInput, MakeFromStringView)
{
    const auto str= L"XYZABC"sv;
    string_input<wchar_t> in = make_char_input(str.substr(2));
    std::wstring out(5, L' ');
    ASSERT_EQ(4, in(out.data(), 4));
    ASSERT_EQ(L"ZABC ", out);
}

TEST_F(TestCharInput, MakeFromStringLvalueRef)
{
    std::wstring str(L"XYZ");
    string_input<wchar_t> in = make_char_input(str);
    std::wstring out(5, L' ');
    ASSERT_EQ(3, in(out.data(), 5));
    ASSERT_EQ(L"XYZ  ", out);
}

TEST_F(TestCharInput, MakeFromStringRvalueRef)
{
    std::string str("XYZ");
    owned_string_input<char> in = make_char_input(std::move(str));
    std::string out(5, ' ');
    ASSERT_EQ(3, in(out.data(), 5));
    ASSERT_EQ("XYZ  ", out);
}

}
