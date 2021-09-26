/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifdef _MSC_VER
#pragma warning(disable:4494)
#pragma warning(disable:4503)
#pragma warning(disable:4996)
#endif

#include <algorithm>
#include <cstring>
#include <deque>
#include <list>
#include <iomanip>
#include <locale>
#include <deque>
#include <scoped_allocator>
#include <string>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <commata/empty_physical_line_aware_handler.hpp>
#include <commata/stored_table.hpp>
#include <commata/parse_csv.hpp>

#include "BaseTest.hpp"
#include "fancy_allocator.hpp"
#include "identified_allocator.hpp"
#include "tracking_allocator.hpp"

using namespace commata;
using namespace commata::test;

static_assert(std::is_nothrow_copy_constructible<stored_value>::value, "");
static_assert(std::is_nothrow_copy_assignable<stored_value>::value, "");
static_assert(std::is_trivially_copyable<stored_value>::value, "");
static_assert(std::is_trivially_copy_assignable<stored_value>::value, "");
static_assert(std::is_trivially_destructible<stored_value>::value, "");
static_assert(noexcept(std::declval<stored_value&>().begin()), "");
static_assert(noexcept(std::declval<stored_value&>().end()), "");
static_assert(noexcept(std::declval<stored_value&>().rbegin()), "");
static_assert(noexcept(std::declval<stored_value&>().rend()), "");
static_assert(noexcept(std::declval<stored_value&>().cbegin()), "");
static_assert(noexcept(std::declval<stored_value&>().cend()), "");
static_assert(noexcept(std::declval<stored_value&>().crbegin()), "");
static_assert(noexcept(std::declval<stored_value&>().crend()), "");
static_assert(noexcept(std::declval<const stored_value&>().begin()), "");
static_assert(noexcept(std::declval<const stored_value&>().end()), "");
static_assert(noexcept(std::declval<const stored_value&>().rbegin()), "");
static_assert(noexcept(std::declval<const stored_value&>().rend()), "");
static_assert(noexcept(std::declval<stored_value&>().c_str()), "");
static_assert(noexcept(std::declval<const stored_value&>().c_str()), "");
static_assert(noexcept(std::declval<stored_value&>().data()), "");
static_assert(noexcept(std::declval<const stored_value&>().data()), "");
static_assert(noexcept(std::declval<stored_value&>().size()), "");
static_assert(noexcept(std::declval<stored_value&>().length()), "");
static_assert(noexcept(std::declval<stored_value&>().empty()), "");
static_assert(noexcept(std::declval<stored_value&>().clear()), "");
static_assert(noexcept(
    std::declval<stored_value&>().swap(std::declval<stored_value&>())), "");
static_assert(noexcept(
    swap(std::declval<stored_value&>(), std::declval<stored_value&>())), "");

template <class Ch>
struct TestStoredValueNoModification : BaseTest
{};

typedef testing::Types<char, wchar_t, const char, const wchar_t> ChsAlsoConst;

TYPED_TEST_SUITE(TestStoredValueNoModification, ChsAlsoConst);

TYPED_TEST(TestStoredValueNoModification, Iterators)
{
    using char_t = TypeParam;
    using decayed_char_t = std::remove_const_t<char_t>;
    using string_t = std::basic_string<decayed_char_t>;
    using value_t = basic_stored_value<char_t>;

    const auto str = char_helper<decayed_char_t>::str;
    const auto str0 = char_helper<decayed_char_t>::str0;

    auto s = str0("strings");   // s.back() == '\0'
    value_t v(&s[0], &s[s.size() - 1]);
    const auto& cv = v;

    // Read from explicitly-const iterators
    {
        string_t copied(v.cbegin(), v.cend());
        ASSERT_EQ(str("strings"), copied);
    }
    {
        string_t copied(v.crbegin(), v.crend());
        ASSERT_EQ(str("sgnirts"), copied);
    }

    // Read from implicitly-const iterators
    {
        string_t copied(cv.begin(), cv.end());
        ASSERT_EQ(str("strings"), copied);
    }
    {
        string_t copied(cv.rbegin(), cv.rend());
        ASSERT_EQ(str("sgnirts"), copied);
    }
}

TYPED_TEST(TestStoredValueNoModification, Empty)
{
    using char_t = TypeParam;
    using decayed_char_t = std::remove_const_t<char_t>;
    using value_t = basic_stored_value<char_t>;

    const auto str0 = char_helper<decayed_char_t>::str0;

    auto s1 = str0("");     // s1.front()|s1.back() == '\0'
    value_t v(&s1[0], &s1[0]);
    const auto& cv = v;

    ASSERT_TRUE(v.empty());
    ASSERT_EQ(0U, v.size());
    ASSERT_EQ(0U, v.length());
    ASSERT_TRUE(v.begin() == v.end());
    ASSERT_TRUE(v.cbegin() == v.cend());
    ASSERT_TRUE(v.rbegin() == v.rend());
    ASSERT_TRUE(v.crbegin() == v.crend());
    ASSERT_TRUE(cv.begin() == cv.end());
    ASSERT_TRUE(cv.rbegin() == cv.rend());
}

