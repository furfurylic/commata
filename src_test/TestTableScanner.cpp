/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifdef _MSC_VER
//#pragma warning(disable:4494)
//#pragma warning(disable:4996)
#endif

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <deque>
#include <functional>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <commata/field_scanners.hpp>
#include <commata/parse_csv.hpp>
#include <commata/text_error.hpp>
#include <commata/table_scanner.hpp>

#include "BaseTest.hpp"
#include "fancy_allocator.hpp"
#include "tracking_allocator.hpp"

using namespace std::literals::string_literals;

using namespace commata;
using namespace commata::test;

typedef testing::Types<char, wchar_t> Chs;

template <class Ch>
struct TestFieldTranslatorForArithmeticTypes : BaseTest
{};

TYPED_TEST_SUITE(TestFieldTranslatorForArithmeticTypes, Chs);

TYPED_TEST(TestFieldTranslatorForArithmeticTypes, Correct)
{
    const auto str = char_helper<TypeParam>::str;

    std::deque<int> xs;
    std::deque<double> ys;

    basic_table_scanner<TypeParam> h(1U);
    h.set_field_scanner(0, make_field_translator(xs,
        replacement_fail, replace_if_conversion_failed<int>(300)));
    h.set_field_scanner(1, make_field_translator(ys,
        replacement_ignore, replace_if_conversion_failed<double>(1.0, -1.0)));

    std::basic_string<TypeParam> s = str("X,Y\n 1,-2.5\n,x\n3");

    try {
        parse_csv(s, std::move(h));
    } catch (const text_error& e) {
        FAIL() << text_error_info(e);
    }

    const std::deque<int> xs_expected = { 1, 300, 3 };
    ASSERT_EQ(xs_expected, xs);
    const std::deque<double> ys_expected = { -2.5, -1.0 };
    ASSERT_EQ(ys_expected, ys);
}

TYPED_TEST(TestFieldTranslatorForArithmeticTypes, Copy)
{
    using char_t = TypeParam;
    using string_t = std::basic_string<TypeParam>;

    const auto str0 = char_helper<char_t>::str0;

    std::vector<unsigned> values;
    auto t = make_field_translator(values, replace_if_skipped<unsigned>(100U));
    auto u = t;

    const string_t s10 = str0("10");
    t(s10.data(), s10.data() + 2);

    u();

    const string_t s20 = str0("20");
    t(s20.data(), s20.data() + 2);

    const std::vector<unsigned> expected = { 10U, 100U, 20U };
    ASSERT_EQ(expected, values);
}

template <class Ch>
struct TestFieldTranslatorForStringTypes : BaseTest
{};

TYPED_TEST_SUITE(TestFieldTranslatorForStringTypes, Chs);

TYPED_TEST(TestFieldTranslatorForStringTypes, Correct)
{
    const auto str = char_helper<TypeParam>::str;

    std::deque<std::basic_string<TypeParam>> values;

    basic_table_scanner<TypeParam> h;
    h.set_field_scanner(0, make_field_translator(values));

    std::basic_string<TypeParam> s = str("ABC  \n\"xy\rz\"\n\"\"");

    try {
        parse_csv(s, std::move(h));
    } catch (const text_error& e) {
        FAIL() << text_error_info(e);
    }

    ASSERT_EQ(3U, values.size());
    ASSERT_EQ(str("ABC  "), values[0]);
    ASSERT_EQ(str("xy\rz"), values[1]);
    ASSERT_TRUE(values[2].empty()) << values[2];
}

TYPED_TEST(TestFieldTranslatorForStringTypes, Copy)
{
    using char_t = TypeParam;
    using string_t = std::basic_string<TypeParam>;

    const auto str = char_helper<char_t>::str;

    std::vector<string_t> values;
    auto t = make_field_translator(values,
        replace_if_skipped<string_t>(str("1")));
    auto u = t;
    t(str("10"));
    u();
    t(str("20"));

    const std::vector<string_t> expected = { str("10"), str("1"), str("20") };
    ASSERT_EQ(expected, values);
}

TYPED_TEST(TestFieldTranslatorForStringTypes, View)
{
    using char_t = TypeParam;
    using string_t = std::basic_string<TypeParam>;
    using string_view_t = std::basic_string_view<TypeParam>;

    const auto str = char_helper<char_t>::str;

    std::basic_ostringstream<char_t> stream;
    string_t replacement = str("!!!");
    auto t = make_field_translator<string_view_t>(
        std::ostream_iterator<string_view_t, char_t>(stream),
        replace_if_skipped<string_view_t>(replacement));
    basic_table_scanner<char_t> scanner;
    scanner.set_field_scanner(1, std::move(t));

    try {
        parse_csv(str("1,ABC\n2\n3,XYZ"), std::move(scanner));
    } catch (const text_error& e) {
        FAIL() << text_error_info(e);
    }

    ASSERT_EQ(str("ABC!!!XYZ"), std::move(stream).str());
}

