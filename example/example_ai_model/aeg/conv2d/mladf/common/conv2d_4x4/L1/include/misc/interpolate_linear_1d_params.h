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

#ifndef __INTERPOLATE_LINEAR_1d_H__
#define __INTERPOLATE_LINEAR_1d_H__
#define SHIFT_DIFF_BIT 10
#define SHIFT_DIFF_VALUE 1024
#define VEC_SIZE 16
#define VEC_SIZE_F 16.f
#define SCALE_FACTOR_UP_SHIFT 15

#include "utils.h"

struct alignas(v32uint16) interpolate_linear_1d_internal_params
{
    //passed in params
    float inv_scale_factor;
    uint16_t total_in_width;
    uint16_t total_out_width;
    uint16_t sv_ifm_width ;
    uint16_t sv_ofm_width ;
    uint16_t in_stride;
    uint16_t out_stride;
    uint8_t align_corners;
    uint8_t depth;
    int8_t output_shift;
    int8_t num_iters;
    //calculated by param setup
    float clamp_hi;
    float x_idx_offset;
    float x_idx_src_offset;
    uint16_t input_offset = 0;
    uint16_t output_offset = 0;
    uint16_t c_step_scale;
    uint16_t channel_loop_cnt;
    uint16_t last_in_idx;
    uint16_t last_out_idx;
    uint16_t val_loop_cnt;
    uint16_t sv_output_sample_num;
    int8_t cur_iter = 0;
};

typedef struct interpolate_linear_1d_internal_params interpolate_linear_1d_params_t;


static void setup_interpolate_linear_1d_params (const void* param_ptr, interpolate_linear_1d_params_t& param) {
    // passed in
    param.inv_scale_factor = *(float*)param_ptr; param_ptr = (int8_t*)param_ptr + 4;
    param.total_in_width = *(uint16_t*) param_ptr; param_ptr = (int8_t*)param_ptr + 2;
    param.total_out_width = *(uint16_t*)param_ptr; param_ptr = (int8_t*)param_ptr + 2;
    param.sv_ifm_width  = *(uint16_t*)param_ptr; param_ptr = (int8_t*)param_ptr + 2;
    param.sv_ofm_width  = *(uint16_t*)param_ptr; param_ptr = (int8_t*)param_ptr + 2;
    param.in_stride = *(uint16_t*)param_ptr; param_ptr = (int8_t*)param_ptr + 2;
    param.out_stride = *(uint16_t*)param_ptr; param_ptr = (int8_t*)param_ptr + 2;
    param.align_corners = *(uint8_t*)param_ptr; param_ptr = (int8_t*)param_ptr + 1;
    param.depth = *(uint8_t*) param_ptr; param_ptr = (int8_t*)param_ptr + 1;
    param.output_shift = *(int8_t*)param_ptr; param_ptr = (int8_t*)param_ptr + 1;
    param.num_iters = *(int8_t*)param_ptr; param_ptr = (int8_t*)param_ptr + 1;
    // calculated
    param.clamp_hi = (float)(param.total_in_width - 1);
    param.c_step_scale = param.sv_ifm_width  * 8;
    param.channel_loop_cnt = param.depth >> 3;
    param.output_shift += SHIFT_DIFF_BIT;
    if (param.align_corners) {
        param.x_idx_offset = 0.f;
    } else {
        param.x_idx_offset = (0.5f * param.inv_scale_factor - 0.5f);
    }
    param.x_idx_src_offset = param.inv_scale_factor * VEC_SIZE_F;
    param.val_loop_cnt = param.sv_ofm_width ;
    param.sv_output_sample_num = param.sv_ofm_width  * param.depth;
}

static INLINE void setup_params_iter(interpolate_linear_1d_params_t& param) {
    param.cur_iter += 1;
    if (param.cur_iter > 1) {
        param.input_offset += param.in_stride >> 3;
        param.output_offset += param.out_stride >> 3;
    }
    param.last_in_idx = param.total_in_width - param.input_offset - 1;
    param.last_out_idx = param.total_out_width - param.output_offset - 1;
    if (param.cur_iter == param.num_iters){
        param.val_loop_cnt = (1 - param.num_iters ) * param.sv_ofm_width  + param.total_out_width;;
    }
}

#endif