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
    
    
#pragma once
    
#include "aie.hpp"
#include <adf.h>

#include "utils.h"
#include "mllib_const.h"
#include "degroupg8_params.h"
#include <math.h>

using namespace aie;

template<typename dtype>
NOINLINE void degroupg8
(
    dtype __aie_dm_resource_a * __restrict in_ptr,       // Input
    dtype __aie_dm_resource_b * __restrict out_ptr,       // Output
    degroupg8_params_t &param
){

    if (param.group_dim == 0) {
        constexpr uint32_t load_vec_sz = 64 / sizeof(dtype);
        constexpr uint32_t save_vec_sz = 32 / sizeof(dtype);
        constexpr uint32_t first_shuffle_mode = std::is_same_v<dtype, bfloat16> ? T16_4x8 : T8_8x8; 
        SETUP_ITR3D_COUNTERS(iters_out)

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4, ) 
        for (int i = 0; i < param.num_iters; i++){
            vector<dtype, load_vec_sz> iv1, iv2, iv3, iv4, tv1, tv2, tv3, tv4, ov1, ov2, ov3, ov4;

            iv1 = load_v<load_vec_sz>(in_ptr); in_ptr = byte_incr(in_ptr, param.in_stride);
            iv2 = load_v<load_vec_sz>(in_ptr); in_ptr = byte_incr(in_ptr, param.in_stride);
            iv3 = load_v<load_vec_sz>(in_ptr); in_ptr = byte_incr(in_ptr, param.in_stride);
            iv4 = load_v<load_vec_sz>(in_ptr); in_ptr = byte_incr(in_ptr, param.in_stride);

            ov1 = shuffle(iv1, iv1, first_shuffle_mode);
            ov2 = shuffle(iv2, iv2, first_shuffle_mode);
            ov3 = shuffle(iv3, iv3, first_shuffle_mode);
            ov4 = shuffle(iv4, iv4, first_shuffle_mode);

            tv1 = shuffle(ov1, ov2, T64_2x8_lo);
            tv2 = shuffle(ov1, ov2, T64_2x8_hi);
            tv3 = shuffle(ov3, ov4, T64_2x8_lo);
            tv4 = shuffle(ov3, ov4, T64_2x8_hi);

            ov1 = shuffle(tv1, tv3, T128_2x4_lo);
            ov2 = shuffle(tv1, tv3, T128_2x4_hi);
            ov3 = shuffle(tv2, tv4, T128_2x4_lo);
            ov4 = shuffle(tv2, tv4, T128_2x4_hi);

            store_v(out_ptr, ov1.template extract<save_vec_sz>(0)); INCR_ITR3D_PARAM(out_ptr, iters_out, param)
            store_v(out_ptr, ov1.template extract<save_vec_sz>(1)); INCR_ITR3D_PARAM(out_ptr, iters_out, param)
            store_v(out_ptr, ov2.template extract<save_vec_sz>(0)); INCR_ITR3D_PARAM(out_ptr, iters_out, param)
            store_v(out_ptr, ov2.template extract<save_vec_sz>(1)); INCR_ITR3D_PARAM(out_ptr, iters_out, param)
            store_v(out_ptr, ov3.template extract<save_vec_sz>(0)); INCR_ITR3D_PARAM(out_ptr, iters_out, param)
            store_v(out_ptr, ov3.template extract<save_vec_sz>(1)); INCR_ITR3D_PARAM(out_ptr, iters_out, param)
            store_v(out_ptr, ov4.template extract<save_vec_sz>(0)); INCR_ITR3D_PARAM(out_ptr, iters_out, param)
            store_v(out_ptr, ov4.template extract<save_vec_sz>(1)); INCR_ITR3D_PARAM(out_ptr, iters_out, param)
        }
    } 
    else if (param.group_dim == 1) {
        constexpr uint32_t load_vec_sz = 64 / sizeof(dtype);
        constexpr uint32_t save_vec_sz = 16 / sizeof(dtype);
        constexpr uint32_t first_shuffle_mode = std::is_same_v<dtype, bfloat16> ? T16_4x8 : T8_8x8; 
        SETUP_ITR3D_COUNTERS(iters_out)

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for (int i = 0; i < param.num_iters; i++){
            vector<dtype, load_vec_sz> iv1, iv2, tv1, tv2, ov1, ov2;

            iv1 = load_v<load_vec_sz>(in_ptr); in_ptr = byte_incr(in_ptr, param.in_stride);
            iv2 = load_v<load_vec_sz>(in_ptr); in_ptr = byte_incr(in_ptr, param.in_stride);

            tv1 = shuffle(iv1, iv1, first_shuffle_mode);
            tv2 = shuffle(iv2, iv2, first_shuffle_mode);

            ov1 = shuffle(tv1, tv2, T64_2x8_lo);
            ov2 = shuffle(tv1, tv2, T64_2x8_hi);

            store_v(out_ptr, ov1.template extract<save_vec_sz>(0)); INCR_ITR3D_PARAM(out_ptr, iters_out, param);
            store_v(out_ptr, ov1.template extract<save_vec_sz>(1)); INCR_ITR3D_PARAM(out_ptr, iters_out, param);
            store_v(out_ptr, ov1.template extract<save_vec_sz>(2)); INCR_ITR3D_PARAM(out_ptr, iters_out, param);
            store_v(out_ptr, ov1.template extract<save_vec_sz>(3)); INCR_ITR3D_PARAM(out_ptr, iters_out, param);
            store_v(out_ptr, ov2.template extract<save_vec_sz>(0)); INCR_ITR3D_PARAM(out_ptr, iters_out, param);
            store_v(out_ptr, ov2.template extract<save_vec_sz>(1)); INCR_ITR3D_PARAM(out_ptr, iters_out, param);
            store_v(out_ptr, ov2.template extract<save_vec_sz>(2)); INCR_ITR3D_PARAM(out_ptr, iters_out, param);
            store_v(out_ptr, ov2.template extract<save_vec_sz>(3)); INCR_ITR3D_PARAM(out_ptr, iters_out, param);
        }
    }
    else if (param.group_dim == 2) {
        constexpr uint32_t vec_sz = 64 / sizeof(dtype);
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(8,) 
        for (int i = 0; i < param.num_iters; i++){
            vector<dtype, vec_sz> vec;
            vec = load_v<vec_sz>(in_ptr);
            in_ptr = byte_incr(in_ptr, 64);
            store_v(out_ptr, vec);
            out_ptr = byte_incr(out_ptr, 64);
        }
    }

    return;
}


template<typename dtype, typename config_ifm, typename config_ofm>
INLINE void degroupg8
(
    adf::input_buffer_conf<dtype, config_ifm>   & __restrict ifm,       // Input
    adf::output_buffer_conf<dtype, config_ofm>  & __restrict ofm,       // Output
    degroupg8_params_t &param
){
    dtype __aie_dm_resource_a * __restrict in_ptr  = (dtype __aie_dm_resource_a * __restrict) ifm.data();
    dtype __aie_dm_resource_b * __restrict out_ptr = (dtype __aie_dm_resource_b * __restrict) ofm.data();
    return degroupg8<dtype>(in_ptr, out_ptr, param);
}
