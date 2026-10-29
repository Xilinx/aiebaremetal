/*
    Copyright (C) 2014 - 2022 Xilinx, Inc. All rights reserved.
    Copyright (C) 2025 Advanced Micro Devices, Inc.

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

#ifndef __PIXELUNSHUFFLE_PARAMS_H__
#define __PIXELUNSHUFFLE_PARAMS_H__

#include "aie.hpp"
#include <adf.h>
#include "utils.h"

#ifndef ST_VEC_LEN
#define ST_VEC_LEN 16
#endif

#ifndef PIXELUNSHUFFLE_LP_SIZE
#define PIXELUNSHUFFLE_LP_SIZE 5
#endif

struct alignas(v32uint16) pixelunshuffle_internal_params
{
    uint16_t cols;
    uint16_t rows;
    uint16_t depths;
    uint8_t factor;
    uint8_t elem_bytes;

    DECL_ITR3D(iterator_inner_in)
    DECL_ITR2D(iterator_inner_out)
    DECL_ITR2D(iterator_outer_out)

    uint32_t inner_loop_nm;
    uint32_t outer_loop_nm;

    int outer_in_inc0;
};

typedef struct pixelunshuffle_internal_params pixelunshuffle_params_t;

// function below is with actual RTPs
static NOINLINE void load_pixelunshuffle_rtp(pixelunshuffle_params_t &param, const uint32_t (&lp)[PIXELUNSHUFFLE_LP_SIZE])
{
    param.cols = lp[0];
    param.rows = lp[1];
    param.depths = lp[2];
    param.factor = lp[3];
    param.elem_bytes = lp[4];
}

static NOINLINE void setup_pixelunshuffle(pixelunshuffle_params_t &param)
{
    // inner loop
    param.inner_loop_nm = param.cols / ST_VEC_LEN * param.rows / param.factor;
    // inner loop input iter setup
    int inner_in_inc1 = param.elem_bytes * param.factor;
    int inner_in_num1 = param.cols / param.factor - 1;
    int inner_in_inc2 = -param.elem_bytes * (param.cols / param.factor - 1) * param.factor + param.elem_bytes;
    int inner_in_num2 = param.factor - 1;
    int inner_in_inc3 = param.elem_bytes - param.cols * param.elem_bytes + param.cols * param.depths * param.factor * param.elem_bytes;
    SETUP_ITR3D_PARAM(iterator_inner_in, inner_in_inc1, inner_in_num1, inner_in_inc2, inner_in_num2, inner_in_inc3, param)
    // inner loop output iter setup
    int inner_out_inc1 = param.elem_bytes * ST_VEC_LEN;
    int inner_out_num1 = param.cols / ST_VEC_LEN - 1;
    int inner_out_inc2 = param.elem_bytes * ST_VEC_LEN - param.cols * param.elem_bytes + param.cols * param.depths * param.factor * param.elem_bytes;
    SETUP_ITR2D_PARAM(iterator_inner_out, inner_out_inc1, inner_out_num1, inner_out_inc2, param)

    // outer loop
    param.outer_loop_nm = param.factor * param.depths;
    // outer loop input byte jump setup
    param.outer_in_inc0 = -(param.rows * param.cols * param.depths * param.elem_bytes) + param.cols * param.elem_bytes;
    // outer loop output iter setup
    int outer_out_inc1 = -(param.rows * param.cols * param.depths * param.elem_bytes) + param.cols * param.factor * param.elem_bytes;
    int outer_out_num1 = param.depths - 1;
    int outer_out_inc2 = -(param.rows * param.cols * param.depths * param.elem_bytes) - (param.depths - 2) * param.factor * param.cols * param.elem_bytes - (param.factor - 1) * param.cols * param.elem_bytes;
    SETUP_ITR2D_PARAM(iterator_outer_out, outer_out_inc1, outer_out_num1, outer_out_inc2, param)
}

// With RTP
static NOINLINE void setup_pixelunshuffle_params(pixelunshuffle_params_t &param, const uint32_t (&lp)[PIXELUNSHUFFLE_LP_SIZE])
{
    load_pixelunshuffle_rtp(param, lp);
    setup_pixelunshuffle(param);
}

#endif //__PIXELUNSHUFFLE_PARAMS_H__
