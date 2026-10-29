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

#include "utils.h"

struct alignas(v32uint16) resize_nearest_internal_params{
    // Loaded
    uint16_t total_ifm_height;
    uint16_t total_ifm_width;
    float step_h;
    float step_w;
    float offset_scale_height;
    float offset_scale_width;
    uint16_t offset_idx_out_height;
    uint16_t offset_idx_out_width;
    uint16_t offset_idx_in_height;
    uint16_t offset_idx_in_width;
    
    uint16_t kernel_ofm_height;
    uint16_t kernel_ofm_width;
    uint16_t kernel_ifm_width;
    uint16_t kernel_ifm_height;
};

typedef struct resize_nearest_internal_params resize_nearest_params_t;

static void setup_resize_nearest_params(resize_nearest_params_t &param,const uint32_t (&lp)[16])
{
    const uint32_t *__restrict params_ptr = lp;
    param.total_ifm_height = (uint16_t)params_ptr[0];
    param.total_ifm_width = (uint16_t)params_ptr[1];
    param.step_h = *reinterpret_cast<const float *>(params_ptr + 2);
    param.step_w = *reinterpret_cast<const float *>(params_ptr + 3);
    param.offset_scale_height = *reinterpret_cast<const float *>(params_ptr + 4);
    param.offset_scale_width = *reinterpret_cast<const float *>(params_ptr + 5);
    
    param.offset_idx_out_height = 0;
    param.offset_idx_out_width = 0;
    param.offset_idx_in_height = 0;
    param.offset_idx_in_width = 0;
}
