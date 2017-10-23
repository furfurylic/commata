/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_324DC0E6_2E5C_4359_8AE3_8CFF2626E941
#define FURFURYLIC_324DC0E6_2E5C_4359_8AE3_8CFF2626E941

#include <locale>
#include <sstream>
#include <string>

#if defined(_MSC_VER) && defined(_DEBUG)
#define FURFURYLIC_TEST_MEMORY_LEAK_CHECK
#include <crtdbg.h>
#endif

#include <gtest/gtest.h>

namespace furfurylic { namespace test {

class MemoryLeakCheck
{
#ifdef FURFURYLIC_TEST_MEMORY_LEAK_CHECK
    _CrtMemState state1;
#endif

public:
    void init()
    {
#ifdef FURFURYLIC_TEST_MEMORY_LEAK_CHECK
        _CrtMemCheckpoint(&state1);
#endif
    }

    void check()
    {
#ifdef FURFURYLIC_TEST_MEMORY_LEAK_CHECK
        if (!testing::Test::HasFatalFailure()) {
            _CrtMemState state2;
            _CrtMemCheckpoint(&state2);
            _CrtMemState state3;
            if (_CrtMemDifference(&state3, &state1, &state2)) {
                const auto oldReportMode = _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
                const auto oldReportFile = _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
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
        check_.init();
    }

    void TearDown() override
    {
        check_.check();
    }
};

template <class T>
class BaseTestWithParam : public testing::TestWithParam<T>
{
    MemoryLeakCheck check_;

public:
    void SetUp() override
    {
        check_.init();
    }

    void TearDown() override
    {
        check_.check();
    }
};

template <class Ch>
class char_helper
{
    static const std::ctype<Ch>& facet_;

public:
    static Ch ch(char c)
    {
        return facet_.widen(c);
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

    template <class T>
    static std::basic_string<Ch> to_string(T value)
    {
        std::basic_stringstream<Ch> s;
        s << value;
        return s.str();
    }

    template <class... Args>
    static std::basic_string<Ch> widen(Args&&... args)
    {
        const std::string s(std::forward<Args>(args)...);
        std::basic_string<Ch> ws(s.size(), 'x');
        facet_.widen(s.c_str(), s.c_str() + s.size(), &ws[0]);
        return ws;
    }
};

template <class Ch>
const std::ctype<Ch>& char_helper<Ch>::facet_ =
    std::use_facet<std::ctype<Ch>>(std::locale::classic());

}}

#endif