TYPED_TEST(TestStoredValueNoModification, Relations)
{
    using char_t = TypeParam;
    using decayed_char_t = std::remove_const_t<char_t>;
    using string_t = std::basic_string<decayed_char_t>;
    using value_t = basic_stored_value<char_t>;

    const auto str = char_helper<decayed_char_t>::str;

    std::vector<std::pair<const char*, const char*>> pairs = {
        { "plastic", "elastic" },       // same length, differ at front
        { "Maria", "Mario" },           // same length, differ at back
        { "galactic", "galactica" },    // have same prefix, lengths differ
        { "identical", "identical" },   // identical
        { "", "empty" }                 // empty
    };

    for (const auto& p : pairs) {
        string_t s1 = str(p.first);
        string_t s2 = str(p.second);
        string_t s01 = s1 + char_t();
        string_t s02 = s2 + char_t();
        value_t v1(&s01[0], &s01[s01.size() - 1]);
        value_t v2(&s02[0], &s02[s02.size() - 1]);

        // stored_value vs stored_value
        ASSERT_EQ(s1 == s2, v1 == v2) << s1 << " == " << s2;
        ASSERT_EQ(s1 != s2, v1 != v2) << s1 << " != " << s2;
        ASSERT_EQ(s2 == s1, v2 == v1) << s2 << " == " << s1;
        ASSERT_EQ(s2 != s1, v2 != v1) << s2 << " != " << s1;
        ASSERT_EQ(s1 < s2, v1 < v2) << s1 << " < " << s2;
        ASSERT_EQ(s1 > s2, v1 > v2) << s1 << " > " << s2;
        ASSERT_EQ(s1 <= s2, v1 <= v2) << s1 << " <= " << s2;
        ASSERT_EQ(s1 >= s2, v1 >= v2) << s1 << " >= " << s2;
        ASSERT_EQ(s2 < s1, v2 < v1) << s2 << " < " << s1;
        ASSERT_EQ(s2 > s1, v2 > v1) << s2 << " > " << s1;
        ASSERT_EQ(s2 <= s1, v2 <= v1) << s2 << " <= " << s1;
        ASSERT_EQ(s2 >= s1, v2 >= v1) << s2 << " >= " << s1;

        // stored_value vs string
        ASSERT_EQ(s1 == s2, v1 == s2) << s1 << " == " << s2;
        ASSERT_EQ(s1 != s2, v1 != s2) << s1 << " != " << s2;
        ASSERT_EQ(s2 == s1, v2 == s1) << s2 << " == " << s1;
        ASSERT_EQ(s2 != s1, v2 != s1) << s2 << " != " << s1;
        ASSERT_EQ(s1 < s2, v1 < s2) << s1 << " < " << s2;
        ASSERT_EQ(s1 > s2, v1 > s2) << s1 << " > " << s2;
        ASSERT_EQ(s1 <= s2, v1 <= s2) << s1 << " <= " << s2;
        ASSERT_EQ(s1 >= s2, v1 >= s2) << s1 << " >= " << s2;
        ASSERT_EQ(s2 < s1, v2 < s1) << s2 << " < " << s1;
        ASSERT_EQ(s2 > s1, v2 > s1) << s2 << " > " << s1;
        ASSERT_EQ(s2 <= s1, v2 <= s1) << s2 << " <= " << s1;
        ASSERT_EQ(s2 >= s1, v2 >= s1) << s2 << " >= " << s1;

        // string vs stored_value
        ASSERT_EQ(s1 == s2, s1 == v2) << s1 << " == " << s2;
        ASSERT_EQ(s1 != s2, s1 != v2) << s1 << " != " << s2;
        ASSERT_EQ(s2 == s1, s2 == v1) << s2 << " == " << s1;
        ASSERT_EQ(s2 != s1, s2 != v1) << s2 << " != " << s1;
        ASSERT_EQ(s1 < s2, s1 < v2) << s1 << " < " << s2;
        ASSERT_EQ(s1 > s2, s1 > v2) << s1 << " > " << s2;
        ASSERT_EQ(s1 <= s2, s1 <= v2) << s1 << " <= " << s2;
        ASSERT_EQ(s1 >= s2, s1 >= v2) << s1 << " >= " << s2;
        ASSERT_EQ(s2 < s1, s2 < v1) << s2 << " < " << s1;
        ASSERT_EQ(s2 > s1, s2 > v1) << s2 << " > " << s1;
        ASSERT_EQ(s2 <= s1, s2 <= v1) << s2 << " <= " << s1;
        ASSERT_EQ(s2 >= s1, s2 >= v1) << s2 << " >= " << s1;

        // stored_value vs NTBS
        ASSERT_EQ(s1 == s2, v1 == s2.c_str()) << s1 << " == " << s2;
        ASSERT_EQ(s1 != s2, v1 != s2.c_str()) << s1 << " != " << s2;
        ASSERT_EQ(s2 == s1, v2 == s1.c_str()) << s2 << " == " << s1;
        ASSERT_EQ(s2 != s1, v2 != s1.c_str()) << s2 << " != " << s1;
        ASSERT_EQ(s1 < s2, v1 < s2.c_str()) << s1 << " < " << s2;
        ASSERT_EQ(s1 > s2, v1 > s2.c_str()) << s1 << " > " << s2;
        ASSERT_EQ(s1 <= s2, v1 <= s2.c_str()) << s1 << " <= " << s2;
        ASSERT_EQ(s1 >= s2, v1 >= s2.c_str()) << s1 << " >= " << s2;
        ASSERT_EQ(s2 < s1, v2 < s1.c_str()) << s2 << " < " << s1;
        ASSERT_EQ(s2 > s1, v2 > s1.c_str()) << s2 << " > " << s1;
        ASSERT_EQ(s2 <= s1, v2 <= s1.c_str()) << s2 << " <= " << s1;
        ASSERT_EQ(s2 >= s1, v2 >= s1.c_str()) << s2 << " >= " << s1;

        // NTBS vs stored_value
        ASSERT_EQ(s1 == s2, s1.c_str() == v2) << s1 << " == " << s2;
        ASSERT_EQ(s1 != s2, s1.c_str() != v2) << s1 << " != " << s2;
        ASSERT_EQ(s2 == s1, s2.c_str() == v1) << s2 << " == " << s1;
        ASSERT_EQ(s2 != s1, s2.c_str() != v1) << s2 << " != " << s1;
        ASSERT_EQ(s1 < s2, s1.c_str() < v2) << s1 << " < " << s2;
        ASSERT_EQ(s1 > s2, s1.c_str() > v2) << s1 << " > " << s2;
        ASSERT_EQ(s1 <= s2, s1.c_str() <= v2) << s1 << " <= " << s2;
        ASSERT_EQ(s1 >= s2, s1.c_str() >= v2) << s1 << " >= " << s2;
        ASSERT_EQ(s2 < s1, s2.c_str() < v1) << s2 << " < " << s1;
        ASSERT_EQ(s2 > s1, s2.c_str() > v1) << s2 << " > " << s1;
        ASSERT_EQ(s2 <= s1, s2.c_str() <= v1) << s2 << " <= " << s1;
        ASSERT_EQ(s2 >= s1, s2.c_str() >= v1) << s2 << " >= " << s1;
    }
}

TYPED_TEST(TestStoredValueNoModification, Strings)
{
    using char_t = TypeParam;
    using decayed_char_t = std::remove_const_t<char_t>;
    using string_t =std::basic_string<decayed_char_t>;
    using value_t = basic_stored_value<char_t>;

    const auto str0 = char_helper<decayed_char_t>::str0;

    auto s = str0("x-ray");   // s.back() == '\0'
    value_t v(&s[0], &s[s.size() - 1]);
    const auto& cv = v;

    string_t str(cv);
    ASSERT_EQ(str, v);

    string_t str2 = to_string(cv);
    ASSERT_EQ(str2, v);
}

TYPED_TEST(TestStoredValueNoModification, Sizes)
{
    using char_t = TypeParam;
    using decayed_char_t = std::remove_const_t<char_t>;
    using value_t = basic_stored_value<char_t>;

    const auto str0 = char_helper<decayed_char_t>::str0;

    auto s = str0("obscura");   // s.back() == '\0'
    value_t v(&s[0], &s[s.size() - 1]);
    const auto& cv = v;

    ASSERT_EQ(s.size() - 1, cv.size());
    ASSERT_EQ(s.length() - 1, cv.size());
    ASSERT_FALSE(cv.empty());
}

TYPED_TEST(TestStoredValueNoModification, RelationsSpecial)
{
    using char_t = TypeParam;
    using decayed_char_t = std::remove_const_t<char_t>;
    using string_t = std::basic_string<decayed_char_t>;
    using value_t = basic_stored_value<char_t>;

    const auto ch = char_helper<decayed_char_t>::ch;

    string_t s0 = { ch('a'), ch('b'), ch('c'), ch('\0'),
                    ch('d'), ch('e'), ch('f') };
    string_t s = s0 + ch('\0');
    ASSERT_EQ(8U, s.size()) << "Test's precondition";
    value_t v = value_t(&s[0], &s[0] + s.size() - 1);
    ASSERT_EQ(7U, v.size()) << "Test's precondition";

    ASSERT_TRUE(v == s0);
    ASSERT_FALSE(v == s0.c_str());  // "abc\0def" vs "abc"
    ASSERT_TRUE(v > s0.c_str());    // ditto
}

TYPED_TEST(TestStoredValueNoModification, FrontBack)
{
    using char_t = TypeParam;
    using decayed_char_t = std::remove_const_t<char_t>;
    using value_t = basic_stored_value<char_t>;

    const auto ch = char_helper<decayed_char_t>::ch;
    const auto str0 = char_helper<decayed_char_t>::str0;

    auto s = str0("mars");  // s.back() == '\0'
    value_t v(&s[0], &s[s.size() - 1]);
    const auto& cv = v;

    ASSERT_EQ(ch('m'), v.front());
    ASSERT_EQ(ch('m'), cv.front());
    ASSERT_EQ(ch('s'), v.back());
    ASSERT_EQ(ch('s'), cv.back());
}

TYPED_TEST(TestStoredValueNoModification, IndexAccess)
{
    using char_t = TypeParam;
    using decayed_char_t = std::remove_const_t<char_t>;
    using value_t = basic_stored_value<char_t>;

    const auto ch = char_helper<decayed_char_t>::ch;
    const auto str0 = char_helper<decayed_char_t>::str0;

    auto s = str0("string");    // s.back() == '\0'
    value_t v(&s[0], &s[s.size() - 1]);
    const auto& cv = v;

    ASSERT_EQ(ch('s'), v[0]);
    ASSERT_EQ(ch('t'), cv[1]);
    ASSERT_EQ(ch('\0'), cv[v.size()]);  // OK
}

TYPED_TEST(TestStoredValueNoModification, At)
{
    using char_t = TypeParam;
    using decayed_char_t = std::remove_const_t<char_t>;
    using value_t = basic_stored_value<char_t>;

    const auto ch = char_helper<decayed_char_t>::ch;
    const auto str0 = char_helper<decayed_char_t>::str0;

    auto s = str0("strings");   // s.back() == '\0'
    value_t v(&s[0], &s[s.size() - 1]);
    const auto& cv = v;

    ASSERT_EQ(ch('s'), v.at(0));
    ASSERT_EQ(ch('t'), cv.at(1));
    ASSERT_EQ(ch('s'), cv.at(v.size() - 1));
    ASSERT_THROW(v.at(v.size()), std::out_of_range);
    ASSERT_THROW(
        cv.at(static_cast<typename value_t::size_type>(-1)),
        std::out_of_range);
}

