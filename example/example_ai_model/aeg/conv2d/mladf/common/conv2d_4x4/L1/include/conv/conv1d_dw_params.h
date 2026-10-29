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

#ifndef __CONV1D_DW_PARAMS_H__
#define __CONV1D_DW_PARAMS_H__

#include "layer_params.h"
#include "utils.h"
#include <adf.h>
#include "aie.hpp"

// Buffer placement directives for IFM, WTs and OFM buffers.
#ifndef IFM_DM_BANK
#define IFM_DM_BANK __aie_dm_resource_a
#endif

#ifndef WTS_DM_BANK
#define WTS_DM_BANK __aie_dm_resource_b
#endif

#ifndef OFM_DM_BANK
#define OFM_DM_BANK __aie_dm_resource_c
#endif

#define LAYER_PARAM_BYTES 64

struct alignas(v16uint16) conv1d_dw_params{
    //RTPs for depthwise convolutional network
    uint8_t ifm_depth;
    uint8_t ifm_width;
    uint8_t ksize;
    uint8_t stride;
    uint8_t gran_in_width;
    uint8_t shift_ifm;
    uint8_t shift_bias;
    uint8_t shift_ofm;

    int outer_g;  //Inner and outer loop counts
    int inner_g;

    int shfMode;
    int stepS;

    int numInnL1;  //Inner 2D iterator parameters
    int incInnL1;
    int incInnL2;

    int numOutL1;  //Outer 2D iterator parameters
    int incOutL1;
    int incOutL2;

    int numResO1;  //Output 2D iterator parameters
    int incResO1;
    int numResO2;
    int incResO2;
    int incResO3;

    int numK1;  // Filter iterator
    int numK2;
    int incK1;
    int incK2;
    int incK3;

    int numB1; // Bias iterator
    int incB1;
    int incB2;
};
typedef struct conv1d_dw_params conv1d_dw_params_t;

static NOINLINE void setup_conv1d_dw_params(int8_t * params_ptr, conv1d_dw_params_t &param, uint8_t batch_size) {
    // Load RTP and set the parameter structure
    aie::vector<int32,8> pars = aie::load_v<8>((int32 *)(params_ptr));
    aie::store_v((int32 __aie_dm_resource_a *) &param, pars);

#if __AIE_ARCH__ == 20
    int step_x = 32; // step in bytes
    int ifm_w_pad = (param.ifm_width + 3)/4; // IFM padding to W4
    int oxp = (param.ifm_width - param.ksize)/param.stride + 1;
    int8_t valid_acc_num = (param.stride == 2 ? 4 : 8);
    int oxp_w4c8 = (oxp + 4 * valid_acc_num - 1)/(4 * valid_acc_num);

    // Output stride store_v & shuffle
    param.shfMode = (param.stride == 2 ? DINTLV_lo_64o128 : DINTLV_hi_256o512);
    param.stepS = (param.stride == 2 ? 0 : step_x);

    // Loop count
    param.outer_g = (param.ifm_depth / 8) * oxp_w4c8;
    param.inner_g = (param.ksize+3) / 4; // ceil(ksize/4)

    // Inner 2D iterartor for addressing IFM
    param.numInnL1 = param.inner_g - 1;
    param.incInnL1 = -7 * step_x; // for next filter_k
    param.incInnL2 = -param.numInnL1 * step_x; // next W32

    // Outer 2D iterartor for addressing IFM
    param.numOutL1 = oxp_w4c8 - 1;
    param.incOutL1 = 0; // sit at next W32
    param.incOutL2 = (ifm_w_pad - 8*(1+param.numOutL1)) * step_x; // next C8

    // Filter 2D iterator for addressing WTS
    param.numK1 = param.numOutL1;
    param.incK1 = -32 * param.inner_g; // reset for next W32
    param.incK2 = 0; // reset for next C8

    // Bias 2D iterator
    param.numB1 = param.numOutL1;
    param.incB1 = 0;
    param.incB2 = 64; // 2*C8*4 per sliding mul

    // Store result 2D iterator for addressing OFM
    param.numResO1 = param.numOutL1;
    param.incResO1 = 32; // next W4C8
    param.incResO2 = 32 * (oxp / 4 - oxp_w4c8 * valid_acc_num + 1);

#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22
    int step_x = 32; // step in bytes
    int ifm_w_pad = (param.ifm_width + 7)/8; // IFM padding to W8
    int oxp = (param.ifm_width - param.ksize)/param.stride + 1;
    int8_t valid_acc_num = (param.stride == 2 ? 4 : 8);
    int oxp_w8c8 = (oxp + 4 * valid_acc_num - 1)/(4 * valid_acc_num);

    // Output stride store_v & shuffle
    param.shfMode = (param.stride == 2 ? DINTLV_lo_64o128 : DINTLV_hi_512o1024);
    param.stepS = (param.stride == 2 ? 0 : (2 * step_x));

    // Loop count
    param.outer_g = (param.ifm_depth / 8) * oxp_w8c8;
    param.inner_g = (param.ksize+7) / 8; // ceil(ksize/8)

    // Inner 2D iterartor for addressing IFM
    param.numInnL1 = param.inner_g - 1;
    param.incInnL1 = -6 * step_x; // for next filter_k
    param.incInnL2 = -param.numInnL1 * 2 * step_x; // next W32

    // Outer 2D iterartor for addressing IFM
    param.numOutL1 = oxp_w8c8 - 1;
    param.incOutL1 = 0; // sit at next W32
    param.incOutL2 = (ifm_w_pad * 2 - 8*(1+param.numOutL1)) * step_x; // next C8

    // Filter 2D iterator for addressing WTS
    param.numK1 = param.numOutL1;
    param.incK1 = -64 * param.inner_g; // reset for next W32
    param.incK2 = 0; // reset for next C8

    // Bias 2D iterator
    param.numB1 = param.numOutL1;
    param.incB1 = 0;
    param.incB2 = 128; // 2*C8*8 per sliding mul

    // Store result 2D iterator for addressing OFM
    param.numResO1 = param.numOutL1;
    param.incResO1 = 64; // next W8C8
    param.incResO2 = 32 * (oxp / 4 - oxp_w8c8 * valid_acc_num + 2);

#endif
}

