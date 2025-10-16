/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <string>
#include <vector>
#include <utility>

#include <gtest/gtest.h>

#include <commata/parse_csv.hpp>
#include <commata/parse_tsv.hpp>
#include <commata/table_pull.hpp>
#include <commata/text_value_translation.hpp>

#include "BaseTest.hpp"

using namespace std::string_view_literals;

using namespace commata;
using namespace commata::test;

namespace {

template <class PrimitiveTablePull>
auto transcript_primitive(PrimitiveTablePull&& pull,
    bool at_start_buffer = false, bool at_end_buffer = false)
{
    using char_t = std::remove_const_t<
        typename std::decay_t<PrimitiveTablePull>::char_type>;

    const auto str = char_helper<char_t>::str;
    const auto ch  = char_helper<char_t>::ch;
    std::basic_string<char_t> s;
    bool in_value = false;
    while (pull()) {
        switch (pull.state()) {
        case primitive_table_pull_state::update:
            EXPECT_EQ(2U, pull.data_size());
            EXPECT_THROW(pull.at(2), std::out_of_range);
            if (!in_value) {
                s.push_back(ch('['));
                in_value = true;
            }
            s.append(pull.at(0), pull[1]);
            break;
        case primitive_table_pull_state::finalize:
            EXPECT_EQ(2U, pull.data_size());
            EXPECT_THROW(pull.at(2), std::out_of_range);
            if (!in_value) {
                s.push_back(ch('['));
            }
            s.append(pull[0], pull.at(1));
            s.push_back(ch(']'));
            in_value = false;
            break;
        case primitive_table_pull_state::start_record:
            EXPECT_EQ(1U, pull.data_size());
            EXPECT_THROW(pull.at(1), std::out_of_range);
            s.append(str("<<"));
            break;
        case primitive_table_pull_state::end_record:
            EXPECT_EQ(1U, pull.data_size());
            EXPECT_THROW(pull.at(1), std::out_of_range);
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
            EXPECT_EQ(1U, pull.data_size());
            EXPECT_THROW(pull.at(1), std::out_of_range);
            s.append(str("--"));
            break;
        case primitive_table_pull_state::start_buffer:
            EXPECT_EQ(2U, pull.data_size());
            EXPECT_THROW(pull.at(2), std::out_of_range);
            if (at_start_buffer) {
                s.push_back(ch('{'));
            }
            break;
        case primitive_table_pull_state::end_buffer:
            EXPECT_EQ(1U, pull.data_size());
            EXPECT_THROW(pull.at(1), std::out_of_range);
            if (at_end_buffer) {
                s.push_back(ch('}'));
            }
            break;
        default:
            break;
        }
    }
    return s;
}

} // end unnamed

template <class ChB>
struct TestTablePull : BaseTest
{};

TYPED_TEST_SUITE_P(TestTablePull);

TYPED_TEST_P(TestTablePull, PrimitiveBasicsOnCsv)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;
   
    const auto csv = str(R"(,"col1", col2 ,col3,)" "\r\n"
                         "\n"
                         R"( cell10 ,,"cell)" "\r\r\n"
                         R"(12","cell""13 ""","")" "\n");
    auto source = make_csv_source(csv);
    primitive_table_pull pull(
        std::move(source), TypeParam::second_type::value);
    ASSERT_EQ(2U, pull.max_data_size());
    std::basic_string<char_t> s = transcript_primitive(pull);
    ASSERT_EQ(str("<<[][col1][ col2 ][col3][]>>@0,20"
                  "--"
                  "<<[ cell10 ][][cell\r\r\n12][cell\"13 \"][]>>@3,20"),
              s);

    static_assert(std::is_nothrow_move_constructible_v<decltype(pull)>);
}

TYPED_TEST_P(TestTablePull, PrimitiveBasicsOnTsv)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;
   
    const auto csv = str("\t" "col1\t" " col2 \t" "col3\t" "\r\n"
                         "\n"
                         " cell10 \t" "\t" "cell\"12\"" "\n");
    auto source = make_tsv_source(csv);
    primitive_table_pull pull(
        std::move(source), TypeParam::second_type::value);
    ASSERT_EQ(2U, pull.max_data_size());
    std::basic_string<char_t> s = transcript_primitive(pull);
    ASSERT_EQ(str("<<[][col1][ col2 ][col3][]>>@0,18"
                  "--"
                  "<<[ cell10 ][][cell\"12\"]>>@2,18"),
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

TYPED_TEST_P(TestTablePull, PrimitiveEvadeCopying)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;
   
    const auto csv = str("col1,col2,col3\n"
                         "val1,val2,val3\n");

    auto source = make_csv_source(csv);
    primitive_table_pull pull(
        std::move(source), TypeParam::second_type::value);
    ASSERT_EQ(2U, pull.max_data_size());
    std::basic_string<char_t> s = transcript_primitive(pull, true, true);

    // start_buffer and end_buffer are reported only on the beginning and the
    // end no matter how small the buffer size is
    ASSERT_EQ(str("{<<[col1][col2][col3]>>@0,14"
                   "<<[val1][val2][val3]>>@1,14}"),
              s);
}

