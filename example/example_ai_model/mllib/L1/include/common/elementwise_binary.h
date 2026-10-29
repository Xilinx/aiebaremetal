
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

#include <array>
#include <type_traits>
#include <typeinfo>

#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"
#include "utils.h"
#include "templatized_traits.h"
// Elementwise Binary Operator's RTP goes through AXIMM
template <class concrete_param_t>
// total 64 bytes rtps, last 32 bytes are designed for concrete params
struct alignas(v32int16) elementwise_binary_params_t {
    // uint32
    uint32_t ifmsv_size;
    // uint32
    int8_t ifm1_shift;
    uint8_t ifm1_shift_biased;
    int8_t ifm2_shift;
    uint8_t ifm2_shift_biased;
    // uint32
    int8_t ofm_shift;
    int8_t ofm_shift_biased;
    uint16_t num_kernel_iters; // used by superkernel to setup params only conce, not used by kernel itself.
    // uint32
    uint8_t scalar_position;
    uint8_t reserved3_;
    uint16_t scalar; // used by attribute broadcasting, as input scalar
    // reserved
    uint32_t reserved2_;
    uint32_t reserved1_;
    uint32_t reserved0_;
    concrete_param_t op_params;
    // skeleton-wise derived parameters  
    int32_t lhs_inc; // use sign int incase we need a reversed traversal pattern in the future
    int32_t rhs_inc;
};

template <typename dtype, class concrete_param_t>
class elementwise_binary_base {
   public:
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 1;
    static constexpr uint16_t vec_size = elementwise_trait<dtype>::vec_size;
    using acc_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, accfloat, acc32>;
    using vec_t = aie::vector<dtype, vec_size>;
    using params_t = elementwise_binary_params_t<concrete_param_t>;
   public:
    elementwise_binary_base() = default;
    elementwise_binary_base(const params_t& op_param){};
    elementwise_binary_base(const elementwise_binary_base&) = delete;
    elementwise_binary_base& operator=(const elementwise_binary_base&) = delete;
    // implement following 2 inferface
    inline __attribute__((always_inline)) void body(const vec_t& in_vec1,
                                                    const vec_t& in_vec2,
                                                    vec_t& out_vec,
                                                    params_t& op_params) {}

    inline __attribute__((always_inline)) static void setup(params_t& op_params, const void* rtps) {}
};

template <typename dtype,
          class concrete_operator,
          class concrete_param_t>
    requires(std::is_base_of_v<elementwise_binary_base<dtype, concrete_param_t>, concrete_operator>)
class elementwise_binary {
   public:
    using vec_t = aie::vector<dtype, concrete_operator::vec_size>;
    using ifm1_ptr_t = dtype __aie_dm_resource_a* __restrict;
    using ifm2_ptr_t = dtype __aie_dm_resource_b* __restrict;
    using ofm_ptr_t = dtype __aie_dm_resource_c* __restrict;
    static constexpr uint32_t byte_incr_size = vec_t::size() * sizeof(dtype);
    using params_t = elementwise_binary_params_t<concrete_param_t>;
    __attribute__((noinline)) static const void setup(params_t& params, const void* rtps) {
        const uint32_t* rtp_uint32 = (const uint32_t*) (rtps);
        uint32_t* params_ptr = (uint32_t*) &params;
        *params_ptr++ = *rtp_uint32++; // ifmsv_size
        *params_ptr++ = *rtp_uint32++; // ifm1_shift, ifm1_shift_biased, ifm2_shift, ifm2_shift_biased
        *params_ptr++ = *rtp_uint32++; // ofm_shift, ofm_shift_biased, num_kernel_iters, scalar_position
        *params_ptr++ = *rtp_uint32++; // scalar        
        setup(params); //setup step size of pointers to ifm1 and ifm2
        concrete_operator::setup(params, rtps);
    };

    __attribute__((noinline)) static const void setup(params_t& params) {
        params.lhs_inc = byte_incr_size;
        params.rhs_inc = byte_incr_size;
    };

