/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_324DC0E6_2E5C_4359_8AE3_8CFF2626E941
#define COMMATA_GUARD_324DC0E6_2E5C_4359_8AE3_8CFF2626E941

#include <locale>
#include <sstream>
#include <string>
#include <utility>

#if defined(_MSC_VER) && defined(_DEBUG)
#define COMMATA_TEST_MEMORY_LEAK_CHECK
#include <crtdbg.h>
#endif

#include <gtest/gtest.h>

namespace commata::test {

class MemoryLeakCheck
{
#ifdef COMMATA_TEST_MEMORY_LEAK_CHECK
    _CrtMemState state1;
#endif

public:
    MemoryLeakCheck()
#ifdef COMMATA_TEST_MEMORY_LEAK_CHECK
      : state1()
#endif
    {}

    void Init()
    {
#ifdef COMMATA_TEST_MEMORY_LEAK_CHECK
        _CrtMemCheckpoint(&state1);
#endif
    }

    void Check()
    {
#ifdef COMMATA_TEST_MEMORY_LEAK_CHECK
        if (!testing::Test::HasFatalFailure()) {
            _CrtMemState state2;
            _CrtMemCheckpoint(&state2);
            _CrtMemState state3;
            if (_CrtMemDifference(&state3, &state1, &state2)) {
                const auto oldReportMode = _CrtSetReportMode(
                    _CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
                const auto oldReportFile = _CrtSetReportFile(
                    _CRT_WARN, _CRTDBG_FILE_STDERR);
                _CrtMemDumpStatistics(&state3);
                _CrtMemDumpAllObjectsSince(&state1);
                _CrtSetReportMode(_CRT_WARN, oldReportMode);
                _CrtSetReportFile(_CRT_WARN, oldReportFile);
                ADD_FAILURE() << "Memory leak detected";
            }
        }
#endif
    }
};

class BaseTest : public testing::Test
{
    MemoryLeakCheck check_;

public:
    void SetUp() override
    {
        check_.Init();
    }

    void TearDown() override
    {
        check_.Check();
    }
};

template <class T>
class BaseTestWithParam : public testing::TestWithParam<T>
{
    MemoryLeakCheck check_;

public:
    void SetUp() override
    {
        check_.Init();
    }

    void TearDown() override
    {
        check_.Check();
    }
};

template <class Ch>
class char_helper
{
public:
    static Ch ch(char c)
    {
        return ctype().widen(c);
    }

    static std::basic_string<Ch> str(const char* s)
    {
        std::basic_string<Ch> r;
        while (*s != char()) {
            r.push_back(ch(*s));
            ++s;
        }
        return r;
    }

    static std::basic_string<Ch> str0(const char* s)
    {
        auto r = str(s);
        r.push_back(Ch());
        return r;
    }

    static std::basic_string<Ch> strv(std::string_view s)
    {
        std::basic_string<Ch> r;
        std::transform(s.cbegin(), s.cend(), std::back_inserter(r), ch);
        return r;
    }

    template <class T>
    static std::basic_string<Ch> to_string(T value)
    {
        std::basic_ostringstream<Ch> s;
        s << value;
        return std::move(s).str();
    }

    template <class... Args>
    static std::basic_string<Ch> widen(Args&&... args)
    {
        const std::string s(std::forward<Args>(args)...);
        std::basic_string<Ch> ws(s.size(), 'x');
        ctype().widen(s.data(), s.data() + s.size(), ws.data());
        return ws;
    }

private:
    static const std::ctype<Ch>& ctype()
    {
        static const std::ctype<Ch>& static_ctype =
            std::use_facet<std::ctype<Ch>>(std::locale::classic());
        return static_ctype;
    }
};

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

#endif
