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

#ifndef __TILE_H__
#define __TILE_H__

#include "aie.hpp"
#include <adf.h>
#include <aie_api/utils.hpp>

#include "tile_params.h"
#include "utils.h"

using namespace aie;

template<typename dtype, int LOAD_SIZE_t>
INLINE void tile_op_1xd(dtype* ifm_ptr, dtype* ofm_ptr, tile_params_t& pmtr){

  SETUP_ITR2D_COUNTERS(ifm_dim_2)
  SETUP_ITR2D_COUNTERS(ifm_dim_1)
  SETUP_ITR2D_COUNTERS(ifm_dim_0)

  int remain_elems = pmtr.rep_shape[3] % LOAD_SIZE_t;

  for (int d0 = 0; d0 < pmtr.out_shape[0]; d0++) {
    for (int d1 = 0; d1 < pmtr.out_shape[1]; d1++){
      for (int d2 = 0; d2 < pmtr.out_shape[2]; d2++){
        auto vec_in = broadcast<dtype, LOAD_SIZE_t>(*ifm_ptr);
        AIE_PREPARE_FOR_PIPELINING 
        for (int d3 = 0; d3 < pmtr.rep_shape[3] / LOAD_SIZE_t; d3+=1){
          store_unaligned_v(ofm_ptr, vec_in);
          ofm_ptr += LOAD_SIZE_t;
        }
        store_unaligned_v(ofm_ptr, vec_in);
        ofm_ptr += remain_elems;
        INCR_ITR2D_PARAM(ifm_ptr, ifm_dim_2, pmtr);
      }
      INCR_ITR2D_PARAM(ifm_ptr, ifm_dim_1, pmtr);
    }
    INCR_ITR2D_PARAM(ifm_ptr, ifm_dim_0, pmtr);
  }
}

template<typename dtype, int LOAD_SIZE_t>
INLINE void tile_op_dxd(dtype* ifm_ptr, dtype* ofm_ptr, tile_params_t& pmtr){

  int num_full_vecs;
  int num_repeat;
  int ifm_offset;
  int ofm_offset;

  int remain_elems = pmtr.in_shape[3] % LOAD_SIZE_t;
  int full_vecs_elems = pmtr.in_shape[3] - remain_elems;

  if (remain_elems == 0){
    num_full_vecs = pmtr.in_shape[3] / LOAD_SIZE_t;
    num_repeat = full_vecs_elems / LOAD_SIZE_t - 1;
    ifm_offset = - full_vecs_elems + LOAD_SIZE_t;
    ofm_offset = LOAD_SIZE_t;
  }
  else{
    num_full_vecs = pmtr.in_shape[3] / LOAD_SIZE_t + 1;
    num_repeat = full_vecs_elems / LOAD_SIZE_t;
    ifm_offset = - full_vecs_elems;
    ofm_offset = remain_elems;
  }

  SETUP_ITR2D_COUNTERS(ofm_dim_3)
  SETUP_ITR2D_PARAM(ofm_dim_3, TILE_VEC_LEN, num_repeat, ofm_offset * sizeof(dtype), pmtr)

  SETUP_ITR2D_COUNTERS(ifm_dim_3)
  SETUP_ITR2D_PARAM(ifm_dim_3, TILE_VEC_LEN, num_repeat, ifm_offset * sizeof(dtype), pmtr)

  SETUP_ITR2D_COUNTERS(ifm_dim_2)
  SETUP_ITR2D_COUNTERS(ifm_dim_1)
  SETUP_ITR2D_COUNTERS(ifm_dim_0)

  for (int d0 = 0; d0 < pmtr.out_shape[0]; d0++) {
    for (int d1 = 0; d1 < pmtr.out_shape[1]; d1++){
      for (int d2 = 0; d2 < pmtr.out_shape[2]; d2++){
        AIE_PREPARE_FOR_PIPELINING 
        for (int d3 = 0; d3 < num_full_vecs * pmtr.rep_shape[3]; d3+=1){
          auto vec_in = load_unaligned_v<LOAD_SIZE_t>(ifm_ptr);
          INCR_ITR2D_PARAM(ifm_ptr, ifm_dim_3, pmtr);

          store_unaligned_v(ofm_ptr, vec_in);
          INCR_ITR2D_PARAM(ofm_ptr, ofm_dim_3, pmtr);
        }
        INCR_ITR2D_PARAM(ifm_ptr, ifm_dim_2, pmtr);
      }
      INCR_ITR2D_PARAM(ifm_ptr, ifm_dim_1, pmtr);
    }
    INCR_ITR2D_PARAM(ifm_ptr, ifm_dim_0, pmtr);
  }
}


template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
__attribute__((noinline)) void tile_op(
    dtype* __restrict ifm_data,
    dtype* __restrict ofm_data,
    tile_params_t& pmtr
) {
    dtype IFM_DM_BANK *__restrict ifm_ptr =
      (dtype IFM_DM_BANK * __restrict)ifm_data;
    dtype OFM_DM_BANK *__restrict ofm_ptr =
      (dtype OFM_DM_BANK * __restrict)ofm_data;

    SETUP_ITR2D_PARAM(ifm_dim_2, pmtr.ifm_inc[2] * sizeof(dtype), pmtr.in_shape[2] - 1, pmtr.ifm_offset[2] * sizeof(dtype), pmtr)
    SETUP_ITR2D_PARAM(ifm_dim_1, pmtr.ifm_inc[1] * sizeof(dtype), pmtr.in_shape[1] - 1, pmtr.ifm_offset[1] * sizeof(dtype), pmtr)
    SETUP_ITR2D_PARAM(ifm_dim_0, pmtr.ifm_inc[0] * sizeof(dtype), pmtr.in_shape[0] - 1, pmtr.ifm_offset[0] * sizeof(dtype), pmtr)

    const int LOAD_SIZE_t = TILE_VEC_LEN / sizeof(dtype);

    if(pmtr.in_shape[3] == 1){// AxBxCx1 -> AxBxCxD or other cases that innermost dim of input euqals to 1
      tile_op_1xd<dtype, LOAD_SIZE_t>(ifm_ptr, ofm_ptr, pmtr);
    }
    else{// AxBx1xD -> AxBxCxD or other cases that innermost dim of input not euqals to 1
      tile_op_dxd<dtype, LOAD_SIZE_t>(ifm_ptr, ofm_ptr, pmtr);
    }
}

template <typename dtype, typename config_ifm, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
INLINE void tile_op(
    adf::input_buffer_conf<dtype, config_ifm> &__restrict ifm,
    adf::output_buffer_conf<dtype, config_ofm> &__restrict ofm,
    tile_params_t& pmtr
) {
  return tile_op<dtype>(ifm.data(), ofm.data(), pmtr);
}
#endif
