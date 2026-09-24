// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <iostream>
#include <vector>
extern "C" {
int gradf_init(const std::string& work_path) {
	printf("gradf_init\n");
	return 0;
}
bool gradf_load_elf(const std::string& work_path, std::vector<std::string>& elfInfoPath) {
	printf("gradf_load_elf\n");
	return 0;
}
}
