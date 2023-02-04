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

struct TestOwnedStringInput : BaseTest
{};

TEST_F(TestOwnedStringInput, ConstructFromStringRValue)
{
    std::string s;
    for (std::size_t i = 0; i < 20; ++i) {
        s += std::to_string(i);     // inhibit short string optimization
    }
    const auto sd = s.data();

    owned_string_input<char> p(std::move(s));
    std::vector<char> a1(3);
    std::vector<char> a2(2);
    const auto len1 = p(a1.data(), 3);
    const auto len2 = p(a2.data(), 2);

    ASSERT_NE(s.data(), sd);        // sd is moved-from
    ASSERT_EQ(3U, len1);
    ASSERT_EQ("012", std::string(a1.data(), 3));
    ASSERT_EQ(2U, len2);
    ASSERT_EQ("34", std::string(a2.data(), 2));
}

TEST_F(TestOwnedStringInput, MoveConstruct)
{
    auto p = std::make_unique<owned_string_input<char>>("ABC"s);

    std::vector<char> a(2);
    std::vector<char> b(3);
    const auto lenp = (*p)(a.data(), 2);
    auto q = std::move(*p);
    p.reset();
    const auto lenq = q(b.data(), 3);

    ASSERT_EQ(2U, lenp);
    ASSERT_EQ("AB", std::string(a.data(), 2));
    ASSERT_EQ(1U, lenq);
    ASSERT_EQ("C", std::string(b.data(), 1));
}

TEST_F(TestOwnedStringInput, MoveAssign)
{
    auto p = std::make_unique<owned_string_input<char>>("ABC"s);
    owned_string_input q("XYZ"s);

    std::vector<char> a1(1);
    std::vector<char> b1(2);
    std::vector<char> b2(2);
    (*p)(a1.data(), 1);
    q(b1.data(), 2);
    q = std::move(*p);
    p.reset();
    const auto lenq2 = q(b2.data(), 2);

    ASSERT_EQ(2U, lenq2);
    ASSERT_EQ("BC", std::string(b2.data(), 2));
}

TEST_F(TestOwnedStringInput, Swap)
{
    owned_string_input p(L"ABC"s);
    owned_string_input q(L"XYZ"s);

    std::vector<wchar_t> a1(1);
    std::vector<wchar_t> a2(1);
    std::vector<wchar_t> b1(2);
    std::vector<wchar_t> b2(2);
    p(a1.data(), 1);
    q(b1.data(), 2);
    using std::swap;
    swap(p, q);
    const auto lenp2 = p(a2.data(), 1);
    const auto lenq2 = q(b2.data(), 2);

    ASSERT_EQ(1U, lenp2);
    ASSERT_EQ(L"Z", std::wstring(a2.data(), 1));
    ASSERT_EQ(2U, lenq2);
    ASSERT_EQ(L"BC", std::wstring(b2.data(), 2));
}

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
