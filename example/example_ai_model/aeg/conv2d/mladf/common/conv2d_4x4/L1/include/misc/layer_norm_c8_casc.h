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

// this kernel implements layer normalization in both bfloat16 & int8 datatypes
#ifndef __LAYER_NORM_C8_CASC_H__
#define __LAYER_NORM_C8_CASC_H__

#include "aie.hpp"
#include "aie_api/utils.hpp"
#include <adf.h>

#include "layer_norm_c8_params.h" // shares the same RTP with standalone kernel

#include "mllib_const.h"
#include "utils.h"
#include "utils.hpp"

using namespace aie;

template <typename dtype_ofm, typename dtype_acc>
    requires(std::is_same_v<dtype_ofm, int16_t>) && (std::is_same_v<dtype_acc, int32_t>)
INLINE void coeff_computation_casc(dtype_ofm* o_mean_invsqrt_ptr, dtype_ofm* o_invsqrt_ptr, dtype_acc* psum_ptr, dtype_acc* psqr_ptr, const layer_norm_c8_params_t& param){

	AIE_PREPARE_FOR_PIPELINING 
	AIE_LOOP_RANGE(4, ) 
	for (int i = 0; i < param.outer_loop / PART1_OUTER_UNROLL_NUM; i++){
            // UNROLL 1
            aie::vector<dtype_acc,32> vec_psum_tmp1 = aie::load_v<32>(psum_ptr);
            aie::vector<dtype_acc,32> vec_psqr_tmp1 = aie::load_v<32>(psqr_ptr);
			psum_ptr = byte_incr(psum_ptr, 32*sizeof(dtype_acc));
			psqr_ptr = byte_incr(psqr_ptr, 32*sizeof(dtype_acc));
            // UNROLL 2
            aie::vector<dtype_acc,32> vec_psum_tmp2 = aie::load_v<32>(psum_ptr);
            aie::vector<dtype_acc,32> vec_psqr_tmp2 = aie::load_v<32>(psqr_ptr);
			psum_ptr = byte_incr(psum_ptr, 32*sizeof(dtype_acc));
			psqr_ptr = byte_incr(psqr_ptr, 32*sizeof(dtype_acc));
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
            aie::vector<dtype_acc,8> vec_reduce_c_sum_int, vec_reduce_c_sqr_int;
            vec_reduce_c_sum_int.insert(0, vec_reduce_c_sum_int1.template extract<4>(0));
            vec_reduce_c_sum_int.insert(1, vec_reduce_c_sum_int2.template extract<4>(0));
            vec_reduce_c_sqr_int.insert(0, vec_reduce_c_sqr_int1.template extract<4>(0));
            vec_reduce_c_sqr_int.insert(1, vec_reduce_c_sqr_int2.template extract<4>(0));

            accum<accfloat,8> acc_reduce_c_sum(aie::to_float(vec_reduce_c_sum_int, param.ifm_shift));
            auto vec_reduce_c_sum = acc_reduce_c_sum.template to_vector<bfloat16>();
            accum<accfloat,8> acc_reduce_c_sqr(aie::to_float(vec_reduce_c_sqr_int, 2 * param.ifm_shift));
            auto vec_reduce_c_sqr = acc_reduce_c_sqr.template to_vector<bfloat16>();
            auto Ex = aie::mul(vec_reduce_c_sum, param.div_factor);
            auto Ex_bf16 = Ex.template to_vector<bfloat16>();
            auto Ex2 = aie::mul(vec_reduce_c_sqr, param.div_factor);
            auto Var = aie::msc_square(Ex2, Ex_bf16);
            auto Var_eps = aie::add(Var, param.epsilon);
            auto Var_eps_bf16 = Var_eps.template to_vector<bfloat16>();
            auto inv_sqrt = aie::invsqrt(Var_eps_bf16);
            auto inv_sqrt_int = aie::to_fixed(inv_sqrt, 16);
            accum<acc32,8> acc_inv_sqrt(inv_sqrt_int);
            // aie::print(acc_inv_sqrt, true, "inv_sqrt: ");

            auto mean_inv_sqrt = aie::mul(Ex_bf16, inv_sqrt);
            auto mean_inv_sqrt_bf16 = mean_inv_sqrt.template to_vector<bfloat16>();
            auto mean_inv_sqrt_int = aie::to_fixed(mean_inv_sqrt_bf16, 16);
            accum<acc32,8> acc_mean_inv_sqrt(mean_inv_sqrt_int);
            // aie::print(acc_mean_inv_sqrt, true, "mean_inv_sqrt: ");

			aie::store_v(o_mean_invsqrt_ptr, acc_mean_inv_sqrt.template to_vector<dtype_ofm>(16 - param.beta_shift + param.gamma_shift));
			o_mean_invsqrt_ptr = byte_incr(o_mean_invsqrt_ptr, 4*PART1_OUTER_UNROLL_NUM*sizeof(dtype_ofm));
			aie::store_v(o_invsqrt_ptr, acc_inv_sqrt.template to_vector<dtype_ofm>(16 - param.beta_shift + param.gamma_shift + param.ifm_shift));
			o_invsqrt_ptr = byte_incr(o_invsqrt_ptr, 4*PART1_OUTER_UNROLL_NUM*sizeof(dtype_ofm));
	}
}

