/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_324DC0E6_2E5C_4359_8AE3_8CFF2626E941
#define FURFURYLIC_324DC0E6_2E5C_4359_8AE3_8CFF2626E941

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

}}

#endif
