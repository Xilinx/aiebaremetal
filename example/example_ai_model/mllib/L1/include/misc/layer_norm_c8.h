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

// this kernel implements layer normalization in both bfloat16 & int8 datatypes
#ifndef __LAYER_NORM_C8_H__
#define __LAYER_NORM_C8_H__

#include "aie.hpp"
#include "aie_api/utils.hpp"
#include <adf.h>
#include "utils.h"
#include "layer_norm_c8_params.h"

using namespace aie;

template <typename dtype_ofm, typename dtype_acc>
    requires(std::is_same_v<dtype_ofm, int16_t>) && (std::is_same_v<dtype_acc, int32_t>)
INLINE void coeff_computation(dtype_ofm *o_mean_invsqrt_ptr, dtype_ofm *o_invsqrt_ptr, dtype_acc *psum_ptr, dtype_acc *psqr_ptr, const layer_norm_c8_params_t &param)
{
	AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(4, ) 
    for (int i = 0; i < param.outer_loop / PART1_OUTER_UNROLL_NUM; i++) {
            // UNROLL 1
            aie::vector<dtype_acc, 32> vec_psum_tmp1 = aie::load_v<32>(psum_ptr);
            aie::vector<dtype_acc, 32> vec_psqr_tmp1 = aie::load_v<32>(psqr_ptr);
            psum_ptr = byte_incr(psum_ptr, 32 * sizeof(dtype_acc));
            psqr_ptr = byte_incr(psqr_ptr, 32 * sizeof(dtype_acc));
            // UNROLL 2
            aie::vector<dtype_acc, 32> vec_psum_tmp2 = aie::load_v<32>(psum_ptr);
            aie::vector<dtype_acc, 32> vec_psqr_tmp2 = aie::load_v<32>(psqr_ptr);
            psum_ptr = byte_incr(psum_ptr, 32 * sizeof(dtype_acc));
            psqr_ptr = byte_incr(psqr_ptr, 32 * sizeof(dtype_acc));
            // aie::print(vec_psum_tmp1, true, "psum1: ");
            // aie::print(vec_psum_tmp2, true, "psum2: ");
            // aie::print(vec_psqr_tmp1, true, "psqr1: ");
            // aie::print(vec_psqr_tmp2, true, "psqr2: ");

            // UNROLL 1
            auto vec1_reduce_sum_c_tmp0 = vec_psum_tmp1.template extract<8>(0);
            auto vec1_reduce_sum_c_tmp1 = vec_psum_tmp1.template extract<8>(1);
            auto vec1_reduce_sum_c_tmp2 = vec_psum_tmp1.template extract<8>(2);
            auto vec1_reduce_sum_c_tmp3 = vec_psum_tmp1.template extract<8>(3);
            // UNROLL 2
            auto vec2_reduce_sum_c_tmp0 = vec_psum_tmp2.template extract<8>(0);
            auto vec2_reduce_sum_c_tmp1 = vec_psum_tmp2.template extract<8>(1);
            auto vec2_reduce_sum_c_tmp2 = vec_psum_tmp2.template extract<8>(2);
            auto vec2_reduce_sum_c_tmp3 = vec_psum_tmp2.template extract<8>(3);

            // UNROLL 1
            auto vec1_reduce_sqr_c_tmp0 = vec_psqr_tmp1.template extract<8>(0);
            auto vec1_reduce_sqr_c_tmp1 = vec_psqr_tmp1.template extract<8>(1);
            auto vec1_reduce_sqr_c_tmp2 = vec_psqr_tmp1.template extract<8>(2);
            auto vec1_reduce_sqr_c_tmp3 = vec_psqr_tmp1.template extract<8>(3);
            // UNROLL 2
            auto vec2_reduce_sqr_c_tmp0 = vec_psqr_tmp2.template extract<8>(0);
            auto vec2_reduce_sqr_c_tmp1 = vec_psqr_tmp2.template extract<8>(1);
            auto vec2_reduce_sqr_c_tmp2 = vec_psqr_tmp2.template extract<8>(2);
            auto vec2_reduce_sqr_c_tmp3 = vec_psqr_tmp2.template extract<8>(3);

            // XXX: only 4 vectors can be reduced in parallel
            auto vec_reduce_c_sum_int1 = aie::reduce_add_v(vec1_reduce_sum_c_tmp0, vec1_reduce_sum_c_tmp1, vec1_reduce_sum_c_tmp2, vec1_reduce_sum_c_tmp3);
            auto vec_reduce_c_sum_int2 = aie::reduce_add_v(vec2_reduce_sum_c_tmp0, vec2_reduce_sum_c_tmp1, vec2_reduce_sum_c_tmp2, vec2_reduce_sum_c_tmp3);
            auto vec_reduce_c_sqr_int1 = aie::reduce_add_v(vec1_reduce_sqr_c_tmp0, vec1_reduce_sqr_c_tmp1, vec1_reduce_sqr_c_tmp2, vec1_reduce_sqr_c_tmp3);
            auto vec_reduce_c_sqr_int2 = aie::reduce_add_v(vec2_reduce_sqr_c_tmp0, vec2_reduce_sqr_c_tmp1, vec2_reduce_sqr_c_tmp2, vec2_reduce_sqr_c_tmp3);
            aie::vector<dtype_acc, 8> vec_reduce_c_sum_int, vec_reduce_c_sqr_int;
            vec_reduce_c_sum_int.insert(0, vec_reduce_c_sum_int1.template extract<4>(0));
            vec_reduce_c_sum_int.insert(1, vec_reduce_c_sum_int2.template extract<4>(0));
            vec_reduce_c_sqr_int.insert(0, vec_reduce_c_sqr_int1.template extract<4>(0));
            vec_reduce_c_sqr_int.insert(1, vec_reduce_c_sqr_int2.template extract<4>(0));

            accum<accfloat, 8> acc_reduce_c_sum(aie::to_float(vec_reduce_c_sum_int, param.ifm_shift));
            auto vec_reduce_c_sum = acc_reduce_c_sum.template to_vector<bfloat16>();
            accum<accfloat, 8> acc_reduce_c_sqr(aie::to_float(vec_reduce_c_sqr_int, 2 * param.ifm_shift));
            auto vec_reduce_c_sqr = acc_reduce_c_sqr.template to_vector<bfloat16>();
            auto Ex = aie::mul(vec_reduce_c_sum, param.div_factor);
            auto Ex_bf16 = Ex.template to_vector<bfloat16>();
            auto Ex2 = aie::mul(vec_reduce_c_sqr, param.div_factor);
            auto Var = aie::msc_square(Ex2, Ex_bf16);
            auto Var_eps = aie::add(Var, param.epsilon);
            auto Var_eps_bf16 = Var_eps.template to_vector<bfloat16>();
            auto inv_sqrt = aie::invsqrt(Var_eps_bf16);
            auto inv_sqrt_int = aie::to_fixed(inv_sqrt, 16);
            accum<acc32, 8> acc_inv_sqrt(inv_sqrt_int);
            // aie::print(acc_inv_sqrt, true, "inv_sqrt: ");

            auto mean_inv_sqrt = aie::mul(Ex_bf16, inv_sqrt);
            auto mean_inv_sqrt_bf16 = mean_inv_sqrt.template to_vector<bfloat16>();
            auto mean_inv_sqrt_int = aie::to_fixed(mean_inv_sqrt_bf16, 16);
            accum<acc32, 8> acc_mean_inv_sqrt(mean_inv_sqrt_int);
            // aie::print(acc_mean_inv_sqrt, true, "mean_inv_sqrt: ");

            aie::store_v(o_mean_invsqrt_ptr, acc_mean_inv_sqrt.template to_vector<dtype_ofm>(16 - param.beta_shift + param.gamma_shift));
            o_mean_invsqrt_ptr = byte_incr(o_mean_invsqrt_ptr, 4 * PART1_OUTER_UNROLL_NUM * sizeof(dtype_ofm));
            aie::store_v(o_invsqrt_ptr, acc_inv_sqrt.template to_vector<dtype_ofm>(16 - param.beta_shift + param.gamma_shift + param.ifm_shift));
            o_invsqrt_ptr = byte_incr(o_invsqrt_ptr, 4 * PART1_OUTER_UNROLL_NUM * sizeof(dtype_ofm));
        }
}

