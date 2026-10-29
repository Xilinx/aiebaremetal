# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.10)

project(testcpputest LANGUAGES C CXX ASM)

option(ENABLE_COVERAGE "Enable coverage reporting with gcov/lcov" ON)
option(ENABLE_STACK_USAGE "Enable stack usage analysis" ON)
option(ENABLE_STACK_PATH_IPA_DUMP "GCC only: emit IPA call-graph dumps under build/gcc-cgraph for worst_stack_path.py" OFF)

add_compile_definitions(AEG_DEBUG)
add_definitions(-DAIE_GEN=5)

include(ExternalProject)
if(ENABLE_COVERAGE)
    message(STATUS "Coverage enabled: Adding coverage flags.")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage -O0 -g)
        add_link_options(--coverage)
    endif()
endif()

if(ENABLE_STACK_USAGE)
    message(STATUS "Stack usage analysis enabled: Adding -fstack-usage flag.")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fstack-usage)
    endif()
endif()

ExternalProject_Add(
    CppUTest
    GIT_REPOSITORY https://github.com/cpputest/cpputest.git
    GIT_TAG master
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/cpputest
    UPDATE_DISCONNECTED 1
)

ExternalProject_Get_Property(CppUTest source_dir)
ExternalProject_Get_Property(CppUTest binary_dir)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BAREMETAL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../src)
get_filename_component(BAREMETAL_SOURCE_DIR_ABS "${BAREMETAL_SOURCE_DIR}" REALPATH)
get_filename_component(CERT_HOST_CC_ABS "${BAREMETAL_SOURCE_DIR_ABS}/cert_host.cc" REALPATH)
set(THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/aielib/include)
execute_process(COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/script_files/setup_includes.sh)

