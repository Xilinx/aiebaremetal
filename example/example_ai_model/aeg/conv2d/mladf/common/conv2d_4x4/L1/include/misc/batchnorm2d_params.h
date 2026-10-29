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

#ifndef __BATCH_NORM_2D_PARAMS_H__
#define __BATCH_NORM_2D_PARAMS_H__

#include <type_traits>
#include <algorithm>
#include <adf.h>
#include "batchnorm2d_base.h"

struct alignas(v32uint16) batchnorm2d_params {
    uint32_t batch_num;
    uint32_t channel;
    uint32_t height;
    uint32_t width;
    uint32_t output_shift;
    // derived parameters
    uint32_t ofm_cw_offset;
    uint32_t ofm_cb_offset;
    uint32_t wts_cb_offset;
    uint32_t outer_loop_cnt;
    uint32_t inner_loop_cnt;
};

typedef struct batchnorm2d_params batchnorm2d_params_t;

template<typename dtype>
void setup_batchnorm2d_params(batchnorm2d_params_t &param, const uint32_t (&lp)[BN2D_LP_SIZE]) {
    using namespace mllib::batchnorm2d;
    param.batch_num = lp[0];
    param.channel = lp[1];
    param.height = lp[2];
    param.width = lp[3];
    param.output_shift = lp[4];
    // derived param
    const uint32_t num_elements = param.batch_num * param.channel * param.height * param.width;
    param.ofm_cw_offset = num_elements;
    param.wts_cb_offset = round_up(param.channel, load_sz<dtype>());    // wts layout make sure cw and cb heads are aligned to load_sz * sizeof(dtype)
    param.ofm_cb_offset = param.ofm_cw_offset + wts_dup<dtype>() * param.wts_cb_offset;   // note cw always have the same data type as ifm
    param.outer_loop_cnt = param.height * param.channel / c_stride;
    param.inner_loop_cnt = param.width * param.batch_num * c_stride / load_sz<dtype>();
}
#endif