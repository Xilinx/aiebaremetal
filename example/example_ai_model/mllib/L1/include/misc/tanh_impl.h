/*
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
#include "tanh_params.h"

// User configuration for design optimization
template <typename dtype> struct tanh_config {};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct tanh_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 32;
};
template <> struct tanh_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 32;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct tanh_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 32;
};
template <> struct tanh_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 32;
};
#endif

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_tanh {};

template <> class elementwise_tanh<bfloat16> : elementwise_unary_base<bfloat16, tanh_templated_params_t<bfloat16>> {

public:
  static constexpr bool peel = tanh_config<bfloat16>::peel;
  static constexpr uint8_t unroll = tanh_config<bfloat16>::unroll;
  static constexpr uint16_t minimum_num_iter = tanh_config<bfloat16>::minimum_num_iter;
  static constexpr uint16_t vec_size = tanh_config<bfloat16>::vec_size;

  using acc_t = aie::accum<accfloat, vec_size>;
  using msk_t = aie::mask<vec_size>;
  using vec_t = aie::vector<bfloat16, vec_size>;
  using params_t = elementwise_unary_params_t<tanh_templated_params_t<bfloat16>>;

  elementwise_tanh(const params_t &op_params) {
  }

  // setup auxiliary member variable if needed
  __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {
  }

  inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
    static const bfloat16 seg_bnd0 = 0.55078125;
    // static const bfloat16 seg_bnd1 = 2.95854917;
    // static const bfloat16 seg_bnd1 = 2.5625;
    static const bfloat16 seg_bnd1 = 2.84375;
    static const bfloat16 ca0 = 0;
    static const bfloat16 ca1 = 1;
    static const bfloat16 ca2 = -2.3314684e-15;
    static const bfloat16 ca3 = -2.9882812e-01;
    static const bfloat16 ca4 = 7.0499162e-15;
    static const bfloat16 cb0 = -0.06298828;
    static const bfloat16 cb1 = 1.3515625;
    static const bfloat16 cb2 = -0.6640625;
    static const bfloat16 cb3 = 0.14648438;
    static const bfloat16 cb4 = -0.01196289;

    msk_t neg_m = aie::lt(in_vec, (bfloat16)0.0);
    vec_t sig_v = aie::select((bfloat16)1.0, (bfloat16)-1.0, neg_m);
    vec_t abs_v = aie::abs(in_vec);
    vec_t x1    = aie::min(abs_v, seg_bnd1);
    msk_t gt_bnd0_m = aie::gt(abs_v, seg_bnd0);
    vec_t c0 = aie::select(ca0, cb0, gt_bnd0_m);
    vec_t c1 = aie::select(ca1, cb1, gt_bnd0_m);
    vec_t c2 = aie::select(ca2, cb2, gt_bnd0_m);
    vec_t c3 = aie::select(ca3, cb3, gt_bnd0_m);
    vec_t c4 = aie::select(ca4, cb4, gt_bnd0_m);

    acc_t x2_float = aie::mul_square(x1);
    vec_t x2 = x2_float.template to_vector<bfloat16>();
    acc_t x3_float = aie::mul(x2, x1);
    vec_t x3 = x3_float.template to_vector<bfloat16>();
    acc_t x4_float = aie::mul_square(x2);
    vec_t x4 = x4_float.template to_vector<bfloat16>();

    acc_t acc;
    acc.template from_vector<bfloat16>(c0, 0);
    acc = aie::mac(acc, x1, c1);
    acc = aie::mac(acc, x2, c2);
    acc = aie::mac(acc, x3, c3);
    acc = aie::mac(acc, x4, c4);

    out_vec = acc.template to_vector<bfloat16>();
    out_vec = aie::mul(out_vec, sig_v);
  }
};

// implementation of tanh<int8_t>/hardsigmoid<int8_t>
template <> class elementwise_tanh<int8_t> : elementwise_unary_base<int8_t, tanh_templated_params_t<int8_t>> {

public:
  static constexpr bool peel = tanh_config<int8_t>::peel;
  static constexpr uint8_t unroll = tanh_config<int8_t>::unroll;
  static constexpr uint16_t minimum_num_iter = tanh_config<int8_t>::minimum_num_iter;
  static constexpr uint16_t vec_size = tanh_config<int8_t>::vec_size;

  using acc_t = acc32;
  using vec_t = aie::vector<int8_t, vec_size>;
  using params_t = elementwise_unary_params_t<tanh_templated_params_t<int8_t>>;

  aie::vector<int16_t, 32> plus_two_vec_scaled, minus_two_vec_scaled;
  aie::vector<int8_t, 32> vec_in;
  aie::vector<int16_t, 32> vec_in_int16, vec_in_abs_int16;
  accum<acc32, 32> acc0, mul0, sub0;
  int32_t up_shift;


  elementwise_tanh(const params_t &op_params) {
        plus_two_vec_scaled  = aie::broadcast<int16_t, 32>((int16_t) ( 2 << op_params.ifm_shift));
        minus_two_vec_scaled = aie::broadcast<int16_t, 32>((int16_t) (-2 << op_params.ifm_shift));
        up_shift = op_params.ifm_shift + 2; 

  }

  // setup auxiliary member variable if needed
  __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {

  }

  inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
        vec_in_int16 = unpack(in_vec);
        vec_in_int16 = min(vec_in_int16, plus_two_vec_scaled);
        vec_in_int16 = max(vec_in_int16, minus_two_vec_scaled);
        vec_in_abs_int16 = aie::abs(vec_in_int16);

        acc0.from_vector(vec_in_int16, up_shift);
        mul0 = aie::mul(vec_in_int16, vec_in_abs_int16);
        sub0 = aie::sub(acc0, mul0);

        out_vec = sub0.to_vector<int8_t>(op_params.ofm_shift);

  }
};
