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

#ifndef __HARDSIGMOID_PARAMS_H__
#define __HARDSIGMOID_PARAMS_H__

#include "aie.hpp"
#include <adf.h>

// Buffer placement directives for IFM, WTs and OFM buffers.
#ifndef IFM_DM_BANK
#define IFM_DM_BANK __aie_dm_resource_a
#endif

#ifndef OFM_DM_BANK
#define OFM_DM_BANK __aie_dm_resource_a
#endif

//bfloat16 sigmoid kernel setup

#ifndef INPUT_SIZE
#define INPUT_SIZE 32
#endif

#ifndef INPUT_ROWS
#define INPUT_ROWS 16
#endif

#ifndef INPUT_COLS
#define INPUT_COLS 64
#endif

#ifndef HARDSIGMOID_LP_SIZE
#define HARDSIGMOID_LP_SIZE 2
#endif

// =========================================================================
// 1. templated params

template <typename dtype>
struct hardsigmoid_templated_params_t {
};

template <>
struct hardsigmoid_templated_params_t<bfloat16> {
    bfloat16 alpha;
    bfloat16 beta;
};

template <>
struct hardsigmoid_templated_params_t<int8_t> {
    uint16_t alpha;
    uint16_t beta;
};


// =========================================================================
// 2. Following codes are original codes without templated
struct alignas(v16uint16) hardsigmoid_internal_params {
  uint8_t input_rows;
  uint8_t input_cols;

  // int8 kernel params
  uint8_t input_scale;
  int8_t shift_out;
  uint16_t div_factor;

  // derived params
  uint16_t constant_factor;
  unsigned int input_size;
  uint32_t num_iter;

};

typedef struct hardsigmoid_internal_params  hardsigmoid_internal_params_t ;

// ********************* bfloat16 hardsigmoid setup ******************

//Compile Time Param Loading

static NOINLINE void load_hardsigmoid_rtp(hardsigmoid_internal_params_t &param)
{
  param.input_rows = INPUT_ROWS;
  param.input_cols = INPUT_COLS;
}

static NOINLINE void setup_hardsigmoid(hardsigmoid_internal_params_t &param)
{
  param.input_size = param.input_rows*param.input_cols;
  // define iter rounds
  param.num_iter = param.input_size / 16;
}

static NOINLINE void setup_hardsigmoid_params(hardsigmoid_internal_params_t &param){
  load_hardsigmoid_rtp(param);
  setup_hardsigmoid(param);
}

//Run Time Param Loading

static NOINLINE void load_hardsigmoid_rtp(hardsigmoid_internal_params_t  &param,const uint32_t (&lp)[HARDSIGMOID_LP_SIZE]) {

    param.input_rows         = (uint8) lp[0];
    param.input_cols         = (uint8) lp[1];
}

static NOINLINE void setup_hardsigmoid_params(hardsigmoid_internal_params_t &param, const uint32_t (&lp)[HARDSIGMOID_LP_SIZE]) {
   load_hardsigmoid_rtp(param,lp);
   setup_hardsigmoid(param);
}

// ********************* int8 hardsigmoid setup ******************

// Run Time Param Loading

static NOINLINE void load_hardsigmoid_rtp(hardsigmoid_internal_params_t  &param, int8_t * ptr) {

    param.input_rows         = (uint8_t) *ptr++;
    param.input_cols         = (uint8_t) *ptr++;
    ptr+=4;
    param.shift_out          = (int8_t) *ptr++;
    ptr+=9;
    param.input_scale        = (uint8_t) *ptr++;
    ptr+=42;
    param.div_factor         = *((uint16_t *) ptr);
}

static NOINLINE void setup_hardsigmoid(hardsigmoid_internal_params_t &param, int8_t * ptr) {

   param.input_size = param.input_rows*param.input_cols;
   param.num_iter = param.input_size / 32;
   param.constant_factor = param.div_factor * 3;
}


static NOINLINE void setup_hardsigmoid_params(hardsigmoid_internal_params_t &param, int8_t * ptr){
  load_hardsigmoid_rtp(param, ptr);
  setup_hardsigmoid(param, ptr);
}



#endif //__HARDSIGMOID_PARAMS_H__
