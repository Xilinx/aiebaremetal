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

#include "resize_nearest_params.h"
#include "utils.h"
#include "aie_api/utils.hpp"
#include "aie_api/aie.hpp"

using namespace aie;

template<typename dtype, typename config_ifm, typename config_ofm, int nearest_mode>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
void resize_nearest
(
    adf::input_buffer_conf<dtype, config_ifm>  & __restrict ifm,
    adf::output_buffer_conf<dtype, config_ofm> & __restrict ofm,
    resize_nearest_params_t  &param
){

    dtype __aie_dm_resource_a * input_ptr = (dtype __aie_dm_resource_a *) ifm.data();
    dtype __aie_dm_resource_b * output_ptr = (dtype __aie_dm_resource_b *) ofm.data();

    constexpr int vec_len = 16;

    int16_t h_des = param.kernel_ofm_height;
    int16_t w_des = param.kernel_ofm_width;
    int16_t offset_idx_out_height = (int16_t)param.offset_idx_out_height;
    int16_t offset_idx_out_width = (int16_t)param.offset_idx_out_width;
    int16_t offset_idx_in_height = (int16_t)param.offset_idx_in_height;
    int16_t offset_idx_in_width = (int16_t)param.offset_idx_in_width;
    int16_t total_ifm_height = (int16_t)param.total_ifm_height;
    int16_t total_ifm_width = (int16_t)param.total_ifm_width;
    float step_h = param.step_h;
    float step_w = param.step_w;
    float offset_height = param.offset_scale_height;
    float offset_width = param.offset_scale_width;
    
    aie::vector<int16_t, 32> y_idx_vect, x_idx_vect;
    for (unsigned i = 0; i < y_idx_vect.size(); i++){
        y_idx_vect[i] = i; // i-th element is updated
    }
    x_idx_vect = y_idx_vect;
    y_idx_vect = aie::add(y_idx_vect, (int16_t)offset_idx_out_height);
    x_idx_vect = aie::add(x_idx_vect, (int16_t)offset_idx_out_width);

    auto y_des_acc0 = aie::mul(aie::to_float(y_idx_vect, 0), step_h);
    y_des_acc0 = aie::add(y_des_acc0, offset_height);
    y_des_acc0.from_vector(aie::max(y_des_acc0.to_vector<float>(), 0.f));
    y_des_acc0.from_vector(aie::min(y_des_acc0.to_vector<float>(), (float)(total_ifm_height - 1)));
    
    auto y_des_temp0  = y_des_acc0.to_vector<float>();
    y_idx_vect = aie::to_fixed<int16_t>(y_des_temp0, 0);
    y_idx_vect = aie::select(y_idx_vect, aie::sub(y_idx_vect, (int16_t)1), aie::lt(aie::sub(y_des_temp0, aie::to_float(y_idx_vect)), 0.f));
    if constexpr (nearest_mode == 1) { // prefer ceil
        auto y_idx_vect_dec = aie::sub(y_des_acc0, aie::to_float(y_idx_vect, 0));
        auto mask_dec = aie::lt(y_idx_vect_dec.to_vector<float>(), 0.5f);
        auto y_idx_vect_plus = aie::add(y_idx_vect, (int16_t)1);
        y_idx_vect = aie::select(y_idx_vect_plus, y_idx_vect, mask_dec);
    } else if constexpr (nearest_mode == 2) { // prefer floor
        auto y_idx_vect_dec = aie::sub(y_des_acc0, aie::to_float(y_idx_vect, 0));
        auto mask_dec = aie::le(y_idx_vect_dec.to_vector<float>(), 0.5f);
        auto y_idx_vect_plus = aie::add(y_idx_vect, (int16_t)1);
        y_idx_vect = aie::select(y_idx_vect_plus, y_idx_vect, mask_dec);
    }
    y_idx_vect = aie::max(y_idx_vect, (int16_t)0);
    y_idx_vect = aie::min(y_idx_vect, (int16_t)(total_ifm_height - 1));
    y_idx_vect = aie::sub(y_idx_vect, (int16_t)offset_idx_in_height);
    auto y_des_acc1 = aie::mul(y_idx_vect, param.kernel_ifm_width * 8);
    y_idx_vect = y_des_acc1.template to_vector<int16_t>(0);

    auto x_des_acc0 = aie::mul(aie::to_float(x_idx_vect, 0), step_w);
    x_des_acc0 = aie::add(x_des_acc0, offset_width);
    auto x_des_temp0  = x_des_acc0.to_vector<float>();
    
    x_idx_vect = aie::to_fixed<int16_t>(x_des_temp0, 0);
    x_idx_vect = aie::select(x_idx_vect, aie::sub(x_idx_vect, (int16_t)1), aie::lt(aie::sub(x_des_temp0, aie::to_float(x_idx_vect)), 0.f));
    if constexpr (nearest_mode == 1) { // prefer ceil
        auto x_idx_vect_dec = aie::sub(x_des_acc0, aie::to_float(x_idx_vect, 0));
        auto mask_dec = aie::lt(x_idx_vect_dec.to_vector<float>(), 0.5f);
        auto x_idx_vect_plus = aie::add(x_idx_vect, (int16_t)1);
        x_idx_vect = aie::select(x_idx_vect_plus, x_idx_vect, mask_dec); 
    } else if constexpr (nearest_mode == 2){ // prefer floor
        auto x_idx_vect_dec = aie::sub(x_des_acc0, aie::to_float(x_idx_vect, 0));
        auto mask_dec = aie::le(x_idx_vect_dec.to_vector<float>(), 0.5f);
        auto x_idx_vect_plus = aie::add(x_idx_vect, (int16_t)1);
        x_idx_vect = aie::select(x_idx_vect_plus, x_idx_vect, mask_dec);
    }

    x_idx_vect = aie::max(x_idx_vect, (int16_t)0);
    x_idx_vect = aie::min(x_idx_vect, (int16_t)(total_ifm_width - 1));
    x_idx_vect = aie::sub(x_idx_vect, (int16_t)offset_idx_in_width);


    auto x_des_acc1 = aie::mul(x_idx_vect, 8);
    x_idx_vect = x_des_acc1.template to_vector<int16_t>(0);
    
    for(int y_des=0; y_des< h_des; y_des++){
        for(int x_des=0; x_des < w_des; x_des++) {
            int y_offset = y_idx_vect[y_des];
            int x_offset = x_idx_vect[x_des];
            aie::vector<dtype,vec_len>  d00 = aie::load_unaligned_v<vec_len>(input_ptr + y_offset + x_offset);
            aie::store_unaligned_v(output_ptr, d00);
            output_ptr += 8;
        }
    }
}