    __attribute__((noinline)) static const void run(dtype* __restrict ifm1,
                                                    dtype* __restrict ifm2,
                                                    dtype* __restrict ofm,
                                                    params_t& params) {
        concrete_operator op_impl(params);

        ifm1_ptr_t p_in1 = (ifm1_ptr_t)ifm1;
        ifm2_ptr_t p_in2 = (ifm2_ptr_t)ifm2;
        ofm_ptr_t p_out = (ofm_ptr_t)ofm;

        int iteration_count = params.ifmsv_size / concrete_operator::vec_size / concrete_operator::unroll;

        std::array<vec_t, concrete_operator::unroll> ifm1_vec_group;
        std::array<vec_t, concrete_operator::unroll> ifm2_vec_group;
        std::array<vec_t, concrete_operator::unroll> ofm_vec_group;
        if constexpr (concrete_operator::peel) {
            unroll_body<concrete_operator::unroll>::run(op_impl, ifm1_vec_group, ifm2_vec_group, ofm_vec_group,
                                                        params, p_in1, p_in2, p_out);
            iteration_count--;
        }
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(concrete_operator::minimum_num_iter, ) 
        for (int i = 0; i < iteration_count; i = i + 1){
                unroll_body<concrete_operator::unroll>::run(op_impl, ifm1_vec_group, ifm2_vec_group, ofm_vec_group,
                                                            params, p_in1, p_in2, p_out);
            }
    }

   protected:
    template <uint8_t N, uint8_t C = N>
    struct unroll_body {
        inline __attribute__((always_inline)) static void run(concrete_operator& op_impl,
                                                              std::array<vec_t, N>& ifm1_vec_group,
                                                              std::array<vec_t, N>& ifm2_vec_group,
                                                              std::array<vec_t, N>& ofm_vec_group,
                                                              params_t& params,
                                                              ifm1_ptr_t& p_in1,
                                                              ifm2_ptr_t& p_in2,
                                                              ofm_ptr_t& p_out) {
            // load
            std::get<N - C>(ifm1_vec_group).load(p_in1);
            p_in1 = byte_incr(p_in1, params.lhs_inc);
            std::get<N - C>(ifm2_vec_group).load(p_in2);
            p_in2 = byte_incr(p_in2, params.rhs_inc);
            // continue unroll
            unroll_body<N, C - 1>::run(op_impl, ifm1_vec_group, ifm2_vec_group, ofm_vec_group, params, p_in1, p_in2,
                                       p_out);
            // store
            aie::store_v(p_out, std::get<C - 1>(ofm_vec_group));
            p_out = byte_incr(p_out, byte_incr_size);
        }
    };

    template <uint8_t N>
    struct unroll_body<N, 1> {
        inline __attribute__((always_inline)) static void run(concrete_operator& op_impl,
                                                              std::array<vec_t, N>& ifm1_vec_group,
                                                              std::array<vec_t, N>& ifm2_vec_group,
                                                              std::array<vec_t, N>& ofm_vec_group,
                                                              params_t& params,
                                                              ifm1_ptr_t& p_in1,
                                                              ifm2_ptr_t& p_in2,
                                                              ofm_ptr_t& p_out) {
            // load
            std::get<N - 1>(ifm1_vec_group).load(p_in1);
            p_in1 = byte_incr(p_in1, params.lhs_inc);
            std::get<N - 1>(ifm2_vec_group).load(p_in2);
            p_in2 = byte_incr(p_in2, params.rhs_inc);
            // calculation
            dup_body<N>::run(op_impl, ifm1_vec_group, ifm2_vec_group, ofm_vec_group, params);
            // store
            aie::store_v(p_out, std::get<0>(ofm_vec_group));
            p_out = byte_incr(p_out, byte_incr_size);
        }
    };

    template <uint8_t N, uint8_t C = N>
    struct dup_body {
        inline __attribute__((always_inline)) static void run(concrete_operator& op_impl,
                                                              const std::array<vec_t, N>& ifm1_vec_group,
                                                              const std::array<vec_t, N>& ifm2_vec_group,
                                                              std::array<vec_t, N>& ofm_vec_group,
                                                              params_t& params) {
            op_impl.body(std::get<N - C>(ifm1_vec_group), std::get<N - C>(ifm2_vec_group),
                         std::get<N - C>(ofm_vec_group), params);
            dup_body<N, C - 1>::run(op_impl, ifm1_vec_group, ifm2_vec_group, ofm_vec_group, params);
        }
    };

    template <uint8_t N>
    struct dup_body<N, 1> {
        inline __attribute__((always_inline)) static void run(concrete_operator& op_impl,
                                                              const std::array<vec_t, N>& ifm1_vec_group,
                                                              const std::array<vec_t, N>& ifm2_vec_group,
                                                              std::array<vec_t, N>& ofm_vec_group,
                                                              params_t& params) {
            op_impl.body(std::get<N - 1>(ifm1_vec_group), std::get<N - 1>(ifm2_vec_group),
                         std::get<N - 1>(ofm_vec_group), params);
        }
    };
};
