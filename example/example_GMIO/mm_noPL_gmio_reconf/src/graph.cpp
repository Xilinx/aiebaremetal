// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
// This tests using multiple instance of the same kernel using multiple GMIO ports.
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "graph.h"
Mm mm;
#else
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "xtime_l.h"
#endif

#include "aiebaremetal.h"
#include <fstream>
#include <xaiengine.h>

AbrGraph gr("mm");

// #include "graph.cpp"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
// #include "include.h"
#include <cstring>
#include <iostream>
#include <vector>

#define SAMPLES 256
#define NBYTES_DATA 4

#define WIDTH 8
#define HEIGHT 8
#define SIZE (WIDTH * HEIGHT)

int main(int argc, char **argv) {
    const int input_size_in_bytes = SIZE * sizeof(float);
    const int output_size_in_bytes = SIZE * sizeof(float);

#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
    mm.init();
#endif
    gr.init();

    gr.run(1);

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
                golden[i * WIDTH + j] += in_bomappedA[i * WIDTH + k] * in_bomappedB[k + WIDTH * j];
        }
    }

    int rval;
    std::string gmioportA, gmioportB, gmioportC;
    int gmio_id = 0;
    gmioportA = std::to_string(gmio_id++);

    gmioportB = std::to_string(gmio_id++);
    gr.gm2aie_nb("mm.inB", reinterpret_cast<char *>(in_bomappedB), SIZE * sizeof(float));
    std::cout << "syncAIEBO B \n";

    gr.gm2aie_nb("mm.inA", reinterpret_cast<char *>(in_bomappedA), SIZE * sizeof(float));
    std::cout << "syncAIEBO A \n";

    gmioportC = std::to_string(gmio_id++);
    gr.aie2gm_nb("mm.out", reinterpret_cast<char *>(out_bomapped), SIZE * sizeof(float));
    gr.gmio_wait("mm.out"); // assuming data from gm1 are processed by the graph and output to gm2
    int match = 0;
    for (int i = 0; i < SIZE; i++) {
        if (out_bomapped[i] != golden[i]) {
            printf("ERROR: Test failed! Error found in sample %d: golden: %f, hardware: %f\n", i, golden[i],
                   out_bomapped[i]);
            match = 1;
            break;
        }
    }

    // std::cout << "Releasing remaining  objects...\n";

    gr.free(in_bomappedA);
    gr.free(in_bomappedB);
    gr.free(out_bomapped);

    std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl;
    return (match ? EXIT_FAILURE : EXIT_SUCCESS);
}
