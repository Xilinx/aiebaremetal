// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "graph.h"
Mm gradf;
#else
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "xtime_l.h"
#endif

#include "aiebaremetal.h"
#include <fstream>
#include <xaiengine.h>
// #include "config.h"

#define SAMPLES 256
#define NBYTES_DATA 4

#define WIDTH 8
#define HEIGHT 8
#define SIZE (WIDTH * HEIGHT)

AbrGraph gr("gradf");

int main(int argc, char **argv) {
    printf("adfr--2---\n");
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
    gradf.init();
#else
#ifdef _RPU_
    // RPU need to disable data cache, without cache diable app will stuck
    Xil_DCacheDisable();
    // Xil_ICacheDisable();
#endif
#endif

    gr.getaiedevinst();

    gr.init();
    const int input_size_in_bytes = SIZE * sizeof(float);
    const int output_size_in_bytes = SIZE * sizeof(float);

    auto out_bomapped = reinterpret_cast<float *>(gr.malloc(output_size_in_bytes));
    memset(out_bomapped, 0, output_size_in_bytes);

    auto in_bomappedA = reinterpret_cast<float *>(gr.malloc(input_size_in_bytes));
    auto in_bomappedB = reinterpret_cast<float *>(gr.malloc(input_size_in_bytes));

    // setting input data
    float *golden = (float *)malloc(output_size_in_bytes);
    for (int i = 0; i < SIZE; i++) {
        in_bomappedA[i] = rand() % SIZE;
        in_bomappedB[i] = rand() % SIZE;
    }
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            golden[i * WIDTH + j] = 0;
            for (int k = 0; k < WIDTH; k++)
                golden[i * WIDTH + j] += in_bomappedA[i * WIDTH + k] * in_bomappedB[k * WIDTH + j];
        }
    }

    int rval;

    std::string gmioportA, gmioportB, gmioportC;

    // gmioportA = "gradf.inA";
    // gmioportB = "gradf.inB";

    gr.gm2aie_nb("gradf.inA", reinterpret_cast<char *>(in_bomappedA), SIZE * sizeof(float));
    std::cout << "syncAIEBO B \n";

    gr.gm2aie_nb("gradf.inB", reinterpret_cast<char *>(in_bomappedB), SIZE * sizeof(float));
    std::cout << "syncAIEBO A \n";

    printf("graph run\n");

    // gmioportC = "gradf.out";
    gr.run(1);
    gr.aie2gm_nb("gradf.out", reinterpret_cast<char *>(out_bomapped), SIZE * sizeof(float));
    gr.gmio_wait("gradf.out");
    int match = 0;
    for (int i = 0; i < SIZE; i++) {
        if (out_bomapped[i] != golden[i]) {
            printf("ERROR: Test failed! Error found in sample %d: golden: %f, hardware: %f\n", i, golden[i],
                   out_bomapped[i]);
            match = 1;
            break;
        }
    }

    printf("graph end\n");

    std::cout << "Releasing remaining objects...\n";

    gr.free(in_bomappedA);
    gr.free(in_bomappedB);
    gr.free(out_bomapped);
    gr.end();

    std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl;
    return (match ? EXIT_FAILURE : EXIT_SUCCESS);
}
