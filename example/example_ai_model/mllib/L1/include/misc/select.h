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

#ifndef __SELECT_H__
#define __SELECT_H__

#include "aie.hpp"
#include <adf.h>
#include <aie_api/utils.hpp>

#include "select_params.h"
#include "utils.h"

using namespace aie;

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
NOINLINE void select(uint32_t* p_pred, dtype* __restrict p_ifm1, dtype* __restrict p_ifm2, dtype* __restrict p_ofm, select_params_t& pmtr){
  // load_size and num_iters
  constexpr int LOAD_SIZE = 64 / sizeof(dtype);
  pmtr.num_iter = pmtr.ifmsv_size / LOAD_SIZE;

  // pointer
  dtype __aie_dm_resource_a *__restrict p_in1 = (dtype __aie_dm_resource_a *__restrict) p_ifm1;// value tensor if true
  dtype __aie_dm_resource_b *__restrict p_in2 = (dtype __aie_dm_resource_b *__restrict) p_ifm2;// value tensor if false
  dtype __aie_dm_resource_c *__restrict p_out = (dtype __aie_dm_resource_c *__restrict) p_ofm;

  uint32_t __aie_dm_resource_b *__restrict p_in_pred;
  uint64_t __aie_dm_resource_b *__restrict p_in_pred_64;

  // pred setting
  if constexpr(LOAD_SIZE == 64){
    p_in_pred_64 = (uint64_t __aie_dm_resource_b *__restrict)(p_pred);
  }else{
    p_in_pred = (uint32_t __aie_dm_resource_b *__restrict)(p_pred);
  }

  AIE_PREPARE_FOR_PIPELINING 
  AIE_LOOP_RANGE(8, ) 
  for(int i = 0; i < pmtr.num_iter; i++){
      auto vec_0 = aie::load_v<LOAD_SIZE>(p_in1);
      p_in1 += LOAD_SIZE;
      auto vec_1 = aie::load_v<LOAD_SIZE>(p_in2);
      p_in2 += LOAD_SIZE;

      aie::mask<LOAD_SIZE> msk;

      if constexpr(LOAD_SIZE == 64){
        msk = aie::mask<LOAD_SIZE>::from_uint64(*p_in_pred_64);
        p_in_pred_64 = byte_incr(p_in_pred_64, 8);
      }else{
        msk = aie::mask<LOAD_SIZE>::from_uint32(*p_in_pred);
        p_in_pred = byte_incr(p_in_pred, 4);
      }

      auto vec_out = aie::select(vec_1, vec_0, msk); // select(false, true, mask)
      aie::store_v(p_out, vec_out);
      p_out += LOAD_SIZE;
  }
}

#endif