namespace {

template <class Ch>
struct french_style_numpunct : std::numpunct<Ch>
{
    explicit french_style_numpunct(std::size_t r = 0) :
        std::numpunct<Ch>(r)
    {}

protected:
    Ch do_decimal_point() const override
    {
        return char_helper<Ch>::ch(',');
    }

    Ch do_thousands_sep() const override
    {
        return char_helper<Ch>::ch(' ');
    }

    std::string do_grouping() const override
    {
        return "\003";
    }
};

}

template <class Ch>
struct TestLocaleBased : BaseTest
{};

TYPED_TEST_SUITE(TestLocaleBased, Chs);

TYPED_TEST(TestLocaleBased, FrenchStyle)
{
    const auto str0 = char_helper<TypeParam>::str0;

    std::vector<int> values0;
    std::deque<double> values1;

    std::locale loc(std::locale::classic(),
        new french_style_numpunct<TypeParam>);

    auto s0 = str0("100 000");
    auto s1 = str0("12 345 678,5");

    auto t = make_field_translator(values0, loc, replacement_ignore);
    auto u = make_field_translator(values1, loc, replacement_fail);

    t(&s0[0], &s0[0] + (s0.size() - 1));
    t();
    u(&s1[0], &s1[0] + (s1.size() - 1));
    ASSERT_THROW(u(), field_not_found);

    ASSERT_EQ(1U, values0.size());
    ASSERT_EQ(100000, values0.back());
    ASSERT_EQ(1U, values1.size());
    ASSERT_EQ(12345678.5, values1.back());
}

TYPED_TEST(TestLocaleBased, Copy)
{
    const auto str0 = char_helper<TypeParam>::str0;

    std::deque<double> values;
    const auto f = [&values](double a) {
        values.push_back(a);
    };

    std::locale loc(std::locale::classic(),
        new french_style_numpunct<TypeParam>);

    auto empty = str0("");
    auto s0 = str0("12 345 678,5");
    auto s1 = str0("-9 999");

    auto t = make_field_translator<double>(f, loc, 33.33,
        replace_if_conversion_failed<double>(777.77));
    auto u = t;

    t(&s0[0], &s0[0] + (s0.size() - 1));
    u();
    t(&empty[0], &empty[0]);
    u(&s1[0], &s1[0] + (s1.size() - 1));

    const std::deque<double> expected = { 12345678.5, 33.33, 777.77, -9999.0 };
    ASSERT_EQ(expected, values);
}

static_assert(std::uses_allocator_v<table_scanner, std::allocator<char>>);

template <class Ch>
struct TestTableScanner : BaseTest
{};

TYPED_TEST_SUITE(TestTableScanner, Chs);

TYPED_TEST(TestTableScanner, BufferSizeEOF)
{
    const auto str = char_helper<TypeParam>::str;

    basic_table_scanner<TypeParam> h(0);
    std::vector<long> values;
    h.set_field_scanner(0, make_field_translator(values));
    parse_csv(str("12\n34\n5678"), std::move(h), 10u);
}

TYPED_TEST(TestTableScanner, Indexed)
{
    using string_t = std::basic_string<TypeParam>;

    const auto ch = char_helper<TypeParam>::ch;
    const auto str = char_helper<TypeParam>::str;

    std::deque<long> values0;
    std::vector<double> values21;
    std::deque<double> values22;
    std::list<string_t> values3;
    std::set<unsigned short> values4;
    int values5[2];
    string_t values6;

    basic_table_scanner<TypeParam> h(1U);
    h.set_field_scanner(0,
        make_field_translator<long>(std::front_inserter(values0)));
    h.set_field_scanner(2, make_field_translator(values22));
    h.set_field_scanner(2);
    h.set_field_scanner(2, make_field_translator(values22));  // overridden
    h.set_field_scanner(2, make_field_translator(values21));
    h.set_field_scanner(5, nullptr);
    h.set_field_scanner(4, make_field_translator(values4));
    h.set_field_scanner(3, make_field_translator(values3));
    h.set_field_scanner(5, make_field_translator<int>(values5));
    h.set_field_scanner(6, make_field_translator<string_t>(
        [&values6, ch](string_t&& s){
            values6 += ch('[');
            values6 += std::move(s);
            values6 += ch(']');
        }));

    ASSERT_EQ(
        typeid(make_field_translator(values21)),
        h.get_field_scanner_type(2));
    ASSERT_EQ(typeid(void), h.get_field_scanner_type(1));
    ASSERT_EQ(typeid(void), h.get_field_scanner_type(100));

    ASSERT_NE(nullptr,
        h.template get_field_scanner<
            decltype(make_field_translator(values3))>(3));
    ASSERT_EQ(nullptr,
        h.template get_field_scanner<
        decltype(make_field_translator(values4))>(3));
    ASSERT_EQ(nullptr, h.template get_field_scanner<void>(1));
    ASSERT_EQ(nullptr, h.template get_field_scanner<void>(100));

    std::basic_stringstream<TypeParam> s;
    s << "F0,F1,F2,F3,F4,F5,F6\r"
         "50,__, 101.2 ,XYZ,  200,1,fixa\n"
         R"(-3,__,3.00e9,"""ab"")" "\r"
         R"(c",200,2,tive)" "\n";
    try {
        parse_csv(s, std::move(h));
    } catch (const text_error& e) {
        FAIL() << text_error_info(e);
    }

    const std::deque<long> expected0 = { -3, 50 };
    const std::vector<double> expected21 = { 101.2, 3.00e9 };
    const std::list<string_t> expected3 = { str("XYZ"), str("\"ab\"\rc") };
    const std::set<unsigned short> expected4 = { 200 };
    ASSERT_EQ(expected0, values0);
    ASSERT_EQ(expected21, values21);
    ASSERT_TRUE(values22.empty());
    ASSERT_EQ(expected3, values3);
    ASSERT_EQ(expected4, values4);
    ASSERT_EQ(1, values5[0]);
    ASSERT_EQ(2, values5[1]);
    ASSERT_EQ(str("[fixa][tive]"), values6);
}

