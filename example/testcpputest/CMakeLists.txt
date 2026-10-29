# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
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
	${CMAKE_CURRENT_SOURCE_DIR}/stubs/fake_elfio
    ${BAREMETAL_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
)
link_directories(${binary_dir}/src/CppUTest)

set(BAREMETAL_SOURCES
    ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_runtime_api.cpp
    ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_api_message.cpp
    ${BAREMETAL_SOURCE_DIR}/xaie_generate_events.cc
)

set(UT_SOURCE
    stubs/main.cpp

    stubs/aiert_mock.c
    stubs/test_comparator.cpp
    tests/test_graph_api_run.cpp
    tests/test_xaiemem.cpp
    tests/test_barrier.cpp
    tests/test_aeg_api_message.cpp
    tests/test_generate_event.cpp
    stubs/stubs.cc
)
add_executable(utests_external ${UT_SOURCE} ${BAREMETAL_SOURCES})
target_compile_definitions(utests_external PRIVATE __BAREMETAL_UTESTS__)
target_compile_options(utests_external PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)
target_link_libraries(utests_external
    ${binary_dir}/src/CppUTest/libCppUTest.a
    ${binary_dir}/src/CppUTestExt/libCppUTestExt.a
    dl
)
add_dependencies(utests_external  CppUTest)

set(CERT_ELF_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/cert-elf-binaries/ctrl_code.elf")
if(EXISTS ${CERT_ELF_SOURCE})
    add_custom_command(TARGET utests_external POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CERT_ELF_SOURCE}
            ${CMAKE_CURRENT_BINARY_DIR}/ctrl_code.elf
        COMMENT "Copying thirdparty/cert-elf-binaries/ctrl_code.elf to build directory"
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/script_files/run_tests_with_logs.sh)
    add_custom_command(TARGET utests_external POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/script_files/run_tests_with_logs.sh
            ${CMAKE_CURRENT_BINARY_DIR}/run_tests_with_logs.sh
        COMMENT "Copying run_tests_with_logs.sh to build directory"
    )
endif()

add_definitions(-DAIE_GEN=5)

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

add_dependencies(utests_external mock_aie_library mock_aie_library_no_symbol)

set(FAL_UTIL_SOURCES
    ${BAREMETAL_SOURCE_DIR}/common_layer/aegothers/fal_util.cpp
)

set(FAL_UTIL_TEST_SOURCE
    stubs/main.cpp
    tests/test_fal_util.cpp
    stubs/stubs.cc
)

add_executable(utests_fal_util ${FAL_UTIL_TEST_SOURCE} ${FAL_UTIL_SOURCES})
target_compile_definitions(utests_fal_util PRIVATE __BAREMETAL_UTESTS__ AIE_GEN=5)
target_include_directories(utests_fal_util BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/xaiefal
)
target_include_directories(utests_fal_util PRIVATE
    ${BAREMETAL_SOURCE_DIR}/include/common_layer/aegothers
)
target_link_libraries(utests_fal_util
    ${binary_dir}/src/CppUTest/libCppUTest.a
    ${binary_dir}/src/CppUTestExt/libCppUTestExt.a
    dl
)
add_dependencies(utests_fal_util CppUTest)

set(PROF_UUT_SRC_FILE "${BAREMETAL_SOURCE_DIR}/common_layer/aegothers/aeg_profiling_api.cpp")
set(PROF_UTS_TEST_FILE "tests/test_aeg_profiling_api.cpp")
set(PROF_UT_SOURCES
    stubs/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/profiling_stubs/stubs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/profiling_stubs/fal_stubs.cpp
    ${PROF_UTS_TEST_FILE}
    ${PROF_UUT_SRC_FILE}
)

