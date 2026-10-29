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
#include "sigmoid_params.h"

// User configuration for design optimization
template <typename dtype>
struct sigmoid_config {};

#if __AIE_ARCH__ == 20 // AIE2
template <>
struct sigmoid_config<int8_t> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 32;
};
template <>
struct sigmoid_config<bfloat16> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 2;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 16;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <>
struct sigmoid_config<int8_t> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 32;
};
template <>
struct sigmoid_config<bfloat16> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 2;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 32;
};
#endif                                         // AIE_ARCH

template <typename dtype>
requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>) class elementwise_sigmoid {};

// implementation of sigmoid<bfloat16>
template <>
class elementwise_sigmoid<bfloat16> : elementwise_unary_base<bfloat16, sigmoid_templated_params_t<bfloat16> > {
   public:
    static constexpr uint16_t vec_size = sigmoid_config<bfloat16>::vec_size;

    using acc_t = aie::accum<accfloat, vec_size>;
    using vec_t = aie::vector<bfloat16, vec_size>;
    using params_t = elementwise_unary_params_t<sigmoid_templated_params_t<bfloat16> >;

   private:
    // Sigmoid const
#ifndef __X86SIM__
    static constexpr bfloat16 C0 = FLOAT_TO_BFLOAT16(float(0.5));
    static constexpr bfloat16 C1 = FLOAT_TO_BFLOAT16(float(0.25));
    static constexpr bfloat16 C2 = FLOAT_TO_BFLOAT16(float(0.03125));
    static constexpr bfloat16 lower_bound = FLOAT_TO_BFLOAT16(float(-4.0));
    static constexpr bfloat16 upper_bound = FLOAT_TO_BFLOAT16(float(4.0));
#else
    bfloat16 C0 = 0.5;
    bfloat16 C1 = 0.25;
    bfloat16 C2 = 0.03125;
    bfloat16 lower_bound = -4.0;
    bfloat16 upper_bound = 4.0;
#endif
    vector<bfloat16, vec_size> C0_vec = broadcast<bfloat16, vec_size>(C0);
    acc_t C0_acc;

   public:
    static constexpr bool peel = sigmoid_config<bfloat16>::peel;
    static constexpr uint8_t unroll = sigmoid_config<bfloat16>::unroll;
    static constexpr uint16_t minimum_num_iter = sigmoid_config<bfloat16>::minimum_num_iter;

    elementwise_sigmoid(const params_t& op_params) {
        C0_acc.from_vector(C0_vec);
    }

    __attribute__((always_inline)) static void setup(params_t& op_params, const void* rtps) {}

    inline __attribute__((always_inline)) void body(const vec_t& in, vec_t& out, params_t& op_params) {
        // using sigmoid for approximation
        vec_t mul1_vec = min(in, upper_bound);
        mul1_vec = max(mul1_vec, lower_bound);
        vec_t mul1_vec_abs = abs(mul1_vec);
        acc_t mul0 = mul(C2, mul1_vec_abs);
        acc_t mac0 = mac(C0_acc, mul1_vec, C1);
        acc_t msc0 = msc(mac0, mul0.template to_vector<bfloat16>(), mul1_vec);
        out = msc0.to_vector<bfloat16>();
    }
};

// implementation of sigmoid<int8_t>
template <>
class elementwise_sigmoid<int8_t> : elementwise_unary_base<int8_t, sigmoid_templated_params_t<int8_t> > {
   public:
    static constexpr uint16_t vec_size = sigmoid_config<int8_t>::vec_size;

    using acc_t = aie::accum<acc32, vec_size>;
    using vec_t = aie::vector<int8_t, vec_size>;
    using params_t = elementwise_unary_params_t<sigmoid_templated_params_t<int8_t> >;

   private:
    int8_t coeff_scale_bits = 0;
#if __AIE_ARCH__ == 20 // AIE2
    int8_t input_scale_bits = 2;
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
    int8_t input_scale_bits = 0;
#endif
    int16_t shift_out = 0;

