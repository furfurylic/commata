cmake_minimum_required(VERSION 3.13)

option(COMMATA_BUILD_TESTS "Whether to build Commata's test" OFF)

project(commata CXX)

add_library(commata INTERFACE)
target_include_directories(commata INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
cmake_policy(SET CMP0076 NEW)
target_sources(commata INTERFACE
    include/commata/allocation_only_allocator.hpp
    include/commata/buffer_control.hpp
    include/commata/buffer_size.hpp
    include/commata/char_input.hpp
    include/commata/empty_string.hpp
    include/commata/formatted_output.hpp
    include/commata/handler_decorator.hpp
    include/commata/key_chars.hpp
    include/commata/member_like_base.hpp
    include/commata/parse_csv.hpp
    include/commata/parse_error.hpp
    include/commata/propagation_controlled_allocator.hpp
    include/commata/record_extractor.hpp
    include/commata/stored_table.hpp
    include/commata/string_value.hpp
    include/commata/table_pull.hpp
    include/commata/table_scanner.hpp
    include/commata/text_error.hpp
    include/commata/typing_aid.hpp
    include/commata/wrapper_handlers.hpp
    include/commata/write_ntmbs.hpp
)
target_compile_features(commata INTERFACE cxx_std_17)

if(COMMATA_BUILD_TESTS)
    add_subdirectory(src_test)
endif()
