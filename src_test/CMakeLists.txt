cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

enable_testing()

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
set_target_properties(gtest gtest_main PROPERTIES FOLDER "Dependencies")

add_executable(test_commata)

get_target_property(COMMATA_HEADERS commata INTERFACE_SOURCES)

set(TEST_COMMATA_SOURCES
    TestCharInput.cpp
    TestParseCsv.cpp
    TestParseTsv.cpp
    TestRecordExtractor.cpp
    TestRecordTranslator.cpp
    TestReplaceIfConversionFailed.cpp
    TestReplaceIfSkipped.cpp
    TestStoredTable.cpp
    TestTablePull.cpp
    TestTableScanner.cpp
    TestTextError.cpp
    TestTextValueTranslation.cpp
    TestWriteNTMBS.cpp
)

set(TEST_COMMATA_HEADERS
    fancy_allocator.hpp
    identified_allocator.hpp
    logging_allocator.hpp
    simple_transcriptor.hpp
    tracking_allocator.hpp
    trivials.hpp
    BaseTest.hpp
)

target_sources(test_commata PRIVATE
    ${COMMATA_HEADERS}
    ${TEST_COMMATA_SOURCES}
    ${TEST_COMMATA_HEADERS}
)

list(GET COMMATA_HEADERS 0 FIRST_COMMATA_HEADER)
get_filename_component(COMMATA_HEADERS_ROOT ${FIRST_COMMATA_HEADER} DIRECTORY)
source_group(TREE ${COMMATA_HEADERS_ROOT}
             PREFIX "Commata Headers"
             FILES ${COMMATA_HEADERS})

target_compile_features(test_commata PRIVATE cxx_std_17)

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
        -Wno-trigraphs
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
        -Wno-trigraphs
        $<$<CONFIG:Debug>:-O0 -g3>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g3>
    )
endif()

target_link_libraries(test_commata PRIVATE commata gtest gtest_main)

add_test(
    NAME test_commata
    COMMAND $<TARGET_FILE:test_commata>
)
