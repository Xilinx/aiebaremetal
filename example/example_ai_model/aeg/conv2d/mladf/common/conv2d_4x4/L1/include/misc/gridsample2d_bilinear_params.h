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

#ifndef __gridsample2d_bilinear_PARAMS_H__
#define __gridsample2d_bilinear_PARAMS_H__

#include "utils.h"

// Buffer placement directives for IFM, WTs and OFM buffers.
#ifndef IFM_DM_BANK
#define IFM_DM_BANK __aie_dm_resource_a
#endif

#ifndef WT_DM_BANK
#define WT_DM_BANK __aie_dm_resource_a
#endif

#ifndef OFM_DM_BANK
#define OFM_DM_BANK
#endif

#ifndef VEC_LEN
#define VEC_LEN 32
#endif

enum AlignedCornerMode {
    CORNER,
    CENTER,
};

enum PaddingMode {
    ZEROS,
    BORDER,
};

// TODO: need to check if keep_in_registers is required.
struct alignas(v16uint16) gridsample2d_bilinear_internal_params {
    uint32_t ifm_height;
    uint32_t ifm_depth;
    uint32_t ifm_width;
    uint32_t ofm_height;
    uint32_t ofm_width;
    uint32_t local_width;
    uint32_t local_height;
    int32_t x_start_pos;
    int32_t y_start_pos;
    int32_t c_start_pos;
};

typedef struct gridsample2d_bilinear_internal_params gridsample2d_bilinear_params_t;

static NOINLINE void setup_gridsample2d_bilinear_params(const uint32_t* params_ptr, gridsample2d_bilinear_params_t& param) {
    param.ifm_height = params_ptr[0];
    param.ifm_width = params_ptr[1];
    param.ifm_depth = params_ptr[2];
    param.ofm_height = params_ptr[3];
    param.ofm_width = params_ptr[4];
    param.local_height = params_ptr[5];
    param.local_width = params_ptr[6];
    param.x_start_pos = 0;
    param.y_start_pos = 0;
    param.c_start_pos = 0;
}

#endif //__gridsample2d_bilinear_PARAMS_H__
