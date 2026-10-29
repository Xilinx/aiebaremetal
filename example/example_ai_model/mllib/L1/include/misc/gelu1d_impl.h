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

#include "../../include/common/mllib_x86sim_workarounds.h"

#include "adf.h"
#include "aie.hpp"
#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"

#include "elementwise_unary.h"
#include "gelu1d_params.h"
#include "gelu_lut.h"

using namespace aie;

// User configuration for design optimization
template <typename dtype> struct gelu_config {};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct gelu_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 8;
  static constexpr uint16_t vec_size = 32;
};
template <> struct gelu_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 8;
  static constexpr uint16_t vec_size = 16;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct gelu_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 8;
  static constexpr uint16_t vec_size = 32; //limited by acc32
};
template <> struct gelu_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 8;
  static constexpr uint16_t vec_size = 32;
};
#endif


template <typename dtype>
    requires(aie::detail::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_gelu {};

// implementation of gelu<bfloat16>
// accuracy are ensure by 256-segment linear approximation
template <> 
class elementwise_gelu<bfloat16> 
  : elementwise_unary_base<bfloat16, gelu1d_params_t<bfloat16>> {
  
  private:
    // aie::lut<4, uint16> gelu_lut_slope(256, gelu_lut<0, 512>::data_ab, gelu_lut<0, 512>::data_cd);
    aie::lut<4, uint16> gelu_lut_slope = aie::lut<4, uint16>(256, gelu_lut<0, 512>::data_ab, gelu_lut<0, 512>::data_cd);
    aie::lut<4, uint16> gelu_lut_offset = aie::lut<4, uint16>(256, gelu_lut<1, 512>::data_ab, gelu_lut<1, 512>::data_cd);
    aie::parallel_lookup<int16, aie::lut<4, uint16>, aie::lut_oor_policy::truncate> slope_lut = 
        aie::parallel_lookup<int16, aie::lut<4, uint16>, aie::lut_oor_policy::truncate>(gelu_lut_slope);
    aie::parallel_lookup<int16, aie::lut<4, uint16>, aie::lut_oor_policy::truncate> offset_lut = 
        aie::parallel_lookup<int16, aie::lut<4, uint16>, aie::lut_oor_policy::truncate>(gelu_lut_offset);

  public:
  static constexpr bool peel = gelu_config<bfloat16>::peel;
  static constexpr uint8_t unroll = gelu_config<bfloat16>::unroll;
  static constexpr uint16_t minimum_num_iter = gelu_config<bfloat16>::minimum_num_iter;
  static constexpr uint16_t vec_size = gelu_config<bfloat16>::vec_size;

  using acc_t = aie::accum<accfloat, vec_size>;
  using vec_t = aie::vector<bfloat16, vec_size>;
  using params_t = elementwise_unary_params_t<gelu1d_params_t<bfloat16>>;

  vec_t P0_vec, P1_vec;
  acc_t approx_acc, result_acc;

  elementwise_gelu(const params_t &op_params) {
        P0_vec = aie::broadcast<bfloat16, vec_size>(-5.0);
        P1_vec = aie::broadcast<bfloat16, vec_size>(5.0);
  }

  // setup auxiliary member variable if needed
  __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {}

  inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
        // step-1: calculate LUT index, clip x to [-5, 5)
        vec_t x_plus_P1 = aie::add(in_vec, P1_vec); //  x+5
        acc_t mul0_acc = aie::mul(x_plus_P1, bfloat16(256)); //  (x+5)*256
        acc_t mul1_acc = aie::mul(mul0_acc.template to_vector<bfloat16>(), bfloat16(0.100097656)); // (x+5)*25.6
        vec_t mul1_bf16 = mul1_acc.template to_vector<bfloat16>();
        aie::vector<int16, vec_size> floor_idx = aie::to_fixed<int16>(mul1_bf16, 0); // floor
        aie::mask<vec_size> lt_idx_mask = aie::lt(floor_idx, int16(0));      // idx<0,->0
        aie::mask<vec_size> gt_idx_mask = aie::gt(floor_idx, int16(255));    // idx>255,->255
        aie::vector<int16, vec_size> lut_idx = aie::select(floor_idx, int16(0), lt_idx_mask);
        lut_idx = aie::select(lut_idx, int16(255), gt_idx_mask);

        // step-2: fetch slope & offset
        aie::vector<uint16, vec_size> slope_vec = slope_lut.fetch(lut_idx);
        aie::vector<uint16, vec_size> offset_vec = offset_lut.fetch(lut_idx);

        // step-3: compute linear approximation, slope * x + offset
        approx_acc.from_vector(offset_vec.cast_to<bfloat16>());
        result_acc = aie::mac(approx_acc, in_vec, slope_vec.cast_to<bfloat16>());

        // step-4: mask output
        aie::mask<vec_size> lt_P0_mask = aie::lt(in_vec, P0_vec);  // x<-5
        aie::mask<vec_size> ge_P1_mask = aie::ge(in_vec, P1_vec);  // x>=5
        out_vec = aie::select(result_acc.template to_vector<bfloat16>(), (bfloat16)0, lt_P0_mask);
        out_vec = aie::select(out_vec, in_vec, ge_P1_mask);
  }
};

