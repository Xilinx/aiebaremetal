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

#include "utils.h"

#include "elementwise_binary.h"
#include "mod_params.h"

namespace {
template <typename dtype> struct mod_config {};
#if __AIE_ARCH__ == 20
template <> struct mod_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 32;
};
#else
template <> struct mod_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 8;
  static constexpr uint16_t vec_size = 64;
};
#endif // __AIE_ARCH__
} // anonymous namespace
template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class mod_impl : public elementwise_binary_base<dtype, mod_params_t<dtype>> {
public:
  static constexpr bool peel = mod_config<dtype>::peel;
  static constexpr uint8_t unroll = mod_config<dtype>::unroll;
  static constexpr uint16_t minimum_num_iter = mod_config<dtype>::minimum_num_iter;
  static constexpr uint16_t vec_size = mod_config<dtype>::vec_size;
  using acc_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, accfloat, acc32>;
  using vec_t = aie::vector<dtype, vec_size>;
  using params_t = elementwise_binary_params_t<mod_params_t<dtype>>;


public:
  inline __attribute__((always_inline)) static auto narrow_res(const aie::accum<acc_t,vec_size> &acc_vec1,
                                                        const aie::vector<bfloat16, vec_size> &vec2_abs,
                                                        const aie::vector<bfloat16, vec_size> &vect2_real,
                                                        int16_t multiples){

      auto res_msc = aie::msc(acc_vec1, vect2_real, (bfloat16) multiples);
      aie::vector<bfloat16, vec_size> vec_msc = res_msc.template to_vector<bfloat16>(0);
      auto msk_lt_err  = aie::lt(vec_msc, vec2_abs);
      auto msk_gt_zero = aie::ge(vec_msc, (bfloat16) 0.0f);
      auto msk_merged  = msk_lt_err & msk_gt_zero;
      auto vec_res = aie::select((bfloat16) 0, vec_msc, msk_merged);
      return vec_res;

  }


  mod_impl(const params_t &op_params) {} // setup auxiliary member variable if needed
  inline __attribute__((always_inline)) void body(const vec_t &in_vec1,
                                                  const vec_t &in_vec2,
                                                  vec_t &out_vec,
                                                  params_t &op_params) {


    auto vec1_cast_i16 = in_vec1.template cast_to<int16_t>();
    auto vec2_cast_i16 = in_vec2.template cast_to<int16_t>();

    aie::vector<int16_t, vec_size> vec1_sign = bit_and((int16_t)0x8000, vec1_cast_i16);
    aie::vector<int16_t, vec_size> vec2_sign = bit_and((int16_t)0x8000, vec2_cast_i16);
    
    aie::vector<int16_t, vec_size> vec1_val = bit_and((int16_t)0x7FFF, vec1_cast_i16);
    aie::vector<int16_t, vec_size> vec2_val = bit_and((int16_t)0x7FFF, vec2_cast_i16);

    aie::vector<int16_t, vec_size> vec1_neg_i16 = bit_or((int16_t)0x8000, vec1_cast_i16);
    aie::vector<int16_t, vec_size> vec2_neg_i16 = bit_or((int16_t)0x8000, vec2_cast_i16);


    auto vec1_abs = vec1_val.template cast_to<bfloat16>();
    auto vec2_abs = vec2_val.template cast_to<bfloat16>();

    auto vec1_neg = vec1_neg_i16.template cast_to<bfloat16>();
    auto vec2_neg = vec2_neg_i16.template cast_to<bfloat16>();

    aie::accum<acc_t,vec_size> acc_vec1;
    aie::accum<acc_t, vec_size> res_acc;

    auto mask_sign_neq = aie::neq(vec1_sign, vec2_sign);

    auto vect1_real = aie::select(vec1_abs, vec1_neg, mask_sign_neq);
    auto vect2_real = aie::select(vec2_abs, vec2_neg, mask_sign_neq);

    acc_vec1.from_vector(vect1_real, 0.f);

    auto acc_multiples = aie::div(vec1_abs, vec2_abs);
    auto vec_multiples = acc_multiples.template to_vector<bfloat16>(0);
    auto vec_multiples_i16  = aie::to_fixed<int16_t>(vec_multiples, 0);
    auto vec_multiples_bf16 = aie::to_float<bfloat16>(vec_multiples_i16);

    acc_vec1 = aie::msc(acc_vec1, vec_multiples_bf16, vect2_real); // 

    res_acc.from_vector(aie::zeros<bfloat16, vec_size>(), 0);
    auto vec1_res = narrow_res(acc_vec1, vec2_abs, vect2_real, 0);
    res_acc = aie::add(res_acc, vec1_res);
    auto vec2_res = narrow_res(acc_vec1, vec2_abs, vect2_real, 1);
    res_acc = aie::add(res_acc, vec2_res);
    auto vec3_res = narrow_res(acc_vec1, vec2_abs, vect2_real, 2);
    res_acc = aie::add(res_acc, vec3_res);

    auto unsigned_vec = res_acc.template to_vector<bfloat16>(op_params.ofm_shift_biased);
    auto int16_vec = bit_or(vec2_sign, unsigned_vec.template cast_to<int16_t>());
    auto mask_neg_zero = aie::eq(int16_vec, (int16_t)0x8000);
    auto int16_vec_res = aie::select(int16_vec, (int16_t)0x0000, mask_neg_zero);
    out_vec = int16_vec_res.template cast_to<bfloat16>();

  }
  inline __attribute__((always_inline)) static void
  setup(params_t &op_params, const void *rtps) {}

private:
  // declare your auxiliary member variable here.
};
