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
    

#ifndef __INSTANCE_NORM_PARAMS_H__
#define __INSTANCE_NORM_PARAMS_H__

#include "aie.hpp"
#include <adf.h>

#ifndef IFM_DM_BANK
#define IFM_DM_BANK __aie_dm_resource_a
#endif

#ifndef WTS_DM_BANK
#define WTS_DM_BANK __aie_dm_resource_a
#endif

#ifndef OFM_DM_BANK
#define OFM_DM_BANK  __aie_dm_resource_b
#endif

// XXX: hard-coded configuration tunned for best performance,
// kernel implementation must be modificed accordingly if user wants to
// optimize the unrolling factor to save accumulators/vectors/PM
#ifndef PART1_INNER_UNROLL_NUM
#define PART1_INNER_UNROLL_NUM 4
#endif

#ifndef PART1_OUTER_UNROLL_NUM
#define PART1_OUTER_UNROLL_NUM 8
#endif

#ifndef PART2_INNER_UNROLL_NUM
#define PART2_INNER_UNROLL_NUM 4
#endif

struct alignas(v16uint16) instance_norm_internal_params {    
    uint32_t channel;
    uint32_t height;
    uint32_t width;
    bfloat16 div_factor;
    bfloat16 epsilon;
    uint32_t loop_num_height;
    uint32_t loop_num_channel;
    uint32_t loop_num_width;
    int8_t ifm_shift;
    int8_t ofm_shift;
    int8_t gamma_shift;
    int8_t beta_shift;
    int32_t i_ptr_inc4;
    int32_t i_ptr_inc1;
    int32_t i_ptr_size1;
    int32_t i_ptr_inc2;
    int32_t i_ptr_size2;
    int32_t i_ptr_offset;
    int32_t coeff_ptr_offset;
    int32_t outer_loop;
    int32_t inner_loop;
    int32_t init_loop;
    int32_t depth_iter;
    int32_t c_idx;
    int32_t w_idx;
    int32_t h_idx;
    // multi-core offset for each row of kernel
    int32_t overlay_row_offset;
};

typedef struct instance_norm_internal_params instance_norm_params_t;

static NOINLINE void setup_instance_norm_part1_params(const uint32_t (&lp)[9], instance_norm_params_t &param){   
    param.channel            = lp[0];
    param.width             = lp[1];
    param.height            = lp[2];
    uint32_t div_factor     = lp[3];
    param.div_factor        = *((bfloat16 *)&div_factor);
    uint32_t epsilon        = lp[4];
    param.epsilon           = *((bfloat16 *)&epsilon);
    param.loop_num_height   = lp[5];
    param.loop_num_channel  = lp[6];
    param.loop_num_width    = lp[7];
    uint32_t shift          = lp[8];
    param.ifm_shift         = shift & 0xFF;
    param.ofm_shift         = (shift >> 8) & 0xFF;
    param.gamma_shift       = (shift >> 16) & 0xFF;
    param.beta_shift        = (shift >> 24) & 0xFF;

    // printf("channel:     %d\n", param.channel);
    // printf("width:       %d\n", param.width);
    // printf("height:      %d\n", param.height);
    // printf("div_factor:  %d\n", div_factor);
    // printf("epsilon:     %d\n", epsilon);
    // printf("num_height:  %d\n", param.loop_num_height);
    // printf("num_channel: %d\n", param.loop_num_channel);
    // printf("num_width:   %d\n", param.loop_num_width);
    // printf("ifm_shift:   %d\n", param.ifm_shift);
    // printf("ofm_shift:   %d\n", param.ofm_shift);
    // printf("gamma_shift: %d\n", param.gamma_shift);
    // printf("beta_shift:  %d\n", param.beta_shift);

    param.depth_iter = 0;
    // ptr jump as W4*unroll4, C/8, H
    param.i_ptr_inc4 = 32; // to next w4
    param.i_ptr_inc1 = -32*3 + param.width * 8;
    param.i_ptr_size1 = (param.channel >> 3) - 1;
    param.i_ptr_inc2 = 8 * (param.width - 12);
    param.i_ptr_size2 = (param.height) - 1;
    param.i_ptr_offset = 32 + 8 * param.width - param.width * param.channel * param.height;
    param.outer_loop = param.channel >> 3; // C8, unrolling is calculated within part1
    param.inner_loop = param.height * param.width >> 2; // W4
    param.init_loop = param.channel * 2 * 4 / 32; // 2 coefficients in C8 layout, init vec_size = 32
}

static NOINLINE void setup_instance_norm_part2_params(const uint32_t (&lp)[7], instance_norm_params_t &param){   
    param.channel            = lp[0];
    param.width             = lp[1];
    param.height            = lp[2];
    param.loop_num_height   = lp[3];
    param.loop_num_channel  = lp[4];
    param.loop_num_width    = lp[5];
    uint32_t shift          = lp[6];
    param.ifm_shift         = shift & 0xFF;
    param.ofm_shift         = (shift >> 8) & 0xFF;
    param.gamma_shift       = (shift >> 16) & 0xFF;
    param.beta_shift        = (shift >> 24) & 0xFF;

    // printf("channel:     %d\n", param.channel);
    // printf("width:       %d\n", param.width);
    // printf("height:      %d\n", param.height);
    // printf("num_height:  %d\n", param.loop_num_height);
    // printf("num_channel: %d\n", param.loop_num_channel);
    // printf("num_width:   %d\n", param.loop_num_width);
    // printf("ifm_shift:   %d\n", param.ifm_shift);
    // printf("ofm_shift:   %d\n", param.ofm_shift);
    // printf("gamma_shift: %d\n", param.gamma_shift);
    // printf("beta_shift:  %d\n", param.beta_shift);

    param.c_idx = 0;
    param.w_idx = 0;
    param.h_idx = 0;

    // ptr jump as W, H, C
    param.i_ptr_inc1 = 32;
    param.i_ptr_size1 = (param.width >> 2) - 1;
    param.i_ptr_inc2 = param.width*param.channel - 8 * param.width + 32;
    param.i_ptr_size2 = (param.height) - 1;
    param.i_ptr_offset = 32 - param.width * param.channel * (param.height - 1);
    // param.coeff_ptr_offset = param.channel
    param.outer_loop = param.channel >> 3; // C8, unrolling is calculated within part1
    param.inner_loop = param.height * param.width >> 2; // W4
    param.init_loop = param.channel * 2 * 4 / 32; // 2 coefficients in C8 layout, init vec_size = 32
}

#endif  //__INSTANCE_NORM_PARAMS_H__
