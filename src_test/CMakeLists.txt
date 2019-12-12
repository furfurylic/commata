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

set(COMMATA_HEADERS
    ../include/commata/allocation_only_allocator.hpp
    ../include/commata/empty_physical_line_aware_handler.hpp
    ../include/commata/formatted_output.hpp
    ../include/commata/handler_decorator.hpp
    ../include/commata/key_chars.hpp
    ../include/commata/member_like_base.hpp
    ../include/commata/parse_csv.hpp
    ../include/commata/parse_error.hpp
    ../include/commata/propagation_controlled_allocator.hpp
    ../include/commata/record_extractor.hpp
    ../include/commata/stored_table.hpp
    ../include/commata/table_scanner.hpp
    ../include/commata/text_error.hpp
    ../include/commata/typing_aid.hpp
)

set(TEST_COMMATA_SOURCES
    TestParseCsv.cpp
    TestRecordExtractor.cpp
    TestStoredTable.cpp
    TestTableScanner.cpp
    TestTextError.cpp
)

set(TEST_COMMATA_HEADERS
    fancy_allocator.hpp
    identified_allocator.hpp
    tracking_allocator.hpp
    BaseTest.hpp
)

add_executable(test_commata
    ${COMMATA_HEADERS}
    ${TEST_COMMATA_SOURCES}
    ${TEST_COMMATA_HEADERS}
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
source_group("Commata Headers" FILES ${COMMATA_HEADERS})

if(MSVC)
    target_compile_options(test_commata PUBLIC /MP /W4 /bigobj)
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
