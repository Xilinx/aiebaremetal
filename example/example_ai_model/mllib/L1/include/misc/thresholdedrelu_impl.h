/*
    Copyright (C) 2014 - 2022 Xilinx, Inc. All rights reserved.
    Copyright (C) 2025 Advanced Micro Devices, Inc.

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
#include "thresholdedrelu_params.h"

// User configuration for design optimization
template <typename dtype> struct thresholdedrelu_config {};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct thresholdedrelu_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 32;
};
template <> struct thresholdedrelu_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 32;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct thresholdedrelu_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 64;
};
template <> struct thresholdedrelu_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 32;
};
#endif

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_thresholdedrelu {};

template <> class elementwise_thresholdedrelu<bfloat16> : elementwise_unary_base<bfloat16, thresholdedrelu_params_t<bfloat16>> {
private:
  bfloat16 replace_val;
  bfloat16 threshold_val;

public:
  static constexpr bool peel = thresholdedrelu_config<bfloat16>::peel;
  static constexpr uint8_t unroll = thresholdedrelu_config<bfloat16>::unroll;
  static constexpr uint16_t minimum_num_iter = thresholdedrelu_config<bfloat16>::minimum_num_iter;
  static constexpr uint16_t vec_size = thresholdedrelu_config<bfloat16>::vec_size;

  using acc_t = accfloat;
  using vec_t = aie::vector<bfloat16, vec_size>;
  using params_t = elementwise_unary_params_t<thresholdedrelu_params_t<bfloat16>>;

  
  elementwise_thresholdedrelu(const params_t &op_params) {
    threshold_val = op_params.op_params.threshold_val;
    replace_val   = op_params.op_params.replace_val;
  }

  // setup auxiliary member variable if needed
  __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {
        bfloat16 thres_v = ((const bfloat16*) rtps)[16];
        bfloat16   rep_v = ((const bfloat16*) rtps)[17];
        op_params.op_params.threshold_val = thres_v;
        op_params.op_params.replace_val = rep_v;

  }

  inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
    auto ls_mask = aie::le(in_vec, threshold_val);
    out_vec = aie::select(in_vec, replace_val, ls_mask);

  }
};

// implementation of thresholdedrelu<int8_t>/hardsigmoid<int8_t>
template <> class elementwise_thresholdedrelu<int8_t> : elementwise_unary_base<int8_t, thresholdedrelu_params_t<int8_t>> {
private:
  int16_t threshold_i16;
  int16_t replace_i16;
  int16_t scale_shift;
  int16_t shift_out;
  uint8_t ifm_shift_biased = 0;

public:
  static constexpr bool peel = thresholdedrelu_config<int8_t>::peel;
  static constexpr uint8_t unroll = thresholdedrelu_config<int8_t>::unroll;
  static constexpr uint16_t minimum_num_iter = thresholdedrelu_config<int8_t>::minimum_num_iter;
  static constexpr uint16_t vec_size = thresholdedrelu_config<int8_t>::vec_size;

  using acc_t = acc32;
  using vec_t = aie::vector<int8_t, vec_size>;
  using params_t = elementwise_unary_params_t<thresholdedrelu_params_t<int8_t>>;

  elementwise_thresholdedrelu(const params_t &op_params) {
    threshold_i16 = op_params.op_params.threshold_i16;
    replace_i16   = op_params.op_params.replace_i16;
    scale_shift   = op_params.op_params.scale_shift;
    shift_out     = scale_shift + op_params.ofm_shift_biased;
    ifm_shift_biased = op_params.ifm_shift_biased;

  }

  // setup auxiliary member variable if needed
  __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {

      // aie::set_saturation(aie::detail::saturation_mode::saturate);
      int16_t thres_i16     = ((const int16_t*) rtps)[16];
      int16_t rep_i16       = ((const int16_t*) rtps)[17];
      uint8_t scale_shift   = ((const uint8_t*) rtps)[36];

      op_params.op_params.threshold_i16 = thres_i16;
      op_params.op_params.replace_i16   = rep_i16;  
      op_params.op_params.scale_shift   = scale_shift;   
  }

  inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {

    aie::accum<acc32, vec_size> temp_acc;
    aie::vector<int16_t, vec_size> input_vec  = unpack(in_vec);
    aie::vector<int16_t, vec_size> input_i16  = upshift(input_vec, scale_shift);
  
    auto ls_mask = aie::le(input_i16, threshold_i16);
    aie::vector<int16_t, vec_size> res_vect = aie::select(input_i16, replace_i16, ls_mask);

    temp_acc.from_vector(res_vect, ifm_shift_biased);
    out_vec = temp_acc.to_vector<int8>(shift_out);

  }
};
