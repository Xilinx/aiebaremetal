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
#include "ceil_params.h"

template <typename dtype> struct ceil_config {};
// use "#if __AIE_ARCH__ == 20, 21, 22" to fine tune your config
template <> struct ceil_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 8;
  static constexpr uint16_t vec_size = 32;
};
template <> struct ceil_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 8;
  static constexpr uint16_t vec_size = 32;
};

using namespace aie;

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_ceil : elementwise_unary_base<dtype, ceil_params_t<dtype>> {
public:
  static constexpr bool peel = ceil_config<dtype>::peel;
  static constexpr uint8_t unroll = ceil_config<dtype>::unroll;
  static constexpr uint16_t minimum_num_iter = ceil_config<dtype>::minimum_num_iter;
  static constexpr uint16_t vec_size = ceil_config<dtype>::vec_size;
  static constexpr uint16_t byte_size = sizeof(dtype);
  using vec_t = aie::vector<dtype, vec_size>;
  using params_t = elementwise_unary_params_t<ceil_params_t<dtype>>;

public:
  elementwise_ceil(const params_t &param) {} // setup auxiliary member variable if needed

  inline __attribute__((always_inline))
  void body(const vec_t &in_vec, vec_t &out_vec, params_t &param) {
    if constexpr (std::is_same_v<dtype, bfloat16>) {
      auto sign = bit_and((uint16_t)0x8000, in_vec.template cast_to<uint16_t>());
      auto neg_in_vec = bit_xor((uint16_t)0x8000, in_vec.template cast_to<uint16_t>()).template cast_to<bfloat16>();
      aie::vector<int16_t, vec_size> i16_vec = aie::to_fixed<int16_t>(neg_in_vec, 0);
      aie::vector<bfloat16, vec_size> tmp_vec = aie::to_float<bfloat16>(i16_vec);
      aie::vector<uint16_t, vec_size> exp_vec = aie::bit_and(mask_exp, neg_in_vec.template cast_to<uint16_t>());
      auto  mask_ints = aie::ge(exp_vec, exp_lower);
      auto floor_vec = aie::select(tmp_vec, neg_in_vec, mask_ints);
      // out_vec = bit_xor((uint16_t)0x8000, floor_vec.template cast_to<uint16_t>()).template cast_to<bfloat16>();
      out_vec = bit_or(sign, bit_and((uint16_t)0x7FFF, floor_vec.template cast_to<uint16_t>())).template cast_to<bfloat16>();
    } else {
      aie::vector<int16_t, vec_size> ifm_int16_vec = in_vec.unpack();
      ifm_int16_vec = aie::add(ifm_int16_vec, param.op_params.compensation_val);
      ifm_int16_vec = aie::bit_and(param.op_params.mask, ifm_int16_vec);
      out_vec = ifm_int16_vec.pack();
    }

    return;
  }

  inline __attribute__((always_inline)) static void setup(params_t &param, const void *rtps) {
    int16_t ifm_shift = param.ifm_shift < (int16_t)0 ? (int16_t)0 : param.ifm_shift;
    param.op_params.mask = (int16_t)((uint16_t)0xffff << ifm_shift);
    param.op_params.compensation_val = (int16_t)((uint16_t)0xffff >> (16 - ifm_shift));
  }

private:
  static constexpr uint16_t mask_exp = 0x7F80;
  static constexpr uint16_t exp_lower = 134 << 7;
};