TYPED_TEST(TestStoredValueNoModification, Data)
{
    using char_t = TypeParam;
    using decayed_char_t = std::remove_const_t<char_t>;
    using value_t = basic_stored_value<char_t>;

    const auto str0 = char_helper<decayed_char_t>::str0;

    auto s = str0("string");    // s.back() == '\0'
    value_t v(&s[0], &s[s.size() - 1]);
    const auto& cv = v;

    ASSERT_EQ(v.begin(), v.c_str());
    ASSERT_EQ(v.begin(), v.data());
    ASSERT_EQ(cv.begin(), cv.c_str());
    ASSERT_EQ(cv.begin(), cv.data());
}

TYPED_TEST(TestStoredValueNoModification, Swap)
{
    using char_t = TypeParam;
    using decayed_char_t = std::remove_const_t<char_t>;
    using value_t = basic_stored_value<char_t>;

    const auto str0 = char_helper<decayed_char_t>::str0;

    auto s1 = str0("swap");
    auto s2 = str0("wasp");
    value_t v1(&s1[0], &s1[s1.size() - 1]);
    value_t v2(&s2[0], &s2[s2.size() - 1]);

    const auto b1 = v1.cbegin();
    const auto b2 = v2.cbegin();

    v1.swap(v2);
    ASSERT_EQ(b1, v2.cbegin());
    ASSERT_EQ(b2, v1.cbegin());

    swap(v1, v2);
    ASSERT_EQ(b1, v1.cbegin());
    ASSERT_EQ(b2, v2.cbegin());
}

TYPED_TEST(TestStoredValueNoModification, Plus)
{
    using char_t = TypeParam;
    using decayed_char_t = std::remove_const_t<char_t>;
    using value_t = basic_stored_value<char_t>;

    const auto str = char_helper<decayed_char_t>::str;

    auto s2 = str("xyz");
    auto s4 = str("789");

    ASSERT_EQ(str("xyz789"), s2 += value_t(&s4[0], &s4[0] + s4.size()));
}

TYPED_TEST(TestStoredValueNoModification, Write)
{
    using namespace std;
    using char_t = TypeParam;
    using decayed_char_t = std::remove_const_t<char_t>;
    using value_t = basic_stored_value<char_t>;
    using string_t = basic_string<decayed_char_t>;
    using stream_t = basic_stringstream<decayed_char_t>;

    const auto ch = char_helper<decayed_char_t>::ch;
    const auto str = char_helper<decayed_char_t>::str;

    string_t s = str("write");
    string_t s0 = s + char_t();
    const value_t v(&s0[0], &s0[s0.size() - 1]);

    stream_t o1;
    o1 << setfill(ch('_')) << setw(10) << s
       << setfill(ch('*')) << left << setw(8) << s
       << setfill(ch('+')) << setw(4) << s
       << 10;
    stream_t o2;
    o2 << setfill(ch('_')) << setw(10) << v
       << setfill(ch('*')) << left << setw(8) << v
       << setfill(ch('+')) << setw(4) << v
       << 10;
    ASSERT_EQ(o1.str(), o2.str());
}

template <class Ch>
struct TestStoredValue : BaseTest
{};

typedef testing::Types<char, wchar_t> Chs;

TYPED_TEST_SUITE(TestStoredValue, Chs);

TYPED_TEST(TestStoredValue, Iterators)
{
    using char_t = TypeParam;
    using decayed_char_t = std::remove_const_t<char_t>;
    using value_t = basic_stored_value<char_t>;

    const auto ch = char_helper<decayed_char_t>::ch;
    const auto str = char_helper<decayed_char_t>::str;
    const auto str0 = char_helper<decayed_char_t>::str0;

    auto s = str0("strings");   // s.back() == '\0'
    value_t v(&s[0], &s[s.size() - 1]);

    // Write through non-const iterators
    v.begin()[3] = ch('a');
    v.rbegin()[0] = ch('e');
    ASSERT_EQ(str("strange"), v);
}

TYPED_TEST(TestStoredValue, FrontBack)
{
    using char_t = TypeParam;
    using value_t = basic_stored_value<char_t>;

    const auto str = char_helper<char_t>::str;
    const auto str0 = char_helper<char_t>::str0;

    auto s = str0("mars");  // s.back() == '\0'
    value_t v(&s[0], &s[s.size() - 1]);

    v.front() = 'c';
    v.back() = 'e';

    ASSERT_EQ(str("care"), v);
}

TYPED_TEST(TestStoredValue, Pop)
{
    using char_t = TypeParam;
    using value_t = basic_stored_value<char_t>;

    const auto str = char_helper<char_t>::str;
    const auto str0 = char_helper<char_t>::str0;

    auto s = str0("hamburger");     // s.back() == '\0'
    value_t v(&s[0], &s[s.size() - 1]);

    v.pop_front();  // "amburger"
    v.pop_front();  // "mburger"
    v.pop_front();  // "burger"
    v.pop_front();  // "urger"
    v.pop_back();   // "urge"
    ASSERT_EQ(str("urge"), v);
}

TYPED_TEST(TestStoredValue, Erase)
{
    using char_t = TypeParam;
    using value_t = basic_stored_value<char_t>;

    const auto ch = char_helper<char_t>::ch;
    const auto str = char_helper<char_t>::str;
    const auto str0 = char_helper<char_t>::str0;

    auto s = str0("hamburger");     // s.back() == '\0'
    value_t v(&s[0], &s[s.size() - 1]);

    ASSERT_EQ(
        ch('a'),
        *v.erase(v.cbegin()));                      // "amburger"
    ASSERT_EQ(str("amburger"), v);

    ASSERT_EQ(
        ch('e'),
        *v.erase(v.cbegin() + 3, v.cbegin() + 6));  // "amber"
    ASSERT_EQ(str("amber"), v);

    ASSERT_EQ(
        ch('r'),
        *v.erase(v.cbegin() + 1, v.cbegin() + 4));  // "ar"
    ASSERT_EQ(str("ar"), v);

    const auto e = v.erase(v.cend() - 1);           // "r"
    ASSERT_EQ(v.cend(), e);
    ASSERT_EQ(str("a"), v);

    v.clear();
    ASSERT_EQ(str(""), v);
    ASSERT_TRUE(v.empty());
}

TYPED_TEST(TestStoredValue, EraseByIndex)
{
    using char_t = TypeParam;
    using value_t = basic_stored_value<char_t>;

    const auto str = char_helper<char_t>::str;
    const auto str0 = char_helper<char_t>::str0;

    auto s = str0("latter");        // s.back() == '\0'
    value_t v(&s[0], &s[s.size() - 1]);

    ASSERT_NO_THROW(v.erase(6));
    ASSERT_THROW(v.erase(7), std::out_of_range);

    ASSERT_EQ(
        &v,
        &v.erase(2, 1));            // "later"
    ASSERT_EQ(str("later"), v);

    ASSERT_EQ(
        &v,
        &v.erase(4));               // "late"
    ASSERT_EQ(str("late"), v);
}

TYPED_TEST(TestStoredValue, IndexAccess)
{
    using char_t = TypeParam;
    using value_t = basic_stored_value<char_t>;

    const auto ch = char_helper<char_t>::ch;
    const auto str = char_helper<char_t>::str;
    const auto str0 = char_helper<char_t>::str0;

    auto s = str0("string");    // s.back() == '\0'
    value_t v(&s[0], &s[s.size() - 1]);

    v.at(3) = ch('o');
    ASSERT_EQ(str("strong"), v);
}

TYPED_TEST(TestStoredValue, At)
{
    using char_t = TypeParam;
    using value_t = basic_stored_value<char_t>;

    const auto ch = char_helper<char_t>::ch;
    const auto str = char_helper<char_t>::str;
    const auto str0 = char_helper<char_t>::str0;

    auto s = str0("strings");   // s.back() == '\0'
    value_t v(&s[0], &s[s.size() - 1]);

    v.at(3) = ch('a');
    v.at(6) = ch('e');
    ASSERT_EQ(str("strange"), v);
}

TYPED_TEST(TestStoredValue, Data)
{
    using char_t = TypeParam;
    using value_t = basic_stored_value<char_t>;

    const auto str = char_helper<char_t>::str;
    const auto str0 = char_helper<char_t>::str0;

    auto s = str0("string");    // s.back() == '\0'
    value_t v(&s[0], &s[s.size() - 1]);
    const auto& cv = v;

    v.c_str()[3] = 'o';
    ASSERT_EQ(str("strong"), cv);

    v.data()[1] = 'w';
    v.pop_front();
    ASSERT_EQ(str("wrong"), cv);
}

