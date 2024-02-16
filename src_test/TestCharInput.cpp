/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <ios>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <commata/char_input.hpp>

#include "BaseTest.hpp"

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

using namespace commata;
using namespace commata::test;

static_assert(std::is_trivially_copyable_v<streambuf_input<char>>);
static_assert(std::is_trivially_copyable_v<istream_input<char>>);
static_assert(std::is_trivially_copyable_v<string_input<wchar_t>>);

struct TestStreambufInput : BaseTest
{};

TEST_F(TestStreambufInput, Basics)
{
    std::stringbuf s("1234567");
    streambuf_input in(s);
    char b[5];

    ASSERT_EQ(4U, in(b, 4));    // reads 1234
    b[4] = '\0';
    ASSERT_STREQ("1234", b);

    ASSERT_EQ(3U, in(b, 4));    // read 567
    b[3] = '\0';
    ASSERT_STREQ("567", b);
}

TEST_F(TestStreambufInput, CopyConstruct)
{
    std::stringbuf s("1234567");
    streambuf_input in1(s);
    streambuf_input in2(in1);
    char b[5];

    ASSERT_EQ(4U, in1(b, 4));   // reads 1234
    ASSERT_EQ(3U, in2(b, 4));   // reads 567
    b[3] = '\0';
    ASSERT_STREQ("567", b);
}

TEST_F(TestStreambufInput, CopyAssign)
{
    std::stringbuf s("1234567");
    streambuf_input in1(s);
    char b[5];

    decltype(in1) in2;
    ASSERT_EQ(0U, in2(b, 4));   // reads nothing (default constructed)

    in2 = in1;
    ASSERT_EQ(4U, in1(b, 4));   // reads 1234
    ASSERT_EQ(3U, in2(b, 4));   // reads 567
    b[3] = '\0';
    ASSERT_STREQ("567", b);
}

TEST_F(TestStreambufInput, Swap)
{
    std::wstringbuf s(L"1234567");
    std::wstringbuf t(L"XYZUVW");
    streambuf_input in1(s);
    streambuf_input in2(t);
    wchar_t b[5];

    ASSERT_EQ(4U, in1(b, 4));   // reads 1234
    using std::swap;
    swap(in1, in2);
    ASSERT_EQ(4U, in1(b, 4));   // reads XYZU
    b[4] = L'\0';
    ASSERT_STREQ(L"XYZU", b);
}

struct TestIStreamInput : BaseTest
{};

TEST_F(TestIStreamInput, Basics)
{
    std::istringstream s("1234567");
    istream_input in(s);
    char b[5];

    ASSERT_EQ(4U, in(b, 4));    // reads 1234
    b[4] = '\0';
    ASSERT_STREQ("1234", b);

    ASSERT_EQ(3U, in(b, 4));    // reads 567
    b[3] = '\0';
    ASSERT_STREQ("567", b);
}

TEST_F(TestIStreamInput, CopyConstruct)
{
    std::istringstream s("1234567");
    istream_input in1(s);
    istream_input in2(in1);
    char b[5];

    ASSERT_EQ(4U, in1(b, 4));   // reads 1234
    ASSERT_EQ(3U, in2(b, 4));   // reads 567
    b[3] = '\0';
    ASSERT_STREQ("567", b);
}

TEST_F(TestIStreamInput, CopyAssign)
{
    std::istringstream s("1234567");
    istream_input in1(s);
    char b[5];

    decltype(in1) in2;
    ASSERT_EQ(0U, in2(b, 4));   // reads nothing (default constructed)

    in2 = in1;
    ASSERT_EQ(4U, in1(b, 4));   // reads 1234
    ASSERT_EQ(3U, in2(b, 4));   // reads 567
    b[3] = '\0';
    ASSERT_STREQ("567", b);
}

TEST_F(TestIStreamInput, Swap)
{
    std::wistringstream s(L"1234567");
    std::wistringstream t(L"XYZUVW");
    istream_input in1(s);
    istream_input in2(t);
    wchar_t b[5];

    ASSERT_EQ(4U, in1(b, 4));   // reads 1234
    using std::swap;
    swap(in1, in2);
    ASSERT_EQ(4U, in1(b, 4));   // reads XYZU
    b[4] = L'\0';
    ASSERT_STREQ(L"XYZU", b);
}

TEST_F(TestIStreamInput, ThrowAsStream)
{
    std::istringstream s("XYZ");
    s.exceptions(std::ios_base::eofbit);
    istream_input in(s);
    char b[4];
    ASSERT_THROW(in(b, 4), std::ios_base::failure); // EOF sets the state
                                                    // to failbit|eofbit
}

struct TestOwnedStreambufInput : BaseTest
{};

TEST_F(TestOwnedStreambufInput, Basics)
{
    owned_streambuf_input in(std::stringbuf("1234567"));
    char b[5];

    ASSERT_EQ(4U, in(b, 4));    // reads 1234
    b[4] = '\0';
    ASSERT_STREQ("1234", b);

    ASSERT_EQ(3U, in(b, 4));    // reads 567
    b[3] = '\0';
    ASSERT_STREQ("567", b);
}

