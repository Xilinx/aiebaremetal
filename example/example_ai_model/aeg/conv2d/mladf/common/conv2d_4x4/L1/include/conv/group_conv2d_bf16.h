// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
/*
(c) Copyright 2019 - 2021 Xilinx, Inc. All Rights reserved.
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

/* 
 * 
 * 2D BF16 Convolution Kernel for AI Engine
 *
 * Performs 2d bf26 convolution on data present in "ifms" using weights from "wts"
 * Result is stored in "ofms".
 *
 */

#pragma once
#include "conv2d_bf16.h"
#include <adf.h>
#include "aie.hpp"

#include "utils.h"         // Contains some utility functions and definitions related to intrinsics usage
#include "mllib_const.h"   // Constants shared across graphs like activation type
#include "group_conv2d_bf16_params.h" // Parameters like increments and wraps needed by kernel

using namespace aie;


template <uint8_t batch_size, act_t act_type, typename dtype_ifm, typename dtype_wts, typename dtype_ofm, typename config_ifm, typename config_wts, typename config_ofm> 
NOINLINE void group_conv2d_bf16
(
        adf::input_buffer_conf<dtype_ifm, config_ifm>  & __restrict ifm,
        adf::input_buffer_conf<dtype_wts, config_wts>  & __restrict wts,
        adf::output_buffer_conf<dtype_ofm, config_ofm> &  ofm,
        group_conv2d_bf16_params_t            &group_conv2d_bf16_params
) {
    conv2d_bf16_params_t& conv2d_params = group_conv2d_bf16_params.conv2d_bf16_params;
    dtype_ifm* ifm_ptr = ifm.data();
    dtype_wts* wts_ptr = wts.data();
    dtype_ofm* ofm_ptr = ofm.data();
    // size in elems
    // TODO(zhonglin, ximing): calculate ifm_size & wts_size from RTP, instead of buffer size
    uint32_t ifm_size = ifm.size();
    uint32_t wts_size = wts.size();


    uint32_t ofm_hop = group_conv2d_bf16_params.single_group_ofm_len;
    uint32_t ifm_hop = ifm_size / group_conv2d_bf16_params.sv_groups;
    uint32_t wts_hop = wts_size / group_conv2d_bf16_params.sv_groups;

    // for sub groups, kernel expect (SV_GROUP, H, C/8, W, C8) for both input and output.
    // tiling is responsible to convert (SV_GROUP, H, C/8, W, C8) into (H, C/8, SV_GROUP, W, C8) for output in L1 - L2 
    AIE_LOOP_RANGE(1, ) 
    for(auto i = 0; i < group_conv2d_bf16_params.sv_groups; i++){
        using sync_config = adf::io_buffer_config<adf::extents<4294967295u>, adf::locking::sync, adf::addressing::linear, adf::margin<0>>;
        // it's internal buffer, should be safe to use sync lock
        // TODO(zhonglin, ximing): adotps conv2d bf16 ptr interface kernel as well
        adf::input_buffer_conf<dtype_ifm, sync_config> group_ifm(ifm_ptr, ifm_hop, ifm_hop*i, ifm_hop);
        adf::input_buffer_conf<dtype_wts, sync_config> group_wts(wts_ptr, wts_hop, wts_hop*i, wts_hop);
        adf::output_buffer_conf<dtype_ofm, sync_config> group_ofm(ofm_ptr, ofm_hop, ofm_hop*i, ofm_hop);

        conv2d_bf16<batch_size, act_type, dtype_ifm, dtype_wts, dtype_ofm, sync_config, sync_config, sync_config>
        (group_ifm, group_wts, group_ofm, conv2d_params);
    }
}
