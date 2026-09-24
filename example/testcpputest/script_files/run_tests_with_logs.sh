#!/bin/bash

# Script to run tests with automated input and logging
# Usage: ./run_tests_with_logs.sh

# Create logs directory if it doesn't exist
mkdir -p logs

# Get timestamp for log files
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Consolidated log file
CONSOLIDATED_LOG="logs/all_tests_${TIMESTAMP}.log"

echo "Running utests -c..."
echo -e "y\ny" | ./utests -c 2>&1 | tee logs/utests_${TIMESTAMP}.log
echo "utests completed. Log saved to logs/utests_${TIMESTAMP}.log"
echo ""

echo "Running ut_profiling..."
./ut_profiling 2>&1 | tee logs/ut_profiling_${TIMESTAMP}.log
echo "ut_profiling completed. Log saved to logs/ut_profiling_${TIMESTAMP}.log"
echo ""

echo "Running test_fal_util..."
./test_fal_util 2>&1 | tee logs/test_fal_util_${TIMESTAMP}.log
echo "test_fal_util completed. Log saved to logs/test_fal_util_${TIMESTAMP}.log"
echo ""

echo "Running test_submit_command..."
./test_submit_command 2>&1 | tee logs/test_submit_command_${TIMESTAMP}.log
echo "test_submit_command completed. Log saved to logs/test_submit_command_${TIMESTAMP}.log"
echo ""

# Create consolidated log file from individual logs
echo "Creating consolidated log file..."
{
    echo "========================================"
    echo "AIE Baremetal Unit Tests - Full Results"
    echo "========================================"
    echo ""

    echo "=========================================="
    echo "TEST 1: utests -c"
    echo "=========================================="
    cat logs/utests_${TIMESTAMP}.log
    echo ""
    echo ""

    echo "=========================================="
    echo "TEST 2: ut_profiling"
    echo "=========================================="
    cat logs/ut_profiling_${TIMESTAMP}.log
    echo ""
    echo ""

    echo "=========================================="
    echo "TEST 3: test_fal_util"
    echo "=========================================="
    cat logs/test_fal_util_${TIMESTAMP}.log
    echo ""
    echo ""

    echo "=========================================="
    echo "TEST 4: test_submit_command"
    echo "=========================================="
    cat logs/test_submit_command_${TIMESTAMP}.log
    echo ""
    echo ""

    echo "========================================"
    echo "All tests completed!"
    echo "========================================"
} > ${CONSOLIDATED_LOG}

echo "All tests completed. Logs saved in logs/ directory"
echo "Consolidated log: ${CONSOLIDATED_LOG}"