namespace privy {

using store_t = detail::stored::table_store<char, std::allocator<char>>;

static_assert(std::is_default_constructible<store_t>::value, "");
static_assert(std::is_nothrow_move_constructible<store_t>::value, "");
static_assert(noexcept(std::declval<store_t&>().secure_any(0)), "");
static_assert(noexcept(std::declval<store_t&>().clear()), "");

}

class TestTableStore : public BaseTest
{};

TEST_F(TestTableStore, Basics)
{
    using store_t = detail::stored::table_store<char, std::allocator<char>>;

    store_t store;

    // Add one buffer
    char* const buffer1 = store.get_allocator().allocate(10);
    store.add_buffer(buffer1, 10);
    ASSERT_EQ(store_t::security{ buffer1 }, store.get_security());

    // Secure on the first buffer
    ASSERT_EQ(buffer1, store.secure_any(4));
    ASSERT_EQ(store_t::security{ buffer1 + 4 }, store.get_security());
    ASSERT_EQ(buffer1 + 4, store.secure_any(6));
    store.secure_current_upto(buffer1 + 4);
    ASSERT_EQ(nullptr, store.secure_any(7));

    // Add another buffer and secure on it
    char* const buffer2 = store.get_allocator().allocate(15);
    store.add_buffer(buffer2, 15);
    ASSERT_EQ(buffer2, store.secure_any(7));

    store.clear();
    ASSERT_TRUE(store.get_security().empty());
}

TEST_F(TestTableStore, Merge)
{
    using store_t = detail::stored::
        table_store<wchar_t, std::allocator<wchar_t>>;

    store_t store1;
    wchar_t* const buffer1 = store1.get_allocator().allocate(10);
    store1.add_buffer(buffer1, 10);
    store1.secure_any(6);
    ASSERT_EQ(nullptr, store1.secure_any(10));
    ASSERT_EQ(store_t::security{ buffer1 + 6 }, store1.get_security());

    store_t store2;
    wchar_t* const buffer2 = store2.get_allocator().allocate(15);
    store2.add_buffer(buffer2, 15);
    store2.secure_any(4);

    store1.merge(std::move(store2));
    {
        store_t::security expected = { buffer1 + 6, buffer2 + 4 };
        ASSERT_EQ(expected, store1.get_security());
    }
    ASSERT_EQ(buffer2 + 4, store1.secure_any(10));
    {
        store_t::security expected = { buffer1 + 6, buffer2 + 14 };
        ASSERT_EQ(expected, store1.get_security());
    }

    {
        store_t::security s = { buffer1 + 6, buffer2 + 10 };
        store1.set_security(s);
        ASSERT_EQ(s, store1.get_security());
    }
}

TEST_F(TestTableStore, Swap)
{
    using store_t = detail::stored::table_store<char, std::allocator<char>>;

    store_t store1;
    char* const buffer11 = store1.get_allocator().allocate(3);
    char* const buffer12 = store1.get_allocator().allocate(3);
    std::strcpy(buffer11, "AB");
    std::strcpy(buffer12, "ab");
    store1.add_buffer(buffer11, 3);
    store1.add_buffer(buffer12, 3);

    store_t store2;
    char* const buffer21 = store2.get_allocator().allocate(3);
    char* const buffer22 = store2.get_allocator().allocate(3);
    std::strcpy(buffer21, "XY");
    std::strcpy(buffer22, "xy");
    store2.add_buffer(buffer21, 3);
    store2.add_buffer(buffer22, 3);

    store_t::security expected1 = { buffer12, buffer11 };
    store_t::security expected2 = { buffer22, buffer21 };

    store1.swap(store2);
    ASSERT_EQ(expected2, store1.get_security());
    ASSERT_EQ(expected1, store2.get_security());

    swap(store1, store2);
    ASSERT_EQ(expected1, store1.get_security());
    ASSERT_EQ(expected2, store2.get_security());
}

static_assert(std::is_default_constructible<stored_table>::value, "");
static_assert(std::is_nothrow_move_constructible<stored_table>::value, "");

struct TestStoredTable : BaseTest
{};

TEST_F(TestStoredTable, ResizeValue)
{
    stored_table table;
    stored_value v;

    {
        const auto& rv = table.resize_value(v, 5);
        ASSERT_EQ(std::addressof(v), std::addressof(rv));
    }
    ASSERT_EQ(5, v.size());
    for (std::size_t i = 0; i < 6; ++i) {
        ASSERT_EQ('\0', v[i]) << i;
    }

    std::strcpy(v.c_str(), "abyss");

    {
        const auto p = &v[0];
        const auto& rv = table.resize_value(v, 2);
        ASSERT_EQ(std::addressof(v), std::addressof(rv));
        ASSERT_EQ(p, &v[0]);    // no reallocation
    }
    ASSERT_EQ(2, v.size());
    ASSERT_STREQ("ab", v.c_str());

    {
        const auto p = &v[0];
        const auto& rv = table.resize_value(v, 6);
        ASSERT_EQ(std::addressof(v), std::addressof(rv));
        ASSERT_NE(&v[0], p);    // reallocation
    }
    ASSERT_EQ(6, v.size());
    ASSERT_STREQ("ab", v.c_str());
    for (std::size_t i = 3; i < 6; ++i) {
        ASSERT_EQ('\0', v[i]) << i;
    }
}

TEST_F(TestStoredTable, MakeValue)
{
    stored_table table;
    stored_value v = table.make_value(8);
    std::strcpy(v.c_str(), "aboard");
    ASSERT_EQ(8, v.size());
    ASSERT_STREQ("aboard", v.c_str());
    for (std::size_t i = 6; i < 9; ++i) {
        ASSERT_EQ('\0', v[i]) << i;
    }
}

TEST_F(TestStoredTable, RewriteValueBasics)
{
    wstored_table table(10U);

    // First record
    table.content().emplace_back();
    table[0].resize(2);

    // Consumes 5 chars
    table.rewrite_value(table[0][0], L"star");
    ASSERT_EQ(L"star", table[0][0]);
    const auto v = table[0][0];

    // In-place contraction is OK
    table.rewrite_value(table[0][0], L"sun");
    ASSERT_EQ(L"sun", table[0][0]);
    ASSERT_TRUE(
        (v.cbegin() <= table[0][0].cbegin())
     && (table[0][0].cend() <= v.cend()));

    // Expansion to 5 chars is fulfilled by comsuming next spaces
    std::wstring moon(L"moon");
    table[0][0] = table.import_value(moon.c_str(), moon.c_str() + 4);
    ASSERT_EQ(L"moon", table[0][0]);
    ASSERT_EQ(v.cbegin() + 5, table[0][0].c_str());

    // Consume another buffer by 5 chars
    table.rewrite_value(table[0][1], table[0][0]);
    ASSERT_EQ(L"moon", table[0][1]);
    ASSERT_TRUE(
        (table[0][0].cend() < v.cbegin())
     || (v.cend() <= table[0][0].cbegin()));
}

TEST_F(TestStoredTable, RewriteValueWithNonPointerIterator)
{
    stored_table table;
    table.content().emplace_back();
    table.content().back().emplace_back();

    std::vector<char> v = { '\0', 'C', 'B', 'A' };

    table.rewrite_value(table.content().back().back(), v.crbegin());
    ASSERT_EQ("ABC", table.content().back().back());
}

namespace {

struct d_end
{};

bool operator!=(const wchar_t* left, d_end)
{
    return *left != L'd';
}

}

TEST_F(TestStoredTable, RewriteValueWithPointerAndSentinel)
{
    wstored_table table;
    table.content().emplace_back();
    table.content().back().emplace_back();

    table.rewrite_value(table.content().back().back(), L"abcdefg", d_end());
    ASSERT_EQ(L"abc", table.content().back().back());
}

TEST_F(TestStoredTable, RewriteValueWithNonPointerIteratorAndSentinel)
{
    wstored_table table;
    table.content().emplace_back();
    table.content().back().emplace_back();

    std::list<wchar_t> v = { L'A', L'B', L'C', L'\0' };

    table.rewrite_value(table.content().back().back(), v.cbegin());
    ASSERT_EQ(L"ABC", table.content().back().back());
}

