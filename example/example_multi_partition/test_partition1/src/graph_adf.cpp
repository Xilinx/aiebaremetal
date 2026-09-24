// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "graph.h"
myGraph gradf;

#include <fstream>
// #include <xaiengine.h>
// #include "aiebaremetal.h"
#include "config.h"

using namespace std;
using namespace adf;

#define DATA_LENGTH 256

int main(int argc, char **argv) {
    printf("adf application\n");
    gradf.init();

    // gr.getaiedevinst();
    int32_t *inputArray = (int32_t *)GMIO::malloc(DATA_LENGTH * sizeof(int32_t));
    int32_t *outputArray = (int32_t *)GMIO::malloc(DATA_LENGTH * sizeof(int32_t));
    int32_t *outputArray_gold = (int32_t *)GMIO::malloc(DATA_LENGTH * sizeof(int32_t));

    gradf.run(1);

    for (int i = 0; i < DATA_LENGTH; i++) {
        inputArray[i] = i + 1;
        outputArray_gold[i] = inputArray[i] * inputArray[i];
    }
    gradf.in.gm2aie_nb(&inputArray[0], DATA_LENGTH * sizeof(int32_t));
    printf("*****gm2aie******\n");
    gradf.out.aie2gm_nb(&outputArray[0], DATA_LENGTH * sizeof(int32_t));

    std::cout << "before gmio_wait ---" << std::endl;
    gradf.out.wait(); // assuming data from gm1 are processed by the graph and output to gm2
    printf("3");
    gradf.wait();
    gradf.end();

    for (int i = 0; i < DATA_LENGTH; i++) {
        std::cout << "value " << outputArray[i] << ", ";
        if (outputArray[i] != outputArray_gold[i]) {
            std::cout << "\n at data[" << i << "] incorrect Test Failed \n";
            return -1;
        }
    }

    std::cout << "\n" << "Test Passed\n";

    GMIO::free(inputArray);
    GMIO::free(outputArray);
    GMIO::free(outputArray_gold);
    return 0;
}
