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


#ifndef __HARDSIGMOID_H__
#define __HARDSIGMOID_H__

#include <adf.h>
#include "hardsigmoid_params.h"
#include "hardsigmoid_impl.h"
#include "aie.hpp"
#include "aie_adf.hpp"

using namespace aie;

// =========================================================================
// 1. Templated hardsigmoid

// flexml interface
template <typename dtype>
using hardsigmoid1d =
    elementwise_unary<dtype, elementwise_hardsigmoid<dtype>,
                      hardsigmoid_templated_params_t<dtype>>;

template <typename dtype, typename config_ifm, typename config_ofm>
__attribute__((noinline)) void hardsigmoid_templated(
    adf::input_buffer_conf<dtype, config_ifm> &__restrict ifm,
    adf::output_buffer_conf<dtype, config_ofm> &__restrict ofm,
    typename hardsigmoid1d<dtype>::params_t &params) {
    hardsigmoid1d<dtype>::run(ifm.data(), ofm.data(), params);
}

// =========================================================================
// 2. Following codes are original codes without templated

// Bfloat16 HardSigmoid
// HardSigmoid const
const bfloat16 C0_hardsigmoid = 0.5;
const bfloat16 C1_hardsigmoid = 0.166015625;
const bfloat16 lower_bound_hardsigmoid = -3.0;
const bfloat16 upper_bound_hardsigmoid = 3.0;


// Input is ifm.
// Output is in ofm.
// All computation is performed in Bfloat16
// Kernel parameters are passed by structure params

template <typename dtype_ifm, typename dtype_ofm>
NOINLINE void hardsigmoid
  (
     dtype_ifm IFM_DM_BANK * __restrict p_in,
     dtype_ofm OFM_DM_BANK * __restrict p_out,
     hardsigmoid_internal_params_t  &params
  ) {

      if constexpr(std::is_same<bfloat16, dtype_ifm>::value){
            vector<dtype_ifm, 16> C0_vec = broadcast<bfloat16, 16>(C0_hardsigmoid);
            accum<accfloat, 16> C0_acc;
            C0_acc.from_vector(C0_vec);

            accum<accfloat, 16> mac0;
            vector<dtype_ifm, 16> vec_in;

            // HardSigmoid function
            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(8,) 
            AIE_LOOP_UNROLL(2) 
            for (int i = 0; i < params.num_iter; i = i + 1){
	    // data input
            vec_in = load_v<16>(p_in);
            p_in = byte_incr(p_in, 32);

            //using hardsigmoid for approximation
            vec_in = min(vec_in, upper_bound_hardsigmoid);
            vec_in = max(vec_in, lower_bound_hardsigmoid);
            mac0 = mac(C0_acc, vec_in, C1_hardsigmoid);

            // result output
            store_v((dtype_ofm *)p_out, mac0.to_vector<dtype_ofm>());
            p_out = byte_incr(p_out, 32);
            }
        } else{
            //4, -4, 1/4 and 1/2 are scale adjusted and broadcasted into respective vectors
            vector<int16_t, 32> plus_three_vec_scaled  = broadcast<int16_t, 32>((int16_t)(3<<params.input_scale));
            vector<int16_t, 32> minus_three_vec_scaled = broadcast<int16_t, 32>((int16_t)((-3)<<params.input_scale));
            vector<int16_t, 32> div_factor             = broadcast<int16_t, 32>((int16_t)(params.div_factor));
            vector<int16_t, 32> constant               = broadcast<int16_t, 32>(params.constant_factor); // broadcast div_factor * 3

            accum<acc32,32> acc_0, mac0;
            acc_0.from_vector(constant,params.input_scale); //Initialize the accum with scaled 1/2.

            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(8,) 
            AIE_LOOP_UNROLL(2) 
            for (int j=0; j<params.num_iter ; j++){
             vector<int8_t, 32> vec_in;
             vector<int16_t,32> vec_in_int16;
             vec_in          = load_v<32>(p_in); p_in = byte_incr(p_in,32);
             vec_in_int16    = unpack(vec_in); //Change input to int16 type. Only type change, no up-shift.
             vec_in_int16    = min(vec_in_int16, plus_three_vec_scaled); //Compare with 3.0 and clip input greater than it.
             vec_in_int16    = max(vec_in_int16, minus_three_vec_scaled); //Compare with -3.0 and clip input less than it.
             mac0            = mac(acc_0,vec_in_int16, div_factor);
             store_v(p_out, mac0.to_vector<dtype_ofm>(params.shift_out)); p_out = byte_incr(p_out,32);
            }
        }
    }


template <typename dtype_ifm, typename dtype_ofm, typename config_ifm, typename config_ofm>
NOINLINE void hardsigmoid
  (
     adf::input_buffer_conf<dtype_ifm, config_ifm> &__restrict in,
     adf::output_buffer_conf<dtype_ofm, config_ofm> &__restrict out,
     hardsigmoid_internal_params_t  &params
  ) {
    dtype_ifm IFM_DM_BANK * __restrict p_in = (dtype_ifm IFM_DM_BANK * __restrict)in.data();
    dtype_ofm OFM_DM_BANK * __restrict p_out = (dtype_ofm OFM_DM_BANK * __restrict)out.data();
    return hardsigmoid<dtype_ifm, dtype_ofm>(p_in, p_out, params);
}

#endif //__HARDSIGMOID_H__
