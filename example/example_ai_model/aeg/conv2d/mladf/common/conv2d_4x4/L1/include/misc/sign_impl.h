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
#include "sign_params.h"

// User configuration for design optimization
template <typename dtype> struct sign_config
{
};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct sign_config<int8_t>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 4;
    static constexpr uint16_t vec_size = 64;
};
template <> struct sign_config<bfloat16>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 4;
    static constexpr uint16_t vec_size = 32;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct sign_config<int8_t>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 4;
    static constexpr uint16_t vec_size = 64;
};
template <> struct sign_config<bfloat16>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 4;
    static constexpr uint16_t vec_size = 32;
};
#endif                                         // AIE_ARCH

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_sign : elementwise_unary_base<dtype, sign_params_t<dtype>>
{
  public:
    static constexpr bool peel = sign_config<dtype>::peel;
    static constexpr uint8_t unroll = sign_config<dtype>::unroll;
    static constexpr uint16_t minimum_num_iter = sign_config<dtype>::minimum_num_iter;
    static constexpr uint16_t vec_size = sign_config<dtype>::vec_size;

    using acc_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, accfloat, acc32>;
    using vec_t = aie::vector<dtype, vec_size>;
    using params_t = elementwise_unary_params_t<sign_params_t<dtype>>;

    elementwise_sign(const params_t &op_params)
    {
    } // setup auxiliary member variable if needed

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps)
    {
    }

    inline __attribute__((always_inline)) void body(const vec_t &in, vec_t &out, params_t &op_params)
    {
        out = vsign<vec_size>(in, op_params);
    }

  private:
    template <int VecLen>
    inline __attribute__((always_inline)) aie::vector<dtype, VecLen> vsign(const vec_t &vec_in, params_t &op_params)
    {
        // Initialize to zero
        aie::vector<dtype, VecLen> vec_out = aie::broadcast<dtype, VecLen>(0);
        aie::vector<dtype, VecLen> one = aie::broadcast<dtype, VecLen>(1);
        aie::vector<dtype, VecLen> neg_one = aie::broadcast<dtype, VecLen>(-1);
        // mask positive number
        aie::mask<VecLen> mask_postive = aie::gt(vec_in, (dtype)0);
        vec_out = aie::select(vec_out, one, mask_postive);
        // mask negative number
        aie::mask<VecLen> mask_negative = aie::lt(vec_in, (dtype)0);
        vec_out = aie::select(vec_out, neg_one, mask_negative);
        return vec_out;
    }
};
