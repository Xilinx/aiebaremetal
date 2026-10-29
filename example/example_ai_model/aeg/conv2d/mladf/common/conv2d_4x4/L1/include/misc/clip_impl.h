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
#include "clip_params.h"

template <typename dtype> struct clip_config {};
// use "#if __AIE_ARCH__ == 20, 21, 22" to fine tune your config
template <> struct clip_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 64;
};
#if __AIE_ARCH__ == 20
template <> struct clip_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 32;
};
#else
template <> struct clip_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 32;
};
#endif
template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_clip {};

template <> class elementwise_clip<bfloat16> : elementwise_unary_base<bfloat16, clip_params_t<bfloat16>> {
public:
  static constexpr bool peel = clip_config<bfloat16>::peel;
  static constexpr uint8_t unroll = clip_config<bfloat16>::unroll;
  static constexpr uint16_t minimum_num_iter = clip_config<bfloat16>::minimum_num_iter;
  static constexpr uint16_t vec_size = clip_config<bfloat16>::vec_size;
  static constexpr uint16_t byte_size = sizeof(bfloat16);
  using vec_t = aie::vector<bfloat16, vec_size>;
  using params_t = elementwise_unary_params_t<clip_params_t<bfloat16>>;

public:
  // setup auxiliary member variable
  elementwise_clip(const params_t &params) {
    clamp_min = params.op_params.clamp_min;
    clamp_max = params.op_params.clamp_max;
  }
  inline __attribute__((always_inline))
  void body(const vec_t &in_vec, vec_t &out_vec, params_t &params) {
    out_vec = aie::min(aie::max(in_vec, clamp_min), clamp_max);
  }
  inline __attribute__((always_inline))
  static void setup(params_t &params, const void *rtps) {
    const uint32_t *rtp_uint32 = (const uint32_t *)(rtps);
    params.op_params.clamp_min = *(bfloat16*)&rtp_uint32[8];
    params.op_params.clamp_max = *(bfloat16*)&rtp_uint32[9];
  }

private:
  bfloat16 clamp_min;
  bfloat16 clamp_max;
};


template <> class elementwise_clip<int8_t> : elementwise_unary_base<int8_t, clip_params_t<int8_t>> {
public:
  static constexpr bool peel = clip_config<int8_t>::peel;
  static constexpr uint8_t unroll = clip_config<int8_t>::unroll;
  static constexpr uint16_t minimum_num_iter = clip_config<int8_t>::minimum_num_iter;
  static constexpr uint16_t vec_size = clip_config<int8_t>::vec_size;
  static constexpr uint16_t byte_size = sizeof(int8_t);
  using vec_t = aie::vector<int8_t, vec_size>;
  using acc_t = aie::accum<acc32, vec_size>;
  using params_t = elementwise_unary_params_t<clip_params_t<int8_t>>;

public:
#if __AIE_ARCH__ == 20 // // INT8 VERSION AIE_ARCH SELECTION
  // setup auxiliary member variable
  elementwise_clip(params_t &params) {
    clamp_min = params.op_params.clamp_min;
    clamp_max = params.op_params.clamp_max;

    aie::tile::current().set_saturation(aie::saturation_mode::saturate);
    params.op_params.shift = params.ifm_shift - params.ofm_shift;
    if (params.op_params.shift >= -1) {
        params.op_params.shift_0 = params.op_params.shift;
        params.op_params.scale = 1;
    } else {
        params.op_params.shift_0 = 0;
        params.op_params.scale = 1 << (-params.op_params.shift);
    }
  }
  inline __attribute__((always_inline))
  void body(const vec_t &in_vec, vec_t &out_vec, params_t &params) {
    out_vec = aie::min(aie::max(in_vec, clamp_min), clamp_max);
    acc_t acc_0;
    acc_0.from_vector(out_vec);
    out_vec = acc_0.template to_vector<int8_t>(params.op_params.shift_0);
    acc_t tmp_scale = aie::mul(out_vec, params.op_params.scale);
    out_vec = tmp_scale.template to_vector<int8_t>();

  }
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // INT8 VERSION AIE_ARCH SELECTION
  // setup auxiliary member variable
  elementwise_clip(const params_t &params) {
    clamp_min = params.op_params.clamp_min;
    clamp_max = params.op_params.clamp_max;
  }
  inline __attribute__((always_inline))
  void body(const vec_t &in_vec, vec_t &out_vec, params_t &params) {
    out_vec = aie::min(aie::max(in_vec, clamp_min), clamp_max);
    acc_t acc_0;
    acc_0.from_vector(out_vec);
    out_vec = acc_0.template to_vector<int8_t>(params.ifm_shift-params.ofm_shift);
  }  
#endif // INT8 VERSION AIE_ARCH SELECTION  

  inline __attribute__((always_inline))
  static void setup(params_t &params, const void *rtps) {
    const uint32_t *rtp_uint32 = (const uint32_t *)(rtps);
    params.ifm_shift           = (int8_t)(((uint8_t*)rtps)[4]);
    params.ofm_shift           = (int8_t)(((uint8_t*)rtps)[6]);
    params.op_params.clamp_min = *(int8_t*)&rtp_uint32[8];
    params.op_params.clamp_max = *(int8_t*)&rtp_uint32[9];
  }

private:
  int8_t clamp_min;
  int8_t clamp_max;
};