include_directories(
    ${source_dir}/include
    ${BAREMETAL_SOURCE_DIR}/include
    ${BAREMETAL_SOURCE_DIR}/include/common_layer
    ${BAREMETAL_SOURCE_DIR}/include/common_layer/aegothers
    ${THIRDPARTY_DIR}
    ${BAREMETAL_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
link_directories(${binary_dir}/src/CppUTest)

set(CPPUTEST_LINK_LIBS
    ${binary_dir}/src/CppUTest/libCppUTest.a
    ${binary_dir}/src/CppUTestExt/libCppUTestExt.a
    dl
)

add_library(mock_aie_library SHARED stubs/mock_aie_library.cpp)
set_target_properties(mock_aie_library PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    PREFIX ""
    SUFFIX ".so"
)
add_library(mock_aie_library_no_symbol SHARED stubs/mock_aie_library_no_symbol.cpp)
set_target_properties(mock_aie_library_no_symbol PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    PREFIX ""
    SUFFIX ".so"
)
# Which executables link / depend on these: see "Reference: executables" block below.

# ==============================================================================
# Reference: executables, source files, and CMake dependencies
# ==============================================================================
#
# Path roots:
#   ${BAREMETAL_SOURCE_DIR}     -> ../../src (production baremetal sources)
#   ${CMAKE_CURRENT_SOURCE_DIR} -> this testcpputest directory (stubs/, tests/, …)
#   ${source_dir}               -> CppUTest ExternalProject source tree
#
# Shared libraries (link line):
#   ${CPPUTEST_LINK_LIBS} -> libCppUTest.a, libCppUTestExt.a, dl
#
# Typical add_dependencies (per target — see each target below):
#   CppUTest                         — ExternalProject; must build before linking.
#   mock_aie_library                 — SHARED ${CMAKE_BINARY_DIR}/mock_aie_library.so
#   mock_aie_library_no_symbol       — SHARED ${CMAKE_BINARY_DIR}/mock_aie_library_no_symbol.so
#
# -----------------------------------------------------------------------------
# utests
# -----------------------------------------------------------------------------
# Sources:
#   ${UT_AIEGRAPH_STUB_SOURCES}
#       stubs/main.cpp, stubs/app_elf_stub.S, stubs/aiert_mock.c, stubs/other_mock.cpp,
#       stubs/baremetal_metadata.cpp, stubs/test_comparator.cpp, stubs/graph_api_ut_stub.cpp,
#       stubs/stubs.cc
#   tests/test_aiebaremetal.cc
#   ${BAREMETAL_SOURCES_UTESTS_AIEGRAPH}  (= ${BAREMETAL_RUNTIME_UTESTS_AIEGRAPH} + stub_cert_elf_loader)
#       ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_api_message.cpp
#       ${BAREMETAL_SOURCE_DIR}/aeg_aiebaremetal.cc
#       stubs/gmio_api_mock.cpp
#       stubs/xaie_generate_events_mock.cpp
#       stubs/aeg_configure_ut_stub.cpp
#       stubs/cert_host_ut_stub.cpp
#       stubs/submit_cmd_stubs/stub_cert_elf_loader.cpp
# CMake: target_link_libraries(utests ${CPPUTEST_LINK_LIBS})
#        add_dependencies(utests CppUTest mock_aie_library mock_aie_library_no_symbol)
# Extra: target_include_directories stubs/fake_elfio (BEFORE), stubs/submit_cmd_stubs;
#        COMPILE_OPTIONS -include stubs/xil_exception_ut_override.h on aeg_aiebaremetal.cc
#
# -----------------------------------------------------------------------------
# utests_components
# -----------------------------------------------------------------------------
# Sources:
#   ${UT_COMPONENT_STUB_SOURCES}  — same as UT_AIEGRAPH_STUB_SOURCES but NO graph_api_ut_stub.cpp
#   ${UT_COMPONENT_TEST_SOURCES}
#       tests/test_graph_api_run.cpp, tests/test_xaiemem.cpp, tests/test_common.cpp,
#       tests/test_barrier.cpp, tests/test_aeg_api_message.cpp, tests/test_generate_event.cpp
#   ${BAREMETAL_RUNTIME_UTESTS_COMPONENTS_SUITE}
#       ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_api_message.cpp
#       ${BAREMETAL_SOURCE_DIR}/aeg_aiebaremetal.cc
#       ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_runtime_api.cpp   (production graph_api)
#       ${BAREMETAL_SOURCE_DIR}/xaie_generate_events.cc
#       stubs/aeg_configure_ut_stub.cpp, stubs/cert_host_ut_stub.cpp,
#       stubs/submit_cmd_stubs/stub_cert_elf_loader.cpp
# CMake: target_link_libraries(utests_components ${CPPUTEST_LINK_LIBS})
#        add_dependencies(utests_components CppUTest mock_aie_library mock_aie_library_no_symbol)
# Extra: same fake_elfio / submit_cmd_stubs / xil_exception_ut_override.h pattern as utests
#
# -----------------------------------------------------------------------------
# utests_cert_elf
# -----------------------------------------------------------------------------
# Sources (${UT_CERT_ELF_LOADER_SOURCES}):
#   stubs/main.cpp, stubs/test_comparator.cpp, tests/test_cert_elf_loader.cpp,
#   ${BAREMETAL_SOURCE_DIR}/cert_elf_loader.cc
# CMake: target_link_libraries(utests_cert_elf ${CPPUTEST_LINK_LIBS})
#        add_dependencies(utests_cert_elf CppUTest)
# Extra: target_include_directories stubs/fake_elfio (BEFORE); __BAREMETAL_UTESTS__
#
# -----------------------------------------------------------------------------
# utests_cert_host
# -----------------------------------------------------------------------------
# Sources (${UT_CERT_HOST_SOURCES}):
#   stubs/main.cpp, stubs/app_elf_stub.S, stubs/aiert_mock.c, stubs/other_mock.cpp,
#   stubs/baremetal_metadata.cpp, stubs/test_comparator.cpp, stubs/stubs.cc,
#   tests/test_cert_host.cc, stubs/submit_cmd_stubs/stub_cert_elf_loader.cpp
#   + ${BAREMETAL_SOURCE_DIR}/cert_host.cc   (only production TU from ../../src for this target)
# CMake: target_link_libraries(utests_cert_host ${CPPUTEST_LINK_LIBS})
#        add_dependencies(utests_cert_host CppUTest mock_aie_library mock_aie_library_no_symbol)
# Extra: stubs/fake_elfio, stubs/submit_cmd_stubs; __UTEST_ISR_HW_PATH_BUILD__ __BAREMETAL_UTESTS__
# Note: No graph_api_ut_stub — tests do not use abr::graph_api. The only production translation
#       unit pulled from ../../src for this executable is cert_host.cc.
#
# -----------------------------------------------------------------------------
# utests_gmioapiimpl
# -----------------------------------------------------------------------------
# Sources: tests/test_gmioapiimpl.cpp, tests/test_externalbufferapiimpl.cpp
#   + ${UT_GMIOAPIIMPL_STUB_SOURCES} (main, app_elf_stub.S, aiert_mock, other_mock, baremetal_metadata,
#     test_comparator, stubs.cc — no graph_api_ut_stub, no aeg_aiebaremetal)
#   + ${UT_GMIOAPIIMPL_RUNTIME_SOURCES}
#       ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_api_message.cpp, stubs/gmio_api_mock.cpp
# CMake: target_link_libraries(utests_gmioapiimpl ${CPPUTEST_LINK_LIBS})
#        add_dependencies(utests_gmioapiimpl CppUTest mock_aie_library mock_aie_library_no_symbol)
#
# -----------------------------------------------------------------------------
# utests_aeg_configure
# -----------------------------------------------------------------------------
# Sources (${UT_AEG_CONFIGURE_SOURCES}):
#   tests/test_aeg_configure.cc, ${BAREMETAL_SOURCE_DIR}/aeg_configure.cc,
#   ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_api_message.cpp, stubs/gmio_api_mock.cpp,
#   stubs/graph_api_ut_stub.cpp, ${UT_GMIOAPIIMPL_STUB_SOURCES}
# CMake: target_link_libraries(utests_aeg_configure ${CPPUTEST_LINK_LIBS})
#        add_dependencies(utests_aeg_configure CppUTest mock_aie_library mock_aie_library_no_symbol)
#
# -----------------------------------------------------------------------------
# test_fal_util
# -----------------------------------------------------------------------------
# Sources: stubs/main.cpp, tests/test_fal_util.cpp, stubs/stubs.cc,
#   ${BAREMETAL_SOURCE_DIR}/common_layer/aegothers/fal_util.cpp
# CMake: target_link_libraries(test_fal_util ${CPPUTEST_LINK_LIBS})
#        add_dependencies(test_fal_util CppUTest)
# Extra: __BAREMETAL_UTESTS__ AIE_GEN=5; extra include dirs for xaiefal / aegothers
#
# -----------------------------------------------------------------------------
# ut_profiling
# -----------------------------------------------------------------------------
# Sources: stubs/main.cpp, stubs/profiling_stubs/stubs.cpp, stubs/profiling_stubs/fal_stubs.cpp,
#   tests/test_aeg_profiling_api.cpp, ${BAREMETAL_SOURCE_DIR}/common_layer/aegothers/aeg_profiling_api.cpp
# CMake: target_link_libraries(ut_profiling ${CPPUTEST_LINK_LIBS})
#        add_dependencies(ut_profiling CppUTest)
# Extra: __BAREMETAL_UTESTS__ AIE_GEN=5 __AIESIM__; profiling_stubs include layout
#
# -----------------------------------------------------------------------------
# test_submit_command  (from add_cert_host_mock_elf_loader_executable)
# -----------------------------------------------------------------------------
# Sources (${SUBMIT_CMD_TEST_SOURCES} + ${SUBMIT_CMD_UUT_SOURCES}):
#   stubs/main.cpp, tests/test_submit_command.cpp,
#   stubs/submit_cmd_stubs/stub_xaie_mem.c, stub_barrier.c, stub_cert_elf_loader.cpp,
#   cert_elf_globals.c, xaiengine.c
#   + ${BAREMETAL_SOURCE_DIR}/cert_host.cc
# CMake: target_link_libraries(test_submit_command ${CPPUTEST_LINK_LIBS})
#        add_dependencies(test_submit_command CppUTest)
# Extra: __BAREMETAL_UTESTS__ AIE_GEN=5; includes stubs/submit_cmd_stubs, aielib, baremetal headers
#
# ==============================================================================
# Source-set "ladder" (only used to build lists above — not targets by themselves)
# ==============================================================================
#
#   BAREMETAL_RUNTIME_SOURCES_COMMON_BASE
#       ../../src/common_layer/aeg_api_message.cpp, ../../src/aeg_aiebaremetal.cc,
#       stubs/gmio_api_mock.cpp, stubs/xaie_generate_events_mock.cpp
#       Used by: BAREMETAL_RUNTIME_UTESTS_AIEGRAPH (utests) and as the prefix of that list only.
#
#   BAREMETAL_RUNTIME_UTESTS_AIEGRAPH (= BASE + aeg_configure_ut_stub + cert_host_ut_stub)
#   BAREMETAL_SOURCES_UTESTS_AIEGRAPH (= AIEGRAPH + stubs/submit_cmd_stubs/stub_cert_elf_loader.cpp)
#
# BAREMETAL_RUNTIME_UTESTS_COMPONENTS_SUITE is a separate explicit list for utests_components
# (includes aeg_runtime_api.cpp + xaie_generate_events.cc; not derived from BASE).
#
# ==============================================================================

# Minimal baremetal + UT mocks (shared prefix for utests / BAREMETAL_RUNTIME_UTESTS_AIEGRAPH).
set(BAREMETAL_RUNTIME_SOURCES_COMMON_BASE
    ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_api_message.cpp
    ${BAREMETAL_SOURCE_DIR}/aeg_aiebaremetal.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/gmio_api_mock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/xaie_generate_events_mock.cpp
)

# Stubs replace configure + cert_host for the main utests AIEGraph binary.
set(BAREMETAL_RUNTIME_UTESTS_AIEGRAPH
    ${BAREMETAL_RUNTIME_SOURCES_COMMON_BASE}
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/aeg_configure_ut_stub.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/cert_host_ut_stub.cpp
)
set(BAREMETAL_SOURCES_UTESTS_AIEGRAPH
    ${BAREMETAL_RUNTIME_UTESTS_AIEGRAPH}
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/submit_cmd_stubs/stub_cert_elf_loader.cpp
)

# Shared stub objects for utests + utests_components (AIEGraph integration tests).
set(UT_AIEGRAPH_STUB_SOURCES
    stubs/main.cpp
    stubs/app_elf_stub.S
    stubs/aiert_mock.c
    stubs/other_mock.cpp
    stubs/baremetal_metadata.cpp
    stubs/test_comparator.cpp
    stubs/graph_api_ut_stub.cpp
    stubs/stubs.cc
)

# Runner objects for @c utests_components: no @c graph_api_ut_stub — production @c graph_api is in @c aeg_runtime_api.cpp.
set(UT_COMPONENT_STUB_SOURCES
    stubs/main.cpp
    stubs/app_elf_stub.S
    stubs/aiert_mock.c
    stubs/other_mock.cpp
    stubs/baremetal_metadata.cpp
    stubs/test_comparator.cpp
    stubs/stubs.cc
)

# Production @c aeg_runtime_api.cpp + @c xaie_generate_events.cc (not @c gmio_api_mock / not events mock)
# for @c test_graph_api_run.cpp and @c test_generate_event.cpp.
set(BAREMETAL_RUNTIME_UTESTS_COMPONENTS_SUITE
    ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_api_message.cpp
    ${BAREMETAL_SOURCE_DIR}/aeg_aiebaremetal.cc
    ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_runtime_api.cpp
    ${BAREMETAL_SOURCE_DIR}/xaie_generate_events.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/aeg_configure_ut_stub.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/cert_host_ut_stub.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/submit_cmd_stubs/stub_cert_elf_loader.cpp
)

# Component / library-level CppUTest sources (single executable).
set(UT_COMPONENT_TEST_SOURCES
    tests/test_graph_api_run.cpp
    tests/test_xaiemem.cpp
    tests/test_barrier.cpp
    tests/test_aeg_api_message.cpp
    tests/test_generate_event.cpp
)

set(UT_SOURCE
    ${UT_AIEGRAPH_STUB_SOURCES}
    tests/test_aiebaremetal.cc
)

# utests — full source + link list: see "Reference: executables" comment block near top of file.
add_executable(utests ${UT_SOURCE} ${BAREMETAL_SOURCES_UTESTS_AIEGRAPH})
target_include_directories(utests BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stubs/fake_elfio)
target_include_directories(utests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stubs/submit_cmd_stubs)
target_compile_definitions(utests PRIVATE __UTEST_ISR_HW_PATH_BUILD__ __BAREMETAL_UTESTS__)
set_source_files_properties(
    ${BAREMETAL_SOURCE_DIR}/aeg_aiebaremetal.cc
    PROPERTIES COMPILE_OPTIONS "-include${CMAKE_CURRENT_SOURCE_DIR}/stubs/xil_exception_ut_override.h"
)
target_link_libraries(utests ${CPPUTEST_LINK_LIBS})
add_dependencies(utests CppUTest mock_aie_library mock_aie_library_no_symbol)

# utests_components — sources/deps: see "Reference: executables" near top of file.
add_executable(utests_components
    ${UT_COMPONENT_STUB_SOURCES}
    ${UT_COMPONENT_TEST_SOURCES}
    ${BAREMETAL_RUNTIME_UTESTS_COMPONENTS_SUITE}
)
target_include_directories(utests_components BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stubs/fake_elfio)
target_include_directories(utests_components PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stubs/submit_cmd_stubs)
target_compile_definitions(utests_components PRIVATE __UTEST_ISR_HW_PATH_BUILD__ __BAREMETAL_UTESTS__)
set_source_files_properties(
    ${BAREMETAL_SOURCE_DIR}/aeg_aiebaremetal.cc
    PROPERTIES COMPILE_OPTIONS "-include${CMAKE_CURRENT_SOURCE_DIR}/stubs/xil_exception_ut_override.h"
)
target_link_libraries(utests_components ${CPPUTEST_LINK_LIBS})
add_dependencies(utests_components CppUTest mock_aie_library mock_aie_library_no_symbol)

# dump_uc_app() and GmioApiImpl::aie2gm_nb() sim paths need __AIESIM__ on these TUs only (not whole utests — breaks
# test_aiebaremetal.cc include order). Stub must match aeg_aiebaremetal.cc so gmioapiimpl.h sees one __AIESIM__.
set_source_files_properties(
    ${BAREMETAL_SOURCE_DIR}/aeg_aiebaremetal.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/aeg_configure_ut_stub.cpp
    APPEND PROPERTIES COMPILE_DEFINITIONS __AIESIM__
)
# Compile abr_init_isr / abr_error_isr HW bodies + XScuGic path in aeg_aiebaremetal.cc alongside __AIESIM__ (gcov).
# test_aiebaremetal.cc gets the same define so AbrInitIsrTests mocks match that TU.
set_source_files_properties(
    ${BAREMETAL_SOURCE_DIR}/aeg_aiebaremetal.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_aiebaremetal.cc
    APPEND PROPERTIES COMPILE_DEFINITIONS __UTEST_ABR_ISR_COVERAGE_PATH__
)

# utests_cert_elf — sources/deps: see "Reference: executables" near top of file.
set(UT_CERT_ELF_LOADER_SOURCES
    stubs/main.cpp
    stubs/test_comparator.cpp
    tests/test_cert_elf_loader.cpp
    ${BAREMETAL_SOURCE_DIR}/cert_elf_loader.cc
)
add_executable(utests_cert_elf ${UT_CERT_ELF_LOADER_SOURCES})
target_include_directories(utests_cert_elf BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stubs/fake_elfio)
target_compile_definitions(utests_cert_elf PRIVATE __BAREMETAL_UTESTS__)
target_link_libraries(utests_cert_elf ${CPPUTEST_LINK_LIBS})
add_dependencies(utests_cert_elf CppUTest)

# utests_cert_host — sources/deps: see "Reference: executables" near top of file.
set(UT_CERT_HOST_SOURCES
    stubs/main.cpp
    stubs/app_elf_stub.S
    stubs/aiert_mock.c
    stubs/other_mock.cpp
    stubs/baremetal_metadata.cpp
    stubs/test_comparator.cpp
    stubs/stubs.cc
    tests/test_cert_host.cc
    stubs/submit_cmd_stubs/stub_cert_elf_loader.cpp
)
add_executable(utests_cert_host ${UT_CERT_HOST_SOURCES} ${BAREMETAL_SOURCE_DIR}/cert_host.cc)
target_include_directories(utests_cert_host BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stubs/fake_elfio)
target_include_directories(utests_cert_host PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stubs/submit_cmd_stubs)
target_compile_definitions(utests_cert_host PRIVATE __UTEST_ISR_HW_PATH_BUILD__ __BAREMETAL_UTESTS__)
target_link_libraries(utests_cert_host ${CPPUTEST_LINK_LIBS})
add_dependencies(utests_cert_host CppUTest mock_aie_library mock_aie_library_no_symbol)

# utests_gmioapiimpl — sources/deps: see "Reference: executables" near top of file.
set(UT_GMIOAPIIMPL_STUB_SOURCES
    stubs/main.cpp
    stubs/app_elf_stub.S
    stubs/aiert_mock.c
    stubs/other_mock.cpp
    stubs/baremetal_metadata.cpp
    stubs/test_comparator.cpp
    stubs/stubs.cc
)
set(UT_GMIOAPIIMPL_RUNTIME_SOURCES
    ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_api_message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/gmio_api_mock.cpp
)
add_executable(utests_gmioapiimpl
    tests/test_gmioapiimpl.cpp
    tests/test_externalbufferapiimpl.cpp
    ${UT_GMIOAPIIMPL_STUB_SOURCES}
    ${UT_GMIOAPIIMPL_RUNTIME_SOURCES}
)
target_compile_definitions(utests_gmioapiimpl PRIVATE __UTEST_ISR_HW_PATH_BUILD__ __BAREMETAL_UTESTS__)
target_link_libraries(utests_gmioapiimpl ${CPPUTEST_LINK_LIBS})
add_dependencies(utests_gmioapiimpl CppUTest mock_aie_library mock_aie_library_no_symbol)

# utests_aeg_configure — sources/deps: see "Reference: executables" near top of file.
set(UT_AEG_CONFIGURE_SOURCES
    tests/test_aeg_configure.cc
    ${BAREMETAL_SOURCE_DIR}/aeg_configure.cc
    ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_api_message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/gmio_api_mock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/graph_api_ut_stub.cpp
    ${UT_GMIOAPIIMPL_STUB_SOURCES}
)
add_executable(utests_aeg_configure ${UT_AEG_CONFIGURE_SOURCES})
target_compile_definitions(utests_aeg_configure PRIVATE __UTEST_ISR_HW_PATH_BUILD__ __BAREMETAL_UTESTS__)
target_link_libraries(utests_aeg_configure ${CPPUTEST_LINK_LIBS})
add_dependencies(utests_aeg_configure CppUTest mock_aie_library mock_aie_library_no_symbol)

# --- Post-build: cert ELF + helper script ---
set(CERT_ELF_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/cert-elf-binaries/ctrl_code.elf")
if(EXISTS ${CERT_ELF_SOURCE})
    foreach(_ut_tgt utests utests_components utests_cert_elf utests_gmioapiimpl utests_aeg_configure utests_cert_host)
        add_custom_command(TARGET ${_ut_tgt} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CERT_ELF_SOURCE}
                ${CMAKE_CURRENT_BINARY_DIR}/ctrl_code.elf
            COMMENT "Copying cert ELF next to ${_ut_tgt}"
        )
    endforeach()
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/script_files/run_tests_with_logs.sh)
    add_custom_command(TARGET utests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/script_files/run_tests_with_logs.sh
            ${CMAKE_CURRENT_BINARY_DIR}/run_tests_with_logs.sh
        COMMENT "Copying run_tests_with_logs.sh to build directory"
    )
endif()

# test_fal_util — sources/deps: see "Reference: executables" near top of file.
add_executable(test_fal_util
    stubs/main.cpp
    tests/test_fal_util.cpp
    stubs/stubs.cc
    ${BAREMETAL_SOURCE_DIR}/common_layer/aegothers/fal_util.cpp
)
target_compile_definitions(test_fal_util PRIVATE __BAREMETAL_UTESTS__ AIE_GEN=5)
target_include_directories(test_fal_util BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/xaiefal
)
target_include_directories(test_fal_util PRIVATE ${BAREMETAL_SOURCE_DIR}/include/common_layer/aegothers)
target_link_libraries(test_fal_util ${CPPUTEST_LINK_LIBS})
add_dependencies(test_fal_util CppUTest)

# ut_profiling — sources/deps: see "Reference: executables" near top of file.
set(PROF_UUT_SRC_FILE "${BAREMETAL_SOURCE_DIR}/common_layer/aegothers/aeg_profiling_api.cpp")
add_executable(ut_profiling
    stubs/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/profiling_stubs/stubs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/profiling_stubs/fal_stubs.cpp
    tests/test_aeg_profiling_api.cpp
    ${PROF_UUT_SRC_FILE}
)
target_compile_definitions(ut_profiling PRIVATE __BAREMETAL_UTESTS__ AIE_GEN=5 __AIESIM__)
set_property(TARGET ut_profiling PROPERTY INCLUDE_DIRECTORIES "")
target_include_directories(ut_profiling BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/profiling_stubs
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/xaiefal
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${source_dir}/include
    ${BAREMETAL_SOURCE_DIR}/include/common_layer/aegothers
    ${BAREMETAL_SOURCE_DIR}/include/common_layer
    ${BAREMETAL_SOURCE_DIR}/include
    ${BAREMETAL_SOURCE_DIR}
)
target_link_libraries(ut_profiling ${CPPUTEST_LINK_LIBS})
add_dependencies(ut_profiling CppUTest)

# test_submit_command — sources/deps: see "Reference: executables" near top.
set(SUBMIT_CMD_TEST_SOURCES
    stubs/main.cpp
    tests/test_submit_command.cpp
    stubs/submit_cmd_stubs/stub_xaie_mem.c
    stubs/submit_cmd_stubs/stub_barrier.c
    stubs/submit_cmd_stubs/stub_cert_elf_loader.cpp
    stubs/submit_cmd_stubs/cert_elf_globals.c
    stubs/submit_cmd_stubs/xaiengine.c
)
set(SUBMIT_CMD_UUT_SOURCES ${BAREMETAL_SOURCE_DIR}/cert_host.cc)

function(add_cert_host_mock_elf_loader_executable _tgt)
    add_executable(${_tgt} ${SUBMIT_CMD_TEST_SOURCES} ${SUBMIT_CMD_UUT_SOURCES})
    target_compile_definitions(${_tgt} PRIVATE __BAREMETAL_UTESTS__ AIE_GEN=5)
    target_include_directories(${_tgt} BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stubs/submit_cmd_stubs)
    target_include_directories(${_tgt} PRIVATE
        ${source_dir}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/stubs
        ${BAREMETAL_SOURCE_DIR}/include
        ${BAREMETAL_SOURCE_DIR}/include/common_layer
        ${BAREMETAL_SOURCE_DIR}/include/common_layer/aegothers
        ${BAREMETAL_SOURCE_DIR}
        ${THIRDPARTY_DIR}
    )
    target_link_libraries(${_tgt} ${CPPUTEST_LINK_LIBS})
    add_dependencies(${_tgt} CppUTest)
endfunction()

add_cert_host_mock_elf_loader_executable(test_submit_command)

# --- Coverage targets ---
if(ENABLE_COVERAGE)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)
    if(LCOV_PATH AND GENHTML_PATH)
        add_custom_target(coverage
            COMMAND ${CMAKE_COMMAND} -E echo "Capturing coverage from existing .gcda under build dir..."
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' '*/testcpputest/*' '*/thirdparty/*' --output-file coverage.info.cleaned
            COMMAND ${GENHTML_PATH} -o coverage_report coverage.info.cleaned
            COMMAND ${LCOV_PATH} --list coverage.info.cleaned > coverage_report.txt
            COMMAND ${CMAKE_COMMAND} -E echo "Generating API_COVERAGE.md..."
            COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/script_files/generate_coverage_md.py coverage_report.txt API_COVERAGE.md
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/API_COVERAGE.md ${CMAKE_CURRENT_SOURCE_DIR}/API_COVERAGE.md
            COMMAND ${CMAKE_COMMAND} -E echo "API_COVERAGE.md copied to main folder"
            COMMENT "lcov/genhtml from existing .gcda run tests manually first with coverage-enabled build"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )

        add_custom_target(coverage_utests
            COMMAND ${CMAKE_COMMAND} -E echo "Running ./utests..."
            COMMAND ./utests
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage_utests_raw.info
            COMMAND ${LCOV_PATH} --remove coverage_utests_raw.info '/usr/*' '*/thirdparty/*' --output-file coverage_utests.info
            COMMAND ${GENHTML_PATH} --legend --title "testcpputest utests" -o coverage_report_utests coverage_utests.info
            COMMAND ${LCOV_PATH} --list coverage_utests.info > coverage_utests_summary.txt
            COMMAND ${CMAKE_COMMAND} -E echo "Open ${CMAKE_BINARY_DIR}/coverage_report_utests/index.html"
            DEPENDS utests
            COMMENT "Run utests and generate coverage_report_utests/"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )

        add_custom_target(coverage_tests_only
            COMMAND ${CMAKE_COMMAND} -E echo "Running ./utests and ./utests_components..."
            COMMAND ./utests
            COMMAND ./utests_components
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage_tests_raw.info
            COMMAND ${LCOV_PATH} --remove coverage_tests_raw.info '/usr/*' '*/thirdparty/*' --output-file coverage_tests_filtered.info
            COMMAND ${LCOV_PATH} --extract coverage_tests_filtered.info '${CMAKE_CURRENT_SOURCE_DIR}/tests/*' --output-file coverage_tests.info
            COMMAND ${GENHTML_PATH} --legend --title "testcpputest tests/*.cpp" -o coverage_report_tests coverage_tests.info
            COMMAND ${LCOV_PATH} --list coverage_tests.info > coverage_tests_summary.txt
            COMMAND ${CMAKE_COMMAND} -E echo "Open ${CMAKE_BINARY_DIR}/coverage_report_tests/index.html"
            DEPENDS utests utests_components
            COMMENT "Coverage HTML for tests/ sources (utests + utests_components)"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )

        # Run every CppUTest / helper binary once, then one lcov capture (merged .gcda in build dir).
        add_custom_target(coverage_all_executables
            COMMAND ${CMAKE_COMMAND} -E echo "Running all test executables for merged coverage..."
            COMMAND bash -c "./utests || true; ./utests_components || true; ./utests_cert_elf || true; ./utests_cert_host || true; ./utests_gmioapiimpl || true; ./utests_aeg_configure || true; ./test_fal_util || true; ./ut_profiling || true; ./test_submit_command || true"
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage_all_executables_raw.info
            COMMAND ${LCOV_PATH} --remove coverage_all_executables_raw.info '/usr/*' '*/thirdparty/*' --output-file coverage_all_executables.info
            COMMAND ${GENHTML_PATH} --legend --title "testcpputest all executables" -o coverage_report_all_executables coverage_all_executables.info
            COMMAND ${LCOV_PATH} --list coverage_all_executables.info > coverage_all_executables_summary.txt
            COMMAND ${CMAKE_COMMAND} -E echo "Open ${CMAKE_BINARY_DIR}/coverage_report_all_executables/index.html"
            DEPENDS utests utests_components utests_cert_elf utests_cert_host utests_gmioapiimpl utests_aeg_configure test_fal_util ut_profiling test_submit_command
            COMMENT "Merged coverage after running all test binaries in this project"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )

        add_custom_target(coverage_utests_cert_host
            COMMAND ${CMAKE_COMMAND} -E echo "Running ./utests_cert_host..."
            COMMAND ./utests_cert_host
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage_utests_cert_host_raw.info
            COMMAND ${LCOV_PATH} --remove coverage_utests_cert_host_raw.info '/usr/*' '*/thirdparty/*' --output-file coverage_utests_cert_host.info
            COMMAND ${GENHTML_PATH} --legend --title "testcpputest utests_cert_host" -o coverage_report_utests_cert_host coverage_utests_cert_host.info
            COMMAND ${LCOV_PATH} --list coverage_utests_cert_host.info > coverage_utests_cert_host_summary.txt
            COMMAND ${CMAKE_COMMAND} -E echo "Open ${CMAKE_BINARY_DIR}/coverage_report_utests_cert_host/index.html"
            DEPENDS utests_cert_host
            COMMENT "Run utests_cert_host and generate coverage_report_utests_cert_host/"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )

        add_custom_target(coverage_utests_all
            COMMAND ${CMAKE_COMMAND} -E echo "Running ./utests then ./utests_cert_host..."
            COMMAND ./utests
            COMMAND ./utests_cert_host
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage_utests_all_raw.info
            COMMAND ${LCOV_PATH} --remove coverage_utests_all_raw.info '/usr/*' '*/thirdparty/*' --output-file coverage_utests_all.info
            COMMAND ${GENHTML_PATH} --legend --title "testcpputest utests + utests_cert_host" -o coverage_report_utests_all coverage_utests_all.info
            COMMAND ${LCOV_PATH} --list coverage_utests_all.info > coverage_utests_all_summary.txt
            COMMAND ${CMAKE_COMMAND} -E echo "Open ${CMAKE_BINARY_DIR}/coverage_report_utests_all/index.html"
            DEPENDS utests utests_cert_host
            COMMENT "Merged coverage for utests and utests_cert_host"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )

        add_custom_target(coverage_cert_host_cc
            COMMAND ${CMAKE_COMMAND} -E echo "Running ./utests_cert_host then extracting ${CERT_HOST_CC_ABS}"
            COMMAND ./utests_cert_host
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage_cert_host_raw.info
            COMMAND ${LCOV_PATH} --remove coverage_cert_host_raw.info '/usr/*' '*/thirdparty/*' --output-file coverage_cert_host_filtered.info
            COMMAND ${LCOV_PATH} --extract coverage_cert_host_filtered.info ${CERT_HOST_CC_ABS} --output-file coverage_cert_host.info
            COMMAND ${GENHTML_PATH} --legend --title "cert_host.cc" -o coverage_report_cert_host_cc coverage_cert_host.info
            COMMAND ${LCOV_PATH} --list coverage_cert_host.info > coverage_cert_host_summary.txt
            COMMAND ${CMAKE_COMMAND} -E echo "Open ${CMAKE_BINARY_DIR}/coverage_report_cert_host_cc/index.html"
            DEPENDS utests_cert_host
            COMMENT "Coverage HTML for cert_host.cc only"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
    else()
        message(WARNING "lcov or genhtml not found")
    endif()
endif()

add_custom_target(cleancov
    COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
    COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcno" -delete
    COMMAND bash -c "rm -f '${CMAKE_BINARY_DIR}/coverage.info' '${CMAKE_BINARY_DIR}/coverage.info.cleaned' '${CMAKE_BINARY_DIR}/coverage_utests_raw.info' '${CMAKE_BINARY_DIR}/coverage_utests.info' '${CMAKE_BINARY_DIR}/coverage_utests_summary.txt' '${CMAKE_BINARY_DIR}/coverage_utests_cert_host_raw.info' '${CMAKE_BINARY_DIR}/coverage_utests_cert_host.info' '${CMAKE_BINARY_DIR}/coverage_utests_cert_host_summary.txt' '${CMAKE_BINARY_DIR}/coverage_utests_all_raw.info' '${CMAKE_BINARY_DIR}/coverage_utests_all.info' '${CMAKE_BINARY_DIR}/coverage_utests_all_summary.txt' '${CMAKE_BINARY_DIR}/coverage_cert_host_raw.info' '${CMAKE_BINARY_DIR}/coverage_cert_host_filtered.info' '${CMAKE_BINARY_DIR}/coverage_cert_host.info' '${CMAKE_BINARY_DIR}/coverage_cert_host_summary.txt' '${CMAKE_BINARY_DIR}/coverage_tests_raw.info' '${CMAKE_BINARY_DIR}/coverage_tests_filtered.info' '${CMAKE_BINARY_DIR}/coverage_tests.info' '${CMAKE_BINARY_DIR}/coverage_tests_summary.txt' '${CMAKE_BINARY_DIR}/coverage_all_executables_raw.info' '${CMAKE_BINARY_DIR}/coverage_all_executables.info' '${CMAKE_BINARY_DIR}/coverage_all_executables_summary.txt'"
    COMMAND bash -c "rm -rf '${CMAKE_BINARY_DIR}/coverage_report' '${CMAKE_BINARY_DIR}/coverage_report_utests' '${CMAKE_BINARY_DIR}/coverage_report_utests_cert_host' '${CMAKE_BINARY_DIR}/coverage_report_utests_all' '${CMAKE_BINARY_DIR}/coverage_report_cert_host_cc' '${CMAKE_BINARY_DIR}/coverage_report_tests' '${CMAKE_BINARY_DIR}/coverage_report_all_executables'"
    COMMENT "Cleaning coverage data files"
)

add_custom_target(fullclean
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
    COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcno" -delete
    COMMENT "Full clean including coverage data"
)

add_custom_target(run_utests
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running utests -c..."
    COMMAND bash -c "echo -e 'y\\ny' | ./utests -c 2>&1 | tee logs/utests_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "utests completed. Check logs directory for output."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS utests
    COMMENT "Running utests with coverage and logging output"
)

add_custom_target(run_utests_components
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running utests_components -c..."
    COMMAND bash -c "echo -e 'y\\ny' | ./utests_components -c 2>&1 | tee logs/utests_components_$$(date +%Y%m%d_%H%M%S).log || true"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS utests_components
    COMMENT "Running utests_components (graph_api, xaiemem, common, barrier, api_message, generate_event)"
)

add_custom_target(run_ut_profiling
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running ut_profiling..."
    COMMAND bash -c "./ut_profiling 2>&1 | tee logs/ut_profiling_$$(date +%Y%m%d_%H%M%S).log || true"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS ut_profiling
    COMMENT "Running ut_profiling with logging"
)

add_custom_target(run_test_fal_util
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running test_fal_util..."
    COMMAND bash -c "./test_fal_util 2>&1 | tee logs/test_fal_util_$$(date +%Y%m%d_%H%M%S).log || true"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS test_fal_util
    COMMENT "Running test_fal_util with logging"
)

add_custom_target(run_all_tests
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running utests -c ==="
    COMMAND bash -c "echo -e 'y\\ny' | ./utests -c 2>&1 | tee logs/utests_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running utests_components -c ==="
    COMMAND bash -c "echo -e 'y\\ny' | ./utests_components -c 2>&1 | tee logs/utests_components_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running ut_profiling ==="
    COMMAND bash -c "./ut_profiling 2>&1 | tee logs/ut_profiling_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running test_fal_util ==="
    COMMAND bash -c "./test_fal_util 2>&1 | tee logs/test_fal_util_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running test_submit_command ==="
    COMMAND bash -c "./test_submit_command 2>&1 | tee logs/test_submit_command_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running utests_cert_host -c ==="
    COMMAND bash -c "echo -e 'y\\ny' | ./utests_cert_host -c 2>&1 | tee logs/utests_cert_host_$$(date +%Y%m%d_%H%M%S).log || true"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS utests utests_components ut_profiling test_fal_util test_submit_command utests_cert_host
    COMMENT "Running all tests with automated input and logging"
)
