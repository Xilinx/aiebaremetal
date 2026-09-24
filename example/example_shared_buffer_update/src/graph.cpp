// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#define NUM_SAMPLES 32

#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "graph.h"
MyGraph gradf;
#else
#include "xil_io.h"
#include "xil_printf.h"
#endif

#include "aiebaremetal.h"

AbrGraph g("gradf");

int main() {
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
    gradf.init();
#endif

    printf("init()\n");
    g.init();
    g.run(1);

    int32_t *inputArray = (int32_t *)g.malloc(NUM_SAMPLES * sizeof(int32_t));
    int32_t *outputArray = (int32_t *)g.malloc(NUM_SAMPLES * sizeof(int32_t));
    int32_t *outputArray_gold = (int32_t *)g.malloc(NUM_SAMPLES * sizeof(int32_t));

    for (int i = 0; i < NUM_SAMPLES; i++) {
        inputArray[i] = 2;
        outputArray[i] = 0;
        outputArray_gold[i] = 300;
    }

    g.gm2aie_nb("gradf.in", reinterpret_cast<char *>(&inputArray[0]), NUM_SAMPLES * sizeof(int32_t));
    g.aie2gm_nb("gradf.out", reinterpret_cast<char *>(&outputArray[0]), NUM_SAMPLES * sizeof(int32_t));
    int data[NUM_SAMPLES];
    for (int i = 0; i < NUM_SAMPLES; i++)
        data[i] = 150;
    g.update("gradf.wts", data, NUM_SAMPLES * sizeof(int32_t)); // update API similar to RTP ports
    std::cout << "before gmio_wait ---" << std::endl;
    g.gmio_wait("gradf.out"); // assuming data from gm1 are processed by the graph and output to gm2
    printf("after gmio_wait\n");
    g.wait();
    g.end();

    for (int i = 0; i < NUM_SAMPLES; i++) {
        std::cout << "value " << outputArray[i] << ", ";
        if (outputArray[i] != outputArray_gold[i]) {
            std::cout << "\n at data[" << i << "] incorrect Test Failed \n";
            return -1;
        }
    }

    std::cout << "\n" << "Test Passed\n";
    return 0;
}