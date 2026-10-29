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

#include "layer_params.h"
#include "utils.h"
#include <adf.h>
#include "aie.hpp"

// Buffer placement directives for IFM, WTs and OFM buffers.
#ifndef IFM_DM_BANK
#define IFM_DM_BANK __aie_dm_resource_a
#endif

#ifndef WT_DM_BANK
#define WT_DM_BANK __aie_dm_resource_a
#endif

#ifndef OFM_DM_BANK
#define OFM_DM_BANK __aie_dm_resource_c
#endif

#define LAYER_PARAM_BYTES 64

struct alignas(v32int16) conv2d_dw_bf16_params{
    // ===================================================================
    // Following variables are external
    //RTPs for depthwise convolutional network
    uint8_t ifm_x;
    uint8_t ifm_y;
    uint8_t ofm;
    uint8_t stride;
    uint8_t kernel_height;
    uint8_t kernel_width;
    uint8_t act;
    uint8_t reserved;
    uint32_t ifmsv_size;
    uint32_t ofm_len;

    // ===================================================================
    // Following variables are internal
    int outer_g;  //Inner and outer loop counts
    int inner_g;

    int inc_input_next_line;
    int inc_output_next_line;
    int inc_output_step;
    // Used to differ the output pointer traversal between stride=1 and stride=2
    int inc_stride_output_0;
    int inc_stride_output_1;
    int inc_stride_output_2;

    int shift_mode;
    int shift_mode_output;

    int num_inner1;  // Inner 3D iterator parameters
    int inc_inner1;
    int num_inner2;
    int inc_inner2;
    int inc_inner3;

    int num_outer1;  // Outer 3D iterator parameters
    int inc_outer1;
    int num_outer2;
    int inc_outer2;
    int inc_outer3;

    int num_output1;  // Output 3D iterator parameters
    int inc_output1;
    int num_output2;
    int inc_output2;
    int inc_output3;

    int num_weight;
    int inc_weight1;
    int inc_weight2;
    int num_bias;
    int inc_bias1;
    int inc_bias2;

    int casc_offset;
    bfloat16 act_bar; // Activation function number comparison target
};
typedef struct conv2d_dw_bf16_params conv2d_dw_bf16_params_t;

