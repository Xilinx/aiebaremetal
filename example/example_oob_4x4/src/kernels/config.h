/******************************************************************************
 * Copyright (C) 2019 - 2022 Xilinx, Inc.  All rights reserved.
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

 #ifndef XGEMM_CONFIG_H
 #define XGEMM_CONFIG_H

 #define ARCH_AIE 10
 #define ARCH_AIEML 20
 #define ARCH_AIE2PS 22

 // #define _PALMYRA_

 // #define _PALMYRA_
 //  Below configuration works for both APU and RPU
 #ifdef _APURPU_
 // #define NUM_HW_COLS 30
 #define NUM_HW_COLS 4
 #if (__AIE_ARCH__ == ARCH_AIE2PS)
 #define NUM_HW_ROWS 4
 #else //(__AIE_ARCH__ == ARCH_AIEML||ARCH_AIE)
 // #define NUM_HW_ROWS 8
 #define NUM_HW_ROWS 4
 #endif
 #define MAT_SIZE 480
 #define WIN_SIZE 480

 // Below configuration works for APU only
 #else
 #if (__AIE_ARCH__ == ARCH_AIE)
 // #define NUM_HW_COLS 50
 #define NUM_HW_COLS 4
 // #define NUM_HW_ROWS 8
 #define NUM_HW_ROWS 4
 #define MAT_SIZE 1200
 #define WIN_SIZE 600
 #elif (__AIE_ARCH__ == ARCH_AIEML)
 // #define NUM_HW_COLS 38
 #define NUM_HW_COLS 4
 // #define NUM_HW_ROWS 8
 #define NUM_HW_ROWS 4
 #define MAT_SIZE 608
 #define WIN_SIZE 608
 #else            //(__AIE_ARCH__ == AIE2ps)
 #ifdef _PALMYRA_ // coroperate with prebuild palmyra 1-11 col pdi
 // #define NUM_HW_COLS 12
 #define NUM_HW_COLS 4
 #else
 // #define NUM_HW_COLS 36
 #define NUM_HW_COLS 4
 #endif
 #define NUM_HW_ROWS 4

 #define MAT_SIZE 384
 #define WIN_SIZE 384
 #endif
 #endif

 /*
  * ---------------------------------------------------------------------------
  * Panel / full-matrix decoupling for large-matrix streaming on the 4x4 array.
  *
  * The 4x4 systolic array computes a single square GEMM of side PANEL_SIZE per
  * graph invocation. All per-core static buffers (a[], b[], intrmdtResult[]) are
  * sized from PANEL_SIZE (via MAT_SIZE / WIN_SIZE / NUM_COLS below), so on-core
  * memory is bounded by the panel, never by the full problem size.
  *
  * The full problem is MATRIX_SIZE x MATRIX_SIZE, made of NUM_PANELS panels per
  * dimension. The host (graph.cpp) blocks the GEMM into PANEL_SIZE^3 products,
  * runs the array once per (mPanel, nPanel, kPanel) block, and accumulates the
  * partial C panels in DDR. This lets the fixed 4x4 array multiply matrices that
  * are far larger than a single tile's local memory.
  *
  * The arch branches above only matter for NUM_HW_ROWS / NUM_HW_COLS (always 4).
  * We override MAT_SIZE / WIN_SIZE here so the panel size is deterministic
  * regardless of which __AIE_ARCH__ branch was selected.
  * ---------------------------------------------------------------------------
  */
#undef MAT_SIZE
#undef WIN_SIZE

#ifndef PANEL_SIZE
#define PANEL_SIZE 256
#endif

#ifndef NUM_PANELS
// 1 panel/dim x 256 = 256x256 matrix (single on-core panel, no blocking).
#define NUM_PANELS 1
#endif

// On-core working dimensions: one panel per array invocation.
#define MAT_SIZE PANEL_SIZE
#define WIN_SIZE PANEL_SIZE

