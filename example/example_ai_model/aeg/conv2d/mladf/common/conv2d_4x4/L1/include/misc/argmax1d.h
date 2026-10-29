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

#ifndef __ARGMAX1D_H__
#define __ARGMAX1D_H__

#define USE_LOAD_64 0 // Set to 1 to use load_v<64>
                      // Set to 0 to use load_v<32>

#include "aie.hpp"
#include "aie_api/utils.hpp" // needed for aie:print
#include <adf.h>

#include "argmax1d_params.h"
#include "utils.h"

//#define DEBUG_argmax1d true

using namespace aie;

template <typename dtype>
NOINLINE void
argmax1d_internal(dtype __aie_dm_resource_a * __restrict input_ptr,
                  uint32_t __aie_dm_resource_b * __restrict output_ptr, uint32_t num_iter,
                  uint32_t start_arg) {

  alignas(aie::vector_decl_align) uint16_t args[32] = {
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

  // the 1-st iteration
  vector<uint16_t, VEC_LEN> args_vec = load_v<VEC_LEN>(args);
  vector<uint16_t, VEC_LEN> current_arg_max = args_vec;
  vector<dtype, VEC_LEN> current_max =
      load_v<VEC_LEN>((dtype __aie_dm_resource_a *)input_ptr);
  args_vec =
      add<vector<uint16_t, VEC_LEN>, uint16_t>(args_vec, (uint16_t)VEC_LEN);
  input_ptr = byte_incr(input_ptr, VEC_LEN * sizeof(dtype));

  // other iterstions
  mask<VEC_LEN> is_large_than_current;
  AIE_PREPARE_FOR_PIPELINING 
  AIE_LOOP_UNROLL(2) 
  AIE_LOOP_RANGE(4, ) 
  for (int j = 0; j < num_iter - 1; j++){

      // load input
      vector<dtype, VEC_LEN> input_buff =
          load_v<VEC_LEN>((dtype __aie_dm_resource_a *)input_ptr);

      // get max args
      is_large_than_current = gt(input_buff, current_max);
      current_max = max(input_buff, current_max);
      current_arg_max =
          select(current_arg_max, args_vec, is_large_than_current);

      // update args
      args_vec =
          add<vector<uint16_t, VEC_LEN>, uint16_t>(args_vec, (uint16_t)VEC_LEN);
      input_ptr = byte_incr(input_ptr, VEC_LEN * sizeof(dtype));

    }

  // reduce vecs to final argmax and generate the smallest arg same as reference
  dtype global_max = reduce_max(current_max);
  is_large_than_current = ge(current_max, global_max);
  vector<uint16_t, VEC_LEN> args_mix =
      select((uint16_t)65535, current_arg_max, is_large_than_current);

  uint16_t arg_max = reduce_min(args_mix);
  uint32_t global_arg_max = start_arg + arg_max;
  output_ptr[0] = global_arg_max;

}

template <typename dtype, typename config_ifm, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
INLINE void argmax1d(adf::input_buffer_conf<dtype, config_ifm> &__restrict ifm,
              adf::output_buffer_conf<uint32_t, config_ofm> &__restrict ofm,
              argmax1d_params_t &argmax1d_params) {

  dtype __aie_dm_resource_a *input_ptr =
      (dtype __aie_dm_resource_a *)ifm.data();
  uint32_t __aie_dm_resource_b *output_ptr =
      (uint32_t __aie_dm_resource_b *)ofm.data();

  argmax1d_internal<dtype>(input_ptr, output_ptr, argmax1d_params.num_iter,
                           argmax1d_params.start_arg);
}

#endif //__ARGMAX1D_H__