TEST_F(TestOwnedStreambufInput, MoveConstruct)
{
    owned_streambuf_input in1(std::stringbuf("1234567"));
    char b[5];

    ASSERT_EQ(4U, in1(b, 4));   // reads 1234

    owned_streambuf_input in2(std::move(in1));
    ASSERT_EQ(3U, in2(b, 4));   // reads 567
    b[3] = '\0';
    ASSERT_STREQ("567", b);
}

TEST_F(TestOwnedStreambufInput, MoveAssign)
{
    owned_streambuf_input in1(std::stringbuf("1234567"));
    owned_streambuf_input in2(std::stringbuf("XYZ"));
    char b[5];

    ASSERT_EQ(4U, in1(b, 4));   // reads 1234

    in2 = std::move(in1);
    ASSERT_EQ(3U, in2(b, 4));   // reads 567
    b[3] = '\0';
    ASSERT_STREQ("567", b);
}

TEST_F(TestOwnedStreambufInput, Swap)
{
    std::wstringbuf s(L"1234567");
    std::wstringbuf t(L"XYZUVW");
    owned_streambuf_input in1(std::move(s));
    owned_streambuf_input in2(std::move(t));
    wchar_t b[5];

    ASSERT_EQ(4U, in1(b, 4));   // reads 1234
    using std::swap;
    swap(in1, in2);
    ASSERT_EQ(4U, in1(b, 4));   // reads XYZU
    b[4] = L'\0';
    ASSERT_STREQ(L"XYZU", b);
}

struct TestOwnedIStreamInput : BaseTest
{};

TEST_F(TestOwnedIStreamInput, Basics)
{
    owned_istream_input in(std::istringstream("1234567"));
    char b[5];

    ASSERT_EQ(4U, in(b, 4));    // reads 1234
    b[4] = '\0';
    ASSERT_STREQ("1234", b);

    ASSERT_EQ(3U, in(b, 4));    // reads 567
    b[3] = '\0';
    ASSERT_STREQ("567", b);
}

TEST_F(TestOwnedIStreamInput, MoveConstruct)
{
    owned_istream_input in1(std::istringstream("1234567"));
    char b[5];

    ASSERT_EQ(4U, in1(b, 4));   // reads 1234

    owned_istream_input in2(std::move(in1));
    ASSERT_EQ(3U, in2(b, 4));   // reads 567
    b[3] = '\0';
    ASSERT_STREQ("567", b);
}

TEST_F(TestOwnedIStreamInput, MoveAssign)
{
    owned_istream_input in1(std::istringstream("1234567"));
    owned_istream_input in2(std::istringstream("XYZ"));
    char b[5];

    ASSERT_EQ(4U, in1(b, 4));   // reads 1234

    in2 = std::move(in1);
    ASSERT_EQ(3U, in2(b, 4));   // reads 567
    b[3] = '\0';
    ASSERT_STREQ("567", b);
}

TEST_F(TestOwnedIStreamInput, Swap)
{
    std::wistringstream s(L"1234567");
    std::wistringstream t(L"XYZUVW");
    owned_istream_input in1(std::move(s));
    owned_istream_input in2(std::move(t));
    wchar_t b[5];

    ASSERT_EQ(4U, in1(b, 4));   // reads 1234
    using std::swap;
    swap(in1, in2);
    ASSERT_EQ(4U, in1(b, 4));   // reads XYZU
    b[4] = L'\0';
    ASSERT_STREQ(L"XYZU", b);
}

TEST_F(TestOwnedIStreamInput, ThrowAsStream)
{
    std::istringstream s("XYZ");
    s.exceptions(std::ios_base::failbit);
    owned_istream_input in(std::move(s));
    char b[4];
    ASSERT_THROW(in(b, 4), std::ios_base::failure); // EOF sets the state
                                                    // to failbit|eofbit
}

struct TestStringInput : BaseTest
{};

TEST_F(TestStringInput, Basics)
{
    string_input in("1234567"sv);
    char b[5];

    ASSERT_EQ(4U, in(b, 4));    // reads 1234
    b[4] = '\0';
    ASSERT_STREQ("1234", b);

    ASSERT_EQ(3U, in(b, 4));    // reads 567
    b[3] = '\0';
    ASSERT_STREQ("567", b);
}

TEST_F(TestStringInput, CopyConstruct)
{
    string_input in1("1234567"sv);
    char b[5];

    ASSERT_EQ(4U, in1(b, 4));   // reads 1234

    string_input in2(in1);
    ASSERT_EQ(3U, in2(b, 4));   // reads 567
    b[3] = '\0';
    ASSERT_STREQ("567", b);

    ASSERT_EQ(3U, in1(b, 4));   // reads 567 again
    b[3] = '\0';
    ASSERT_STREQ("567", b);
}

