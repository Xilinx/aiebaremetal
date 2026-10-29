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

#include "utils.h"
#include "mllib_const.h"
#include "groupg4_params.h"
#include <math.h>

using namespace aie;

template<typename dtype>
NOINLINE void groupg4
(
    dtype __aie_dm_resource_a * __restrict in_ptr,       // Input
    dtype __aie_dm_resource_b * __restrict out_ptr,      // Output
    groupg4_params_t &param
){
    //printf("==> d0=%d, d1=%d, d2=%d, grp_dim=%d, num_iters=%d\n", param.d0_size, param.d1_size, param.d2_size, param.group_dim, param.num_iters);

    if (param.group_dim == 0) {
        constexpr uint32_t vec_sz = 64 / sizeof(dtype);
        constexpr uint32_t load_sz = 64 / 2 / sizeof(dtype);
        constexpr uint32_t final_shuffle_mode = std::is_same_v<dtype, bfloat16> ? T16_4x8 : T8_4x16; 
        SETUP_ITR3D_COUNTERS(iters_in)

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4, ) 
        for (int i = 0; i < param.num_iters; i++){
            vector<dtype, vec_sz> iv1, iv2, tv1, tv2, ov1, ov2;

            iv1.insert(0, load_v<load_sz>(in_ptr)); INCR_ITR3D_PARAM(in_ptr, iters_in, param)
            iv1.insert(1, load_v<load_sz>(in_ptr)); INCR_ITR3D_PARAM(in_ptr, iters_in, param)
            iv2.insert(0, load_v<load_sz>(in_ptr)); INCR_ITR3D_PARAM(in_ptr, iters_in, param)
            iv2.insert(1, load_v<load_sz>(in_ptr)); INCR_ITR3D_PARAM(in_ptr, iters_in, param)

            tv1 = shuffle(iv1, iv2, T128_4x2_lo);
            tv2 = shuffle(iv1, iv2, T128_4x2_hi);

            ov1 = shuffle(tv1, tv1, final_shuffle_mode);
            ov2 = shuffle(tv2, tv2, final_shuffle_mode);

            store_v(out_ptr, ov1); out_ptr = byte_incr(out_ptr, param.out_stride);
            store_v(out_ptr, ov2); out_ptr = byte_incr(out_ptr, param.out_stride);
        }
    } 
    else if (param.group_dim == 1) {
        constexpr uint32_t vec_sz = 64 / sizeof(dtype);
        constexpr uint32_t load_sz = 64 / 4 / sizeof(dtype);
        constexpr uint32_t shuffle_mode = std::is_same_v<dtype, bfloat16> ? T16_4x8 : T8_4x16; 
        SETUP_ITR3D_COUNTERS(iters_in)

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for (int i = 0; i < param.num_iters; i++){
            vector<dtype, vec_sz> iv1, ov1;

            iv1.insert(0, load_v<load_sz>(in_ptr)); INCR_ITR3D_PARAM(in_ptr, iters_in, param)
            iv1.insert(1, load_v<load_sz>(in_ptr)); INCR_ITR3D_PARAM(in_ptr, iters_in, param)
            iv1.insert(2, load_v<load_sz>(in_ptr)); INCR_ITR3D_PARAM(in_ptr, iters_in, param)
            iv1.insert(3, load_v<load_sz>(in_ptr)); INCR_ITR3D_PARAM(in_ptr, iters_in, param)

            ov1 = shuffle(iv1, iv1, shuffle_mode);

            store_v(out_ptr, ov1); out_ptr = byte_incr(out_ptr, param.out_stride);
        }
    }
    else if (param.group_dim == 2) {
        constexpr uint32_t vec_sz = 64 / sizeof(dtype);
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(8,) 
        for (int i = 0; i < param.num_iters; i++){
            vector<dtype, vec_sz> vec;
            vec = load_v<vec_sz>(in_ptr); in_ptr = byte_incr(in_ptr, 64);
            store_v(out_ptr, vec); out_ptr = byte_incr(out_ptr, 64);
        }
    }

    return;
}


template<typename dtype, typename config_ifm, typename config_ofm>
INLINE void groupg4
(
    adf::input_buffer_conf<dtype, config_ifm>   & __restrict ifm,       // Input
    adf::output_buffer_conf<dtype, config_ofm>  & __restrict ofm,       // Output
    groupg4_params_t &param
){
    dtype __aie_dm_resource_a * __restrict in_ptr  = (dtype __aie_dm_resource_a * __restrict) ifm.data();
    dtype __aie_dm_resource_b * __restrict out_ptr = (dtype __aie_dm_resource_b * __restrict) ofm.data();
    return groupg4<dtype>(in_ptr, out_ptr, param);
}
