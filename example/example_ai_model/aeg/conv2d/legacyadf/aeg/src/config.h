// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef __CONFIG__
#define __CONFIG__
// #define HW_TEST
#if defined(HW_TEST)
#define WIN_SIZE 256 //
#define DATA_LENGTH (1200 * 1200)
#else
#define WIN_SIZE 32 //
#define DATA_LENGTH 128
#endif

#define IFM_H_SIZE 224
#define IFM_W_SIZE 224
#define IFM_INPUT_C_SIZE 3
#define IFM_PAD_SIZE 3
#define IFM_K_SIZE 7
#define STRIDE_SIZE 2
#define OFM_H_SIZE (((IFM_H_SIZE + 2 * IFM_PAD_SIZE - 7) / STRIDE_SIZE) + 1)
#define OFM_W_SIZE (((IFM_W_SIZE + 2 * IFM_PAD_SIZE - 7) / STRIDE_SIZE) + 1)

#define IFM_H_AFTER_PAD_SIZE (IFM_H_SIZE + 2 * IFM_PAD_SIZE)
#define IFM_W_AFTER_PAD_SIZE (IFM_W_SIZE + 2 * IFM_PAD_SIZE)

#define TILE_BLOCK_H                                                                                                   \
    ((((IFM_H_SIZE + 2 * IFM_PAD_SIZE - IFM_K_SIZE) / STRIDE_SIZE + 1) / P_COL_NUM_USED) * STRIDE_SIZE +               \
     2 * IFM_PAD_SIZE)

#define CORE_NUM_PER_COL 2
#define P_COL_NUM_USED 4

#define IFM_SIZE ((IFM_K_SIZE) * (IFM_K_SIZE) * (IFM_INPUT_C_SIZE))
#define WGT_MAX_FRAME_NUM 8
#define WGT_PERCORE_MAX_KERNEL_COUNT 8

#endif
