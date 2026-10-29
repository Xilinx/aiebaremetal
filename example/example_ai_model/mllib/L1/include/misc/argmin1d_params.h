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

#ifndef __ARGMIN1D_PARAMS_H__
#define __ARGMIN1D_PARAMS_H__

#include "aie.hpp"
#include <adf.h>

// Buffer placement directives for IFM, WTs and OFM buffers.
#ifndef IFM_DM_BANK
#define IFM_DM_BANK __aie_dm_resource_a
#endif

#ifndef OFM_DM_BANK
#define OFM_DM_BANK __aie_dm_resource_b
#endif

#ifndef INPUT_SIZE
#define INPUT_SIZE 32
#endif

#ifndef INPUT_ROWS
#define INPUT_ROWS 16
#endif

#ifndef INPUT_COLS
#define INPUT_COLS 64
#endif

#ifndef VEC_LEN_ARGMIN
#define VEC_LEN_ARGMIN 32
#endif

#ifndef ARGMIN1D_LP_SIZE
#define ARGMIN1D_LP_SIZE 5
#endif

struct alignas(v32uint16) argmin1d_params_t {
  uint32_t input_rows;
  uint32_t input_cols;
  uint32_t input_size;
  uint32_t num_iter;
  uint32_t start_arg;
  uint32_t previous_argmin;
  uint32_t previous_min;
};

// Compile Time Param Loading
static NOINLINE void load_argmin1d_rtp(argmin1d_params_t &param) {
  param.input_rows = INPUT_ROWS;
  param.input_cols = INPUT_COLS;
}

static NOINLINE void setup_argmin1d(argmin1d_params_t &param) {
  param.input_size = param.input_rows * param.input_cols;
  param.num_iter = param.input_size / (VEC_LEN_ARGMIN);
}

// Params fixed at compile time
static NOINLINE void setup_argmin1d_params(argmin1d_params_t &param) {
  load_argmin1d_rtp(param);
  setup_argmin1d(param);
}

// Run Time Param Loading
static NOINLINE void load_argmin1d_rtp(argmin1d_params_t &param,
                                const uint32_t *lp) {
  param.input_rows = lp[0];
  param.input_cols = lp[1];
  param.start_arg = lp[2];
  param.previous_argmin = lp[3];
  param.previous_min = lp[4];
}

static NOINLINE void setup_argmin1d_params(argmin1d_params_t &param,
                                    const uint32_t *lp) {
  load_argmin1d_rtp(param, lp);
  setup_argmin1d(param);
}

#endif //__ARGMIN1D_PARAMS_H__