TEST_F(TestStoredTable, Copy)
{
    stored_table table1(10U);
    table1.content().emplace_back(2U);
    table1.content().emplace_back(1U);
    table1.rewrite_value(table1[0][0], "sky");              // 4 chars
    table1.rewrite_value(table1[0][1], "anaesthesia");      // 11 chars
    table1.rewrite_value(table1[1][0], "catalogue");        // 9 chars
    table1[1][0].erase(
        table1[1][0].cbegin() + 3, table1[1][0].cend());    // "cat"

    // Copy ctor

    stored_table table2(table1);

    ASSERT_EQ(2U, table2.size());
    ASSERT_EQ(2U, table2[0].size());
    ASSERT_EQ(1U, table2[1].size());
    ASSERT_EQ("sky", table2[0][0]);
    ASSERT_EQ("anaesthesia", table2[0][1]);
    ASSERT_EQ("cat", table2[1][0]);

    // In table2, "cat" is placed in the first buffer,
    // just after "sky"
    ASSERT_EQ(table2[0][0].cend() + 1, table2[1][0].cbegin());

    // Shrink to fit

    table1.shrink_to_fit();

    // Compacted just like table2
    ASSERT_EQ(table1[0][0].cend() + 1, table1[1][0].cbegin());

    // Copy assignment

    table2.content().pop_front();

    table1 = table2;
    table2.clear();

    ASSERT_EQ(1U, table1.size());
    ASSERT_EQ(1U, table1[0].size());
    ASSERT_EQ("cat", table1[0][0]);
}

TEST_F(TestStoredTable, Move)
{
    stored_table table1;
    table1.content().emplace_back(1U);
    table1.rewrite_value(table1[0][0], "table");

    const auto content = &table1.content();
    const auto record0 = &table1[0];
    const auto value00 = &table1[0][0];
    const auto char000 = &table1[0][0][0];

    // Move ctor

    stored_table table2(std::move(table1));

    ASSERT_TRUE(table1.empty());
    ASSERT_EQ(table1.size(), 0U);

    ASSERT_EQ(1U, table2.size());
    ASSERT_EQ(1U, table2[0].size());
    ASSERT_EQ("table", table2[0][0]);

    ASSERT_EQ(content, &table2.content());
    ASSERT_EQ(record0, &table2[0]);
    ASSERT_EQ(value00, &table2[0][0]);
    ASSERT_EQ(char000, &table2[0][0][0]);

    // Move assignment

    stored_table table3;

    table3 = std::move(table2);

    ASSERT_TRUE(table2.empty());
    ASSERT_EQ(table2.size(), 0U);
    table2.clear();

    ASSERT_EQ(1U, table3.size());
    ASSERT_EQ(1U, table3[0].size());
    ASSERT_EQ("table", table3[0][0]);

    ASSERT_EQ(content, &table3.content());
    ASSERT_EQ(record0, &table3[0]);
    ASSERT_EQ(value00, &table3[0][0]);
    ASSERT_EQ(char000, &table3[0][0][0]);
}

TEST_F(TestStoredTable, WithMovedFrom)
{
    wstored_table table1;
    table1.content().emplace_back(1U);
    table1.rewrite_value(table1[0][0], L"table");

    const auto char000 = &table1[0][0][0];

    wstored_table table2(std::move(table1));

    ASSERT_TRUE(table1.empty()) << "Test's precondition";

    // Copy ctor with moved-from

    wstored_table table3(table1);
    ASSERT_TRUE(table3.empty());

    // Move ctor with moved-from

    wstored_table table4(table3);
    ASSERT_TRUE(table3.empty());
    ASSERT_TRUE(table4.empty());

    // Copy assignment from moved-from

    wstored_table table5;
    table5 = table3;
    ASSERT_TRUE(table5.empty());

    // Move assignment from moved-from

    wstored_table table6;
    table6 = std::move(table4);
    ASSERT_TRUE(table4.empty());
    ASSERT_TRUE(table6.empty());

    // Swap with moved-from

    swap(table2, table6);
    ASSERT_TRUE (table2.empty());
    ASSERT_FALSE(table6.empty());
    ASSERT_EQ(char000, &table6[0][0][0]);
}

TEST_F(TestStoredTable, MergeLists)
{
    basic_stored_table<std::list<std::vector<stored_value>>> table1(10U);
    table1.content().emplace_back();
    table1.content().back().emplace_back();
    table1.rewrite_value(table1.content().back().back(), "apples");

    basic_stored_table<std::list<std::vector<stored_value>>> table2(10U);
    table2.content().emplace_back();
    table2.content().back().emplace_back();
    table2.rewrite_value(table2.content().back().back(), "oranges");

    const auto field100 = &table1.content().front().front();
    const auto field200 = &table2.content().front().front();

    const auto table3 = std::move(table1) + std::move(table2);
    ASSERT_EQ(2U, table3.size());
    ASSERT_EQ(1U, table3.content().front().size());
    ASSERT_EQ("apples", table3.content().front().front());
    ASSERT_EQ(1U, table3.content().back().size());
    ASSERT_EQ("oranges", table3.content().back().front());

    // merger of lists shall be done by splicing,
    // so addesses of csv_values shall not be modified
    ASSERT_EQ(field100, &table3.content().front().front());
    ASSERT_EQ(field200, &table3.content().back().front());
}

template <class ContentLR>
struct TestStoredTableMerge : BaseTest
{};

typedef testing::Types<
    std::pair<
        std::vector<std::vector<stored_value>>,
        std::deque<std::vector<stored_value>>>,
    std::pair<
        std::deque<std::deque<stored_value>>,
        std::deque<std::vector<stored_value>>>,
    std::pair<
        std::list<std::deque<stored_value>>,
        std::deque<std::vector<stored_value>>>,
    std::pair<
        std::list<std::vector<stored_value>>,
        std::deque<std::vector<stored_value>>>> ContentLRs;

TYPED_TEST_SUITE(TestStoredTableMerge, ContentLRs);

TYPED_TEST(TestStoredTableMerge, Merge)
{
    basic_stored_table<typename TypeParam::first_type> table1(20U);
    table1.content().emplace_back();
    table1.content().begin()->resize(3);
    table1.rewrite_value((*table1.content().begin())[0], "Lorem");
    table1.rewrite_value((*table1.content().begin())[1], "ipsum");
    table1.rewrite_value((*table1.content().begin())[2], "dolor");

    basic_stored_table<typename TypeParam::second_type> table2(25U);
    table2.content().resize(2);
    table2.content().begin() ->resize(2);
    table2.content().rbegin()->resize(1);
    table2.rewrite_value((*table2.content().begin()) [0], "sit");
    table2.rewrite_value((*table2.content().begin()) [1], "amet,");
    table2.rewrite_value((*table2.content().rbegin())[0], "consectetur");

    table1 += std::move(table2);
    ASSERT_EQ(3U, table1.size());
    ASSERT_EQ("Lorem"      , (*table1.content().cbegin())           [0]);
    ASSERT_EQ("ipsum"      , (*table1.content().cbegin())           [1]);
    ASSERT_EQ("dolor"      , (*table1.content().cbegin())           [2]);
    ASSERT_EQ("sit"        , (*std::next(table1.content().cbegin()))[0]);
    ASSERT_EQ("amet,"      , (*std::next(table1.content().cbegin()))[1]);
    ASSERT_EQ("consectetur", (*table1.content().crbegin())          [0]);
}

TYPED_TEST(TestStoredTableMerge, WithMovedFrom)
{
    using t1_t = basic_stored_table<typename TypeParam::first_type>;
    using t2_t = basic_stored_table<typename TypeParam::second_type>;

    t1_t table1;

    t2_t table2;
    table2.content().emplace_back();
    table2.content().back().emplace_back();
    table2.content().back().back() = table2.import_value("value");
    const auto char000 = &table2.content().back().back().back();

    {
        t1_t table3(std::move(table1));
        ASSERT_TRUE(table1.empty()) << "Test's precondition";
    }

    table2 += table1;
    ASSERT_EQ(1U, table2.size());
    ASSERT_EQ(1U, table2.content().back().size());
    ASSERT_EQ("value", table2.content().back().back());
    ASSERT_EQ(char000, &table2.content().back().back().back());

    table2 += std::move(table1);
    ASSERT_EQ(1U, table2.size());
    ASSERT_EQ(1U, table2.content().back().size());
    ASSERT_EQ("value", table2.content().back().back());
    ASSERT_EQ(char000, &table2.content().back().back().back());

    table1 += std::move(table2);
    ASSERT_EQ(1U, table1.size());
    ASSERT_EQ(1U, table1.content().back().size());
    ASSERT_EQ("value", table1.content().back().back());
    ASSERT_EQ(char000, &table2.content().back().back().back());

    if (table2.empty()) {
        table2 += table1;
        ASSERT_EQ(1U, table2.size());
        ASSERT_EQ(1U, table2.content().back().size());
        ASSERT_EQ("value", table2.content().back().back());
        ASSERT_NE(char000, &table2.content().back().back().back());
    }
}

