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
#include "hardswish_params.h"

// User configuration for design optimization
template <typename dtype> struct hardswish_config {};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct hardswish_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 8;
  static constexpr uint16_t vec_size = 32;
};
template <> struct hardswish_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 8;
  static constexpr uint16_t vec_size = 16;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct hardswish_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 8;
  static constexpr uint16_t vec_size = 64;
};
template <> struct hardswish_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 8;
  static constexpr uint16_t vec_size = 32;
};
#endif

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_hardswish {};

// implementation of hardswish<bfloat16>/hardsigmoid<bfloat16>
template <> class elementwise_hardswish<bfloat16> : elementwise_unary_base<bfloat16, hardswish_params_t<bfloat16>> {
private:
  static constexpr bfloat16 inv_six = FLOAT_TO_BFLOAT16(float(0.16666666667));
  static constexpr bfloat16 half = FLOAT_TO_BFLOAT16(float(0.5));
  static constexpr bfloat16 lower_bound = FLOAT_TO_BFLOAT16(float(-3.0));
  static constexpr bfloat16 upper_bound = FLOAT_TO_BFLOAT16(float(3.0));

public:
  static constexpr bool peel = hardswish_config<bfloat16>::peel;
  static constexpr uint8_t unroll = hardswish_config<bfloat16>::unroll;
  static constexpr uint16_t minimum_num_iter = hardswish_config<bfloat16>::minimum_num_iter;
  static constexpr uint16_t vec_size = hardswish_config<bfloat16>::vec_size;

  using acc_t = accfloat;
  using vec_t = aie::vector<bfloat16, vec_size>;
  using params_t = elementwise_unary_params_t<hardswish_params_t<bfloat16>>;

  aie::vector<bfloat16, vec_size> one_vec;
  aie::vector<bfloat16, vec_size> half_vec;
  aie::mask<vec_size> sigmoid_swish_mask; // true when RTP requires Hardswish, false when RTP requires Hardsigmoid

  elementwise_hardswish(const params_t &op_params) {
      one_vec = aie::broadcast<bfloat16, vec_size>((bfloat16)1.0);
      half_vec = aie::broadcast<bfloat16, vec_size>((bfloat16)0.5);
      if (op_params.op_params.is_hardsigmoid) {
        sigmoid_swish_mask = aie::eq(one_vec, (bfloat16)1.0);
      } else {
        sigmoid_swish_mask = aie::neq(one_vec, (bfloat16)1.0);
      }
  }

  // setup auxiliary member variable if needed
  __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {
      op_params.op_params.is_hardsigmoid = ((const uint8_t*) rtps)[32];
  }

  inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
    aie::vector<bfloat16, vec_size> clip_vec = aie::max(aie::min(in_vec, upper_bound), lower_bound);
    aie::vector<bfloat16, vec_size> coeff_vec = aie::select(in_vec, one_vec, sigmoid_swish_mask);
    aie::accum<accfloat, vec_size> hardsigmoid;
    hardsigmoid.template from_vector(half_vec);
    hardsigmoid = aie::mac(hardsigmoid, clip_vec, inv_six);
    aie::accum<accfloat, vec_size> res = aie::mul(coeff_vec, hardsigmoid.template to_vector<bfloat16>());
    out_vec = res.template to_vector<bfloat16>();
  }
};

// implementation of hardswish<int8_t>/hardsigmoid<int8_t>
template <> class elementwise_hardswish<int8_t> : elementwise_unary_base<int8_t, hardswish_params_t<int8_t>> {
private:
  int16_t inv_six = 1365; // 1/6 at scale 13
  int16_t lower_bound = -3;
  int16_t upper_bound = 3;
  int16_t input_shift = 0;
  int16_t coeff_shift = 0;

public:
  static constexpr bool peel = hardswish_config<int8_t>::peel;
  static constexpr uint8_t unroll = hardswish_config<int8_t>::unroll;
  static constexpr uint16_t minimum_num_iter = hardswish_config<int8_t>::minimum_num_iter;
  static constexpr uint16_t vec_size = hardswish_config<int8_t>::vec_size;

  using acc_t = acc32;
  using vec_t = aie::vector<int8_t, vec_size>;
  using params_t = elementwise_unary_params_t<hardswish_params_t<int8_t>>;

  aie::vector<int16_t, vec_size> one_vec;
  aie::vector<int16_t, vec_size> half_vec;
  aie::mask<vec_size> sigmoid_swish_mask; // true when RTP requires Hardswish, false when RTP requires Hardsigmoid

  elementwise_hardswish(const params_t &op_params) {
      // up-scale coeff if ifm_shift >= 0, otherwise up-scale input
      if (op_params.ifm_shift < 0) {
          input_shift = abs(op_params.ifm_shift);
      } else {
          coeff_shift = op_params.ifm_shift;
      }
      one_vec = aie::broadcast<int16_t, vec_size>((int16_t)(1 << coeff_shift));
      half_vec = aie::broadcast<int16_t, vec_size>(1); // at scale 1
      if (op_params.op_params.is_hardsigmoid) {
        sigmoid_swish_mask = aie::eq(one_vec, (int16_t)(1 << coeff_shift));
      } else {
        sigmoid_swish_mask = aie::neq(one_vec, (int16_t)(1 << coeff_shift));
      }
      lower_bound = (int16_t)(lower_bound << coeff_shift);
      upper_bound = (int16_t)(upper_bound << coeff_shift);
  }

  // setup auxiliary member variable if needed
  __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {
      op_params.op_params.is_hardsigmoid = ((const uint8_t*) rtps)[32];
  }

  inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
    aie::vector<int16_t, vec_size> clip_vec = aie::max(aie::min(aie::upshift(aie::unpack(in_vec), input_shift), upper_bound), lower_bound);
    aie::vector<int16_t, vec_size> coeff_vec = aie::select(aie::upshift(aie::unpack(in_vec), input_shift), one_vec, sigmoid_swish_mask);
    aie::accum<acc32, vec_size> hardsigmoid;
    hardsigmoid.template from_vector(half_vec, coeff_shift + 12);
    hardsigmoid = aie::mac(hardsigmoid, clip_vec, inv_six);
    aie::accum<acc32, vec_size> res = aie::mul(coeff_vec, hardsigmoid.template to_vector<int16_t>(coeff_shift));
    out_vec = aie::pack(res.template to_vector<int16_t>(coeff_shift + 13 - op_params.ofm_shift));
  }
};