add_executable(utests_profiling ${PROF_UT_SOURCES})
target_compile_definitions(utests_profiling PRIVATE __BAREMETAL_UTESTS__ AIE_GEN=5 __AIESIM__)
target_compile_options(utests_profiling PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)
set_property(TARGET utests_profiling PROPERTY INCLUDE_DIRECTORIES "")
target_include_directories(utests_profiling BEFORE PRIVATE
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
target_link_libraries(utests_profiling
    ${binary_dir}/src/CppUTest/libCppUTest.a
    ${binary_dir}/src/CppUTestExt/libCppUTestExt.a
    dl
)
add_dependencies(utests_profiling CppUTest)

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

add_executable(utests_submit_command
    ${SUBMIT_CMD_TEST_SOURCES}
    ${SUBMIT_CMD_UUT_SOURCES}
)
target_compile_definitions(utests_submit_command PRIVATE __BAREMETAL_UTESTS__ AIE_GEN=5)
target_compile_options(utests_submit_command PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)
target_include_directories(utests_submit_command BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/fake_elfio
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/submit_cmd_stubs
)
target_include_directories(utests_submit_command PRIVATE
    ${source_dir}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${BAREMETAL_SOURCE_DIR}/include
    ${BAREMETAL_SOURCE_DIR}/include/common_layer
    ${BAREMETAL_SOURCE_DIR}/include/common_layer/aegothers
    ${BAREMETAL_SOURCE_DIR}
    ${THIRDPARTY_DIR}
)
target_link_libraries(utests_submit_command
    ${binary_dir}/src/CppUTest/libCppUTest.a
    ${binary_dir}/src/CppUTestExt/libCppUTestExt.a
    dl
)
add_dependencies(utests_submit_command CppUTest)

set(UT_CERT_ELF_LOADER_SOURCES
    stubs/main.cpp
    tests/test_cert_elf_loader.cpp
    ${BAREMETAL_SOURCE_DIR}/cert_elf_loader.cc
)
add_executable(utests_cert_elf ${UT_CERT_ELF_LOADER_SOURCES})
target_include_directories(utests_cert_elf BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/fake_elfio
)
target_compile_definitions(utests_cert_elf PRIVATE __BAREMETAL_UTESTS__ AIE_GEN=5)
target_compile_options(utests_cert_elf PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)
target_link_libraries(utests_cert_elf
    ${binary_dir}/src/CppUTest/libCppUTest.a
    ${binary_dir}/src/CppUTestExt/libCppUTestExt.a
    dl
)
add_dependencies(utests_cert_elf CppUTest)

set(UT_CERT_HOST_SOURCES
    stubs/main.cpp
    stubs/aiert_mock.c
    stubs/test_comparator.cpp
    stubs/stubs.cc
    tests/test_cert_host.cc
    stubs/submit_cmd_stubs/stub_cert_elf_loader.cpp
)
add_executable(utests_cert_host ${UT_CERT_HOST_SOURCES} ${BAREMETAL_SOURCE_DIR}/cert_host.cc)
target_include_directories(utests_cert_host BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/fake_elfio
)
target_include_directories(utests_cert_host PRIVATE
    ${source_dir}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${BAREMETAL_SOURCE_DIR}/include
    ${BAREMETAL_SOURCE_DIR}/include/common_layer
    ${BAREMETAL_SOURCE_DIR}/include/common_layer/aegothers
    ${BAREMETAL_SOURCE_DIR}
    ${THIRDPARTY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/submit_cmd_stubs
)
target_compile_definitions(utests_cert_host PRIVATE __UTEST_ISR_HW_PATH_BUILD__ __BAREMETAL_UTESTS__ AIE_GEN=5)
target_compile_options(utests_cert_host PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)
target_link_libraries(utests_cert_host
    ${binary_dir}/src/CppUTest/libCppUTest.a
    ${binary_dir}/src/CppUTestExt/libCppUTestExt.a
    dl
)
add_dependencies(utests_cert_host CppUTest mock_aie_library mock_aie_library_no_symbol)

set(UT_GMIOAPIIMPL_STUB_SOURCES
    stubs/main.cpp
    stubs/aiert_mock.c
    stubs/other_mock.cpp
    stubs/baremetal_metadata.cpp
    stubs/test_comparator.cpp
    stubs/stubs.cc
)
set(UT_GMIOAPIIMPL_RUNTIME_SOURCES
    ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_api_message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/gmio_api_mock.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/stubs/graph_api_mock.cpp
)
add_executable(utests_gmioapiimpl
    tests/test_gmioapiimpl.cpp
    tests/test_externalbufferapiimpl.cpp
    ${UT_GMIOAPIIMPL_STUB_SOURCES}
    ${UT_GMIOAPIIMPL_RUNTIME_SOURCES}
)

target_include_directories(utests_gmioapiimpl BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
)
target_compile_options(utests_gmioapiimpl PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-include${CMAKE_CURRENT_SOURCE_DIR}/stubs/gmio_shim/xaiemem.h>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_gmioapiimpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_externalbufferapiimpl.cpp
    PROPERTIES
        COMPILE_DEFINITIONS "__AIESIM__"
)
target_compile_definitions(utests_gmioapiimpl PRIVATE __UTEST_ISR_HW_PATH_BUILD__ __BAREMETAL_UTESTS__ AIE_GEN=5)
target_link_libraries(utests_gmioapiimpl
    ${binary_dir}/src/CppUTest/libCppUTest.a
    ${binary_dir}/src/CppUTestExt/libCppUTestExt.a
    dl
)
add_dependencies(utests_gmioapiimpl CppUTest mock_aie_library mock_aie_library_no_symbol)

set(UT_AEG_CONFIGURE_SOURCES
    tests/test_aeg_configure.cc
    ${BAREMETAL_SOURCE_DIR}/aeg_configure.cc
    ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_api_message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/gmio_api_mock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/graph_api_mock.cpp
    ${UT_GMIOAPIIMPL_STUB_SOURCES}
)
add_executable(utests_aeg_configure ${UT_AEG_CONFIGURE_SOURCES})
target_include_directories(utests_aeg_configure BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
)
target_compile_definitions(utests_aeg_configure PRIVATE __UTEST_ISR_HW_PATH_BUILD__ __BAREMETAL_UTESTS__ AIE_GEN=5)
target_compile_options(utests_aeg_configure PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)
target_link_libraries(utests_aeg_configure
    ${binary_dir}/src/CppUTest/libCppUTest.a
    ${binary_dir}/src/CppUTestExt/libCppUTestExt.a
    dl
)
add_dependencies(utests_aeg_configure CppUTest mock_aie_library mock_aie_library_no_symbol)

set(UT_AIEBAREMETAL_STUB_SOURCES
    stubs/main.cpp
    stubs/aiert_mock.c
    stubs/other_mock.cpp
    stubs/baremetal_metadata.cpp
    stubs/test_comparator.cpp
    stubs/graph_api_mock.cpp
    stubs/stubs.cc
)
set(UT_AIEBAREMETAL_RUNTIME_SOURCES
    ${BAREMETAL_SOURCE_DIR}/common_layer/aeg_api_message.cpp
    ${BAREMETAL_SOURCE_DIR}/aeg_aiebaremetal.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/gmio_api_mock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/xaie_generate_events_mock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/aeg_configure_mock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/cert_host_mock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/submit_cmd_stubs/stub_cert_elf_loader.cpp
)
add_executable(utests_aiebaremetal
    tests/test_aiebaremetal.cc
    ${UT_AIEBAREMETAL_STUB_SOURCES}
    ${UT_AIEBAREMETAL_RUNTIME_SOURCES}
)
target_include_directories(utests_aiebaremetal BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/aiebaremetal_configure
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/fake_elfio
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/fake_klogr
)
target_include_directories(utests_aiebaremetal PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/submit_cmd_stubs
)
target_compile_definitions(utests_aiebaremetal PRIVATE
    __UTEST_ISR_HW_PATH_BUILD__
    __BAREMETAL_UTESTS__
    AIE_GEN=5
    CERT_ELF_SYMBOLS_FROM_OTHER_MOCK
)
target_compile_options(utests_aiebaremetal PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-include${CMAKE_CURRENT_SOURCE_DIR}/stubs/gmio_shim/xaiemem.h>
    $<$<COMPILE_LANGUAGE:CXX>:-include${CMAKE_CURRENT_SOURCE_DIR}/stubs/fal_util_mock/fal_util.h>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
)
set_source_files_properties(
    ${BAREMETAL_SOURCE_DIR}/aeg_aiebaremetal.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/aeg_configure_mock.cpp
    APPEND PROPERTIES COMPILE_DEFINITIONS __AIESIM__
)
set_source_files_properties(
    ${BAREMETAL_SOURCE_DIR}/aeg_aiebaremetal.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_aiebaremetal.cc
    APPEND PROPERTIES COMPILE_DEFINITIONS __UTEST_ABR_ISR_COVERAGE_PATH__
)
target_link_libraries(utests_aiebaremetal
    ${binary_dir}/src/CppUTest/libCppUTest.a
    ${binary_dir}/src/CppUTestExt/libCppUTestExt.a
    dl
)
add_dependencies(utests_aiebaremetal CppUTest mock_aie_library mock_aie_library_no_symbol)

if(EXISTS ${CERT_ELF_SOURCE})
    add_custom_command(TARGET utests_cert_elf POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CERT_ELF_SOURCE}
            ${CMAKE_CURRENT_BINARY_DIR}/ctrl_code.elf
        COMMENT "Copying cert ELF next to utests_cert_elf"
    )
endif()

if(ENABLE_COVERAGE)
    set(_COVERAGE_TEST_DEPS
        utests_external
        utests_fal_util
        utests_profiling
        utests_submit_command
        utests_cert_elf
        utests_cert_host
        utests_gmioapiimpl
        utests_aeg_configure
        utests_aiebaremetal
    )

    find_program(LCOV_PATH lcov HINTS $ENV{HOME}/.local/bin /usr/local/bin /usr/bin)
    find_program(GENHTML_PATH genhtml HINTS $ENV{HOME}/.local/bin /usr/local/bin /usr/bin)
    if(LCOV_PATH AND GENHTML_PATH)
        set(LCOV_RC_FLAGS --rc lcov_branch_coverage=1)
        add_custom_target(coverage
            COMMAND ${LCOV_PATH} ${LCOV_RC_FLAGS} --directory . --capture --output-file coverage.info
            COMMAND ${LCOV_PATH} ${LCOV_RC_FLAGS} --remove coverage.info '/usr/*' '*/testcpputest/*' '*/thirdparty/*' '*/src/include/rtpapiimpl.h' '*/src/include/sharedbufferapiimpl.h' --output-file coverage.info.cleaned
            COMMAND ${LCOV_PATH} ${LCOV_RC_FLAGS} --capture --directory CMakeFiles/utests_gmioapiimpl.dir --output-file gmio_target.info
            COMMAND ${LCOV_PATH} ${LCOV_RC_FLAGS} --extract gmio_target.info '*/src/include/gmioapiimpl.h' '*/src/include/externalbufferapiimpl.h' --output-file gmio_uut_only.info
            COMMAND ${LCOV_PATH} ${LCOV_RC_FLAGS} --remove coverage.info.cleaned '*/src/include/gmioapiimpl.h' '*/src/include/externalbufferapiimpl.h' --output-file coverage.info.others
            COMMAND ${LCOV_PATH} ${LCOV_RC_FLAGS} --add-tracefile coverage.info.others --add-tracefile gmio_uut_only.info --output-file coverage.info.cleaned
            COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/script_files/exclude_lines_from_coverage.sh
                    coverage.info.cleaned
                    aeg_aiebaremetal.cc:2002-2192
                    aeg_aiebaremetal.cc:875-886
            COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/coverage_report
            COMMAND ${GENHTML_PATH} --branch-coverage --rc genhtml_branch_coverage=1 -o coverage_report coverage.info.cleaned
            COMMAND ${LCOV_PATH} ${LCOV_RC_FLAGS} --list coverage.info.cleaned > coverage_report.txt
            COMMAND ${CMAKE_COMMAND} -E echo "Generating API_COVERAGE.md..."
            COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/script_files/generate_coverage_md.sh coverage_report.txt API_COVERAGE.md
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/API_COVERAGE.md ${CMAKE_CURRENT_SOURCE_DIR}/API_COVERAGE.md
            COMMAND ${CMAKE_COMMAND} -E echo "API_COVERAGE.md copied to main folder"
            COMMENT "Generating coverage report with lcov/genhtml, text report, and markdown report"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS ${_COVERAGE_TEST_DEPS}
        )
    else()
        message(WARNING "lcov or genhtml not found")
    endif()
endif()

add_custom_target(cleancov
    COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/coverage.info ${CMAKE_BINARY_DIR}/coverage.info.cleaned
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/coverage_report
    COMMENT "Remove .gcda and reports only; keep .gcno. Re-run all test binaries before make coverage."
)

add_custom_target(fullclean
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
    COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcno" -delete
    COMMENT "Full clean including coverage notes (.gcno) — run make -j32 before the next coverage run"
)

add_custom_target(run_utests_external
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running utests_external -c..."
    COMMAND bash -c "echo -e 'y\\ny' | ./utests_external -c 2>&1 | tee logs/utests_external_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "utests_external completed. Check logs directory for output."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS utests_external
    COMMENT "Running utests_external with coverage and logging output"
)

add_custom_target(run_utests_profiling
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running utests_profiling..."
    COMMAND bash -c "./utests_profiling 2>&1 | tee logs/utests_profiling_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "utests_profiling completed. Check logs directory for output."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS utests_profiling
    COMMENT "Running utests_profiling with logging"
)

add_custom_target(run_utests_fal_util
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running utests_fal_util..."
    COMMAND bash -c "./utests_fal_util 2>&1 | tee logs/utests_fal_util_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "utests_fal_util completed. Check logs directory for output."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS utests_fal_util
    COMMENT "Running utests_fal_util with logging"
)

add_custom_target(run_utests_submit_command
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running utests_submit_command..."
    COMMAND bash -c "./utests_submit_command 2>&1 | tee logs/utests_submit_command_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "utests_submit_command completed. Check logs directory for output."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS utests_submit_command
    COMMENT "Running utests_submit_command with logging"
)

add_custom_target(run_utests_cert_elf
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running utests_cert_elf..."
    COMMAND bash -c "./utests_cert_elf 2>&1 | tee logs/utests_cert_elf_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "utests_cert_elf completed. Check logs directory for output."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS utests_cert_elf
    COMMENT "Running utests_cert_elf with logging"
)

add_custom_target(run_utests_cert_host
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running utests_cert_host..."
    COMMAND bash -c "./utests_cert_host 2>&1 | tee logs/utests_cert_host_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "utests_cert_host completed. Check logs directory for output."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS utests_cert_host
    COMMENT "Running utests_cert_host with logging"
)

add_custom_target(run_utests_gmioapiimpl
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running utests_gmioapiimpl..."
    COMMAND bash -c "./utests_gmioapiimpl 2>&1 | tee logs/utests_gmioapiimpl_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "utests_gmioapiimpl completed. Check logs directory for output."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS utests_gmioapiimpl
    COMMENT "Running utests_gmioapiimpl with logging"
)

add_custom_target(run_utests_aeg_configure
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running utests_aeg_configure..."
    COMMAND bash -c "./utests_aeg_configure 2>&1 | tee logs/utests_aeg_configure_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "utests_aeg_configure completed. Check logs directory for output."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS utests_aeg_configure
    COMMENT "Running utests_aeg_configure with logging"
)

add_custom_target(run_utests_aiebaremetal
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running utests_aiebaremetal..."
    COMMAND bash -c "./utests_aiebaremetal 2>&1 | tee logs/utests_aiebaremetal_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo "utests_aiebaremetal completed. Check logs directory for output."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS utests_aiebaremetal
    COMMENT "Running utests_aiebaremetal with logging"
)

add_custom_target(run_all_tests
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E echo "Running all tests with logging..."
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running utests_external -c ==="
    COMMAND bash -c "echo -e 'y\\ny' | ./utests_external -c 2>&1 | tee logs/utests_external_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running utests_profiling ==="
    COMMAND bash -c "./utests_profiling 2>&1 | tee logs/utests_profiling_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running utests_fal_util ==="
    COMMAND bash -c "./utests_fal_util 2>&1 | tee logs/utests_fal_util_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running utests_submit_command ==="
    COMMAND bash -c "./utests_submit_command 2>&1 | tee logs/utests_submit_command_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running utests_cert_elf ==="
    COMMAND bash -c "./utests_cert_elf 2>&1 | tee logs/utests_cert_elf_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running utests_cert_host ==="
    COMMAND bash -c "./utests_cert_host 2>&1 | tee logs/utests_cert_host_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running utests_gmioapiimpl ==="
    COMMAND bash -c "./utests_gmioapiimpl 2>&1 | tee logs/utests_gmioapiimpl_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running utests_aeg_configure ==="
    COMMAND bash -c "./utests_aeg_configure 2>&1 | tee logs/utests_aeg_configure_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running utests_aiebaremetal ==="
    COMMAND bash -c "./utests_aiebaremetal 2>&1 | tee logs/utests_aiebaremetal_$$(date +%Y%m%d_%H%M%S).log || true"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "All tests completed. Logs saved in logs/ directory"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS utests_external utests_profiling utests_fal_util utests_submit_command utests_cert_elf utests_cert_host utests_gmioapiimpl utests_aeg_configure utests_aiebaremetal
    COMMENT "Running all tests with automated input and logging"
)
