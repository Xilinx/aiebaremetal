/*
    Copyright (C) 2019 - 2022 Xilinx, Inc. All rights reserved.
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
#include "conv2d_bf16_params.h" 

struct alignas(v32uint16) group_conv2d_bf16_params_t {
    conv2d_bf16_params_t conv2d_bf16_params;
    uint8_t sv_groups; // at byte index 61
    // derived
    int32_t single_group_ofm_len;
};


static void setup_group_conv2d_bf16_params (int8_t * params_ptr, group_conv2d_bf16_params_t &param, uint8_t batch_size=1, uint8_t aximm_rtp_enabled = 0) {
    setup_conv2d_bf16_params(params_ptr, param.conv2d_bf16_params, batch_size, aximm_rtp_enabled);
    param.sv_groups = params_ptr[61];

    param.single_group_ofm_len = param.conv2d_bf16_params.oxp * param.conv2d_bf16_params.oyp * param.conv2d_bf16_params.ofm_sv_depth * param.conv2d_bf16_params.batch;
}