TYPED_TEST(TestTableScanner, RecordEndScanner)
{
    const auto str = char_helper<TypeParam>::str;

    std::vector<std::basic_string<TypeParam>> v;

    basic_table_scanner<TypeParam> h(1U);
    h.set_field_scanner(0, make_field_translator(v));

    ASSERT_FALSE(h.has_record_end_scanner());
    auto f = [&v, str] { v.push_back(str("*")); };
    h.set_record_end_scanner(f);
    ASSERT_TRUE(h.has_record_end_scanner());
    ASSERT_EQ(typeid(f), h.get_record_end_scanner_type());
    ASSERT_NE(nullptr, h.template get_record_end_scanner<decltype(f)>());
    ASSERT_EQ(nullptr, h.template get_record_end_scanner<int>());

    try {
        std::basic_stringstream<TypeParam> s;
        s << "Word\r"
             R"("aban)" "\n"
             R"(don")" "\n"
             "Abbott";  // elaborately does not end with CR/LF
        parse_csv(s, std::move(h));
    } catch (const text_error& e) {
        FAIL() << text_error_info(e);
    }

    std::vector<std::basic_string<TypeParam>> expected;
    expected.push_back(str("aban\ndon"));
    expected.push_back(str("*"));
    expected.push_back(str("Abbott"));
    expected.push_back(str("*"));
    ASSERT_EQ(expected, v);
}

TYPED_TEST(TestTableScanner, ComplexRecordEndScanner)
{
    const auto str = char_helper<TypeParam>::str;

    std::vector<int> ns;
    std::vector<std::basic_string<TypeParam>> ss;

    basic_table_scanner<TypeParam> h;
    int record_num = 0;
    h.set_field_scanner(0, make_field_translator(ns));
    h.set_record_end_scanner([&record_num, &ss](auto& scanner) {
        ++record_num;
        switch (record_num) {
        case 2:
            scanner.set_field_scanner(0, make_field_translator(ss));
            break;
        case 4:
            return false;
        default:
            break;
        }
        return true;
    });

    try {
        std::basic_stringstream<TypeParam> s;
        s << "100\r"
             "200\r"
             "ABC\r"
             "XYZ\r"
             "\"";  // Bad CSV but does not reach here
        parse_csv(s, std::move(h));
    } catch (const text_error& e) {
        FAIL() << text_error_info(e);
    }

    ASSERT_EQ(2U, ns.size());
    ASSERT_EQ(100, ns[0]);
    ASSERT_EQ(200, ns[1]);
    ASSERT_EQ(2U, ss.size());
    ASSERT_EQ(str("ABC"), ss[0]);
    ASSERT_EQ(str("XYZ"), ss[1]);
}

TYPED_TEST(TestTableScanner, MultilinedHeader)
{
    std::deque<long> values;

    basic_table_scanner<TypeParam> h(3U);
    h.set_field_scanner(0, make_field_translator(values));

    std::basic_stringstream<TypeParam> s;
    s << "H1\r"
         "H2\n"
         "H3\n"
         "12345";
    try {
        parse_csv(s, std::move(h));
    } catch (const text_error& e) {
        FAIL() << text_error_info(e);
    }

    const std::deque<long> expected = { 12345 };
    ASSERT_EQ(expected, values);
}

TYPED_TEST(TestTableScanner, SkippedWithNoErrors)
{
    using string_t = std::basic_string<TypeParam>;

    const auto str = char_helper<TypeParam>::str;

    std::deque<string_t> values0;
    std::deque<int> values1;

    basic_table_scanner<TypeParam> h;
    h.set_field_scanner(0, make_field_translator(
        values0, replace_if_skipped<string_t>()));
    h.set_field_scanner(1, make_field_translator(
        values1, replace_if_skipped<int>(50)));

    const auto scanner1 = h.template
        get_field_scanner<decltype(make_field_translator(
            values1, replace_if_skipped<int>()))>(1U);
    ASSERT_NE(scanner1, nullptr);
    ASSERT_EQ(50, *scanner1->get_skipping_handler()());

    std::basic_stringstream<TypeParam> s;
    s << "XYZ,20\n"
         "\n"
         "A";
    try {
        parse_csv(s, make_empty_physical_line_aware(std::move(h)));
    } catch (const text_error& e) {
        FAIL() << text_error_info(e);
    }

    const std::deque<string_t> expected0 =
        { str("XYZ"), string_t(), str("A") };
    const std::deque<int> expected1 = { 20, 50, 50 };
    ASSERT_EQ(expected0, values0);
    ASSERT_EQ(expected1, values1);
}

