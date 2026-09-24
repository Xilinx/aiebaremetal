/**<!--
 * Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
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
 * -->
 */

#include "graph.h"
#include <fstream>

myGraph gradf;
void adf_run() {
    int32_t *inputArray = (int32_t *)GMIO::malloc(256 * sizeof(int32_t));
    int32_t *outputArray = (int32_t *)GMIO::malloc(256 * sizeof(int32_t));
    printf("1\n");
    for (int i = 0; i < 256; i++)
        inputArray[i] = i + 1;

    std::ofstream ofs;

    gradf.init();
    gradf.run(1);
    for (int i = 0; i < 1; i++) {
        gradf.in.gm2aie_nb(inputArray, 256 * sizeof(int32_t));
        gradf.out.aie2gm_nb(outputArray, 256 * sizeof(int32_t));
    }
    gradf.out.wait(); // assuming data from gm1 are processed by the graph and output to gm2

    for (int i = 0; i < 256; i++) {
        std::cout << outputArray[i] << " " << outputArray[i] << std::endl;
    }
    std::cout << "end" << std::endl;
    gradf.end();

    GMIO::free(inputArray);
    GMIO::free(outputArray);
}

int main(int argc, char **argv) {
    printf("test\n");
    adf_run();
    return 0;
}
