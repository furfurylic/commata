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
#include <utility>

#include <gtest/gtest.h>

#include <commata/parse_csv.hpp>
#include <commata/table_pull.hpp>

#include "BaseTest.hpp"

using namespace commata;
using namespace commata::test;

template <class ChB>
struct TestTablePull : BaseTest
{};

TYPED_TEST_SUITE_P(TestTablePull);

TYPED_TEST_P(TestTablePull, PrimitiveBasics)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;
    const auto ch  = char_helper<char_t>::ch;
   
    const auto csv = str(R"(,"col1", col2 ,col3,)" "\r\n"
                         "\n"
                         R"( cell10 ,,"cell)" "\r\n"
                         R"(12","cell""13""","")" "\n");
    auto source = make_csv_source(csv);
    primitive_table_pull pull(
        std::move(source), TypeParam::second_type::value);
    ASSERT_EQ(2U, pull.max_data_size());
    std::basic_string<char_t> s;
    bool in_value = false;
    while (pull()) {
        switch (pull.state()) {
        case primitive_table_pull_state::update:
            ASSERT_EQ(2U, pull.data_size());
            ASSERT_THROW(pull.at(2), std::out_of_range);
            if (!in_value) {
                s.push_back(ch('['));
                in_value = true;
            }
            s.append(pull.at(0), pull[1]);
            break;
        case primitive_table_pull_state::finalize:
            ASSERT_EQ(2U, pull.data_size());
            ASSERT_THROW(pull.at(2), std::out_of_range);
            if (!in_value) {
                s.push_back(ch('['));
            }
            s.append(pull[0], pull.at(1));
            s.push_back(ch(']'));
            in_value = false;
            break;
        case primitive_table_pull_state::start_record:
            ASSERT_EQ(1U, pull.data_size());
            ASSERT_THROW(pull.at(1), std::out_of_range);
            s.append(str("<<"));
            break;
        case primitive_table_pull_state::end_record:
            ASSERT_EQ(1U, pull.data_size());
            ASSERT_THROW(pull.at(1), std::out_of_range);
            s.append(str(">>"));
            {
                const auto pos = pull.get_physical_position();
                s.push_back('@');
                s += str(std::to_string(pos.first).c_str());
                s.push_back(',');
                s += str(std::to_string(pos.second).c_str());
            }
            break;
        case primitive_table_pull_state::empty_physical_line:
            ASSERT_EQ(1U, pull.data_size());
            ASSERT_THROW(pull.at(1), std::out_of_range);
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

    static_assert(std::is_nothrow_move_constructible_v<decltype(pull)>);
}

TYPED_TEST_P(TestTablePull, PrimitiveMove)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;
   
    const auto csv = str("A,B\nC,D");
    auto source = make_csv_source(csv);
    primitive_table_pull pull(
        std::move(source), TypeParam::second_type::value);

    // Skip first line
    while (pull().state() != primitive_table_pull_state::end_record);

    auto pull2 = std::move(pull);
    ASSERT_EQ(primitive_table_pull_state::eof/* actually unspecified */,
        pull.state());
    ASSERT_EQ(primitive_table_pull_state::end_record, pull2.state());

    std::basic_string<char_t> s;
    while (pull2()) {
        switch (pull2.state()) {
        case primitive_table_pull_state::update:
            if (!s.empty()) {
                s.push_back('+');
            }
            s.append(pull2[0], pull2[1]);
            break;
        case primitive_table_pull_state::finalize:
            s.append(pull2[0], pull2[1]);
            break;
        default:
            break;
        }
    }
    ASSERT_EQ(str("C+D"), s);
}