TYPED_TEST(TestStoredTableMerge, WithMovedFroms)
{
    using t1_t = basic_stored_table<typename TypeParam::first_type>;
    using t2_t = basic_stored_table<typename TypeParam::second_type>;

    t1_t table1;
    t2_t table2;

    {
        t1_t a(std::move(table1));
        t2_t b(std::move(table2));
        a += b; // dummy
    }

    table1 += table2;
    ASSERT_TRUE(table1.empty());
}

class TestStoredTableAllocator : public BaseTest
{
protected:
    template <class T>
    using TA = tracking_allocator<std::allocator<T>>;
};

TEST_F(TestStoredTableAllocator, Basics)
{
    using Record = std::vector<
        stored_value,
        TA<stored_value>>;
    using Content = std::deque<
        Record,
        std::scoped_allocator_adaptor<
            TA<Record>,
            Record::allocator_type>>;
    using Table = basic_stored_table<
        Content,
        std::scoped_allocator_adaptor<
            TA<Content>,
            TA<Record>,
            TA<stored_value>>>;

    std::vector<std::pair<char*, char*>> allocated2;
    TA<stored_value> a2(allocated2);

    std::vector<std::pair<char*, char*>> allocated1;
    TA<Record> a1(allocated1);

    std::vector<std::pair<char*, char*>> allocated0;
    TA<Content> a0(allocated0);

    Table::allocator_type a(a0, a1, a2);

    Table table(std::allocator_arg, a, 1024);
    {
        const char* s = "Col1,Col2\n"
                        "aaa,bbb,ccc\n"
                        "AAA,BBB,CCC\n";
        try {
            parse_csv(s, make_stored_table_builder(table));
        } catch (const text_error& e) {
            FAIL() << e.info();
        }
    }

    ASSERT_EQ(a1, table.content().get_allocator());
    ASSERT_EQ(a2, table.content().front().get_allocator());
    ASSERT_TRUE(a1.tracks(&table.content().front()));
    ASSERT_TRUE(a2.tracks(&table.content().front().front()));
    ASSERT_TRUE(a0.tracks(&table.content().front().front().front()));
    ASSERT_TRUE(a0.tracks(&table.content()));

    std::vector<std::pair<char*, char*>> bllocated2;
    Record::allocator_type b2(bllocated2);

    std::vector<std::pair<char*, char*>> bllocated1;
    TA<Record> b1(bllocated1);

    std::vector<std::pair<char*, char*>> bllocated0;
    TA<Content> b0(bllocated0);

    Table::allocator_type b(b0, b1, b2);

    Table table2(std::allocator_arg, b);
    {
        const char* s = "Col1,Col2\n"
                        "xxx,yyy\n";
        try {
            parse_csv(s, make_stored_table_builder(table2));
        } catch (const text_error& e) {
            FAIL() << e.info();
        }
        table2.content().pop_front();
    }

    table += std::move(table2); // Not move but copy because of allocators

    ASSERT_EQ(1U, table2.size());
    ASSERT_TRUE(b1.tracks(&table2[0]));
    ASSERT_TRUE(b2.tracks(&table2[0].front()));
    ASSERT_TRUE(b0.tracks(&table2[0].front().front()));
    ASSERT_TRUE(b0.tracks(&table2.content()));

    table2.clear();

    ASSERT_EQ(4U, table.size());
    ASSERT_TRUE(a1.tracks(&table[3]));
    ASSERT_TRUE(a2.tracks(&table[3].front()));
    ASSERT_TRUE(a0.tracks(&table[3].front().front()));
}

typedef testing::Types<
    std::tuple<std::true_type, std::true_type, std::true_type>,
    std::tuple<std::true_type, std::true_type, std::false_type>,
    std::tuple<std::true_type, std::false_type, std::true_type>,
    std::tuple<std::true_type, std::false_type, std::false_type>,
    std::tuple<std::false_type, std::true_type, std::true_type>,
    std::tuple<std::false_type, std::true_type, std::false_type>,
    std::tuple<std::false_type, std::false_type, std::true_type>,
    std::tuple<std::false_type, std::false_type, std::false_type>
> TF3;

template <class T>
struct TestStoredTableAllocatorPropagation : BaseTest
{
    template <class Content, class Allocator>
    static void init_table(basic_stored_table<Content, Allocator>& table)
    {
        table.content().resize(1);
        table[0].resize(1);
        table[0][0] = table.import_value("ABC");
    }
};

TYPED_TEST_SUITE(TestStoredTableAllocatorPropagation, TF3);

TYPED_TEST(TestStoredTableAllocatorPropagation, CopyAssignment)
{
    const auto pocca = std::tuple_element_t<0, TypeParam>::value;
    const auto pocma = std::tuple_element_t<1, TypeParam>::value;
    const auto pocs  = std::tuple_element_t<2, TypeParam>::value;

    using content_t = std::vector<std::vector<stored_value>>;
    using a_t = identified_allocator<content_t, pocca, pocma, pocs>;

    a_t a1(1);
    basic_stored_table<content_t, a_t> table1(std::allocator_arg, a1);
    this->init_table(table1);

    a_t a2(2);
    basic_stored_table<content_t, a_t> table2(std::allocator_arg, a2);
    this->init_table(table2);

    table2 = table1;
    const a_t& expected = pocca ? a1 : a2;
    ASSERT_EQ(expected, table2.get_allocator());
}

TYPED_TEST(TestStoredTableAllocatorPropagation, MoveAssignment)
{
    const auto pocca = std::tuple_element_t<0, TypeParam>::value;
    const auto pocma = std::tuple_element_t<1, TypeParam>::value;
    const auto pocs  = std::tuple_element_t<2, TypeParam>::value;

    using content_t = std::vector<std::vector<stored_value>>;
    using a_t = identified_allocator<content_t, pocca, pocma, pocs>;

    a_t a1(1);
    basic_stored_table<content_t, a_t> table1(std::allocator_arg, a1);
    this->init_table(table1);

    a_t a2(2);
    basic_stored_table<content_t, a_t> table2(std::allocator_arg, a2);
    this->init_table(table2);

    table2 = std::move(table1);
    const a_t& expected = pocma ? a1 : a2;
    ASSERT_EQ(expected, table2.get_allocator());

    ASSERT_EQ(pocma, table1.empty());
}

TYPED_TEST(TestStoredTableAllocatorPropagation, MoveAssignmentCompatibleAlloc)
{
    const auto pocca = std::tuple_element_t<0, TypeParam>::value;
    const auto pocma = std::tuple_element_t<1, TypeParam>::value;
    const auto pocs  = std::tuple_element_t<2, TypeParam>::value;

    using content_t = std::vector<std::vector<stored_value>>;
    using a_t = identified_allocator<content_t, pocca, pocma, pocs, true>;

    a_t a1(1);
    basic_stored_table<content_t, a_t> table1(std::allocator_arg, a1);
    this->init_table(table1);

    a_t a2(2);
    basic_stored_table<content_t, a_t> table2(std::allocator_arg, a2);
    this->init_table(table2);

    table2 = std::move(table1);
    const a_t& expected = pocma ? a1 : a2;
    ASSERT_EQ(expected, table2.get_allocator());

    ASSERT_TRUE(table1.empty());
}

TYPED_TEST(TestStoredTableAllocatorPropagation, Swap)
{
    const auto pocca = std::tuple_element_t<0, TypeParam>::value;
    const auto pocma = std::tuple_element_t<1, TypeParam>::value;
    const auto pocs  = std::tuple_element_t<2, TypeParam>::value;

    using content_t = std::vector<std::vector<stored_value>>;
    using a_t = identified_allocator<content_t, pocca, pocma, pocs, true>;

    a_t a1(1);
    basic_stored_table<content_t, a_t> table1(std::allocator_arg, a1);
    this->init_table(table1);

    a_t a2(2);
    basic_stored_table<content_t, a_t> table2(std::allocator_arg, a2);
    this->init_table(table2);

    table2.swap(table1);
    const auto expected1 = pocs ? a2.id() : a1.id();
    const auto expected2 = pocs ? a1.id() : a2.id();
    ASSERT_EQ(expected1, table1.get_allocator().id());
    ASSERT_EQ(expected2, table2.get_allocator().id());
}

