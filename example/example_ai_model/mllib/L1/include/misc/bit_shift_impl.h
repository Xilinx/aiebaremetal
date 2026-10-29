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
#include "bit_shift_params.h"

namespace {
struct bit_shift_config
{
    static constexpr bool peel = false;
    static constexpr uint16_t loop_unroll = 2;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 32;
};

using namespace aie;

} // anonymous namespace


template <typename dtype, class concrete_param_t>
class custom_body_binary_base : public elementwise_binary_base<dtype, concrete_param_t> {
   public:
    static constexpr bool peel = false;
    static constexpr uint16_t loop_unroll = 1;
    static constexpr uint16_t minimum_num_iter = 1;
    static constexpr uint16_t vec_size = elementwise_trait<dtype>::vec_size;
    using acc_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, accfloat, acc32>;
    using vec_t = aie::vector<dtype, vec_size>;
    using params_t = elementwise_binary_params_t<concrete_param_t>;
    INLINE static bool loop_condition(params_t& op_params) {}
};


template <typename dtype,
          class concrete_operator,
          class concrete_param_t>
    requires(std::is_base_of_v<custom_body_binary_base<dtype, concrete_param_t>, concrete_operator>)
class custom_body_binary: public elementwise_binary<dtype, concrete_operator, concrete_param_t> {
  public:
    using vec_t = aie::vector<dtype, concrete_operator::vec_size>;
    using ifm1_ptr_t = dtype __aie_dm_resource_a* __restrict;
    using ifm2_ptr_t = dtype __aie_dm_resource_b* __restrict;
    using ofm_ptr_t = dtype __aie_dm_resource_c* __restrict;
    static constexpr uint32_t byte_incr_size = vec_t::size() * sizeof(dtype);
    using params_t = elementwise_binary_params_t<concrete_param_t>;
    
    __attribute__((noinline)) static const void run(dtype* __restrict ifm1,
                                                    dtype* __restrict ifm2,
                                                    dtype* __restrict ofm,
                                                    params_t& params) {
        concrete_operator op_impl(params);

        ifm1_ptr_t p_in1 = (ifm1_ptr_t)ifm1;
        ifm2_ptr_t p_in2 = (ifm2_ptr_t)ifm2;
        ofm_ptr_t p_out = (ofm_ptr_t)ofm;

        int iteration_count = params.ifmsv_size / concrete_operator::vec_size;
        
        vec_t ifm1_vec;
        vec_t ifm2_vec;
        vec_t ofm_vec;

        if (op_impl.loop_condition(params)) {
            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(concrete_operator::minimum_num_iter, ) 
            AIE_LOOP_UNROLL(concrete_operator::loop_unroll) 
            for (int i = 0; i < iteration_count; i = i + 1){
                ifm1_vec.load(p_in1); p_in1 = byte_incr(p_in1, byte_incr_size);
                ifm2_vec.load(p_in2); p_in2 = byte_incr(p_in2, byte_incr_size);
                op_impl.body1(ifm1_vec, ifm2_vec, ofm_vec, params);    
                aie::store_v(p_out, ofm_vec); p_out = byte_incr(p_out, byte_incr_size);
            }
        } else {
            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(concrete_operator::minimum_num_iter, ) 
            AIE_LOOP_UNROLL(concrete_operator::loop_unroll) 
            for (int i = 0; i < iteration_count; i = i + 1){
                ifm1_vec.load(p_in1); p_in1 = byte_incr(p_in1, byte_incr_size);
                ifm2_vec.load(p_in2); p_in2 = byte_incr(p_in2, byte_incr_size);
                op_impl.body2(ifm1_vec, ifm2_vec, ofm_vec, params);    
                aie::store_v(p_out, ofm_vec); p_out = byte_incr(p_out, byte_incr_size);
            }
        }
    }
};


template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, uint8>)
class bit_shift_impl : public custom_body_binary_base<dtype, bit_shift_params_t<dtype>> {
   public:
    static constexpr bool peel = bit_shift_config::peel;
    static constexpr uint16_t loop_unroll = bit_shift_config::loop_unroll;
    static constexpr uint16_t minimum_num_iter = bit_shift_config::minimum_num_iter;
    static constexpr uint16_t vec_size = bit_shift_config::vec_size;

    using vec_t = aie::vector<dtype, vec_size>;
    using params_t = elementwise_binary_params_t<bit_shift_params_t<dtype>>;

   public:
    bit_shift_impl(const params_t& op_params) {
        tile::current().set_saturation(saturation_mode::none);
        tile::current().set_rounding(rounding_mode::floor);
    }

    INLINE static bool loop_condition(params_t& op_params) {
        return op_params.op_params.direction == 0;
    }

    INLINE void body1(const vec_t& in_vec1,
                      const vec_t& in_vec2,
                      vec_t& out_vec,
                      params_t& op_params) 
    {
        out_vec = in_vec1;
        // CR-1191502
        out_vec = select(upshift(out_vec, 4), out_vec, eq((dtype)0, bit_and((dtype)4, in_vec2)));
        out_vec = select(upshift(out_vec, 2), out_vec, eq((dtype)0, bit_and((dtype)2, in_vec2)));
        out_vec = select(upshift(out_vec, 1), out_vec, eq((dtype)0, bit_and((dtype)1, in_vec2)));
        out_vec = select(out_vec, (dtype)0, ge(in_vec2, (dtype)8));
    }
    INLINE void body2(const vec_t& in_vec1,
                      const vec_t& in_vec2,
                      vec_t& out_vec,
                      params_t& op_params) 
    {
        out_vec = in_vec1;
        // CR-1191502
        out_vec = select(downshift(out_vec, 4), out_vec, eq((dtype)0, bit_and((dtype)4, in_vec2)));
        out_vec = select(downshift(out_vec, 2), out_vec, eq((dtype)0, bit_and((dtype)2, in_vec2)));
        out_vec = select(downshift(out_vec, 1), out_vec, eq((dtype)0, bit_and((dtype)1, in_vec2)));
        out_vec = select(out_vec, (dtype)0, ge(in_vec2, (dtype)8));
    }
    inline __attribute__((always_inline)) static void setup(params_t& param, const void* rtps) {
        const uint8_t* rtp_uint8 = (const uint8_t*) (rtps);
        param.op_params.direction = *(rtp_uint8 + 32);
    }
   private:
};



