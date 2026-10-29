// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "graph.h"
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xtime_l.h"
#include <fstream>

myGraph gradf;
void adf_run() {
    int32_t *inputArray = (int32_t *)GMIO::malloc(256 * sizeof(int32_t));
    int32_t *outputArray = (int32_t *)GMIO::malloc(256 * sizeof(int32_t));
    printf("1\n");
    for (int i = 0; i < 256; i++)
        inputArray[i] = i + 1;

    std::ofstream ofs;

    XTime tStart, tEnd;
    XTime_GetTime(&tStart);
    gradf.init();
    gradf.run(1);
    for (int i = 0; i < 1; i++) {
        gradf.in.gm2aie_nb(&inputArray[i * 32], 32 * sizeof(int32_t));
        gradf.out.aie2gm_nb(&outputArray[i * 32], 32 * sizeof(int32_t));
    }
    gradf.out.wait(); // assuming data from gm1 are processed by the graph and output to gm2

    XTime_GetTime(&tEnd);

    printf("Output took %llu clock cycles.\n", 2 * (tEnd - tStart));
    printf("Output took %.2f us.\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND / 1000000));

    for (int i = 0; i < 256; i++) {
        std::cout << outputArray[i] << ", ";
    }
    std::cout << "before end" << std::endl;
    gradf.end();

    GMIO::free(inputArray);
    GMIO::free(outputArray);
}
int main(int argc, char **argv) {
    printf("test\n");
    adf_run();
    return 0;
}
