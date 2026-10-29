/******************************************************************************
 * Copyright (C) 2019 - 2022 Xilinx, Inc.  All rights reserved.
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "config.h"
#include "mmul_gemm.h"
#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <aie_api/utils.hpp>

void OneInput(input_window_int8 *dataIn, output_window_int8 *dataOut, output_window_int8 *result) {
    static GEMM_AB_T a[NUM_A_ELMNTS_PER_TILE];
    // The MM_N B columns of the current N-block, staged K-major (bbuf[k*MM_N + n])
    // so each MM_K x MM_N B tile is a single contiguous vector load for the
    // matrix engine.
    static GEMM_AB_T bbuf[NUM_COLS * MM_N];
    static GEMM_AB_T bstage[MM_N * NUM_COLS];  // B as it arrives: n-major
    static GEMM_C_T intrmdtResult[WIN_SIZE];
    static uint32_t currentCol;

    currentCol = (get_coreid() & 0x7F0000) >> 16;

    // Load this tile's A stripe from the input stream (int8).
    for (unsigned i = 0; i < NUM_A_ELMNTS_PER_TILE / WIN_SIZE; i++) {
        window_acquire(dataIn);
        for (unsigned w = 0; w < WIN_SIZE / VEC_IN; w++)
            chess_prepare_for_pipelining {
                aie::vector<GEMM_AB_T, VEC_IN> temp = window_readincr_v<VEC_IN>(dataIn);
                aie::store_unaligned_v<VEC_IN>(a + (i * WIN_SIZE) + (w * VEC_IN), temp);
            }
        window_release(dataIn);
    }

    // Forward the A stripes destined for the downstream cores in this chain.
    for (unsigned i = 0; i < NUM_A_ELMNTS_PER_TILE * (NUM_HW_COLS - currentCol - 1) / WIN_SIZE; i++) {
        window_acquire(dataOut);
        window_acquire(dataIn);
        for (unsigned w = 0; w < WIN_SIZE / VEC_IN; w++)
            chess_prepare_for_pipelining { window_writeincr(dataOut, window_readincr_v<VEC_IN>(dataIn)); }
        window_release(dataIn);
        window_release(dataOut);
    }

    /*
     * Stream B in MM_N-column blocks. For each block: read + forward the columns
     * (staging them K-major in bbuf), run the matrix-engine GEMM for the block,
     * then flush a full WIN_SIZE of results once enough blocks have accumulated.
     */
    for (unsigned nb = 0; nb < MM_N_BLOCKS; nb++) {
        for (unsigned nn = 0; nn < MM_N; nn++) {
            for (unsigned w = 0; w < (NUM_COLS / WIN_SIZE); w++) {
                window_acquire(dataOut);
                window_acquire(dataIn);
                for (unsigned x = 0; x < WIN_SIZE / VEC_IN; x++)
                    chess_prepare_for_pipelining {
                        aie::vector<GEMM_AB_T, VEC_IN> temp = window_readincr_v<VEC_IN>(dataIn);
                        window_writeincr(dataOut, temp);
                        const unsigned kbase = (w * WIN_SIZE) + (x * VEC_IN);
                        // Land B n-major with one vector store; the n-major ->
                        // k-major reshape happens once per N-block below, as a
                        // vector transpose instead of VEC_IN scalar scatters.
                        aie::store_unaligned_v(bstage + (nn * NUM_COLS) + kbase, temp);
                    }
                window_release(dataIn);
                window_release(dataOut);
            }
        }

        mm_stage_b_transpose(bstage, bbuf);

        // CEILING_REPEAT==1 in normal runs; >1 keeps the core busy on local
        // buffers to probe the compute ceiling (idempotent, output unchanged).
        for (unsigned rep = 0; rep < CEILING_REPEAT; rep++)
            mm_compute_nblock(a, bbuf, intrmdtResult, nb);

        if ((nb % MM_NBLK_PER_FLUSH) == (MM_NBLK_PER_FLUSH - 1)) {
            window_acquire(result);
            for (unsigned z = 0; z < WIN_SIZE / VEC_OUT; z++)
                chess_prepare_for_pipelining {
                    window_writeincr(result, aie::load_v<VEC_OUT>(intrmdtResult + (z * VEC_OUT)));
                    for (unsigned i = 0; i < VEC_OUT; ++i)
                        intrmdtResult[(z * VEC_OUT) + i] = 0;
                }
            window_release(result);
        }
    }
}
