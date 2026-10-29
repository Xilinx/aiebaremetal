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
#include "reciprocal_params.h"
#include "reciprocal_lut.h"

// User configuration for design optimization
template <typename dtype> struct reciprocal_config {};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct reciprocal_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 64;
};
template <> struct reciprocal_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 4;
  static constexpr uint16_t minimum_num_iter = 1;
  static constexpr uint16_t vec_size = 16;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct reciprocal_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 64;
};
template <> struct reciprocal_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 4;
  static constexpr uint16_t minimum_num_iter = 1;
  static constexpr uint16_t vec_size = 32;
};
#endif

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_reciprocal{};

// implementation of reciprocal<bfloat16>
template <>
class elementwise_reciprocal<bfloat16> : elementwise_unary_base<bfloat16, reciprocal_params_t<bfloat16>> {
  public:
    static constexpr bool peel = reciprocal_config<bfloat16>::peel;
    static constexpr uint8_t unroll = reciprocal_config<bfloat16>::unroll;
    static constexpr uint16_t minimum_num_iter = reciprocal_config<bfloat16>::minimum_num_iter;
    static constexpr uint16_t vec_size = reciprocal_config<bfloat16>::vec_size;

    using vec_t = aie::vector<bfloat16, vec_size>;
    using params_t = elementwise_unary_params_t<reciprocal_params_t<bfloat16>>;

  public:
    elementwise_reciprocal(const params_t &op_params) {}

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {}

    inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
      out_vec = aie::inv(in_vec);
    }
};

// implementation of reciprocal<int8_t>
template <>
class elementwise_reciprocal<int8_t> : elementwise_unary_base<int8_t, reciprocal_params_t<int8_t>> {
  public:
    static constexpr bool peel = reciprocal_config<int8_t>::peel;
    static constexpr uint8_t unroll = reciprocal_config<int8_t>::unroll;
    static constexpr uint16_t minimum_num_iter = reciprocal_config<int8_t>::minimum_num_iter;
    static constexpr uint16_t vec_size = reciprocal_config<int8_t>::vec_size;

    using vec_t = aie::vector<int8_t, vec_size>;
    using params_t = elementwise_unary_params_t<reciprocal_params_t<int8_t>>;
    using lut_t = aie::lut<4, uint16_t, uint16_t>;
    using parallel_lut_t = aie::parallel_lookup<uint16, lut_t, aie::lut_oor_policy::truncate>;

  public:
    elementwise_reciprocal(const params_t &op_params) {}

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {}

    inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
      //aie::downshift() do carry, so mask it off
      aie::vector<int8_t, vec_size> in_vec_for_shft = aie::bit_and((int8_t)0xFE, in_vec);
      aie::vector<int8_t, vec_size> index_lut = aie::downshift(in_vec_for_shft, 1);
      aie::vector<int8_t, vec_size> mask_pre =aie::bit_and((int8_t)0x1, in_vec);
      aie::mask<vec_size> mask_lut = aie::le(mask_pre, (int8_t)0);

      //unroll=2 manually
      aie::vector<int8_t, vec_size/2> index_lut_0 = index_lut.extract<vec_size/2>(0);
      aie::vector<uint16_t, vec_size/2> res_lut_0 = lookup_inv.fetch(index_lut_0);
      aie::vector<int8_t, vec_size/2> index_lut_1 = index_lut.extract<vec_size/2>(1);
      aie::vector<uint16_t, vec_size/2> res_lut_1 = lookup_inv.fetch(index_lut_1);
      aie::vector<uint16_t, vec_size> res_lut = aie::concat(res_lut_0, res_lut_1);

      //aie::vector<uint16_t, vec_size> res_lut = lookup_inv.fetch(index_lut);
      aie::vector<int8_t, vec_size> res_lut_even = aie::filter_even(res_lut.cast_to<int8_t>());
      aie::vector<int8_t, vec_size> res_lut_odd  = aie::filter_odd(res_lut.cast_to<int8_t>());
      out_vec = aie::select(res_lut_even, res_lut_odd, mask_lut);
    }
private:
    lut_t lut_inv = lut_t(128, inv_lut_int8_ab, inv_lut_int8_cd);
    parallel_lut_t lookup_inv = parallel_lut_t(lut_inv);
};
