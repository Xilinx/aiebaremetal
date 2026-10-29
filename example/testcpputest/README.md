<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# AIE-BAREMETAL Unit Test Framework

## Overview

This directory contains the unit test framework for AIE Baremetal runtime library using CppUTest. The test suite provides comprehensive coverage of:
- Runtime API (`aeg_runtime_api`, `aeg_api_message`) and related component tests in **`utests_external`**
- GMIO and external buffer implementations (**`utests_gmioapiimpl`**)
- CERT host and ELF loader (**`utests_cert_elf`**, **`utests_cert_host`**, **`utests_submit_command`**)
- Configure parser (**`utests_aeg_configure`**) and AIEGraph / baremetal API (**`utests_aiebaremetal`**)
- Event generation and barrier management (**`utests_external`**)
- Profiling API and FAL utilities (**`utests_profiling`**, **`utests_fal_util`**)

## Prerequisites

- **CMake** (version 3.10 or higher)
- **C++ Compiler** (GCC or Clang with C++11 support)
- **Python 3** (for viewing HTML coverage reports)
- **lcov/genhtml** (for coverage report generation)
- **CppUTest** (automatically downloaded during build)

**Cert ELF for loader tests:** `aiebaremetal/thirdparty/cert-elf-binaries/ctrl_code.elf` is copied next to `utests_external` and `utests_cert_elf` in the build directory as `ctrl_code.elf` (see `CMakeLists.txt` `POST_BUILD` rules).

## Directory Structure

```
testcpputest/
├── compile/            # Dependency build (build.sh + src/) for the runtime library
├── script_files/       # Utility scripts
│   ├── run_tests_with_logs.sh        # Run all nine test executables with timestamped logs
│   ├── setup_includes.sh             # Setup include paths / dependencies during configure
│   ├── calculate_stack_size.sh       # Analyze stack usage from .su files (manual tool)
│   ├── calculate_code_size.sh        # Analyze code size from .o files (manual tool)
│   ├── exclude_lines_from_coverage.sh # Strip specified line ranges from lcov .info (used by `make coverage`)
│   ├── generate_coverage_md.sh       # Convert lcov text report into API_COVERAGE.md (used by `make coverage`)
│   └── analyse_real_gaps.sh          # Manual tool: list testable, still-uncovered branches in an lcov .info
├── stubs/              # Mock implementations and test stubs
│   ├── CMakeLists.txt              # Reference / alternate CMake (not used by top-level example/testcpputest/CMakeLists.txt)
│   ├── main.cpp                    # CppUTest runner entry
│   ├── stubs.cc / stubs.h          # XAie tile-type / wakeup / GMIO–graph UT globals and helpers
│   ├── aiert_mock.c                # XAie API mocks for tests
│   ├── baremetal_metadata.cpp      # Embedded _binary_build_* config blobs for parser tests
│   ├── mock_baremetal_metadata.cpp # Orphan: alternate empty-metadata variant, not compiled by any target
│   ├── mock_aie_library.cpp        # Builds mock_aie_library.so (dlopen success path); not exercised by active tests
│   ├── mock_aie_library_no_symbol.cpp # Builds mock_aie_library_no_symbol.so (dlsym failure path); not exercised by active tests
│   ├── other_mock.cpp              # Profiling + shim_config stubs; cert_elf when not using stub_cert_elf_loader defs
│   ├── test_comparator.cpp / test_comparator.h
│   ├── aeg_configure_mock.cpp      # ConfigureParser + get_partition_start_column (replaces aeg_configure.cc on UT path)
│   ├── cert_host_mock.cpp          # cert_host::cert_api (replaces cert_host.cc on UT path)
│   ├── gmio_api_mock.cpp           # abr::gmio_api / dma_api for tests without full aeg_runtime_api.cpp
│   ├── graph_api_mock.cpp          # Minimal abr::graph_api + config_manager
│   ├── xaie_generate_events_mock.cpp  # generate_events_on_col etc. for utests_aiebaremetal
│   ├── aiebaremetal_configure/     # aeg_configure.h header used by aeg_configure_mock.cpp (BEFORE include)
│   ├── common_layer/               # Header overrides for common_layer (aeg_aie_control_api.h, aeg_runtime_api.h)
│   ├── drivers/aiengine/           # XAie driver header overrides
│   ├── fake_elfio/                 # Minimal ELFIO-shaped headers for cert_elf_loader UTs (<elfio/elfio.hpp>)
│   ├── fake_klogr/log/             # klogr/kcommon header overrides for utests_aiebaremetal
│   ├── fal_util_mock/              # fal_util.h header injected via `-include` for utests_aiebaremetal
│   ├── gmio_shim/                  # xaiemem.h header injected via `-include` for utests_aiebaremetal
│   ├── profiling_stubs/            # Used by utests_profiling (fal_stubs, xaiengine, xaiefal_mock)
│   ├── submit_cmd_stubs/           # Used by utests_submit_command (stub_xaie_mem, barrier, stub_cert_elf_loader, xaiengine)
│   └── xaiefal/                    # FAL / XAie headers for test_fal_util and profiling stubs
├── tests/              # Unit test files
│   ├── test_aeg_api_message.cpp
│   ├── test_aeg_configure.cc
│   ├── test_aeg_profiling_api.cpp
│   ├── test_aiebaremetal.cc
│   ├── test_aiebaremetal_api.cpp     # Orphan: 1371-line standalone BaremetalGraphSim test; not built by any target
│   ├── test_barrier.cpp
│   ├── test_cert_elf_loader.cpp
│   ├── test_cert_host.cc
│   ├── test_common.cpp               # Orphan: single TEST for gdb_point; not built by any target
│   ├── test_externalbufferapiimpl.cpp
│   ├── test_fal_util.cpp
│   ├── test_generate_event.cpp
│   ├── test_gmioapiimpl.cpp
│   ├── test_graph_api_run.cpp
│   ├── test_submit_command.cpp       # sources for utests_submit_command
│   └── test_xaiemem.cpp
├── API_COVERAGE.md     # Latest coverage summary written by `make coverage`
├── CMakeLists.txt      # CMake build configuration
└── README.md           # This file
```