static NOINLINE void setup_conv1d_dw_params_bf16(int8_t * params_ptr, conv1d_dw_params_t &param, uint8_t batch_size) {
    // Load RTP and set the parameter structure
    aie::vector<int32,8> pars = aie::load_v<8>((int32 *)(params_ptr));
    aie::store_v((int32 __aie_dm_resource_a *) &param, pars);

#if __AIE_ARCH__ == 20
    int step_x = 32; // step in bytes
    int ifm_w_pad = (param.ifm_width + 3)/4; // IFM padding to W4
    int oxp = (param.ifm_width - param.ksize)/param.stride + 1;
    int8_t valid_acc_num = (param.stride == 2 ? 2 : 4);
    int oxp_w4c8 = (oxp + 4 * valid_acc_num - 1)/(4 * valid_acc_num);

    // Output stride store_v & shuffle
    param.shfMode = (param.stride == 2 ? 8 : 16);
    param.stepS = (param.stride == 2 ? 0 : step_x);

    // Loop count
    param.outer_g = (param.ifm_depth / 8) * oxp_w4c8;
    param.inner_g = ((param.ksize+3) / 4) * 2; // ceil(ksize/4) * 2

    // Inner 2D iterartor for addressing IFM
    param.numInnL1 = param.inner_g - 1;
    param.incInnL1 = -7 * step_x; // for next filter_k
    param.incInnL2 = -param.numInnL1 * step_x; // next W16

    // Outer 2D iterartor for addressing IFM
    param.numOutL1 = oxp_w4c8 - 1;
    param.incOutL1 = 0; // sit at next W16
    param.incOutL2 = (ifm_w_pad * 2 - 8*(1+param.numOutL1)) * step_x; // next C8

    // Filter 2D iterator for addressing WTS
    param.numK1 = param.numOutL1;
    param.incK1 = -32 * param.inner_g; // reset for next W16
    param.incK2 = 0; // reset for next C8

    // Bias 2D iterator
    param.numB1 = param.numOutL1;
    param.incB1 = 0;
    param.incB2 = 64; // 2*C8*2 per mac_elem, skip 2*C8*2 dummy

    // Store result 2D iterator for addressing OFM
    param.numResO1 = param.numOutL1;
    param.incResO1 = param.stepS; // next W2C8
    param.incResO2 = 32 * (oxp / 2 - oxp_w4c8 * valid_acc_num * 2) + param.stepS;

#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22
    int step_x = 32; // step in bytes
    int ifm_w_pad = (param.ifm_width + 3)/4; // IFM padding to W4
    int oxp = (param.ifm_width - param.ksize)/param.stride + 1;
    int8_t valid_acc_num = (param.stride == 2 ? 1 : 2);
    int oxp_w4c8 = (oxp + 4 * valid_acc_num - 1)/(4 * valid_acc_num);

    // Output stride store_v & shuffle
    param.shfMode = (param.stride == 2 ? DINTLV_lo_128o256 : DINTLV_hi_512o1024);
    param.stepS = (param.stride == 2 ? 0 : 2 * step_x);

    // Loop count
    param.outer_g = (param.ifm_depth / 8) * oxp_w4c8;
    param.inner_g = (param.ksize+3) / 4; // ceil(ksize/4)

    // Inner 2D iterartor for addressing IFM
    param.numInnL1 = param.inner_g - 1;
    param.incInnL1 = -2 * step_x; // for next filter_k
    param.incInnL2 = -param.numInnL1 * 2 * step_x; // next W8

    // Outer 2D iterartor for addressing IFM
    param.numOutL1 = oxp_w4c8 - 1;
    param.incOutL1 = 0; // sit at next W8
    param.incOutL2 = (ifm_w_pad * 2 - 4*(1+param.numOutL1)) * step_x; // next C8

    // Filter 2D iterator for addressing WTS
    param.numK1 = param.numOutL1;
    param.incK1 = -64 * param.inner_g; // reset for next W8
    param.incK2 = 0; // reset for next C8

    // Bias 2D iterator
    param.numB1 = param.numOutL1;
    param.incB1 = 0;
    param.incB2 = 64;

    // Store result 2D iterator for addressing OFM
    param.numResO1 = param.numOutL1;
    param.incResO1 = 64; // next W4C8
    param.incResO2 = 64 * (oxp / 4 - oxp_w4c8 * valid_acc_num + 1);

#endif
}

#endif  //__CONV1D_DW_PARAMS_H__
