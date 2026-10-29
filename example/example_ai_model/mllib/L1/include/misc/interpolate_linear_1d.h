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

#ifndef __INTERPOLATE_LINEAR_1D__
#define __INTERPOLATE_LINEAR_1D__

// #include <iostream>
#include <adf.h>
#include "aie.hpp"
#include "mask.hpp"
#include "utils.h"
#include "aie_api/utils.hpp"
#include "interpolate_linear_1d_params.h"

using namespace aie;

template<typename dtype>
struct interperlation_trait {};

template<>
struct interperlation_trait<int8_t> {
    using ptr_type = int16_t;
    static const int16_t nbyte = 1;
    static const int16_t left_scale = 8;
    static const int16_t left_scale_bit = 3;
};

template<>
struct interperlation_trait<bfloat16> {
    using ptr_type = bfloat16;
    static const int16_t nbyte = 2;
    static const int16_t left_scale = 16;
    static const int16_t left_scale_bit = 4;
};

static INLINE void interpolate_linear_1d_int8
(
    int8_t* input_ptr,
    int8_t* output_ptr,
    int16_t* x_idx_diff_ptr,
    int16_t* x_idx_left_ptr,
    interpolate_linear_1d_params_t &param
) {
    for (uint16_t c=0; c < param.channel_loop_cnt; c++)
    {
        int16_t c_step = c * param.c_step_scale;
        int8_t* input_ptr_cur = byte_incr(input_ptr, c_step);

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(8,) 
        for (uint16_t w=0; w < param.val_loop_cnt; w+=4){
            // input data and idx diff loading
            vector<int16_t, 8> idx_diff = load_unaligned_v<8>(byte_incr(x_idx_diff_ptr,2 * w), 4);
            vector<int16_t, 8> left_idx = load_unaligned_v<8>(byte_incr(x_idx_left_ptr,2 * w), 4);

            vector<int8_t, 16> int_data_pair_0 = load_unaligned_v<16>(byte_incr(input_ptr_cur, left_idx[0]), 8);
            vector<int8_t, 16> int_data_pair_1 = load_unaligned_v<16>(byte_incr(input_ptr_cur, left_idx[1]), 8);
            vector<int8_t, 16> int_data_pair_2 = load_unaligned_v<16>(byte_incr(input_ptr_cur, left_idx[2]), 8);
            vector<int8_t, 16> int_data_pair_3 = load_unaligned_v<16>(byte_incr(input_ptr_cur, left_idx[3]), 8);

            // concat and shuffle input data into separate acc
            // [left_0|right_0][left_1|right_1]... -> [left_0|left_1|...][right_0|right_1...]
            vector<int8_t, 64> full_data_vec = concat(int_data_pair_0, int_data_pair_1, int_data_pair_2, int_data_pair_3);
            full_data_vec = shuffle(full_data_vec, shuffle_down(full_data_vec, 8), T64_8x2_lo);
            accum<acc32, 32> data_output_acc(full_data_vec.extract<32>(0)); // stores left value for now
            accum<acc32, 32> data_diff_acc(full_data_vec.extract<32>(1)); // stores right value for now

            // concat idx_diff data
            auto idx_diff_vec = concat(
                broadcast<int16_t,8>(idx_diff[0]),
                broadcast<int16_t,8>(idx_diff[1]),
                broadcast<int16_t,8>(idx_diff[2]),
                broadcast<int16_t,8>(idx_diff[3])
            );

            // perform the real linear interpolation computation y = x_left + data_diff * idx_diff
            vector<int16_t, 32> data_diff_vec = sub(data_diff_acc, data_output_acc).to_vector<int16_t>();
            data_diff_acc = mul(data_output_acc.to_vector<int16_t>(), (int16_t)SHIFT_DIFF_VALUE); // shift left value by SHIFT_DIFF_BIT
            data_output_acc = mac(data_diff_acc, data_diff_vec, idx_diff_vec);
            auto output_vec = data_output_acc.to_vector<int8_t>(param.output_shift); // param.output_shift already contains SHIFT_DIFF_BIT
            // store 32 int8 -> 256 bits
            store_v(output_ptr, output_vec); output_ptr = byte_incr(output_ptr, 32);
        }
    }
}