## Test Executables

The test suite builds **nine** separate executables (see `CMakeLists.txt`):

1. **utests_external** — Main CppUTest driver: `test_graph_api_run`, `test_xaiemem`, `test_barrier`, `test_aeg_api_message`, `test_generate_event`; links `aeg_runtime_api.cpp` and `xaie_generate_events.cc` (not the full AIEGraph binary).

2. **utests_fal_util** — FAL utility tests (`fal_util.cpp` UUT).

3. **utests_profiling** — Profiling API tests (`aeg_profiling_api.cpp` + profiling stubs).

4. **utests_submit_command** — `cert_host::cert_api::submit_command` and related paths; links production `cert_host.cc` with `stubs/submit_cmd_stubs/`.

5. **utests_cert_elf** — `cert_elf_loader.cc` with `test_cert_elf_loader.cpp`.

6. **utests_cert_host** — `test_cert_host.cc` + production `cert_host.cc`, XAie mocks, `stub_cert_elf_loader.cpp`.

7. **utests_gmioapiimpl** — `test_gmioapiimpl.cpp`, `test_externalbufferapiimpl.cpp`; `gmio_api_mock.cpp`, `graph_api_mock.cpp`, `other_mock.cpp`, etc.

8. **utests_aeg_configure** — `test_aeg_configure.cc` + production `aeg_configure.cc` (not the mock parser).

9. **utests_aiebaremetal** — `test_aiebaremetal.cc` + `aeg_aiebaremetal.cc` with UT stubs (`aeg_configure_mock.cpp`, `cert_host_mock.cpp`, `xaie_generate_events_mock.cpp`, `gmio_api_mock.cpp`, `graph_api_mock.cpp`, `stub_cert_elf_loader.cpp`, `other_mock.cpp`, …).

## Utility Scripts

### script_files/run_tests_with_logs.sh
Automated script to run **all** test executables with logging (same set as `make coverage` depends on).

**Usage (from the CMake build directory):**
```bash
cd build
./run_tests_with_logs.sh
# Or, if the script was not copied into build yet:
bash ../script_files/run_tests_with_logs.sh
```

