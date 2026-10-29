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

#include <adf.h>
#include <aie.hpp>
#include <aie_adf.hpp>
#include <utils.hpp>

#include "elementwise_unary.h"
#include "log_lut_f32.h"
#include "pow_lut.h"
#include "pow_params.h"

// User configuration for design optimization
template <typename dtype>
struct pow_config {};

#if __AIE_ARCH__ == 20  // AIE2
template <>
struct pow_config<int8_t> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 32;
};
template <>
struct pow_config<bfloat16> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 32;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22  // AIE2p and AIE2ps
template <>
struct pow_config<int8_t> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 32;
};
template <>
struct pow_config<bfloat16> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 8;
    static constexpr uint16_t vec_size = 32;
};
#endif                                          // AIE_ARCH

template <typename dtype>
class elementwise_pow {};

// ====================================================================
// implementation of pow<int8_t>
template <>
class elementwise_pow<int8_t> : elementwise_unary_base<int8_t, pow_templated_params_t<int8_t>> {
};

template<>
class elementwise_unary<int8_t, elementwise_pow<int8_t>, pow_templated_params_t<int8_t>> {
    public:
        using ifm_ptr_t = int8_t __aie_dm_resource_a *__restrict;
        using ofm_ptr_t = int8_t __aie_dm_resource_b *__restrict;
        using params_t = elementwise_unary_params_t<pow_templated_params_t<int8_t>>;
        static constexpr bool peel = pow_config<int8_t>::peel;
        static constexpr uint8_t unroll = pow_config<int8_t>::unroll;
        static constexpr uint16_t minimum_num_iter = pow_config<int8_t>::minimum_num_iter;
        static constexpr uint16_t vec_size = pow_config<int8_t>::vec_size;


        __attribute__((noinline)) static const void setup(params_t &params,
                                                          const void *rtps) {
            uint32_t* ptr_params = (uint32_t*) &params;
            ptr_params[0] = ((const uint32_t*) rtps)[0]; // ifmsv_size
            ptr_params[1] = ((const uint32_t*) rtps)[1]; // ifm_shift, ifm_shift_biased, ofm_shift, ofm_shift_biased
            ptr_params[2] = ((const uint32_t*) rtps)[2]; // num_kernel_iters
            params.op_params.exponent = ((int8_t *)((int8_t*)rtps + 32))[0];
        };