TYPED_TEST_P(TestTablePull, PrimitiveEvadeCopyingNonconst)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;
   
    auto csv = str("col1,col2,col3\n"
                   "val1,val2,val3\n");
    auto source = make_csv_source(std::move(csv));
    primitive_table_pull pull(
        std::move(source), TypeParam::second_type::value);
    ASSERT_EQ(2U, pull.max_data_size());

    static_assert(std::is_same_v<char_t*, decltype(pull[0])>);

    std::basic_string<char_t> s = transcript_primitive(pull, true, true);
    ASSERT_EQ(str("{<<[col1][col2][col3]>>@0,14"
                   "<<[val1][val2][val3]>>@1,14}"),
              s);
}

TYPED_TEST_P(TestTablePull, Basics)
{
    using char_t = typename TypeParam::first_type;
    using pos_t = std::pair<std::size_t, std::size_t>;

    const auto str = char_helper<char_t>::str;
   
    const auto csv = str(R"(,"col1", col2 ,col3,)" "\r\n"
                         "\n"
                         R"( cell10 ,,"cell)" "\r\n"
                         R"(12","cell""13 ""","")" "\n");

    for (auto e : { false, true }) {
        auto pull = make_table_pull(make_csv_source(indirect, csv),
                                    TypeParam::second_type::value);
        pull.set_empty_physical_line_aware(e);

        ASSERT_TRUE(pull) << e;
        ASSERT_EQ(table_pull_state::before_parse, pull.state()) << e;
        ASSERT_TRUE(pull->empty());

        std::size_t i = 0;
        std::size_t j = 0;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str(""), *pull) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 0), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str("col1"), *pull) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 7), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str(" col2 "), *pull) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 14), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str("col3"), pull.c_str()) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 19), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_TRUE(pull->empty()) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 20), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::record_end, pull.state()) << e;
        ASSERT_TRUE(pull->empty()) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(0, 20), pull.get_physical_position()) << e;
        ++i;
        j = 0;

        if (e) {
            ASSERT_TRUE(pull()) << e;
            ASSERT_EQ(table_pull_state::record_end, pull.state()) << e;
            ASSERT_TRUE(pull->empty()) << e;
            ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
            ASSERT_EQ(pos_t(1, 0), pull.get_physical_position()) << e;
            ++i;
        }

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str(" cell10 "), *pull) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(2, 8), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_TRUE(pull->empty()) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(2, 9), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str("cell\r\n12"), *pull) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(3, 3), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_EQ(str("cell\"13 \""), *pull) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(3, 17), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::field, pull.state()) << e;
        ASSERT_TRUE(pull->empty()) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(3, 20), pull.get_physical_position()) << e;
        ++j;

        ASSERT_TRUE(pull()) << e;
        ASSERT_EQ(table_pull_state::record_end, pull.state()) << e;
        ASSERT_TRUE(pull->empty()) << e;
        ASSERT_EQ(std::make_pair(i, j), pull.get_position()) << e;
        ASSERT_EQ(pos_t(3, 20), pull.get_physical_position()) << e;
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

TYPED_TEST_P(TestTablePull, EvadeCopying)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;

    const auto s = str("col1,col2,col3\n"
                       "val1,val2,val3\n");
    auto source = make_csv_source(s);
    table_pull<decltype(source), std::allocator<char_t>>
        pull(std::move(source));
    std::size_t offset = 0;
    while (pull()) {
        if (pull.state() == table_pull_state::field) {
            ASSERT_EQ(s.data() + offset, pull->data())
                << "offset = " << offset;
            offset += 5;
        }
    }
}

