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

#include "resize_bilinear_params.h"
#include "utils.h"
#include "aie_api/utils.hpp"
#include "aie_api/aie.hpp"

using namespace aie;

aie::vector<bfloat16, 16> INLINE v16int8_to_v16bfloat16(aie::vector<int8, 16> x) {
    aie::accum<accfloat, 16> acc_x = v16accfloat(aie::to_float(x.unpack()));
    return acc_x.template to_vector<bfloat16>();
}

template<typename dtype, typename config_ifm, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
__attribute__((noinline))
void resize_bilinear
(
    adf::input_buffer_conf<dtype, config_ifm>  & __restrict ifm,
    adf::output_buffer_conf<dtype, config_ofm> & __restrict ofm,
    resize_bilinear_params_t  &param
){
    //Bilinear Interpolation
    dtype __aie_dm_resource_a * input_ptr = (dtype __aie_dm_resource_a *) ifm.data();
    dtype __aie_dm_resource_b * output_ptr = (dtype __aie_dm_resource_b *) ofm.data();

    int16_t w_des = (int16_t)param.kernel_ofm_width;
    int16_t h_des = (int16_t)param.kernel_ofm_height;
    float step_w = param.step_w;
    float step_h = param.step_h;
    float off_step_h = param.offset_scale_height;
    float off_step_w = param.offset_scale_width;
    int16_t step_Yi = 8 * (int16_t)param.kernel_ifm_width;
    int16_t offset_idx_out_height = (int16_t)param.offset_idx_out_height;
    int16_t offset_idx_out_width = (int16_t)param.offset_idx_out_width;
    int16_t offset_idx_in_height = (int16_t)(param.offset_idx_in_height *step_Yi);
    int16_t offset_idx_in_width = (int16_t)(param.offset_idx_in_width * 8);
    int16_t kernel_ofm_width = (int16_t)param.kernel_ofm_width;
    int16_t kernel_ofm_height = (int16_t)param.kernel_ofm_height;
    int16_t total_ifm_height = (int16_t)param.total_ifm_height;
    int16_t total_ifm_width = (int16_t)param.total_ifm_width;

    aie::vector<int16_t, 32> y_idx_vect, y_idx_vect_d1x, x_idx_vect, x_idx_vect_dx1;
    aie::vector<bfloat16, 32> v_wx_left, v_wx_right, v_wy_top, v_wy_bottom;

    for (unsigned i = 0; i < y_idx_vect.size(); i++){
        y_idx_vect[i] = i; // i-th element is updated
    }
    x_idx_vect = y_idx_vect;
    y_idx_vect = add(y_idx_vect, offset_idx_out_height);
    x_idx_vect = add(x_idx_vect, offset_idx_out_width);

    aie::vector<int16_t, 32> dis_w_1 = broadcast<int16_t, 32>(1);
    aie::accum<accfloat, 32> dis_w_1_f32 = v32accfloat(aie::to_float(dis_w_1));

    // compute y index
    auto y_des_acc00 = aie::mul(aie::to_float(y_idx_vect, 0), step_h);
    auto y_des_acc0 = aie::add(y_des_acc00, off_step_h);
    y_des_acc0.from_vector(aie::max(y_des_acc0.to_vector<float>(), 0.f));
    y_des_acc0.from_vector(aie::min(y_des_acc0.to_vector<float>(), (float)(total_ifm_height - 1)));
    auto y_des_temp0 = y_des_acc0.template to_vector<float>();
    y_idx_vect = aie::to_fixed<int16_t>(y_des_temp0, 0);
    y_idx_vect = aie::select(y_idx_vect, aie::sub(y_idx_vect, (int16_t)1), aie::lt(aie::sub(y_des_temp0, aie::to_float(y_idx_vect)), 0.f));
    
    aie::accum<accfloat, 32> acc_temp_y = v32accfloat(aie::to_float(y_idx_vect));
    auto y_des_acc1 = aie::add(acc_temp_y, dis_w_1_f32);
    v_wy_bottom = aie::sub(y_des_acc0, acc_temp_y).template to_vector<bfloat16>();
    v_wy_top = aie::sub(y_des_acc1, y_des_acc0).template to_vector<bfloat16>();

    auto y_des_acc2 = aie::mul(y_idx_vect, step_Yi);
    y_idx_vect = y_des_acc2.template to_vector<int16_t>(0);
    aie::vector<int16_t, 32> vec_temp_y = broadcast<int16_t, 32>(step_Yi);
    y_idx_vect_d1x = aie::add(y_idx_vect, step_Yi);
    y_idx_vect_d1x = aie::min(y_idx_vect_d1x, (int16_t)(param.kernel_ifm_width * (total_ifm_height - 1) * 8));

    // compute x index
    auto x_des_acc00 = aie::mul(aie::to_float(x_idx_vect, 0), step_w);
    auto x_des_acc0 = aie::add(x_des_acc00, off_step_w);
    x_des_acc0.from_vector(aie::max(x_des_acc0.to_vector<float>(), 0.f));
    x_des_acc0.from_vector(aie::min(x_des_acc0.to_vector<float>(), (float)(total_ifm_width - 1)));
    auto x_des_temp0 = x_des_acc0.template to_vector<float>();
    x_idx_vect = aie::to_fixed<int16_t>(x_des_temp0, 0);
    x_idx_vect = aie::select(x_idx_vect, aie::sub(x_idx_vect, (int16_t)1), aie::lt(aie::sub(x_des_temp0, aie::to_float(x_idx_vect)), 0.f));

    aie::accum<accfloat, 32> acc_temp_x = v32accfloat(aie::to_float(x_idx_vect));
    auto x_des_acc1 = aie::add(acc_temp_x, dis_w_1_f32);
    v_wx_right = aie::sub(x_des_acc0, acc_temp_x).template to_vector<bfloat16>();
    v_wx_left = aie::sub(x_des_acc1, x_des_acc0).template to_vector<bfloat16>();

    auto x_des_acc2 = aie::mul(x_idx_vect, 8);
    x_idx_vect = x_des_acc2.template to_vector<int16_t>(0);
    x_idx_vect_dx1 = aie::add(x_idx_vect, (int16_t)8);
    x_idx_vect_dx1 = aie::min(x_idx_vect_dx1, (int16_t)((total_ifm_width - 1) * 8));

    for(int y_des=0; y_des < h_des; y_des++) {
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for(int x_des=0; x_des < w_des; x_des++){
            int y_offset = y_idx_vect[y_des] - offset_idx_in_height;
            int x_offset = x_idx_vect[x_des] - offset_idx_in_width;
            int y_offset_d1x = y_idx_vect_d1x[y_des] - offset_idx_in_height;
            int x_offset_dx1 = x_idx_vect_dx1[x_des] - offset_idx_in_width;
            vector<bfloat16, 16> d00, d01, d10, d11; //vectors to store input data
            vector<dtype, 16> dst_v;

            // loading unaligned data
            auto d00_in = aie::load_unaligned_v<16>(input_ptr + y_offset + x_offset);
            auto d01_in = aie::load_unaligned_v<16>(input_ptr + y_offset + x_offset_dx1);
            auto d10_in = aie::load_unaligned_v<16>(input_ptr + y_offset_d1x + x_offset);
            auto d11_in = aie::load_unaligned_v<16>(input_ptr + y_offset_d1x + x_offset_dx1);

            // convert int8 to bfloat16
            if constexpr (sizeof(dtype) == 1) {
                d00 = v16int8_to_v16bfloat16(d00_in);
                d01 = v16int8_to_v16bfloat16(d01_in);
                d10 = v16int8_to_v16bfloat16(d10_in);
                d11 = v16int8_to_v16bfloat16(d11_in);
            } else {
                d00 = d00_in;
                d01 = d01_in;
                d10 = d10_in;
                d11 = d11_in;
            }

            // init wight and scala_w_des_1_inv
            vector<bfloat16,16> wx_left_d0 = broadcast<bfloat16, 16>(v_wx_left[x_des]);
            vector<bfloat16,16> wx_right_d0 = broadcast<bfloat16, 16>(v_wx_right[x_des]);
            vector<bfloat16,16> wy_top_d0 = broadcast<bfloat16, 16>(v_wy_top[y_des]);
            vector<bfloat16,16> wy_bottom_d0 = broadcast<bfloat16, 16>(v_wy_bottom[y_des]);


            // compute f(x, y)
            aie::accum<accfloat, 16> temp_acc_d00, temp_acc_d01, temp_acc_d10, temp_acc_d11;
            aie::accum<accfloat, 16> temp_add_d0, temp_add_d1, temp_add_dt, temp_add_db, temp_add_v;

            temp_acc_d00 = mul(d00, wx_left_d0);
            temp_acc_d01 = mul(wx_right_d0, d01);
            temp_acc_d10 = mul(d10, wx_left_d0);
            temp_acc_d11 = mul(wx_right_d0, d11);

            temp_add_d0 = add(temp_acc_d00, temp_acc_d01);
            temp_add_d1 = add(temp_acc_d10, temp_acc_d11);
            // change data
            temp_add_dt = mul(temp_add_d0.template to_vector<bfloat16>(), wy_top_d0);
            temp_add_db = mul(temp_add_d1.template to_vector<bfloat16>(), wy_bottom_d0);

            temp_add_v = add(temp_add_dt, temp_add_db);
            auto dst_out = temp_add_v.template to_vector<bfloat16>();

            if constexpr (sizeof(dtype) == 1) {
                dst_v = aie::to_fixed<dtype>(dst_out);
            } else {
                dst_v = dst_out;
            }

            // write out data
            store_unaligned_v((dtype __aie_dm_resource_a *)output_ptr, dst_v);
            output_ptr=output_ptr + 8;
        }
    }
}