        __attribute__((noinline)) static const void run(int8_t* __restrict ifm,
                                                        int8_t* __restrict ofm,
                                                        params_t &params) {
            ifm_ptr_t input = (ifm_ptr_t)ifm;
            ofm_ptr_t output = (ofm_ptr_t)ofm;
            int8_t exponent = params.op_params.exponent;
            uint16_t iteration_count = params.ifmsv_size / vec_size;

            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(4, ) 
            for (int j = 0; j < iteration_count; j++){
                vector<int8_t, vec_size> output_vec = aie::broadcast<int8_t, vec_size>(1);
                store_v((int8_t __aie_dm_resource_c *)output, output_vec);
                output = byte_incr(output, vec_size * sizeof(int8_t));
            }
            for (int i = 0; i < exponent; ++i) {
                ifm_ptr_t input0 = (ifm_ptr_t)ifm;
                ifm_ptr_t input1 = (ifm_ptr_t)ifm+vec_size;
                ifm_ptr_t input2 = (ifm_ptr_t)ifm+vec_size*2;
                ifm_ptr_t input3 = (ifm_ptr_t)ifm+vec_size*3;
                ofm_ptr_t output0 = (ofm_ptr_t)ofm;
                ofm_ptr_t output1 = (ofm_ptr_t)ofm+vec_size;
                ofm_ptr_t output2 = (ofm_ptr_t)ofm+vec_size*2;
                ofm_ptr_t output3 = (ofm_ptr_t)ofm+vec_size*3;
                AIE_PREPARE_FOR_PIPELINING 
                AIE_LOOP_RANGE(2, ) 
                for (int j = 0; j < iteration_count/4; j++){
                    vector<int8_t, vec_size> input_vec0 = load_v<vec_size>(input0);
                    vector<int8_t, vec_size> input_vec1 = load_v<vec_size>(input1);
                    vector<int8_t, vec_size> input_vec2 = load_v<vec_size>(input2);
                    vector<int8_t, vec_size> input_vec3 = load_v<vec_size>(input3);

                    vector<int8_t, vec_size> output_vec0 = load_v<vec_size>(output0);
                    vector<int8_t, vec_size> output_vec1 = load_v<vec_size>(output1);
                    vector<int8_t, vec_size> output_vec2 = load_v<vec_size>(output2);
                    vector<int8_t, vec_size> output_vec3 = load_v<vec_size>(output3);

                    accum<acc32, vec_size> output_acc0;
                    accum<acc32, vec_size> output_acc1;
                    accum<acc32, vec_size> output_acc2;
                    accum<acc32, vec_size> output_acc3;

                    output_acc0 = aie::mul(output_vec0, input_vec0);
                    output_acc1 = aie::mul(output_vec1, input_vec1);
                    output_acc2 = aie::mul(output_vec2, input_vec2);
                    output_acc3 = aie::mul(output_vec3, input_vec3);


                    // Emulation of int16 to int8 overflow
                    vector<int32_t, vec_size> new_output_0 =
                            aie::bit_and(output_acc0.to_vector<int32_t>(), aie::broadcast<int32_t, vec_size>(0x000000FF));
                    vector<int16_t, vec_size> new_output_a_0 = filter_even(new_output_0.cast_to<int16_t>());
                    vector<int8_t, vec_size> new_output_b_0 = filter_even(new_output_a_0.cast_to<int8_t>());

                    vector<int32_t, vec_size> new_output_1 =
                            aie::bit_and(output_acc1.to_vector<int32_t>(), aie::broadcast<int32_t, vec_size>(0x000000FF));
                    vector<int16_t, vec_size> new_output_a_1 = filter_even(new_output_1.cast_to<int16_t>());
                    vector<int8_t, vec_size> new_output_b_1 = filter_even(new_output_a_1.cast_to<int8_t>());

                    vector<int32_t, vec_size> new_output_2 =
                            aie::bit_and(output_acc2.to_vector<int32_t>(), aie::broadcast<int32_t, vec_size>(0x000000FF));
                    vector<int16_t, vec_size> new_output_a_2 = filter_even(new_output_2.cast_to<int16_t>());
                    vector<int8_t, vec_size> new_output_b_2 = filter_even(new_output_a_2.cast_to<int8_t>());

                    vector<int32_t, vec_size> new_output_3 =
                            aie::bit_and(output_acc3.to_vector<int32_t>(), aie::broadcast<int32_t, vec_size>(0x000000FF));
                    vector<int16_t, vec_size> new_output_a_3 = filter_even(new_output_3.cast_to<int16_t>());
                    vector<int8_t, vec_size> new_output_b_3 = filter_even(new_output_a_3.cast_to<int8_t>());


                    store_v(output0, new_output_b_0);
                    store_v(output1, new_output_b_1);
                    store_v(output2, new_output_b_2);
                    store_v(output3, new_output_b_3);

                    input0 = byte_incr(input0, 4 * vec_size * sizeof(int8_t));
                    input1 = byte_incr(input1, 4 * vec_size * sizeof(int8_t));
                    input2 = byte_incr(input2, 4 * vec_size * sizeof(int8_t));
                    input3 = byte_incr(input3, 4 * vec_size * sizeof(int8_t));
                    output0 = byte_incr(output0, 4 * vec_size * sizeof(int8_t));
                    output1 = byte_incr(output1, 4 * vec_size * sizeof(int8_t));
                    output2 = byte_incr(output2, 4 * vec_size * sizeof(int8_t));
                    output3 = byte_incr(output3, 4 * vec_size * sizeof(int8_t));
                }
            }

        }
};

// ====================================================================
// implementation of pow<bfloat16>