TYPED_TEST(TestTableScanner, SkippedWithErrors)
{
    std::deque<int> values0;
    std::deque<int> values1;

    basic_table_scanner<TypeParam> h;
    h.set_field_scanner(0, make_field_translator(
        values0, replace_if_skipped<int>(10)));
    h.set_field_scanner(1, make_field_translator(values1));

    std::basic_stringstream<TypeParam> s;
    s << "10,20\n"
        "-5";
    try {
        parse_csv(s, std::move(h));
        FAIL();
    } catch (const field_not_found& e) {
        ASSERT_TRUE(e.get_physical_position());
        ASSERT_EQ(1U, e.get_physical_position()->first);
    }

    const std::deque<int> expected0 = { 10, -5 };
    const std::deque<int> expected1 = { 20 };
    ASSERT_EQ(expected0, values0);
    ASSERT_EQ(expected1, values1);
}

TYPED_TEST(TestTableScanner, HeaderScan)
{
    const auto str = char_helper<TypeParam>::str;

    std::vector<unsigned> ids;
    std::vector<short> values1;

    basic_table_scanner<TypeParam> h(
        [&ids, &values1, str]
        (std::size_t j, const auto* range, auto& f) {
            const std::basic_string<TypeParam>
                field_name(range->first, range->second);
            if (field_name == str("ID")) {
                f.set_field_scanner(j, make_field_translator(ids));
                return true;
            } else if (field_name == str("Value1")) {
                f.set_field_scanner(j, make_field_translator(values1));
                return false;
            } else {
                return true;
            }
        });

    auto s = str("ID,Value0,Value1,Value1\n"
                 "1,ABC,123,xyz\n");
    try {
        parse_csv(s, std::move(h));
    } catch (const text_error& e) {
        FAIL() << text_error_info(e);
    }

    ASSERT_EQ(1U, ids.size());
    ASSERT_EQ(1U, values1.size());

    ASSERT_EQ(1U, ids[0]);
    ASSERT_EQ(123, values1[0]);
}

TYPED_TEST(TestTableScanner, HeaderScanToTheEnd)
{
    const auto str = char_helper<TypeParam>::str;

    basic_table_scanner<TypeParam> h(
        [](std::size_t j, const auto* range, auto&) {
            if (j == 1) {
                if (range) {
                    throw std::runtime_error("Header's end with a range");
                }
                return false;   // cease scanning
            } else if (!range) {
                throw std::runtime_error("Not a header's end without a range");
            } else {
                return true;    // scan more
            }
    });

    ASSERT_NO_THROW(parse_csv(str("A\n1\n"), std::move(h)));
}

namespace {

template <class Ch>
class two_lined_fx_header_scanner
{
    std::map<std::basic_string<Ch>, std::vector<double>>* fx_;

    std::size_t i_ = 0;
    std::map<std::size_t, std::basic_string<Ch>> first_ccys_;

public:
    explicit two_lined_fx_header_scanner(
        std::map<std::basic_string<Ch>, std::vector<double>>& fx) :
        fx_(&fx)
    {}

    bool operator()(std::size_t j,
        const std::pair<Ch*, Ch*>* range, basic_table_scanner<Ch>& s)
    {
        if (i_ == 0) {
            if (range) {
                first_ccys_[j].assign(range->first, range->second);
            } else {
                ++i_;
            }
        } else {
            if (range) {
                std::basic_string<Ch> name(std::move(first_ccys_[j]));
                name.append(range->first, range->second);
                s.set_field_scanner(j,
                    make_field_translator((*fx_)[std::move(name)]));
            } else {
                return false;
            }
        }
        return true;
    }
};

}

TYPED_TEST(TestTableScanner, MultilinedHeaderScan)
{
    const auto str = char_helper<TypeParam>::str;

    std::map<std::basic_string<TypeParam>, std::vector<double>> fx;
    two_lined_fx_header_scanner<TypeParam> h(fx);
    basic_table_scanner<TypeParam> scanner(std::move(h));
    auto s = str("AUD,AUD,EUR\r"
                 "JPY,USD,USD\r"
                 "80.0,0.9,1.3\r"
                 "82.1,0.91,1.35");
    parse_csv(s, std::move(scanner));

    std::vector<double> aud_jpy = { 80.0, 82.1 };
    ASSERT_EQ(aud_jpy, fx[str("AUDJPY")]);

    std::vector<double> aud_usd = { 0.9, 0.91 };
    ASSERT_EQ(aud_usd, fx[str("AUDUSD")]);

    std::vector<double> eur_usd = { 1.3, 1.35 };
    ASSERT_EQ(eur_usd, fx[str("EURUSD")]);
}

