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

#include "adf.h"
#include "aie.hpp"
#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"

#include "reduce_base_c8.h"
#include "reduce_prod_c8_params.h"

// User configuration for design optimization
template <typename dtype> struct reduce_prod_c8_config {
};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct reduce_prod_c8_config<int8_t> {
    static constexpr uint8_t inner_unroll_num = 4;
    static constexpr uint8_t accum_3d_unroll_num = 4;
    static constexpr uint16_t minimum_num_iter = 4;
    static constexpr uint16_t vec_size = 32;
    static constexpr bool if_double_partial_result_size = true;
    static constexpr bool if_highest_partial_result_size = false;
    static constexpr bool if_postprocess = true;
};
template <> struct reduce_prod_c8_config<bfloat16> {
    static constexpr uint8_t inner_unroll_num = 4;
    static constexpr uint8_t accum_3d_unroll_num = 4;
    static constexpr uint16_t minimum_num_iter = 4;
    static constexpr uint16_t vec_size = 32;
    static constexpr bool if_double_partial_result_size = false;
    static constexpr bool if_highest_partial_result_size = false;
    static constexpr bool if_postprocess = true;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct reduce_prod_c8_config<int8_t> {
    static constexpr uint8_t inner_unroll_num = 4;
    static constexpr uint8_t accum_3d_unroll_num = 4;
    static constexpr uint16_t minimum_num_iter = 4;
    static constexpr uint16_t vec_size = 32;
    static constexpr bool if_double_partial_result_size = true;
    static constexpr bool if_highest_partial_result_size = false;
    static constexpr bool if_postprocess = true;
};
template <> struct reduce_prod_c8_config<bfloat16> {
    static constexpr uint8_t inner_unroll_num = 4;
    static constexpr uint8_t accum_3d_unroll_num = 4;
    static constexpr uint16_t minimum_num_iter = 4;
    static constexpr uint16_t vec_size = 32;
    static constexpr bool if_double_partial_result_size = false;
    static constexpr bool if_highest_partial_result_size = false;
    static constexpr bool if_postprocess = true;
};
#endif  // AIE_ARCH

using namespace aie;

template <typename dtype> requires(mllib::utils::is_one_of_v<dtype, bfloat16>)
class reduce_prod_c8_impl : reduce_base_c8<dtype, reduce_prod_c8_params_t<dtype>> {
public:
    static constexpr uint8_t inner_unroll_num = reduce_prod_c8_config<dtype>::inner_unroll_num;
    static constexpr uint8_t accum_3d_unroll_num = reduce_prod_c8_config<dtype>::accum_3d_unroll_num;
    static constexpr uint16_t minimum_num_iter = reduce_prod_c8_config<dtype>::minimum_num_iter;
    static constexpr uint16_t vec_size = reduce_prod_c8_config<dtype>::vec_size;
    static constexpr bool if_double_partial_result_size = reduce_prod_c8_config<dtype>::if_double_partial_result_size;
    static constexpr bool if_highest_partial_result_size = reduce_prod_c8_config<dtype>::if_highest_partial_result_size;
    static constexpr bool if_postprocess = reduce_prod_c8_config<dtype>::if_postprocess;
    using dtype_double_size_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, bfloat16, int16_t>;
    using dtype_highest_size_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, float, int32_t>;
    using dtype_data_t = std::conditional_t<if_highest_partial_result_size, dtype_highest_size_t, std::conditional_t<if_double_partial_result_size, dtype_double_size_t, dtype>>;
    static dtype_data_t zero_iter_init_value;// = (dtype_data_t)1.0;
    using vec_data_t = aie::vector<dtype_data_t, vec_size>;
    using dtype_acc_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, accfloat, acc32>;
    using acc_t = std::conditional_t<std::is_same_v<dtype, bfloat16>,
                                     aie::accum<accfloat, vec_size>,
                                     aie::accum<acc32, vec_size>>;
    using vec_t = aie::vector<dtype, vec_size>;
    using mask_t = aie::mask<vec_size>;
    using params_t = reduce_c8_params_t<reduce_prod_c8_params_t<dtype>>;

public:
    reduce_prod_c8_impl(const params_t &param) {
    }

    INLINE static void setup(params_t &param, const void *rtps) {
            param.op_params.shift_out = (int8_t)(((uint32_t*)rtps)[0]);
            // printf("shift_out: %d\n", param.op_params.shift_out);
            zero_iter_init_value = (dtype_data_t)1.0;
        }

    INLINE static void setup_per_iter(params_t &param) {
    }

    INLINE static void body_main(vec_t &input_vec, vec_data_t &output_vec,
                                                                params_t &param) {
        if constexpr (std::is_same<bfloat16, dtype>::value) {
            output_vec = aie::mul(output_vec, input_vec);
        } else {
            output_vec = aie::mul(output_vec, aie::unpack(input_vec));
        }
    }

    INLINE static void body_postprocess(void* p_out, params_t &param) {
        // padding vector initialized as all 0s
        aie::vector<dtype_data_t, vec_size> vec_pad_full_double = aie::broadcast<dtype_data_t, vec_size>(0);

        dtype* o_ptr = (dtype*)p_out;
        dtype_data_t* psum_ptr = (dtype_data_t*)p_out;

	    aie::mask<8> msk_eq_w_hw;
	    aie::mask<vec_size> msk_eq_h;
        aie::mask<8> msk_eq_c8;
        aie::mask<vec_size> msk_eq_c;
	    aie::mask<8> msk_8_ones_full(true);
	    aie::mask<vec_size> msk_ones_full(true);

	    aie::mask<vec_size> msk_eq_hwc = aie::mask<vec_size>::from_uint32(1);
        aie::mask<vec_size> msk_pad = aie::mask<vec_size>::from_uint32(0x01010101);
        // aie::print(msk_eq_hwc, true, "msk_eq_hwc = ");

	    int32_t o_ptr_last_delta;
	    if (param.reduce_axis == 2 || param.reduce_axis == 6){// w or hw dim
	    	msk_eq_w_hw = msk_8_ones_full;
            msk_pad = aie::mask<vec_size>::from_uint32(0xffffffff);
	    	o_ptr_last_delta = 8;
	    }else if(param.reduce_axis == 3){// cw dim
	    	o_ptr_last_delta = 8;
	    }else if(param.reduce_axis == 1 || param.reduce_axis == 5){// c or hc dim
            msk_eq_c = msk_ones_full;
	    	o_ptr_last_delta = vec_size;
	    }else if(param.reduce_axis == 4){// h dim
	    	msk_eq_h = msk_ones_full;
	    	o_ptr_last_delta = vec_size;
	    }

        if (param.reduce_axis == 7) {
            aie::vector<dtype_data_t, vec_size> vec_reduce_in = aie::broadcast<dtype_data_t, vec_size>(1);
	        AIE_PREPARE_FOR_PIPELINING
	        AIE_LOOP_RANGE(minimum_num_iter, )
	        for (int i = 0; i < param.outer_loop; i++)
	        	
	        	{
	        		auto vec_psum_tmp = aie::load_v<vec_size>(psum_ptr);
                    // aie::print(vec_psum_tmp, true, "vec_psum_tmp = ");
	        		psum_ptr = byte_incr(psum_ptr, vec_size * sizeof(dtype_data_t));
                    vec_reduce_in = aie::mul(vec_reduce_in, vec_psum_tmp);
	        }
            // aie::print(vec_reduce_in, true, "vec_reduce_in = ");
            // aie::vector<dtype_data_t, vec_size> vec_reduce_all = aie::reduce_mul_v(vec_reduce_in);
            aie::vector<dtype_data_t, vec_size> vec_reduce_all;
            vec_reduce_all[0] = aie::reduce_mul(vec_reduce_in);
            // aie::print(vec_reduce_all, true, "vec_reduce_all = ");
            aie::vector<dtype_data_t, vec_size> vec_pad = aie::select(vec_pad_full_double, vec_reduce_all, msk_eq_hwc);
            auto acc_out = aie::from_vector<dtype_acc_t>(vec_pad);
            aie::vector<dtype, vec_size> vec_out = acc_out.template to_vector<dtype>(param.op_params.shift_out);
            // aie::print(vec_out, true, "vec_out = ");
	        aie::store_v(o_ptr, vec_out);
        } else {
	        AIE_PREPARE_FOR_PIPELINING
	        AIE_LOOP_RANGE(minimum_num_iter, )
	        for (int i = 0; i < param.outer_loop; i++)
	        	
	        	{
	        		auto vec_psum_tmp = aie::load_v<vec_size>(psum_ptr);
                    // aie::print(vec_psum_tmp, true, "psum = ");
                    psum_ptr = byte_incr(psum_ptr, vec_size * sizeof(dtype_data_t));

	        		aie::vector<dtype_data_t, 8> vec_reduce_w_tmp0 = vec_psum_tmp.template extract<8>(0);
	        		aie::vector<dtype_data_t, 8> vec_reduce_w_tmp1 = vec_psum_tmp.template extract<8>(1);
	        		aie::vector<dtype_data_t, 8> vec_reduce_w_tmp2 = vec_psum_tmp.template extract<8>(2);
	        		aie::vector<dtype_data_t, 8> vec_reduce_w_tmp3 = vec_psum_tmp.template extract<8>(3);
	        		aie::vector<dtype_data_t, 8> vec_reduce_w_sum = aie::mul(vec_reduce_w_tmp0, vec_reduce_w_tmp1);
	        		vec_reduce_w_sum = aie::mul(vec_reduce_w_sum, vec_reduce_w_tmp2);
	        		vec_reduce_w_sum = aie::mul(vec_reduce_w_sum, vec_reduce_w_tmp3);
                    // aie::print(vec_reduce_w_sum, true, "vec_reduce_w_sum = ");

                    auto vec_reduce_cw_sum = aie::broadcast<dtype_data_t, 8>((dtype_data_t)(aie::reduce_mul(vec_reduce_w_sum)));
	        		auto vec_select_cw = aie::select(vec_reduce_cw_sum, vec_reduce_w_sum, msk_eq_w_hw);
                    aie::vector<dtype_data_t, vec_size> vec_reduce_cw;
                    vec_reduce_cw.insert(0, vec_select_cw);
                    vec_reduce_cw.insert(1, vec_select_cw);
                    vec_reduce_cw.insert(2, vec_select_cw);
                    vec_reduce_cw.insert(3, vec_select_cw);
                    // aie::print(msk_eq_w_hw, true, "msk_eq_w_hw = ");
                    // aie::print(vec_reduce_cw_sum, true, "vec_reduce_cw_sum = ");
                    // aie::print(vec_select_cw, true, "vec_select_cw     = ");
                    // aie::print(vec_reduce_cw, true, "vec_reduce_cw     = ");

                    auto vec_reduce_c_tmp0 = aie::reduce_mul(vec_reduce_w_tmp0);
                    auto vec_reduce_c_tmp1 = aie::reduce_mul(vec_reduce_w_tmp1);
                    auto vec_reduce_c_tmp2 = aie::reduce_mul(vec_reduce_w_tmp2);
                    auto vec_reduce_c_tmp3 = aie::reduce_mul(vec_reduce_w_tmp3);
                    aie::vector<dtype_data_t, vec_size> vec_reduce_c;
                    vec_reduce_c[0] = vec_reduce_c_tmp0;
                    vec_reduce_c[8] = vec_reduce_c_tmp1;
                    vec_reduce_c[16] = vec_reduce_c_tmp2;
                    vec_reduce_c[24] = vec_reduce_c_tmp3;
                    // aie::print(vec_reduce_c, true, "vec_reduce_c = ");

                    auto vec_select_c = aie::select(vec_reduce_cw, vec_reduce_c, msk_eq_c);
                    auto vec_pad_c = aie::select(vec_pad_full_double, vec_select_c, msk_pad);
                    auto vec_reduce_out = aie::select(vec_pad_c, vec_psum_tmp, msk_eq_h);
                    // aie::print(msk_eq_c, true, "msk_eq_c = ");
                    // aie::print(vec_select_c, true, "vec_select_c = ");
                    // aie::print(msk_pad, true, "msk_pad = ");
                    // aie::print(vec_pad_c, true, "vec_pad_c = ");
                    // aie::print(msk_eq_h, true, "msk_eq_h = ");
                    // aie::print(vec_reduce_out, true, "vec_reduce_out = ");

                    auto acc_out = aie::from_vector<dtype_acc_t>(vec_reduce_out);
                    // aie::print(acc_out, true, "acc_out = ");
                    aie::vector<dtype, vec_size> vec_out = acc_out.template to_vector<dtype>(param.op_params.shift_out);
                    // aie::print(vec_out, true, "vec_out = ");
                    aie::store_unaligned_v(o_ptr, vec_out); 
	        		o_ptr = byte_incr(o_ptr, o_ptr_last_delta * sizeof(dtype));
            }
        }
    }

private:
};
template<>
bfloat16 reduce_prod_c8_impl<bfloat16>::zero_iter_init_value = (bfloat16)1.0;