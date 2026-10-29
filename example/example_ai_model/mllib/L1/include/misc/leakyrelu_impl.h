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
#include "leakyrelu_params.h"

// User configuration for design optimization
template <typename dtype> struct leakyrelu_config {};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct leakyrelu_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 64;
};
template <> struct leakyrelu_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 64;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct leakyrelu_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 64;
};
template <> struct leakyrelu_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 64;
};
#endif

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_leakyrelu {};

template <> class elementwise_leakyrelu<bfloat16> : elementwise_unary_base<bfloat16, leakyrelu_params_t<bfloat16>> {

public:
  static constexpr bool peel = leakyrelu_config<bfloat16>::peel;
  static constexpr uint8_t unroll = leakyrelu_config<bfloat16>::unroll;
  static constexpr uint16_t minimum_num_iter = leakyrelu_config<bfloat16>::minimum_num_iter;
  static constexpr uint16_t vec_size = leakyrelu_config<bfloat16>::vec_size;

  using acc_t = accfloat;
  using vec_t = aie::vector<bfloat16, vec_size>;
  using params_t = elementwise_unary_params_t<leakyrelu_params_t<bfloat16>>;

  bfloat16 alpha;

  elementwise_leakyrelu(const params_t &op_params) {
        alpha = op_params.op_params.alpha;
  }

  // setup auxiliary member variable if needed
  __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {
        bfloat16 alpha_rtp = ((const bfloat16*) rtps)[16];
        op_params.op_params.alpha = alpha_rtp;
  }

  inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
        accum<accfloat,vec_size> scaled_acc_0 = mul(in_vec, alpha);
        vector<bfloat16,vec_size> scaled_vec_0 = scaled_acc_0.to_vector<bfloat16>();
        out_vec = max(in_vec, scaled_vec_0);
  }
};

// implementation of leakyrelu<int8_t>
template <> class elementwise_leakyrelu<int8_t> : elementwise_unary_base<int8_t, leakyrelu_params_t<int8_t>> {

public:
  static constexpr bool peel = leakyrelu_config<int8_t>::peel;
  static constexpr uint8_t unroll = leakyrelu_config<int8_t>::unroll;
  static constexpr uint16_t minimum_num_iter = leakyrelu_config<int8_t>::minimum_num_iter;
  static constexpr uint16_t vec_size = leakyrelu_config<int8_t>::vec_size;

  using acc_t = acc32;
  using vec_t = aie::vector<int8_t, vec_size>;
  using params_t = elementwise_unary_params_t<leakyrelu_params_t<int8_t>>;

  int16_t alpha_i16   = 0;
  uint8_t shift_alpha = 0;
  uint8_t shift_out   = 0;
  uint8_t ifm_shift_biased = 0;


  elementwise_leakyrelu(const params_t &op_params) {
    alpha_i16   = op_params.op_params.alpha_i16;
    shift_alpha = op_params.op_params.shift_alpha;
    shift_out   = shift_alpha + op_params.ofm_shift_biased;
    ifm_shift_biased = op_params.ifm_shift_biased;
  }

  // setup auxiliary member variable if needed
  __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {
      int16_t alpha_i16       = ((const int16_t*) rtps)[16];
      uint8_t shift_alpha     = ((const uint8_t*) rtps)[34];
      op_params.op_params.alpha_i16   = alpha_i16;
      op_params.op_params.shift_alpha = shift_alpha;

  }

  inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
    accum<acc32,   vec_size> temp_acc;
    vector<int16_t,vec_size> input_vec  = unpack(in_vec);
    accum<acc32,   vec_size> alpha_acc  = mul(input_vec, alpha_i16);
    vector<int16_t, vec_size> input_int16 = upshift(input_vec, shift_alpha);
    vector<int16_t, vec_size> alpha_vec  = alpha_acc.to_vector<int16_t>(0);
    vector<int16_t, vec_size> result_vec = max(input_int16, alpha_vec);
    temp_acc.from_vector(result_vec, ifm_shift_biased);
    out_vec = temp_acc.to_vector<int8>(shift_out);

  }
};
