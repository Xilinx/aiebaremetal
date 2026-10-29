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

#pragma once

#include "adf.h"
#include "aie.hpp"
#include "aie_api/aie.hpp"

#include "elementwise_unary.h"
#include "shrink_params.h"
#include "aie_api/utils.hpp" // needed for aie:print

// User configuration for design optimization
template <typename dtype> struct shrink_config {};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct shrink_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 2;
  static constexpr uint16_t vec_size = 64;
};
template <> struct shrink_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 2;
  static constexpr uint16_t vec_size = 32;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct shrink_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 2;
  static constexpr uint16_t vec_size = 64;
};
template <> struct shrink_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 2;
  static constexpr uint16_t vec_size = 32;
};
#endif

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_shrink{};

// implementation of shrink<bfloat16>
template <>
class elementwise_shrink<bfloat16> : elementwise_unary_base<bfloat16, shrink_params_t<bfloat16>> {
  public:
    static constexpr bool peel = shrink_config<bfloat16>::peel;
    static constexpr uint8_t unroll = shrink_config<bfloat16>::unroll;
    static constexpr uint16_t minimum_num_iter = shrink_config<bfloat16>::minimum_num_iter;
    static constexpr uint16_t vec_size = shrink_config<bfloat16>::vec_size;

    using vec_t = aie::vector<bfloat16, vec_size>;
    using params_t = elementwise_unary_params_t<shrink_params_t<bfloat16>>;

  public:
    elementwise_shrink(const params_t &op_params) {}

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {
      op_params.op_params.bias = ((bfloat16 *)((int8_t*)rtps + 32))[0];
      op_params.op_params.lambd = ((bfloat16 *)((int8_t*)rtps + 34))[0];
    }

    inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
      aie::vector<bfloat16, vec_size> in_abs = aie::abs(in_vec);
      aie::vector<bfloat16, vec_size> tmp = aie::sub(in_abs, op_params.op_params.bias);
      // sign(in)*tmp
      auto sign_in = aie::bit_and(mask_sig, in_vec.template cast_to<uint16_t>());
      out_vec = aie::bit_or(sign_in, tmp.template cast_to<uint16_t>()).template cast_to<bfloat16>();
      // |x| - lambd > 0 ?
      auto mask_lambd = aie::gt(in_abs, op_params.op_params.lambd);
      out_vec = aie::select((bfloat16)0.0, out_vec, mask_lambd);
    }
  private:
    static constexpr uint16_t mask_sig = 0x8000;
};

// implementation of shrink<int8_t>
template <>
class elementwise_shrink<int8_t> : elementwise_unary_base<int8_t, shrink_params_t<int8_t>> {
  public:
    static constexpr bool peel = shrink_config<int8_t>::peel;
    static constexpr uint8_t unroll = shrink_config<int8_t>::unroll;
    static constexpr uint16_t minimum_num_iter = shrink_config<int8_t>::minimum_num_iter;
    static constexpr uint16_t vec_size = shrink_config<int8_t>::vec_size;

    using vec_t = aie::vector<int8_t, vec_size>;
    using acc_t = aie::accum<acc32, vec_size>;
    using params_t = elementwise_unary_params_t<shrink_params_t<int8_t>>;
  
    elementwise_shrink(const params_t &op_params) {}

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {
      op_params.op_params.ifm_shift_bias = ((int8_t*)rtps + 5)[0];
      op_params.op_params.ofm_shift_bias = ((int8_t*)rtps + 7)[0];
      op_params.op_params.bias = ((int16_t *)((int8_t*)rtps + 32))[0];
      op_params.op_params.lambd = (((int8_t*)rtps + 34))[0];
      op_params.op_params.lambd_neg = (((int8_t*)rtps + 35))[0];
    }

    inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
      acc_t acc0 = aie::from_vector<acc32>(in_vec, op_params.op_params.ifm_shift_bias);
      acc_t acc1 = aie::from_vector<acc32>(in_vec, op_params.op_params.ifm_shift_bias);
      acc0 = aie::add(acc0, op_params.op_params.bias);
      acc1 = aie::sub(acc1, op_params.op_params.bias);
      aie::vector<int8, vec_size> out0 = acc0.template to_vector<int8_t>(op_params.op_params.ofm_shift_bias);
      aie::vector<int8, vec_size> out1 = acc1.template to_vector<int8_t>(op_params.op_params.ofm_shift_bias);
      auto mask0 = aie::gt(in_vec, op_params.op_params.lambd);
      auto mask1 = aie::lt(in_vec, op_params.op_params.lambd_neg);
      auto tmp1 = aie::select((int8)0, out1, mask0);
      out_vec = aie::select(tmp1, out0, mask1);
    }
};