template <uint8_t batch_size, typename dtype_ifm, typename dtype_ofm>
requires(std::is_same_v<dtype_ifm, int8_t>) && (std::is_same_v<dtype_ofm, int16_t>) && (batch_size == 1)
NOINLINE void layer_norm_c8_part1(
    dtype_ifm * __restrict i_ptr,
    dtype_ofm * __restrict o_mean_invsqrt_ptr,
    dtype_ofm * __restrict o_invsqrt_ptr,
    layer_norm_c8_params_t &param)
{
    dtype_ofm *ofm_ptr = o_mean_invsqrt_ptr;

    // pointers to store current sum(x) & sum(x^2) along C dimension
    int32_t OFM_DM_BANK *__restrict psum_ptr = (int32_t OFM_DM_BANK *__restrict)ofm_ptr;
    int32_t OFM_DM_BANK *__restrict psqr_ptr = (int32_t OFM_DM_BANK *__restrict)ofm_ptr + param.height * param.width * 8;
    // pointers to load previous sum(x) & sum(x^2) along C dimension
    int32_t OFM_DM_BANK *__restrict psum_ptr_init = (int32_t OFM_DM_BANK *__restrict)ofm_ptr;
    int32_t OFM_DM_BANK *__restrict psqr_ptr_init = (int32_t OFM_DM_BANK *__restrict)ofm_ptr + param.height * param.width * 8;

    addr_t i_ptr_addr_count1 = 0;
    addr_t i_ptr_addr_count2 = 0;
    addr_t psum_ptr_addr_count = 0;
    addr_t psqr_ptr_addr_count = 0;
    addr_t psum_ptr_init_addr_count = 0;
    addr_t psqr_ptr_init_addr_count = 0;

    // clean up the output at the beginning
    int32_t OFM_DM_BANK *__restrict pcoeff_ptr_zeros = (int32_t OFM_DM_BANK *__restrict)ofm_ptr;
    aie::vector<int32_t, 32> vec_init = aie::broadcast<int32_t, 32>(0);
    if (param.depth_iter == 0)
    {
        // printf("[Kernel]: 1st iteration to clean up the OFM buffer\n");
		AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4, ) 
        for (int i = 0; i < param.init_loop; i++) {
            aie::store_v(pcoeff_ptr_zeros, vec_init);
            pcoeff_ptr_zeros += 32;
        }
    }

    // printf("[Kernel]: intermediate iterations to accumulate for sum(x) & sum(x^2)\n");
	AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(4, ) 
    for (int i = 0; i < param.outer_loop * param.inner_loop / PART1_INNER_UNROLL_NUM; i++) {
            // LOAD PSUM & PSQR
            aie::vector<int32_t, 32> vec_psum = aie::load_v<32>(psum_ptr_init);
            aie::vector<int32_t, 32> vec_psqr = aie::load_v<32>(psqr_ptr_init);
            psum_ptr_init = add_2d_ptr(psum_ptr_init, -32 * param.outer_loop + 32, param.outer_loop - 1, psum_ptr_init_addr_count, 32);
            psqr_ptr_init = add_2d_ptr(psqr_ptr_init, -32 * param.outer_loop + 32, param.outer_loop - 1, psqr_ptr_init_addr_count, 32);
            // aie::print(vec_psum, true, "vec_psum: ");
            // aie::print(vec_psqr, true, "vec_psqr: ");

            aie::accum<acc32, 32> acc_sum, acc_sqr;
            acc_sum.template from_vector(vec_psum);
            acc_sqr.template from_vector(vec_psqr);

            // UNROLL 1
            auto vec_0 = aie::unpack(aie::load_v<32>(i_ptr));
            i_ptr = i_ptr + param.i_ptr_inc4;
            acc_sum = aie::add(acc_sum, vec_0);
            acc_sqr = aie::mac_square(acc_sqr, vec_0);

            // UNROLL 2
            auto vec_1 = aie::unpack(aie::load_v<32>(i_ptr));
            i_ptr = i_ptr + param.i_ptr_inc4;
            acc_sum = aie::add(acc_sum, vec_1);
            acc_sqr = aie::mac_square(acc_sqr, vec_1);

            // UNROLL 3
            auto vec_2 = aie::unpack(aie::load_v<32>(i_ptr));
            i_ptr = i_ptr + param.i_ptr_inc4;
            acc_sum = aie::add(acc_sum, vec_2);
            acc_sqr = aie::mac_square(acc_sqr, vec_2);

            // UNROLL 4
            auto vec_3 = aie::unpack(aie::load_v<32>(i_ptr));
            i_ptr = add_3d_ptr(i_ptr, param.i_ptr_offset, param.i_ptr_size1, i_ptr_addr_count1, param.i_ptr_inc1, param.i_ptr_size2, i_ptr_addr_count2, param.i_ptr_inc2);
            acc_sum = aie::add(acc_sum, vec_3);
            acc_sqr = aie::mac_square(acc_sqr, vec_3);
            // aie::print(vec_0, true, "vec_0: ");
            // aie::print(vec_1, true, "vec_1: ");
            // aie::print(vec_2, true, "vec_2: ");
            // aie::print(vec_3, true, "vec_3: ");

            // STORE PSUM & PSQR
            aie::store_v(psum_ptr, acc_sum.template to_vector<int32_t>());
            aie::store_v(psqr_ptr, acc_sqr.template to_vector<int32_t>());
            psum_ptr = add_2d_ptr(psum_ptr, -32 * param.outer_loop + 32, param.outer_loop - 1, psum_ptr_addr_count, 32);
            psqr_ptr = add_2d_ptr(psqr_ptr, -32 * param.outer_loop + 32, param.outer_loop - 1, psqr_ptr_addr_count, 32);
        }

    // calculate the coefficients at the last sub-volume in C dimension
    if (param.depth_iter == param.loop_num_channel - 1)
    {
        // printf("[Kernel]: final iteration to calculate the E(x) & Var(x)\n");
        coeff_computation<dtype_ofm, int32_t>(o_mean_invsqrt_ptr, o_invsqrt_ptr, psum_ptr, psqr_ptr, param);
    }
    param.depth_iter++;
}

template <uint8_t batch_size, typename dtype_ifm, typename dtype_ofm, typename config_ifm, typename config_ofm>
    requires(std::is_same_v<dtype_ifm, int8_t>) && (std::is_same_v<dtype_ofm, int16_t>) && (batch_size == 1)
NOINLINE void layer_norm_c8_part1(
    adf::input_buffer_conf<dtype_ifm, config_ifm> &__restrict ifm,
    adf::output_buffer_conf<dtype_ofm, config_ofm> &__restrict ofm,
    layer_norm_c8_params_t &param)
{
    dtype_ifm IFM_DM_BANK *__restrict ifm_ptr =
        (dtype_ifm IFM_DM_BANK *__restrict)ifm.data() + param.overlay_row_offset;
    dtype_ofm OFM_DM_BANK *__restrict ofm_ptr =
        (dtype_ofm OFM_DM_BANK *__restrict)ofm.data();

    dtype_ofm *o_invsqrt_ptr = ofm_ptr + param.height * param.width;

    layer_norm_c8_part1<batch_size, dtype_ifm, dtype_ofm>(ifm_ptr, ofm_ptr, o_invsqrt_ptr, param);
}

template <uint8_t batch_size, typename dtype>
    requires(std::is_same_v<dtype, int8_t>) && (batch_size == 1)
