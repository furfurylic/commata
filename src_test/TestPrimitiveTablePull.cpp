/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include <commata/parse_csv.hpp>
#include <commata/parse_tsv.hpp>
#include <commata/table_pull.hpp>

#include "BaseTest.hpp"

using namespace commata;
using namespace commata::test;

namespace {

template <class PrimitiveTablePull>
auto transcript(PrimitiveTablePull&& pull,
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
struct TestPrimitiveTablePull : BaseTest
{};

TYPED_TEST_SUITE_P(TestPrimitiveTablePull);

TYPED_TEST_P(TestPrimitiveTablePull, BasicsOnCsv)
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
    std::basic_string<char_t> s = transcript(pull);
    ASSERT_EQ(str("<<[][col1][ col2 ][col3][]>>@0,20"
                  "--"
                  "<<[ cell10 ][][cell\r\r\n12][cell\"13 \"][]>>@3,20"),
              s);

    static_assert(std::is_nothrow_move_constructible_v<decltype(pull)>);
}

TYPED_TEST_P(TestPrimitiveTablePull, BasicsOnTsv)
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
    std::basic_string<char_t> s = transcript(pull);
    ASSERT_EQ(str("<<[][col1][ col2 ][col3][]>>@0,18"
                  "--"
                  "<<[ cell10 ][][cell\"12\"]>>@2,18"),
              s);

    static_assert(std::is_nothrow_move_constructible_v<decltype(pull)>);
}

TYPED_TEST_P(TestPrimitiveTablePull, PartialOnCsv)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;
   
    const auto csv = str(",10, 20, 30\nA,\"\",B,C,D");
    auto source = make_csv_source(csv);
    auto pull = make_primitive_table_pull<
        primitive_table_pull_handle::update
      | primitive_table_pull_handle::finalize>(
        std::move(source), TypeParam::second_type::value);
    std::basic_string<char_t> s = transcript(pull);
    ASSERT_EQ(str("[][10][ 20][ 30][A][][B][C][D]"), s);
}

TYPED_TEST_P(TestPrimitiveTablePull, Move)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;
   
    const auto csv = str("A,B\nC,D");
    auto source = make_csv_source(csv);
    auto pull = make_primitive_table_pull(
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

TYPED_TEST_P(TestPrimitiveTablePull, EvadeCopying)
{
    using char_t = typename TypeParam::first_type;

    const auto str = char_helper<char_t>::str;
   
    const auto csv = str("col1,col2,col3\n"
                         "val1,val2,val3\n");

    auto source = make_csv_source(csv);
    primitive_table_pull pull(
        std::move(source), TypeParam::second_type::value);
    ASSERT_EQ(2U, pull.max_data_size());
    std::basic_string<char_t> s = transcript(pull, true, true);

    // start_buffer and end_buffer are reported only on the beginning and the
    // end no matter how small the buffer size is
    ASSERT_EQ(str("{<<[col1][col2][col3]>>@0,14"
                   "<<[val1][val2][val3]>>@1,14}"),
              s);
}

TYPED_TEST_P(TestPrimitiveTablePull, EvadeCopyingNonconst)
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

    std::basic_string<char_t> s = transcript(pull, true, true);
    ASSERT_EQ(str("{<<[col1][col2][col3]>>@0,14"
                   "<<[val1][val2][val3]>>@1,14}"),
              s);
}

REGISTER_TYPED_TEST_SUITE_P(TestPrimitiveTablePull,
    BasicsOnCsv, BasicsOnTsv, PartialOnCsv,
    Move, EvadeCopying, EvadeCopyingNonconst);

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

INSTANTIATE_TYPED_TEST_SUITE_P(
    CharsBufferSizes, TestPrimitiveTablePull, ChBs, );
