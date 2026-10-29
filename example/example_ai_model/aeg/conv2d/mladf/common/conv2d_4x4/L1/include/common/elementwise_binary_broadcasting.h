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
#include "utils.h"
#include "templatized_traits.h"

template <typename dtype,
          class concrete_operator,
          class concrete_param_t>
    requires(std::is_base_of_v<elementwise_binary_base<dtype, concrete_param_t>, concrete_operator>)
class elementwise_binary_broadcasting : 
public elementwise_binary<dtype, concrete_operator, concrete_param_t>{
   public:
    using vec_t = aie::vector<dtype, concrete_operator::vec_size>;
    using ifm1_ptr_t = dtype __aie_dm_resource_a* __restrict;
    using ifm2_ptr_t = dtype __aie_dm_resource_b* __restrict;
    using ofm_ptr_t = dtype __aie_dm_resource_c* __restrict;
    static constexpr uint32_t byte_incr_size = vec_t::size() * sizeof(dtype);
    using params_t = elementwise_binary_params_t<concrete_param_t>;
    using elementwise_binary_t = elementwise_binary<dtype, concrete_operator, concrete_param_t>;

    __attribute__((noinline)) static const void setup(params_t& params, const void* rtps) {
        elementwise_binary_t::setup(params, rtps);
        setup(params);
    };

    __attribute__((noinline)) static const void setup(params_t& params) {
        params.lhs_inc = (params.scalar_position == 0) ? 0 : byte_incr_size;
        params.rhs_inc = (params.scalar_position == 1) ? 0 : byte_incr_size;
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

        if (params.scalar_position == 0){
            vec_t sca_vec = aie::broadcast<dtype, concrete_operator::vec_size>(*p_in1);
            aie::store_v(p_in1, sca_vec);// write back to ifm
        }else{
            vec_t sca_vec = aie::broadcast<dtype, concrete_operator::vec_size>(*p_in2);
            aie::store_v(p_in2, sca_vec);
        }

        if constexpr (concrete_operator::peel) {
            elementwise_binary_broadcasting::template unroll_body<concrete_operator::unroll>::run(op_impl, ifm1_vec_group, ifm2_vec_group, ofm_vec_group,
                                                        params, p_in1, p_in2, p_out);
            iteration_count--;
        }
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(concrete_operator::minimum_num_iter, ) 
        for (int i = 0; i < iteration_count; i = i + 1){
                elementwise_binary_broadcasting::template unroll_body<concrete_operator::unroll>::run(op_impl, ifm1_vec_group, ifm2_vec_group, ofm_vec_group,
                                                            params, p_in1, p_in2, p_out);
            } 
    }

};
