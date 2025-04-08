/**
* Copyright (C) 2025 Advanced Micro Devices, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

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
