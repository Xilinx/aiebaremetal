#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# Script to run all testcpputest executables with logging (matches CMake add_executable targets).
# Usage: run from the CMake build directory (same folder as the binaries), e.g.:
#   cd build && bash ../script_files/run_tests_with_logs.sh

# Create logs directory if it doesn't exist
mkdir -p logs

# Get timestamp for log files
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Consolidated log file
CONSOLIDATED_LOG="logs/all_tests_${TIMESTAMP}.log"

run_one() {
	local name="$1"
	shift
	echo "Running ${name}..."
	"$@" 2>&1 | tee "logs/${name}_${TIMESTAMP}.log"
	echo "${name} completed. Log saved to logs/${name}_${TIMESTAMP}.log"
	echo ""
}

run_one utests_external bash -c 'echo -e "y\ny" | ./utests_external -c'
run_one utests_fal_util ./utests_fal_util
run_one utests_profiling ./utests_profiling
run_one utests_submit_command ./utests_submit_command
run_one utests_cert_elf ./utests_cert_elf
run_one utests_cert_host ./utests_cert_host
run_one utests_gmioapiimpl ./utests_gmioapiimpl
run_one utests_aeg_configure ./utests_aeg_configure
run_one utests_aiebaremetal ./utests_aiebaremetal

# Create consolidated log file from individual logs
echo "Creating consolidated log file..."
{
	echo "========================================"
	echo "AIE Baremetal Unit Tests - Full Results"
	echo "========================================"
	echo ""

	n=1
	for t in utests_external utests_fal_util utests_profiling utests_submit_command utests_cert_elf utests_cert_host utests_gmioapiimpl utests_aeg_configure utests_aiebaremetal; do
		echo "=========================================="
		echo "TEST ${n}: ${t}"
		echo "=========================================="
		if [[ -f "logs/${t}_${TIMESTAMP}.log" ]]; then
			cat "logs/${t}_${TIMESTAMP}.log"
		else
			echo "(missing log: logs/${t}_${TIMESTAMP}.log)"
		fi
		echo ""
		echo ""
		n=$((n + 1))
	done

	echo "========================================"
	echo "All tests completed!"
	echo "========================================"
} >"${CONSOLIDATED_LOG}"

echo "All tests completed. Logs saved in logs/ directory"
echo "Consolidated log: ${CONSOLIDATED_LOG}"
