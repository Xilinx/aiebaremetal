/**<!--
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
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
    setbuf(stdout, NULL);

#if defined(AEG_HARDWARE) && defined(_RPU_)
    Xil_DCacheDisable();
    Xil_ICacheDisable();
#endif

    gr.getaiedevinst();
    gr.init();
    PROFILE_ID handle0 = gr.in.start_profiling(io_total_stream_running_to_idle_cycles);
    printf("start profiling\n");

    XTime tStart, tEnd;
    XTime_GetTime(&tStart);
    int32_t *inputArray = (int32_t *)gr.malloc(DATA_LENGTH * sizeof(int32_t));
    int32_t *outputArray = (int32_t *)gr.malloc(DATA_LENGTH * sizeof(int32_t));
    int32_t *outputArray_gold = (int32_t *)gr.malloc(DATA_LENGTH * sizeof(int32_t));
    printf("handle0 = 0x%d\n", handle0);

    gr.run(1);

    for (int i = 0; i < DATA_LENGTH; i++) {
        inputArray[i] = i + 1;
        outputArray_gold[i] = inputArray[i] * inputArray[i];
    }

    gr.in.gm2aie(reinterpret_cast<char *>(&inputArray[0]), DATA_LENGTH * sizeof(int32_t));
    gr.out.aie2gm(reinterpret_cast<char *>(&outputArray[0]), DATA_LENGTH * sizeof(int32_t));
    gr.wait();

    XTime_GetTime(&tEnd);
    printf("Output took %llu clock cycles.\n", 2 * (tEnd - tStart));

    gr.end();

    if (handle0 != -1) {
        long long cycle_count0 = gr.in.read_profiling(handle0);
        gr.in.stop_profiling(handle0);
        printf("cycle_count0 = %lld\n", cycle_count0);
    }

    for (int i = 0; i < DATA_LENGTH; i++) {
        std::cout << "value " << outputArray[i] << ", ";
        if (outputArray[i] != outputArray_gold[i]) {
            std::cout << "\n at data[" << i << "] incorrect Test Failed \n";
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
