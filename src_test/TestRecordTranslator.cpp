/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <commata/char_input.hpp>
#include <commata/parse_tsv.hpp>
#include <commata/record_translator.hpp>

#include "BaseTest.hpp"

using namespace std::literals;

using namespace commata;
using namespace commata::test;

struct TestRecordTranslator : BaseTest
{};

TEST_F(TestRecordTranslator, All)
{
    using planet_t = std::tuple<std::size_t, std::string, double>;
    std::vector<planet_t> planets;
    auto t = make_record_translator(
        [&planets](std::string&& name, std::optional<std::size_t>&& index,
                   double mass) {
            planets.emplace_back(index.value_or(0U), std::move(name), mass);
        },
        field_spec<std::string>("Name"),
        field_spec<std::optional<std::size_t>>("#"sv),
        field_spec<double>("Mass"s));
    parse_tsv(make_char_input(
        "#\tName\tMass\n"
        "3\tEarth\t1\n"
        "2\tVenus\t0.815\n"
        "4\tMars\t0.107\n"
        "\tEris\t0.0002757\n"),
        std::move(t));

    std::vector<planet_t> expected;
    expected.emplace_back(3, "Earth", std::stod("1"));
    expected.emplace_back(2, "Venus", std::stod("0.815"));
    expected.emplace_back(4, "Mars", std::stod("0.107"));
    expected.emplace_back(0, "Eris", std::stod("0.0002757"));
    ASSERT_EQ(expected, planets);
}