After a successful `make utests_external`, CMake copies `run_tests_with_logs.sh` into the build directory as `./run_tests_with_logs.sh`.

**Features:**
- Runs, in order: `utests_external -c`, `utests_fal_util`, `utests_profiling`, `utests_submit_command`, `utests_cert_elf`, `utests_cert_host`, `utests_gmioapiimpl`, `utests_aeg_configure`, `utests_aiebaremetal`
- Provides automated input (`y\ny`) for `utests_external -c` only
- Writes per-binary logs under `build/logs/<name>_YYYYMMDD_HHMMSS.log`
- Builds `build/logs/all_tests_YYYYMMDD_HHMMSS.log` (concatenated transcript)

**CMake equivalent:** `make run_all_tests` runs the same binaries from the build tree with `tee` to `logs/` (see `CMakeLists.txt` target `run_all_tests`).

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
../script_files/calculate_stack_size.sh CMakeFiles/utests_aeg_configure.dir/tests/test_aeg_configure.cc.o CMakeFiles/utests_aeg_configure.dir/tests/test_aeg_configure.cc.su

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
../script_files/calculate_code_size.sh CMakeFiles/utests_aeg_configure.dir/tests/test_aeg_configure.cc.o

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

### script_files/exclude_lines_from_coverage.sh
Strips specified `<file>:<startLine>-<endLine>` ranges from an lcov `.info` tracefile so that intentionally-uncoverable code (e.g. simulator-only branches) does not skew the report.

**Usage (invoked automatically by `make coverage`):**
```bash
bash script_files/exclude_lines_from_coverage.sh \
    coverage.info.cleaned \
    aeg_aiebaremetal.cc:2002-2192 \
    aeg_aiebaremetal.cc:875-886
```

### script_files/generate_coverage_md.sh
Converts the `lcov --list` text report into a markdown summary + per-file detailed table, written as `API_COVERAGE.md`. Pure POSIX shell + awk (no Python dependency).

**Usage (invoked automatically by `make coverage`):**
```bash
bash script_files/generate_coverage_md.sh coverage_report.txt API_COVERAGE.md
```

`make coverage` runs this and then `copy_if_different`'s the resulting `API_COVERAGE.md` back into the source tree so the latest summary is committed alongside the code.

### script_files/analyse_real_gaps.sh
Manual diagnostic tool. Parses an lcov `.info` tracefile and lists the genuinely-testable branches (BRDA records) that remain uncovered, filtering out compiler-injected exception/RTTI branches.

**Usage:**
```bash
bash script_files/analyse_real_gaps.sh build/coverage.info.cleaned
```

Not invoked by the build — useful when chasing real coverage gaps.

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

# Run ALL test binaries — REQUIRED after every rebuild, before `make coverage`.
# `make coverage` does NOT run the tests; it only captures `.gcda` files left
# behind by the previous test run. See "Coverage Workflow & Pitfalls" below.

# Option 1: Use the shell script (recommended — runs all nine executables)
./run_tests_with_logs.sh
# Or: bash ../script_files/run_tests_with_logs.sh
# Logs: logs/<binary>_*.log and logs/all_tests_*.log

# Option 2: CMake driver (same binaries, tee to logs/)
make run_all_tests

# Option 3: Run individual tests manually (must run ALL nine for a complete report)
echo -e "y\ny" | ./utests_external -c    # Only utests_external -c requires y/y for coverage prompts
./utests_fal_util
./utests_profiling
./utests_submit_command
./utests_cert_elf
./utests_cert_host
./utests_gmioapiimpl
./utests_aeg_configure
./utests_aiebaremetal

# Generates the html and text reports (capture only — must run tests first)
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
- `make` — Build all test executables
- `make utests_external` — Main test driver (graph API, xaiemem, barrier, api_message, generate_event)
- `make utests_fal_util` — FAL utility tests
- `make utests_profiling` — Profiling API tests
- `make utests_submit_command` — `cert_host::submit_command` tests
- `make utests_cert_elf` — Cert ELF loader tests
- `make utests_cert_host` — Cert host tests
- `make utests_gmioapiimpl` — GMIO / external buffer API impl tests
- `make utests_aeg_configure` — Configure parser tests (production `aeg_configure.cc`)
- `make utests_aiebaremetal` — AIEGraph / baremetal API tests (stub configure + mocks)
- `make clean` — Clean build artifacts

