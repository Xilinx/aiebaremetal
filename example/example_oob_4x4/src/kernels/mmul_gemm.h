/******************************************************************************
 * Copyright (C) 2019 - 2022 Xilinx, Inc.  All rights reserved.
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#ifndef XGEMM_MMUL_GEMM_H
#define XGEMM_MMUL_GEMM_H

#include "config.h"
#include <aie_api/aie.hpp>

/*
 * Matrix-engine compute for one N-block (MM_N streamed B columns).
 *
 * Inputs:
 *   a       : local A stripe, row-major [row * NUM_COLS + k], NUM_ROWS_PER_TILE
 *             rows x NUM_COLS (K) elements.
 *   bbuf    : the current MM_N B columns staged K-major as bbuf[k * MM_N + n],
 *             length NUM_COLS * MM_N. (Filled by the kernel's B read/forward
 *             loop; K-major so each MM_K x MM_N B tile is a contiguous load.)
 *   nb      : current N-block index (0 .. MM_N_BLOCKS-1).
 *
 * Output:
 *   intrmdtResult : results scattered to match the legacy per-column write
 *             order. Within a WIN_SIZE flush window (MM_NBLK_PER_FLUSH blocks),
 *             element index = (local_col * NUM_ROWS_PER_TILE) + m_row, exactly
 *             as the original column-by-column kernel produced, so the result
 *             stream to the host/downstream cores is byte-identical.
 *
 * The matrix engine computes an MM_M x MM_N C tile per mmul object, accumulating
 * over K with one mul + (MM_K_STEPS-1) mac calls, replacing the vector-MAC +
 * horizontal reduce of the scalar-reduce kernel.
 */
/*
 * Vectorised B staging: n-major -> k-major.
 *
 * B arrives one column at a time, so the natural landing zone is n-major
 * (bstage[n * NUM_COLS + k], each column contiguous in K — a plain vector
 * store). aie::mmul wants each MM_K x MM_N tile contiguous, i.e. k-major
 * (bbuf[k * MM_N + n]). The kernels used to bridge that with a per-lane scalar
 * scatter — `for j < VEC_IN: bbuf[(kbase+j)*MM_N + nn] = temp.get(j)` — which
 * costs VEC_IN lane-extracts plus VEC_IN strided byte stores for every 32-byte
 * vector read, i.e. 65,536 scalar operations per kernel invocation against
 * 2,048 matrix-engine ops. Measured on hardware that left the cores at 0.61 %
 * vector-slot utilisation.
 *
 * Here the same reshape is one aie::transpose per MM_K block: gather the MM_N
 * columns' 16-byte slices, concat into a row-major MM_N x MM_K vector, and
 * transpose it to MM_K x MM_N — which IS the k-major layout, by definition of
 * the two orderings. MM_K_STEPS blocks per N-block, ~12 vector ops each, no
 * scalar element handling at all.
 */
static inline void mm_stage_b_transpose(const GEMM_AB_T *bstage, GEMM_AB_T *bbuf) {
    for (unsigned ks = 0; ks < MM_K_STEPS; ks++)
        chess_prepare_for_pipelining {
            aie::vector<GEMM_AB_T, MM_K> c0 = aie::load_v<MM_K>(bstage + 0 * NUM_COLS + ks * MM_K);
            aie::vector<GEMM_AB_T, MM_K> c1 = aie::load_v<MM_K>(bstage + 1 * NUM_COLS + ks * MM_K);
            aie::vector<GEMM_AB_T, MM_K> c2 = aie::load_v<MM_K>(bstage + 2 * NUM_COLS + ks * MM_K);
            aie::vector<GEMM_AB_T, MM_K> c3 = aie::load_v<MM_K>(bstage + 3 * NUM_COLS + ks * MM_K);
            aie::vector<GEMM_AB_T, MM_K> c4 = aie::load_v<MM_K>(bstage + 4 * NUM_COLS + ks * MM_K);
            aie::vector<GEMM_AB_T, MM_K> c5 = aie::load_v<MM_K>(bstage + 5 * NUM_COLS + ks * MM_K);
            aie::vector<GEMM_AB_T, MM_K> c6 = aie::load_v<MM_K>(bstage + 6 * NUM_COLS + ks * MM_K);
            aie::vector<GEMM_AB_T, MM_K> c7 = aie::load_v<MM_K>(bstage + 7 * NUM_COLS + ks * MM_K);
            // Row-major MM_N rows (n) x MM_K cols (k) ...
            aie::vector<GEMM_AB_T, MM_K * MM_N> nmaj =
                aie::concat(aie::concat(c0, c1, c2, c3), aie::concat(c4, c5, c6, c7));
            // ... transposed to MM_K rows (k) x MM_N cols (n) = bbuf[k * MM_N + n].
            aie::store_unaligned_v(bbuf + ks * (MM_K * MM_N),
                                   aie::transpose(nmaj, MM_N, MM_K));
        }
}

static inline void mm_compute_nblock(const GEMM_AB_T *a, const GEMM_AB_T *bbuf,
                                     GEMM_C_T *intrmdtResult, unsigned nb) {
    using MMUL = aie::mmul<MM_M, MM_K, MM_N, GEMM_AB_T, GEMM_AB_T>;

    const unsigned flush_blk = nb % MM_NBLK_PER_FLUSH;

    for (unsigned ms = 0; ms < MM_M_SUB; ms++) {
        MMUL C;
        for (unsigned ks = 0; ks < MM_K_STEPS; ks++)
            chess_prepare_for_pipelining {
                aie::vector<GEMM_AB_T, MM_K> r0 = aie::load_v<MM_K>(a + (ms * MM_M + 0) * NUM_COLS + ks * MM_K);
                aie::vector<GEMM_AB_T, MM_K> r1 = aie::load_v<MM_K>(a + (ms * MM_M + 1) * NUM_COLS + ks * MM_K);
                aie::vector<GEMM_AB_T, MM_K> r2 = aie::load_v<MM_K>(a + (ms * MM_M + 2) * NUM_COLS + ks * MM_K);
                aie::vector<GEMM_AB_T, MM_K> r3 = aie::load_v<MM_K>(a + (ms * MM_M + 3) * NUM_COLS + ks * MM_K);
                aie::vector<GEMM_AB_T, MM_M * MM_K> av = aie::concat(r0, r1, r2, r3);
                aie::vector<GEMM_AB_T, MM_K * MM_N> bv = aie::load_v<MM_K * MM_N>(bbuf + ks * (MM_K * MM_N));
                if (ks == 0)
                    C.mul(av, bv);
                else
                    C.mac(av, bv);
            }

        // Extract the full-K result as int32 from the acc32 accumulator, then
        // SATURATE to int8 explicitly (aiehlc simplematmul2 does the same clamp).
        // NB: acc.to_vector<int8>() applies a shift/round-narrow whose semantics
        // are not a plain clamp (it returned 0 here), so extract int32 and clamp.
        aie::vector<int32_t, MM_M * MM_N> cv = C.template to_vector<int32_t>();
        for (unsigned mrow = 0; mrow < MM_M; mrow++)
            for (unsigned nn = 0; nn < MM_N; nn++) {
                int32_t v = cv.get(mrow * MM_N + nn);
                v = (v > 127) ? 127 : ((v < -128) ? -128 : v);
                intrmdtResult[(flush_blk * MM_N + nn) * NUM_ROWS_PER_TILE + ms * MM_M + mrow] =
                    (GEMM_C_T)v;
            }
    }
}

#endif
