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
#include "sqrt_params.h"

// User configuration for design optimization
template <typename dtype> struct sqrt_config
{
};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct sqrt_config<int8_t>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 4;
    static constexpr uint16_t vec_size = 32;
};
template <> struct sqrt_config<bfloat16>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 4;
    static constexpr uint16_t vec_size = 16;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct sqrt_config<int8_t>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 4;
    static constexpr uint16_t vec_size = 32;
};
template <> struct sqrt_config<bfloat16>
{
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 4;
    static constexpr uint16_t vec_size = 32;
};
#endif                                         // AIE_ARCH

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_sqrt : elementwise_unary_base<dtype, sqrt_params_t<dtype>>
{
  public:
    static constexpr bool peel = sqrt_config<dtype>::peel;
    static constexpr uint8_t unroll = sqrt_config<dtype>::unroll;
    static constexpr uint16_t minimum_num_iter = sqrt_config<dtype>::minimum_num_iter;
    static constexpr uint16_t vec_size = sqrt_config<dtype>::vec_size;

    using acc_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, accfloat, acc32>;
    using vec_t = aie::vector<dtype, vec_size>;
    using params_t = elementwise_unary_params_t<sqrt_params_t<dtype>>;

    elementwise_sqrt(const params_t &op_params)
    {
    } // setup auxiliary member variable if needed

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps)
    {
    }

    inline __attribute__((always_inline)) void body(const vec_t &in, vec_t &out, params_t &op_params)
    {
        out = vsqrt<vec_size>(in, op_params);
    }

  private:
    template <int VecLen>
    inline __attribute__((always_inline)) aie::vector<dtype, VecLen> vsqrt(const vec_t &vec_in, params_t &op_params)
    {
        aie::vector<dtype, VecLen> vec_out;
        aie::vector<bfloat16, VecLen> vec_out_bf16, vec_out_bf16_0;
        aie::vector<bfloat16, VecLen> vec_in_bf16;
        accum<accfloat, VecLen> acc;

        aie::vector<bfloat16, sqrt_config<dtype>::vec_size> threeHalf = aie::broadcast<bfloat16, vec_size>(1.5f);
        aie::vector<int16, sqrt_config<dtype>::vec_size> magics = aie::broadcast<int16, vec_size>(0x5F37);
        aie::vector<bfloat16, sqrt_config<dtype>::vec_size> vec_half = aie::broadcast<bfloat16, vec_size>(0.5f);
        aie::vector<int16, sqrt_config<dtype>::vec_size> mask_subnormal = aie::broadcast<int16, vec_size>(0x007F);
        // NaN
        aie::vector<bfloat16, sqrt_config<dtype>::vec_size> nan_v = aie::bit_or(int16(0x7FC0), vec_half.template cast_to<int16>()).template cast_to<bfloat16>();
        // 2^128-1
        aie::vector<int16, sqrt_config<dtype>::vec_size> pow_2_128 = aie::broadcast<int16, vec_size>(0x7F7F);

        if constexpr(!std::is_same<bfloat16, dtype>::value) {
            acc.template from_vector(aie::to_float(vec_in, op_params.ifm_shift));
            vec_in_bf16 = acc.template to_vector<bfloat16>();
        } else {
            vec_in_bf16 = vec_in;
        }

        // mask negative value
        aie::mask<VecLen> mask_nan = aie::ge(vec_in_bf16, bfloat16(0.0));

        // Step 1: Extract exponent part and fraction part of vec_in_bf16
        aie::vector<int16, VecLen> exponent = aie::downshift(aie::bit_and(int16(0x7F00), vec_in_bf16.template cast_to<int16>()), 1);
        // fraction branch 1: exponent bits not all zero
        aie::vector<int16, VecLen> fraction = aie::bit_and(int16(0x00FF), vec_in_bf16.template cast_to<int16>());
        fraction = aie::bit_or(int16(0x4000), fraction);
        // fraction branch 2: exponent bits all zero
        aie::vector<bfloat16, VecLen> fraction_part = aie::mul(vec_in_bf16, pow_2_128.template cast_to<bfloat16>());
        // mask to distinguish two branch
        aie::mask<VecLen> mask_exponent_all_zero = aie::gt(vec_in_bf16, mask_subnormal.template cast_to<bfloat16>()); // greater than 0000 0000 0111 1111
        // choose branch result from mask to get final fraction part
        fraction_part = aie::select(fraction_part, fraction.template cast_to<bfloat16>(), mask_exponent_all_zero);

        // Step 2: Calulate square root using Fast-Inverse-Square-Root algorithm for fraction part of vec_in_bf16
        // 1st iteration in Newton's method
        aie::vector<bfloat16, VecLen> y = aie::sub(magics, aie::downshift(fraction_part.template cast_to<int16>(), 1)).template cast_to<bfloat16>();
		aie::accum<accfloat, VecLen> x1 = aie::mul(fraction_part, vec_half);		                            // x1 = input * 0.5F;
		aie::accum<accfloat, VecLen> x2 = aie::mul(x1.template to_vector<bfloat16>(), y);	                    // x1 * y
		x2 = aie::mul(x2.template to_vector<bfloat16>(), y);						                            // x1 * y * y
		aie::vector<bfloat16, VecLen> t = aie::sub(threeHalf, x2.template to_vector<bfloat16>());  		        // threehalfs - (x1 * y * y)
		x2 = aie::mul(t, y);									         			                            // Inverse Sqrt Root: x2 = y * (threehalfs - (x1 * y * y))

        // 2nd iteration in Newton's method
		aie::accum<accfloat, VecLen> x3 = aie::mul(x1.template to_vector<bfloat16>(), x2.template to_vector<bfloat16>());	// x1 * x2
		x3 = aie::mul(x3.template to_vector<bfloat16>(), x2.template to_vector<bfloat16>());	                // x1 * x2 * x2
		t = aie::sub(threeHalf, x3.template to_vector<bfloat16>());  		                                    // threehalfs - (x1 * x2 * x2)
		x3 = aie::mul(t, x2.template to_vector<bfloat16>());                                                    // Inverse Sqrt Root: x3 = x2 * (threehalfs - (x1 * x2 * x2))

        // Inverse Square root -> Square root
		x3 = aie::mul(x3.template to_vector<bfloat16>(), fraction_part);                                        // Square Root: Inverse Sqrt Root * input
        vec_out_bf16 = x3.template to_vector<bfloat16>();

        // Step 3: Calculate exponent part of vec_in_bf16
        vec_out_bf16_0 = aie::add(exponent, int16(0x1F80)).template cast_to<bfloat16>() ;                       // int add operation : exponent - 64 + 127 --> exponent + 63

        // Step 4: Calculate vec_out_bf16 by multiply fraction part and exponent part
		vec_out_bf16 = aie::mul(vec_out_bf16, vec_out_bf16_0);                                                  // bfloat16 mul operation : fraction part * exponent part

        if constexpr(!std::is_same<bfloat16, dtype>::value) {
            vec_out = aie::to_fixed<int8_t>(vec_out_bf16, -op_params.ofm_shift);
            int8_t neg_inf = int8_t(0x80);
            // convert negative values to -128
            vec_out = aie::select(neg_inf, vec_out, mask_nan);
        } else {
            // convert negative values to NaN
            vec_out_bf16 = aie::select(nan_v, vec_out_bf16, mask_nan);
            vec_out = vec_out_bf16;
        }
        return vec_out;
    }
};