namespace {

template <class Ch>
class basic_table_scanner_wrapper
{
    basic_table_scanner<Ch> s_;

public:
    using char_type = Ch;

    basic_table_scanner_wrapper(basic_table_scanner<Ch>&& s) :
        s_(std::move(s))
    {}

    void start_buffer(Ch* b, Ch* e)
    {
        return s_.start_buffer(b, e);
    }

    void end_buffer(Ch* m)
    {
        s_.end_buffer(m);
    }

    void start_record(Ch* record_begin)
    {
        s_.start_record(record_begin);
    }

    void end_record(Ch* record_end)
    {
        s_.end_record(record_end);
    }

    void update(Ch* first, Ch* last)
    {
        if (s_.is_in_header()) {
            to_upper(first, last);
        }
        s_.update(first, last);
    }

    void finalize(Ch* first, Ch* last)
    {
        if (s_.is_in_header()) {
            to_upper(first, last);
        }
        s_.finalize(first, last);
    }

private:
    void to_upper(Ch* first, Ch* last) const;
};

template <>
void basic_table_scanner_wrapper<char>::
to_upper(char* first, char* last) const
{
    std::transform(first, last, first, [](auto c) {
        return static_cast<char>(
            std::toupper(static_cast<int>(c)));
    });
}

template <>
void basic_table_scanner_wrapper<wchar_t>::
to_upper(wchar_t* first, wchar_t* last) const
{
    std::transform(first, last, first, [](auto c) {
        return static_cast<wchar_t>(
            std::towupper(static_cast<std::wint_t>(c)));
    });
}

}

TYPED_TEST(TestTableScanner, IsInHeader)
{
    using string = std::basic_string<TypeParam>;

    const auto str = char_helper<TypeParam>::str;

    std::map<string, string> currency_map;
    std::pair<string, string> record;
    basic_table_scanner<TypeParam> scanner(
        [&str, &record](auto j, auto v, auto& t) {
            if (v) {
                const string field_name(v->first, v->second);
                if (field_name == str("COUNTRY")) {
                    t.set_field_scanner(j, make_field_translator<string>(
                        [&record](string&& s) {
                            record.first = std::move(s);
                        }));
                } else if (field_name == str("CURRENCY")) {
                    t.set_field_scanner(j, make_field_translator<string>(
                        [&record](string&& s) {
                            record.second = std::move(s);
                        }));
                }
                return true;
            } else {
                return false;
            }
        });
    scanner.set_record_end_scanner([&currency_map, &record] {
        currency_map.insert(std::move(record));
    });

    auto s = str("Country,Currency\r"
                 "Ukraine,Hryvnia\r"
                 "Estonia,Euro\r");
    parse_csv(s,
        basic_table_scanner_wrapper<TypeParam>(std::move(scanner)));

    ASSERT_EQ(2U, currency_map.size());
    ASSERT_EQ(str("Hryvnia"), currency_map[str("Ukraine")]);
    ASSERT_EQ(str("Euro"), currency_map[str("Estonia")]);
}

TYPED_TEST(TestTableScanner, BufferSize)
{
    using string_t = std::basic_string<TypeParam>;

    const auto str = char_helper<TypeParam>::str;

    std::vector<string_t> values0;
    std::vector<int> values1;

    for (std::size_t buffer_size : { 2U, 3U, 4U, 7U  }) {
        basic_table_scanner<TypeParam> h(0U);
        h.set_field_scanner(0, make_field_translator(values0));
        h.set_field_scanner(1, make_field_translator(values1));

        std::basic_stringbuf<TypeParam> buf;
        const auto line = str("ABC,123\n");
        for (std::size_t i = 0; i < 50; ++i) {
            buf.sputn(line.data(), line.size());
        }

        try {
            parse_csv(buf, std::move(h), buffer_size);
        } catch (const text_error& e) {
            FAIL() << text_error_info(e) << "\nbuffer_size=" << buffer_size;
        }

        ASSERT_EQ(50U, values0.size());
        ASSERT_EQ(50U, values1.size());
        for (std::size_t i = 0; i < 50; ++i) {
            ASSERT_EQ(str("ABC"), values0[i])
                << "buffer_size=" << buffer_size << " i=" << i;
            ASSERT_EQ(123, values1[i])
                << "buffer_size=" << buffer_size << " i=" << i;
        }
        values0.clear();
        values1.clear();
    }
}

