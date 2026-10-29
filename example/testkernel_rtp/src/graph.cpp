// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "aiebaremetal.h"
#include "aeg_platform.h"
#if defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf;
#endif

#include <fstream>
#include <xaiengine.h>

AbrGraph gr("gradf");

#if defined(__ADF_FRONTEND__)
void adf_run() {
    int32_t *inputArray = (int32_t *)GMIO::malloc(256 * sizeof(int32_t));
    int32_t *outputArray = (int32_t *)GMIO::malloc(256 * sizeof(int32_t));

    for (int i = 0; i < 256; i++)
        inputArray[i] = i + 1;

    std::ofstream ofs;

    gradf.run(1);
    for (int i = 0; i < 1; i++) {
        gradf.in.gm2aie_nb(&inputArray[i * 32], 32 * sizeof(int32_t));
        gradf.out.aie2gm_nb(&outputArray[i * 32], 32 * sizeof(int32_t));
    }
    gradf.out.wait(); // assuming data from gm1 are processed by the graph and output to gm2

    for (int i = 0; i < 256; i++) {
        std::cout << outputArray[i] << ", ";
    }
    std::cout << "before end" << std::endl;
    gradf.end();

    GMIO::free(inputArray);
    GMIO::free(outputArray);
}
#endif
int main(int argc, char **argv) {
#if defined(AEG_HARDWARE)
    Xil_DCacheDisable();
    Xil_ICacheDisable();
#endif

    XTime tStart, tEnd;
    XTime_GetTime(&tStart);
	gr.init();

    int param = 5;
    int32_t *inputArray = (int32_t *)gr.malloc(256 * sizeof(int32_t));
    int32_t *outputArray = (int32_t *)gr.malloc(256 * sizeof(int32_t));
    int32_t *outputArray_gold = (int32_t *)gr.malloc(256 * sizeof(int32_t));

    for (int i = 0; i < 64; i++) {
        inputArray[i] = i + 1;
        outputArray_gold[i] = inputArray[i] * inputArray[i] + param;
    }

    if (gr.update("gradf.sq.in[1]", reinterpret_cast<char *>(&param), sizeof(int)) != aeg_err::ok) {
        std::cout << "Test Failed: update\n";
        return -1;
    }

    gr.run(2);
    std::cout << "before:run---" << std::endl;

    for (int i = 0; i < 2; i++) {
        gr.gm2aie_nb("gradf.in", reinterpret_cast<char *>(&inputArray[i * 32]), 32 * sizeof(int32_t));
        gr.aie2gm_nb("gradf.out", reinterpret_cast<char *>(&outputArray[i * 32]), 32 * sizeof(int32_t));
        gr.gmio_wait("gradf.out");
    }
    gr.wait();
    XTime_GetTime(&tEnd);
    printf("Output took %llu clock cycles.\n", 2 * (tEnd - tStart));
    printf("Output took %.2f us.\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND / 1000000));
    std::cout << "6. run---" << std::endl;

    std::cout << "[debug] param=" << param << " gold[32]=" << outputArray_gold[32]
              << " gold[63]=" << outputArray_gold[63] << std::endl;
    std::cout << "[debug] block boundaries: ";
    for (int b = 0; b < 2; b++)
        std::cout << "[" << b << "] " << outputArray[b * 32] << "," << outputArray[b * 32 + 31] << "  ";
    std::cout << std::endl;

    for (int i = 0; i < 64; i++) {
        std::cout << outputArray[i] << ", ";
        if (outputArray[i] != outputArray_gold[i]) {
            std::cout << "\n" << "Test Failed \n";
            return -1;
        }
    }

    std::cout << "\n" << "Test Passed\n";
    return 0;
}
