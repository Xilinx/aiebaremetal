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

#ifndef __ARGMIN1D_H__
#define __ARGMIN1D_H__

#include "aie.hpp"
#include "aie_api/utils.hpp" // needed for aie:print
#include <adf.h>

#include "argmin1d_params.h"
#include "utils.h"

//#define DEBUG_argmin1d true

using namespace aie;

template <typename dtype>
NOINLINE void
argmin1d_internal(dtype __aie_dm_resource_a * __restrict input_ptr,
                  uint32_t __aie_dm_resource_b * __restrict output_ptr, uint32_t num_iter,
                  uint32_t start_arg, uint32_t previous_argmin,
                  uint32_t previous_min) {

#if VEC_LEN_ARGMIN == 32
  alignas(aie::vector_decl_align) uint16_t args[VEC_LEN_ARGMIN] = {
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
#elif VEC_LEN_ARGMIN == 64
  alignas(aie::vector_decl_align) uint16_t args[VEC_LEN_ARGMIN] = {
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
      32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
      48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};
#endif

  union {
    uint32_t uint_value;
    dtype bf16_value[2];
  } bf16_cast;
  dtype previous_min_value;
  if
    constexpr(std::is_same<bfloat16, dtype>::value) {
      bf16_cast.uint_value = previous_min;
      previous_min_value = bf16_cast.bf16_value[0];
    }
  else {
    previous_min_value = previous_min & 0xff;
  }

  // the 1-st iteration
  vector<uint16_t, VEC_LEN_ARGMIN> args_vec = load_v<VEC_LEN_ARGMIN>(args);
  vector<uint16_t, VEC_LEN_ARGMIN> current_argmin = args_vec;
  vector<dtype, VEC_LEN_ARGMIN> current_min =
      load_v<VEC_LEN_ARGMIN>((dtype __aie_dm_resource_a *)input_ptr);
  args_vec = add<vector<uint16_t, VEC_LEN_ARGMIN>, uint16_t>(
      args_vec, (uint16_t)VEC_LEN_ARGMIN);
  input_ptr = byte_incr(input_ptr, VEC_LEN_ARGMIN * sizeof(dtype));

  // other iterstions
  mask<VEC_LEN_ARGMIN> is_large_than_current;
  AIE_PREPARE_FOR_PIPELINING 
  AIE_LOOP_UNROLL(2) 
  AIE_LOOP_RANGE(4, ) 
  for (int j = 0; j < num_iter - 1; j++){

      // load input
      vector<dtype, VEC_LEN_ARGMIN> input_buff =
          load_v<VEC_LEN_ARGMIN>((dtype __aie_dm_resource_a *)input_ptr);

      // get min args
      is_large_than_current = gt(current_min, input_buff);
      current_min = min(input_buff, current_min);
      current_argmin = select(current_argmin, args_vec, is_large_than_current);

      // update args
      args_vec = add<vector<uint16_t, VEC_LEN_ARGMIN>, uint16_t>(
          args_vec, (uint16_t)VEC_LEN_ARGMIN);
      input_ptr = byte_incr(input_ptr, VEC_LEN_ARGMIN * sizeof(dtype));

    }

  // reduce vecs to final argmin and generate the smallest arg same as reference
  dtype partial_min = reduce_min(current_min);
  is_large_than_current = ge(partial_min, current_min);
  vector<uint16_t, VEC_LEN_ARGMIN> args_mix =
      select((uint16_t)65535, current_argmin, is_large_than_current);
  uint32_t partial_argmin = start_arg + reduce_min(args_mix);

  // compare with previous value
  dtype global_min;
  uint32_t global_argmin;
  if (partial_min < previous_min_value) {
    global_argmin = partial_argmin;
    global_min = partial_min;
  } else {
    global_argmin = previous_argmin;
    global_min = previous_min_value;
  }

  // output
  // [0]:argmin (uint32)
  // [1]:min value (dtype: int8, bf16)
  if
    constexpr(std::is_same<bfloat16, dtype>::value) {
      bf16_cast.bf16_value[0] = global_min;
      bf16_cast.bf16_value[1] = 0;
      output_ptr[0] = global_argmin;
      output_ptr[1] = bf16_cast.uint_value;
    }
  else {
    output_ptr[0] = global_argmin;
    output_ptr[1] = global_min & 0xff;
  }
}

template <typename dtype, typename config_ifm, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
INLINE void argmin1d(adf::input_buffer_conf<dtype, config_ifm> &__restrict ifm,
              adf::output_buffer_conf<uint32_t, config_ofm> &__restrict ofm,
              argmin1d_params_t &argmin1d_params) {

  dtype __aie_dm_resource_a *input_ptr =
      (dtype __aie_dm_resource_a *)ifm.data();
  uint32_t __aie_dm_resource_b *output_ptr =
      (uint32_t __aie_dm_resource_b *)ofm.data();

  argmin1d_internal<dtype>(input_ptr, output_ptr, argmin1d_params.num_iter,
                           argmin1d_params.start_arg,
                           argmin1d_params.previous_argmin,
                           argmin1d_params.previous_min);
}

#endif //__ARGMIN1D_H__
