cmake_minimum_required(VERSION 3.0.2)
option(commata_disable_pthreads "Disable uses of pthreads." OFF)
set(EXECUTABLE_OUTPUT_PATH ..)
if(MSVC)
    add_definitions("/W4 /bigobj")
else(MSVC)
    add_definitions("-Wall -std=c++14 -g3")
endif(MSVC)
find_package(GTest REQUIRED)
include_directories(
    ${GTEST_INCLUDE_DIRS}
    ../include
)
add_executable(test_commata
    TestCsvError.cpp
    TestPrimitiveParser.cpp
    TestRecordExtractor.cpp
    TestCsvTable.cpp
    TestCsvScanner.cpp
)
if(commata_disable_pthreads)
    target_link_libraries(test_commata
        ${GTEST_BOTH_LIBRARIES}
    )
else(commata_disable_pthreads)
    target_link_libraries(test_commata
        pthread
        ${GTEST_BOTH_LIBRARIES}
    )
endif(commata_disable_pthreads)
add_test(
    NAME test_commata
    COMMAND $<TARGET_FILE:test_commata>
)