TYPED_TEST(TestTableScanner, Allocators)
{
    using Alloc = tracking_allocator<std::allocator<TypeParam>>;
    using Tr = std::char_traits<TypeParam>;
    using String = std::basic_string<TypeParam, Tr, Alloc>;

    const auto str = char_helper<TypeParam>::str;

    std::vector<std::pair<char*, char*>> allocated0;
    std::size_t total0 = 0U;
    Alloc a0(allocated0, total0);

    std::vector<std::pair<char*, char*>> allocated2;
    std::size_t total2 = 0U;
    Alloc a2(allocated2, total2);

    basic_table_scanner<TypeParam, Tr, Alloc> scanner(
        std::allocator_arg, a0, 0U);

    // The same allocator as the scanner
    std::vector<String> v0;
    auto f0 = make_field_translator(
        std::allocator_arg, scanner.get_allocator(), v0);
    scanner.set_field_scanner(0, std::move(f0));

    // A different allocator from the scanner
    std::vector<std::basic_string<TypeParam>> v1;
    auto f1 = make_field_translator(v1);
    scanner.set_field_scanner(1, std::move(f1));

    // A different allocator from the scanner, but types are same
    std::vector<String> v2;
    auto f2 = make_field_translator(std::allocator_arg, a2, v2);
    scanner.set_field_scanner(2, std::move(f2));

    // Field scanners are stored into the memories allocated by a0
    ASSERT_TRUE(a0.tracks(
        scanner.template get_field_scanner<decltype(f0)>(0)));

    // A lengthy field is required to make sure String uses an allocator
    try {
        std::basic_string<TypeParam> s =
            str("ABCDEFGHIJKLMNOPQRSTUVWXYZ,"
                "abcdefghijklmnopqrstuvwxyz,"
                "12345678901234567890123456");
        parse_csv(s, std::move(scanner), 20);   // make underflow happen
    } catch (const text_error& e) {
        FAIL() << text_error_info(e);
    }

    ASSERT_EQ(a0, v0[0].get_allocator());
    ASSERT_TRUE(a0.tracks(&v0[0][0]));

    ASSERT_FALSE(a0.tracks(&v1[0][0]));
    ASSERT_FALSE(a0.tracks(&v2[0][0]));

    ASSERT_EQ(a2, v2[0].get_allocator());
    ASSERT_TRUE(a2.tracks(&v2[0][0]));
}

TYPED_TEST(TestTableScanner, MovedFromState)
{
    std::vector<int> values;

    basic_table_scanner<TypeParam> h1;
    auto t = make_field_translator(values);
    h1.set_field_scanner(0, std::move(t));

    basic_table_scanner<TypeParam> h2(std::move(h1));

    ASSERT_FALSE(h1.has_field_scanner(0));
    ASSERT_EQ(typeid(void), h1.get_field_scanner_type(0));
    ASSERT_EQ(nullptr, h1.template get_field_scanner<decltype(t)>(0));

    ASSERT_NE(nullptr, h2.template get_field_scanner<decltype(t)>(0));
}

namespace {

template <class Ch, class F>
class check_scanner
{
    F f_;

public:
    explicit check_scanner(F f) : f_(f) {}

    void operator()(Ch* b, Ch* e) { f_(b); f_(e - 1); }

    void operator()() {}
};

template <class Ch, class F>
check_scanner<Ch, F> make_check_scanner(F f)
{
    return check_scanner<Ch, F>(f);
}

}

namespace {

template <class T>
class calc_average
{
    T n_ = 0;
    T sum_ = T();

public:
    void operator()(T n)
    {
        ++n_;
        sum_ += n;
    }

    T yield() const
    {
        return sum_ / n_;
    }
};

}

TYPED_TEST(TestTableScanner, Ignored)
{
    replace_if_conversion_failed<int> r(
        replacement_ignore, replacement_ignore);
    calc_average<int> a;
    basic_table_scanner<TypeParam> scanner;
    scanner.set_field_scanner(0, make_field_translator<int>(
        std::ref(a), fail_if_skipped(), std::move(r)));

    const auto str = char_helper<TypeParam>::str;

    try {
        parse_csv(str("100\nn/a\n\n200"), std::move(scanner));
    } catch (const text_error& e) {
        FAIL() << text_error_info(e);
    }

    ASSERT_EQ(150, a.yield());
}

namespace {

struct stateful_header_scanner
{
    std::size_t index = 0;
    std::vector<int>* values = nullptr;

    template <class Ch, class T>
    bool operator()(std::size_t j, const std::pair<Ch*, Ch*>*, T& t) const
    {
        if (j == index) {
            t.set_field_scanner(j, make_field_translator(*values));
            return false;
        } else {
            return true;
        }
    }
};

}

struct TestTableScannerReference : BaseTest
{};

TEST_F(TestTableScannerReference, HeaderScanner)
{
    std::vector<int> values;
    stateful_header_scanner header_scanner;
    header_scanner.values = &values;
    table_scanner scanner(std::ref(header_scanner));

    header_scanner.index = 1;

    try {
        parse_csv("A,B\n100,200", std::move(scanner));
    } catch (const text_error& e) {
        FAIL() << text_error_info(e);
    }

    ASSERT_EQ(1U, values.size());
    ASSERT_EQ(200, values[0]);
}

