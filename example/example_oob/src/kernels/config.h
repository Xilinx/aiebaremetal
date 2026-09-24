/******************************************************************************
 * Copyright (C) 2019 - 2022 Xilinx, Inc.  All rights reserved.
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#ifndef XGEMM_CONFIG_H
#define XGEMM_CONFIG_H

#define _AIE_GEN1_ 10
#define AIEML 20
#define AIE2PS 22

// #define _PALMYRA_

// #define _PALMYRA_
//  Below configuration works for both APU and RPU
#ifdef _APURPU_
#define NUM_HW_COLS 30
#if (__AIE_ARCH__ == AIE2PS)
#define NUM_HW_ROWS 4
#else //(__AIE_ARCH__ == AIEML||_AIE_GEN1_)
#define NUM_HW_ROWS 8
#endif
#define MAT_SIZE 480
#define WIN_SIZE 480

// Below configuration works for APU only
#else
#if (__AIE_ARCH__ == _AIE_GEN1_)
#define NUM_HW_COLS 50
#define NUM_HW_ROWS 8
#define MAT_SIZE 1200
#define WIN_SIZE 600
#elif (__AIE_ARCH__ == AIEML)
#define NUM_HW_COLS 38
#define NUM_HW_ROWS 8
#define MAT_SIZE 608
#define WIN_SIZE 608
#else            //(__AIE_ARCH__ == AIE2ps)
#ifdef _PALMYRA_ // coroperate with prebuild palmyra 1-11 col pdi
#define NUM_HW_COLS 12
#else
#define NUM_HW_COLS 36
#endif
#define NUM_HW_ROWS 4

#define MAT_SIZE 576
#define WIN_SIZE 576
#endif
#endif

#define NUM_COLS MAT_SIZE
#define NUM_ROWS MAT_SIZE
#define WIN_SIZE_BYTES (WIN_SIZE * sizeof(int))

#define VECTOR_LENGTH 8

#define NUM_ROWS_PER_HW_ROW (NUM_ROWS / NUM_HW_ROWS)
#define NUM_ROWS_PER_TILE (NUM_ROWS_PER_HW_ROW / NUM_HW_COLS)

#define NUM_ELMNTS (NUM_ROWS * NUM_COLS)
#define NUM_A_ELMNTS_PER_TILE ((NUM_ROWS_PER_HW_ROW * NUM_COLS) / NUM_HW_COLS)

#define MAT_A_CHUNK (NUM_ROWS_PER_HW_ROW * NUM_COLS)
#define MAT_A_CHUNK_SIZE (MAT_A_CHUNK * sizeof(int))

#endif