TEST_F(TestStringInput, CopyAssign)
{
    string_input in1("1234567"sv);
    char b[5];

    decltype(in1) in2;
    ASSERT_EQ(0U, in2(b, 4));   // reads nothing (default constructed)

    ASSERT_EQ(4U, in1(b, 4));   // reads 1234

    in2 = in1;
    ASSERT_EQ(3U, in2(b, 4));   // reads 567
    b[3] = '\0';
    ASSERT_STREQ("567", b);

    ASSERT_EQ(3U, in1(b, 4));   // reads 567 again
    b[3] = '\0';
    ASSERT_STREQ("567", b);
}

TEST_F(TestStringInput, Swap)
{
    string_input in1(L"1234567"sv);
    string_input in2(L"XYZUVW"sv);
    wchar_t b[5];

    ASSERT_EQ(4U, in1(b, 4));   // reads 1234
    using std::swap;
    swap(in1, in2);
    ASSERT_EQ(4U, in1(b, 4));   // reads XYZU
    b[4] = L'\0';
    ASSERT_STREQ(L"XYZU", b);
}

TEST_F(TestStringInput, Direct)
{
    const char* const s = "ABCDEFGHIJKL";
    string_input in(s);

    {
        auto r = in(3);
        ASSERT_EQ(s, r.first);
        ASSERT_EQ(3U, r.second);
    }
    {
        // 'Normal' copying mixed
        char buf[4];
        auto len = in(buf, 4);
        ASSERT_EQ(4U, len);
        ASSERT_EQ("DEFG"sv, std::string_view(buf, len));
    }
    {
        auto r = in();
        ASSERT_EQ(s + 7, r.first);
        ASSERT_EQ(5U, r.second);
    }
}

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
    const auto lenp = (*p)(a.data(), 2);    // reads AB
    auto q = std::move(*p);
    p.reset();
    const auto lenq = q(b.data(), 3);       // reads C

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

TEST_F(TestOwnedStringInput, Direct)
{
    // Lengthy string is needed to evade short string optimization
    const std::wstring s(L"ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    const wchar_t* const p = s.data();
    string_input in(std::move(s));

    {
        auto r = in(3);
        ASSERT_EQ(p, r.first);
        ASSERT_EQ(3U, r.second);
    }
    {
        // 'Normal' copying mixed
        wchar_t buf[4];
        auto len = in(buf, 4);
        ASSERT_EQ(4U, len);
        ASSERT_EQ(L"DEFG"sv, std::wstring_view(buf, len));
    }
    {
        auto r = in();
        ASSERT_EQ(p + 7, r.first);
        ASSERT_EQ(19U, r.second);
    }
}

struct TestCharInput : BaseTest
{};

TEST_F(TestCharInput, MakeFromStreambufPtr)
{
    std::wstringbuf buf(L"XYZ");
    streambuf_input<wchar_t> in = make_char_input(buf);
    std::wstring out(5, L' ');
    ASSERT_EQ(3, in(out.data(), 5));
    ASSERT_EQ(L"XYZ  ", out);
}

TEST_F(TestCharInput, MakeFromIStreamLvalueRef)
{
    std::istringstream buf("XYZ");
    istream_input<char> in = make_char_input(buf);
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

namespace {

namespace indirect_input_asserts {

using is_t = indirect_input<string_input<char>>;
using io_t = indirect_input<owned_string_input<char>>;

static_assert(std::is_nothrow_copy_constructible_v<is_t>);
static_assert(std::is_nothrow_move_constructible_v<is_t>);
static_assert(std::is_nothrow_copy_assignable_v<is_t>);
static_assert(std::is_nothrow_move_assignable_v<is_t>);
static_assert(std::is_trivially_copyable_v<is_t>);

static_assert(!std::is_copy_constructible_v<io_t>);
static_assert(std::is_nothrow_move_constructible_v<io_t>);
static_assert(!std::is_copy_assignable_v<io_t>);
static_assert(std::is_nothrow_move_assignable_v<io_t>
          || !std::is_nothrow_move_assignable_v<std::string>);
    // There are libs whose std::string is not MoveAssignable. Sigh...

static_assert(std::is_same_v<is_t,
    decltype(make_char_input(indirect, "123"sv))>);
static_assert(noexcept(make_char_input(indirect, "123"sv)));

static_assert(std::is_same_v<io_t,
    decltype(make_char_input(indirect, "123"s))>);
static_assert(
    noexcept(make_char_input(indirect, std::declval<std::string>())));

static_assert(std::is_same_v<io_t,
    decltype(make_char_input(indirect, make_char_input("123"s)))>);
static_assert(
    noexcept(make_char_input(indirect, std::declval<string_input<char>>())));

static_assert(std::is_same_v<is_t,
    decltype(make_char_input(indirect, std::declval<is_t>()))>);

}

}
