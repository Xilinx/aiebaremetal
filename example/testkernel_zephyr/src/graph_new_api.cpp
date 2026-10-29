// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#if defined(__AEG_METADATA_ONLY__) || defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf;
#else
#include "aiebaremetal.h"
#include "aeg_platform.h"

#include "config.h"
#include <fstream>
#include <xaiengine.h>

AEGGraph<aeg::gradf> gr;

int main(int argc, char **argv) {
#if defined(__ADF_FRONTEND__)
    gradf.init();
#else
#ifdef _RPU_
    Xil_DCacheDisable();
    Xil_ICacheDisable();
#endif
#endif
    gr.init();
    gr.getaiedevinst();

    PROFILE_ID handle0 = gr.in.start_profiling(io_total_stream_running_to_idle_cycles);

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

    gr.in.gm2aie_nb(reinterpret_cast<char *>(&inputArray[0]), DATA_LENGTH * sizeof(int32_t));

    gr.out.aie2gm_nb(reinterpret_cast<char *>(&outputArray[0]), DATA_LENGTH * sizeof(int32_t));

    gr.out.gmio_wait();

    gr.wait();

    XTime_GetTime(&tEnd);
    printf("Output took %llu clock cycles.\n", 2 * (tEnd - tStart));
    printf("Output 2 took %.2f us.\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND / 1000000));

    gr.end();

    if (handle0 != -1) {
        long long cycle_count0 = gr.in.read_profiling(handle0);
        gr.in.stop_profiling(handle0);
        printf("cycle_count0 = %lld\n", cycle_count0);
        fflush(stdout);
    }

    for (int i = 0; i < DATA_LENGTH; i++) {
        std::cout << "value " << outputArray[i] << ", ";
        fflush(stdout);
        if (outputArray[i] != outputArray_gold[i]) {
            std::cout << "\n at data[" << i << "] incorrect Test Failed \n";
            fflush(stdout);
            return -1;
        }
    }

    std::cout << "\n" << "Test Passed (New API)\n";
    fflush(stdout);

    gr.free(inputArray);
    gr.free(outputArray);
    gr.free(outputArray_gold);
    return 0;
}
#endif /* !__AEG_METADATA_ONLY__ && !__ADF_FRONTEND__ */
