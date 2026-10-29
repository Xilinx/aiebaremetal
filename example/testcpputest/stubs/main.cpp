// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "CppUTest/CommandLineTestRunner.h"
#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    std::string cmakeBinaryDir = "";
    std::filesystem::path currentPath = std::filesystem::current_path();

    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
    std::cout << "Starting unit tests..." << std::endl;

    if ((currentPath.filename() == "build"
            && currentPath.parent_path().filename() == "testcpputest"
            && currentPath.parent_path().parent_path().filename() == "example")) {
        cmakeBinaryDir = "CMakeFiles/utests_external.dir";
    } else if (currentPath.filename() == "testcpputest"
            && currentPath.parent_path().filename() == "example") {
        cmakeBinaryDir = "build/CMakeFiles/utests_external.dir";
    } else {
        std::cout << "Skipping coverage file cleanup... Run from the testcpputest or "
                  << "testcpputest/build directory so the old coverage files can be found."
                  << std::endl;
    }

    return CommandLineTestRunner::RunAllTests(argc, argv);
}