TYPED_TEST_P(TestTablePull, EvadeCopyingNonconst)
{
    using char_t = typename TypeParam::first_type;

    const auto ch = char_helper<char_t>::ch;
    const auto str = char_helper<char_t>::str;

    auto s = str("col1,col2,col3\n"
                 "val1,val2,val3\n");
    const auto sdata = s.data();
    auto source = make_csv_source(std::move(s));
    table_pull<decltype(source), std::allocator<char_t>>
        pull(std::move(source));
    std::size_t offset = 0;
    while (pull()) {
        if (pull.state() == table_pull_state::field) {
            ASSERT_EQ(4U, std::basic_string_view<char_t>(pull.c_str()).size());
            pull.rewrite([&ch](char_t* f, char_t* l) {
                if (*f == ch('v')) {
                    *f = ch('V');
                }
                return l;
            });
            if (pull->front() != 'c') {
                ASSERT_EQ('V', *pull.c_str());
            }
            ASSERT_EQ(sdata + offset, pull.c_str()) << "offset = " << offset;
            offset += 5;
        }
    }
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

TYPED_TEST_P(TestTablePull, ToArithmetic)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;

    auto pull = make_table_pull(make_csv_source(indirect,
                                                str("X,Y\n"
                                                    "1,-51.3\n"
                                                    "1.9,\"1 234,5\"")),
                                TypeParam::second_type::value);
    pull.skip_record();

    const auto x1 = to_arithmetic<unsigned>(pull());
    ASSERT_EQ(1U, x1);
    const auto y1 = to_arithmetic<double>(pull());
    ASSERT_EQ(std::stod("-51.3"), y1);
    pull(); // to record-end

    pull(); // to "1.9"
    // As int
    ASSERT_THROW(to_arithmetic<int>(pull), text_value_invalid_format);
    const auto x2i = to_arithmetic<std::optional<int>>(pull);
    ASSERT_FALSE(x2i.has_value());
    // As double
    const auto x2d = to_arithmetic<std::optional<double>>(pull);
    ASSERT_TRUE(x2d.has_value());
    ASSERT_EQ(std::stod("1.9"), x2d);

    pull(); // to "\"1 234,5\""
    pull.rewrite(numpunct_replacer_to_c(std::locale(std::locale::classic(),
        new french_style_numpunct<char_t>)));
    const double y2 = to_arithmetic<double>(pull);
    ASSERT_EQ(std::stod("1234.5"), y2);
}

TYPED_TEST_P(TestTablePull, ParsePoint)
{
    using char_t = typename TypeParam::first_type;

    const auto ch = char_helper<char_t>::ch;
    const auto strv = char_helper<char_t>::strv;

    const auto row1 = "Col1,\"Col2\""sv;
    const auto row2_val1 = "Val11"sv;
    const auto row2_val2 = "Val21"sv;

    const auto str = strv(row1) + ch('\n') +
                     ch('\"') + strv(row2_val1) + ch('\"') + ch(',') +
                     strv(row2_val2) + strv("\n\n");
    auto pull = make_table_pull(make_csv_source(str),
                                TypeParam::second_type::value);

    pull.skip_record();
    ASSERT_EQ(table_pull_state::record_end, pull.state());   // precondition
    ASSERT_EQ(row1.size(), pull.get_parse_point());
    pull(); // "Val11"
    ASSERT_GE(pull.get_parse_point(),
        row1.size() + 2/*LF+DQUOTE*/ + row2_val1.size());
    ASSERT_LT(pull.get_parse_point(),
        row1.size() + 2/*LF+DQUOTE*/ + row2_val1.size() + 2/*DQUOTE+COMMA*/);
    pull(); // Val21
    pull(); // EOF
    ASSERT_EQ(str.size(), pull.get_parse_point() + 2/*LF+LF*/);
}

REGISTER_TYPED_TEST_SUITE_P(TestTablePull,
    PrimitiveBasicsOnCsv, PrimitiveBasicsOnTsv,
    PrimitiveMove, PrimitiveEvadeCopying, PrimitiveEvadeCopyingNonconst,
    Basics, SkipRecord, SkipField, Error, EvadeCopying, EvadeCopyingNonconst,
    Move, ToArithmetic, ParsePoint);

namespace {

using ChBs = testing::Types<
    std::pair<char, std::integral_constant<std::size_t, 1>>,
    std::pair<char, std::integral_constant<std::size_t, 2>>,
    std::pair<char, std::integral_constant<std::size_t, 4>>,
    std::pair<char, std::integral_constant<std::size_t, 1024>>,
    std::pair<wchar_t, std::integral_constant<std::size_t, 1>>,
    std::pair<wchar_t, std::integral_constant<std::size_t, 2>>,
    std::pair<wchar_t, std::integral_constant<std::size_t, 4>>,
    std::pair<wchar_t, std::integral_constant<std::size_t, 1024>>>;

} // end unnamed

INSTANTIATE_TYPED_TEST_SUITE_P(CharsBufferSizes, TestTablePull, ChBs, );
