// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "graph.h"
myGraph gradf;
#else
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
#if __AIE_ARCH__ == 22
#include "xiltimer.h"
#else
#include "xtime_l.h"
#endif
#endif

#include "aiebaremetal.h"
#include <fstream>
#include <math.h>
#include <xaiengine.h>

AbrGraph gr("gradf");

#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
void adf_run() {
    int32_t *inputArray = (int32 *)GMIO::malloc(256 * sizeof(int32));
    int32_t *outputArray = (int32 *)GMIO::malloc(256 * sizeof(int32));

    for (int i = 0; i < 256; i++)
        inputArray[i] = i + 1;

    std::ofstream ofs;

    gradf.run(1);
    for (int i = 0; i < 1; i++) {
        gradf.in.gm2aie_nb(&inputArray[i * 32], 32 * sizeof(int32));
        gradf.out.aie2gm_nb(&outputArray[i * 32], 32 * sizeof(int32));
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
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
    std::cout << "gradf.a  =" << gradf.a << std::endl;
    gradf.init();
#endif

    float *inputArray = (float *)gr.malloc(256 * sizeof(float));
    float *outputArray = (float *)gr.malloc(256 * sizeof(float));
    float *outputArray_gold = (float *)gr.malloc(256 * sizeof(float));

    gr.init();
    gr.run(8);

    for (int i = 0; i < 256; i++)
        inputArray[i] = (i + 1) / 500.0;
    std::cout << "before:run---" << std::endl;

    for (int i = 0; i < 8; i++) {
        gr.gm2aie_nb("gradf.in", reinterpret_cast<char *>(&inputArray[i * 32]), 32 * sizeof(float));
        gr.aie2gm_nb("gradf.out", reinterpret_cast<char *>(&outputArray[i * 32]), 32 * sizeof(float));
    }
    gr.gmio_wait("gradf.out"); // assuming data from gm1 are processed by the graph and output to gm2
    std::cout << "run---" << std::endl;
    gr.wait();
    std::cout << "Test Passed\n";

    return 0;
}
