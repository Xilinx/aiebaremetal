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
#include "log_params.h"
#include "log_lut.h"

// User configuration for design optimization
template <typename dtype> struct log_config
{
};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct log_config<int8_t>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 4;
    static constexpr uint16_t vec_size = 32;
};
template <> struct log_config<bfloat16>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 4;
    static constexpr uint16_t vec_size = 16;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct log_config<int8_t>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 4;
    static constexpr uint16_t vec_size = 64;
};
template <> struct log_config<bfloat16>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 4;
    static constexpr uint16_t vec_size = 32;
};
#endif                                         // AIE_ARCH

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_log {};

// implementation of log<bfloat16>
template <> class elementwise_log<bfloat16> : elementwise_unary_base<bfloat16, log_params_t<bfloat16>>
{
  private:
    static constexpr bfloat16 log2 = FLOAT_TO_BFLOAT16(float(0.69140625));
    static constexpr int16_t exp_mask = 0x7f80;
    static constexpr int16_t frac_mask = 0x7F;
    static constexpr int16_t shift = 127;

    static constexpr unsigned step = 0;
    static constexpr unsigned bias = 0;

    template <int VecLen = 16>
    inline __attribute__((always_inline)) aie::vector<bfloat16, VecLen> vlog(const aie::vector<bfloat16, VecLen> &vec_in)
    {
        aie::lut<4, bfloat16, bfloat16> lut_f(128, fraction_table_ab, fraction_table_cd);
        aie::parallel_lookup<int8_t, aie::lut<4, bfloat16, bfloat16>, aie::lut_oor_policy::truncate>
            lookup_fraction(lut_f, step, bias);


        aie::vector<bfloat16, VecLen> vec_zero = aie::zeros<bfloat16, VecLen>();
        aie::vector<uint16_t, VecLen> vec_nan  = aie::broadcast<uint16_t, VecLen>(0x7fc0);
        aie::vector<bfloat16, VecLen> vec_nan_bf16 = vec_nan.template cast_to<bfloat16>(); // just type casting, no cycle consumption, same as below
        auto msk_lt = aie::lt(vec_in, vec_zero); // set negative values to zero

        aie::vector<int16_t, VecLen> vec_in_int = vec_in.template cast_to<int16_t>(); // just type casting, no cycle consumption, same as below
        //const aie::vector<int16_t, VecLen> vec_in_int = *(reinterpret_cast<const aie::vector<int16_t, VecLen>*>(&vec_in));
        aie::vector<int16_t, VecLen> exponent;
        aie::vector<int16_t, VecLen> fraction;
        aie::accum<accfloat, VecLen> acc;

        // split exponent and fraction
        aie::accum<acc32, VecLen> exp_acc;
        exp_acc.template from_vector(aie::bit_and(exp_mask, vec_in_int));
        exponent = exp_acc.template to_vector<int16_t>(7);
        exponent = aie::sub(exponent, shift);
        fraction = aie::bit_and(frac_mask, vec_in_int);

        // exp
        aie::accum<accfloat, VecLen> res0_acc;
        res0_acc.template from_vector(aie::to_float(exponent));
        aie::vector<bfloat16, VecLen> res0 = res0_acc.template to_vector<bfloat16>();

        // frac
        aie::vector<bfloat16, VecLen> res1 = lookup_fraction.fetch(fraction);

        // exp * log2 + log(frac)
        acc.template from_vector(res1);
        acc = aie::mac(acc, res0, log2);

        // output
        aie::vector<bfloat16, VecLen> vec_out = acc.template to_vector<bfloat16>();
        vec_out = aie::select(vec_out, vec_nan_bf16, msk_lt);
        return vec_out;
    }

  public:
    static constexpr bool peel = log_config<bfloat16>::peel;
    static constexpr uint8_t unroll = log_config<bfloat16>::unroll;
    static constexpr uint16_t minimum_num_iter = log_config<bfloat16>::minimum_num_iter;
    static constexpr uint16_t vec_size = log_config<bfloat16>::vec_size;

    using acc_t = accfloat;
    using vec_t = aie::vector<bfloat16, vec_size>;
    using params_t = elementwise_unary_params_t<log_params_t<bfloat16>>;

    elementwise_log(const params_t &op_params)
    {
    } // setup auxiliary member variable if needed

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps)
    {
    }

    inline __attribute__((always_inline)) void body(const vec_t &in, vec_t &out, params_t &op_params)
    {
        out = vlog<vec_size>(in);
    }
};

// implementation of log<int8_t>
template <> class elementwise_log<int8_t> : elementwise_unary_base<int8_t, log_params_t<int8_t>>
{
  private:
    template <int VecLen = 32>
    inline __attribute__((always_inline)) aie::vector<int8_t, VecLen> vlog(const aie::vector<int8_t, VecLen> &vec_in)
    {
        aie::vector<int8_t, VecLen> vec_out = aie::broadcast<int8_t, VecLen>((int8_t)-128);
        aie::mask<VecLen> is_large_1 = aie::ge(vec_in, (int8_t)1);
        aie::mask<VecLen> is_large_2 = aie::ge(vec_in, (int8_t)2);
        aie::mask<VecLen> is_large_5 = aie::ge(vec_in, (int8_t)5);
        aie::mask<VecLen> is_large_13 = aie::ge(vec_in, (int8_t)13);
        aie::mask<VecLen> is_large_33 = aie::ge(vec_in, (int8_t)33);
        aie::mask<VecLen> is_large_92 = aie::ge(vec_in, (int8_t)92);

        vec_out = aie::select(vec_out, (int8_t)0, is_large_1);
        vec_out = aie::select(vec_out, (int8_t)1, is_large_2);
        vec_out = aie::select(vec_out, (int8_t)2, is_large_5);
        vec_out = aie::select(vec_out, (int8_t)3, is_large_13);
        vec_out = aie::select(vec_out, (int8_t)4, is_large_33);
        vec_out = aie::select(vec_out, (int8_t)5, is_large_92);

        return vec_out;
    }

  public:
    static constexpr bool peel = log_config<int8_t>::peel;
    static constexpr uint8_t unroll = log_config<int8_t>::unroll;
    static constexpr uint16_t minimum_num_iter = log_config<int8_t>::minimum_num_iter;
    static constexpr uint16_t vec_size = log_config<int8_t>::vec_size;

    using acc_t = acc32;
    using vec_t = aie::vector<int8_t, vec_size>;
    using params_t = elementwise_unary_params_t<log_params_t<int8_t>>;

    elementwise_log(const params_t &op_params)
    {
    } // setup auxiliary member variable if needed

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps)
    {
    }

    inline __attribute__((always_inline)) void body(const vec_t &in, vec_t &out, params_t &op_params)
    {
        out = vlog<vec_size>(in);
    }
};