TYPED_TEST(TestStoredTableAllocatorPropagation, GenericCopyCtor)
{
    const auto pocca = std::tuple_element_t<0, TypeParam>::value;
    const auto pocma = std::tuple_element_t<1, TypeParam>::value;
    const auto pocs = std::tuple_element_t<2, TypeParam>::value;

    using content_t = std::vector<std::vector<stored_value>>;
    using a_t = identified_allocator<content_t, pocca, pocma, pocs>;

    a_t a1(1);
    basic_stored_table<content_t, a_t> table1(std::allocator_arg, a1);
    this->init_table(table1);

    a_t a2(2);
    basic_stored_table<content_t, a_t> table2(std::allocator_arg, a2, table1);

    ASSERT_EQ(a2, table2.get_allocator());
}

TYPED_TEST(TestStoredTableAllocatorPropagation, GenericMoveCtor)
{
    const auto pocca = std::tuple_element_t<0, TypeParam>::value;
    const auto pocma = std::tuple_element_t<1, TypeParam>::value;
    const auto pocs = std::tuple_element_t<2, TypeParam>::value;

    using content_t = std::vector<std::vector<stored_value>>;
    using a_t = identified_allocator<content_t, pocca, pocma, pocs>;

    a_t a1(1);
    basic_stored_table<content_t, a_t> table1(std::allocator_arg, a1);
    this->init_table(table1);

    a_t a2(2);
    basic_stored_table<content_t, a_t> table2(
        std::allocator_arg, a2, std::move(table1));

    ASSERT_EQ(a2, table2.get_allocator());
}

static_assert(
    std::is_nothrow_move_constructible<
        stored_table_builder<
            wstored_table::content_type,
            wstored_table::allocator_type>>::value, "");
static_assert(
    std::is_nothrow_move_constructible<
        stored_table_builder<
            stored_table::content_type,
            stored_table::allocator_type, true>>::value, "");

struct TestStoredTableReusingBuffer : BaseTest
{
    using at_t =
        typename std::allocator_traits<stored_table::allocator_type>::
            template rebind_traits<char>;
    using a_t = at_t::allocator_type;

    stored_table* table;
    char* m1;
    char* m2;
    char* m3;

    void SetUp()
    {
        BaseTest::SetUp();
        table = new stored_table(50U);
        a_t a(table->get_allocator());
        m1 = at_t::allocate(a, 52U);
        m2 = at_t::allocate(a, 51U);
        m3 = at_t::allocate(a, 50U);
        table->add_buffer(m1, 52U);
        table->add_buffer(m2, 51U);
        table->add_buffer(m3, 50U);
        table->clear();
    }

    void TearDown()
    {
        delete table;
        BaseTest::TearDown();
    }
};

TEST_F(TestStoredTableReusingBuffer, First)
{
    auto p = table->generate_buffer(50U);
    ASSERT_EQ(m3, p.first);
    ASSERT_EQ(50U, p.second);

    table->consume_buffer(m3, 50U);
}

TEST_F(TestStoredTableReusingBuffer, Middle)
{
    auto p = table->generate_buffer(51U);
    ASSERT_EQ(m2, p.first);
    ASSERT_EQ(51U, p.second);

    table->consume_buffer(m2, 51U);
}

TEST_F(TestStoredTableReusingBuffer, Last)
{
    auto p = table->generate_buffer(52U);
    ASSERT_EQ(m1, p.first);
    ASSERT_EQ(52U, p.second);

    table->consume_buffer(m1, 52U);
}

TEST_F(TestStoredTableReusingBuffer, NoSuitable)
{
    auto p = table->generate_buffer(53U);
    ASSERT_NE(p.first, m1);
    ASSERT_NE(p.first, m2);
    ASSERT_NE(p.first, m3);
    ASSERT_GE(p.second, 53U);

    table->consume_buffer(p.first, p.second);
}

TEST_F(TestStoredTableReusingBuffer, Secured)
{
    auto v = table->import_value("ABC");
    ASSERT_GE(v.cbegin(), m3);
    ASSERT_LT(v.cend(), m3 + 50U);

    // In use:  |50|
    // Cleared: |51|52|

    auto p = table->generate_buffer(50U);
    ASSERT_EQ(m2, p.first);
    ASSERT_EQ(51U, p.second);

    table->consume_buffer(m2, 51U);
}

