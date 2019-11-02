cmake_minimum_required(VERSION 3.14)

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.10.0
)
FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory(${googletest_SOURCE_DIR}
                     ${googletest_BINARY_DIR}
                     EXCLUDE_FROM_ALL)
endif()

set(CMAKE_CXX_STANDARD 14)

set(EXECUTABLE_OUTPUT_PATH ..)

add_executable(test_commata
    TestParseCsv.cpp
    TestRecordExtractor.cpp
    TestStoredTable.cpp
    TestTableScanner.cpp
    TestTextError.cpp
)

if(MSVC)
    target_compile_options(test_commata PUBLIC /W4 /bigobj)
    target_compile_options(test_commata PUBLIC
        $<$<CONFIG:MinSizeRel>:/wd4702>
        $<$<CONFIG:Release>:/wd4702>
        $<$<CONFIG:RelWithDebInfo>:/wd4702>
    )
else(MSVC)
    target_compile_definitions(test_commata PUBLIC
        $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
    )
    target_compile_options(test_commata PUBLIC
        $<$<CONFIG:Debug>:-O0 -g3>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g3>
    )
endif(MSVC)

include_directories(../include)

target_link_libraries(test_commata gtest gtest_main)

add_test(
    NAME test_commata
    COMMAND $<TARGET_FILE:test_commata>
)