static INLINE void interpolate_linear_1d_bfloat16
(
    bfloat16* input_ptr,
    bfloat16* output_ptr,
    bfloat16* x_idx_diff_ptr,
    int16_t* x_idx_left_ptr,
    interpolate_linear_1d_params_t &param
) {
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(1,) 
    for (uint16_t c=0; c<param.channel_loop_cnt; c++){
        uint16_t c_step = c * param.c_step_scale;
        bfloat16* input_ptr_cur = input_ptr + c_step;

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(8,) 
        for (uint16_t w=0; w<param.val_loop_cnt; w+=4){
            vector<bfloat16, 8> idx_diff = load_unaligned_v<8>(byte_incr(x_idx_diff_ptr,2 * w), 4);
            vector<int16_t, 8> left_idx = load_unaligned_v<8>(byte_incr(x_idx_left_ptr,2 * w), 4);
            vector<bfloat16, 16> int_data_pair_0 = load_unaligned_v<16>(byte_incr(input_ptr_cur, left_idx[0]), 8);
            vector<bfloat16, 16> int_data_pair_1 = load_unaligned_v<16>(byte_incr(input_ptr_cur, left_idx[1]), 8);
            vector<bfloat16, 16> int_data_pair_2 = load_unaligned_v<16>(byte_incr(input_ptr_cur, left_idx[2]), 8);
            vector<bfloat16, 16> int_data_pair_3 = load_unaligned_v<16>(byte_incr(input_ptr_cur, left_idx[3]), 8);

            // concat input data into separate acc
            // [left_0|right_0][left_1|right_1]... -> [left_0|left_1|...][right_0|right_1...]
            accum<accfloat, 32> data_output_acc(concat(int_data_pair_0.extract<8>(0), int_data_pair_1.extract<8>(0), int_data_pair_2.extract<8>(0), int_data_pair_3.extract<8>(0))); // stores left value for now
            accum<accfloat, 32> data_diff_acc(concat(int_data_pair_0.extract<8>(1), int_data_pair_1.extract<8>(1), int_data_pair_2.extract<8>(1), int_data_pair_3.extract<8>(1))); // stores right value for now

            // concat idx_diff data
            auto idx_diff_vec = concat(
                broadcast<bfloat16,8>(idx_diff[0]),
                broadcast<bfloat16,8>(idx_diff[1]),
                broadcast<bfloat16,8>(idx_diff[2]),
                broadcast<bfloat16,8>(idx_diff[3])
            );

            // perform the real linear interpolation computation y = x_left + data_diff * idx_diff
            vector<bfloat16, 32> data_diff_vec = sub(data_diff_acc, data_output_acc).to_vector<bfloat16>();
            data_output_acc = mac(data_output_acc, data_diff_vec, idx_diff_vec);

            // store 32 bfloat16 -> 512 bits
            store_v(output_ptr, data_output_acc.to_vector<bfloat16>()); output_ptr = byte_incr(output_ptr, 64);
        }
    }

}

