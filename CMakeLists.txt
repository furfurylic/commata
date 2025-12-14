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
    include/commata/char_input.hpp
    include/commata/field_handling.hpp
    include/commata/field_scanners.hpp
    include/commata/parse_csv.hpp
    include/commata/parse_error.hpp
    include/commata/parse_result.hpp
    include/commata/parse_tsv.hpp
    include/commata/record_extractor.hpp
    include/commata/stored_table.hpp
    include/commata/table_pull.hpp
    include/commata/table_scanner.hpp
    include/commata/text_error.hpp
    include/commata/text_value_translation.hpp
    include/commata/wrapper_handlers.hpp
    include/commata/detail/allocate_deallocate.hpp
    include/commata/detail/allocation_only_allocator.hpp
    include/commata/detail/base_parser.hpp
    include/commata/detail/base_source.hpp
    include/commata/detail/buffer_control.hpp
    include/commata/detail/buffer_size.hpp
    include/commata/detail/formatted_output.hpp
    include/commata/detail/full_ebo.hpp
    include/commata/detail/handler_decorator.hpp
    include/commata/detail/key_chars.hpp
    include/commata/detail/member_like_base.hpp
    include/commata/detail/propagation_controlled_allocator.hpp
    include/commata/detail/string_pred.hpp
    include/commata/detail/string_value.hpp
    include/commata/detail/typing_aid.hpp
    include/commata/detail/write_ntmbs.hpp
)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_sources(commata INTERFACE
        include/commata/commata.natvis
    )
endif()
target_compile_features(commata INTERFACE cxx_std_17)

if(COMMATA_BUILD_TESTS)
    add_subdirectory(src_test)
endif()
