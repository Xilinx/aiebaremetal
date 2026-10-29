// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#if defined(__AEG_METADATA_ONLY__) || defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf;
#else

#include "aiebaremetal.h"
#include "aeg_platform.h"

#include <fstream>
#include <xaiengine.h>

AEGGraph<aeg::gradf> gr;

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

    gr.sq.in[1].update(reinterpret_cast<char *>(&param), sizeof(int));

    gr.run(2);
    std::cout << "before:run---" << std::endl;

    for (int i = 0; i < 2; i++) {
        gr.in.gm2aie_nb(reinterpret_cast<char *>(&inputArray[i * 32]), 32 * sizeof(int32_t));
        gr.out.aie2gm_nb(reinterpret_cast<char *>(&outputArray[i * 32]), 32 * sizeof(int32_t));
        gr.out.gmio_wait();
    }
    gr.wait();
    XTime_GetTime(&tEnd);
    printf("Output took %llu clock cycles.\n", 2 * (tEnd - tStart));
    printf("Output took %.2f us.\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND / 1000000));

    std::cout << "[debug] param=" << param << " gold[32]=" << outputArray_gold[32]
              << " gold[63]=" << outputArray_gold[63] << std::endl;

    for (int i = 0; i < 64; i++) {
        std::cout << outputArray[i] << ", ";
        if (outputArray[i] != outputArray_gold[i]) {
            std::cout << "\n" << "Test Failed \n";
            return -1;
        }
    }

    std::cout << "\n" << "Test Passed (New API)\n";

    gr.free(inputArray);
    gr.free(outputArray);
    gr.free(outputArray_gold);
    return 0;
}
#endif /* !__AEG_METADATA_ONLY__ && !__ADF_FRONTEND__ */