template<typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
NOINLINE void interpolate_linear_1d
(
    dtype __aie_dm_resource_a * __restrict input_ptr,
    dtype __aie_dm_resource_b * __restrict output_ptr,
    interpolate_linear_1d_params_t  &param
) {
    using ptr_type = typename interperlation_trait<dtype>::ptr_type;

    setup_params_iter(param);

    // x_idx_diff_ptr will be quantized to int16_t for storage, saves
    // recomputation of quantization by each param.channel_loop_cnt
    ptr_type* x_idx_diff_ptr = (ptr_type*)byte_incr(output_ptr , param.sv_output_sample_num * interperlation_trait<dtype>::nbyte);
    int16_t* x_idx_left_ptr = (int16_t*)byte_incr(x_idx_diff_ptr ,2 * param.sv_ofm_width );

    // initialize the index vector
    vector<int16_t, VEC_SIZE> x_idx_dst_vec;
    AIE_LOOP_UNROLL(1) 
    for (int16_t i = 0; i < VEC_SIZE; i++){
        x_idx_dst_vec[i] = i;
    }
    x_idx_dst_vec = add(x_idx_dst_vec, (int16_t)param.output_offset);

    // update x_idx_diff, x_left for value interpolation
    accum<accfloat, VEC_SIZE> x_idx_src_acc(broadcast<float, VEC_SIZE>(param.x_idx_offset));
    vector<float, VEC_SIZE> x_idx_src_vec = mac(x_idx_src_acc, to_float(x_idx_dst_vec), param.inv_scale_factor);
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (int16_t i = 0; i < param.sv_ofm_width ; i += VEC_SIZE){
        vector<float, VEC_SIZE> x_idx_src_vec_temp = min(max(x_idx_src_vec, 0.0f), param.clamp_hi);
        x_idx_src_vec = add(x_idx_src_vec, param.x_idx_src_offset);
        vector<int16_t, VEC_SIZE> x_idx_left_vec = to_fixed<int16_t>(sub(x_idx_src_vec_temp, 0.5f));
        vector<float, VEC_SIZE> x_idx_diff_vec = sub(x_idx_src_vec_temp, to_float(x_idx_left_vec));
        x_idx_left_vec = sub(x_idx_left_vec, (int16_t)param.input_offset);
        vector<ptr_type, VEC_SIZE> x_idx_diff_store_vec;
        x_idx_left_vec = upshift(x_idx_left_vec, interperlation_trait<dtype>::left_scale_bit); // left *= 8 or 16
        if constexpr (std::is_same<int8_t, dtype>::value) {
            x_idx_diff_store_vec = to_fixed<int16_t>(x_idx_diff_vec, SHIFT_DIFF_BIT);
        } else {
            accum<accfloat, VEC_SIZE> x_idx_diff_acc(x_idx_diff_vec);
            x_idx_diff_store_vec = x_idx_diff_acc.to_vector<ptr_type>();
        }
        store_v(byte_incr(x_idx_left_ptr, i * 2), x_idx_left_vec);
        store_v(byte_incr(x_idx_diff_ptr ,i * 2), x_idx_diff_store_vec);
    }

    if (param.align_corners && param.cur_iter == param.num_iters) {
        *(x_idx_left_ptr + param.last_out_idx) = param.last_in_idx * interperlation_trait<dtype>::left_scale;
        *(x_idx_diff_ptr + param.last_out_idx) = 0;
    }

    if constexpr (std::is_same<int8_t, dtype>::value) {
        interpolate_linear_1d_int8(input_ptr, output_ptr, x_idx_diff_ptr, x_idx_left_ptr, param);
    } else {
        interpolate_linear_1d_bfloat16(input_ptr, output_ptr, x_idx_diff_ptr, x_idx_left_ptr, param);
    }
}

template<typename dtype, typename config_ifm, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
INLINE void interpolate_linear_1d
(
    adf::input_buffer_conf<dtype, config_ifm>  & __restrict ifm,
    adf::output_buffer_conf<dtype, config_ofm> & __restrict ofm,
    interpolate_linear_1d_params_t  &param
) {
    dtype __aie_dm_resource_a * input_ptr = (dtype __aie_dm_resource_a *) ifm.data();
    dtype __aie_dm_resource_b * output_ptr = (dtype __aie_dm_resource_b *) ofm.data();
    return interpolate_linear_1d<dtype>(input_ptr, output_ptr, param);
}

#endif //__INTERPOLATE_LINEAR_1D__
