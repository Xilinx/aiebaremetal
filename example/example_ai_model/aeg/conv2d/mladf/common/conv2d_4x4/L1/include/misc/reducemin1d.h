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

#ifndef __REDUCEMIN1D_H__
#define __REDUCEMIN1D_H__

#include "aie.hpp"
#include <adf.h>
#include <aie_api/utils.hpp>

#include "reducemin1d_params.h"
#include "utils.h"

using namespace aie;

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
__attribute__((noinline)) void
reducemin1d(dtype* __restrict ifm_data,
            dtype* __restrict ofm_data,
            reducemin1d_params_t & __restrict params) {
  dtype IFM_DM_BANK *__restrict p_in =
      (dtype IFM_DM_BANK * __restrict)ifm_data;
  dtype OFM_DM_BANK *__restrict p_out =
      (dtype OFM_DM_BANK * __restrict)ofm_data;

  // initial global_min with the first elem
  dtype global_min = *p_in;

  // reduce_min operate on input 1-d tensor
  AIE_PREPARE_FOR_PIPELINING 
  AIE_LOOP_RANGE(4, ) 
  for (int i = 0; i < params.num_iter; i = i + 1){

      aie::vector<dtype, VEC_LEN> vec_in = aie::load_v<VEC_LEN>(p_in);
      p_in = byte_incr(p_in, VEC_LEN * sizeof(dtype));

      dtype min_val = aie::reduce_min(vec_in);

      // update global min in each iter
      global_min = std::min(global_min, min_val);
    }

  // output min_val for dim[0] (currently this kernel supports dim-1 operation)
  *p_out = global_min;
}

template <typename dtype, typename config_ifm, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
INLINE void
reducemin1d(adf::input_buffer_conf<dtype, config_ifm> &__restrict ifm,
            adf::output_buffer_conf<dtype, config_ofm> &__restrict ofm,
            reducemin1d_params_t &params) {
  return reducemin1d<dtype>(ifm.data(), ofm.data(), params);
}

#endif //__REDUCEMIN1D_H__