TYPED_TEST_P(TestTablePull, Basics)
{
    using char_t = typename TypeParam::first_type;
    using string_t = std::basic_string<char_t>;
    using pos_t = std::pair<std::size_t, std::size_t>;

    const auto ch = char_helper<char_t>::ch;
    const auto str = char_helper<char_t>::str;
   
    const auto csv = str(R"(,"col1", col2 ,col3,)" "\r\n"
                         "\n"
                         R"( cell10 ,,"cell)" "\r\n"
                         R"(12","cell""13""","")" "\n");

    for (auto e : { false, true }) {
        auto pull = make_table_pull(
            make_csv_source(csv), TypeParam::second_type::value);
        pull.set_empty_physical_line_aware(e);

        ASSERT_TRUE(pull) << e;
        ASSERT_EQ(table_pull_state::before_parse, pull.state()) << e;
        ASSERT_EQ(str(""), string_t(*pull)) << e;
        ASSERT_TRUE(pull->empty());
        ASSERT_EQ(0U, pull->size());

        std::size_t i = 0;
        std::size_t j = 0;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str(""), string_t(*pull)) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 0), pull.get_physical_position()) << e;
        ASSERT_EQ(str(""), string_t(*pull)) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str("col1"), string_t(*pull)) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 7), pull.get_physical_position()) << e;
        ASSERT_FALSE(pull->empty()) << e;
        ASSERT_EQ(4U, pull->size()) << e;
        ASSERT_EQ('\0', pull->data()[4]) << e;
        ASSERT_EQ(ch('o'), (*pull)[1]) << e;
        ASSERT_EQ(ch('l'), pull->at(2)) << e;
        ASSERT_THROW(static_cast<void>(pull->at(4)), std::out_of_range) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str(" 2loc "), string_t(pull->crbegin(), pull->crend()))
            << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 14), pull.get_physical_position()) << e;
        ASSERT_EQ(str(" col2 "), string_t(*pull)) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str("col3"), pull.c_str()) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 19), pull.get_physical_position()) << e;
        {
            std::basic_ostringstream<char_t> o1;
            o1 << *pull;
            ASSERT_EQ(str("col3"), std::move(o1).str()) << e;
        }
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str(""), string_t(pull.c_str())) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 20), pull.get_physical_position()) << e;
        {
            std::basic_ostringstream<char_t> o2;
            o2 << *pull;
            ASSERT_EQ(str(""), std::move(o2).str()) << e;
        }
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::record_end, pull.state()) << e;
        ASSERT_EQ(str(""), string_t(*pull)) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 20), pull.get_physical_position()) << e;
        ASSERT_EQ(str(""), string_t(*pull));
        ++i;
        j = 0;

        if (e) {
            ASSERT_TRUE(pull()) << e;
            ASSERT_EQ(table_pull_state::record_end, pull.state()) << e;
            ASSERT_EQ(str(""), string_t(*pull)) << e;
            ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
            ASSERT_EQ(pos_t(1, 0), pull.get_physical_position()) << e;
            ++i;
        }

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str(" 01llec "), string_t(pull->rbegin(), pull->rend()))
            << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(2, 8), pull.get_physical_position()) << e;
        ASSERT_EQ(str(" cell10 "), string_t(*pull));
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str(""), string_t(*pull)) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(2, 9), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str("cell\r\n12"), string_t(*pull)) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(2, 20), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str("cell\"13\""), string_t(*pull)) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(2, 33), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str(""), string_t(*pull)) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(2, 36), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::record_end, pull.state()) << e;
        ASSERT_EQ(str(""), string_t(*pull)) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(2, 36), pull.get_physical_position()) << e;
        ++i;
        j = 0;

        ASSERT_FALSE(pull()) << e;
        ASSERT_EQ(table_pull_state::eof, pull.state()) << e;
        ASSERT_EQ(i, pull.get_position().first) << e;

        // Already EOF
        ASSERT_FALSE(pull()) << e;
        ASSERT_EQ(table_pull_state::eof, pull.state()) << e;
        ASSERT_EQ(i, pull.get_position().first) << e;

        static_assert(std::is_nothrow_move_constructible_v<decltype(pull)>);
    }
}

TYPED_TEST_P(TestTablePull, SkipField)
{
    using char_t = typename TypeParam::first_type;
    using string_t = std::basic_string<char_t>;

    const auto str = char_helper<char_t>::str;
   
    const auto csv = str("1A,1B,1C,1D,1E,1F\n2A,2B");

    auto pull = make_table_pull(
        make_csv_source(csv), TypeParam::second_type::value);

    std::size_t i = 0;
    std::size_t j = 0;

    pull();
    ASSERT_TRUE(pull(2));
    ASSERT_EQ(table_pull_state::field, pull.state());
    ASSERT_EQ(str("1D"), string_t(*pull));
    i = 0; j = 3;
    ASSERT_EQ(std::make_pair(i, j), pull.get_position());

    ASSERT_TRUE(pull(5));
    ASSERT_EQ(table_pull_state::record_end, pull.state());
    ASSERT_TRUE(pull->empty());
    i = 0; j = 6;
    ASSERT_EQ(std::make_pair(i, j), pull.get_position());

    ASSERT_TRUE(pull(1));
    ASSERT_EQ(table_pull_state::field, pull.state());
    ASSERT_EQ(str("2B"), string_t(*pull));
    i = 1; j = 1;
    ASSERT_EQ(std::make_pair(i, j), pull.get_position());
}

