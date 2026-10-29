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

#include "aie.hpp"
#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"

//#define DEBUG_REQ 1

#include "requantize_params.h"

// User configuration for design optimization
template <typename dtype> struct requantize_config {};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct requantize_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 32;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct requantize_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 32;
};
#endif                                         // AIE_ARCH

template <typename ifm_dtype, typename ofm_dtype>
requires(mllib::utils::is_one_of_v<ifm_dtype, uint16_t, int16_t>) class elementwise_requantize {};

template <typename ifm_dtype, typename ofm_dtype, class concrete_operator, class concrete_param_t>
class requantize_elementwise_unary {
public:
  using vec_in_t = aie::vector<ifm_dtype, concrete_operator::vec_size>;
  using vec_out_t = aie::vector<ofm_dtype, concrete_operator::vec_size>;
  using ifm_ptr_t = ifm_dtype __aie_dm_resource_a *__restrict;
  using ofm_ptr_t = ofm_dtype __aie_dm_resource_b *__restrict;
  static constexpr uint32_t ifm_byte_incr_size =
      vec_in_t::size() * sizeof(ifm_dtype);
  static constexpr uint32_t ofm_byte_incr_size =
      vec_out_t::size() * sizeof(ofm_dtype);
  using params_t = elementwise_unary_params_t<concrete_param_t>;

  __attribute__((noinline)) static const void setup(params_t &params,
                                                    const void *rtps) {
    uint32_t *ptr_params = (uint32_t *)&params;
    ptr_params[0] = ((const uint32_t *)rtps)[0]; // ifmsv_size
    ptr_params[1] = ((const uint32_t *)rtps)[1]; // ifm_shift, ifm_shift_biased,
                                                 // ofm_shift, ofm_shift_biased
    ptr_params[2] = ((const uint32_t *)rtps)[2]; // num_kernel_iters
    concrete_operator::setup(params, rtps);
  };

  static const void run(ifm_dtype * __restrict ifm, ofm_dtype * __restrict ofm, params_t &params) {
    concrete_operator op_impl(params);

    ifm_ptr_t p_in = (ifm_ptr_t)ifm;
    ofm_ptr_t p_out = (ofm_ptr_t)ofm;

    int iteration_count = params.ifmsv_size / concrete_operator::vec_size /
                          concrete_operator::unroll;

    std::array<vec_in_t, concrete_operator::unroll> ifm_vec_group;
    std::array<vec_out_t, concrete_operator::unroll> ofm_vec_group;
    if
      constexpr(concrete_operator::peel) {
        unroll_body<concrete_operator::unroll>::run(
            op_impl, ifm_vec_group, ofm_vec_group, params, p_in, p_out);
        iteration_count--;
      }
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(concrete_operator::minimum_num_iter, ) 
    for (int i = 0; i < iteration_count; i = i + 1) {
        unroll_body<concrete_operator::unroll>::run(
            op_impl, ifm_vec_group, ofm_vec_group, params, p_in, p_out);
      }
  }

private:
  template <uint8_t N, uint8_t C = N> struct unroll_body {
    inline __attribute__((always_inline)) static void
    run(concrete_operator &op_impl, std::array<vec_in_t, N> &ifm_vec_group,
        std::array<vec_out_t, N> &ofm_vec_group, params_t &params,
        ifm_ptr_t &p_in, ofm_ptr_t &p_out) {
      // load
      std::get<N - C>(ifm_vec_group).load(p_in);
      p_in = byte_incr(p_in, ifm_byte_incr_size);
      // continue unroll
      unroll_body<N, C - 1>::run(op_impl, ifm_vec_group, ofm_vec_group, params,
                                 p_in, p_out);
      // store
      aie::store_v(p_out, std::get<C - 1>(ofm_vec_group));
      p_out = byte_incr(p_out, ofm_byte_incr_size);
    }
  };

