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

template <class Ch>
struct TestRecordTranslator : BaseTest
{};

namespace {

using Chs = testing::Types<char, wchar_t>;

} // end unnamed

TYPED_TEST_SUITE(TestRecordTranslator, Chs, );

TYPED_TEST(TestRecordTranslator, All)
{
    using char_t = TypeParam;
    using string_t = std::basic_string<char_t>;
    using string_view_t = std::basic_string_view<char_t>;

    const auto str = char_helper<char_t>::str;

    using planet_t = std::tuple<std::size_t, string_t, double, double>;
    std::vector<planet_t> planets;
    auto t = make_basic_record_translator<char_t, std::char_traits<char_t>>(
        [&planets](string_t&& name, std::optional<std::size_t> index,
                   double mass, double orbitalPeriod) {
            planets.emplace_back(
                index.value_or(0U), std::move(name), mass, orbitalPeriod);
        },
        field_spec<string_t>(str("Name")),
        field_spec<std::optional<std::size_t>>(string_view_t(str("#"))),
        field_spec(str("Mass").c_str(),
            arithmetic_field_translator_factory<
                double, replace_if_skipped<double>>(-1.0)),
        field_spec<double>(
            [name = str("orbital period")](string_view_t s) {
                return std::equal(
                    s.cbegin(), s.cend(),
                    name.cbegin(), name.cend(),
                    [](char_t l, char_t r) {
                        const auto tolower = char_helper<char_t>::tolower;
                        return tolower(l) == tolower(r);
                    });
            }));
    parse_tsv(
        make_char_input(str(
            "#\tName\tOrbital Period\tMass\n"
            "3\tEarth\t1\t1\n"
            "2\tVenus\t0.615\t0.815\n"
            "4\tMars\t1.88\t0.107\n"
            "\tEris\t561\t0.000276\n"
            "\tSedna\t1.29e4\n")),
        std::move(t));

    std::vector<planet_t> expected;
    expected.emplace_back(
        3, str("Earth"), std::stod("1"), std::stod("1"));
    expected.emplace_back(
        2, str("Venus"), std::stod("0.815"), std::stod("0.615"));
    expected.emplace_back(
        4, str("Mars"), std::stod("0.107"), std::stod("1.88"));
    expected.emplace_back(
        0, str("Eris"), std::stod("0.000276"), std::stod("561"));
    expected.emplace_back(
        0, str("Sedna"), -1.0, std::stod("1.29e4"));
    ASSERT_EQ(expected, planets);
}
