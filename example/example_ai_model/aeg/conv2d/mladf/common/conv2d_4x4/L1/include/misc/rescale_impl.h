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
#include "rescale_params.h"

// User configuration for design optimization
template <typename dtype> struct rescale_config
{
};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct rescale_config<int8_t>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 2;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 32;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct rescale_config<int8_t>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 2;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 64;
};
#endif                                         // AIE_ARCH

template <typename dtype>
    requires(std::is_same_v<dtype, int8_t>)
class elementwise_rescale : public elementwise_unary_base<dtype, rescale_params_t<dtype>>
{
  private:

  public:
    static constexpr bool peel = rescale_config<int8_t>::peel;
    static constexpr uint8_t unroll = rescale_config<int8_t>::unroll;
    static constexpr uint16_t minimum_num_iter = rescale_config<int8_t>::minimum_num_iter;
    static constexpr uint16_t vec_size = rescale_config<int8_t>::vec_size;

    using acc_t = acc32;
    using vec_t = aie::vector<int8_t, vec_size>;
    using params_t = elementwise_unary_params_t<rescale_params_t<int8_t>>;

    elementwise_rescale(const params_t &op_params)
    {
    } // setup auxiliary member variable if needed

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps)
    {
    }

    inline __attribute__((always_inline)) void body(const vec_t &in, vec_t &out, params_t &op_params)
    {
        auto intermediate = aie::from_vector<acc_t>(in, op_params.ifm_shift_biased);
        out = intermediate.template to_vector<int8_t>(op_params.ofm_shift_biased);
    }
};