TEST_F(TestStoredTableReusingBuffer, DoubleClear)
{
    // Cleared: |50|51|52|

    auto v = table->import_value(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

    // In use:  |x (>= 53)|
    // Cleared: |50|51|52|

    table->clear();

    // Cleared: |50|51|52|x|

    auto p = table->generate_buffer(53U);
    ASSERT_EQ(v.cbegin(), p.first);

    table->consume_buffer(p.first, p.second);
}

struct TestStoredTableBuilder : BaseTestWithParam<std::size_t>
{};

TEST_P(TestStoredTableBuilder, Basics)
{
    const char* s = "\r\n\n"
                    "\"key_a\",key_b,value_a,value_b\n"
                    "ka1,\"kb\"\"01\"\"\",va1,\n"
                    "ka2,\"\",\"\"\"va2\"\"\",vb2\n"
                    "\"k\"\"a\"\"1\",\"kb\"\"13\"\"\",\"vb\n3\"";
    stored_table table(GetParam());
    try {
        parse_csv(s, make_stored_table_builder(table));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(4U, table.size());
    ASSERT_EQ(4U, table[0].size());
    ASSERT_EQ("key_a",    table[0][0]);
    ASSERT_EQ("key_b",    table[0][1]);
    ASSERT_EQ("value_a",  table[0][2]);
    ASSERT_EQ("value_b",  table[0][3]);
    ASSERT_EQ(4U, table[1].size());
    ASSERT_EQ("ka1",      table[1][0]);
    ASSERT_EQ("kb\"01\"", table[1][1]);
    ASSERT_EQ("va1",      table[1][2]);
    ASSERT_EQ("",         table[1][3]);
    ASSERT_EQ(4U, table[2].size());
    ASSERT_EQ("ka2",      table[2][0]);
    ASSERT_EQ("",         table[2][1]);
    ASSERT_EQ("\"va2\"",  table[2][2]);
    ASSERT_EQ("vb2",      table[2][3]);
    ASSERT_EQ(4U, table.size());
    ASSERT_EQ(3U, table[3].size());
    ASSERT_EQ("k\"a\"1",  table[3][0]);
    ASSERT_EQ("kb\"13\"", table[3][1]);
    ASSERT_EQ("vb\n3",    table[3][2]);
}

TEST_P(TestStoredTableBuilder, MaxRecordNum)
{
    const char* s1 = "\"key_a\",key_b,value_a,value_b\n"
                     "ka1,\"kb\"\"01\"\"\",va1,\n";
    stored_table table(GetParam());
    try {
        parse_csv(s1, make_stored_table_builder(table, 1U));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(1U, table.size());
    ASSERT_EQ(4U, table[0].size());
    ASSERT_EQ("key_a",   table[0][0]);
    ASSERT_EQ("key_b",   table[0][1]);
    ASSERT_EQ("value_a", table[0][2]);
    ASSERT_EQ("value_b", table[0][3]);
}

TEST_P(TestStoredTableBuilder, MaxRecordNumPathological)
{
    const char* s1 = "\r\n\n\"key_a\",key_b,value_a,value_b";
    stored_table table(GetParam());
    try {
        parse_csv(s1, make_stored_table_builder(table, 5U));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(1U, table.size());
    ASSERT_EQ(4U, table[0].size());
    ASSERT_EQ("key_a",   table[0][0]);
    ASSERT_EQ("key_b",   table[0][1]);
    ASSERT_EQ("value_a", table[0][2]);
    ASSERT_EQ("value_b", table[0][3]);
}

TEST_P(TestStoredTableBuilder, EndRecordHandler)
{
    const wchar_t* const s1 = L"A,B,C\nI,J,K\nX,Y,Z\n\"";
    wstored_table table(GetParam());
    try {
        parse_csv(s1, make_stored_table_builder(table,
            [](auto& t) {
                auto& b = t.content().back();
                if (b.front() == L"I") {
                    t.content().pop_back();
                } else if (b.front() == L"X") {
                    std::reverse(b.begin(), b.end());
                    return false;
                }
                return true;
            }));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(2U, table.size());
    ASSERT_EQ(3U, table[0].size());
    ASSERT_STREQ(L"A", table[0][0].c_str());
    ASSERT_STREQ(L"B", table[0][1].c_str());
    ASSERT_STREQ(L"C", table[0][2].c_str());
    ASSERT_STREQ(L"Z", table[1][0].c_str());
    ASSERT_STREQ(L"Y", table[1][1].c_str());
    ASSERT_STREQ(L"X", table[1][2].c_str());
}

TEST_P(TestStoredTableBuilder, EmptyLineAware)
{
    const char* s = "\r1,2,3,4\na,b\r\n\nx,y,z\r\n\"\"";
    stored_table table(GetParam());
    try {
        parse_csv(s, make_empty_physical_line_aware(
            make_stored_table_builder(table)));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(6U, table.size());
    ASSERT_EQ(0U, table[0].size());
    ASSERT_EQ(4U, table[1].size());
    ASSERT_EQ("1", table[1][0]);
    ASSERT_EQ("2", table[1][1]);
    ASSERT_EQ("3", table[1][2]);
    ASSERT_EQ("4", table[1][3]);
    ASSERT_EQ(2U, table[2].size());
    ASSERT_EQ("a", table[2][0]);
    ASSERT_EQ("b", table[2][1]);
    ASSERT_EQ(0U, table[3].size());
    ASSERT_EQ(3U, table[4].size());
    ASSERT_EQ("x", table[4][0]);
    ASSERT_EQ("y", table[4][1]);
    ASSERT_EQ("z", table[4][2]);
    ASSERT_EQ(1U, table[5].size());
    ASSERT_EQ("", table[5][0]);
}

TEST_P(TestStoredTableBuilder, Transpose)
{
    const char* s = "Col1,Col2\n"
                    "aaa,bbb,ccc\n"
                    "AAA,BBB,CCC\n";
    stored_table table(GetParam());
    try {
        parse_csv(s, make_transposed_stored_table_builder(table));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    // |Col1|aaa|AAA|
    // |Col2|bbb|BBB|
    // |    |ccc|CCC|

    ASSERT_EQ(3U, table.size());
    ASSERT_EQ(3U, table[0].size());
    ASSERT_EQ("Col1", table[0][0]);
    ASSERT_EQ("aaa", table[0][1]);
    ASSERT_EQ("AAA", table[0][2]);
    ASSERT_EQ(3U, table[1].size());
    ASSERT_EQ("Col2", table[1][0]);
    ASSERT_EQ("bbb", table[1][1]);
    ASSERT_EQ("BBB", table[1][2]);
    ASSERT_EQ(3U, table[2].size());
    ASSERT_EQ("", table[2][0]);
    ASSERT_EQ("ccc", table[2][1]);
    ASSERT_EQ("CCC", table[2][2]);

    const char* t = "AAa,BBb";
    try {
        parse_csv(t, make_transposed_stored_table_builder(table));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    // |Col1|aaa|AAA|AAa|
    // |Col2|bbb|BBB|BBb|
    // |    |ccc|CCC|

    ASSERT_EQ(3U, table.size());
    ASSERT_EQ(4U, table[0].size());
    ASSERT_EQ("AAa", table[0][3]);
    ASSERT_EQ(4U, table[1].size());
    ASSERT_EQ("BBb", table[1][3]);
    ASSERT_EQ(3U, table[2].size());
}

TEST_P(TestStoredTableBuilder, Fancy)
{
    using content_t = std::vector<std::vector<wstored_value>>;
    using alloc_t = tracking_allocator<fancy_allocator<content_t>>;

    std::vector<std::pair<char*, char*>> allocated;
    alloc_t a(allocated);

    const wchar_t* s = L"Col1,Col2\n"
                       L"aaa,bbb,ccc\n"
                       L"AAA,BBB,CCC\n";
    basic_stored_table<content_t, alloc_t> table(
        std::allocator_arg, a, GetParam());
    try {
        parse_csv(s, make_stored_table_builder(table));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_TRUE(a.tracks(table[0][0].cbegin()));
    ASSERT_TRUE(a.tracks(table.content().back().back().cend()));
}

INSTANTIATE_TEST_SUITE_P(,
    TestStoredTableBuilder, testing::Values(2, 11, 1024));

struct TestStoredTableBuilderReusingBuffer : BaseTest
{};

TEST_F(TestStoredTableBuilderReusingBuffer, Basics)
{
    wstored_table table(100U);

    // use the first buffer
    const auto v = table.import_value(L"1234567890");

    table.clear();  // the buffer shall be retained

    const wchar_t* s = L"ABCDEFG";
    try {
        parse_csv(s, make_stored_table_builder(table));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    // ensure the buffer is reused after table.clear() was called
    ASSERT_EQ(v.cbegin(), table[0][0].cbegin());
}

struct TestStoredTableConst : BaseTest
{};

TEST_F(TestStoredTableConst, Value)
{
    const auto str = char_helper<char>::str;
    const auto str0 = char_helper<char>::str0;

    auto s = str0("abcde");
    stored_value v(&s[0], &s[0] + s.size() - 1);

    auto sa = str("abcd");
    auto sb = str("abcde");
    auto sc = str("abcdE");
    auto sd = str("abcdef");

    // test copy ctor and relationship
    cstored_value cv(v);
    // ==
    ASSERT_TRUE(cv == v);
    ASSERT_TRUE(v == cv);
    ASSERT_TRUE(cv == "abcde");
    ASSERT_TRUE(cv == sb);
    ASSERT_TRUE("abcde" == cv);
    ASSERT_TRUE(sb == cv);
    // !=
    ASSERT_TRUE(cv != "abcdE");
    ASSERT_TRUE(cv != sc);
    ASSERT_TRUE("abcdE" != cv);
    ASSERT_TRUE(sc != cv);
    // <
    ASSERT_TRUE(cv < "abcdef");
    ASSERT_TRUE(cv < sd);
    ASSERT_TRUE("abcd" < cv);
    ASSERT_TRUE(sa < cv);
    // >
    ASSERT_TRUE(cv > "abcd");
    ASSERT_TRUE(cv > sa);
    ASSERT_TRUE("abcdef" > cv);
    ASSERT_TRUE(sd > cv);
    // <=
    ASSERT_TRUE(cv <= "abcdef");
    ASSERT_TRUE(cv <= sd);
    ASSERT_TRUE("abcd" <= cv);
    ASSERT_TRUE(sa <= cv);
    ASSERT_TRUE(cv <= "abcde");
    ASSERT_TRUE(cv <= sb);
    ASSERT_TRUE("abcde" <= cv);
    ASSERT_TRUE(sb <= cv);
    // >=
    ASSERT_TRUE(cv >= "abcd");
    ASSERT_TRUE(cv >= sa);
    ASSERT_TRUE("abcdef" >= cv);
    ASSERT_TRUE(sd >= cv);
    ASSERT_TRUE(cv >= "abcde");
    ASSERT_TRUE(cv >= sb);
    ASSERT_TRUE("abcde" >= cv);
    ASSERT_TRUE(sb >= cv);

    // test copy assignment
    auto t = str0("xyzuv");
    basic_stored_value<char> v2(&t[0], &t[0] + t.size() - 1);
    cv = v2;
    ASSERT_EQ(cv, "xyzuv");

    // test with streams
    std::stringstream stream;
    stream << cv;
    ASSERT_EQ("xyzuv", stream.str());
}

TEST_F(TestStoredTableConst, Table)
{
    cwstored_table table;
    static_assert(
        std::is_same<decltype(table)::char_type, wchar_t>::value, "");
    auto value = table.import_value(L"alpha-beta-gamma");
    const auto b = value.begin();
    table.rewrite_value(value, L"alpha-beta-delta");
    // in-place rewriting cannot take place
    ASSERT_NE(value.begin(), b);
}

TEST_F(TestStoredTableConst, Build)
{
    const char* s = "A1,B1\n"
                    "A2,B2\n"
                    "A3,B3\n";
    cstored_table table;
    try {
        parse_csv(s, make_stored_table_builder(table));
    } catch (const text_error& e) {
        FAIL() << e.info();
    }

    ASSERT_EQ(3U, table.size());
    ASSERT_EQ(2U, table[0].size());
    ASSERT_EQ("A1", table[0][0]);
    ASSERT_EQ(2U, table[2].size());
    ASSERT_EQ("B3", table[2][1]);
}
