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

#include "elementwise_binary.h"
#include "div_params.h"
namespace {
template <typename dtype>
struct div_config {};
template <>
struct div_config<int8_t> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 1;
    static constexpr uint16_t vec_size = 64;
};
#if __AIE_ARCH__ == 20
template <>
struct div_config<bfloat16> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 1;
    static constexpr uint16_t vec_size = 64;
};
#else
template <>
struct div_config<bfloat16> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 2;
    static constexpr uint16_t minimum_num_iter = 1;
    static constexpr uint16_t vec_size = 32;
};
#endif // __AIE_ARCH__
} // anonymous namespace


template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class div_impl{};
template <>
// In div, we do a manual unroll by loading double vec size and splitting.
// The skeleton unroll didn't provide desired compiler optimization.
class div_impl<int8_t> : public elementwise_binary_base<int8_t, div_params_t<int8_t>> {
   public:
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 1;
    static constexpr uint16_t vec_size = 64;
    using vec_t = aie::vector<int8_t, vec_size>;
    using params_t = elementwise_binary_params_t<div_params_t<int8_t>>;

   public:
    div_impl(const params_t& op_params) {} // setup auxiliary member variable if needed
    inline __attribute__((always_inline)) void body(const vec_t& in_vec1,
                                                    const vec_t& in_vec2,
                                                    vec_t& out_vec,
                                                    params_t& op_params) {
      // a plain integer div based on bf16 div. As stated in https://www.mlplatform.org/tosa/tosa_spec.html#_intdiv
      // quantized div should be implemented as reciprocal + mul
      aie::vector<int8, vec_size/2> in_vec1_0 = in_vec1.template extract<vec_size/2>(0);
      aie::vector<int8, vec_size/2> in_vec1_1 = in_vec1.template extract<vec_size/2>(1);
      aie::vector<int8, vec_size/2> in_vec2_0 = in_vec2.template extract<vec_size/2>(0);
      aie::vector<int8, vec_size/2> in_vec2_1 = in_vec2.template extract<vec_size/2>(1);

      aie::vector<bfloat16, vec_size/2> in_vec1_bf_0 = int8_to_bfloat16(in_vec1_0);
      aie::vector<bfloat16, vec_size/2> in_vec1_bf_1 = int8_to_bfloat16(in_vec1_1);
      aie::vector<bfloat16, vec_size/2> in_vec2_bf_0 = int8_to_bfloat16(in_vec2_0);
      aie::vector<bfloat16, vec_size/2> in_vec2_bf_1 = int8_to_bfloat16(in_vec2_1);

      aie::vector<bfloat16, vec_size/2> out_vec_bf_0 = aie::div(in_vec1_bf_0, in_vec2_bf_0).template to_vector<bfloat16>();
      aie::vector<bfloat16, vec_size/2> out_vec_bf_1 = aie::div(in_vec1_bf_1, in_vec2_bf_1).template to_vector<bfloat16>();

      out_vec = aie::concat(aie::to_fixed<int8>(out_vec_bf_0), aie::to_fixed<int8>(out_vec_bf_1));
    }
    inline __attribute__((always_inline)) static void setup(params_t& op_params, const void* rtps) {}

   private:
    aie::vector<bfloat16, vec_size/2> __attribute__((always_inline))
      int8_to_bfloat16(const aie::vector<int8, vec_size/2>& x) {
      aie::accum<accfloat, vec_size/2> acc_x = from_vector<accfloat>(aie::to_float(x));
    return acc_x.template to_vector<bfloat16>();
  }
};

template <>
class div_impl<bfloat16> : public elementwise_binary_base<bfloat16, div_params_t<bfloat16>> {
   public:
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 1;
    static constexpr uint16_t vec_size = 64;
    using vec_t = aie::vector<bfloat16, vec_size>;
    using params_t = elementwise_binary_params_t<div_params_t<bfloat16>>;

   public:
    div_impl(const params_t& op_params) {} // setup auxiliary member variable if needed
    inline __attribute__((always_inline)) void body(const vec_t& in_vec1,
                                                    const vec_t& in_vec2,
                                                    vec_t& out_vec,
                                                    params_t& op_params) {
      aie::vector<bfloat16, vec_size/2> in_vec1_0 = in_vec1.template extract<vec_size/2>(0);
      aie::vector<bfloat16, vec_size/2> in_vec2_0 = in_vec2.template extract<vec_size/2>(0);
      aie::vector<bfloat16, vec_size/2> in_vec1_1 = in_vec1.template extract<vec_size/2>(1);
      aie::vector<bfloat16, vec_size/2> in_vec2_1 = in_vec2.template extract<vec_size/2>(1);
      aie::vector<bfloat16, vec_size/2> out_vec_bf_0 = aie::div(in_vec1_0, in_vec2_0).template to_vector<bfloat16>();
      aie::vector<bfloat16, vec_size/2> out_vec_bf_1 = aie::div(in_vec1_1, in_vec2_1).template to_vector<bfloat16>();

      out_vec = aie::concat(out_vec_bf_0, out_vec_bf_1);

    }
    inline __attribute__((always_inline)) static void setup(params_t& op_params, const void* rtps) {}
};