TEST_F(TestTableScannerReference, FieldScanner)
{
    std::vector<int> values1;
    auto field_scanner = make_field_translator(values1);
    table_scanner scanner;
    scanner.set_field_scanner(0, std::ref(field_scanner));

    ASSERT_EQ(typeid(std::reference_wrapper<decltype(field_scanner)>),
        scanner.get_field_scanner_type(0));

    try {
        parse_csv("100", std::move(scanner));
    } catch (const text_error& e) {
        FAIL() << text_error_info(e);
    }

    ASSERT_EQ(1U, values1.size());
    ASSERT_EQ(100, values1[0]);
}

TEST_F(TestTableScannerReference, RecordEndScanner)
{
    std::vector<int> v;

    table_scanner scanner;
    scanner.set_field_scanner(0, make_field_translator(v));

    std::function<void()> record_end_scanner = [] {};
    scanner.set_record_end_scanner();                   // shall be nop
    scanner.set_record_end_scanner(record_end_scanner); // overwritten
    scanner.set_record_end_scanner(std::ref(record_end_scanner));

    record_end_scanner = [&v]() {
        v.push_back(-12345);
    };

    ASSERT_EQ(typeid(std::reference_wrapper<decltype(record_end_scanner)>),
        scanner.get_record_end_scanner_type());

    try {
        parse_csv("100\n200", std::move(scanner));
    } catch (const text_error& e) {
        FAIL() << text_error_info(e);
    }

    ASSERT_EQ(4U, v.size());
    ASSERT_EQ(   100, v[0]);
    ASSERT_EQ(-12345, v[1]);
    ASSERT_EQ(   200, v[2]);
    ASSERT_EQ(-12345, v[3]);
}

struct TestReplaceIfSkipped : BaseTest
{};

TEST_F(TestReplaceIfSkipped, ActionInstallmentWithCtors)
{
    // default ctor
    {
        replace_if_skipped<std::string> r;
        ASSERT_STREQ("", r()->c_str());
    }

    // copy
    {
        replace_if_skipped<std::string> r(3, 'A');
        ASSERT_STREQ("AAA", r()->c_str());
    }

    // ignore
    {
        replace_if_skipped<std::string> r(replacement_ignore);
        ASSERT_TRUE(!r());
    }

    // fail
    {
        replace_if_skipped<std::string> r(replacement_fail);
        ASSERT_THROW(r(), field_not_found);
    }
}

TEST_F(TestReplaceIfSkipped, CopyCtor)
{
    // copy
    {
        const replace_if_skipped<std::string> r0("XYZ");
        replace_if_skipped<std::string> r(r0);
        ASSERT_STREQ("XYZ", r()->c_str());
    }

    // ignore
    {
        const replace_if_skipped<std::string> r0(replacement_ignore);
        replace_if_skipped<std::string> r(r0);
        ASSERT_TRUE(!r());
    }

    // fail
    {
        const replace_if_skipped<std::string> r0(replacement_fail);
        replace_if_skipped<std::string> r(r0);
        ASSERT_THROW(r(), field_not_found);
    }
}

TEST_F(TestReplaceIfSkipped, MoveCtor)
{
    // copy
    {
        replace_if_skipped<std::string> r0("XYZ");
        replace_if_skipped<std::string> r(std::move(r0));
        ASSERT_STREQ("XYZ", r()->c_str());
    }

    // ignore
    {
        replace_if_skipped<std::string> r0(replacement_ignore);
        replace_if_skipped<std::string> r(std::move(r0));
        ASSERT_TRUE(!r());
    }

    // fail
    {
        replace_if_skipped<std::string> r0(replacement_fail);
        replace_if_skipped<std::string> r(std::move(r0));
        ASSERT_THROW(r(), field_not_found);
    }
}

TEST_F(TestReplaceIfSkipped, CopyAssign)
{
    using r_t = replace_if_skipped<std::vector<int>>;

    // from copy
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(std::vector<int>{ -1, -2, -3 });

        const std::vector<int> v = { 10, 20, 30 };
        r_t r0(v);

        rs[0] = r0;
        rs[1] = r0;
        rs[2] = r0;
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_EQ(v, *rs[i]()) << i;
        }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
        r0 = r0;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
        ASSERT_EQ(v, *r0());
    }

    // from ignore
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(std::vector<int>{ -1, -2, -3 });

        r_t r0(replacement_ignore);

        rs[0] = r0;
        rs[1] = r0;
        rs[2] = r0;
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_TRUE(!rs[i]()) << i;
        }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
        r0 = r0;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
        ASSERT_TRUE(!r0());
    }

    // from fail
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(std::vector<int>{ -1, -2, -3 });

        r_t r0(replacement_fail);

        rs[0] = r0;
        rs[1] = r0;
        rs[2] = r0;
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_THROW(rs[i](), field_not_found) << i;
        }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
        r0 = r0;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
        ASSERT_THROW(r0(), field_not_found);
    }
}

