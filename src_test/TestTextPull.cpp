/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <commata/parse_csv.hpp>
#include <commata/text_pull.hpp>

#include "BaseTest.hpp"

using namespace commata;
using namespace commata::test;

template <class ChB>
struct TestTextPull : BaseTest
{};

TYPED_TEST_SUITE_P(TestTextPull);

TYPED_TEST_P(TestTextPull, PrimitiveBasics)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;
    const auto ch  = char_helper<char_t>::ch;
   
    const auto csv = str(",\"col1\", col2 ,col3,\r\n\n"
                         " cell10 ,,\"cell\r\n12\",\"cell\"\"13\"\"\",\"\"\n");
    auto source = make_csv_source(csv);
    primitive_text_pull<decltype(source)> pull(
        std::move(source), TypeParam::second_type::value);
    ASSERT_EQ(2, pull.max_data_size());
    std::basic_string<char_t> s;
    bool in_value = false;
    while (pull()) {
        switch (pull.state()) {
        case primitive_text_pull_state::update:
            if (!in_value) {
                s.push_back(ch('['));
                in_value = true;
            }
            s.append(pull[0], pull[1]);
            break;
        case primitive_text_pull_state::finalize:
            if (!in_value) {
                s.push_back(ch('['));
            }
            s.append(pull[0], pull[1]);
            s.push_back(ch(']'));
            in_value = false;
            break;
        case primitive_text_pull_state::start_record:
            s.append(str("<<"));
            break;
        case primitive_text_pull_state::end_record:
            s.append(str(">>"));
            {
                const auto pos = pull.get_physical_position();
                s.push_back('@');
                s += str(std::to_string(pos.first).c_str());
                s.push_back(',');
                s += str(std::to_string(pos.second).c_str());
            }
            break;
        case primitive_text_pull_state::empty_physical_line:
            s.append(str("--"));
            break;
        default:
            break;
        }
    }
    ASSERT_EQ(str("<<[][col1][ col2 ][col3][]>>@0,20"
                  "--"
                  "<<[ cell10 ][][cell\r\n12][cell\"13\"][]>>@2,36"),
              s);

    static_assert(std::is_nothrow_move_constructible<decltype(pull)>::value,
        "");
}

TYPED_TEST_P(TestTextPull, PrimitiveMove)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;
   
    const auto csv = str("A,B\nC,D");
    auto source = make_csv_source(csv);
    primitive_text_pull<decltype(source)> pull(
        std::move(source), TypeParam::second_type::value);

    // Skip first line
    while (pull().state() != primitive_text_pull_state::end_record);

    auto pull2 = std::move(pull);
    ASSERT_EQ(primitive_text_pull_state::moved, pull.state());
    ASSERT_EQ(primitive_text_pull_state::end_record, pull2.state());

    std::basic_string<char_t> s;
    while (pull2()) {
        switch (pull2.state()) {
        case primitive_text_pull_state::update:
            if (!s.empty()) {
                s.push_back('+');
            }
            s.append(pull2[0], pull2[1]);
            break;
        case primitive_text_pull_state::finalize:
            s.append(pull2[0], pull2[1]);
            break;
        default:
            break;
        }
    }
    ASSERT_EQ(str("C+D"), s);
}

