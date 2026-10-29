// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
/*
(c) Copyright 2014 - 2021 Xilinx, Inc. All Rights reserved.
(c) Copyright 2022 - 2024 Advanced Micro Devices, Inc. All Rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */

#ifndef __TILE_PARAMS_H__
#define __TILE_PARAMS_H__
#include "utils.h"

// Buffer placement directives for IFM, WTs and OFM buffers.
#ifndef IFM_DM_BANK
#define IFM_DM_BANK __aie_dm_resource_a
#endif
#ifndef WT_DM_BANK
#define WT_DM_BANK __aie_dm_resource_a
#endif
#ifndef OFM_DM_BANK
#define OFM_DM_BANK __aie_dm_resource_b
#endif

#ifndef TILE_VEC_LEN
#if __AIE_ARCH__ == 20 // AIE2
#define TILE_VEC_LEN 32
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
#define TILE_VEC_LEN 64
#endif // AIE_ARCH
#endif // TILE_VEC_LEN

#ifndef TILE_LP_SIZE
#define TILE_LP_SIZE 8
#endif

// TODO: need to check if keep_in_registers is required.
struct alignas(v32uint16) tile_internal_params{
    uint16_t in_shape[4];
    uint16_t out_shape[4];
    uint16_t rep_shape[4];
    int16_t ifm_offset[4];
    int16_t ifm_dim_0_incr0;
    int16_t ifm_dim_0_wrap0;
    int16_t ifm_dim_0_incr1;
    int16_t ifm_dim_1_incr0;
    int16_t ifm_dim_1_wrap0;
    int16_t ifm_dim_1_incr1;
    int16_t ifm_dim_2_incr0;
    int16_t ifm_dim_2_wrap0;
    int16_t ifm_dim_2_incr1;
    int16_t ifm_dim_3_incr0;
    int16_t ifm_dim_3_wrap0;
    int16_t ifm_dim_3_incr1;
    int16_t ofm_dim_3_incr0;
    int16_t ofm_dim_3_wrap0;
    int16_t ofm_dim_3_incr1;
    int16_t ifm_inc[4];
};

using tile_params_t = tile_internal_params;

// Compile Time Param Loading

static NOINLINE void load_tile_rtp(uint32_t *params_ptr, tile_params_t &param){
    // dim AxBxCxD -> [0]x[1]x[2]x[3], dim 'D' or [3] is the innermost dim
    param.in_shape[0]       = params_ptr[0];
    param.in_shape[1]       = params_ptr[1];
    param.in_shape[2]       = params_ptr[2];
    param.in_shape[3]       = params_ptr[3];
    param.rep_shape[0]      = params_ptr[4];
    param.rep_shape[1]      = params_ptr[5];
    param.rep_shape[2]      = params_ptr[6];
    param.rep_shape[3]      = params_ptr[7];
}

static NOINLINE void setup_tile(tile_params_t &param){
    param.out_shape[0] = param.in_shape[0] * param.rep_shape[0];
    param.out_shape[1] = param.in_shape[1] * param.rep_shape[1];
    param.out_shape[2] = param.in_shape[2] * param.rep_shape[2];
    param.out_shape[3] = param.in_shape[3] * param.rep_shape[3];

    param.ifm_offset[0] = - param.in_shape[3] * param.in_shape[2] * param.in_shape[1] * (param.in_shape[0] - 1);
    param.ifm_offset[1] = - param.in_shape[3] * param.in_shape[2] * (param.in_shape[1] - 1);
    param.ifm_offset[2] = - param.in_shape[3] * (param.in_shape[2] - 1);
    param.ifm_offset[3] = - (param.in_shape[3] - 1);

    param.ifm_inc[0] = param.in_shape[3] * param.in_shape[2] * param.in_shape[1];
    param.ifm_inc[1] = param.in_shape[3] * param.in_shape[2];
    param.ifm_inc[2] = param.in_shape[3];
    param.ifm_inc[3] = 1;
}

static NOINLINE void setup_tile_params(uint32_t *params_ptr, tile_params_t &param){
    load_tile_rtp(params_ptr, param);
    setup_tile(param);
}

// Run Time Param Loading

static NOINLINE void load_tile_rtp(tile_params_t &param, const uint32_t (&lp)[TILE_LP_SIZE]){
    // dim AxBxCxD -> [0]x[1]x[2]x[3], dim 'D' or [3] is the innermost dim
    param.in_shape[0]       = lp[0];
    param.in_shape[1]       = lp[1];
    param.in_shape[2]       = lp[2];
    param.in_shape[3]       = lp[3];
    param.rep_shape[0]      = lp[4];
    param.rep_shape[1]      = lp[5];
    param.rep_shape[2]      = lp[6];
    param.rep_shape[3]      = lp[7];
}

static NOINLINE void setup_tile_params(tile_params_t &param, const uint32_t (&lp)[TILE_LP_SIZE]){
    load_tile_rtp(param, lp);
    setup_tile(param);
}

#endif  //__TILE_PARAMS_H__