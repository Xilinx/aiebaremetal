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

#ifndef __TANH_H__
#define __TANH_H__

#include "aie.hpp"
#include "tanh_params.h"
#include "tanh_impl.h"
#include "utils.h"
#include <adf.h>

using namespace aie;

// GELU const
const bfloat16 scale_factor = 2.0;
const bfloat16 c0 = 0.5;
const bfloat16 c1 = 0.25;
const bfloat16 c2 = 0.03125;
const bfloat16 sigmoid_lower_bound = -4.0;
const bfloat16 sigmoid_upper_bound = 4.0;
const bfloat16 offset_factor = 1.0;


// =========================================================================
// 1. Templated sigmoid
// flexml interface
template <typename dtype>
using tanh1d =elementwise_unary<dtype, elementwise_tanh<dtype>, tanh_templated_params_t<dtype>>;

template <typename dtype, typename config_ifm, typename config_ofm>
__attribute__((noinline)) void tanh_templated(
    adf::input_buffer_conf<dtype, config_ifm> &__restrict ifm,
    adf::output_buffer_conf<dtype, config_ofm> &__restrict ofm,
    typename tanh1d<dtype>::params_t &params) {
  tanh1d<dtype>::run(ifm.data(), ofm.data(), params);
}


// =========================================================================
// 2. Following codes are original codes without templated
// Input is ifm.
// Output is in ofm.
// All computation is performed in Bfloat16
// Kernel parameters are passed by structure params

template <typename dtype_ifm, typename dtype_ofm>
    requires(mllib::utils::is_one_of_v<dtype_ifm, int8_t, bfloat16>)
NOINLINE void tanh
  (
     dtype_ifm IFM_DM_BANK * __restrict p_in,
     dtype_ofm OFM_DM_BANK * __restrict p_out,
     tanh_params_t  &params
  ) {

          if constexpr(std::is_same<bfloat16, dtype_ifm>::value){

	      // preparing vector & accum
	      aie::vector<dtype_ifm, VEC_LEN> vec_in, mul1_vec, mul1_vec_abs, P1_vec, Q0_vec, offset_vec;
	      aie::accum<accfloat, VEC_LEN> P1_acc, mac0_acc, mul1_acc, mul2_acc, res_acc;
	      aie::accum<accfloat, VEC_LEN> Q0_acc, mul0, mac0, msc0;

              offset_vec = aie::broadcast<bfloat16, VEC_LEN>(offset_factor);

	      P1_vec = aie::broadcast<bfloat16, VEC_LEN>(scale_factor);
	      P1_acc.from_vector(P1_vec);

	      Q0_vec = aie::broadcast<bfloat16, VEC_LEN>(c0);
	      Q0_acc.from_vector(Q0_vec);

	      // Tanh func
	      AIE_PREPARE_FOR_PIPELINING 
	      AIE_LOOP_RANGE(4, ) 
	      for (int i = 0; i < params.num_iter; i = i + 1){

	          // data input
	          vec_in = aie::load_v<VEC_LEN>(p_in);
                  p_in = byte_incr(p_in, 64);

	          // part-1: in * 2.0
	          mul1_acc = aie::mul(P1_vec, vec_in);
	          mul1_vec = mul1_acc.template to_vector<dtype_ifm>();

	          // part-2: using sigmoid for approximation
	          mul1_vec = aie::min(mul1_vec, sigmoid_upper_bound);
	          mul1_vec = aie::max(mul1_vec, sigmoid_lower_bound);
	          mul1_vec_abs = aie::abs(mul1_vec);
	          mul0 = aie::mul(c2, mul1_vec_abs);
	          mac0 = aie::mac(Q0_acc, mul1_vec, c1);
	          msc0 = aie::msc(mac0, mul0.template to_vector<dtype_ifm>(), mul1_vec);

	          //part-3: 2 * sigmoid_results - 1
	          mul2_acc = aie::mul(msc0.template to_vector<dtype_ifm>(), P1_vec);
                  res_acc = aie::sub(mul2_acc, offset_vec);

	          // result output
	          aie::store_v((bfloat16 *)p_out, res_acc.to_vector<dtype_ofm>());
                  p_out = byte_incr(p_out, 64);
                  }
         } else{

             aie::vector<int16_t, 32> plus_two_vec_scaled = aie::broadcast<int16_t, 32>((int16_t) (2 << params.input_scale));
             aie::vector<int16_t, 32> minus_two_vec_scaled = aie::broadcast<int16_t, 32>((int16_t) (-2 << params.input_scale));

             aie::vector<int8_t, 32> vec_in;
             aie::vector<int16_t, 32> vec_in_int16, vec_in_abs_int16;

             accum<acc32, 32> acc0, mul0, sub0;

	     // Tanh func
	     AIE_PREPARE_FOR_PIPELINING 
	     AIE_LOOP_RANGE(4, ) 
	     for (int i = 0; i < params.num_iter; i = i + 1){

                 vec_in = load_v<32>(p_in); p_in = byte_incr(p_in, 32);
                 vec_in_int16 = unpack(vec_in);

                 vec_in_int16 = min(vec_in_int16, plus_two_vec_scaled);
                 vec_in_int16 = max(vec_in_int16, minus_two_vec_scaled);
                 vec_in_abs_int16 = aie::abs(vec_in_int16);

                 acc0.from_vector(vec_in_int16, params.up_shift);
                 mul0 = aie::mul(vec_in_int16, vec_in_abs_int16);
                 sub0 = aie::sub(acc0, mul0);

                 aie::store_v(p_out, sub0.to_vector<dtype_ofm>(params.shift_out)); p_out = byte_incr(p_out, 32);
                 }
               }
        }

template <typename dtype_ifm, typename dtype_ofm, typename config_ifm, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype_ifm, int8_t, bfloat16>)
INLINE void tanh
  (
     adf::input_buffer_conf<dtype_ifm, config_ifm> &__restrict in,
     adf::output_buffer_conf<dtype_ofm, config_ofm> &__restrict out,
     tanh_params_t  &params
  ) {
    dtype_ifm IFM_DM_BANK * __restrict p_in = (dtype_ifm IFM_DM_BANK * __restrict)in.data();
    dtype_ofm OFM_DM_BANK * __restrict p_out = (dtype_ofm OFM_DM_BANK * __restrict)out.data();
    return tanh<dtype_ifm, dtype_ofm>(p_in, p_out, params);
}

#endif //__TANH_H__
