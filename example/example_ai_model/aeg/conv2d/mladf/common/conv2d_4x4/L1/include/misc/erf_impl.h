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
#include "erf_params.h"

// User configuration for design optimization
template <typename dtype> struct erf_config
{
};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct erf_config<int8_t>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 2;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 32;
};
template <> struct erf_config<bfloat16>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 2;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 16;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct erf_config<int8_t>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 64;
};
template <> struct erf_config<bfloat16>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 2;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 32;
};
#endif                                         // AIE_ARCH

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_erf {};

// implementation of erf<bfloat16>
template <> class elementwise_erf<bfloat16> : elementwise_unary_base<bfloat16, erf_params_t<bfloat16>>
{
  private:
    static constexpr bfloat16 Q1 = FLOAT_TO_BFLOAT16(float(1.128379167095476));
    static constexpr bfloat16 Q3 = FLOAT_TO_BFLOAT16(float(-0.3761263890318252));
    static constexpr bfloat16 Q5 = FLOAT_TO_BFLOAT16(float(0.1128379167095475));

    static constexpr bfloat16 one = FLOAT_TO_BFLOAT16(float(1.0));
    static constexpr bfloat16 minus_one = FLOAT_TO_BFLOAT16(float(-1.0));

    template <int VecLen = 16>
    inline __attribute__((always_inline)) aie::vector<bfloat16, VecLen> verf(const aie::vector<bfloat16, VecLen> &vec_in)
    {
        // Approximate the erf() using Maclaurin expansion, with x^1, x^3, and x^5 degrees
        // generate x^1 - x^5
        aie::vector<bfloat16, VecLen> x1 = vec_in;
        aie::accum<accfloat, VecLen> x2_float = aie::mul_square(x1);
        aie::vector<bfloat16, VecLen> x2 = x2_float.template to_vector<bfloat16>();
        aie::accum<accfloat, VecLen> x3_float = aie::mul(x2, x1);
        aie::vector<bfloat16, VecLen> x3 = x3_float.template to_vector<bfloat16>();
        aie::accum<accfloat, VecLen> x4_float = aie::mul_square(x2);
        aie::vector<bfloat16, VecLen> x4 = x4_float.template to_vector<bfloat16>();
        aie::accum<accfloat, VecLen> x5_float = aie::mul(x4, x1);
        aie::vector<bfloat16, VecLen> x5 = x5_float.template to_vector<bfloat16>();

        // accumulate Qi * x^i
        aie::accum<accfloat, VecLen> acc = aie::mul(x1, Q1);
        acc = aie::mac(acc, x3, Q3);
        acc = aie::mac(acc, x5, Q5);

        // process overflows
        aie::vector<bfloat16, VecLen> res = acc.template to_vector<bfloat16>();
        aie::vector<bfloat16, VecLen> min_vec = aie::min(res, one);
        aie::vector<bfloat16, VecLen> vec_out = aie::max(min_vec, minus_one);
        return vec_out;
    }

  public:
    static constexpr bool peel = erf_config<bfloat16>::peel;
    static constexpr uint8_t unroll = erf_config<bfloat16>::unroll;
    static constexpr uint16_t minimum_num_iter = erf_config<bfloat16>::minimum_num_iter;
    static constexpr uint16_t vec_size = erf_config<bfloat16>::vec_size;

    using acc_t = accfloat;
    using vec_t = aie::vector<bfloat16, vec_size>;
    using params_t = elementwise_unary_params_t<erf_params_t<bfloat16>>;

    elementwise_erf(const params_t &op_params)
    {
    } // setup auxiliary member variable if needed

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps)
    {
    }

    inline __attribute__((always_inline)) void body(const vec_t &in, vec_t &out, params_t &op_params)
    {
        out = verf<vec_size>(in);
    }
};

// implementation of erf<int8_t>
template <> class elementwise_erf<int8_t> : elementwise_unary_base<int8_t, erf_params_t<int8_t>>
{
  private:
    // Q1, Q3, Q5 are at scale 13
    static constexpr int16_t Q1_int = 9244; // 1.128379167095476
    static constexpr int16_t Q3_int = 3081; // 0.3761263890318252
    static constexpr int16_t Q5_int = 924; // 0.1128379167095475

    // 1.0 & -1.0 are at scale 6
    static constexpr int16_t one_int = 64;
    static constexpr int16_t minus_one_int = -64;

    template <int VecLen = 32>
    inline __attribute__((always_inline)) aie::vector<int8_t, VecLen> verf(const aie::vector<int8_t, VecLen> &vec_in, params_t & op_params)
    {
        // Approximate the erf() using Maclaurin expansion, with x^1, x^3, and x^5 degrees
        // generate x^1 - x^5
        aie::vector<int16_t, VecLen> x1 = aie::unpack(vec_in); // at scale ifm_shift_biased
        aie::accum<acc32, VecLen> x2_acc = aie::mul_square(x1);
        aie::vector<int16_t, VecLen> x2 = x2_acc.template to_vector<int16_t>(op_params.ifm_shift_biased); // at scale ifm_shift_biased
        aie::accum<acc32, VecLen> x3_acc = aie::mul(x2, x1);
        aie::vector<int16_t, VecLen> x3 = x3_acc.template to_vector<int16_t>(op_params.ifm_shift_biased); // at scale ifm_shift_biased
        aie::accum<acc32, VecLen> x4_acc = aie::mul_square(x2);
        aie::vector<int16_t, VecLen> x4 = x4_acc.template to_vector<int16_t>(op_params.ifm_shift_biased); // at scale ifm_shift_biased
        aie::accum<acc32, VecLen> x5_acc = aie::mul(x4, x1);
        aie::vector<int16_t, VecLen> x5 = x5_acc.template to_vector<int16_t>(op_params.ifm_shift_biased); // at scale ifm_shift_biased

        // accumulate Qi * x^i
        aie::accum<acc32, VecLen> acc = aie::mul(x1, Q1_int); // at scale ifm_shift_biased + 13
        acc = aie::msc(acc, x3, Q3_int); // at scale ifm_shift_biased + 13
        acc = aie::mac(acc, x5, Q5_int); // at scale ifm_shift_biased + 13

        // process overflows
        aie::vector<int16_t, VecLen> res = acc.template to_vector<int16_t>(op_params.ifm_shift_biased + (13 - op_params.ofm_shift_biased));
        aie::vector<int16_t, VecLen> min_vec = aie::min(res, one_int);
        aie::vector<int16_t, VecLen> vec_out = aie::max(min_vec, minus_one_int);
        return aie::pack(vec_out);
    }

  public:
    static constexpr bool peel = erf_config<int8_t>::peel;
    static constexpr uint8_t unroll = erf_config<int8_t>::unroll;
    static constexpr uint16_t minimum_num_iter = erf_config<int8_t>::minimum_num_iter;
    static constexpr uint16_t vec_size = erf_config<int8_t>::vec_size;

    using acc_t = acc32;
    using vec_t = aie::vector<int8_t, vec_size>;
    using params_t = elementwise_unary_params_t<erf_params_t<int8_t>>;

    elementwise_erf(const params_t &op_params)
    {
    } // setup auxiliary member variable if needed

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps)
    {
    }

    inline __attribute__((always_inline)) void body(const vec_t &in, vec_t &out, params_t &op_params)
    {
        out = verf<vec_size>(in, op_params);
    }
};