NOINLINE void layer_norm_c8_part2(
    dtype * __restrict ifm_ptr,
    dtype * __restrict gamma_ptr0,
    dtype * __restrict beta_ptr0,
    int16_t * __restrict mean_invsqrt_ptr0,
    int16_t * __restrict invsqrt_ptr0,
    dtype * __restrict ofm_ptr,
    layer_norm_c8_params_t &param)
{
    dtype *gamma_ptr1 = gamma_ptr0 + 32;
    dtype *gamma_ptr2 = gamma_ptr0 + 32 * 2;
    dtype *gamma_ptr3 = gamma_ptr0 + 32 * 3;
    dtype *beta_ptr1 = beta_ptr0 + 32;
    dtype *beta_ptr2 = beta_ptr0 + 32 * 2;
    dtype *beta_ptr3 = beta_ptr0 + 32 * 3;
    int16_t *mean_invsqrt_ptr1 = mean_invsqrt_ptr0 + 1;
    int16_t *mean_invsqrt_ptr2 = mean_invsqrt_ptr0 + 2;
    int16_t *mean_invsqrt_ptr3 = mean_invsqrt_ptr0 + 3;
    int16_t *invsqrt_ptr1 = invsqrt_ptr0 + 1;
    int16_t *invsqrt_ptr2 = invsqrt_ptr0 + 2;
    int16_t *invsqrt_ptr3 = invsqrt_ptr0 + 3;

    aie::vector<int16_t, 32> vec_gamma0, vec_gamma1, vec_gamma2, vec_gamma3;
    aie::vector<int16_t, 32> vec_beta0, vec_beta1, vec_beta2, vec_beta3;
    aie::vector<int16_t, 32> vec_invsqrt, vec_mean_invsqrt;

    addr_t gamma_ptr0_addr_cnt = 0;
    addr_t gamma_ptr1_addr_cnt = 0;
    addr_t gamma_ptr2_addr_cnt = 0;
    addr_t gamma_ptr3_addr_cnt = 0;
    addr_t beta_ptr0_addr_cnt = 0;
    addr_t beta_ptr1_addr_cnt = 0;
    addr_t beta_ptr2_addr_cnt = 0;
    addr_t beta_ptr3_addr_cnt = 0;
    addr_t invsqrt_ptr0_addr_cnt = 0;
    addr_t invsqrt_ptr1_addr_cnt = 0;
    addr_t invsqrt_ptr2_addr_cnt = 0;
    addr_t invsqrt_ptr3_addr_cnt = 0;
    addr_t mean_invsqrt_ptr0_addr_cnt = 0;
    addr_t mean_invsqrt_ptr1_addr_cnt = 0;
    addr_t mean_invsqrt_ptr2_addr_cnt = 0;
    addr_t mean_invsqrt_ptr3_addr_cnt = 0;
    addr_t ifm_ptr_addr_count1 = 0;
    addr_t ifm_ptr_addr_count2 = 0;
    addr_t ofm_ptr_addr_count1 = 0;
    addr_t ofm_ptr_addr_count2 = 0;

	AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(4, ) 
    for (int i = 0; i < param.outer_loop * param.inner_loop / PART2_INNER_UNROLL_NUM; i++) {
        // printf("Iteration-%d\n", i);
        // load gamma/beta vector
        // UNROLL 1
        vec_gamma0 = aie::unpack(aie::load_v<32>(gamma_ptr0));
		gamma_ptr0 = add_2d_ptr(gamma_ptr0, 32*PART2_INNER_UNROLL_NUM, param.outer_loop-1, gamma_ptr0_addr_cnt, 0);
        vec_beta0 = aie::unpack(aie::load_v<32>(beta_ptr0));
		beta_ptr0 = add_2d_ptr(beta_ptr0, 32*PART2_INNER_UNROLL_NUM, param.outer_loop-1, beta_ptr0_addr_cnt, 0);
        // UNROLL 2
        vec_gamma1 = aie::unpack(aie::load_v<32>(gamma_ptr1));
		gamma_ptr1 = add_2d_ptr(gamma_ptr1, 32*PART2_INNER_UNROLL_NUM, param.outer_loop-1, gamma_ptr1_addr_cnt, 0);
        vec_beta1 = aie::unpack(aie::load_v<32>(beta_ptr1));
		beta_ptr1 = add_2d_ptr(beta_ptr1, 32*PART2_INNER_UNROLL_NUM, param.outer_loop-1, beta_ptr1_addr_cnt, 0);
        // UNROLL 3
        vec_gamma2 = aie::unpack(aie::load_v<32>(gamma_ptr2));
		gamma_ptr2 = add_2d_ptr(gamma_ptr2, 32*PART2_INNER_UNROLL_NUM, param.outer_loop-1, gamma_ptr2_addr_cnt, 0);
        vec_beta2 = aie::unpack(aie::load_v<32>(beta_ptr2));
		beta_ptr2 = add_2d_ptr(beta_ptr2, 32*PART2_INNER_UNROLL_NUM, param.outer_loop-1, beta_ptr2_addr_cnt, 0);
        // UNROLL 4
        vec_gamma3 = aie::unpack(aie::load_v<32>(gamma_ptr3));
		gamma_ptr3 = add_2d_ptr(gamma_ptr3, 32*PART2_INNER_UNROLL_NUM, param.outer_loop-1, gamma_ptr3_addr_cnt, 0);
        vec_beta3 = aie::unpack(aie::load_v<32>(beta_ptr3));
		beta_ptr3 = add_2d_ptr(beta_ptr3, 32*PART2_INNER_UNROLL_NUM, param.outer_loop-1, beta_ptr3_addr_cnt, 0);
        // aie::print(vec_gamma0, true, "gamma0: ");
        // aie::print(vec_gamma1, true, "gamma1: ");
        // aie::print(vec_gamma2, true, "gamma2: ");
        // aie::print(vec_gamma3, true, "gamma3: ");
        // aie::print(vec_beta0, true, "beta0: ");
        // aie::print(vec_beta1, true, "beta1: ");
        // aie::print(vec_beta2, true, "beta2: ");
        // aie::print(vec_beta3, true, "beta3: ");

            // load mean_invsqrt/invsqrt in W4
            int16_t invsqrt0 = *invsqrt_ptr0;
            invsqrt_ptr0 = add_2d_ptr(invsqrt_ptr0, param.coeff_ptr_offset, param.i_ptr_size1, invsqrt_ptr0_addr_cnt, 4);
            int16_t invsqrt1 = *invsqrt_ptr1;
            invsqrt_ptr1 = add_2d_ptr(invsqrt_ptr1, param.coeff_ptr_offset, param.i_ptr_size1, invsqrt_ptr1_addr_cnt, 4);
            int16_t invsqrt2 = *invsqrt_ptr2;
            invsqrt_ptr2 = add_2d_ptr(invsqrt_ptr2, param.coeff_ptr_offset, param.i_ptr_size1, invsqrt_ptr2_addr_cnt, 4);
            int16_t invsqrt3 = *invsqrt_ptr3;
            invsqrt_ptr3 = add_2d_ptr(invsqrt_ptr3, param.coeff_ptr_offset, param.i_ptr_size1, invsqrt_ptr3_addr_cnt, 4);
            int16_t mean_invsqrt0 = *mean_invsqrt_ptr0;
            mean_invsqrt_ptr0 = add_2d_ptr(mean_invsqrt_ptr0, param.coeff_ptr_offset, param.i_ptr_size1, mean_invsqrt_ptr0_addr_cnt, 4);
            int16_t mean_invsqrt1 = *mean_invsqrt_ptr1;
            mean_invsqrt_ptr1 = add_2d_ptr(mean_invsqrt_ptr1, param.coeff_ptr_offset, param.i_ptr_size1, mean_invsqrt_ptr1_addr_cnt, 4);
            int16_t mean_invsqrt2 = *mean_invsqrt_ptr2;
            mean_invsqrt_ptr2 = add_2d_ptr(mean_invsqrt_ptr2, param.coeff_ptr_offset, param.i_ptr_size1, mean_invsqrt_ptr2_addr_cnt, 4);
            int16_t mean_invsqrt3 = *mean_invsqrt_ptr3;
            mean_invsqrt_ptr3 = add_2d_ptr(mean_invsqrt_ptr3, param.coeff_ptr_offset, param.i_ptr_size1, mean_invsqrt_ptr3_addr_cnt, 4);

            aie::vector<int16_t, 8> vec_invsqrt_C0 = aie::broadcast<int16_t, 8>(invsqrt0);
            aie::vector<int16_t, 8> vec_invsqrt_C1 = aie::broadcast<int16_t, 8>(invsqrt1);
            aie::vector<int16_t, 8> vec_invsqrt_C2 = aie::broadcast<int16_t, 8>(invsqrt2);
            aie::vector<int16_t, 8> vec_invsqrt_C3 = aie::broadcast<int16_t, 8>(invsqrt3);
            aie::vector<int16_t, 8> vec_mean_invsqrt_C0 = aie::broadcast<int16_t, 8>(mean_invsqrt0);
            aie::vector<int16_t, 8> vec_mean_invsqrt_C1 = aie::broadcast<int16_t, 8>(mean_invsqrt1);
            aie::vector<int16_t, 8> vec_mean_invsqrt_C2 = aie::broadcast<int16_t, 8>(mean_invsqrt2);
            aie::vector<int16_t, 8> vec_mean_invsqrt_C3 = aie::broadcast<int16_t, 8>(mean_invsqrt3);

            // duplication for W4C8
            vec_mean_invsqrt.insert(0, vec_mean_invsqrt_C0);
            vec_mean_invsqrt.insert(1, vec_mean_invsqrt_C1);
            vec_mean_invsqrt.insert(2, vec_mean_invsqrt_C2);
            vec_mean_invsqrt.insert(3, vec_mean_invsqrt_C3);
            vec_invsqrt.insert(0, vec_invsqrt_C0);
            vec_invsqrt.insert(1, vec_invsqrt_C1);
            vec_invsqrt.insert(2, vec_invsqrt_C2);
            vec_invsqrt.insert(3, vec_invsqrt_C3);
            // aie::print(vec_invsqrt, true, "invsqrt: ");
            // aie::print(vec_mean_invsqrt, true, "mean_invsqrt: ");

            // UNROLL 1
            auto vec_0 = aie::unpack(aie::load_v<32>(ifm_ptr));
            ifm_ptr = ifm_ptr + param.i_ptr_inc4;
            // UNROLL 2
            auto vec_1 = aie::unpack(aie::load_v<32>(ifm_ptr));
            ifm_ptr = ifm_ptr + param.i_ptr_inc4;
            // UNROLL 3
            auto vec_2 = aie::unpack(aie::load_v<32>(ifm_ptr));
            ifm_ptr = ifm_ptr + param.i_ptr_inc4;
            // UNROLL 4
            auto vec_3 = aie::unpack(aie::load_v<32>(ifm_ptr));
            ifm_ptr = add_3d_ptr(ifm_ptr, param.i_ptr_offset, param.i_ptr_size1, ifm_ptr_addr_count1, param.i_ptr_inc1, param.i_ptr_size2, ifm_ptr_addr_count2, param.i_ptr_inc2);
            // aie::print(vec_0, true, "vec_0: ");
            // aie::print(vec_1, true, "vec_1: ");
            // aie::print(vec_2, true, "vec_2: ");
            // aie::print(vec_3, true, "vec_3: ");

            aie::accum<acc32, 32> acc_beta0(vec_beta0);
            aie::accum<acc32, 32> acc_beta1(vec_beta1);
            aie::accum<acc32, 32> acc_beta2(vec_beta2);
            aie::accum<acc32, 32> acc_beta3(vec_beta3);

            // UNROLL 1
            auto acc_mean_invsqrt0 = aie::mul(vec_0, vec_invsqrt);
            acc_mean_invsqrt0 = aie::sub(acc_mean_invsqrt0, vec_mean_invsqrt);
            // aie::print(acc_mean_invsqrt0, true, "inner_res0: ");
            acc_beta0 = aie::mac(acc_beta0, acc_mean_invsqrt0.template to_vector<int16_t>(), vec_gamma0);
            // UNROLL 2
            auto acc_mean_invsqrt1 = aie::mul(vec_1, vec_invsqrt);
            acc_mean_invsqrt1 = aie::sub(acc_mean_invsqrt1, vec_mean_invsqrt);
            // aie::print(acc_mean_invsqrt1, true, "inner_res1: ");
            acc_beta1 = aie::mac(acc_beta1, acc_mean_invsqrt1.template to_vector<int16_t>(), vec_gamma1);
            // UNROLL 3
            auto acc_mean_invsqrt2 = aie::mul(vec_2, vec_invsqrt);
            acc_mean_invsqrt2 = aie::sub(acc_mean_invsqrt2, vec_mean_invsqrt);
            // aie::print(acc_mean_invsqrt2, true, "inner_res2: ");
            acc_beta2 = aie::mac(acc_beta2, acc_mean_invsqrt2.template to_vector<int16_t>(), vec_gamma2);
            // UNROLL 4
            auto acc_mean_invsqrt3 = aie::mul(vec_3, vec_invsqrt);
            acc_mean_invsqrt3 = aie::sub(acc_mean_invsqrt3, vec_mean_invsqrt);
            // aie::print(acc_mean_invsqrt3, true, "inner_res3: ");
            acc_beta3 = aie::mac(acc_beta3, acc_mean_invsqrt3.template to_vector<int16_t>(), vec_gamma3);
            // aie::print(acc_beta0, true, "outer_res0: ");
            // aie::print(acc_beta1, true, "outer_res1: ");
            // aie::print(acc_beta2, true, "outer_res2: ");
            // aie::print(acc_beta3, true, "outer_res3: ");

            // STORE OFM
            // UNROLL 1
            aie::store_v(ofm_ptr, acc_beta0.template to_vector<dtype>(param.beta_shift - param.ofm_shift));
            ofm_ptr = ofm_ptr + param.i_ptr_inc4;
            // UNROLL 2
            aie::store_v(ofm_ptr, acc_beta1.template to_vector<dtype>(param.beta_shift - param.ofm_shift));
            ofm_ptr = ofm_ptr + param.i_ptr_inc4;
            // UNROLL 3
            aie::store_v(ofm_ptr, acc_beta2.template to_vector<dtype>(param.beta_shift - param.ofm_shift));
            ofm_ptr = ofm_ptr + param.i_ptr_inc4;
            // UNROLL 4
            aie::store_v(ofm_ptr, acc_beta3.template to_vector<dtype>(param.beta_shift - param.ofm_shift));
            ofm_ptr = add_3d_ptr(ofm_ptr, param.i_ptr_offset, param.i_ptr_size1, ofm_ptr_addr_count1, param.i_ptr_inc1, param.i_ptr_size2, ofm_ptr_addr_count2, param.i_ptr_inc2);
        }
    // Iterate the full-volume along C-W-H
    if (param.c_idx == param.loop_num_channel - 1)
    {
        param.c_idx = 0;
        if (param.w_idx == param.loop_num_width - 1)
        {
            param.w_idx = 0;
            param.h_idx++;
        }
        else
        {
            param.w_idx++;
        }
    }
    else
    {
        param.c_idx++;
    }
}

