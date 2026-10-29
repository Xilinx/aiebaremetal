// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
/*
(c) Copyright 2019 - 2021 Xilinx, Inc. All Rights reserved.
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
#pragma once

#include "conv2d_bf16_params.h"
#include "mllib_const.h"
#include "aie.hpp"
#include "aie_adf.hpp"
#include "types.hpp"
#include "utils.h"
using namespace aie;
#include <cstdint>
#include <type_traits>

// bfloat16 helper function 
// TODO:(zhonglin) refactor bfloat16 kernel body
#ifdef __X86SIM__
#define MY_LOAD_V my_load_v
#else
#define MY_LOAD_V aie::load_v
#endif

#define INCR_ITR3D_PARAM_BF16(p_in, iterator_name, param)                                                        \
    p_in = ADD_3D_BYTE(p_in, param.iterator_name##_incr2 * 2, param.iterator_name##_wrap0, iterator_name##_cnt0, \
                       param.iterator_name##_incr0 * 2, param.iterator_name##_wrap1, iterator_name##_cnt1,       \
                       param.iterator_name##_incr1 * 2);
#define INCR_ITR3D_BF16(p_in, iterator_name)                                                         \
    p_in = ADD_3D_BYTE(p_in, iterator_name##_incr2 * 2, iterator_name##_wrap0, iterator_name##_cnt0, \
                       iterator_name##_incr0 * 2, iterator_name##_wrap1, iterator_name##_cnt1,       \
                       iterator_name##_incr1 * 2);
#define INCR_ITR2D_PARAM_BF16(p_in, iterator_name, param)                                                        \
    p_in = ADD_2D_BYTE(p_in, param.iterator_name##_incr1 * 2, param.iterator_name##_wrap0, iterator_name##_cnt0, \
                       param.iterator_name##_incr0 * 2);
#define INCR_ITR2D_BF16(p_in, iterator_name)                                                         \
    p_in = ADD_2D_BYTE(p_in, iterator_name##_incr1 * 2, iterator_name##_wrap0, iterator_name##_cnt0, \
                       iterator_name##_incr0 * 2);

template <int N>
__attribute__((always_inline)) aie::vector<bfloat16, N> my_load_v(bfloat16* ptr) {
#ifdef __X86SIM__
    aie::vector<bfloat16, N> res;

    int tmp_val = (long)ptr;
    tmp_val %= 32;
    ptr -= (tmp_val / 2);

    for (int i = 0; i < N; i++) {
        res[i] = *(ptr + i);
    }

    return res;
#else
    return aie::load_v<N>(ptr);
#endif
}

inline __attribute__((always_inline)) void ld_b(accum<accfloat, 16>& b0,
                                                accum<accfloat, 16>& b1,
                                                int shift,
                                                bfloat16 WT_DM_BANK* ptr) {
    aie::vector<bfloat16, 16> t0;
    aie::vector<bfloat16, 8> tmp = load_v<8>(ptr);
    t0.insert(0, tmp);
    t0.insert(1, tmp);
    aie::vector<bfloat16, 16> t1 = t0;
    auto t01 = aie::interleave_unzip(t0, t1, 4);
    b0 = aie::from_vector<accfloat>(t01.first, shift);
    b1 = aie::from_vector<accfloat>(t01.second, shift);
}

inline __attribute__((always_inline)) void leaky_relu_compute(accum<accfloat, 16>& input0,
                                                            accum<accfloat, 16>& input1,
                                                            bfloat16 alpha,
                                                            bfloat16* p_out) {
        vector<bfloat16, 16> input_vec0 = input0.to_vector<bfloat16>();
        accum<accfloat,16> acc0 = mul<accfloat,vector<bfloat16,16>,bfloat16>(input_vec0, alpha);
        vector<bfloat16,16> alpha_vec0  = acc0.to_vector<bfloat16>();
        vector<bfloat16,16> result_vec0 = max<vector<bfloat16,16>,vector<bfloat16,16>>(input_vec0,alpha_vec0);
        store_v(p_out,result_vec0);p_out = byte_incr(p_out, 32);

        vector<bfloat16, 16> input_vec1 = input1.to_vector<bfloat16>();
        accum<accfloat,16> acc1 = mul<accfloat,vector<bfloat16,16>,bfloat16>(input_vec1, alpha);
        vector<bfloat16,16> alpha_vec1  = acc1.to_vector<bfloat16>();
        vector<bfloat16,16> result_vec1 = max<vector<bfloat16,16>,vector<bfloat16,16>>(input_vec1,alpha_vec1);
        store_v(p_out,result_vec1);
}
template <int VecLen>
inline __attribute__((always_inline)) void leaky_relu_compute(vector<bfloat16, VecLen>& input0,
                                                            bfloat16 alpha,
                                                            bfloat16* p_out) {
        accum<accfloat,VecLen> acc0 = mul<accfloat,vector<bfloat16,VecLen>,bfloat16>(input0, alpha);
        vector<bfloat16,VecLen> alpha_vec0  = acc0.template to_vector<bfloat16>();
        vector<bfloat16,VecLen> result_vec0 = max<vector<bfloat16,VecLen>,vector<bfloat16,VecLen>>(input0,alpha_vec0);
        store_v(p_out,result_vec0);
}