    // 4, -4, 1/4 and 1/2 are scale adjusted and broadcasted into respective vectors
    // 1/32 is always scaled to 1
    vector<int16_t, vec_size> plus_four_vec;
    vector<int16_t, vec_size> minus_four_vec;
    vector<uint16_t, vec_size> vec_1by4;
    vector<int32_t, vec_size> vec_1by2;
    acc_t acc_0;

   public:
    static constexpr bool peel = sigmoid_config<int8_t>::peel;
    static constexpr uint8_t unroll = sigmoid_config<int8_t>::unroll;
    static constexpr uint16_t minimum_num_iter = sigmoid_config<int8_t>::minimum_num_iter;

#if __AIE_ARCH__ == 20 // AIE2
    elementwise_sigmoid(const params_t& op_params) {
        // up-scale coeff if ifm_shift >= 0, otherwise up-scale input
        if (op_params.ifm_shift < 0) {
            input_scale_bits = 2 - op_params.ifm_shift;
        } else {
            coeff_scale_bits = op_params.ifm_shift;
        }
        shift_out = 2 * coeff_scale_bits + (5 + 4) - op_params.ofm_shift;

        // 4, -4, 1/4 and 1/2 are scale adjusted and broadcasted into respective vectors
        // 1/32 is always scaled to 1
        plus_four_vec = broadcast<int16_t, vec_size>((int16_t)(16 << (coeff_scale_bits)));
        minus_four_vec = broadcast<int16_t, vec_size>((int16_t)((-16) << (coeff_scale_bits)));
        vec_1by4 = broadcast<uint16_t, vec_size>((uint16_t)(32 << (coeff_scale_bits)));
        vec_1by2 = broadcast<int32_t, vec_size>((int32_t)(256 << (2 * (coeff_scale_bits))));
        acc_0.from_vector(vec_1by2); // Initialize the accum with scaled 1/2.
    }

    __attribute__((always_inline)) static void setup(params_t& op_params, const void* rtps) {}

#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
    elementwise_sigmoid(const params_t& op_params) {
        // up-scale coeff if ifm_shift >= 0, otherwise up-scale input
        if (op_params.ifm_shift < 0) {
            input_scale_bits = abs(op_params.ifm_shift);
        } else {
            coeff_scale_bits = op_params.ifm_shift;
        }
        shift_out = 2 * coeff_scale_bits + 5 - op_params.ofm_shift;
        // 4, -4, 1/4 and 1/2 are scale adjusted and broadcasted into respective vectors
        // 1/32 is always scaled to 1
        plus_four_vec = broadcast<int16_t, vec_size>((int16_t)(4 << coeff_scale_bits));
        minus_four_vec = broadcast<int16_t, vec_size>((int16_t)((-4) << coeff_scale_bits));
        vec_1by4 = broadcast<uint16_t, vec_size>((uint16_t)(8 << coeff_scale_bits));
        vec_1by2 = broadcast<int32_t, vec_size>((int32_t)(16 << (2 * coeff_scale_bits)));
        acc_0.from_vector(vec_1by2); // Initialize the accum with scaled 1/2.
    }

    __attribute__((always_inline)) static void setup(params_t& op_params, const void* rtps) {}

#endif

    inline __attribute__((always_inline)) void body(const vec_t& in, vec_t& out, params_t& op_params) {
        vector<int16_t, vec_size> vec_in_int16, vec_in_abs;
        vec_in_int16 = unpack(in); // Change input to int16 type. Only type change, no up-shift.
        vec_in_int16 = upshift(vec_in_int16, input_scale_bits);
        vec_in_int16 = min(vec_in_int16, plus_four_vec);  // Compare with 4.0 and clip input greater than it.
        vec_in_int16 = max(vec_in_int16, minus_four_vec); // Compare with -4.0 and clip input less than it.
        vec_in_abs = abs(vec_in_int16);
        acc_t acc_1 = mac(acc_0, vec_in_int16, vec_1by4);   //(1/2) + (1/4)*x
        acc_t acc_2 = msc(acc_1, vec_in_abs, vec_in_int16); //(1/2)+(1/4)*x-(1/32)*x*|x| (1/32 is scaled to 1).
        out = acc_2.to_vector<int8_t>(shift_out);
    }
};
