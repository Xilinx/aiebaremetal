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
#include "aie_api/utils.hpp"

#include "elementwise_unary.h"
#include "round_params.h"

template <typename dtype> struct round_config {};
// use "#if __AIE_ARCH__ == 20, 21, 22" to fine tune your config
template <> struct round_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 8;
  static constexpr uint16_t vec_size = 32;
};
#if __AIE_ARCH__ == 20
template <> struct round_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 2;
  static constexpr uint16_t vec_size = 32;
};
#else
template <> struct round_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 2;
  static constexpr uint16_t vec_size = 32;
};
#endif

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_round : elementwise_unary_base<dtype, round_params_t<dtype>> {
public:
  static constexpr bool peel = round_config<dtype>::peel;
  static constexpr uint8_t unroll = round_config<dtype>::unroll;
  static constexpr uint16_t minimum_num_iter = round_config<dtype>::minimum_num_iter;
  static constexpr uint16_t vec_size = round_config<dtype>::vec_size;
  static constexpr uint16_t byte_size = sizeof(dtype);
  using vec_t = aie::vector<dtype, vec_size>;
  using params_t = elementwise_unary_params_t<round_params_t<dtype>>;

public:
  elementwise_round(const params_t &params) {} // setup auxiliary member variable if needed

  inline __attribute__((always_inline))
  void body(const vec_t &in_vec, vec_t &out_vec, params_t &params) {
    if constexpr (std::is_same_v<dtype, bfloat16>) {
      auto sig_vec = aie::bit_and(mask_sig, in_vec.template cast_to<uint16_t>());
      aie::accum<accfloat, vec_size> in_acc;
      in_acc.from_vector(in_vec);
      aie::vector<float, vec_size> in_float = in_acc.template to_vector<float>(0);
      aie::vector<int16_t, vec_size> i16_vec = aie::to_fixed<int16_t>(in_float, 0);
      aie::vector<bfloat16, vec_size> tmp_vec = aie::to_float<bfloat16>(i16_vec);
      aie::vector<uint16_t, vec_size> exp_vec = aie::bit_and(mask_exp, in_vec.template cast_to<uint16_t>());
      auto  mask_ints = aie::ge(exp_vec, exp_lower);
      out_vec = aie::select(tmp_vec, in_vec, mask_ints);
      out_vec = aie::bit_or(sig_vec, out_vec.template cast_to<uint16_t>()).template cast_to<bfloat16>();
    } else {
      aie::accum<acc32,vec_size> in_acc; in_acc.from_vector(in_vec);
      aie::vector<int8_t, vec_size> tmp= in_acc.template to_vector<int8_t>(params.op_params.ifm_shift);
      out_vec = aie::upshift(tmp, params.op_params.ifm_shift);
    }
    return;
  }

  inline __attribute__((always_inline)) static void setup(params_t &params, const void *rtps) {
      params.op_params.ifm_shift = params.ifm_shift < (int8_t)0 ? (int8_t)0 : params.ifm_shift;
  }

private:
  static constexpr uint16_t mask_exp = 0x7F80;
  static constexpr uint16_t mask_sig = 0x8000;
  static constexpr uint16_t exp_lower = 134 << 7;
};
