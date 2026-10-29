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


#pragma once

#include "aie.hpp"
#include <adf.h>
#include <aie_api/utils.hpp>

#include "utils.h"
#include "mllib_const.h"

using namespace aie;

#ifndef GROUPG4_LP_SIZE
#define GROUPG4_LP_SIZE 4
#endif

struct alignas(v32uint16) groupg4_params {
    uint32_t d0_size;
    uint32_t d1_size;
    uint32_t d2_size;
    uint32_t group_dim;

    uint32_t num_bytes;
    uint32_t num_iters;
    uint32_t out_stride;
    DECL_ITR3D(iters_in)
};
typedef struct groupg4_params groupg4_params_t;

template <typename dtype>
NOINLINE void setup_groupg4_params(groupg4_params_t &param, const uint32_t (&lp)[GROUPG4_LP_SIZE]) {
    const uint32_t *__restrict rtps = lp;
    param.d0_size = rtps[0];
    param.d1_size = rtps[1];
    param.d2_size = rtps[2];
    param.group_dim = rtps[3];
    param.num_bytes = param.d0_size * param.d1_size * param.d2_size * sizeof(dtype);
    if (param.group_dim == 0) {
        // *** (d1_size * d2_size *sizeof(dtype)) MUST be multiple of 32bytes ***
        // process 2*64B data each time
        param.num_iters = param.num_bytes / 64 / 2;
        param.out_stride = 64;
        param.iters_in_incr0 = param.d1_size * param.d2_size * sizeof(dtype);
        param.iters_in_wrap0 = 4 - 1;
        param.iters_in_incr1 = 32 - 3 * param.iters_in_incr0;
        param.iters_in_wrap1 = param.iters_in_incr0 / 32 - 1;
        param.iters_in_incr2 = 32;
    } else if (param.group_dim == 1) {
        // *** (d2_size * sizeof(dtype)) MUST be multiple of 16bytes ***
        // process 64B each time
        param.num_iters = param.num_bytes / 64;
        param.out_stride = 64;
        param.iters_in_incr0 = param.d2_size * sizeof(dtype);
        param.iters_in_wrap0 = 4 - 1;
        param.iters_in_incr1 = 16 - 3 * param.iters_in_incr0;
        param.iters_in_wrap1 = param.iters_in_incr0 / 16 - 1;
        param.iters_in_incr2 = 16;
    } else if (param.group_dim == 2) {
        param.num_iters = param.num_bytes / 64;
    }
}
