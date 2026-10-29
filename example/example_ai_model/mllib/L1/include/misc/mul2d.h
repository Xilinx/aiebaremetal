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


#ifndef __MUL2D_H__
#define __MUL2D_H__

#include "aie.hpp"
#include <adf.h>

#include "utils.h"
#include "mllib_const.h"
#include "mul2d_params.h"

using namespace aie;

template <typename dtype_ifm1, typename dtype_ifm2, typename dtype_ofm>
    requires(mllib::utils::is_one_of_v<dtype_ifm1, int8_t, uint8_t, bfloat16>) &&
            (mllib::utils::is_one_of_v<dtype_ifm2, int8_t, uint8_t, bfloat16>) &&
            (mllib::utils::is_one_of_v<dtype_ofm, int8_t, uint8_t, bfloat16>)
void __attribute__((noinline)) mul2d(
   dtype_ifm1 __aie_dm_resource_a * __restrict it_ifm1,
   dtype_ifm2 __aie_dm_resource_b * __restrict it_ifm2,
   dtype_ofm __aie_dm_resource_c * __restrict it_ofm,
   mul2d_params_t params
) {

   SETUP_ITR3D_COUNTERS(itr_left)
   const int result_sign = 1 - (int) params.act;

   #define MUL2D(it_ifm1, it_ifm2, acc_res){\
      vector<dtype_ifm1, 64> input_ifm1;\
      vector<dtype_ifm2, 64> input_ifm2;\
      input_ifm1 = load_v<64>(it_ifm1);\
      input_ifm2 = load_v<64>(it_ifm2);\
      it_ifm1 = INCR_ITR3D_PARAM(it_ifm1, itr_left, params);\
      it_ifm2 = INCR_ITR3D_PARAM(it_ifm2, itr_left, params);\
      acc_res = mul(input_ifm1, input_ifm2);\
      store_v(it_ofm, acc_res.to_vector_sign<dtype_ofm>(result_sign,params.shift_out)); it_ofm+=64;\
   }\

   AIE_PREPARE_FOR_PIPELINING 
   AIE_LOOP_RANGE(8,) 
   AIE_LOOP_UNROLL(2) 
   for(int i=0; i<params.num_iters; i++){
      if constexpr(std::is_same<bfloat16, dtype_ifm1>::value){
         accum<accfloat, 64> acc_res;
	 MUL2D(it_ifm1, it_ifm2, acc_res)
      }else{
         accum<acc32,64> acc_res;
	 MUL2D(it_ifm1, it_ifm2, acc_res)
      }
    }
}

template <typename dtype_ifm1, typename dtype_ifm2, typename dtype_ofm, typename config_ifm1, typename config_ifm2, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype_ifm1, int8_t, uint8_t, bfloat16>) &&
            (mllib::utils::is_one_of_v<dtype_ifm2, int8_t, uint8_t, bfloat16>) &&
            (mllib::utils::is_one_of_v<dtype_ofm, int8_t, uint8_t, bfloat16>)
INLINE void mul2d(
   adf::input_buffer_conf<dtype_ifm1, config_ifm1>  & __restrict ifm1,
   adf::input_buffer_conf<dtype_ifm2, config_ifm2>  & __restrict ifm2,
   adf::output_buffer_conf<dtype_ofm, config_ofm> & __restrict ofm,
   mul2d_params_t params
) {

   dtype_ifm1 __aie_dm_resource_a * __restrict it_ifm1 = (dtype_ifm1 __aie_dm_resource_a * __restrict) (ifm1.data() + params.row_offset);
   dtype_ifm2 __aie_dm_resource_b * __restrict it_ifm2 = (dtype_ifm2 __aie_dm_resource_b * __restrict) (ifm2.data() + params.col_offset + params.header_offset);
   dtype_ofm __aie_dm_resource_c * __restrict it_ofm   = (dtype_ofm __aie_dm_resource_c * __restrict) ofm.data();
   return mul2d<dtype_ifm1, dtype_ifm2, dtype_ofm>(it_ifm1, it_ifm2, it_ofm, params);
}

#endif //__MUL2D_H__

