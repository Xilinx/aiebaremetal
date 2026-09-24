// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "config.h"
#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <stdio.h>
#define VECTOR_LENGTH 16
#define FORWARD_TEST
void domath(int32 VEC[], int len) {
    for (int l = 0; l < len; l++) {
        VEC[l] *= VEC[l];
    }
}
void square(input_window_int32 *win, output_window_int32 *out) {
    int method = 2;
    switch (method) {
    case 0:
        int32_t VEC[WIN_SIZE];
        for (unsigned i = 0; i < DATA_LENGTH / WIN_SIZE; i++) {
            window_acquire(win);
            window_acquire(out);
            for (unsigned w = 0; w < WIN_SIZE / VECTOR_LENGTH; w++)
                chess_prepare_for_pipelining {
                    aie::vector<int32_t, VECTOR_LENGTH> temp_a = window_readincr_v<VECTOR_LENGTH>(win);
                    auto temp_b = aie::mul(temp_a, temp_a);
                    aie::store_unaligned_v<VECTOR_LENGTH>(VEC + w * VECTOR_LENGTH, temp_b.to_vector<int32_t>());
                    for (int s = 0; s < VECTOR_LENGTH; s++) {
                        auto vv = VEC + w * VECTOR_LENGTH;
                        printf("vv[%d] is %d\n", s, vv[s]);
                    }
                    window_writeincr(out, aie::load_v<VECTOR_LENGTH>(VEC + w * VECTOR_LENGTH));
                }

            window_release(out);
            window_release(win);
        }
        break;

    case 1:
#ifndef FORWARD_TEST
        int32_t A[DATA_LENGTH];
#else
        int32_t A[VECTOR_LENGTH];
#endif
        for (unsigned i = 0; i < DATA_LENGTH / WIN_SIZE; i++) {
            window_acquire(win);
            for (unsigned w = 0; w < WIN_SIZE / VECTOR_LENGTH; w++)
                chess_prepare_for_pipelining {
                    aie::vector<int32_t, VECTOR_LENGTH> temp_a = window_readincr_v<VECTOR_LENGTH>(win);
#ifndef FORWARD_TEST
                    aie::store_unaligned_v<VECTOR_LENGTH>(A + i * WIN_SIZE + (w * VECTOR_LENGTH), temp_a);
#else
                    aie::store_unaligned_v<VECTOR_LENGTH>(A, temp_a);
#endif
                }
            window_release(win);
        }
#ifndef FORWARD_TEST
        for (int i = 0; i < DATA_LENGTH; i++) {
            A[i] *= A[i];
        }
#else
        for (int j = 0; j < DATA_LENGTH / VECTOR_LENGTH; j++) {
            for (int i = 0; i < VECTOR_LENGTH; i++) {
                A[i] *= A[i];
            }
        }
#endif

        for (unsigned i = 0; i < DATA_LENGTH / WIN_SIZE; i++) {
            window_acquire(out);
            for (unsigned z = 0; z < WIN_SIZE / VECTOR_LENGTH; z++)
                chess_prepare_for_pipelining {
#ifndef FORWARD_TEST
                    window_writeincr(out, aie::load_v<VECTOR_LENGTH>(A + i * WIN_SIZE + (z * VECTOR_LENGTH)));
#else
                    window_writeincr(out, aie::load_v<VECTOR_LENGTH>(A));
#endif
                }
            window_release(out);
        }
        break;

    case 2:
        printf("square log\n");
        for (unsigned i = 0; i < DATA_LENGTH / WIN_SIZE; i++) {
            window_acquire(win);
            window_acquire(out);
            for (int k = 0; k < WIN_SIZE; k++) {
                int j = window_readincr(win);
                window_writeincr(out, j * j);
            }
            window_release(out);
            window_release(win);
        }
        printf("\nsquare log end \n");
        break;
    }
}
