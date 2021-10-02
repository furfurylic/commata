cmake_minimum_required(VERSION 3.11)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(test_commata)
enable_testing()

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.11.0
)
FetchContent_GetProperties(googletest)
# We don't use FetchContent_MakeAvailable to take advantage of EXCLUDE_FROM_ALL
# because we don't need googlemock built with our "All" target
if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory(${googletest_SOURCE_DIR}
                     ${googletest_BINARY_DIR}
                     EXCLUDE_FROM_ALL)
endif()
set_target_properties(gtest gtest_main PROPERTIES FOLDER Depenencies)

add_executable(test_commata)

set(COMMATA_HEADERS
    ../include/commata/allocation_only_allocator.hpp
    ../include/commata/buffer_control.hpp
    ../include/commata/buffer_size.hpp
    ../include/commata/char_input.hpp
    ../include/commata/empty_physical_line_aware_handler.hpp
    ../include/commata/empty_string.hpp
    ../include/commata/formatted_output.hpp
    ../include/commata/handler_decorator.hpp
    ../include/commata/key_chars.hpp
    ../include/commata/member_like_base.hpp
    ../include/commata/nothrow_move_constructible.hpp
    ../include/commata/parse_csv.hpp
    ../include/commata/parse_error.hpp
    ../include/commata/propagation_controlled_allocator.hpp
    ../include/commata/record_extractor.hpp
    ../include/commata/stored_table.hpp
    ../include/commata/string_value.hpp
    ../include/commata/table_pull.hpp
    ../include/commata/table_scanner.hpp
    ../include/commata/text_error.hpp
    ../include/commata/typing_aid.hpp
    ../include/commata/write_ntmbs.hpp
)

set(TEST_COMMATA_SOURCES
    TestCharInput.cpp
    TestParseCsv.cpp
    TestRecordExtractor.cpp
    TestStoredTable.cpp
    TestTablePull.cpp
    TestTableScanner.cpp
    TestTextError.cpp
)

set(TEST_COMMATA_HEADERS
    fancy_allocator.hpp
    identified_allocator.hpp
    tracking_allocator.hpp
    BaseTest.hpp
)

target_sources(test_commata PRIVATE
    ${COMMATA_HEADERS}
    ${TEST_COMMATA_SOURCES}
    ${TEST_COMMATA_HEADERS}
)

source_group("Commata Headers" FILES ${COMMATA_HEADERS})

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(test_commata PRIVATE
        /MP /W4 /bigobj
        $<$<CONFIG:MinSizeRel>:/wd4702>
        $<$<CONFIG:Release>:/wd4702>
        $<$<CONFIG:RelWithDebInfo>:/wd4702>
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(test_commata PRIVATE
        $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
    )
    target_compile_options(test_commata PRIVATE
        -Wall -Wextra -pedantic-errors -Werror=pedantic
        $<$<CONFIG:Debug>:-O0 -g3>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g3>
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_definitions(test_commata PRIVATE
        $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
    )
    target_compile_options(test_commata PRIVATE
        -Wall -Wextra -pedantic-errors -Werror=pedantic
        -Wno-gnu-zero-variadic-macro-arguments
        $<$<CONFIG:Debug>:-O0 -g3>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g3>
    )
endif()

target_include_directories(test_commata PRIVATE ../include)

target_link_libraries(test_commata gtest gtest_main)

add_test(
    NAME test_commata
    COMMAND $<TARGET_FILE:test_commata>
)
