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
#include "hardsigmoid_params.h"

// User configuration for design optimization
template <typename dtype>
struct hardsigmoid_config {};

#if __AIE_ARCH__ == 20  // AIE2
template <>
struct hardsigmoid_config<int8_t> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 32;
};
template <>
struct hardsigmoid_config<bfloat16> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 16;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22  // AIE2p and AIE2ps
template <>
struct hardsigmoid_config<int8_t> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 64;
};
template <>
struct hardsigmoid_config<bfloat16> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 32;
};
#endif                                          // AIE_ARCH

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_hardsigmoid {};

// implementation of hardsigmoid<bfloat16>
template <>
class elementwise_hardsigmoid<bfloat16>
    : elementwise_unary_base<bfloat16, hardsigmoid_templated_params_t<bfloat16>> {
    using acc_t = aie::accum<accfloat, vec_size>;
    using vec_t = aie::vector<bfloat16, vec_size>;
    using params_t = elementwise_unary_params_t<hardsigmoid_templated_params_t<bfloat16>>;
  private:
#ifndef __X86SIM__
    static constexpr bfloat16 lower_bound_hardsigmoid = FLOAT_TO_BFLOAT16(float(-3.0));
    static constexpr bfloat16 upper_bound_hardsigmoid = FLOAT_TO_BFLOAT16(float(3.0));
#else
    bfloat16 lower_bound_hardsigmoid = -3.0;
    bfloat16 upper_bound_hardsigmoid = 3.0;
#endif

    vec_t C0_vec;
    acc_t C0_acc;

  public:
    static constexpr bool peel = hardsigmoid_config<bfloat16>::peel;
    static constexpr uint8_t unroll = hardsigmoid_config<bfloat16>::unroll;
    static constexpr uint16_t minimum_num_iter = hardsigmoid_config<bfloat16>::minimum_num_iter;
    static constexpr uint16_t vec_size = hardsigmoid_config<bfloat16>::vec_size;

    elementwise_hardsigmoid(const params_t &op_params) {
        C0_vec = broadcast<bfloat16, vec_size>(op_params.op_params.beta);
        C0_acc.from_vector(C0_vec);
    }

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {
        op_params.op_params.alpha = ((bfloat16 *)((int8_t*)rtps + 32))[0];
        op_params.op_params.beta = ((bfloat16 *)((int8_t*)rtps + 34))[0];
    }

    inline __attribute__((always_inline)) void body(const vec_t &in, vec_t &out,
                                                    params_t &op_params) {
        vec_t in_tmp = min(in, upper_bound_hardsigmoid);
        in_tmp = max(in_tmp, lower_bound_hardsigmoid);
        acc_t mac0 = mac(C0_acc, in_tmp, op_params.op_params.alpha);
        out = mac0.to_vector<bfloat16>();
    }
};

// implementation of hardsigmoid<int8_t>
template <>
class elementwise_hardsigmoid<int8_t>
    : elementwise_unary_base<int8_t, hardsigmoid_templated_params_t<int8_t>> {
    using acc_t = aie::accum<acc32, vec_size>;
    using vec_t = aie::vector<int8_t, vec_size>;
    using params_t = elementwise_unary_params_t<hardsigmoid_templated_params_t<int8_t>>;
   private:
    vector<int16_t, vec_size> plus_three_vec_scaled;
    vector<int16_t, vec_size> minus_three_vec_scaled;
    vector<uint16_t, vec_size> alpha_vec, beta_vec;
    accum<acc32, vec_size> acc0;

   public:
    static constexpr bool peel = hardsigmoid_config<int8_t>::peel;
    static constexpr uint8_t unroll = hardsigmoid_config<int8_t>::unroll;
    static constexpr uint16_t minimum_num_iter = hardsigmoid_config<int8_t>::minimum_num_iter;
    static constexpr uint16_t vec_size = hardsigmoid_config<int8_t>::vec_size;

    elementwise_hardsigmoid(const params_t &op_params) {
        // 4, -4, 1/4 and 1/2 are scale adjusted and broadcasted into respective vectors
        plus_three_vec_scaled =
            broadcast<int16_t, vec_size>((int16_t)(3 << op_params.ifm_shift));
        minus_three_vec_scaled =
            broadcast<int16_t, vec_size>((int16_t)((-3) << op_params.ifm_shift));
        alpha_vec = broadcast<uint16_t, vec_size>(op_params.op_params.alpha);
        beta_vec = broadcast<uint16_t, vec_size>(op_params.op_params.beta);
        acc0.from_vector(beta_vec);
    }

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {
        op_params.op_params.alpha = ((uint16_t *)((int8_t*)rtps + 32))[0];
        op_params.op_params.beta = ((uint16_t *)((int8_t*)rtps + 34))[0];
    }

    inline __attribute__((always_inline)) void body(const vec_t &in, vec_t &out,
                                                    params_t &op_params) {
        vector<int16_t, vec_size> vec_in_int16;
        vec_in_int16 = unpack(in);  // Change input to int16 type. Only type change, no up-shift.
                                    // Compare with 3.0 and clip input greater than it.
        vec_in_int16 = aie::min(vec_in_int16, plus_three_vec_scaled);
        // Compare with -3.0 and clip input less than it.
        vec_in_int16 = max(vec_in_int16, minus_three_vec_scaled);
        acc_t mac0 = mac(acc0, vec_in_int16, alpha_vec);
        out = mac0.to_vector<int8_t>(13-op_params.ofm_shift);
    }
};
