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
#include "logical_not_params.h"

template <typename dtype> struct logical_not_config {};
// use "#if __AIE_ARCH__ == 20, 21, 22" to fine tune your config
template <> struct logical_not_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 8;
  static constexpr uint16_t vec_size = 64;
};

template <typename dtype>
    requires(std::is_same_v<dtype, int8_t>)
class elementwise_logical_not : elementwise_unary_base<dtype, logical_not_params_t<dtype>> {
public:
  static constexpr bool peel = logical_not_config<dtype>::peel;
  static constexpr uint8_t unroll = logical_not_config<dtype>::unroll;
  static constexpr uint16_t minimum_num_iter = logical_not_config<dtype>::minimum_num_iter;
  static constexpr uint16_t vec_size = logical_not_config<dtype>::vec_size;
  using vec_t = aie::vector<dtype, vec_size>;
  using params_t = elementwise_unary_params_t<logical_not_params_t<dtype>>;

public:
  elementwise_logical_not(const params_t &op_params) {} // setup auxiliary member variable if needed

  inline __attribute__((always_inline))
  void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
    // 0b00000001 is true, 0b00000000 is false, others are undefined.
    vec_t norm_invec = aie::bit_not(in_vec);
    out_vec = aie::bit_and((int8)0x01, norm_invec);
    return;
  }

  inline __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {}

private:
};
