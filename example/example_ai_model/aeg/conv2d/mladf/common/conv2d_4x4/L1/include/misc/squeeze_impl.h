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
#include "squeeze_params.h"

// User configuration for design optimization
template <typename dtype> struct squeeze_config
{
};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct squeeze_config<int8_t>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 32;
};
template <> struct squeeze_config<bfloat16>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 16;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct squeeze_config<int8_t>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 64;
};
template <> struct squeeze_config<bfloat16>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 32;
};
#endif                                         // AIE_ARCH

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_squeeze {};

// implementation of squeeze<bfloat16>
template <> class elementwise_squeeze<bfloat16> : elementwise_unary_base<bfloat16, squeeze_params_t<bfloat16>>
{
  public:
    static constexpr bool peel = squeeze_config<bfloat16>::peel;
    static constexpr uint8_t unroll = squeeze_config<bfloat16>::unroll;
    static constexpr uint16_t minimum_num_iter = squeeze_config<bfloat16>::minimum_num_iter;
    static constexpr uint16_t vec_size = squeeze_config<bfloat16>::vec_size;

    using acc_t = accfloat;
    using vec_t = aie::vector<bfloat16, vec_size>;
    using params_t = elementwise_unary_params_t<squeeze_params_t<bfloat16>>;

    elementwise_squeeze(const params_t &op_params)
    {
    } // setup auxiliary member variable if needed

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps)
    {
    }

    inline __attribute__((always_inline)) void body(const vec_t &in, vec_t &out, params_t &op_params)
    {
        out = in;
    }
};

// implementation of squeeze<int8_t>
template <> class elementwise_squeeze<int8_t> : elementwise_unary_base<int8_t, squeeze_params_t<int8_t>>
{
  public:
    static constexpr bool peel = squeeze_config<int8_t>::peel;
    static constexpr uint8_t unroll = squeeze_config<int8_t>::unroll;
    static constexpr uint16_t minimum_num_iter = squeeze_config<int8_t>::minimum_num_iter;
    static constexpr uint16_t vec_size = squeeze_config<int8_t>::vec_size;

    using acc_t = acc32;
    using vec_t = aie::vector<int8_t, vec_size>;
    using params_t = elementwise_unary_params_t<squeeze_params_t<int8_t>>;

    elementwise_squeeze(const params_t &op_params)
    {
    } // setup auxiliary member variable if needed

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps)
    {
    }

    inline __attribute__((always_inline)) void body(const vec_t &in, vec_t &out, params_t &op_params)
    {
        out = in;
    }
};