// Pow kernel calculation procedure:
// x^a = 2^(log2(x) * a)
// bfloat16 x = M * 2^E, which is x' binary expression
// x^a = 2^(log2(M * 2^E) * a) = 2^((ln(M) / ln2 + E) * a)
//     = 2^F * 2^I
// where I is the integer part of exponent, F is decimal part of exponent
// 2^F * 2^I = M' * 2^E', is just the binary expression of result
template <>
class elementwise_pow<bfloat16>
    : elementwise_unary_base<bfloat16, pow_templated_params_t<bfloat16>> {
  private:
    const int16_t exp_mask = 0x7f80;
    const int16_t frac_mask = 0x7F;
    const int16_t shift = 127;
    const float invc_ln2 = 1.4426950216293334960937500000000000000000;  // 1/ln2
    aie::lut<4, float, float> lut_f = aie::lut<4, float, float>(256, log_f32_lut<256>::fraction_table_ab_f32, log_f32_lut<256>::fraction_table_cd_f32);
    aie::parallel_lookup<int16_t, aie::lut<4, float, float>, aie::lut_oor_policy::truncate> lookup_fraction =
            aie::parallel_lookup<int16_t, aie::lut<4, float, float>, aie::lut_oor_policy::truncate>(lut_f);
    aie::lut<4, uint16_t, uint16_t> lut_f_exp2 = aie::lut<4, uint16_t, uint16_t>(512, exp2_lut<512>::exp2_table_ab, exp2_lut<512>::exp2_table_cd);
    aie::parallel_lookup<uint16_t, aie::lut<4, uint16_t, uint16_t>, aie::lut_oor_policy::truncate> lookup_fraction_exp2 =
            aie::parallel_lookup<uint16_t, aie::lut<4, uint16_t, uint16_t>, aie::lut_oor_policy::truncate>(lut_f_exp2);

  public:
    static constexpr bool peel = pow_config<bfloat16>::peel;
    static constexpr uint8_t unroll = pow_config<bfloat16>::unroll;
    static constexpr uint16_t minimum_num_iter = pow_config<bfloat16>::minimum_num_iter;
    static constexpr uint16_t vec_size = pow_config<bfloat16>::vec_size;
    using acc_t = aie::accum<accfloat, vec_size>;
    using vec_t = aie::vector<bfloat16, vec_size>;
    using params_t = elementwise_unary_params_t<pow_templated_params_t<bfloat16>>;
    aie::vector<int16_t, vec_size> vec_nan_int = aie::broadcast<int16_t, vec_size>(0x7fff);// nan
    vec_t vec_nan = vec_nan_int.cast_to<bfloat16>();
    vec_t vec_zero = aie::broadcast<bfloat16, vec_size>(0.0);
    aie::mask<vec_size> is_fraction, is_odd;

    elementwise_pow(const params_t &op_params) {
        int16_t exponent_bf, exponent;
        exponent_bf = *(int16_t *)(&op_params.op_params.exponent);
        exponent = (exponent_bf >> 7) - shift;
        aie::mask<vec_size> msk_true(true);    
        int16_t mask_odd = (1 << (7 - exponent));
        int16_t mask_fraction = mask_odd - 1;

        if(exponent < 7){
            if(mask_fraction & exponent_bf){
                is_fraction = msk_true;// printf("exponent is a fraction!\n");
            } else if(mask_odd & exponent_bf){
                is_odd = msk_true;// printf("exponent is a odd number!\n");
            }
        } else if(exponent == 7){
            if(mask_odd & exponent_bf){
                is_odd = msk_true;// printf("exponent is a odd number!\n");
            }
        }
    }

    __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {
        op_params.op_params.exponent = ((bfloat16 *)((int8_t*)rtps + 32))[0];
    }

    inline __attribute__((always_inline)) void body(const vec_t &in, vec_t &out,
                                                    params_t &op_params) {
        // 1. first do the calculation of ln(M)
        vector<int16_t, vec_size> input_buff = in.cast_to<int16_t>();
        vector<int16_t, vec_size> exponent;
        vector<int16_t, vec_size> fraction;
        vector<bfloat16, vec_size> output_bf16;
        accum<accfloat, vec_size> acc;
        bfloat16 exponent_f16 = op_params.op_params.exponent;

        // split exponent and fraction
        accum<acc32, vec_size> exp_acc;
        exp_acc.template from_vector(bit_and(exp_mask, input_buff));
        exponent = exp_acc.to_vector<int16_t>(7);
        exponent = sub(exponent, shift);
        fraction = bit_and(frac_mask, input_buff);
        vector<int16_t, vec_size/2> fraction_0 = fraction.extract<vec_size/2>(0);
        vector<int16_t, vec_size/2> fraction_1 = fraction.extract<vec_size/2>(1);
        vector<int16_t, vec_size/2> exponent_0 = exponent.extract<vec_size/2>(0);
        vector<int16_t, vec_size/2> exponent_1 = exponent.extract<vec_size/2>(1);

        accum<accfloat, vec_size> res0_acc;
        res0_acc.template from_vector(to_float(exponent));
        vector<bfloat16, vec_size> res0 = res0_acc.to_vector<bfloat16>();

        // frac
        vector<float, vec_size/2> mantissa_log_0 = lookup_fraction.fetch(fraction_0);
        vector<float, vec_size/2> mantissa_log_1 = lookup_fraction.fetch(fraction_1);

        vector<float, vec_size> mantissa_log = aie::concat(mantissa_log_0, mantissa_log_1);

        // 2. calculate (ln(M) / ln2 + E) * a
        acc = aie::mac(res0_acc, mantissa_log, invc_ln2);
        vector<float, vec_size> acc_float = aie::mul(acc.to_vector<float>(), float(exponent_f16));
        acc.from_vector(acc_float);

        // 3. Get I of 2^I
         // avoid a number like 19.998 cast to 20 when cast to bf16
        tile::current().set_rounding(rounding_mode::floor);
        // to_fixed only act like floor in bf16, in float it act like round
        vector<int32_t, vec_size> exponent_int = aie::to_fixed<int32_t>(acc.to_vector<bfloat16>(), 0);
        tile::current().set_rounding(rounding_mode::conv_even);
        vector<float, vec_size> exponent_f32_vec = aie::to_float<float>(exponent_int);
        exponent_int = aie::add(exponent_int, int32_t(127));
        vector<int16_t, vec_size> exponent_int16 = filter_even(exponent_int.cast_to<int16_t>());

        // 4. Get F of 2^F, and use lookup table to calculate 2^F
        exponent_f32_vec = aie::sub(acc.to_vector<float>(), exponent_f32_vec);
        acc.from_vector(exponent_f32_vec);
        vector<bfloat16, vec_size> exponent_f16_vec = acc.to_vector<bfloat16>();
        // if cast to bfloat16 overflow to 1.0, we will do carry operation to get higher accuracy
        // when exponent_f16_vec equals to zero, it means exponent_int16 need a carry
        mask<vec_size> is_carry = aie::eq(exponent_f16_vec, bfloat16(1.0));
        vector<int16_t, vec_size> carry_vec = aie::zeros<int16_t, vec_size>();
        carry_vec = aie::select(carry_vec, (int16_t)1, is_carry);
        exponent_int16 = aie::add(exponent_int16, carry_vec);
        mask<vec_size> exponent_minus_zero_mask = aie::lt(exponent_int16, int16(-127));

        vector<int16_t, vec_size> exponent_f16_vec_int16 = exponent_f16_vec.cast_to<int16_t>();
        // get the mantissa index
        exponent_f16_vec_int16 = aie::bit_and((int16_t)0x01FF, exponent_f16_vec_int16);
        // Because lookup_table only support unint16 return rather not int8, But we only need int8 return.
        // For saving storage, we combine index n and index n + 1 together, returning ((int8 << 8) | int8).
        // And we split result into int8 later
        // aie::downshift has a carry feature, but we dont need it
        vector<int16_t, vec_size> exponent_f16_vec_int16_for_downshift = aie::bit_and((int16_t)0xFFFE, exponent_f16_vec_int16);
        // original index divided by 2 to get new index
        vector<int16_t, vec_size> exponent_f16_vec_int16_index = aie::downshift(exponent_f16_vec_int16_for_downshift, 1);
        vector<int16_t, vec_size> exponent_f16_vec_int16_remainder = aie::bit_and((int16_t)0x0001, exponent_f16_vec_int16);
        mask<vec_size> mask_exponent_remainder_0 = aie::eq((int16_t)0, exponent_f16_vec_int16_remainder);
        vector<uint16_t, vec_size> result_mantissa_combine = lookup_fraction_exp2.fetch(exponent_f16_vec_int16_index);
        vector<uint8_t, vec_size> result_mantissa_combine_0 = aie::filter_even(result_mantissa_combine.cast_to<uint8_t>());
        vector<uint8_t, vec_size> result_mantissa_combine_1 = aie::filter_odd(result_mantissa_combine.cast_to<uint8_t>());
        // select the 1st or 2nd int8 from uint16_t return
        vector<uint8_t, vec_size> result_mantissa = aie::select(result_mantissa_combine_0, result_mantissa_combine_1, mask_exponent_remainder_0);

        // 5. 2^F lookup table is not complete, we will implemente result use some comparision
        //    When F is between 0.112304875 and 0.0712890625, lookup table has no answer. we
        //    use a comparision to get right mantissa
        mask<vec_size> is_large_0 = aie::le(exponent_f16_vec, bfloat16(0.071289062500000));
        mask<vec_size> is_large_1 = aie::le(exponent_f16_vec, bfloat16(0.060546875000000));
        mask<vec_size> is_large_2 = aie::le(exponent_f16_vec, bfloat16(0.049804687500000));
        mask<vec_size> is_large_3 = aie::le(exponent_f16_vec, bfloat16(0.038818359375000));
        mask<vec_size> is_large_4 = aie::le(exponent_f16_vec, bfloat16(0.027832031250000));
        mask<vec_size> is_large_5 = aie::le(exponent_f16_vec, bfloat16(0.016723632812500));
        mask<vec_size> is_large_6 = aie::le(exponent_f16_vec, bfloat16(0.011230468750000));

        result_mantissa = aie::select(result_mantissa, (uint8_t)6, is_large_0);
        result_mantissa = aie::select(result_mantissa, (uint8_t)5, is_large_1);
        result_mantissa = aie::select(result_mantissa, (uint8_t)4, is_large_2);
        result_mantissa = aie::select(result_mantissa, (uint8_t)3, is_large_3);
        result_mantissa = aie::select(result_mantissa, (uint8_t)2, is_large_4);
        result_mantissa = aie::select(result_mantissa, (uint8_t)1, is_large_5);
        result_mantissa = aie::select(result_mantissa, (uint8_t)0, is_large_6);

        accum<acc32, vec_size> result_mantissa_acc;
        result_mantissa_acc.from_vector(result_mantissa);
        vector<int16_t, vec_size> result_mantissa_uint16 = result_mantissa_acc.to_vector<int16_t>();

        // 6. Combine I and 2^F, get the result
        exponent_int16 = aie::upshift(exponent_int16, 7);
        vector<int16_t, vec_size> output_bf16_int = aie::bit_or(exponent_int16, result_mantissa_uint16);
        output_bf16 = output_bf16_int.cast_to<bfloat16>();

        // 7. judge the result when x == 0
        mask<vec_size> is_zero = aie::eq(input_buff, int16_t(0));
        is_zero = is_zero | exponent_minus_zero_mask;
        out = aie::select(output_bf16, (bfloat16)0.0, is_zero);

        // 8. judge the result when x < 0 & exponent is a odd number
        mask<vec_size> less_than_zero = aie::lt(input_buff, int16_t(0));
        mask<vec_size> lt_odd = less_than_zero & is_odd;
        out = aie::select(out, sub(vec_zero, out), lt_odd);// out = -out

        // 9. judge the result when x < 0 & exponent is fraction
        mask<vec_size> lt_fraction = less_than_zero & is_fraction;
        out = aie::select(out, vec_nan, lt_fraction);// 'nan'
    }
};
