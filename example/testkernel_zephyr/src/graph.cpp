// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "aiebaremetal.h"
#include "aeg_platform.h"
#if defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf;
#endif

#include "config.h"
#include <fstream>
#include <xaiengine.h>

AbrGraph gr("gradf");

int main(int argc, char **argv) {
    for (int i = 0; i < 100; i++) {
        printf(".");
    }
    printf("\n");

#if defined(AEG_HARDWARE) && defined(_RPU_) && !defined(__ZEPHYR__)
    Xil_DCacheDisable();
    Xil_ICacheDisable();
#endif

    gr.init();
    gr.getaiedevinst();
    //PROFILE_ID handle0 = gr.start_profiling("gradf.in", io_total_stream_running_to_idle_cycles);
    XTime tStart, tEnd;
    XTime_GetTime(&tStart);
    int32_t *inputArray = (int32_t *)gr.malloc(DATA_LENGTH * sizeof(int32_t));
    int32_t *outputArray = (int32_t *)gr.malloc(DATA_LENGTH * sizeof(int32_t));
    int32_t *outputArray_gold = (int32_t *)gr.malloc(DATA_LENGTH * sizeof(int32_t));
    gr.run(1);
    for (int i = 0; i < DATA_LENGTH; i++) {
        inputArray[i] = i + 1;
        outputArray_gold[i] = inputArray[i] * inputArray[i];
    }

    gr.gm2aie_nb("gradf.in", reinterpret_cast<char *>(&inputArray[0]), DATA_LENGTH * sizeof(int32_t));
    gr.aie2gm_nb("gradf.out", reinterpret_cast<char *>(&outputArray[0]), DATA_LENGTH * sizeof(int32_t));
    gr.gmio_wait("gradf.out"); // assuming data from gm1 are processed by the graph and output to gm2

    gr.wait();

    XTime_GetTime(&tEnd);
    printf("Output took %llu clock cycles.\n", 2 * (tEnd - tStart));
    printf("Output 2 took %.2f us.\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND / 1000000));

    gr.end();
#if 0
    if (handle0 != -1) {
        long long cycle_count0 = gr.read_profiling(handle0);
        gr.stop_profiling(handle0);
        printf("cycle_count0 = %lld\n", cycle_count0);
    }
#endif

    for (int i = 0; i < DATA_LENGTH; i++) {
        std::cout << "value " << outputArray[i] << ", ";
        if (outputArray[i] != outputArray_gold[i]) {
            std::cout << "\n at data[" << i << "] incorrect Test Failed \n";
            return -1;
        }
    }

    std::cout << "\n" << "Test Passed\n";

    gr.free(inputArray);
    gr.free(outputArray);
    gr.free(outputArray_gold);
    return 0;
}
