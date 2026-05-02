/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <commata/char_input.hpp>
#include <commata/parse_csv.hpp>
#include <commata/parse_tsv.hpp>
#include <commata/record_translator.hpp>

#include "BaseTest.hpp"
#include "logging_allocator.hpp"

using namespace std::literals;
using namespace std::placeholders;

using namespace commata;
using namespace commata::test;

template <class Ch>
struct TestRecordTranslatorBasics : BaseTest
{};

namespace {

using Chs = testing::Types<char, wchar_t>;

} // end unnamed

TYPED_TEST_SUITE(TestRecordTranslatorBasics, Chs, );

TYPED_TEST(TestRecordTranslatorBasics, All)
{
    using char_t = TypeParam;
    using string_t = std::basic_string<char_t>;
    using string_view_t = std::basic_string_view<char_t>;

    const auto str = char_helper<char_t>::str;

    using planet_t =
        std::tuple<std::size_t, string_t, double, double, double>;
    std::vector<planet_t> planets;
    const std::locale loc(std::locale::classic(),
        new french_style_numpunct<char_t>);
    auto t = make_basic_record_translator<char_t>(
        [&planets](string_t&& name, std::optional<std::size_t> index,
                   double mass, double eccentricity, double orbital_period) {
            planets.emplace_back(index.value_or(0U),
                std::move(name), mass, orbital_period, eccentricity);
        },
        field_spec<string_t>(str("Name")),
        field_spec<std::optional<std::size_t>>(string_view_t(str("#"))),
        field_spec(str("Mass").c_str(),
            locale_based_arithmetic_field_translator_factory(loc, -1.0)),
        field_spec<double>(string_view_t(str("Eccentricity")), loc),
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
            "#\tName\tOrbital Period\tEccentricity\tMass\n"
            "3\tEarth\t1\t0,0167\t1\n"
            "2\tVenus\t0.615\t6,77e-3\t0,815\n"
            "4\tMars\t1.88\t0,0934\t0,107\n"
            "\tEris\t561\t0,433\t0,000276\n"
            "\tSedna\t1.29e4\t0,850\n")),
        std::move(t));

    std::vector<planet_t> expected;
    expected.emplace_back(3, str("Earth"),
        std::stod("1"), std::stod("1"), std::stod("0.0167"));
    expected.emplace_back(2, str("Venus"),
        std::stod("0.815"), std::stod("0.615"), std::stod("0.00677"));
    expected.emplace_back(4, str("Mars"),
        std::stod("0.107"), std::stod("1.88"), std::stod("0.0934"));
    expected.emplace_back(0, str("Eris"),
        std::stod("0.000276"), std::stod("561"), std::stod("0.433"));
    expected.emplace_back(0, str("Sedna"),
        -1.0, std::stod("1.29e4"), std::stod("0.850"));
    ASSERT_EQ(expected, planets);
}

struct TestRecordTranslator : BaseTest
{};

TEST_F(TestRecordTranslator, Refs)
{
    const std::string name = "Name"s;
    const arithmetic_field_translator_factory<int> to_int;

    std::vector<std::pair<std::string, int>> actual;
    auto s = make_record_translator(
        [&actual](std::string&& name, int&& value) {
            actual.emplace_back(std::move(name), std::move(value));
        },
        // tuple<&, &&>
        std::tuple_cat(
            std::tie(name),
            std::forward_as_tuple(
                string_field_translator_factory<std::string>())),
        // tuple<&&, &>
        std::tuple_cat(
            std::forward_as_tuple("Value"s),
            std::tie(to_int)));
    parse_csv("Name,Value\nHaircut,100", std::move(s));

    decltype(actual) expected = {{ "Haircut", 100 }};
    ASSERT_EQ(expected, actual);
}

TEST_F(TestRecordTranslator, Allocators)
{
    std::vector<std::size_t> allocations;
    logging_allocator<char> a(allocations);

    std::vector<int> actual;
    std::basic_string a100(100, 'a', a);
    default_field_translator_factory_t<int> fac;
    auto s = make_basic_record_translator<char>(
        std::allocator_arg, a,
        [&actual](int value) {
            actual.emplace_back(value);
        },
        std::tie(a100, fac));
    parse_csv(a100 + "\n-10", std::move(s));

    decltype(actual) expected = { -10 };
    ASSERT_EQ(expected, actual);

    ASSERT_GE(
        std::count_if(
            allocations.cbegin(), allocations.cend(),
            std::bind(std::greater_equal<>(), _1, 100U)),
        3U);
        // #1: making of a100
        // #2: making of a copy of a100 in the string pred
        // #3: making of a100 + "\n-10"
}

