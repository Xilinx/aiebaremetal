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

#ifndef __SLICE_H__
#define __SLICE_H__

#include "aie.hpp"
#include <adf.h>
#include <aie_api/utils.hpp>

#include "slice_params.h"
#include "utils.h"

using namespace aie;



template <typename dtype, typename config_ifm, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
__attribute__((noinline)) void slice(
    adf::input_buffer_conf<dtype, config_ifm> &__restrict ifm,
    adf::output_buffer_conf<dtype, config_ofm> &__restrict ofm,
    slice_params_t& params
) {
    dtype IFM_DM_BANK *__restrict ifm_ptr =
      (dtype IFM_DM_BANK * __restrict)ifm.data();
    dtype OFM_DM_BANK *__restrict ofm_ptr =
      (dtype OFM_DM_BANK * __restrict)ofm.data();
    SETUP_ITR3D_COUNTERS(iters_in)
    const int vec_size= 32;
    vector<dtype, vec_size> out_vec;
    uint8_t wrap0 = int((params.w_ed - params.w_st - 1) / params.w_sp);
    int16_t incr0 = params.w_sp;
    uint8_t wrap1 = int((params.c_ed - params.c_st - 1) / params.c_sp);
    int16_t incr1 = params.d2_size * params.c_sp - wrap0 * params.w_sp;
    int16_t incr2 = params.d1_size * params.d2_size * params.h_sp - params.d2_size * wrap1 * params.c_sp - wrap0 * params.w_sp;    
    SETUP_ITR3D_PARAM(iters_in,incr0 * sizeof(dtype),wrap0,incr1 * sizeof(dtype),wrap1,incr2 * sizeof(dtype),params)
    ifm_ptr += params.offset;

    AIE_LOOP_RANGE(4,) 
    AIE_PREPARE_FOR_PIPELINING 
    for (int i = 0; i < params.iter_num; i++){
      out_vec[0] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[1] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[2] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[3] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[4] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[5] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[6] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[7] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[8] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[9] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[10] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[11] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[12] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[13] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[14] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[15] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[16] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[17] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[18] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[19] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[20] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[21] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[22] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[23] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[24] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[25] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[26] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[27] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[28] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[29] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[30] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      out_vec[31] = *ifm_ptr;
      INCR_ITR3D_PARAM(ifm_ptr, iters_in, params)
      store_v(ofm_ptr, out_vec);
      ofm_ptr += vec_size;
    }




}
#endif