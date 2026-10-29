// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#define NUM_SAMPLES 32

#if defined(__AEG_METADATA_ONLY__) || defined(__ADF_FRONTEND__)
#include "graph.h"
MyGraph gradf;
#else

#include "aiebaremetal.h"
#include "aeg_platform.h"

AEGGraph<aeg::gradf> gr;

int main(int argc, char **argv) {
    printf("init()\n");
    gr.init();
    gr.run(1);

    int32_t *inputArray = (int32_t *)gr.malloc(NUM_SAMPLES * sizeof(int32_t));
    int32_t *outputArray = (int32_t *)gr.malloc(NUM_SAMPLES * sizeof(int32_t));
    int32_t *outputArray_gold = (int32_t *)gr.malloc(NUM_SAMPLES * sizeof(int32_t));

    for (int i = 0; i < NUM_SAMPLES; i++) {
        inputArray[i] = 2;
        outputArray[i] = 0;
        outputArray_gold[i] = 300;
    }

    gr.in.gm2aie_nb(reinterpret_cast<char *>(&inputArray[0]), NUM_SAMPLES * sizeof(int32_t));
    gr.out.aie2gm_nb(reinterpret_cast<char *>(&outputArray[0]), NUM_SAMPLES * sizeof(int32_t));

    int data[NUM_SAMPLES];
    for (int i = 0; i < NUM_SAMPLES; i++)
        data[i] = 150;
    gr.wts.update(data, NUM_SAMPLES * sizeof(int32_t));

    std::cout << "before gmio_wait ---" << std::endl;
    gr.out.gmio_wait();
    printf("after gmio_wait\n");
    gr.wait();
    gr.end();

    for (int i = 0; i < NUM_SAMPLES; i++) {
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
