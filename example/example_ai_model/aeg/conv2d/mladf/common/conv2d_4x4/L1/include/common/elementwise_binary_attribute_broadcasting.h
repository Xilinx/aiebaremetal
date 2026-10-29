// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

/*
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

#include <array>
#include <type_traits>
#include <typeinfo>

#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"
#include "templatized_traits.h"

#include "elementwise_binary.h"
#include "elementwise_binary_broadcasting.h"

// Specific for Add/Sub/Mul/Div Binary Operator
template <typename dtype,
          class concrete_operator,
          class concrete_param_t>
class elementwise_binary_attribute_broadcasting: public elementwise_binary_broadcasting<dtype, concrete_operator, concrete_param_t>
{
public:
    using params_t = elementwise_binary_params_t<concrete_param_t>;
    using elementwise_binary_t = elementwise_binary_broadcasting<dtype, concrete_operator, concrete_param_t>;

    union itof
    {
        uint16_t i;
        bfloat16 f;
    };

    __attribute__((noinline)) static const void run(dtype * __restrict ifm,
                                                    dtype * __restrict ofm,
                                                    params_t &params)
    {
        // add/mul/div/sub
        dtype *p_in1 = NULL;
        dtype *p_in2 = NULL;
        union itof scalar;
        // int offset = sizeof(dtype) == 1 ? 16384 : 8192;
        int offset = params.ifmsv_size;

        if (params.scalar_position == 0)
        { // scalar on LHS
            // p_in1 = ifm - offset;
            p_in1 = ifm + offset;
            p_in2 = ifm;
            if constexpr (sizeof(dtype) == 1)
                p_in1[0] = (dtype)params.scalar;
            else
            {
                scalar.i = params.scalar;
                p_in1[0] = scalar.f;
            }
        }
        else
        { // scalar on RHS
            p_in1 = ifm;
            p_in2 = ifm + offset;
            if constexpr (sizeof(dtype) == 1)
                p_in2[0] = (dtype)params.scalar;
            else
            {
                scalar.i = params.scalar;
                p_in2[0] = scalar.f;
            }
        }

        elementwise_binary_t::run(p_in1, p_in2, ofm, params);
    }
};
