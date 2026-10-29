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

#pragma once

#include "adf.h"
#include "aie.hpp"
#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"

#include "elementwise_unary.h"
#include "silu_params.h"

// User configuration for design optimization
template <typename dtype> struct silu_config {};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct silu_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 32;
};
template <> struct silu_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 16;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct silu_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 2;
  static constexpr uint16_t vec_size = 32;
};
template <> struct silu_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 2;
  static constexpr uint16_t vec_size = 32;
};
#endif

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_silu {};

template <> class elementwise_silu<bfloat16> : elementwise_unary_base<bfloat16, silu_params_t<bfloat16>> {
// private:
//   static constexpr bfloat16 C0 = 0.5;
//   static constexpr bfloat16 C1 = 0.25;
//   static constexpr bfloat16 C2 = 0.03125;
//   static constexpr bfloat16 lower_bound = -4.0;
//   static constexpr bfloat16 upper_bound = 4.0;

public:
  static constexpr bool peel = silu_config<bfloat16>::peel;
  static constexpr uint8_t unroll = silu_config<bfloat16>::unroll;
  static constexpr uint16_t minimum_num_iter = silu_config<bfloat16>::minimum_num_iter;
  static constexpr uint16_t vec_size = silu_config<bfloat16>::vec_size;

  using acc_t = accfloat;
  using vec_t = aie::vector<bfloat16, vec_size>;
  using params_t = elementwise_unary_params_t<silu_params_t<bfloat16>>;

  aie::vector<bfloat16, vec_size> vec_in, mul1_vec, mul1_vec_abs, C0_vec;
  aie::accum<accfloat, vec_size> mul0, mac0, msc0, acc_res, C0_acc;

  elementwise_silu(const params_t &op_params) {
    C0_vec = broadcast<bfloat16, vec_size>(C0);
    C0_acc.from_vector(C0_vec);

  }

  // setup auxiliary member variable if needed
  __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {
  }

  inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
        mul1_vec = min(in_vec, upper_bound);
        mul1_vec = max(mul1_vec, lower_bound);
        mul1_vec_abs = abs(mul1_vec);
        mul0 = mul(C2, mul1_vec_abs);
        mac0 = mac(C0_acc, mul1_vec, C1);
        msc0 = msc(mac0, mul0.template to_vector<bfloat16>(), mul1_vec);
        acc_res = mul(in_vec, msc0.template to_vector<bfloat16>());
        out_vec = acc_res.to_vector<bfloat16>();
  }
};

// implementation of silu<int8_t>/hardsigmoid<int8_t>
template <> class elementwise_silu<int8_t> : elementwise_unary_base<int8_t, silu_params_t<int8_t>> {

public:
  static constexpr bool peel = silu_config<int8_t>::peel;
  static constexpr uint8_t unroll = silu_config<int8_t>::unroll;
  static constexpr uint16_t minimum_num_iter = silu_config<int8_t>::minimum_num_iter;
  static constexpr uint16_t vec_size = silu_config<int8_t>::vec_size;

  using acc_t = acc32;
  using vec_t = aie::vector<int8_t, vec_size>;
  using params_t = elementwise_unary_params_t<silu_params_t<int8_t>>;

  aie::vector<int16_t, vec_size> plus_four_vec;
  aie::vector<int16_t, vec_size> minus_four_vec;
  aie::vector<uint16_t,vec_size> vec_1by4;
  aie::vector<int32_t, vec_size> vec_1by2;

  aie::accum<acc32,vec_size> acc_0, acc_1, acc_2;
  aie::accum<acc48,vec_size> acc_res;
  aie::vector<int32_t, vec_size> vect_acc_2;

  aie::vector<int8_t, vec_size> vec_in;
  aie::vector<int16_t,vec_size> vec_in_int16_orig, vec_in_int16, vec_in_abs;

  int16_t input_shift = 0;
  int16_t coeff_shift = 0;
  int16_t lower_bound = -1;
  int16_t upper_bound =  1;
  int16_t shift_out   =  0;

  #if __AIE_ARCH__ == 20 // AIE2
  int16_t shift_init  =  0;
  int16_t shift_scale =  0;
  #endif

  elementwise_silu(const params_t &op_params) {
    int16_t num_coeffs = 0;
      if (op_params.ifm_shift < 0) {
          input_shift = abs(op_params.ifm_shift);
          num_coeffs = 2;
      } else {
          coeff_shift = op_params.ifm_shift;
          num_coeffs = 3;
      }
    int8_t scale_bits = coeff_shift + 2;
    plus_four_vec  = aie::broadcast<int16_t, vec_size>((int16_t)(1<<scale_bits));
    minus_four_vec = aie::broadcast<int16_t, vec_size>((int16_t)((-1)<<scale_bits));
    vec_1by4       = aie::broadcast<uint16_t,vec_size>((uint16_t)(2<<scale_bits));
    vec_1by2       = aie::broadcast<int32_t, vec_size>((int32_t)(1<<(2*scale_bits)));
    acc_0.from_vector(vec_1by2);
    lower_bound = (int16_t)(lower_bound << scale_bits);
    upper_bound = (int16_t)(upper_bound << scale_bits);
    shift_out = num_coeffs * coeff_shift - op_params.ofm_shift + 5;

    #if __AIE_ARCH__ == 20 // AIE2
    if(shift_out >= -1){
      shift_init = shift_out;
      shift_scale = 1;
    }else{
      aie::set_saturation(aie::saturation_mode::saturate);
      shift_init = 0;
      shift_scale = 1 << (-shift_out);
    }
    #endif

  }

  // setup auxiliary member variable if needed
  __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {

  }

  inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
        vec_in_int16_orig = unpack(in_vec); //Change input to int16 type. Only type change, no up-shift.
        vec_in_int16 = aie::upshift(vec_in_int16_orig, input_shift);
        aie::vector<int16_t, vec_size> clip_vec = aie::max(aie::min(vec_in_int16, upper_bound), lower_bound);
        vec_in_abs   = abs(clip_vec);
        acc_1        = mac(acc_0,clip_vec, vec_1by4); //(1/2) + (1/4)*x
        acc_2        = msc(acc_1,vec_in_abs,clip_vec); //(1/2)+(1/4)*x-(1/32)*x*|x| (1/32 is scaled to 1).
        vect_acc_2   = acc_2.to_vector<int32_t>(0);
        acc_res      = mul(vec_in_int16, vect_acc_2);

        #if __AIE_ARCH__ == 20 // AIE2
        auto vec_init = acc_res.template to_vector<int16_t>(shift_init);
        auto out_acc = aie::mul(shift_scale, vec_init);
        out_vec = aie::pack(out_acc.template to_vector<int16_t>(0));
        #elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
        out_vec = aie::pack(acc_res.template to_vector<int16_t>(shift_out));
        #endif

  }
};
