// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#if defined(__AEG_METADATA_ONLY__) || defined(__ADF_FRONTEND__)
#include "graph.h"
mygraph gradf;
#else

#include "aiebaremetal.h"
#include "aeg_platform.h"
#include <fstream>
#include <xaiengine.h>

AEGGraph<aeg::gradf> gr;

// M is starting index
#define M 0
// P is number of active GMIOs (12 for vek385, up to 32 for other devices)
#define P 12
#define ITER 1

int main(int argc, char **argv) {
#if defined(AEG_HARDWARE) && defined(_RPU_)
    Xil_DCacheDisable();
    Xil_ICacheDisable();
#endif

    XTime tStart, tEnd;
    XTime_GetTime(&tStart);

    gr.init();

    for (int i = M; i < M + P; i++) {
        int data = i + 1;
        gr.k[i].in[1].update(reinterpret_cast<char *>(&data), sizeof(int));
    }
    std::cout << "RTP update done" << std::endl;

    int32_t *inBOMap[P];
    for (int i = M; i < M + P; i++) {
        inBOMap[i] = (int32_t *)gr.malloc(256 * sizeof(int32_t));
        for (int j = 0; j < 256; j++) {
            inBOMap[i][j] = i + 1;
        }
    }

    int32_t *outBOMap[P];
    for (int i = M; i < M + P; i++) {
        outBOMap[i] = (int32_t *)gr.malloc(256 * sizeof(int32_t));
        for (int j = 0; j < 256; j++) {
            outBOMap[i][j] = 0xABCDEF00;
        }
    }
    std::cout << "data is ready" << std::endl;

    gr.run(ITER);

    for (int j = 0; j < ITER; j++) {
        for (int i = M; i < M + P; i++) {
            gr.input[i].gm2aie_nb(reinterpret_cast<char *>(&inBOMap[i][j * 32]), 32 * sizeof(int32_t));
            gr.output[i].aie2gm_nb(reinterpret_cast<char *>(&outBOMap[i][j * 32]), 32 * sizeof(int32_t));
        }
    }

    gr.wait();

    for (int i = M; i < M + P; i++) {
        gr.output[i].gmio_wait();
    }
    std::cout << "****************************GMIO is done************************************" << std::endl;

    int errorCount = 0;
    for (int i = M; i < M + P; i++) {
        for (int j = 0; j < 32 * ITER; j++) {
            if (outBOMap[i][j] != (1 + i) * 2) {
                errorCount++;
            }
        }
    }
    if (errorCount)
        printf("Test failed with %d errors\n", errorCount);
    else
        printf("Test passed\n");

    gr.end();

    XTime_GetTime(&tEnd);
    printf("Output took %llu clock cycles.\n", 2 * (tEnd - tStart));
    printf("Output took %.2f us.\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND / 1000000));

    for (int i = M; i < M + P; i++) {
        gr.free(inBOMap[i]);
        gr.free(outBOMap[i]);
    }
    std::cout << "TEST " << (errorCount ? "FAILED" : "PASSED") << std::endl;
    return (errorCount ? EXIT_FAILURE : EXIT_SUCCESS);
}
#endif /* !__AEG_METADATA_ONLY__ && !__ADF_FRONTEND__ */