TEST_F(TestRecordTranslator, Duplication)
{
    std::string s1;
    double s2;
    int s3;

    auto t = make_record_translator(
        [&s1, &s2, &s3](std::string&& f1, double f2, int f3) {
            std::tie(s1, s2, s3) =
                std::forward_as_tuple(std::move(f1), f2, f3);
        },
        field_spec<std::string>("ABC"),
        field_spec<double>("ABC"),
        field_spec<int>("ABC"));
    parse_tsv(
        make_char_input(
            "ABC\tABC\tABC\n"
            "1\t2\t3\n"),
        std::move(t));
    ASSERT_EQ("1"sv, s1);
    ASSERT_EQ(2.0, s2);
    ASSERT_EQ(3, s3);
}

// Compile-time tests of deduction guides

static_assert(std::is_same_v<
    arithmetic_field_translator_factory<int,
        replace_if_skipped<int>>,
    decltype(arithmetic_field_translator_factory(100))>);
static_assert(std::is_same_v<
    arithmetic_field_translator_factory<double,
        replace_if_skipped<double>, replace_if_conversion_failed<double>>,
    decltype(arithmetic_field_translator_factory(-0.1, 1.2))>);
static_assert(std::is_same_v<
    arithmetic_field_translator_factory<float,
        replace_if_skipped<float>, fail_if_conversion_failed>,
    decltype(arithmetic_field_translator_factory(-0.1f, replacement_fail))>);
static_assert(std::is_same_v<
    arithmetic_field_translator_factory<char,
        replace_if_skipped<char>, ignore_if_conversion_failed>,
    decltype(arithmetic_field_translator_factory('?', replacement_ignore))>);

static_assert(std::is_same_v<
    locale_based_arithmetic_field_translator_factory<double,
        replace_if_skipped<double>>,
    decltype(locale_based_arithmetic_field_translator_factory(
        std::declval<std::locale>(), 1.0))>);
static_assert(std::is_same_v<
    locale_based_arithmetic_field_translator_factory<unsigned,
        replace_if_skipped<unsigned>, replace_if_conversion_failed<unsigned>>,
    decltype(locale_based_arithmetic_field_translator_factory(
        std::declval<std::locale>(), 5U, 10U))>);
static_assert(std::is_same_v<
    locale_based_arithmetic_field_translator_factory<long,
        replace_if_skipped<long>, fail_if_conversion_failed>,
    decltype(locale_based_arithmetic_field_translator_factory(
        std::declval<std::locale>(), 5L, replacement_fail))>);
static_assert(std::is_same_v<
    locale_based_arithmetic_field_translator_factory<unsigned long,
        replace_if_skipped<unsigned long>, ignore_if_conversion_failed>,
    decltype(locale_based_arithmetic_field_translator_factory(
        std::declval<std::locale>(), 0UL, replacement_ignore))>);

static_assert(std::is_same_v<
    string_field_translator_factory<std::string,
        replace_if_skipped<std::string>>,
    decltype(string_field_translator_factory("..."s))>);
static_assert(std::is_same_v<
    string_field_translator_factory<std::string,
        replace_if_skipped<std::string>>,
    decltype(string_field_translator_factory("..."sv))>);
static_assert(std::is_same_v<
    string_field_translator_factory<std::wstring,
        replace_if_skipped<std::wstring>>,
    decltype(string_field_translator_factory(L"..."))>);

static_assert(std::is_same_v<
    string_view_field_translator_factory<std::wstring_view,
        replace_if_skipped<std::wstring_view>>,
    decltype(string_view_field_translator_factory(L"!!!"sv))>);
static_assert(std::is_same_v<
    string_view_field_translator_factory<std::string_view,
        replace_if_skipped<std::string_view>>,
    decltype(string_view_field_translator_factory("!!!"))>);
