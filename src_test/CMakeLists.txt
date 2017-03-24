cmake_minimum_required(VERSION 2.8)
set(EXECUTABLE_OUTPUT_PATH ..)
add_definitions("-Wall -std=c++11")
include_directories(
    ../external/googletest/include
    ../include
    )
add_executable(test_commata
    TestPrimitiveParser.cpp
    TestRecordExtractor.cpp
    )
find_library(gtest NAMES libgtest.a PATHS ../external/googletest/build)
find_library(gtest_main NAMES libgtest_main.a PATHS ../external/googletest/build)
target_link_libraries(test_commata
    pthread
    ${gtest}
    ${gtest_main}
    )
