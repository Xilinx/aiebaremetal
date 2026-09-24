/**
 * @file main.cpp
 * @brief Main entry point for CppUTest test runner
 *
 * This file contains the main function that initializes and runs all unit tests
 * using the CppUTest framework. It handles memory leak detection configuration
 * and test execution.
 *
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 */

#include "CppUTest/CommandLineTestRunner.h"
#include <filesystem>
#include <iostream>

/**
 * @brief Main entry point for CppUTest test runner
 *
 * This function initializes the test environment, optionally cleans up
 * old coverage files from previous test runs, and executes all registered
 * unit tests using CppUTest framework.
 *
 * The function attempts to locate and clean coverage files (.gcda, .gcno)
 * based on the current working directory. It expects to be run from either:
 * - testcpputest/build directory
 * - testcpputest directory
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @return int Exit code from test runner (0 for success, non-zero for failures)
 */
int main(int argc, char** argv) {
    std::string cmakeBinaryDir = "";
    std::filesystem::path currentPath = std::filesystem::current_path();

    // Turn off memory leak detection for new/delete to avoid false positives
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
    std::cout << "Starting unit tests..." << std::endl;
    // Determine the build directory based on current path
    if ((currentPath.filename() == "build"
            && currentPath.parent_path().filename() == "testcpputest"
            && currentPath.parent_path().parent_path().filename() == "example")) {
        cmakeBinaryDir = "CMakeFiles/utests.dir";
    } else if (currentPath.filename() == "testcpputest"
            && currentPath.parent_path().filename() == "example") {
        cmakeBinaryDir = "build/CMakeFiles/utests.dir";
    } else {
        std::cout << "Skipping coverage file cleanup... Run from the testcpputest or "
                  << "testcpputest/build directory so the old coverage files can be found."
                  << std::endl;
    }

    // Run all registered unit tests
    return CommandLineTestRunner::RunAllTests(argc, argv);
}