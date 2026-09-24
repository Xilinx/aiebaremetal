// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "graph.h"
myGraph gradf;
#else
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
#if __AIE_ARCH__ != 30
#include "xtime_l.h"
#endif
#endif

#include "aiebaremetal.h"
#include "config.h"
#include <fstream>
#include <xaiengine.h>

AbrGraph gr("gradf");

int main(int argc, char **argv) {
    printf("adfr--3---\n");
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
    gradf.init();
#else
#ifdef _RPU_
    // RPU need to disable data cache, without cache diable app will stuck
    // Xil_DCacheDisable();
    // Xil_ICacheDisable();
#endif
#endif

    gr.getaiedevinst();
    gr.init();
    PROFILE_ID handle0 = gr.start_profiling("gradf.in", io_total_stream_running_to_idle_cycles);
    printf("start profiling\n");
#if not((defined(__AIESIM__) || defined(__ADF_FRONTEND__) || (__AIE_ARCH__ == 30)))
    XTime tStart, tEnd;
    XTime_GetTime(&tStart);
#endif
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

#if not((defined(__AIESIM__) || defined(__ADF_FRONTEND__) || (__AIE_ARCH__ == 30)))
    XTime_GetTime(&tEnd);
    printf("Output took %llu clock cycles.\n", 2 * (tEnd - tStart));
    printf("Output 2 took %.2f us.\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND / 1000000));
#endif

    gr.end();
    if (handle0 != -1) {
        long long cycle_count0 = gr.read_profiling(handle0);
        gr.stop_profiling(handle0);
        printf("cycle_count0 = %lld\n", cycle_count0);
    }

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