template <uint8_t batch_size, typename dtype, typename config_ifm, typename config_wts, typename config_ofm>
    requires(std::is_same_v<dtype, int8_t>) && (batch_size == 1)
NOINLINE void layer_norm_c8_part2(
    adf::input_buffer_conf<dtype, config_ifm> &__restrict ifm,
    adf::input_buffer_conf<dtype, config_wts> &__restrict wts,
    adf::output_buffer_conf<dtype, config_ofm> &__restrict ofm,
    layer_norm_c8_params_t &param)
{
    dtype IFM_DM_BANK *__restrict ifm_ptr =
        (dtype IFM_DM_BANK *__restrict)ifm.data() + param.overlay_row_offset;
    dtype WTS_DM_BANK *__restrict wts_ptr =
        (dtype WTS_DM_BANK *__restrict)wts.data();
    dtype OFM_DM_BANK *__restrict ofm_ptr =
        (dtype OFM_DM_BANK *__restrict)ofm.data();

    dtype *gamma_ptr0 = wts_ptr + 4 * param.channel * param.c_idx;
    dtype *beta_ptr0 = gamma_ptr0 + 4 * param.channel * param.loop_num_channel;
    int16_t *mean_invsqrt_ptr0 = (int16_t *)(wts_ptr + 2 * 4 * param.channel * param.loop_num_channel) + param.width * param.loop_num_width * param.height * param.h_idx + param.width * param.w_idx;
    int16_t *invsqrt_ptr0 = mean_invsqrt_ptr0 + param.height * param.loop_num_height * param.width * param.loop_num_width;

    layer_norm_c8_part2<batch_size, dtype>(ifm_ptr, gamma_ptr0, beta_ptr0, mean_invsqrt_ptr0, invsqrt_ptr0, ofm_ptr, param);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename dtype_ofm, typename dtype_acc>
    requires(std::is_same_v<dtype_ofm, bfloat16>) && (std::is_same_v<dtype_acc, float>)
INLINE void coeff_computation(dtype_ofm *o_mean_invsqrt_ptr, dtype_ofm *o_invsqrt_ptr, dtype_acc *psum_ptr, dtype_acc *psqr_ptr, const layer_norm_c8_params_t &param)
{
	AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(4, ) 
    for (int i = 0; i < param.outer_loop / PART1_OUTER_UNROLL_NUM; i++) {
            // UNROLL 1
            auto vec_psum_tmp1 = aie::load_v<32>(psum_ptr);
            auto vec_psqr_tmp1 = aie::load_v<32>(psqr_ptr);
            psum_ptr = byte_incr(psum_ptr, 32 * sizeof(dtype_acc));
            psqr_ptr = byte_incr(psqr_ptr, 32 * sizeof(dtype_acc));
            // UNROLL 2
            auto vec_psum_tmp2 = aie::load_v<32>(psum_ptr);
            auto vec_psqr_tmp2 = aie::load_v<32>(psqr_ptr);
            psum_ptr = byte_incr(psum_ptr, 32 * sizeof(dtype_acc));
            psqr_ptr = byte_incr(psqr_ptr, 32 * sizeof(dtype_acc));
            // aie::print(vec_psum_tmp1, true, "psum1: ");
            // aie::print(vec_psum_tmp2, true, "psum2: ");
            // aie::print(vec_psqr_tmp1, true, "psqr1: ");
            // aie::print(vec_psqr_tmp2, true, "psqr2: ");

            // UNROLL 1
            auto vec1_reduce_sum_c_tmp0 = vec_psum_tmp1.template extract<8>(0);
            auto vec1_reduce_sum_c_tmp1 = vec_psum_tmp1.template extract<8>(1);
            auto vec1_reduce_sum_c_tmp2 = vec_psum_tmp1.template extract<8>(2);
            auto vec1_reduce_sum_c_tmp3 = vec_psum_tmp1.template extract<8>(3);
            // UNROLL 2
            auto vec2_reduce_sum_c_tmp0 = vec_psum_tmp2.template extract<8>(0);
            auto vec2_reduce_sum_c_tmp1 = vec_psum_tmp2.template extract<8>(1);
            auto vec2_reduce_sum_c_tmp2 = vec_psum_tmp2.template extract<8>(2);
            auto vec2_reduce_sum_c_tmp3 = vec_psum_tmp2.template extract<8>(3);

            // UNROLL 1
            auto vec1_reduce_sqr_c_tmp0 = vec_psqr_tmp1.template extract<8>(0);
            auto vec1_reduce_sqr_c_tmp1 = vec_psqr_tmp1.template extract<8>(1);
            auto vec1_reduce_sqr_c_tmp2 = vec_psqr_tmp1.template extract<8>(2);
            auto vec1_reduce_sqr_c_tmp3 = vec_psqr_tmp1.template extract<8>(3);
            // UNROLL 2
            auto vec2_reduce_sqr_c_tmp0 = vec_psqr_tmp2.template extract<8>(0);
            auto vec2_reduce_sqr_c_tmp1 = vec_psqr_tmp2.template extract<8>(1);
            auto vec2_reduce_sqr_c_tmp2 = vec_psqr_tmp2.template extract<8>(2);
            auto vec2_reduce_sqr_c_tmp3 = vec_psqr_tmp2.template extract<8>(3);

            // XXX: only 4 vectors can be reduced in parallel
            auto vec_reduce_c_sum1 = aie::reduce_add_v(vec1_reduce_sum_c_tmp0, vec1_reduce_sum_c_tmp1, vec1_reduce_sum_c_tmp2, vec1_reduce_sum_c_tmp3);
            auto vec_reduce_c_sum2 = aie::reduce_add_v(vec2_reduce_sum_c_tmp0, vec2_reduce_sum_c_tmp1, vec2_reduce_sum_c_tmp2, vec2_reduce_sum_c_tmp3);
            auto vec_reduce_c_sqr1 = aie::reduce_add_v(vec1_reduce_sqr_c_tmp0, vec1_reduce_sqr_c_tmp1, vec1_reduce_sqr_c_tmp2, vec1_reduce_sqr_c_tmp3);
            auto vec_reduce_c_sqr2 = aie::reduce_add_v(vec2_reduce_sqr_c_tmp0, vec2_reduce_sqr_c_tmp1, vec2_reduce_sqr_c_tmp2, vec2_reduce_sqr_c_tmp3);
            aie::vector<dtype_acc, 8> vec_reduce_c_sum, vec_reduce_c_sqr;
            vec_reduce_c_sum.insert(0, vec_reduce_c_sum1.template extract<4>(0));
            vec_reduce_c_sum.insert(1, vec_reduce_c_sum2.template extract<4>(0));
            vec_reduce_c_sqr.insert(0, vec_reduce_c_sqr1.template extract<4>(0));
            vec_reduce_c_sqr.insert(1, vec_reduce_c_sqr2.template extract<4>(0));
            aie::accum<accfloat, 8> acc_reduce_c_sum(vec_reduce_c_sum);
            aie::accum<accfloat, 8> acc_reduce_c_sqr(vec_reduce_c_sqr);
            // aie::print(acc_reduce_c_sum, true, "sum: ");
            // aie::print(acc_reduce_c_sqr, true, "sqr: ");

            auto Ex = aie::mul(acc_reduce_c_sum.template to_vector<dtype_ofm>(), param.div_factor);
            auto Ex_bf16 = Ex.template to_vector<bfloat16>();
            auto Ex2 = aie::mul(acc_reduce_c_sqr.template to_vector<dtype_ofm>(), param.div_factor);
            auto Var = aie::msc_square(Ex2, Ex_bf16);
            auto Var_eps = aie::add(Var, param.epsilon);
            auto Var_eps_bf16 = Var_eps.template to_vector<bfloat16>();
            auto inv_sqrt_bf16 = aie::invsqrt(Var_eps_bf16);
            // aie::print(inv_sqrt_bf16, true, "inv_sqrt: ");

            auto mean_inv_sqrt = aie::mul(Ex_bf16, inv_sqrt_bf16);
            auto mean_inv_sqrt_bf16 = mean_inv_sqrt.template to_vector<bfloat16>();
            // aie::print(mean_inv_sqrt_bf16, true, "mean_inv_sqrt: ");

            aie::store_v(o_mean_invsqrt_ptr, mean_inv_sqrt_bf16);
            o_mean_invsqrt_ptr = byte_incr(o_mean_invsqrt_ptr, 4 * PART1_OUTER_UNROLL_NUM * sizeof(dtype_ofm));
            aie::store_v(o_invsqrt_ptr, inv_sqrt_bf16);
            o_invsqrt_ptr = byte_incr(o_invsqrt_ptr, 4 * PART1_OUTER_UNROLL_NUM * sizeof(dtype_ofm));
        }
}

template <uint8_t batch_size, typename dtype_ifm, typename dtype_ofm>
requires(std::is_same_v<dtype_ifm, bfloat16>) && (std::is_same_v<dtype_ofm, bfloat16>) && (batch_size == 1)
NOINLINE void layer_norm_c8_part1(
    dtype_ifm * __restrict i_ptr,
    dtype_ofm * __restrict o_mean_invsqrt_ptr,
    dtype_ofm * __restrict o_invsqrt_ptr,
    layer_norm_c8_params_t &param)
{
    dtype_ofm *ofm_ptr = o_mean_invsqrt_ptr;

    // pointers to store current sum(x) & sum(x^2) along C dimension
    float OFM_DM_BANK *__restrict psum_ptr = (float OFM_DM_BANK *__restrict)ofm_ptr;
    float OFM_DM_BANK *__restrict psqr_ptr = (float OFM_DM_BANK *__restrict)ofm_ptr + param.height * param.width * 8;
    // pointers to load previous sum(x) & sum(x^2) along C dimension
    float OFM_DM_BANK *__restrict psum_ptr_init = (float OFM_DM_BANK *__restrict)ofm_ptr;
    float OFM_DM_BANK *__restrict psqr_ptr_init = (float OFM_DM_BANK *__restrict)ofm_ptr + param.height * param.width * 8;

    addr_t i_ptr_addr_count1 = 0;
    addr_t i_ptr_addr_count2 = 0;
    addr_t psum_ptr_addr_count = 0;
    addr_t psqr_ptr_addr_count = 0;
    addr_t psum_ptr_init_addr_count = 0;
    addr_t psqr_ptr_init_addr_count = 0;

    // clean up the output at the beginning
    int32_t OFM_DM_BANK *__restrict pcoeff_ptr_zeros = (int32_t OFM_DM_BANK *__restrict)ofm_ptr;
    aie::vector<int32_t, 32> vec_init = aie::broadcast<int32_t, 32>(0);
    if (param.depth_iter == 0)
    {
        // printf("[Kernel]: 1st iteration to clean up the OFM buffer\n");
		AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4, ) 
        for (int i = 0; i < param.init_loop; i++) {
            aie::store_v(pcoeff_ptr_zeros, vec_init);
            pcoeff_ptr_zeros += 32;
        }
    }

    // printf("[Kernel]: intermediate iterations to accumulate for sum(x) & sum(x^2)\n");
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(4, ) 
    for (int i = 0; i < param.outer_loop * param.inner_loop / PART1_INNER_UNROLL_NUM; i++) {
            // LOAD PSUM & PSQR
            auto vec_psum = aie::load_v<32>(psum_ptr_init);
            auto vec_psqr = aie::load_v<32>(psqr_ptr_init);
            psum_ptr_init = add_2d_ptr(psum_ptr_init, -32 * param.outer_loop + 32, param.outer_loop - 1, psum_ptr_init_addr_count, 32);
            psqr_ptr_init = add_2d_ptr(psqr_ptr_init, -32 * param.outer_loop + 32, param.outer_loop - 1, psqr_ptr_init_addr_count, 32);
            // aie::print(vec_psum, true, "vec_psum: ");
            // aie::print(vec_psqr, true, "vec_psqr: ");

            aie::accum<accfloat, 32> acc_sum, acc_sqr;
            acc_sum.template from_vector(vec_psum);
            acc_sqr.template from_vector(vec_psqr);

            // UNROLL 1
            auto vec_0 = aie::load_v<32>(i_ptr);
            i_ptr = i_ptr + param.i_ptr_inc4;
            acc_sum = aie::add(acc_sum, vec_0);
            acc_sqr = aie::mac_square(acc_sqr, vec_0);

            // UNROLL 2
            auto vec_1 = aie::load_v<32>(i_ptr);
            i_ptr = i_ptr + param.i_ptr_inc4;
            acc_sum = aie::add(acc_sum, vec_1);
            acc_sqr = aie::mac_square(acc_sqr, vec_1);

            // UNROLL 3
            auto vec_2 = aie::load_v<32>(i_ptr);
            i_ptr = i_ptr + param.i_ptr_inc4;
            acc_sum = aie::add(acc_sum, vec_2);
            acc_sqr = aie::mac_square(acc_sqr, vec_2);

            // UNROLL 4
            auto vec_3 = aie::load_v<32>(i_ptr);
            i_ptr = add_3d_ptr(i_ptr, param.i_ptr_offset, param.i_ptr_size1, i_ptr_addr_count1, param.i_ptr_inc1, param.i_ptr_size2, i_ptr_addr_count2, param.i_ptr_inc2);
            acc_sum = aie::add(acc_sum, vec_3);
            acc_sqr = aie::mac_square(acc_sqr, vec_3);
            // aie::print(vec_0, true, "vec_0: ");
            // aie::print(vec_1, true, "vec_1: ");
            // aie::print(vec_2, true, "vec_2: ");
            // aie::print(vec_3, true, "vec_3: ");

            // STORE PSUM & PSQR
            aie::store_v(psum_ptr, acc_sum.template to_vector<float>());
            aie::store_v(psqr_ptr, acc_sqr.template to_vector<float>());
            psum_ptr = add_2d_ptr(psum_ptr, -32 * param.outer_loop + 32, param.outer_loop - 1, psum_ptr_addr_count, 32);
            psqr_ptr = add_2d_ptr(psqr_ptr, -32 * param.outer_loop + 32, param.outer_loop - 1, psqr_ptr_addr_count, 32);
        }

    // calculate the coefficients at the last sub-volume in C dimension
    if (param.depth_iter == param.loop_num_channel - 1)
    {
        // printf("[Kernel]: final iteration to calculate the E(x) & Var(x)\n");
        coeff_computation<dtype_ofm, float>(o_mean_invsqrt_ptr, o_invsqrt_ptr, psum_ptr, psqr_ptr, param);
    }
    param.depth_iter++;
}