// Full problem dimensions, assembled by the host across panels.
#define MATRIX_SIZE (NUM_PANELS * PANEL_SIZE)
#define PANEL_M PANEL_SIZE
#define PANEL_N PANEL_SIZE
#define PANEL_K PANEL_SIZE

 #define NUM_COLS MAT_SIZE
 #define NUM_ROWS MAT_SIZE

 /*
  * ---------------------------------------------------------------------------
  * INT8 datapath.
  *
  * A and B stream as int8; the matrix engine accumulates in int32 internally,
  * but each tile SATURATES its full-K result down to int8 (aie::accum::to_vector
  * <int8>) before it is written to the C window. Because the systolic C path
  * only *gathers* (concatenates) each tile's already-final result — it never
  * re-accumulates across tiles — an int8 C stream is correct end to end. This
  * mirrors aiehlc's simplematmul2 (int8-in / int8-out, saturate-at-end) so the
  * two can be compared head to head. Both stream directions now carry int8, so
  * the A/B ("IN") and C ("OUT") windows are the same width.
  * ---------------------------------------------------------------------------
  */
 #define GEMM_AB_T int8_t   // A / B element type (streamed in)
 #define GEMM_C_T  int8_t   // C result element type (int32 accum saturated -> int8 out)

 // Per-window element count is WIN_SIZE for both directions; byte sizes differ.
 #define WIN_SIZE_BYTES_AB (WIN_SIZE * (int)sizeof(GEMM_AB_T))
 #define WIN_SIZE_BYTES_C  (WIN_SIZE * (int)sizeof(GEMM_C_T))
 // Legacy alias (kept for any 32-bit-oriented callers): result window byte size.
 #define WIN_SIZE_BYTES WIN_SIZE_BYTES_C

 // Vector widths per direction: 256-bit int8 loads on the A/B side, 256-bit
 // int8 stores on the C side (both are int8 now, so 32 lanes each).
 #define VEC_IN  32   // int8 lanes per vector on the A/B stream
 #define VEC_OUT 32   // int8 lanes per vector on the C stream
 #define VECTOR_LENGTH VEC_IN

 #define NUM_ROWS_PER_HW_ROW (NUM_ROWS / NUM_HW_ROWS)
 #define NUM_ROWS_PER_TILE (NUM_ROWS_PER_HW_ROW / NUM_HW_COLS)

 /*
  * ---------------------------------------------------------------------------
  * Matrix-engine (aie::mmul) tiling for the on-core INT32 GEMM compute.
  *
  * The per-tile compute multiplies a local A stripe (NUM_ROWS_PER_TILE rows x
  * NUM_COLS K) by the streamed B columns. Instead of vector-MAC + horizontal
  * reduce per output element, we map the block onto the AIE-ML matrix engine via
  * aie::mmul<MM_M,MM_K,MM_N,uint32,uint32> which emits an MM_M x MM_N C tile per
  * call (no per-element reduce).
  *
  * Shape 4x4x4 chosen (see aie2/mmul_32_32.hpp <4,4,4> specialization). The
  * streaming protocol (B columns in/out) and the result write order are
  * unchanged, so the host (graph.cpp) and downstream cores are unaffected.
  * ---------------------------------------------------------------------------
  */
 // INT8 matrix-engine shape (aie2p/aie2ps mmul_8_8 <4,16,8> specialization,
 // int8 x int8 -> int32 accumulate). K=16 so each per-row A load is a full
 // 128-bit int8 sub-vector (the AIE API requires concat sub-vectors >= 128b);
 // K=8 (64-bit rows) is rejected by aie::concat. PANEL_SIZE (256) is divisible
 // by 16 and 8, so all derived K-step / N-block / flush counts stay integral.
 #define MM_M 4
 #define MM_K 16
 #define MM_N 8

 /*
  * CEILING PROBE (compute-ceiling experiment).
  *
  * When CEILING_REPEAT > 1, each tile runs the matrix-engine GEMM for every
  * N-block CEILING_REPEAT times back-to-back on its LOCAL static buffers
  * (a[], bbuf, intrmdtResult) before reading the next window. Because the
  * compute is idempotent (same locals -> same result), the output stays correct
  * and verification still passes; we simply do CEILING_REPEAT x the MAC work
  * between window ops. As CEILING_REPEAT grows, input-feed and lock-stall
  * overhead amortize away and the perf counters converge on the kernel's pure
  * compute ceiling. The host report scales its FLOP accounting by this factor.
  *
  * Keep at 1 for normal (streaming-bound) runs.
  */
 #ifndef CEILING_REPEAT
 #define CEILING_REPEAT 1
 #endif
 // M-subtiles per tile (rows / MM_M), K-steps over the contraction, N-blocks over
 // the streamed columns, and how many N-blocks fill one WIN_SIZE result flush.
 #define MM_M_SUB (NUM_ROWS_PER_TILE / MM_M)
 #define MM_K_STEPS (NUM_COLS / MM_K)
 #define MM_N_BLOCKS (NUM_COLS / MM_N)
 #define MM_NBLK_PER_FLUSH ((WIN_SIZE / NUM_ROWS_PER_TILE) / MM_N)

 #define NUM_ELMNTS (NUM_ROWS * NUM_COLS)
 // Host DMA byte sizes: an A stripe streams in as int8, the C result streams
 // out as int32 (same element count, MAT_A_CHUNK).
 #define MAT_A_CHUNK_BYTES_IN  (MAT_A_CHUNK * (int)sizeof(GEMM_AB_T))
 #define MAT_A_CHUNK_BYTES_OUT (MAT_A_CHUNK * (int)sizeof(GEMM_C_T))
 // Per-core local A storage (the "A stripe" each tile holds). Sized purely from
 // the panel dimensions, so it never grows with MATRIX_SIZE.
 #define NUM_A_ELMNTS_PER_TILE ((NUM_ROWS_PER_HW_ROW * NUM_COLS) / NUM_HW_COLS)
 #define A_STRIPE_ELEMS NUM_A_ELMNTS_PER_TILE
 #define A_STRIPE_ROWS NUM_ROWS_PER_TILE

 // One chain's slice of an A panel (NUM_ROWS_PER_HW_ROW rows x panel columns).
 #define MAT_A_CHUNK (NUM_ROWS_PER_HW_ROW * NUM_COLS)
 #define MAT_A_CHUNK_SIZE (MAT_A_CHUNK * sizeof(int))

 #endif