#if __AIE_ARCH__ == 20
static NOINLINE void setup_conv2d_dw_params_bf16(void *params_ptr,
                                          conv2d_dw_bf16_params_t &param,
                                          uint8_t batch_size) {
    uint32_t* params_ptr_uint32 = (uint32_t*) &param;
    params_ptr_uint32[0] = ((const uint32_t*) params_ptr)[0]; // ifm_x, ifm_y, ofm, stride
    params_ptr_uint32[1] = ((const uint32_t*) params_ptr)[1]; // kernel_height, kernel_width, act
    params_ptr_uint32[2] = ((const uint32_t*) params_ptr)[2]; // ifmsv_size
    params_ptr_uint32[3] = ((const uint32_t*) params_ptr)[3]; // ofm_len

    uint32_t oxp = (param.ifm_x - param.kernel_width) / param.stride + 1;
    uint32_t oyp = (param.ifm_y - param.kernel_height) / param.stride + 1;
    uint32_t ixp = oxp * param.stride + (((param.kernel_width + 2) / 4) * 4);
    uint32_t iyp = param.ifm_y;

    int step_x = 32;
    int step_co = 8 * oxp;
    int step_ci = 8 * ixp;
    int step_yo = oxp * (batch_size * param.ofm);
    int step_yi = ixp * (batch_size * param.ofm);
    int mulS = 2;
    param.casc_offset = step_yo / 2;

    // Loop count
    param.inner_g = ((param.kernel_width + 1) / 2) * param.kernel_height;
    param.outer_g = oyp * step_yo * param.stride / 128;

    param.inc_input_next_line = step_yi * param.stride * mulS - 4 * step_x;
    param.shift_mode = (param.stride == 2 ? 8 : 16);
    param.shift_mode_output = (param.stride == 2 ? 0 : 16);
    param.inc_output_step = (param.stride == 2 ? 0 : step_x);
    param.inc_output_next_line = step_yo * mulS - (param.stride == 2 ? 2 : 3) * step_x;
    // when stride = 2, upper right quarter output pointer jumps to next line in advance
    // during the outer loop, to avoid disbehaved writing to wrong output buffer 
    param.inc_stride_output_0 = param.stride == 2 ? (param.inc_output_next_line+32) : 32;
    param.inc_stride_output_1 = param.stride == 2 ? 0 : param.inc_output_next_line;
    // Same reason for 2nd line
    param.inc_stride_output_2 = param.stride == 2 ? 0 : 32;

    // Inner 3D iterartor increments
    param.num_inner1 = (param.kernel_width + 1) / 2 - 1;
    param.inc_inner1 = -3 * step_x - (step_yi * param.stride) * mulS;
    param.num_inner2 = param.kernel_height - 1;
    param.inc_inner2 =
        -(step_yi * param.stride) * mulS + step_yi * mulS - (param.num_inner1 + 4) * step_x;
    param.inc_inner3 = -(step_yi * param.stride) * mulS - step_yi * param.num_inner2 * mulS -
                   (param.num_inner1 + 4) * step_x;
    // Outer 3d iterator increments
    param.num_outer1 = batch_size * param.ofm / 8 - 1;
    param.inc_outer1 = step_ci * mulS;
    param.num_outer2 = oxp * param.stride / 8 - 1;
    param.inc_outer2 = step_ci * mulS + 4 * step_x - step_yi * mulS;
    param.inc_outer3 = step_ci * mulS - 4 * step_x * param.num_outer2 +
                   2 * (step_yi * param.stride * mulS) - (step_yi * mulS);
    // Output pointer increments
    param.num_output1 = param.num_outer1;
    param.inc_output1 = step_co * mulS - step_yo * mulS - (param.stride == 2 ? 1 : 3) * step_x;
    param.num_output2 = param.num_outer2;
    param.inc_output2 = step_co * mulS + step_x - 2 * step_yo * mulS;
    param.inc_output3 = step_x;
    // Weights iterartor increments
    param.num_weight = (param.num_inner1 + 1) * param.kernel_height * (param.ofm / 8) - 1;
    param.inc_weight1 = 32;
    param.inc_weight2 = -32 * param.num_weight;
    // bias iterator increments
    param.num_bias = (param.ofm / 8) - 1;
    param.inc_bias1 = 64;
    param.inc_bias2 = -64 * param.num_bias;

    param.act_bar = (param.act == 0) ? -3.38953139e38 : 0;
}
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22
static NOINLINE void setup_conv2d_dw_params_bf16(void *params_ptr,
                                          conv2d_dw_bf16_params_t &param,
                                          uint8_t batch_size) {
    uint32_t* params_ptr_uint32 = (uint32_t*) &param;
    params_ptr_uint32[0] = ((const uint32_t*) params_ptr)[0]; // ifm_x, ifm_y, ofm, stride
    params_ptr_uint32[1] = ((const uint32_t*) params_ptr)[1]; // kernel_height, kernel_width, act

    uint32_t oxp = (param.ifm_x - param.kernel_width) / param.stride + 1;
    uint32_t oyp = (param.ifm_y - param.kernel_height) / param.stride + 1;
    uint32_t ixp = oxp * param.stride + (((param.kernel_width + 2) / 4) * 4);
    uint32_t iyp = param.ifm_y;

    int step_x = 32;
    int step_co = 8 * oxp;
    int step_ci = 8 * ixp;
    int step_yo = oxp * (batch_size * param.ofm);
    int step_yi = ixp * (batch_size * param.ofm);
    int mulS = 2;

    // Loop count
    param.inner_g = ((param.kernel_width + 3) / 4) * param.kernel_height;
    param.outer_g = oyp * step_yo * param.stride / 64;

    param.shift_mode = (param.stride == 2 ? DINTLV_lo_128o256 : DINTLV_hi_512o1024);
    param.inc_output_step = (param.stride == 2 ? 0 : step_x*2);

    // Inner 3D iterartor increments
    param.num_inner1 = (param.kernel_width + 3) / 4 - 1;
    param.inc_inner1 = -2 * step_x;
    param.num_inner2 = param.kernel_height - 1;
    param.inc_inner2 =
        step_yi * mulS - (param.num_inner1*2 + 4) * step_x;
    param.inc_inner3 = -step_yi * param.num_inner2 * mulS - (param.num_inner1*2 + 4) * step_x;
    // Outer 3d iterator increments
    param.num_outer1 = batch_size * param.ofm / 8 - 1;
    param.inc_outer1 = step_ci * mulS;
    param.num_outer2 = oxp * param.stride / 8 - 1;
    param.inc_outer2 = step_ci * mulS + 4 * step_x - step_yi * mulS;
    param.inc_outer3 = step_ci * mulS - 4 * step_x * param.num_outer2 +
                   1 * (step_yi * param.stride * mulS) - (step_yi * mulS);
    // Output pointer increments
    param.num_output1 = param.num_outer1;
    param.inc_output1 = step_co * mulS - (param.stride == 2 ? 0 : (2 * step_x));
    param.num_output2 = param.num_outer2;
    param.inc_output2 = step_co * mulS + 2 * step_x - step_yo * mulS;
    param.inc_output3 = 2 * step_x;
    // Weights iterartor increments
    param.num_weight = (param.num_inner1 + 1) * param.kernel_height * (param.ofm / 8) - 1;
    param.inc_weight1 = 64;
    param.inc_weight2 = -64 * param.num_weight;
    // bias iterator increments
    param.num_bias = (param.ofm / 8) - 1;
    param.inc_bias1 = 64;
    param.inc_bias2 = -64 * param.num_bias;

    param.act_bar = (param.act == 0) ? -3.38953139e38 : 0;
}
#endif  // __AIE_ARCH__

