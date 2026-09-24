<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# AIE-BAREMETAL Unit Test Framework

## Overview

This directory contains the unit test framework for AIE Baremetal runtime library using CppUTest. The test suite provides comprehensive coverage of:
- AIE Baremetal API (aeg_aiebaremetal, aeg_configure)
- Runtime API (aeg_runtime_api, aeg_api_message)
- GMIO and External Buffer implementations
- CERT host and ELF loader functionality
- `cert_host::cert_api::submit_command` (dedicated `test_submit_command` executable)
- Event generation and barrier management
- Profiling API and FAL utilities

## Prerequisites

- **CMake** (version 3.10 or higher)
- **C++ Compiler** (GCC or Clang with C++11 support)
- **Python 3** (for viewing HTML coverage reports)
- **lcov/genhtml** (for coverage report generation)
- **CppUTest** (automatically downloaded during build)

**Cert ELF for loader tests:** `aiebaremetal/thirdparty/cert-elf-binaries/ctrl_code.elf` is copied next to `utests` in the build directory as `ctrl_code.elf`. Override with `-DTEST_ELF_FILE=...` if needed.

## Directory Structure

```
testcpputest/
├── compile/            # Dependency files
├── script_files/       # Utility scripts
│   ├── run_tests_with_logs.sh    # Script to run all tests with automated logging
│   ├── setup_includes.sh         # Script to setup include paths and dependencies
│   ├── calculate_stack_size.sh   # Script to analyze and report stack usage
│   └── calculate_code_size.sh    # Script to analyze and report code size
├── stubs/              # Mock implementations and test stubs
│   ├── aiert_mock.c
│   ├── baremetal_metadata.cpp
│   ├── main.cpp
│   ├── mock_aie_library.cpp
│   ├── mock_aie_library_no_symbol.cpp
│   ├── other_mock.cpp
│   ├── stubs.cc
│   ├── stubs.h
│   ├── test_comparator.cpp
│   ├── test_comparator.h
│   ├── profiling_stubs/    # Stubs for profiling tests
│   ├── submit_cmd_stubs/   # Stubs for test_submit_command (XAie mem, barrier, cert ELF loader)
│   └── xaiefal/            # FAL utility stubs
├── tests/              # Unit test files
│   ├── test_aeg_api_message.cpp
│   ├── test_aeg_configure.cc
│   ├── test_aeg_profiling_api.cpp
│   ├── test_aiebaremetal.cc
│   ├── test_barrier.cpp
│   ├── test_cert_elf_loader.cpp
│   ├── test_cert_host.cc
│   ├── test_common.cpp
│   ├── test_externalbufferapiimpl.cpp
│   ├── test_fal_util.cpp
│   ├── test_generate_event.cpp
│   ├── test_gmioapiimpl.cpp
│   ├── test_graph_api_run.cpp
│   ├── test_submit_command.cpp   # cert_api::submit_command (standalone binary)
│   └── test_xaiemem.cpp
├── CMakeLists.txt      # CMake build configuration
└── README.md           # This file
```

## Test Executables

The test suite builds **four** separate executables:

1. **utests** - Main test executable covering:
   - AIE Baremetal core functionality
   - Runtime API and message handling
   - GMIO and External Buffer APIs
   - CERT host and ELF loader
   - Event generation and barriers

2. **ut_profiling** - Profiling API tests:
   - Profiling API functionality
   - Event profiling features

3. **test_fal_util** - FAL utility tests:
   - FAL utility functions
   - Helper functions

4. **test_submit_command** - Focused tests for `cert_host::cert_api::submit_command`:
   - Control-code submission paths (single column, 2–24 columns, more than 24 columns)
   - Input validation, allocation failures, external buffer maps, completion signaling
   - Uses `stubs/submit_cmd_stubs/` (no hardware); separate binary so it can link a minimal stub set without pulling in the full `utests` object closure

## Utility Scripts

### script_files/run_tests_with_logs.sh
Automated script to run all test executables with logging.

**Usage:**
```bash
cd build
./run_tests_with_logs.sh
```

**Features:**
- Runs all four test executables (`utests -c`, `ut_profiling`, `test_fal_util`, `test_submit_command`)
- Provides automated input (`y\ny`) for `utests -c`
- Saves logs to `build/logs/` with timestamps
- Displays test execution status

### script_files/setup_includes.sh
Script to setup include paths and dependencies for the build process.

**Usage:**
Automatically invoked during CMake configuration:
```bash
cmake ..
```

**Purpose:**
- Configures include paths for test compilation
- Sets up dependencies for test execution

### script_files/calculate_stack_size.sh
Script to analyze and report stack usage of compiled test executables.