template <uint8_t batch_size, typename dtype_ifm, typename dtype_ofm, typename config_ifm, typename config_ofm>
    requires(std::is_same_v<dtype_ifm, bfloat16>) && (std::is_same_v<dtype_ofm, bfloat16>) && (batch_size == 1)
NOINLINE void layer_norm_c8_part1(
    adf::input_buffer_conf<dtype_ifm, config_ifm> &__restrict ifm,
    adf::output_buffer_conf<dtype_ofm, config_ofm> &__restrict ofm,
    layer_norm_c8_params_t &param)
{
    dtype_ifm IFM_DM_BANK *__restrict ifm_ptr =
        (dtype_ifm IFM_DM_BANK *__restrict)ifm.data() + param.overlay_row_offset;
    dtype_ofm OFM_DM_BANK *__restrict ofm_ptr =
        (dtype_ofm OFM_DM_BANK *__restrict)ofm.data();

    dtype_ofm *o_invsqrt_ptr = ofm_ptr + param.height * param.width;

    layer_norm_c8_part1<batch_size, dtype_ifm, dtype_ofm>(ifm_ptr, ofm_ptr, o_invsqrt_ptr, param);
}

template <uint8_t batch_size, typename dtype>
    requires(std::is_same_v<dtype, bfloat16>) && (batch_size == 1)
