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

#ifndef __CONV2D_DW_PARAMS_H__
#define __CONV2D_DW_PARAMS_H__

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

struct alignas(v16uint16) conv2d_dw_params{
    //RTPs for depthwise convolutional network
    uint8_t ifm_x;
    uint8_t ifm_y;
    uint8_t ofm;
    uint8_t stride;
    uint8_t ksize;
    int8_t shift_res;
    int8_t bias_shift;
    uint8_t act;
    uint32_t ifmsv_size;
    uint32_t ofm_len;

    int outer_g;  //Inner and outer loop counts 
    int inner_g;

    int incAI1;
    int incAI2;
    int incAI3;
    int incS0;

    int stepAL;   //Stride supportive iterators
    int stepAS;
    int shftAS;
    int shflOL;
    int shflOH;

    int numAL1;  //Inner 3D iterator parameters
    int incAL1;
    int numAL2;
    int incAL2;
    int incAL3;

    int numAO1;  //Outer 3D iterator parameters
    int incAO1;
    int numAO2;
    int incAO2;
    int incAO3;

    int numS1;  //Output 3D iterator parameters
    int incS1;
    int numS2;
    int incS2;
    int incS3;

    int numB;
    int incB1;
    int incB2;
    int numofm;
    int ofmB1;
    int ofmB2;

    int result_sign; //Activation (0: Linear, 1: ReLU)
    int ofm_elem_size;
};
typedef struct conv2d_dw_params conv2d_dw_params_t;

static NOINLINE void setup_conv2d_dw_params(int8_t * params_ptr, conv2d_dw_params_t &param, uint8_t batch_size) {
    param.ifm_x      = (uint8_t) *params_ptr++;
    param.ifm_y      = (uint8_t) *params_ptr++;
    param.ofm        = (uint8_t) *params_ptr++;
    param.stride     = (uint8_t) *params_ptr++;
    param.ksize      = (uint8_t) *params_ptr++;
    param.shift_res  = (int8_t) *params_ptr++;
    param.bias_shift = (int8_t) *params_ptr++;
    param.act        = (uint8_t) *params_ptr++;
    param.ifmsv_size = *((uint32_t *)params_ptr); params_ptr += 4;
    param.ofm_len = *((uint32_t *)params_ptr);

    uint8_t bits_o   = 8;
    uint8_t oxp      = (param.ifm_x - param.ksize + 1)/param.stride ;
    uint8_t ixp      = oxp*param.stride + (((param.ksize + 2)/4)*4);   	
    uint8_t oyp      = (param.ifm_y - param.ksize + 1)/param.stride;
    uint8_t iyp      = param.ifm_y;
 
    int step_x   = 32;
    int step_xo1 = (param.stride == 2 ? 0 : step_x);
    int step_co  = 8 * oxp;
    int step_ci  = 8 * ixp;
    int step_yo  = oxp * (batch_size * param.ofm);
    int step_yi  = ixp * (batch_size * param.ofm);
    int mulS     = bits_o/8;

    param.incAI1 = step_yi*param.stride  - 4*step_x;
    param.shftAS = (param.stride == 2 ? DINTLV_lo_64o128 : DINTLV_hi_256o512);
    param.stepAS = step_xo1 * mulS;
    param.incS0  = step_yo * mulS -(3/param.stride)* step_x * mulS;
    //Inner 3D iterartor increments
    param.numAL1 = (param.ksize+3)/4 - 1;
    param.incAL1 = -3*step_x - (step_yi * param.stride);
    param.numAL2 = param.ksize - 1;
    param.incAL2 = -(step_yi * param.stride) + step_yi - (param.numAL1+4) * step_x;
    param.incAL3 = -(step_yi * param.stride) - step_yi * param.numAL2 - (param.numAL1+4) * step_x;
    //Outer 3d iterator increments
    param.numAO1 = batch_size * param.ofm / 8 - 1;
    param.incAO1 = step_ci;
    param.numAO2 = oxp*param.stride / 16 - 1;
    param.incAO2 = step_ci + 4*step_x - step_yi;
    param.incAO3 = step_ci - 4*step_x * param.numAO2 + 2*(step_yi * param.stride) - (step_yi);
    //Output pointer increments
    param.numS1 = param.numAO1;
    param.incS1 = (step_co - step_yo - (3/param.stride)*step_x) * mulS;
    param.numS2 = param.numAO2;
    param.incS2 = (step_co + step_x - 2*step_yo) * mulS;
    param.incS3 = step_x * mulS;
    //Weights iterartor increments
    param.numB  = (param.numAL1+1) * param.ksize * (param.ofm/8) - 1;
    param.incB1 = 32;
    param.incB2 = -32*param.numB;
    //bias iterator increments	    
    param.numofm= (param.ofm/8) - 1;
    param.ofmB1 = 64;
    param.ofmB2 = -64*param.numofm;
    param.inner_g = ((param.ksize+3)/4) * param.ksize;
    param.outer_g = oyp * step_yo * param.stride / 256;
    //activation
    param.result_sign = 1 - param.act;
    param.ofm_elem_size = oxp * oyp * batch_size * param.ofm;

}

#endif  //__CONV2D_DW_PARAMS_H__