TEST_F(TestReplaceIfSkipped, MoveAssign)
{
    using r_t = replace_if_skipped<std::vector<int>>;

    // from copy
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(std::vector<int>{ -1, -2, -3 });

        const std::vector<int> v = { 10, 20, 30 };

        rs[0] = r_t(v);
        rs[1] = r_t(v);
        rs[2] = r_t(v);
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_EQ(v, *rs[i]()) << i;
        }

        rs[0] = std::move(rs[0]);
        ASSERT_EQ(v, *rs[0]());
    }

    // from ignore
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(std::vector<int>{ -1, -2, -3 });

        rs[0] = r_t(replacement_ignore);
        rs[1] = r_t(replacement_ignore);
        rs[2] = r_t(replacement_ignore);
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_TRUE(!rs[i]()) << i;
        }

        rs[0] = std::move(rs[0]);
        ASSERT_TRUE(!rs[0]());
    }

    // from fail
    {
        std::vector<r_t> rs;
        rs.emplace_back(replacement_ignore);
        rs.emplace_back(replacement_fail);
        rs.emplace_back(std::vector<int>{ -1, -2, -3 });

        rs[0] = r_t(replacement_fail);
        rs[1] = r_t(replacement_fail);
        rs[2] = r_t(replacement_fail);
        for (std::size_t i = 0, ie = rs.size(); i < ie; ++i) {
            ASSERT_THROW(rs[i](), field_not_found) << i;
        }

        rs[0] = std::move(rs[0]);
        ASSERT_THROW(rs[0](), field_not_found);
    }
}

TEST_F(TestReplaceIfSkipped, Swap)
{
    std::vector<replace_if_skipped<std::string>> rs;
    rs.emplace_back("ABC");
    rs.emplace_back(replacement_ignore);
    rs.emplace_back(replacement_fail);
    rs.emplace_back("xyz");

    using std::swap;

    // copy vs ignore
    swap(rs[0], rs[1]);
    ASSERT_TRUE(!rs[0]());
    ASSERT_STREQ("ABC", rs[1]()->c_str());
    swap(rs[0], rs[1]);
    ASSERT_STREQ("ABC", rs[0]()->c_str());
    ASSERT_TRUE(!rs[1]());

    // ignore vs fail
    swap(rs[1], rs[2]);
    ASSERT_TRUE(!rs[2]());
    ASSERT_THROW(rs[1](), field_not_found);
    swap(rs[1], rs[2]);
    ASSERT_TRUE(!rs[1]());
    ASSERT_THROW(rs[2](), field_not_found);

    // fail vs copy
    swap(rs[2], rs[3]);
    ASSERT_STREQ("xyz", rs[2]()->c_str());
    ASSERT_THROW(rs[3](), field_not_found);
    swap(rs[2], rs[3]);
    ASSERT_STREQ("xyz", rs[3]()->c_str());
    ASSERT_THROW(rs[2](), field_not_found);

    // copy vs copy
    swap(rs[3], rs[0]);
    ASSERT_STREQ("ABC", rs[3]()->c_str());
    ASSERT_STREQ("xyz", rs[0]()->c_str());
    swap(rs[3], rs[0]);
    ASSERT_STREQ("xyz", rs[3]()->c_str());
    ASSERT_STREQ("ABC", rs[0]()->c_str());

    // swap with self
    swap(rs[0], rs[0]);
    ASSERT_STREQ("ABC", rs[0]()->c_str());
    swap(rs[1], rs[1]);
    ASSERT_TRUE(!rs[1]());
    swap(rs[2], rs[2]);
    ASSERT_THROW(rs[2](), field_not_found);
}

TEST_F(TestReplaceIfSkipped, DeductionGuides)
{
    replace_if_skipped r1(10);
    static_assert(std::is_same_v<replace_if_skipped<int>, decltype(r1)>);
    ASSERT_TRUE(r1().has_value());
    ASSERT_EQ(10, *r1());

    const std::string s("skipped");
    replace_if_skipped r2(s);
    static_assert(
        std::is_same_v<replace_if_skipped<std::string>, decltype(r2)>);
    ASSERT_TRUE(r2().has_value());
    ASSERT_STREQ("skipped", r2()->c_str());
}

namespace replace_if_skipped_static_asserts {

using ri_t = replace_if_skipped<int>;
using rv_t = replace_if_skipped<std::vector<int>>;

static_assert(std::is_nothrow_copy_constructible_v<ri_t>);
static_assert(std::is_nothrow_move_constructible_v<ri_t>);
static_assert(std::is_nothrow_copy_assignable_v<ri_t>);
static_assert(std::is_nothrow_move_assignable_v<ri_t>);
static_assert(std::is_nothrow_swappable_v<ri_t>);

static_assert(!std::is_nothrow_copy_constructible_v<rv_t>);
static_assert(std::is_nothrow_move_constructible_v<rv_t>);
static_assert(!std::is_nothrow_copy_assignable_v<rv_t>);
static_assert(std::is_nothrow_move_assignable_v<rv_t>);
static_assert(std::is_nothrow_swappable_v<rv_t>);

static_assert(std::is_trivially_copyable<ri_t>::value);

}