**Usage:**
```bash
../script_files/calculate_stack_size.sh
```

**Example:**
```bash
# Analyzes .su files corresponding to .o files (both must exist)
../script_files/calculate_stack_size.sh CMakeFiles/utests.dir/tests/test_aeg_configure.cc.o CMakeFiles/utests.dir/tests/test_aeg_configure.cc.su

# Output shows stack usage per function:
# File: test_aeg_configure.cc.o
# Stack Usage File: test_aeg_configure.cc.su
# Function: test_function_name
# Stack Usage: 256 bytes
```

**Purpose:**
- Parses `.su` files generated during compilation (when `-fstack-usage` flag is enabled)
- Calculates stack usage for each function
- Generates stack usage report
- Identifies functions with high stack consumption

**Note:** Requires `-DENABLE_STACK_USAGE=ON` during cmake configuration (enabled by default)

### script_files/calculate_code_size.sh
Script to analyze and report code size metrics of compiled test executables.

**Usage:**
```bash
../script_files/calculate_code_size.sh
```

**Example:**
```bash
# Analyzes .o files for code size
../script_files/calculate_code_size.sh CMakeFiles/utests.dir/tests/test_aeg_configure.cc.o

# Output shows size breakdown:
# File: test_aeg_configure.cc.o
# Text (code): 4096 bytes
# Data: 512 bytes
# BSS: 128 bytes
# Total: 4736 bytes
```

**Purpose:**
- Analyzes binary size of test executables and object files
- Reports text, data, and BSS segment sizes
- Provides total memory footprint information
- Useful for tracking code size optimization

## Build and Run Tests + Generate and View Coverage Reports

```bash
cd example/testcpputest

# This compiles and generates the runtime libarary and its dependencies
cd compile
source build.sh 1
cd ../

mkdir -p build
cd build

cmake ..
make -j32

# Run tests with automated input and logging

# Option 1: Use the shell script (recommended - runs all tests)
./run_tests_with_logs.sh     # Runs utests -c (with automated y/y), ut_profiling, test_fal_util, test_submit_command
                             # Logs saved in logs/ directory with timestamps

# Option 2: Run individual tests manually
echo -e "y\ny" | ./utests -c    # Only utests -c requires y/y input
./ut_profiling
./test_fal_util
./test_submit_command           # No extra stdin required

# Generates the html and text reports
make coverage

# View HTML report located in build/coverage_report
cd coverage_report

# Choose an open port
python3 -m http.server 9090

# In your local browser open http://localhost:9090
# You should see the full html report

# If it does not work then you may need to run the line below first
# replace user@remote_machine with correct values

# ssh -L 9090:localhost:9090 user@remote_machine

# View text report (not recommended)
# vim coverage.txt
```

## CMake Targets

### Build Targets
- `make` - Build all test executables
- `make utests` - Build main test executable
- `make ut_profiling` - Build profiling test executable
- `make test_fal_util` - Build FAL utility test executable
- `make test_submit_command` - Build `submit_command` unit tests
- `make clean` - Clean build artifacts

### Coverage Targets
- `make coverage` - Generate coverage report (HTML and text)
- `make cleancov` - Clean coverage data files
- `make fullclean` - Full clean including coverage data

## Cleaning and Rebuilding

```bash
# From build directory
cd build

# Clean build artifacts only
make clean

# Clean coverage data only
make cleancov

# Full clean (build artifacts + coverage data)
make fullclean

# Rebuild after cleaning
make -j32

# Complete rebuild from scratch (delete everything and reconfigure)
cd ..
rm -rf build
mkdir -p build
cd build
cmake ..
make -j32
```

## Coverage Report

After running tests and generating coverage, you can find:
- **HTML Report**: `build/coverage_report/` (view in browser)
- **Text Report**: `build/coverage_report.txt`
- **Markdown Report**: `build/API_COVERAGE.md` (formatted coverage summary)

## Test Logs

Test execution logs are automatically saved in `build/logs/` with timestamps:
- `utests_YYYYMMDD_HHMMSS.log`
- `ut_profiling_YYYYMMDD_HHMMSS.log`
- `test_fal_util_YYYYMMDD_HHMMSS.log`
- `test_submit_command_YYYYMMDD_HHMMSS.log`

## Notes

- The `utests` executable requires automated input (`y\ny`) for certain test scenarios; `test_submit_command` does not
- Coverage is enabled by default. Use `-DENABLE_COVERAGE=OFF` to disable
- Stack usage analysis is enabled by default. Use `-DENABLE_STACK_USAGE=OFF` to disable
- CppUTest is automatically downloaded and built from GitHub during the first build