TYPED_TEST_P(TestTextPull, Basics)
{
    using char_t = typename TypeParam::first_type;
    using string_t = std::basic_string<char_t>;
    using pos_t = std::pair<std::size_t, std::size_t>;

    const auto ch = char_helper<char_t>::ch;
    const auto str = char_helper<char_t>::str;
   
    const auto csv = str(",\"col1\", col2 ,col3,\r\n\n"
                         " cell10 ,,\"cell\r\n12\",\"cell\"\"13\"\"\",\"\"\n");

    for (auto e : { false, true }) {
        auto pull = make_text_pull(
            make_csv_source(csv), TypeParam::second_type::value);
        pull.set_empty_physical_line_aware(e);

        ASSERT_TRUE(pull) << e;
        ASSERT_EQ(text_pull_state::before_parse, pull.state()) << e;
        ASSERT_EQ(str(""), string_t(pull.cbegin(), pull.cend())) << e;
        ASSERT_TRUE(pull.empty());
        ASSERT_EQ(0U, pull.size());

        std::size_t i = 0;
        std::size_t j = 0;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(text_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str(""), string_t(pull.cbegin(), pull.cend())) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 0), pull.get_physical_position()) << e;
        ASSERT_EQ(str(""), to_string(pull)) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(text_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str("col1"), string_t(pull.cbegin(), pull.cend())) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 7), pull.get_physical_position()) << e;
        ASSERT_FALSE(pull.empty());
        ASSERT_EQ(4U, pull.size());
        ASSERT_EQ('\0', pull.cbegin()[4]);
        ASSERT_EQ(ch('o'), pull[1]);
        ASSERT_EQ(ch('\0'), pull[4]);
        ASSERT_EQ(ch('l'), pull.at(2));
        ASSERT_THROW(pull.at(4), std::out_of_range);
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(text_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str(" 2loc "), string_t(pull.crbegin(), pull.crend())) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 14), pull.get_physical_position()) << e;
        ASSERT_EQ(str(" col2 "), to_string(pull)) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(text_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str("col3"), string_t(pull.begin(), pull.end())) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 19), pull.get_physical_position()) << e;
        std::basic_ostringstream<char_t> o1;
        o1 << pull;
        ASSERT_EQ(str("col3"), o1.str()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(text_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str(""), string_t(pull.cbegin(), pull.cend())) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 20), pull.get_physical_position()) << e;
        std::basic_ostringstream<char_t> o2;
        o2 << pull;
        ASSERT_EQ(str(""), o2.str()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(text_pull_state::record_end, pull.state()) << e;
        ASSERT_EQ(str(""), string_t(pull.cbegin(), pull.cend())) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 20), pull.get_physical_position()) << e;
        ASSERT_EQ(str(""), static_cast<string_t>(pull));
        ++i;
        j = 0;

        if (e) {
            ASSERT_TRUE(pull()) << e;
            ASSERT_EQ(text_pull_state::record_end, pull.state()) << e;
            ASSERT_EQ(str(""), string_t(pull.cbegin(), pull.cend())) << e;
            ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
            ASSERT_EQ(pos_t(1, 0), pull.get_physical_position()) << e;
            ++i;
        }

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(text_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str(" 01llec "), string_t(pull.rbegin(), pull.rend())) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(2, 8), pull.get_physical_position()) << e;
        ASSERT_EQ(str(" cell10 "), static_cast<string_t>(pull));
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(text_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str(""), string_t(pull.cbegin(), pull.cend())) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(2, 9), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(text_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str("cell\r\n12"),
            string_t(pull.cbegin(), pull.cend())) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(2, 20), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(text_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str("cell\"13\""),
            string_t(pull.cbegin(), pull.cend())) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(2, 33), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(text_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str(""), string_t(pull.cbegin(), pull.cend())) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(2, 36), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(text_pull_state::record_end, pull.state()) << e;
        ASSERT_EQ(str(""), string_t(pull.cbegin(), pull.cend())) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(2, 36), pull.get_physical_position()) << e;
        ++i;
        j = 0;

        ASSERT_FALSE(pull()) << e;
        ASSERT_EQ(text_pull_state::eof, pull.state()) << e;
        ASSERT_EQ(i, pull.get_position().first) << e;

        // Already EOF
        ASSERT_FALSE(pull()) << e;
        ASSERT_EQ(text_pull_state::eof, pull.state()) << e;
        ASSERT_EQ(i, pull.get_position().first) << e;

        static_assert(
            std::is_nothrow_move_constructible<decltype(pull)>::value, "");
    }
}

TYPED_TEST_P(TestTextPull, SkipField)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;
   
    const auto csv = str("1A,1B,1C,1D,1E,1F");

    auto pull = make_text_pull(
        make_csv_source(csv), TypeParam::second_type::value);

    std::size_t i = 0;
    std::size_t j = 0;

    pull();
    ASSERT_TRUE(pull(2));
    ASSERT_EQ(text_pull_state::field, pull.state());
    ASSERT_EQ(str("1D"), to_string(pull));
    i = 0; j = 3;
    ASSERT_EQ(std::make_pair(i, j), pull.get_position());

    ASSERT_TRUE(pull(5));
    ASSERT_EQ(text_pull_state::record_end, pull.state());
    ASSERT_TRUE(pull.empty());
    i = 0; j = 6;
    ASSERT_EQ(std::make_pair(i, j), pull.get_position());
}

TYPED_TEST_P(TestTextPull, SkipRecord)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;
   
    const auto csv = str("1A,1B\n2A,2B\n3A,3B\n4A,4B");

    auto pull = make_text_pull(
        make_csv_source(csv), TypeParam::second_type::value);

    std::size_t i = 0;
    std::size_t j = 0;

    pull();
    ASSERT_TRUE(pull.skip_record(2));
    ASSERT_EQ(text_pull_state::record_end, pull.state());
    i = 2; j = 2;
    ASSERT_EQ(std::make_pair(i, j), pull.get_position());

    ASSERT_TRUE(pull());
    ASSERT_EQ(text_pull_state::field, pull.state());
    ASSERT_EQ(str("4A"), to_string(pull));

    ASSERT_FALSE(pull.skip_record(5));
    ASSERT_EQ(text_pull_state::eof, pull.state());
    i = 4; j = 0;
    ASSERT_EQ(std::make_pair(i, j), pull.get_position());
}

