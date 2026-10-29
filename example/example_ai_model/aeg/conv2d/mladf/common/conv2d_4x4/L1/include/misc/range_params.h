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

#ifndef __RANGE_PARAMS_H__
#define __RANGE_PARAMS_H__

#include "aie.hpp"
#include <adf.h>
#include "utils.h"

#ifndef RANGE_VEC_LEN
#define RANGE_VEC_LEN 64
#endif

#ifndef RANGE_LP_SIZE
#define RANGE_LP_SIZE 5
#endif

#ifndef RANGE_UNROLL
#define RANGE_UNROLL 2
#endif

template <typename dtype>
struct alignas(v32uint16) range_internal_params
{
    dtype start;
    dtype limit;
    dtype delta;

    uint32_t num_iters;
    aie::mask<RANGE_VEC_LEN> last_iter_mask_0 = aie::mask<RANGE_VEC_LEN>(false);
    aie::mask<RANGE_VEC_LEN> last_iter_mask_1 = aie::mask<RANGE_VEC_LEN>(false);

#if RANGE_VEC_LEN == 64
    alignas(aie::vector_decl_align) dtype base[64] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};
#else
    alignas(aie::vector_decl_align) dtype base[32] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
#endif

    dtype vec_len_delta;
    uint32_t shift_in;
    uint32_t shift_out;
};

template <typename dtype>
using range_params_t = range_internal_params<dtype>;

// function below is with actual RTPs
template <typename dtype>
NOINLINE void load_range_rtp(range_params_t<dtype> &param, const uint32_t (&lp)[RANGE_LP_SIZE])
{
    param.start = ((dtype *)(lp))[0];
    param.limit = ((dtype *)(lp + 1))[0];
    param.delta = ((dtype *)(lp + 2))[0];
    param.shift_in = ((uint32_t *)(lp + 3))[0];
    param.shift_out = ((uint32_t *)(lp + 4))[0];
}

static NOINLINE void setup_range_int8(range_params_t<int8> &param)
{
    uint32_t num_elems = (param.limit - param.start) / param.delta + ((param.limit - param.start) % param.delta != 0);

    num_elems = num_elems > 0 ? num_elems : 0;

    uint32_t num_last_elems = num_elems % (RANGE_VEC_LEN * RANGE_UNROLL);
    num_last_elems = num_last_elems > 0 ? num_last_elems : (RANGE_VEC_LEN * RANGE_UNROLL);
    num_last_elems = num_elems > 0 ? num_last_elems : 0;

    for (int i = 0; i < num_last_elems; i++) {
        if (i < 64) {
            param.last_iter_mask_0.set(i);
        } else {
            param.last_iter_mask_1.set(i-64);
        }
    }

    param.num_iters = (num_elems + RANGE_VEC_LEN - 1) / RANGE_VEC_LEN;
    param.num_iters = (param.num_iters + RANGE_UNROLL - 1) / RANGE_UNROLL * RANGE_UNROLL;
    param.vec_len_delta = RANGE_VEC_LEN * param.delta;

    // param.shift_in = 6; //lp[3]; 
    // param.shift_out = 7; //lp[4]; 
}


static NOINLINE void setup_range_bf16(range_params_t<bfloat16> &param)
{
    vector<float, 16> vec_limit = aie::broadcast((float)param.limit);
    vector<float, 16> vec_start = aie::broadcast((float)param.start);
    vector<float, 16> vec_delta = aie::broadcast((float)param.delta);
    aie::vector<float, 16> vec_sub = aie::sub(vec_limit, vec_start);
    accum<accfloat, 16> acc_div = aie::div(vec_sub, vec_delta);
    aie::vector<float, 16> vec_div = acc_div.to_vector<float>();
    vector<float, 16> vec_div_floor = aie::to_float(aie::to_fixed(vec_div, 0), 0);
    auto mask = aie::lt(vec_div_floor, vec_div);
    vector<int32_t, 16> vec_num_elems_floor = aie::to_fixed(vec_div, 0);
    vector<int32_t, 16> vec_num_elems_ceil = aie::add(aie::to_fixed(vec_div, 0), 1);
    vector<int32_t, 16> vec_num_elems = aie::select(vec_num_elems_floor, vec_num_elems_ceil, mask);
    uint32_t num_elems = vec_num_elems[0];
    num_elems = num_elems > 0 ? num_elems : 0;

    uint32_t num_last_elems = num_elems % (RANGE_VEC_LEN * RANGE_UNROLL);
    num_last_elems = num_last_elems > 0 ? num_last_elems : (RANGE_VEC_LEN * RANGE_UNROLL);
    num_last_elems = num_elems > 0 ? num_last_elems : 0;

    for (int i = 0; i < num_last_elems; i++) {
        if (i < 64) {
            param.last_iter_mask_0.set(i);
        } else {
            param.last_iter_mask_1.set(i-64);
        }
    }
    param.num_iters = (num_elems + RANGE_VEC_LEN - 1) / RANGE_VEC_LEN;
    param.num_iters = (param.num_iters + RANGE_UNROLL - 1) / RANGE_UNROLL * RANGE_UNROLL;
    param.vec_len_delta = (bfloat16)RANGE_VEC_LEN * param.delta;
}

// With RTP
template <typename dtype>
NOINLINE void setup_range_params(range_params_t<dtype> &param, const uint32_t (&lp)[RANGE_LP_SIZE])
{
    load_range_rtp(param, lp);
    if constexpr (std::is_same<bfloat16, dtype>::value) {
        setup_range_bf16(param);
    } else {
        setup_range_int8(param);
    }
}

#endif //__RANGE_PARAMS_H__