// implementation of gelu<int8_t>
template <> 
class elementwise_gelu<int8_t> 
  : elementwise_unary_base<int8_t, gelu1d_params_t<int8_t>> {

private:
#ifndef __X86SIM__
    // Gelu const
    static constexpr int8_t P0 = 109;//1.702 * 64
    static constexpr int16_t shift_p0 = 6;
#else
    int8_t P0 = 109;
    int16_t shift_p0 = 6;
#endif

public:
  static constexpr bool peel = gelu_config<int8_t>::peel;
  static constexpr uint8_t unroll = gelu_config<int8_t>::unroll;
  static constexpr uint16_t minimum_num_iter = gelu_config<int8_t>::minimum_num_iter;
  static constexpr uint16_t vec_size = gelu_config<int8_t>::vec_size;

  using acc_t = aie::accum<acc32, vec_size>;
  using vec_t = aie::vector<int8_t, vec_size>;
  using params_t = elementwise_unary_params_t<gelu1d_params_t<int8_t>>;

  // 4, -4, 1/4 and 1/2 are scale adjusted and broadcasted into respective vectors
  // 1/32 is always scaled to 1
  aie::vector<int16_t, vec_size> plus_four_vec;
  aie::vector<int16_t, vec_size> minus_four_vec;
  aie::vector<uint16_t, vec_size> vec_1by4;
  aie::vector<int32_t, vec_size> vec_1by2;

  acc_t acc_0, acc_1, acc_2, acc_in_int16_p0;
  aie::accum<acc48,vec_size> acc_res;
  aie::vector<int32_t, vec_size> vec_acc_2;

  aie::vector<int16_t,vec_size> vec_in_int16_orig, vec_in_int16, vec_in_int16_p0, vec_in_abs;
  
  int16_t input_shift = 0;
  int16_t coeff_shift = 0;
  int16_t shift_out   = 0;
  int16_t num_coeffs = 0;

  #if __AIE_ARCH__ == 20 // AIE2
  int16_t shift_init  =  0;
  int16_t shift_scale =  0;
  #endif


  elementwise_gelu(const params_t &op_params) {
          // up-scale coeff if ifm_shift >= 0, otherwise up-scale input
          if (op_params.ifm_shift < 0) { 
              input_shift = abs(op_params.ifm_shift);
              num_coeffs = 2; // with coeff_shift = 0
          } else {
              coeff_shift = op_params.ifm_shift;
              num_coeffs = 3;
          }

        plus_four_vec  = aie::broadcast<int16_t, vec_size>((int16_t)(4<<coeff_shift));
        minus_four_vec = aie::broadcast<int16_t, vec_size>((int16_t)((-4)<<coeff_shift));
        vec_1by4       = aie::broadcast<uint16_t,vec_size>((uint16_t)(8<<coeff_shift));
        vec_1by2       = aie::broadcast<int32_t, vec_size>((int32_t)(16<<(2*coeff_shift)));
        acc_0.from_vector(vec_1by2);
        shift_out = num_coeffs * coeff_shift - op_params.ofm_shift + 5;//5 is shift_1by32   

        #if __AIE_ARCH__ == 20 // AIE2
        if(shift_out >= -1){
          shift_init = shift_out;
          shift_scale = 1;
        }else{
          aie::set_saturation(aie::saturation_mode::saturate);
          shift_init = 0;
          shift_scale = 1 << (-shift_out);
        }
        #endif

  }

  // setup auxiliary member variable if needed
  __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {}

  inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
        // step1 : 1.702*x
        vec_in_int16_orig = unpack(in_vec); //Change input to int16 type. Only type change, no up-shift.
        vec_in_int16 = aie::upshift(vec_in_int16_orig, input_shift);
        acc_in_int16_p0 = mul(vec_in_int16, P0); 
        vec_in_int16_p0 = acc_in_int16_p0.to_vector<int16_t>(shift_p0);
        // step2 : sigmoid(1.702*x)
        aie::vector<int16_t, vec_size> clip_vec = aie::max(aie::min(vec_in_int16_p0, plus_four_vec), minus_four_vec);
        vec_in_abs   = abs(clip_vec);
        acc_1        = mac(acc_0,clip_vec, vec_1by4); //(1/2) + (1/4)*x
        acc_2        = msc(acc_1,vec_in_abs,clip_vec); //(1/2)+(1/4)*x-(1/32)*x*|x| (1/32 is scaled to 1).
        vec_acc_2   = acc_2.to_vector<int32_t>(0);
        // step3 : x*sigmoid(1.702*x)
        acc_res      = mul(vec_in_int16, vec_acc_2);
        // out_vec = aie::pack(acc_res.template to_vector<int16_t>(shift_out));

        #if __AIE_ARCH__ == 20 // AIE2
        auto vec_init = acc_res.template to_vector<int16_t>(shift_init);
        auto out_acc = aie::mul(shift_scale, vec_init);
        out_vec = aie::pack(out_acc.template to_vector<int16_t>(0));
        #elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
        out_vec = aie::pack(acc_res.template to_vector<int16_t>(shift_out));
        #endif


  }
};