TYPED_TEST_P(TestTextPull, SuppressedError)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;
   
    const auto csv = str("\nA\nB,\"C");

    auto pull = make_text_pull(
        make_csv_source(csv), TypeParam::second_type::value);
    pull.set_suppressing_errors();
    ASSERT_EQ(text_pull_state::field, pull().state());
    ASSERT_EQ(text_pull_state::record_end, pull().state());
    ASSERT_EQ(text_pull_state::field, pull().state());
    ASSERT_EQ(text_pull_state::error, pull().state());  // causes an error

    // The state is 'error'
    ASSERT_FALSE(pull);
    ASSERT_EQ(text_pull_state::error, pull.state());

    // One more call of operator() will not change the state
    ASSERT_FALSE(pull());
    ASSERT_EQ(text_pull_state::error, pull.state());

    ASSERT_THROW(pull.rethrow_suppressed(), parse_error);

    // Rethrowing the error will not change the state
    // except that it will lose the error
    ASSERT_FALSE(pull);

    ASSERT_EQ(text_pull_state::error, pull.state());
    ASSERT_NO_THROW(pull.rethrow_suppressed());

    // Yet another call of operator() will not change the state
    ASSERT_FALSE(pull());
    ASSERT_EQ(text_pull_state::error, pull.state());
}

TYPED_TEST_P(TestTextPull, Relations)
{
    using char_t = typename TypeParam::first_type;
    using string_t = std::basic_string<char_t>;

    const auto str = char_helper<char_t>::str;

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

        auto p1 = make_text_pull(make_csv_source(s1),
            TypeParam::second_type::value);
        auto p2 = make_text_pull(make_csv_source(s2),
            TypeParam::second_type::value);
        p1();
        p2();

        // stored_value vs stored_value
        ASSERT_EQ(s1 == s2, p1 == p2) << s1 << " == " << s2;
        ASSERT_EQ(s1 != s2, p1 != p2) << s1 << " != " << s2;
        ASSERT_EQ(s2 == s1, p2 == p1) << s2 << " == " << s1;
        ASSERT_EQ(s2 != s1, p2 != p1) << s2 << " != " << s1;
        ASSERT_EQ(s1 < s2, p1 < p2) << s1 << " < " << s2;
        ASSERT_EQ(s1 > s2, p1 > p2) << s1 << " > " << s2;
        ASSERT_EQ(s1 <= s2, p1 <= p2) << s1 << " <= " << s2;
        ASSERT_EQ(s1 >= s2, p1 >= p2) << s1 << " >= " << s2;
        ASSERT_EQ(s2 < s1, p2 < p1) << s2 << " < " << s1;
        ASSERT_EQ(s2 > s1, p2 > p1) << s2 << " > " << s1;
        ASSERT_EQ(s2 <= s1, p2 <= p1) << s2 << " <= " << s1;
        ASSERT_EQ(s2 >= s1, p2 >= p1) << s2 << " >= " << s1;

        // stored_value vs string
        ASSERT_EQ(s1 == s2, p1 == s2) << s1 << " == " << s2;
        ASSERT_EQ(s1 != s2, p1 != s2) << s1 << " != " << s2;
        ASSERT_EQ(s2 == s1, p2 == s1) << s2 << " == " << s1;
        ASSERT_EQ(s2 != s1, p2 != s1) << s2 << " != " << s1;
        ASSERT_EQ(s1 < s2, p1 < s2) << s1 << " < " << s2;
        ASSERT_EQ(s1 > s2, p1 > s2) << s1 << " > " << s2;
        ASSERT_EQ(s1 <= s2, p1 <= s2) << s1 << " <= " << s2;
        ASSERT_EQ(s1 >= s2, p1 >= s2) << s1 << " >= " << s2;
        ASSERT_EQ(s2 < s1, p2 < s1) << s2 << " < " << s1;
        ASSERT_EQ(s2 > s1, p2 > s1) << s2 << " > " << s1;
        ASSERT_EQ(s2 <= s1, p2 <= s1) << s2 << " <= " << s1;
        ASSERT_EQ(s2 >= s1, p2 >= s1) << s2 << " >= " << s1;

        // string vs stored_value
        ASSERT_EQ(s1 == s2, s1 == p2) << s1 << " == " << s2;
        ASSERT_EQ(s1 != s2, s1 != p2) << s1 << " != " << s2;
        ASSERT_EQ(s2 == s1, s2 == p1) << s2 << " == " << s1;
        ASSERT_EQ(s2 != s1, s2 != p1) << s2 << " != " << s1;
        ASSERT_EQ(s1 < s2, s1 < p2) << s1 << " < " << s2;
        ASSERT_EQ(s1 > s2, s1 > p2) << s1 << " > " << s2;
        ASSERT_EQ(s1 <= s2, s1 <= p2) << s1 << " <= " << s2;
        ASSERT_EQ(s1 >= s2, s1 >= p2) << s1 << " >= " << s2;
        ASSERT_EQ(s2 < s1, s2 < p1) << s2 << " < " << s1;
        ASSERT_EQ(s2 > s1, s2 > p1) << s2 << " > " << s1;
        ASSERT_EQ(s2 <= s1, s2 <= p1) << s2 << " <= " << s1;
        ASSERT_EQ(s2 >= s1, s2 >= p1) << s2 << " >= " << s1;

        // stored_value vs NTBS
        ASSERT_EQ(s1 == s2, p1 == s2.c_str()) << s1 << " == " << s2;
        ASSERT_EQ(s1 != s2, p1 != s2.c_str()) << s1 << " != " << s2;
        ASSERT_EQ(s2 == s1, p2 == s1.c_str()) << s2 << " == " << s1;
        ASSERT_EQ(s2 != s1, p2 != s1.c_str()) << s2 << " != " << s1;
        ASSERT_EQ(s1 < s2, p1 < s2.c_str()) << s1 << " < " << s2;
        ASSERT_EQ(s1 > s2, p1 > s2.c_str()) << s1 << " > " << s2;
        ASSERT_EQ(s1 <= s2, p1 <= s2.c_str()) << s1 << " <= " << s2;
        ASSERT_EQ(s1 >= s2, p1 >= s2.c_str()) << s1 << " >= " << s2;
        ASSERT_EQ(s2 < s1, p2 < s1.c_str()) << s2 << " < " << s1;
        ASSERT_EQ(s2 > s1, p2 > s1.c_str()) << s2 << " > " << s1;
        ASSERT_EQ(s2 <= s1, p2 <= s1.c_str()) << s2 << " <= " << s1;
        ASSERT_EQ(s2 >= s1, p2 >= s1.c_str()) << s2 << " >= " << s1;

        // NTBS vs stored_value
        ASSERT_EQ(s1 == s2, s1.c_str() == p2) << s1 << " == " << s2;
        ASSERT_EQ(s1 != s2, s1.c_str() != p2) << s1 << " != " << s2;
        ASSERT_EQ(s2 == s1, s2.c_str() == p1) << s2 << " == " << s1;
        ASSERT_EQ(s2 != s1, s2.c_str() != p1) << s2 << " != " << s1;
        ASSERT_EQ(s1 < s2, s1.c_str() < p2) << s1 << " < " << s2;
        ASSERT_EQ(s1 > s2, s1.c_str() > p2) << s1 << " > " << s2;
        ASSERT_EQ(s1 <= s2, s1.c_str() <= p2) << s1 << " <= " << s2;
        ASSERT_EQ(s1 >= s2, s1.c_str() >= p2) << s1 << " >= " << s2;
        ASSERT_EQ(s2 < s1, s2.c_str() < p1) << s2 << " < " << s1;
        ASSERT_EQ(s2 > s1, s2.c_str() > p1) << s2 << " > " << s1;
        ASSERT_EQ(s2 <= s1, s2.c_str() <= p1) << s2 << " <= " << s1;
        ASSERT_EQ(s2 >= s1, s2.c_str() >= p1) << s2 << " >= " << s1;
    }
}

