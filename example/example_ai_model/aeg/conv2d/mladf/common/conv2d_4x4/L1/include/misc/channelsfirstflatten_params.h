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

#ifndef __CHANNELSFIRSTFLATTEN_PARAMS_H__
#define __CHANNELSFIRSTFLATTEN_PARAMS_H__

#include "aie.hpp"
#include <adf.h>
#include "utils.h"

#ifndef CHANNELSFIRSTFLATTEN_ST_VEC_LEN
#define CHANNELSFIRSTFLATTEN_ST_VEC_LEN 16
#endif

#ifndef CHANNELSFIRSTFLATTEN_LP_SIZE
#define CHANNELSFIRSTFLATTEN_LP_SIZE 4
#endif

struct alignas(v32uint16) channelsfirstflatten_internal_params
{
    uint16_t cols;
    uint16_t rows;
    uint16_t depths;
    uint8_t elem_bytes;

    DECL_ITR3D(iterator_inner_in)
    DECL_ITR2D(iterator_outer_out)

    uint32_t inner_loop_nm;
    uint32_t outer_loop_nm;
    bool is_store_tail;

    int outer_in_inc0;
};

typedef struct channelsfirstflatten_internal_params channelsfirstflatten_params_t;

// function below is with actual RTPs
static NOINLINE void load_channelsfirstflatten_rtp(channelsfirstflatten_params_t &param, const uint32_t (&lp)[CHANNELSFIRSTFLATTEN_LP_SIZE])
{
    param.cols = lp[0];
    param.rows = lp[1];
    param.depths = lp[2];
    param.elem_bytes = lp[3];
}

static NOINLINE void setup_channelsfirstflatten(channelsfirstflatten_params_t &param)
{

    // outer loop
    int outer_tail_nm = param.cols * param.rows * 8 % CHANNELSFIRSTFLATTEN_ST_VEC_LEN;
    bool is_store_tail = (outer_tail_nm == 8) ? true : false;// false will jump back
    param.is_store_tail = is_store_tail;

    // inner loop
    param.inner_loop_nm = param.cols * param.rows * 8 / CHANNELSFIRSTFLATTEN_ST_VEC_LEN;
    // inner loop input iter setup
    int inner_in_inc1 = 8 * param.elem_bytes; //C8 from front layer conv2d
    int inner_in_num1 = param.cols - 1;
    int inner_in_inc2 = (param.cols * param.depths - param.cols * 8 + 8) * param.elem_bytes;
    int inner_in_num2 = param.rows - 1;
    int inner_in_inc3 = (1 - (param.cols - 1) * 8 - (param.rows - 1) * param.cols * param.depths) * param.elem_bytes;
    SETUP_ITR3D_PARAM(iterator_inner_in, inner_in_inc1, inner_in_num1, inner_in_inc2, inner_in_num2, inner_in_inc3, param)

    // outer loop input byte jump setup
    // 3D ADD BYTE has jump to +1 step inc3, so just continue 4D jump address
    param.outer_in_inc0 = ((param.cols - 1) * 8) * param.elem_bytes;// just for aie2 bf16

    // outer loop
    param.outer_loop_nm = param.depths / 8;
    int outer_out_inc1 = is_store_tail ? (CHANNELSFIRSTFLATTEN_ST_VEC_LEN * param.elem_bytes) : (0);
    int outer_out_num1 = CHANNELSFIRSTFLATTEN_ST_VEC_LEN / 8 - 1; //st_len / c8 - 1
    int outer_out_inc2 = is_store_tail ? (0) : (0);
    SETUP_ITR2D_PARAM(iterator_outer_out, outer_out_inc1, outer_out_num1, outer_out_inc2, param) 
}

// With RTP
static NOINLINE void setup_channelsfirstflatten_params(channelsfirstflatten_params_t &param, const uint32_t (&lp)[CHANNELSFIRSTFLATTEN_LP_SIZE])
{
    load_channelsfirstflatten_rtp(param, lp);
    setup_channelsfirstflatten(param);
}

#endif //__CHANNELSFIRSTFLATTEN_PARAMS_H__