### Run Targets (from `build/`, logs under `logs/`)
- `make run_utests_external` — `utests_external -c` only
- `make run_utests_profiling` — `utests_profiling` only
- `make run_utests_fal_util` — `utests_fal_util` only
- `make run_all_tests` — All nine binaries (same coverage set as `make coverage` `DEPENDS`)

### Coverage Targets
- `make coverage` — Generate coverage report (HTML and text). Depends only on the nine test binaries being **built**, not run. You MUST run all binaries (`make run_all_tests` or `./run_tests_with_logs.sh`) AFTER any rebuild and BEFORE invoking `make coverage`. See "Coverage Workflow & Pitfalls" below.
- `make cleancov` - Clean coverage data files
- `make fullclean` - Full clean including coverage data

## Coverage Workflow & Pitfalls

### Required order (every time)

```bash
make -j32              # 1. build (or rebuild) all binaries
make run_all_tests     # 2. run all nine binaries — REQUIRED after any rebuild
make coverage          # 3. capture + generate HTML/text/MD reports
```

`make coverage` itself only **builds** the test binaries; it does not run them. lcov captures whatever `.gcda` files are on disk at that moment. If a binary was rebuilt and not re-run, its `.gcda` is missing and lcov has nothing to merge for that translation unit.

### Pitfall: file shows 0% even though its tests pass

Several source files are compiled into **multiple** test binaries but are exercised by only **one** of them. For example:

| Source file | Compiled into | Tests live in |
|---|---|---|
| `common_layer/aeg_api_message.cpp` | `utests_external`, `utests_gmioapiimpl`, `utests_aeg_configure`, `utests_aiebaremetal` | `utests_external` only |
| `common_layer/aeg_runtime_api.cpp` | `utests_external`, plus others | `utests_external` only |

Each binary produces its own `.gcda` file under `build/CMakeFiles/<binary>.dir/.../<src>.gcda`. The three binaries that do NOT call the functions write `.gcda` files with all-zero counters. lcov merges all four `.gcda` files per source path; if the *one* binary that has real coverage data was rebuilt without being re-run, its `.gcda` is absent and the merge collapses to 0%.

**Symptom:** a file shows `0.0%` lines / `0.0%` functions in `API_COVERAGE.md` even though `tests/test_<file>.cpp` passes.

**Fix:** always run `make run_all_tests` after any rebuild, before `make coverage`. If unsure, run `make cleancov && make -j32 && make run_all_tests && make coverage` for a clean cycle.

### Quick sanity check

To confirm a binary's `.gcda` exists and is fresh:

```bash
ls -la build/CMakeFiles/utests_external.dir/scratch/.../<source>.cpp.gcda
```

If the file is missing or older than the corresponding binary, re-run `make run_all_tests` before `make coverage`.

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
- **Markdown Report**: `build/API_COVERAGE.md` (formatted coverage summary); also copied back to the source tree as `testcpputest/API_COVERAGE.md` so the latest summary lives next to the code.

## Test Logs

When using `run_tests_with_logs.sh` or `make run_all_tests`, logs are written under `build/logs/` with timestamps, for example:

- `utests_external_*.log`, `utests_fal_util_*.log`, `utests_profiling_*.log`, `utests_submit_command_*.log`
- `utests_cert_elf_*.log`, `utests_cert_host_*.log`, `utests_gmioapiimpl_*.log`
- `utests_aeg_configure_*.log`, `utests_aiebaremetal_*.log`
- `all_tests_*.log` — single consolidated file (shell script only)

## Notes

- Only **`utests_external -c`** needs automated stdin (`y\ny`) for coverage prompts; the other eight executables do not
- Coverage is enabled by default. Use `-DENABLE_COVERAGE=OFF` to disable
- Stack usage analysis is enabled by default. Use `-DENABLE_STACK_USAGE=OFF` to disable
- CppUTest is automatically downloaded and built from GitHub during the first build
