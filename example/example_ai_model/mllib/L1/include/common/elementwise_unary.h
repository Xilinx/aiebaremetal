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

template <class concrete_param_t> struct alignas(v32int16) elementwise_unary_params_t
{
    // uint32
    uint32_t ifmsv_size;
    // uint32
    int8_t ifm_shift;
    uint8_t ifm_shift_biased;
    int8_t ofm_shift;
    uint8_t ofm_shift_biased;
    // uint32
    uint16_t num_kernel_iters; // used by superkernel to setup params only conce, not used by kernel itself.
    uint16_t reserved2_;
    // reserved
    uint32_t reserved3_;
    uint32_t reserved4_;
    uint32_t reserved5_;
    uint32_t reserved6_;
    uint32_t reserved7_;
    concrete_param_t op_params;
};

template <typename dtype, class concrete_param_t> class elementwise_unary_base
{
  public:
    // XXX: this is default configuration and it's the most conservative way,
    // op implementers should tune these configurations for their benefit
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 1;
    static constexpr uint16_t vec_size = elementwise_trait<dtype>::vec_size;
    using acc_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, accfloat, acc32>;
    using vec_t = aie::vector<dtype, vec_size>;
    using params_t = elementwise_unary_params_t<concrete_param_t>;

    elementwise_unary_base() = default;
    elementwise_unary_base(const params_t &op_param){};
    elementwise_unary_base(const elementwise_unary_base &) = delete;
    elementwise_unary_base &operator=(const elementwise_unary_base &) = delete;
    // implement following 2 inferface
    inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params)
    {
    }

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps)
    {
    }
};

template <typename dtype, class concrete_operator, class concrete_param_t>
    requires(std::is_base_of_v<elementwise_unary_base<dtype, concrete_param_t>, concrete_operator>)
class elementwise_unary
{
  public:
    using vec_t = aie::vector<dtype, concrete_operator::vec_size>;
    using ifm_ptr_t = dtype __aie_dm_resource_a *__restrict;
    using ofm_ptr_t = dtype __aie_dm_resource_b *__restrict;
    static constexpr uint32_t byte_incr_size = vec_t::size() * sizeof(dtype);
    using params_t = elementwise_unary_params_t<concrete_param_t>;

    __attribute__((noinline)) static const void setup(params_t &params,
                                                      const void *rtps)
    {
        uint32_t* ptr_params = (uint32_t*) &params;
        ptr_params[0] = ((const uint32_t*) rtps)[0]; // ifmsv_size
        ptr_params[1] = ((const uint32_t*) rtps)[1]; // ifm_shift, ifm_shift_biased, ofm_shift, ofm_shift_biased
        ptr_params[2] = ((const uint32_t*) rtps)[2]; // num_kernel_iters
        concrete_operator::setup(params, rtps);
    };

    __attribute__((noinline)) __attribute__((no_builtin("memcpy")))
    static const void run(dtype* __restrict ifm,
                          dtype* __restrict ofm,
                          params_t &params)
    {
        concrete_operator op_impl(params);

        ifm_ptr_t p_in = (ifm_ptr_t)ifm;
        ofm_ptr_t p_out = (ofm_ptr_t)ofm;

        int iteration_count = params.ifmsv_size / concrete_operator::vec_size / concrete_operator::unroll;

        std::array<vec_t, concrete_operator::unroll> ifm_vec_group;
        std::array<vec_t, concrete_operator::unroll> ofm_vec_group;
        if constexpr (concrete_operator::peel)
        {
            unroll_body<concrete_operator::unroll>::run(op_impl, ifm_vec_group, ofm_vec_group, params, p_in,
                                                        p_out);
            iteration_count--;
        }
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(concrete_operator::minimum_num_iter, ) 
        for (int i = 0; i < iteration_count; i = i + 1){
                unroll_body<concrete_operator::unroll>::run(op_impl, ifm_vec_group, ofm_vec_group, params,
                                                            p_in, p_out);
            }
    }

  private:
    template <uint8_t N, uint8_t C = N> struct unroll_body
    {
        inline __attribute__((always_inline)) static void run(concrete_operator &op_impl,
                                                              std::array<vec_t, N> &ifm_vec_group,
                                                              std::array<vec_t, N> &ofm_vec_group,
                                                              params_t &params, ifm_ptr_t &p_in,
                                                              ofm_ptr_t &p_out)
        {
            // load
            std::get<N - C>(ifm_vec_group).load(p_in);
            p_in = byte_incr(p_in, byte_incr_size);
            // continue unroll
            unroll_body<N, C - 1>::run(op_impl, ifm_vec_group, ofm_vec_group, params, p_in, p_out);
            // store
            aie::store_v(p_out, std::get<C - 1>(ofm_vec_group));
            p_out = byte_incr(p_out, byte_incr_size);
        }
    };

    template <uint8_t N> struct unroll_body<N, 1>
    {
        inline __attribute__((always_inline)) static void run(concrete_operator &op_impl,
                                                              std::array<vec_t, N> &ifm_vec_group,
                                                              std::array<vec_t, N> &ofm_vec_group,
                                                              params_t &params, ifm_ptr_t &p_in,
                                                              ofm_ptr_t &p_out)
        {
            // load
            std::get<N - 1>(ifm_vec_group).load(p_in);
            p_in = byte_incr(p_in, byte_incr_size);
            // calculation
            dup_body<N>::run(op_impl, ifm_vec_group, ofm_vec_group, params);
            // store
            aie::store_v(p_out, std::get<0>(ofm_vec_group));
            p_out = byte_incr(p_out, byte_incr_size);
        }
    };

    template <uint8_t N, uint8_t C = N> struct dup_body
    {
        inline __attribute__((always_inline)) static void run(concrete_operator &op_impl,
                                                              const std::array<vec_t, N> &ifm_vec_group,
                                                              std::array<vec_t, N> &ofm_vec_group,
                                                              params_t &params)
        {
            op_impl.body(std::get<N - C>(ifm_vec_group), std::get<N - C>(ofm_vec_group), params);
            dup_body<N, C - 1>::run(op_impl, ifm_vec_group, ofm_vec_group, params);
        }
    };

    template <uint8_t N> struct dup_body<N, 1>
    {
        inline __attribute__((always_inline)) static void run(concrete_operator &op_impl,
                                                              const std::array<vec_t, N> &ifm_vec_group,
                                                              std::array<vec_t, N> &ofm_vec_group,
                                                              params_t &params)
        {
            op_impl.body(std::get<N - 1>(ifm_vec_group), std::get<N - 1>(ofm_vec_group), params);
        }
    };
};
