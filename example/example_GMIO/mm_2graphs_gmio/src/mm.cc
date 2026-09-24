// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "mm.h"
#include <adf.h>
#include <stdio.h>

// assuming NUM_CUS==2
void krnlmm(input_window_float *ina, input_window_float *inb, output_window_float *out) {

    float vala, valb;
    float valr;

    float matA[WIDTH];
    float matB[SIZE];

    for (unsigned ii = 0; ii < HEIGHT; ii++) {
        for (unsigned i = 0; i < WIDTH; i++) {
            window_readincr(inb, valb);
            matB[WIDTH * ii + i] = valb;
        }
    }

    for (unsigned ii = 0; ii < HEIGHT; ii++) {

        for (unsigned i = 0; i < WIDTH; i++) {
            window_readincr(ina, vala);
            matA[i] = vala;
        }
        // write back results
        for (unsigned i = 0; i < WIDTH; i++) {
            valr = 0.0;
            for (unsigned j = 0; j < WIDTH; j++) {

                valr += matA[j] * matB[i * WIDTH + j];
            }

            window_writeincr(out, valr);
        }
    }
}