TYPED_TEST_P(TestTextPull, Plus)
{
    using char_t = typename TypeParam::first_type;
    using string_t = std::basic_string<char_t>;

    const auto str = char_helper<char_t>::str;

    auto pull = make_text_pull(make_csv_source(str("XYZ")),
        TypeParam::second_type::value);
    pull();

    string_t s1 = str("xyz");

    ASSERT_EQ(str("xyzXYZ"), s1 + pull);
    ASSERT_EQ(str("XYZxyz"), pull + s1);

    ASSERT_EQ(str("xyzXYZ"), std::move(s1) + pull);
    ASSERT_EQ(str("XYZ123"), pull + str("123"));

    string_t s2 = str("abc");
    ASSERT_EQ(str("abcXYZ"), s2 += pull);
}

REGISTER_TYPED_TEST_SUITE_P(TestTextPull,
    PrimitiveBasics, PrimitiveMove,
    Basics, SkipRecord, SkipField, SuppressedError, Relations, Plus);

typedef testing::Types<
    std::pair<char, std::integral_constant<std::size_t, 2>>,
    std::pair<char, std::integral_constant<std::size_t, 4>>,
    std::pair<char, std::integral_constant<std::size_t, 1024>>,
    std::pair<wchar_t, std::integral_constant<std::size_t, 2>>,
    std::pair<wchar_t, std::integral_constant<std::size_t, 4>>,
    std::pair<wchar_t, std::integral_constant<std::size_t, 1024>>> ChBs;
INSTANTIATE_TYPED_TEST_SUITE_P(, TestTextPull, ChBs);