TYPED_TEST_P(TestTablePull, SkipRecord)
{
    using char_t = typename TypeParam::first_type;
    using string_t = std::basic_string<char_t>;

    const auto str = char_helper<char_t>::str;

    const auto csv = str("1A,1B\n2A,2B\n3A,3B\n4A,4B");

    auto pull = make_table_pull(
        make_csv_source(csv), TypeParam::second_type::value);

    std::size_t i = 0;
    std::size_t j = 0;

    pull();
    ASSERT_TRUE(pull.skip_record(2));
    ASSERT_EQ(table_pull_state::record_end, pull.state());
    i = 2; j = 2;
    ASSERT_EQ(std::make_pair(i, j), pull.get_position());

    ASSERT_TRUE(pull());
    ASSERT_EQ(table_pull_state::field, pull.state());
    ASSERT_EQ(str("4A"), string_t(*pull));

    ASSERT_FALSE(pull.skip_record(5));
    ASSERT_EQ(table_pull_state::eof, pull.state());
    i = 4; j = 0;
    ASSERT_EQ(std::make_pair(i, j), pull.get_position());
}

TYPED_TEST_P(TestTablePull, Error)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;
   
    const auto csv = str("\nA\nB,\"C");

    auto pull = make_table_pull(
        make_csv_source(csv), TypeParam::second_type::value);
    ASSERT_EQ(table_pull_state::field, pull().state());
    ASSERT_EQ(table_pull_state::record_end, pull().state());
    ASSERT_EQ(table_pull_state::field, pull().state());
    ASSERT_THROW(pull(), parse_error);  // causes an error

    // The state is 'eof', which is, however, not specified by the spec
    ASSERT_EQ(table_pull_state::eof, pull.state());
    ASSERT_FALSE(pull);
    // first is the number of successfully read records and
    // secod is the number of successfully read fields after
    // the last successfully read record, which is specified by the spec
    // for 'eof'
    ASSERT_EQ(1U, pull.get_position().first);
    ASSERT_EQ(1U, pull.get_position().second);

    // One more call of operator() will not change the state
    ASSERT_FALSE(pull());
    ASSERT_EQ(table_pull_state::eof, pull.state());
}

TYPED_TEST_P(TestTablePull, Move)
{
    using char_t = typename TypeParam::first_type;
    using pos_t = std::pair<std::size_t, std::size_t>;

    const auto str = char_helper<char_t>::str;

    auto pull = make_table_pull(make_csv_source(str("XYZ,UVW\n"
                                                    "abc,def\n"
                                                    "\"\"\"")),
                                TypeParam::second_type::value);
    pull.skip_record()();

    auto pull2 = std::move(pull);
    ASSERT_EQ(pos_t(1U, 0U), pull2.get_position());
    ASSERT_EQ(str("def"), *pull2());

    ASSERT_EQ(table_pull_state::eof, pull.state());
                                    // Unspecified but implemented so
    ASSERT_TRUE(pull->empty());     // ditto

    ASSERT_THROW(pull2()(), parse_error);
    ASSERT_EQ(table_pull_state::eof, pull2.state());
                                    // Unspecified but implemented so
    ASSERT_EQ(pos_t(2U, 0U), pull2.get_position());

    auto pull3 = std::move(pull2);  // also the state is moved
    ASSERT_EQ(table_pull_state::eof, pull3.state());
}

REGISTER_TYPED_TEST_SUITE_P(TestTablePull,
    PrimitiveBasics, PrimitiveMove,
    Basics, SkipRecord, SkipField, Error, Move);

typedef testing::Types<
    std::pair<char, std::integral_constant<std::size_t, 1>>,
    std::pair<char, std::integral_constant<std::size_t, 2>>,
    std::pair<char, std::integral_constant<std::size_t, 4>>,
    std::pair<char, std::integral_constant<std::size_t, 1024>>,
    std::pair<wchar_t, std::integral_constant<std::size_t, 1>>,
    std::pair<wchar_t, std::integral_constant<std::size_t, 2>>,
    std::pair<wchar_t, std::integral_constant<std::size_t, 4>>,
    std::pair<wchar_t, std::integral_constant<std::size_t, 1024>>> ChBs;
INSTANTIATE_TYPED_TEST_SUITE_P(CharsBufferSizes, TestTablePull, ChBs);