#if __AIE_ARCH__ == 20
static NOINLINE void setup_conv2d_dw_params_bf16_casc(void *params_ptr,
                                          conv2d_dw_bf16_params_t &param,
                                          uint8_t batch_size) {
    uint32_t* params_ptr_uint32 = (uint32_t*) &param;
    params_ptr_uint32[0] = ((const uint32_t*) params_ptr)[0]; // ifm_x, ifm_y, ofm, stride
    params_ptr_uint32[1] = ((const uint32_t*) params_ptr)[1]; // kernel_height, kernel_width, act

    uint32_t oxp = (param.ifm_x - param.kernel_width) / param.stride + 1;
    uint32_t oyp = (param.ifm_y - param.kernel_height) / param.stride + 1;
    uint32_t ixp = oxp * param.stride + (((param.kernel_width + 2) / 4) * 4);
    uint32_t iyp = param.ifm_y;

    int step_x = 32;
    int step_co = 8 * oxp * 2;
    int step_ci = 8 * ixp;
    int step_yo = oxp * (batch_size * param.ofm * 2);
    int step_yi = ixp * (batch_size * param.ofm);
    int mulS = 2;
    param.casc_offset = step_yo/2;

    // Loop count
    param.inner_g = ((param.kernel_width + 1) / 2) * param.kernel_height;
    param.outer_g = oyp * step_yo * param.stride / 128;

    param.inc_input_next_line = step_yi * param.stride * mulS - 4 * step_x;
    param.shift_mode = (param.stride == 2 ? 8 : 16);
    param.shift_mode_output = (param.stride == 2 ? 0 : 16);
    param.inc_output_step = (param.stride == 2 ? 0 : step_x);
    param.inc_output_next_line = step_yo * mulS - (param.stride == 2 ? 2 : 3) * step_x;
    // when stride = 2, upper right quarter output pointer jumps to next line in advance
    // during the outer loop, to avoid disbehaved writing to wrong output buffer 
    param.inc_stride_output_0 = param.stride == 2 ? (param.inc_output_next_line+32) : 32;
    param.inc_stride_output_1 = param.stride == 2 ? 0 : param.inc_output_next_line;
    // Same reason for 2nd line
    param.inc_stride_output_2 = param.stride == 2 ? 0 : 32;

    // Inner 3D iterartor increments
    param.num_inner1 = (param.kernel_width + 1) / 2 - 1;
    param.inc_inner1 = -3 * step_x - (step_yi * param.stride) * mulS;
    param.num_inner2 = param.kernel_height - 1;
    param.inc_inner2 =
        -(step_yi * param.stride) * mulS + step_yi * mulS - (param.num_inner1 + 4) * step_x;
    param.inc_inner3 = -(step_yi * param.stride) * mulS - step_yi * param.num_inner2 * mulS -
                   (param.num_inner1 + 4) * step_x;
    // Outer 3d iterator increments
    param.num_outer1 = batch_size * param.ofm / 8 - 1;
    param.inc_outer1 = step_ci * mulS;
    param.num_outer2 = oxp * param.stride / 8 - 1;
    param.inc_outer2 = step_ci * mulS + 4 * step_x - step_yi * mulS;
    param.inc_outer3 = step_ci * mulS - 4 * step_x * param.num_outer2 +
                   2 * (step_yi * param.stride * mulS) - (step_yi * mulS);
    // Output pointer increments
    param.num_output1 = param.num_outer1;
    param.inc_output1 = step_co * mulS - step_yo * mulS - (param.stride == 2 ? 1 : 3) * step_x;
    param.num_output2 = param.num_outer2;
    param.inc_output2 = step_co * mulS + step_x - 2 * step_yo * mulS;
    param.inc_output3 = step_x + step_yo/2*mulS;
    // Weights iterartor increments
    param.num_weight = (param.num_inner1 + 1) * param.kernel_height * (param.ofm / 8) - 1;
    param.inc_weight1 = 32;
    param.inc_weight2 = -32 * param.num_weight;
    // bias iterator increments
    param.num_bias = (param.ofm / 8) - 1;
    param.inc_bias1 = 64;
    param.inc_bias2 = -64 * param.num_bias;

    param.act_bar = (param.act == 0) ? -3.38953139e38 : 0;
}
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22
static NOINLINE void setup_conv2d_dw_params_bf16_casc(void *params_ptr,
                                          conv2d_dw_bf16_params_t &param,
                                          uint8_t batch_size) {
    uint32_t* params_ptr_uint32 = (uint32_t*) &param;
    params_ptr_uint32[0] = ((const uint32_t*) params_ptr)[0]; // ifm_x, ifm_y, ofm, stride
    params_ptr_uint32[1] = ((const uint32_t*) params_ptr)[1]; // kernel_height, kernel_width, act

    uint32_t oxp = (param.ifm_x - param.kernel_width) / param.stride + 1;
    uint32_t oyp = (param.ifm_y - param.kernel_height) / param.stride + 1;
    uint32_t ixp = oxp * param.stride + (((param.kernel_width + 2) / 4) * 4);
    uint32_t iyp = param.ifm_y;

    int step_x = 32;
    int step_co = 8 * oxp*2;
    int step_ci = 8 * ixp;
    int step_yo = oxp * (batch_size * param.ofm*2);
    int step_yi = ixp * (batch_size * param.ofm);
    int mulS = 2;
    param.casc_offset = step_yo/2;

    // Loop count
    param.inner_g = ((param.kernel_width + 3) / 4) * param.kernel_height;
    param.outer_g = oyp * step_yo * param.stride / 64;

    param.shift_mode = (param.stride == 2 ? DINTLV_lo_128o256 : DINTLV_hi_512o1024);
    param.inc_output_step = (param.stride == 2 ? 0 : step_x*2);

    // Inner 3D iterartor increments
    param.num_inner1 = (param.kernel_width + 3) / 4 - 1;
    param.inc_inner1 = -2 * step_x;
    param.num_inner2 = param.kernel_height - 1;
    param.inc_inner2 =
        step_yi * mulS - (param.num_inner1*2 + 4) * step_x;
    param.inc_inner3 = -step_yi * param.num_inner2 * mulS - (param.num_inner1*2 + 4) * step_x;
    // Outer 3d iterator increments
    param.num_outer1 = batch_size * param.ofm / 8 - 1;
    param.inc_outer1 = step_ci * mulS;
    param.num_outer2 = oxp * param.stride / 8 - 1;
    param.inc_outer2 = step_ci * mulS + 4 * step_x - step_yi * mulS;
    param.inc_outer3 = step_ci * mulS - 4 * step_x * param.num_outer2 +
                   1 * (step_yi * param.stride * mulS) - (step_yi * mulS);
    // Output pointer increments
    param.num_output1 = param.num_outer1;
    param.inc_output1 = step_co * mulS - (param.stride == 2 ? 0 : (2 * step_x));
    param.num_output2 = param.num_outer2;
    param.inc_output2 = step_co * mulS + 2 * step_x - step_yo * mulS;
    param.inc_output3 = 2 * step_x + step_yo/2*mulS;
    // Weights iterartor increments
    param.num_weight = (param.num_inner1 + 1) * param.kernel_height * (param.ofm / 8) - 1;
    param.inc_weight1 = 64;
    param.inc_weight2 = -64 * param.num_weight;
    // bias iterator increments
    param.num_bias = (param.ofm / 8) - 1;
    param.inc_bias1 = 64;
    param.inc_bias2 = -64 * param.num_bias;

    param.act_bar = (param.act == 0) ? -3.38953139e38 : 0;
}
#endif  // __AIE_ARCH__
