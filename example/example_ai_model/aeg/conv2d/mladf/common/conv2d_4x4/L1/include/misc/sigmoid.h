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


#ifndef __SIGMOID_H__
#define __SIGMOID_H__

#include <adf.h>
#include "sigmoid_params.h"
#include "sigmoid_impl.h"
#include "aie.hpp"
#include "aie_adf.hpp"

using namespace aie;

// =========================================================================
// 1. Templated sigmoid

// flexml interface
template <typename dtype>
using sigmoid1d = elementwise_unary<dtype, elementwise_sigmoid<dtype>,
                                    sigmoid_templated_params_t<dtype>>;

template <typename dtype, typename config_ifm, typename config_ofm>
__attribute__((noinline)) void sigmoid_templated(
    adf::input_buffer_conf<dtype, config_ifm> &__restrict ifm,
    adf::output_buffer_conf<dtype, config_ofm> &__restrict ofm,
    typename sigmoid1d<dtype>::params_t &params) {
    sigmoid1d<dtype>::run(ifm.data(), ofm.data(), params);
}

// =========================================================================
// 2. Following codes are original codes without templated
template <typename dtype>
NOINLINE void sigmoid(
        dtype * __restrict ifm_ptr,
        dtype * __restrict ofm_ptr,
        sigmoid_params_t &params
) {

    int8_t IFM_DM_BANK  * p_in    = (int8_t  IFM_DM_BANK *) ifm_ptr;
    int8_t OFM_DM_BANK  * p_out   = (int8_t  OFM_DM_BANK *) ofm_ptr;

    int8_t scale_bits = (params.input_scale_bits + 2); //Offset to avoid negative shift in UPS.

    //4, -4, 1/4 and 1/2 are scale adjusted and broadcasted into respective vectors
    //1/32 is always scaled to 1
    vector<int16_t, 32> plus_four_vec  = broadcast<int16_t, 32>((int16_t)(1<<scale_bits));
    vector<int16_t, 32> minus_four_vec = broadcast<int16_t, 32>((int16_t)((-1)<<scale_bits));
    vector<uint16_t,32> vec_1by4       = broadcast<uint16_t,32>((uint16_t)(2<<scale_bits));
    vector<int32_t, 32> vec_1by2       = broadcast<int32_t, 32>((int32_t)(1<<(2*scale_bits)));

    accum<acc32,32> acc_0;
    acc_0.from_vector(vec_1by2); //Initialize the accum with scaled 1/2.

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(8,) 
    for (int j=0; j<params.size/32; j++){
        vector<int8_t, 32> vec_in;
        vector<int16_t,32> vec_in_int16, vec_in_abs;
        accum<acc32,32>    acc_1,acc_2;

        vec_in       = load_v<32>(p_in); p_in = byte_incr(p_in,32);

        vec_in_int16 = unpack(vec_in); //Change input to int16 type. Only type change, no up-shift.
        vec_in_int16 = min(vec_in_int16, plus_four_vec); //Compare with 4.0 and clip input greater than it.
        vec_in_int16 = max(vec_in_int16, minus_four_vec); //Compare with -4.0 and clip input less than it.
        vec_in_abs   = abs(vec_in_int16);
        acc_1        = mac(acc_0,vec_in_int16, vec_1by4); //(1/2) + (1/4)*x
        acc_2        = msc(acc_1,vec_in_abs,vec_in_int16); //(1/2)+(1/4)*x-(1/32)*x*|x| (1/32 is scaled to 1).

        store_v(p_out, acc_2.to_vector<int8_t>(params.shift_out)); p_out = byte_incr(p_out,32);
    }
}

template <typename dtype, typename config_ifm, typename config_ofm>
INLINE void sigmoid(
        adf::input_buffer_conf<int8_t, config_ifm>  & __restrict ifm,
        adf::output_buffer_conf<int8_t, config_ofm> & __restrict ofm,
        sigmoid_params_t &params
) {
    return sigmoid<dtype>(ifm.data(), ofm.data(), params);
}

// Bfloat16 Sigmoid

// Sigmoid const
const bfloat16 C0 = 0.5;
const bfloat16 C1 = 0.25;
const bfloat16 C2 = 0.03125;
const bfloat16 lower_bound = -4.0;
const bfloat16 upper_bound = 4.0;


// Input is ifm.
// Output is in ofm.
// All computation is performed in Bfloat16
// Kernel parameters are passed by structure params

template <typename dtype>
NOINLINE void sigmoid
  (
     bfloat16 * __restrict in_ptr,
     bfloat16 * __restrict out_ptr,
     sigmoid_bf16_params_t  &params
  ) {
      dtype IFM_DM_BANK * __restrict p_in = (bfloat16 IFM_DM_BANK * __restrict)in_ptr; //TODO: Restrict
      dtype OFM_DM_BANK * __restrict p_out = (bfloat16 OFM_DM_BANK * __restrict)out_ptr; //TODO: Restrict

      vector<dtype, 16> C0_vec = broadcast<bfloat16, 16>(C0);
      accum<accfloat, 16> C0_acc;
      C0_acc.from_vector(C0_vec);

      // Sigmoid function
      AIE_PREPARE_FOR_PIPELINING 
      AIE_LOOP_RANGE(8,) 
      AIE_LOOP_UNROLL(2) 
      for (int i = 0; i < params.num_iter; i = i + 1){
         // preparing vector & accum
         vector<dtype, 16> vec_in, mul1_vec, mul1_vec_abs;
         accum<accfloat, 16> mul0, mac0, msc0;

	 // data input
         vec_in = load_v<16>(p_in);
         p_in = byte_incr(p_in, 32);

         //using sigmoid for approximation
         mul1_vec = min(vec_in, upper_bound);
         mul1_vec = max(mul1_vec, lower_bound);
         mul1_vec_abs = abs(mul1_vec);
         mul0 = mul(C2, mul1_vec_abs);
         mac0 = mac(C0_acc, mul1_vec, C1);
         msc0 = msc(mac0, mul0.template to_vector<dtype>(), mul1_vec);

         // result output
         store_v((bfloat16 *)p_out, msc0.to_vector<dtype>());
         p_out = byte_incr(p_out, 32);
      }
    }

template <typename dtype, typename config_ifm,typename config_ofm>
INLINE void sigmoid
  (
     adf::input_buffer_conf<bfloat16, config_ifm> &__restrict in,
     adf::output_buffer_conf<bfloat16, config_ofm> &__restrict out,
     sigmoid_bf16_params_t  &params
  ) {
    return sigmoid<dtype>(in.data(), out.data(), params);
}

#endif //__SIGMOID_H__