template <uint8_t batch_size, casc_mode_t casc_mode, typename dtype_ifm, typename dtype_ofm, typename dtype_casc, typename config_ifm, typename config_ofm>
    requires(std::is_same_v<dtype_ifm, int8_t>) && (std::is_same_v<dtype_ofm, int16_t>) && (batch_size == 1) && (casc_mode == CASC_START || casc_mode == CASC_END) && (std::is_same_v<dtype_casc, acc32>)
NOINLINE void layer_norm_c8_part1_casc
(
    adf::input_buffer_conf <dtype_ifm, config_ifm>       & __restrict ifm,
    adf::output_buffer_conf<dtype_ofm, config_ofm>       & __restrict ofm,
    layer_norm_c8_params_t &param,
    IN_CASC<dtype_casc>* casc_acc_in,
    OUT_CASC<dtype_casc>* casc_acc_out
) {
    // printf("[Kernel]: param.depth_iter = %d\n", param.depth_iter);
	dtype_ifm IFM_DM_BANK * __restrict ifm_ptr =
      (dtype_ifm IFM_DM_BANK * __restrict)ifm.data() + param.overlay_row_offset;
  	dtype_ofm OFM_DM_BANK * __restrict ofm_ptr =
      (dtype_ofm OFM_DM_BANK * __restrict)ofm.data();

	dtype_ifm* i_ptr = ifm_ptr;
	dtype_ofm* o_mean_invsqrt_ptr = ofm_ptr;
	dtype_ofm* o_invsqrt_ptr = ofm_ptr + param.height * param.width;

    // pointers to store current sum(x) & sum(x^2) along C dimension
	int32_t OFM_DM_BANK * __restrict psum_ptr = (int32_t OFM_DM_BANK * __restrict)ofm_ptr;
	int32_t OFM_DM_BANK * __restrict psqr_ptr = (int32_t OFM_DM_BANK * __restrict)ofm_ptr + param.height * param.width * 8;
    // pointers to load previous sum(x) & sum(x^2) along C dimension
	int32_t OFM_DM_BANK * __restrict psum_ptr_init = (int32_t OFM_DM_BANK * __restrict)ofm_ptr;
	int32_t OFM_DM_BANK * __restrict psqr_ptr_init = (int32_t OFM_DM_BANK * __restrict)ofm_ptr + param.height * param.width * 8;

	addr_t i_ptr_addr_count1 = 0;
	addr_t i_ptr_addr_count2 = 0;
	addr_t psum_ptr_addr_count = 0;
	addr_t psqr_ptr_addr_count = 0;
	addr_t psum_ptr_init_addr_count = 0;
	addr_t psqr_ptr_init_addr_count = 0;

    // clean up the output at the beginning
	int32_t OFM_DM_BANK * __restrict pcoeff_ptr_zeros = (int32_t OFM_DM_BANK * __restrict)ofm_ptr;
	aie::vector<int32_t, 32> vec_init = aie::broadcast<int32_t, 32>(0);
	if (param.depth_iter == 0){
        // printf("[Kernel]: 1st iteration to clean up the OFM buffer\n");
		AIE_PREPARE_FOR_PIPELINING 
		AIE_LOOP_RANGE(4, ) 
		for (int i = 0; i < param.init_loop; i++){
				aie::store_v(pcoeff_ptr_zeros, vec_init);
				pcoeff_ptr_zeros += 32;
		}
	}

    // printf("[Kernel]: intermediate iterations to accumulate for sum(x) & sum(x^2)\n");
	AIE_PREPARE_FOR_PIPELINING 
	AIE_LOOP_RANGE(4, ) 
	for (int i = 0; i < param.outer_loop * param.inner_loop / PART1_INNER_UNROLL_NUM; i++){
			// LOAD PSUM & PSQR
			aie::vector<int32_t,32> vec_psum = aie::load_v<32>(psum_ptr_init);
			aie::vector<int32_t,32> vec_psqr = aie::load_v<32>(psqr_ptr_init);
			psum_ptr_init = add_2d_ptr(psum_ptr_init, -32*param.outer_loop+32, param.outer_loop - 1, psum_ptr_init_addr_count, 32);
			psqr_ptr_init = add_2d_ptr(psqr_ptr_init, -32*param.outer_loop+32, param.outer_loop - 1, psqr_ptr_init_addr_count, 32);
            // aie::print(vec_psum, true, "vec_psum: ");
            // aie::print(vec_psqr, true, "vec_psqr: ");

            aie::accum<acc32,32> acc_sum, acc_sqr;
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
			psum_ptr = add_2d_ptr(psum_ptr, -32*param.outer_loop+32, param.outer_loop - 1, psum_ptr_addr_count, 32);
			psqr_ptr = add_2d_ptr(psqr_ptr, -32*param.outer_loop+32, param.outer_loop - 1, psqr_ptr_addr_count, 32);
	}

    // at the last sub-volume in C dimension
	// for CASC_START kernel, directly pass the intermediate accumulated partial SUM/SQR to CASC_END kernel
    // for CASC_END kernel, accumulate the partial SUM/SQR from CASC_START firstly, then calculate the coefficients
	psum_ptr_addr_count = 0;
	psqr_ptr_addr_count = 0;
	if (param.depth_iter == param.loop_num_channel - 1) {
        aie::accum<dtype_casc,32> acc_psum, acc_psqr;
        if constexpr (casc_mode == CASC_START) {
            // printf("[CASC_START]: final iteration egress %d rounds of data\n", param.outer_loop);
	        AIE_PREPARE_FOR_PIPELINING 
	        AIE_LOOP_RANGE(4, ) 
	        for (int i = 0; i < param.outer_loop; i++){
                    acc_psum.from_vector(aie::load_v<32>(psum_ptr),0);
                    acc_psqr.from_vector(aie::load_v<32>(psqr_ptr),0);

                    writeincr(casc_acc_out, acc_psum);
                    writeincr(casc_acc_out, acc_psqr);

			        psum_ptr = add_2d_ptr(psum_ptr, -32*param.outer_loop+32, param.outer_loop - 1, psum_ptr_addr_count, 32);
			        psqr_ptr = add_2d_ptr(psqr_ptr, -32*param.outer_loop+32, param.outer_loop - 1, psqr_ptr_addr_count, 32);
                }
        } else if constexpr (casc_mode == CASC_END) {
            // printf("[CASC_END]: final iteration ingress %d rounds of data\n", param.outer_loop);
	        AIE_PREPARE_FOR_PIPELINING 
	        AIE_LOOP_RANGE(4, ) 
	        for (int i = 0; i < param.outer_loop; i++){
                    acc_psum = readincr_v<32>(casc_acc_in);
                    acc_psqr = readincr_v<32>(casc_acc_in);

                    aie::vector<int32_t,32> vec_psum = aie::load_v<32>(psum_ptr);
                    aie::vector<int32_t,32> vec_psqr = aie::load_v<32>(psqr_ptr);

			        acc_psum = aie::add(acc_psum, vec_psum);
			        acc_psqr = aie::add(acc_psqr, vec_psqr);

			        aie::store_v(psum_ptr, acc_psum.template to_vector<int32_t>());
                    aie::store_v(psqr_ptr, acc_psqr.template to_vector<int32_t>());

			        psum_ptr = add_2d_ptr(psum_ptr, -32*param.outer_loop+32, param.outer_loop - 1, psum_ptr_addr_count, 32);
			        psqr_ptr = add_2d_ptr(psqr_ptr, -32*param.outer_loop+32, param.outer_loop - 1, psqr_ptr_addr_count, 32);
                }
            // XXX: only happens in CASC_END kernel at the last C iteration
		    coeff_computation_casc<dtype_ofm, int32_t>(o_mean_invsqrt_ptr, o_invsqrt_ptr, psum_ptr, psqr_ptr, param);
        }
        // TODO: add CASC_MID support if needed
	}
	param.depth_iter++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename dtype_ofm, typename dtype_acc>
    requires(std::is_same_v<dtype_ofm, bfloat16>) && (std::is_same_v<dtype_acc, float>)
INLINE void coeff_computation_casc(dtype_ofm* o_mean_invsqrt_ptr, dtype_ofm* o_invsqrt_ptr, dtype_acc* psum_ptr, dtype_acc* psqr_ptr, const layer_norm_c8_params_t& param){

	AIE_PREPARE_FOR_PIPELINING 
	AIE_LOOP_RANGE(4, ) 
	for (int i = 0; i < param.outer_loop / PART1_OUTER_UNROLL_NUM; i++){
            // UNROLL 1
            auto vec_psum_tmp1 = aie::load_v<32>(psum_ptr);
            auto vec_psqr_tmp1 = aie::load_v<32>(psqr_ptr);
			psum_ptr = byte_incr(psum_ptr, 32*sizeof(dtype_acc));
			psqr_ptr = byte_incr(psqr_ptr, 32*sizeof(dtype_acc));
            // UNROLL 2
            auto vec_psum_tmp2 = aie::load_v<32>(psum_ptr);
            auto vec_psqr_tmp2 = aie::load_v<32>(psqr_ptr);
			psum_ptr = byte_incr(psum_ptr, 32*sizeof(dtype_acc));
			psqr_ptr = byte_incr(psqr_ptr, 32*sizeof(dtype_acc));
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
            aie::vector<dtype_acc,8> vec_reduce_c_sum, vec_reduce_c_sqr;
            vec_reduce_c_sum.insert(0, vec_reduce_c_sum1.template extract<4>(0));
            vec_reduce_c_sum.insert(1, vec_reduce_c_sum2.template extract<4>(0));
            vec_reduce_c_sqr.insert(0, vec_reduce_c_sqr1.template extract<4>(0));
            vec_reduce_c_sqr.insert(1, vec_reduce_c_sqr2.template extract<4>(0));
            aie::accum<accfloat,8> acc_reduce_c_sum(vec_reduce_c_sum);
            aie::accum<accfloat,8> acc_reduce_c_sqr(vec_reduce_c_sqr);
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
			o_mean_invsqrt_ptr = byte_incr(o_mean_invsqrt_ptr, 4*PART1_OUTER_UNROLL_NUM*sizeof(dtype_ofm));
			aie::store_v(o_invsqrt_ptr, inv_sqrt_bf16);
			o_invsqrt_ptr = byte_incr(o_invsqrt_ptr, 4*PART1_OUTER_UNROLL_NUM*sizeof(dtype_ofm));
	}
}

template <uint8_t batch_size, casc_mode_t casc_mode, typename dtype_ifm, typename dtype_ofm, typename dtype_casc, typename config_ifm, typename config_ofm>
    requires(std::is_same_v<dtype_ifm, bfloat16>) && (std::is_same_v<dtype_ofm, bfloat16>) && (batch_size == 1) && (casc_mode == CASC_START || casc_mode == CASC_END) && (std::is_same_v<dtype_casc, accfloat>)
NOINLINE void layer_norm_c8_part1_casc
(
    adf::input_buffer_conf <dtype_ifm, config_ifm>       & __restrict ifm,
    adf::output_buffer_conf<dtype_ofm, config_ofm>       & __restrict ofm,
    layer_norm_c8_params_t &param,
    IN_CASC<dtype_casc>* casc_acc_in,
    OUT_CASC<dtype_casc>* casc_acc_out
) {
    // printf("[Kernel]: param.depth_iter = %d\n", param.depth_iter);
	dtype_ifm IFM_DM_BANK * __restrict ifm_ptr =
      (dtype_ifm IFM_DM_BANK * __restrict)ifm.data() + param.overlay_row_offset;
  	dtype_ofm OFM_DM_BANK * __restrict ofm_ptr =
      (dtype_ofm OFM_DM_BANK * __restrict)ofm.data();

	dtype_ifm* i_ptr = ifm_ptr;
	dtype_ofm* o_mean_invsqrt_ptr = ofm_ptr;
	dtype_ofm* o_invsqrt_ptr = ofm_ptr + param.height * param.width;

    // pointers to store current sum(x) & sum(x^2) along C dimension
	float OFM_DM_BANK * __restrict psum_ptr = (float OFM_DM_BANK * __restrict)ofm_ptr;
	float OFM_DM_BANK * __restrict psqr_ptr = (float OFM_DM_BANK * __restrict)ofm_ptr + param.height * param.width * 8;
    // pointers to load previous sum(x) & sum(x^2) along C dimension
	float OFM_DM_BANK * __restrict psum_ptr_init = (float OFM_DM_BANK * __restrict)ofm_ptr;
	float OFM_DM_BANK * __restrict psqr_ptr_init = (float OFM_DM_BANK * __restrict)ofm_ptr + param.height * param.width * 8;

	addr_t i_ptr_addr_count1 = 0;
	addr_t i_ptr_addr_count2 = 0;
	addr_t psum_ptr_addr_count = 0;
	addr_t psqr_ptr_addr_count = 0;
	addr_t psum_ptr_init_addr_count = 0;
	addr_t psqr_ptr_init_addr_count = 0;

    // clean up the output at the beginning
	int32_t OFM_DM_BANK * __restrict pcoeff_ptr_zeros = (int32_t OFM_DM_BANK * __restrict)ofm_ptr;
	aie::vector<int32_t, 32> vec_init = aie::broadcast<int32_t, 32>(0);
	if (param.depth_iter == 0){
        // printf("[Kernel]: 1st iteration to clean up the OFM buffer\n");
		AIE_PREPARE_FOR_PIPELINING 
		AIE_LOOP_RANGE(4, ) 
		for (int i = 0; i < param.init_loop; i++){
				aie::store_v(pcoeff_ptr_zeros, vec_init);
				pcoeff_ptr_zeros += 32;
		}
	}

    // printf("[Kernel]: intermediate iterations to accumulate for sum(x) & sum(x^2)\n");
	AIE_PREPARE_FOR_PIPELINING 
	AIE_LOOP_RANGE(4, ) 
	for (int i = 0; i < param.outer_loop * param.inner_loop / PART1_INNER_UNROLL_NUM; i++){
			// LOAD PSUM & PSQR
			auto vec_psum = aie::load_v<32>(psum_ptr_init);
			auto vec_psqr = aie::load_v<32>(psqr_ptr_init);
			psum_ptr_init = add_2d_ptr(psum_ptr_init, -32*param.outer_loop+32, param.outer_loop - 1, psum_ptr_init_addr_count, 32);
			psqr_ptr_init = add_2d_ptr(psqr_ptr_init, -32*param.outer_loop+32, param.outer_loop - 1, psqr_ptr_init_addr_count, 32);
            // aie::print(vec_psum, true, "vec_psum: ");
            // aie::print(vec_psqr, true, "vec_psqr: ");

            aie::accum<accfloat,32> acc_sum, acc_sqr;
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
			psum_ptr = add_2d_ptr(psum_ptr, -32*param.outer_loop+32, param.outer_loop - 1, psum_ptr_addr_count, 32);
			psqr_ptr = add_2d_ptr(psqr_ptr, -32*param.outer_loop+32, param.outer_loop - 1, psqr_ptr_addr_count, 32);
	}

    // at the last sub-volume in C dimension
	// for CASC_START kernel, directly pass the intermediate accumulated partial SUM/SQR to CASC_END kernel
    // for CASC_END kernel, accumulate the partial SUM/SQR from CASC_START firstly, then calculate the coefficients
	psum_ptr_addr_count = 0;
	psqr_ptr_addr_count = 0;
	if (param.depth_iter == param.loop_num_channel - 1) {
        // printf("[Kernel]: final iteration to calculate the E(x) & Var(x)\n");
        aie::accum<dtype_casc,32> acc_psum, acc_psqr;
        if constexpr (casc_mode == CASC_START) {
	        AIE_PREPARE_FOR_PIPELINING 
	        AIE_LOOP_RANGE(4, ) 
	        for (int i = 0; i < param.outer_loop; i++){
                    acc_psum.from_vector(aie::load_v<32>(psum_ptr),0);
                    acc_psqr.from_vector(aie::load_v<32>(psqr_ptr),0);

                    writeincr(casc_acc_out, acc_psum);
                    writeincr(casc_acc_out, acc_psqr);

			        psum_ptr = add_2d_ptr(psum_ptr, -32*param.outer_loop+32, param.outer_loop - 1, psum_ptr_addr_count, 32);
			        psqr_ptr = add_2d_ptr(psqr_ptr, -32*param.outer_loop+32, param.outer_loop - 1, psqr_ptr_addr_count, 32);
                }
        } else if constexpr (casc_mode == CASC_END) {
	        AIE_PREPARE_FOR_PIPELINING 
	        AIE_LOOP_RANGE(4, ) 
	        for (int i = 0; i < param.outer_loop; i++){
                    acc_psum = readincr_v<32>(casc_acc_in);
                    acc_psqr = readincr_v<32>(casc_acc_in);

                    aie::vector<float,32> vec_psum = aie::load_v<32>(psum_ptr);
                    aie::vector<float,32> vec_psqr = aie::load_v<32>(psqr_ptr);

			        acc_psum = aie::add(acc_psum, vec_psum);
			        acc_psqr = aie::add(acc_psqr, vec_psqr);

			        aie::store_v(psum_ptr, acc_psum.template to_vector<float>());
                    aie::store_v(psqr_ptr, acc_psqr.template to_vector<float>());

			        psum_ptr = add_2d_ptr(psum_ptr, -32*param.outer_loop+32, param.outer_loop - 1, psum_ptr_addr_count, 32);
			        psqr_ptr = add_2d_ptr(psqr_ptr, -32*param.outer_loop+32, param.outer_loop - 1, psqr_ptr_addr_count, 32);
                }
            // XXX: only happens in CASC_END kernel at the last C iteration
		    coeff_computation_casc<dtype_ofm, float>(o_mean_invsqrt_ptr, o_invsqrt_ptr, psum_ptr, psqr_ptr, param);
        }
        // TODO: add CASC_MID support if needed
	}
	param.depth_iter++;
}

#endif //__LAYER_NORM_C8_CASC_H__