NOINLINE void layer_norm_c8_part2(
    dtype * __restrict ifm_ptr,
    dtype * __restrict gamma_ptr0,
    dtype * __restrict beta_ptr0,
    dtype * __restrict mean_invsqrt_ptr0,
    dtype * __restrict invsqrt_ptr0,
    dtype * __restrict ofm_ptr,
    layer_norm_c8_params_t &param)
{
    dtype *gamma_ptr1 = gamma_ptr0 + 32;
    dtype *gamma_ptr2 = gamma_ptr0 + 32 * 2;
    dtype *gamma_ptr3 = gamma_ptr0 + 32 * 3;
    dtype *beta_ptr1 = beta_ptr0 + 32;
    dtype *beta_ptr2 = beta_ptr0 + 32 * 2;
    dtype *beta_ptr3 = beta_ptr0 + 32 * 3;
    dtype *mean_invsqrt_ptr1 = mean_invsqrt_ptr0 + 1;
    dtype *mean_invsqrt_ptr2 = mean_invsqrt_ptr0 + 2;
    dtype *mean_invsqrt_ptr3 = mean_invsqrt_ptr0 + 3;
    dtype *invsqrt_ptr1 = invsqrt_ptr0 + 1;
    dtype *invsqrt_ptr2 = invsqrt_ptr0 + 2;
    dtype *invsqrt_ptr3 = invsqrt_ptr0 + 3;

    aie::vector<dtype, 32> vec_gamma0, vec_gamma1, vec_gamma2, vec_gamma3;
    aie::vector<dtype, 32> vec_beta0, vec_beta1, vec_beta2, vec_beta3;
    aie::vector<dtype, 32> vec_invsqrt, vec_mean_invsqrt;

    addr_t gamma_ptr0_addr_cnt = 0;
    addr_t gamma_ptr1_addr_cnt = 0;
    addr_t gamma_ptr2_addr_cnt = 0;
    addr_t gamma_ptr3_addr_cnt = 0;
    addr_t beta_ptr0_addr_cnt = 0;
    addr_t beta_ptr1_addr_cnt = 0;
    addr_t beta_ptr2_addr_cnt = 0;
    addr_t beta_ptr3_addr_cnt = 0;
    addr_t invsqrt_ptr0_addr_cnt = 0;
    addr_t invsqrt_ptr1_addr_cnt = 0;
    addr_t invsqrt_ptr2_addr_cnt = 0;
    addr_t invsqrt_ptr3_addr_cnt = 0;
    addr_t mean_invsqrt_ptr0_addr_cnt = 0;
    addr_t mean_invsqrt_ptr1_addr_cnt = 0;
    addr_t mean_invsqrt_ptr2_addr_cnt = 0;
    addr_t mean_invsqrt_ptr3_addr_cnt = 0;
    addr_t ifm_ptr_addr_count1 = 0;
    addr_t ifm_ptr_addr_count2 = 0;
    addr_t ofm_ptr_addr_count1 = 0;
    addr_t ofm_ptr_addr_count2 = 0;

	AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(4, ) 
    for (int i = 0; i < param.outer_loop * param.inner_loop / PART2_INNER_UNROLL_NUM; i++) {
        // printf("Iteration-%d\n", i);
        // load gamma/beta vector
        // UNROLL 1
        vec_gamma0 = aie::load_v<32>(gamma_ptr0);
        gamma_ptr0 = add_2d_ptr(gamma_ptr0, 32 * PART2_INNER_UNROLL_NUM, param.outer_loop - 1, gamma_ptr0_addr_cnt, 0);
        vec_beta0 = aie::load_v<32>(beta_ptr0);
        beta_ptr0 = add_2d_ptr(beta_ptr0, 32 * PART2_INNER_UNROLL_NUM, param.outer_loop - 1, beta_ptr0_addr_cnt, 0);
        // UNROLL 2
        vec_gamma1 = aie::load_v<32>(gamma_ptr1);
        gamma_ptr1 = add_2d_ptr(gamma_ptr1, 32 * PART2_INNER_UNROLL_NUM, param.outer_loop - 1, gamma_ptr1_addr_cnt, 0);
        vec_beta1 = aie::load_v<32>(beta_ptr1);
        beta_ptr1 = add_2d_ptr(beta_ptr1, 32 * PART2_INNER_UNROLL_NUM, param.outer_loop - 1, beta_ptr1_addr_cnt, 0);
        // UNROLL 3
        vec_gamma2 = aie::load_v<32>(gamma_ptr2);
        gamma_ptr2 = add_2d_ptr(gamma_ptr2, 32 * PART2_INNER_UNROLL_NUM, param.outer_loop - 1, gamma_ptr2_addr_cnt, 0);
        vec_beta2 = aie::load_v<32>(beta_ptr2);
        beta_ptr2 = add_2d_ptr(beta_ptr2, 32 * PART2_INNER_UNROLL_NUM, param.outer_loop - 1, beta_ptr2_addr_cnt, 0);
        // UNROLL 4
        vec_gamma3 = aie::load_v<32>(gamma_ptr3);
        gamma_ptr3 = add_2d_ptr(gamma_ptr3, 32 * PART2_INNER_UNROLL_NUM, param.outer_loop - 1, gamma_ptr3_addr_cnt, 0);
        vec_beta3 = aie::load_v<32>(beta_ptr3);
        beta_ptr3 = add_2d_ptr(beta_ptr3, 32 * PART2_INNER_UNROLL_NUM, param.outer_loop - 1, beta_ptr3_addr_cnt, 0);
        // aie::print(vec_gamma0, true, "gamma0: ");
        // aie::print(vec_gamma1, true, "gamma1: ");
        // aie::print(vec_gamma2, true, "gamma2: ");
        // aie::print(vec_gamma3, true, "gamma3: ");
        // aie::print(vec_beta0, true, "beta0: ");
        // aie::print(vec_beta1, true, "beta1: ");
        // aie::print(vec_beta2, true, "beta2: ");
        // aie::print(vec_beta3, true, "beta3: ");

        // load mean_invsqrt/invsqrt in W4
        dtype invsqrt0 = *invsqrt_ptr0;
        invsqrt_ptr0 = add_2d_ptr(invsqrt_ptr0, param.coeff_ptr_offset, param.i_ptr_size1, invsqrt_ptr0_addr_cnt, 4);
        dtype invsqrt1 = *invsqrt_ptr1;
        invsqrt_ptr1 = add_2d_ptr(invsqrt_ptr1, param.coeff_ptr_offset, param.i_ptr_size1, invsqrt_ptr1_addr_cnt, 4);
        dtype invsqrt2 = *invsqrt_ptr2;
        invsqrt_ptr2 = add_2d_ptr(invsqrt_ptr2, param.coeff_ptr_offset, param.i_ptr_size1, invsqrt_ptr2_addr_cnt, 4);
        dtype invsqrt3 = *invsqrt_ptr3;
        invsqrt_ptr3 = add_2d_ptr(invsqrt_ptr3, param.coeff_ptr_offset, param.i_ptr_size1, invsqrt_ptr3_addr_cnt, 4);
        dtype mean_invsqrt0 = *mean_invsqrt_ptr0;
        mean_invsqrt_ptr0 = add_2d_ptr(mean_invsqrt_ptr0, param.coeff_ptr_offset, param.i_ptr_size1, mean_invsqrt_ptr0_addr_cnt, 4);
        dtype mean_invsqrt1 = *mean_invsqrt_ptr1;
        mean_invsqrt_ptr1 = add_2d_ptr(mean_invsqrt_ptr1, param.coeff_ptr_offset, param.i_ptr_size1, mean_invsqrt_ptr1_addr_cnt, 4);
        dtype mean_invsqrt2 = *mean_invsqrt_ptr2;
        mean_invsqrt_ptr2 = add_2d_ptr(mean_invsqrt_ptr2, param.coeff_ptr_offset, param.i_ptr_size1, mean_invsqrt_ptr2_addr_cnt, 4);
        dtype mean_invsqrt3 = *mean_invsqrt_ptr3;
        mean_invsqrt_ptr3 = add_2d_ptr(mean_invsqrt_ptr3, param.coeff_ptr_offset, param.i_ptr_size1, mean_invsqrt_ptr3_addr_cnt, 4);

        auto vec_invsqrt_C0 = aie::broadcast<dtype, 8>(invsqrt0);
        auto vec_invsqrt_C1 = aie::broadcast<dtype, 8>(invsqrt1);
        auto vec_invsqrt_C2 = aie::broadcast<dtype, 8>(invsqrt2);
        auto vec_invsqrt_C3 = aie::broadcast<dtype, 8>(invsqrt3);
        auto vec_mean_invsqrt_C0 = aie::broadcast<dtype, 8>(mean_invsqrt0);
        auto vec_mean_invsqrt_C1 = aie::broadcast<dtype, 8>(mean_invsqrt1);
        auto vec_mean_invsqrt_C2 = aie::broadcast<dtype, 8>(mean_invsqrt2);
        auto vec_mean_invsqrt_C3 = aie::broadcast<dtype, 8>(mean_invsqrt3);

        // duplication for W4C8
        vec_mean_invsqrt.insert(0, vec_mean_invsqrt_C0);
        vec_mean_invsqrt.insert(1, vec_mean_invsqrt_C1);
        vec_mean_invsqrt.insert(2, vec_mean_invsqrt_C2);
        vec_mean_invsqrt.insert(3, vec_mean_invsqrt_C3);
        vec_invsqrt.insert(0, vec_invsqrt_C0);
        vec_invsqrt.insert(1, vec_invsqrt_C1);
        vec_invsqrt.insert(2, vec_invsqrt_C2);
        vec_invsqrt.insert(3, vec_invsqrt_C3);
        // aie::print(vec_invsqrt, true, "invsqrt: ");
        // aie::print(vec_mean_invsqrt, true, "mean_invsqrt: ");

        // UNROLL 1
        auto vec_0 = aie::load_v<32>(ifm_ptr);
        ifm_ptr = ifm_ptr + param.i_ptr_inc4;
        // UNROLL 2
        auto vec_1 = aie::load_v<32>(ifm_ptr);
        ifm_ptr = ifm_ptr + param.i_ptr_inc4;
        // UNROLL 3
        auto vec_2 = aie::load_v<32>(ifm_ptr);
        ifm_ptr = ifm_ptr + param.i_ptr_inc4;
        // UNROLL 4
        auto vec_3 = aie::load_v<32>(ifm_ptr);
        ifm_ptr = add_3d_ptr(ifm_ptr, param.i_ptr_offset, param.i_ptr_size1, ifm_ptr_addr_count1, param.i_ptr_inc1, param.i_ptr_size2, ifm_ptr_addr_count2, param.i_ptr_inc2);
        // aie::print(vec_0, true, "vec_0: ");
        // aie::print(vec_1, true, "vec_1: ");
        // aie::print(vec_2, true, "vec_2: ");
        // aie::print(vec_3, true, "vec_3: ");

        aie::accum<accfloat, 32> acc_beta0(vec_beta0);
        aie::accum<accfloat, 32> acc_beta1(vec_beta1);
        aie::accum<accfloat, 32> acc_beta2(vec_beta2);
        aie::accum<accfloat, 32> acc_beta3(vec_beta3);

        // UNROLL 1
        auto acc_mean_invsqrt0 = aie::mul(vec_0, vec_invsqrt);
        acc_mean_invsqrt0 = aie::sub(acc_mean_invsqrt0, vec_mean_invsqrt);
        // aie::print(acc_mean_invsqrt0, true, "inner_res0: ");
        acc_beta0 = aie::mac(acc_beta0, acc_mean_invsqrt0.template to_vector<dtype>(), vec_gamma0);
        // UNROLL 2
        auto acc_mean_invsqrt1 = aie::mul(vec_1, vec_invsqrt);
        acc_mean_invsqrt1 = aie::sub(acc_mean_invsqrt1, vec_mean_invsqrt);
        // aie::print(acc_mean_invsqrt1, true, "inner_res1: ");
        acc_beta1 = aie::mac(acc_beta1, acc_mean_invsqrt1.template to_vector<dtype>(), vec_gamma1);
        // UNROLL 3
        auto acc_mean_invsqrt2 = aie::mul(vec_2, vec_invsqrt);
        acc_mean_invsqrt2 = aie::sub(acc_mean_invsqrt2, vec_mean_invsqrt);
        // aie::print(acc_mean_invsqrt2, true, "inner_res2: ");
        acc_beta2 = aie::mac(acc_beta2, acc_mean_invsqrt2.template to_vector<dtype>(), vec_gamma2);
        // UNROLL 4
        auto acc_mean_invsqrt3 = aie::mul(vec_3, vec_invsqrt);
        acc_mean_invsqrt3 = aie::sub(acc_mean_invsqrt3, vec_mean_invsqrt);
        // aie::print(acc_mean_invsqrt3, true, "inner_res3: ");
        acc_beta3 = aie::mac(acc_beta3, acc_mean_invsqrt3.template to_vector<dtype>(), vec_gamma3);
        // aie::print(acc_beta0, true, "outer_res0: ");
        // aie::print(acc_beta1, true, "outer_res1: ");
        // aie::print(acc_beta2, true, "outer_res2: ");
        // aie::print(acc_beta3, true, "outer_res3: ");
        
        // STORE OFM
        // UNROLL 1
        aie::store_v(ofm_ptr, acc_beta0.template to_vector<dtype>());
        ofm_ptr = ofm_ptr + param.i_ptr_inc4;
        // UNROLL 2
        aie::store_v(ofm_ptr, acc_beta1.template to_vector<dtype>());
        ofm_ptr = ofm_ptr + param.i_ptr_inc4;
        // UNROLL 3
        aie::store_v(ofm_ptr, acc_beta2.template to_vector<dtype>());
        ofm_ptr = ofm_ptr + param.i_ptr_inc4;
        // UNROLL 4
        aie::store_v(ofm_ptr, acc_beta3.template to_vector<dtype>());
        ofm_ptr = add_3d_ptr(ofm_ptr, param.i_ptr_offset, param.i_ptr_size1, ofm_ptr_addr_count1, param.i_ptr_inc1, param.i_ptr_size2, ofm_ptr_addr_count2, param.i_ptr_inc2);
    }
    // Iterate the full-volume along C-W-H
    if (param.c_idx == param.loop_num_channel - 1)
    {
        param.c_idx = 0;
        if (param.w_idx == param.loop_num_width - 1)
        {
            param.w_idx = 0;
            param.h_idx++;
        }
        else
        {
            param.w_idx++;
        }
    }
    else
    {
        param.c_idx++;
    }
}

