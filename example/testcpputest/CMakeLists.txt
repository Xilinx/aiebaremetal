# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.10)

project(testcpputest)

option(ENABLE_COVERAGE "Enable coverage reporting with gcov/lcov" ON)
option(ENABLE_STACK_USAGE "Enable stack usage analysis" ON)

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
set(THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/aielib/include)
execute_process(COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/script_files/setup_includes.sh)

include_directories(
    ${source_dir}/include
    ${BAREMETAL_SOURCE_DIR}/include
    ${BAREMETAL_SOURCE_DIR}/include/common_layer
    ${BAREMETAL_SOURCE_DIR}/include/common_layer/aegothers
    ${THIRDPARTY_DIR}
	${BAREMETAL_SOURCE_DIR}/include/elfio
    ${BAREMETAL_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
link_directories(${binary_dir}/src/CppUTest)

set(BAREMETAL_SOURCES
    ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_runtime_api.cpp
    ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_api_message.cpp
    ${BAREMETAL_SOURCE_DIR}/aeg_aiebaremetal.cc
    ${BAREMETAL_SOURCE_DIR}/aeg_configure.cc
    
    # ${BAREMETAL_SOURCE_DIR}/common_layer/aegothers/aeg_profiling_api.cpp
    #${BAREMETAL_SOURCE_DIR}/common_layer/aegothers/fal_util.cpp
    ${BAREMETAL_SOURCE_DIR}/xaie_generate_events.cc

    ${BAREMETAL_SOURCE_DIR}/cert_host.cc
    #${BAREMETAL_SOURCE_DIR}/cert_elf.cc
    ${BAREMETAL_SOURCE_DIR}/cert_elf_loader.cc
)

set(UT_SOURCE
    stubs/main.cpp
    
    stubs/aiert_mock.c
    stubs/other_mock.cpp

    stubs/baremetal_metadata.cpp

    stubs/test_comparator.cpp
    tests/test_graph_api_run.cpp
    tests/test_xaiemem.cpp
    tests/test_common.cpp
    tests/test_barrier.cpp
    tests/test_aeg_api_message.cpp
    tests/test_generate_event.cpp
    stubs/stubs.cc
    tests/test_gmioapiimpl.cpp
    tests/test_externalbufferapiimpl.cpp
    tests/test_cert_host.cc
    tests/test_cert_elf_loader.cpp
    tests/test_aeg_configure.cc
    tests/test_aiebaremetal.cc
   # tests/test_aiebaremetal_api.cpp
)
add_executable(utests ${UT_SOURCE} ${BAREMETAL_SOURCES})
target_compile_definitions(utests PRIVATE __BAREMETAL_UTESTS__)
target_link_libraries(utests
    ${binary_dir}/src/CppUTest/libCppUTest.a
    ${binary_dir}/src/CppUTestExt/libCppUTestExt.a
    dl
)
add_dependencies(utests  CppUTest)

# Copy cert ELF from repo (thirdparty/cert-elf-binaries/ctrl_code.elf) next to utests
set(CERT_ELF_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/cert-elf-binaries/ctrl_code.elf")
if(EXISTS ${CERT_ELF_SOURCE})
    add_custom_command(TARGET utests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CERT_ELF_SOURCE}
            ${CMAKE_CURRENT_BINARY_DIR}/ctrl_code.elf
        COMMENT "Copying thirdparty/cert-elf-binaries/ctrl_code.elf to build directory"
    )
endif()

# Copy test script to build directory
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/script_files/run_tests_with_logs.sh)
    add_custom_command(TARGET utests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/script_files/run_tests_with_logs.sh
            ${CMAKE_CURRENT_BINARY_DIR}/run_tests_with_logs.sh
        COMMENT "Copying run_tests_with_logs.sh to build directory"
    )
endif()

add_definitions(-DAIE_GEN=5)
# target_link_libraries(utests CppUTest CppUTestExt)

# Build mock AIE library for testing
add_library(mock_aie_library SHARED stubs/mock_aie_library.cpp)
set_target_properties(mock_aie_library PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    PREFIX ""
    SUFFIX ".so"
)

# Build mock AIE library without symbol for testing
add_library(mock_aie_library_no_symbol SHARED stubs/mock_aie_library_no_symbol.cpp)
set_target_properties(mock_aie_library_no_symbol PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    PREFIX ""
    SUFFIX ".so"
)

# Make utests depend on mock libraries
add_dependencies(utests mock_aie_library mock_aie_library_no_symbol)

# Build separate executable for FAL utility tests
set(FAL_UTIL_SOURCES
    ${BAREMETAL_SOURCE_DIR}/common_layer/aegothers/fal_util.cpp
)

set(FAL_UTIL_TEST_SOURCE
    stubs/main.cpp
    tests/test_fal_util.cpp
    stubs/stubs.cc
)

add_executable(test_fal_util ${FAL_UTIL_TEST_SOURCE} ${FAL_UTIL_SOURCES})
target_compile_definitions(test_fal_util PRIVATE __BAREMETAL_UTESTS__ AIE_GEN=5)
target_include_directories(test_fal_util BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/xaiefal
)
target_include_directories(test_fal_util PRIVATE
    ${BAREMETAL_SOURCE_DIR}/include/common_layer/aegothers
)
target_link_libraries(test_fal_util
    ${binary_dir}/src/CppUTest/libCppUTest.a
    ${binary_dir}/src/CppUTestExt/libCppUTestExt.a
    dl
)
add_dependencies(test_fal_util CppUTest)

# Build separate executable for profiling API tests
set(PROF_UUT_SRC_FILE "${BAREMETAL_SOURCE_DIR}/common_layer/aegothers/aeg_profiling_api.cpp")
set(PROF_UTS_TEST_FILE "tests/test_aeg_profiling_api.cpp")
set(PROF_UT_SOURCES
    stubs/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/profiling_stubs/stubs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/profiling_stubs/fal_stubs.cpp
    ${PROF_UTS_TEST_FILE}
    ${PROF_UUT_SRC_FILE}
)

add_executable(ut_profiling ${PROF_UT_SOURCES})
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
target_link_libraries(ut_profiling
    ${binary_dir}/src/CppUTest/libCppUTest.a
    ${binary_dir}/src/CppUTestExt/libCppUTestExt.a
    dl
)
add_dependencies(ut_profiling CppUTest)

set(SUBMIT_CMD_TEST_SOURCES
    stubs/main.cpp
    tests/test_submit_command.cpp
    stubs/submit_cmd_stubs/stub_xaie_mem.c
    stubs/submit_cmd_stubs/stub_barrier.c
    stubs/submit_cmd_stubs/stub_cert_elf_loader.cpp
    stubs/submit_cmd_stubs/xaiengine.c
)

set(SUBMIT_CMD_UUT_SOURCES
    ${BAREMETAL_SOURCE_DIR}/cert_host.cc
)

add_executable(test_submit_command
    ${SUBMIT_CMD_TEST_SOURCES}
    ${SUBMIT_CMD_UUT_SOURCES}
)
target_compile_definitions(test_submit_command PRIVATE __BAREMETAL_UTESTS__ AIE_GEN=5)
target_include_directories(test_submit_command BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/submit_cmd_stubs
)
target_include_directories(test_submit_command PRIVATE
    ${source_dir}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${BAREMETAL_SOURCE_DIR}/include
    ${BAREMETAL_SOURCE_DIR}/include/common_layer
    ${BAREMETAL_SOURCE_DIR}/include/common_layer/aegothers
    ${BAREMETAL_SOURCE_DIR}
    ${THIRDPARTY_DIR}
)
target_link_libraries(test_submit_command
    ${binary_dir}/src/CppUTest/libCppUTest.a
    ${binary_dir}/src/CppUTestExt/libCppUTestExt.a
    dl
)
add_dependencies(test_submit_command CppUTest)

if(ENABLE_COVERAGE)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)
    if(LCOV_PATH AND GENHTML_PATH)
        add_custom_target(coverage
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' '*/testcpputest/*' '*/thirdparty/*' --output-file coverage.info.cleaned
            COMMAND ${GENHTML_PATH} -o coverage_report coverage.info.cleaned
            COMMAND ${LCOV_PATH} --list coverage.info.cleaned > coverage_report.txt
            COMMAND ${CMAKE_COMMAND} -E echo "Generating API_COVERAGE.md..."
            COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/script_files/generate_coverage_md.py coverage_report.txt API_COVERAGE.md
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/API_COVERAGE.md ${CMAKE_CURRENT_SOURCE_DIR}/API_COVERAGE.md
            COMMAND ${CMAKE_COMMAND} -E echo "API_COVERAGE.md copied to main folder"
            COMMENT "Generating coverage report with lcov/genhtml, text report, and markdown report"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS utests test_fal_util ut_profiling test_submit_command
	  )
    else()
        message(WARNING "lcov or genhtml not found")
    endif()
endif()

add_custom_target(cleancov
    COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
    COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcno" -delete
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/coverage.info ${CMAKE_BINARY_DIR}/coverage.info.cleaned
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/coverage_report
    COMMENT "Cleaning coverage data files"
)

add_custom_target(fullclean
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
    COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcno" -delete
    COMMENT "Full clean including coverage data"
)

# Custom targets for running tests with logging
add_custom_target(run_utests
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running utests -c..."
    COMMAND bash -c "echo -e 'y\\ny' | ./utests -c 2>&1 | tee logs/utests_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "utests completed. Check logs directory for output."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS utests
    COMMENT "Running utests with coverage and logging output"
)

add_custom_target(run_ut_profiling
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running ut_profiling..."
    COMMAND bash -c "./ut_profiling 2>&1 | tee logs/ut_profiling_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "ut_profiling completed. Check logs directory for output."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS ut_profiling
    COMMENT "Running ut_profiling with logging"
)

add_custom_target(run_test_fal_util
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running test_fal_util..."
    COMMAND bash -c "./test_fal_util 2>&1 | tee logs/test_fal_util_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "test_fal_util completed. Check logs directory for output."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS test_fal_util
    COMMENT "Running test_fal_util with logging"
)

add_custom_target(run_all_tests
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running all tests with logging..."
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running utests -c ==="
    COMMAND bash -c "echo -e 'y\\ny' | ./utests -c 2>&1 | tee logs/utests_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running ut_profiling ==="
    COMMAND bash -c "./ut_profiling 2>&1 | tee logs/ut_profiling_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running test_fal_util ==="
    COMMAND bash -c "./test_fal_util 2>&1 | tee logs/test_fal_util_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running test_submit_command ==="
    COMMAND bash -c "./test_submit_command 2>&1 | tee logs/test_submit_command_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "All tests completed. Logs saved in logs/ directory"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS utests ut_profiling test_fal_util test_submit_command
    COMMENT "Running all tests with automated input and logging"
)