  template <uint8_t N> struct unroll_body<N, 1> {
    inline __attribute__((always_inline)) static void
    run(concrete_operator &op_impl, std::array<vec_in_t, N> &ifm_vec_group,
        std::array<vec_out_t, N> &ofm_vec_group, params_t &params,
        ifm_ptr_t &p_in, ofm_ptr_t &p_out) {
      // load
      std::get<N - 1>(ifm_vec_group).load(p_in);
      p_in = byte_incr(p_in, ifm_byte_incr_size);
      // calculation
      dup_body<N>::run(op_impl, ifm_vec_group, ofm_vec_group, params);
      // store
      aie::store_v(p_out, std::get<0>(ofm_vec_group));
      p_out = byte_incr(p_out, ofm_byte_incr_size);
    }
  };

  template <uint8_t N, uint8_t C = N> struct dup_body {
    inline __attribute__((always_inline)) static void
    run(concrete_operator &op_impl,
        const std::array<vec_in_t, N> &ifm_vec_group,
        std::array<vec_out_t, N> &ofm_vec_group, params_t &params) {
      op_impl.body(std::get<N - C>(ifm_vec_group),
                   std::get<N - C>(ofm_vec_group), params);
      dup_body<N, C - 1>::run(op_impl, ifm_vec_group, ofm_vec_group, params);
    }
  };

  template <uint8_t N> struct dup_body<N, 1> {
    inline __attribute__((always_inline)) static void
    run(concrete_operator &op_impl,
        const std::array<vec_in_t, N> &ifm_vec_group,
        std::array<vec_out_t, N> &ofm_vec_group, params_t &params) {
      op_impl.body(std::get<N - 1>(ifm_vec_group),
                   std::get<N - 1>(ofm_vec_group), params);
    }
  };
};

// implementation of requantize<int16_t, int8_t>
template <>
class elementwise_requantize<int16_t, int8_t>
    : elementwise_unary_base<int8_t, requantize_params_t<int8_t>> {

public:
  static constexpr bool peel = requantize_config<int8_t>::peel;
  static constexpr uint8_t unroll = requantize_config<int8_t>::unroll;
  static constexpr uint16_t minimum_num_iter =
      requantize_config<int8_t>::minimum_num_iter;
  static constexpr uint16_t vec_size = requantize_config<int8_t>::vec_size;

  using vec_in_t = aie::vector<int16_t, vec_size>;
  using vec_out_t = aie::vector<int8_t, vec_size>;
  using params_t = elementwise_unary_params_t<requantize_params_t<int8_t>>;

  elementwise_requantize(const params_t &params) {
  } // setup auxiliary member variable if needed

  __attribute__((always_inline)) static void setup(params_t &params,
                                                   const void *rtps) {

    const int32_t *rtp = (const int32_t *)rtps;
    params.op_params.ifmsv_size = rtp[0];
    params.op_params.shift_out = rtp[1];
    params.op_params.num_kernel_iters = rtp[2];
    params.op_params.multiplier = rtp[3];
    params.op_params.quant_bits = rtp[4];
    params.op_params.in_zero_point = rtp[5];
    params.op_params.out_zero_point = rtp[6];
    params.op_params.q_add_shift =
        params.op_params.quant_bits + params.op_params.shift_out;

  }

  inline __attribute__((always_inline)) void
  body(const vec_in_t &in, vec_out_t &out, params_t &params) {
    out = vrequantize<vec_size>(in, params);
  }

private:
  template <int VecLen = 16>
  inline __attribute__((always_inline)) aie::vector<int8_t, VecLen>
  vrequantize(const aie::vector<int16_t, VecLen> &vec_in, params_t &params) {
    aie::accum<acc64, VecLen> round_acc = aie::zeros<acc64, VecLen>();
    aie::accum<acc32, VecLen> res_acc;
    aie::vector<int16_t, VecLen> vec_sub =
        aie::sub(vec_in, (int16_t)params.op_params.in_zero_point);
    round_acc = mac(round_acc, vec_sub, params.op_params.multiplier);
    res_acc.from_vector(
        round_acc.template to_vector_sign<int32_t>(1, params.op_params.q_add_shift));
    res_acc = aie::add(res_acc, params.op_params.out_zero_point);
    aie::vector<int8_t, VecLen> vec_out =
        res_acc.template to_vector_sign<int8_t>(1, 0);
    return vec_out;
  }
};