template <uint8_t batch_size, typename dtype, typename config_ifm, typename config_wts, typename config_ofm>
    requires(std::is_same_v<dtype, bfloat16>) && (batch_size == 1)
NOINLINE void layer_norm_c8_part2(
    adf::input_buffer_conf<dtype, config_ifm> &__restrict ifm,
    adf::input_buffer_conf<dtype, config_wts> &__restrict wts,
    adf::output_buffer_conf<dtype, config_ofm> &__restrict ofm,
    layer_norm_c8_params_t &param)
{
    dtype IFM_DM_BANK *__restrict ifm_ptr =
        (dtype IFM_DM_BANK *__restrict)ifm.data() + param.overlay_row_offset;
    dtype WTS_DM_BANK *__restrict wts_ptr =
        (dtype WTS_DM_BANK *__restrict)wts.data();
    dtype OFM_DM_BANK *__restrict ofm_ptr =
        (dtype OFM_DM_BANK *__restrict)ofm.data();

    dtype *gamma_ptr0 = wts_ptr + 4 * param.channel * param.c_idx;
    dtype *beta_ptr0 = gamma_ptr0 + 4 * param.channel * param.loop_num_channel;
    dtype *mean_invsqrt_ptr0 = wts_ptr + 2 * 4 * param.channel * param.loop_num_channel + param.width * param.loop_num_width * param.height * param.h_idx + param.width * param.w_idx;
    dtype *invsqrt_ptr0 = mean_invsqrt_ptr0 + param.height * param.loop_num_height * param.width * param.loop_num_width;
    
    layer_norm_c8_part2<batch_size, dtype>(ifm_ptr, gamma_ptr0, beta_ptr0, mean_invsqrt_ptr0, invsqrt_ptr0, ofm_ptr, param);
}
#endif //__LAYER_NORM_C8_H__
