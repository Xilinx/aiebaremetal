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


#ifndef __SOFTMAX__
#define __SOFTMAX__
    
#include <adf.h>
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "mllib_const.h"
#include "softmax_lut.h"
#include "softmax_params.h"

#define SM_VEC_LEN 16 //32
#define VEC_LEN_BITS 4 //5
#define log2e 1.4453125//1.44269504089
using namespace aie;


#if __AIE_ARCH__ == 20
#ifndef SOFTMAX_LUT
#define SOFTMAX_LUT
#endif
#endif

template <int rows, data_type_t type, uint16_t maxsub_en=0, typename VecIteratorIn, typename VecIteratorOut>
float __attribute__((always_inline)) compute_exp (
        int num_elems,
        VecIteratorIn &it_input,
        VecIteratorOut &it_output,
        int r,
        int16 * itm_input,
        int8 * itm_input_int8
) {
#ifdef SOFTMAX_LUT
    bfloat16  __aie_dm_resource_a * ilut_ab = (bfloat16  __aie_dm_resource_a * ) softmax_ilut_ab ;
    bfloat16  __aie_dm_resource_b * ilut_cd = (bfloat16  __aie_dm_resource_b * ) softmax_ilut_cd ;
    bfloat16  __aie_dm_resource_a * flut_ab = (bfloat16  __aie_dm_resource_a * ) softmax_flut_ab ;
    bfloat16  __aie_dm_resource_b * flut_cd = (bfloat16  __aie_dm_resource_b * ) softmax_flut_cd ;
    using lut_type = aie::lut<4, bfloat16, bfloat16>;
    const int LUT_elems = 256;
    const int step_i = 8;
    const int step_f = 0;

    lut_type lut_i(LUT_elems, ilut_ab, ilut_cd);
    lut_type lut_f(LUT_elems, flut_ab, flut_cd);
    aie::parallel_lookup<uint16, lut_type, aie::lut_oor_policy::truncate> lookup_i(lut_i, step_i);
    aie::parallel_lookup<uint16, lut_type, aie::lut_oor_policy::truncate> lookup_f(lut_f, step_f);
    aie::accum<accfloat, SM_VEC_LEN> exp_val;
#else
    aie::vector<bfloat16, SM_VEC_LEN> exp_val;
    aie::vector<float, SM_VEC_LEN> input_fp32;
    aie::vector<bfloat16, SM_VEC_LEN> log2e_vec = aie::broadcast<bfloat16, SM_VEC_LEN>(log2e);  
#endif

    const int elem_iters = num_elems / SM_VEC_LEN; //number of iterations need to be performed
    aie::vector<bfloat16, SM_VEC_LEN> I_val_vec, F_val_vec, res0, input_bf16;
    aie::accum<accfloat, SM_VEC_LEN> exp_val_accum;
    aie::accum<accfloat, SM_VEC_LEN> exp_val_accum_shift;
    exp_val_accum = aie::zeros<accfloat, SM_VEC_LEN>();
    //Maximum value computation
    bfloat16 max_value;
    aie::vector<bfloat16, SM_VEC_LEN> max_bfloat16;  
    aie::accum<accfloat, SM_VEC_LEN> acc0, acc1, acc_res;
    aie::vector<int16, SM_VEC_LEN> input;
    aie::vector<int16, 2*SM_VEC_LEN> input0;
    aie::vector<int8, SM_VEC_LEN> input_int8;
 
    if constexpr(maxsub_en == 1){
       auto input_max    = it_input;
       uint16 neg_infinity = (uint16) 0xff80;
       bfloat16 *bf_neg_infinity = (bfloat16 *)&neg_infinity;
       aie::vector<bfloat16,32> max_vec = broadcast((*bf_neg_infinity));
       //vector<bfloat16,16> max_vec = zeros<bfloat16,16>();
       aie::vector<bfloat16, 32> temp;
       AIE_PREPARE_FOR_PIPELINING 
       AIE_LOOP_RANGE(4,) 
       AIE_LOOP_UNROLL(8) 
       for(int i=0; i<elem_iters; i++){
              temp.insert(0, *input_max++);
              max_vec = aie::max(max_vec, temp);
           }
           max_value = aie::reduce_max(max_vec.template extract<16>(0));
           max_bfloat16 = aie::broadcast<bfloat16, SM_VEC_LEN>(max_value);
    }
    
    itm_input = byte_incr(itm_input, SM_VEC_LEN*r);
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(4,) 
    for(int i=0; i<elem_iters; i++){

    	if constexpr(type == INT8){
	       
        input_int8 = load_v<SM_VEC_LEN>((int8 *)itm_input_int8);
        itm_input_int8 = byte_incr(itm_input_int8, SM_VEC_LEN);
               
#ifndef SOFTMAX_LUT
	    input_fp32 = aie::to_float(input_int8, SM_SCALE_FAC);
	    input_bf16 = aie::accum<accfloat, SM_VEC_LEN>(input_fp32).to_vector<bfloat16>();
#endif
	} 	
	else if constexpr(type == INT16){
	      if constexpr(rows!=1){ //rows -> batch_size
	          aie::vector<int16, SM_VEC_LEN/2> temp0, temp1;
	          temp0 = load_v<SM_VEC_LEN/2>((int16 *)itm_input);
	          itm_input = byte_incr(itm_input, SM_VEC_LEN*rows);
                  temp1 = load_v<SM_VEC_LEN/2>((int16 *)itm_input);
	          itm_input = byte_incr(itm_input, SM_VEC_LEN*rows);
                  input = concat(temp0, temp1);
	       }
	       else{
	          input = load_v<SM_VEC_LEN>((int16 *)itm_input);
	          itm_input = byte_incr(itm_input, SM_VEC_LEN*2);
               }
#ifndef SOFTMAX_LUT
	    input_fp32 = aie::to_float(input, 8);
	    input_bf16 = aie::accum<accfloat, SM_VEC_LEN>(input_fp32).to_vector<bfloat16>();
#endif
	}
	else if constexpr(maxsub_en == 1){
	    aie::vector<bfloat16, SM_VEC_LEN> input_org = *it_input++;
	    acc0.from_vector(input_org, 0);
            acc1.from_vector(max_bfloat16, 0);
	    acc_res =  sub(acc0, acc1);
            input_bf16 = to_v16bfloat16(acc_res);
	}
	else{
	    input_bf16 = *it_input++;
	}
#ifndef SOFTMAX_LUT
	aie::accum<accfloat, 16> exp_in;
	exp_in = aie::mul(input_bf16, log2e_vec);
	
        exp_val = aie::exp2<bfloat16>(exp_in.to_vector<float>());
        exp_val_accum = add(exp_val_accum, exp_val);
	*it_output++ = exp_val;
#else
        if constexpr(type == BFLOAT16){
            input0 = v32int16(bfloat16_to_int(input_bf16,SM_SCALE_FAC));
            #ifndef SM_USE_MSB
            input = filter_even(input0);
            #else
            input = filter_odd(input0);
            #endif
	}

	I_val_vec = lookup_i.fetch(input.cast_to<uint16>());
	F_val_vec = lookup_f.fetch(input.cast_to<uint16>());
        exp_val = aie::mul(I_val_vec, F_val_vec);
	exp_val_accum = add(exp_val_accum, exp_val) ; 
	*it_output++ = exp_val.to_vector<bfloat16>();
#endif
    }
    //Variant not using emulated FP32 for the mul reduce, off by +/- 1 in final result and 10 cycles slower
    aie::vector<float, SM_VEC_LEN> reduce = exp_val_accum.to_vector<float>();
    float res = aie::reduce_add(reduce);
    return res;
 }

bfloat16 __attribute__((always_inline)) compute_inv ( float x ) {
#ifdef SOFTMAX_LUT
        unsigned int *B_x;
        unsigned int exp_mask     = 0x7F800000;
        unsigned int mantissa_mask= 0x007FFFFF;
        unsigned int mantissa_Q   = 0x00008000;
        unsigned char exponent, mantissa;
        unsigned inv_exponent;
        unsigned short inv_x_val;
	unsigned int B_Q;
        bfloat16 *inv_x;
        B_x = (unsigned int*) &x;
        B_Q = *B_x + mantissa_Q;
        exponent = (B_Q & exp_mask) >> 23;
        mantissa = (B_Q & mantissa_mask) >> 16;
        inv_exponent = (mantissa==0)+(253-exponent);
        inv_x_val = (inv_exponent<<7) + m_inv_lut[mantissa];
        inv_x = (bfloat16*)&inv_x_val;
        return *inv_x;
#else
	float inv_x_float = aie::inv(x);
        bfloat16 inv_x = (bfloat16)(inv_x_float);
	return inv_x;
#endif
}


//# kernel with input INT8 
template <int softmax_batch_size, int sm_input_size, int num_softmax_classes>
void __attribute__((noinline)) softmax(
        int8 * __restrict ifm_data,
        int16 * __restrict ofm_data
) {
    // TODO:Interleaved batching is not supported in this version of softmax, batching is over-simplified
    // Adding this code for batch size=1, need to work on batching for batch size>1
    #ifndef SM_NULL_VAL_INT8
        #define SM_NULL_VAL_INT8 (-128) //(-120*256)=-30720
    #endif
    
    // Preprocess inputs. Null out the last k values; k=sm_input_size - num_softmax_classes

    if constexpr(sm_input_size - num_softmax_classes != 0)
    {
        const int num_sm_nullsamp = sm_input_size - num_softmax_classes; 
        int8 * in_ptr  = ((int8    *) ifm_data) + num_softmax_classes*softmax_batch_size;

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for (unsigned k=0;k<num_sm_nullsamp*softmax_batch_size;k++){
            *in_ptr=(int8) SM_NULL_VAL_INT8;
            in_ptr++;
        }
    }
    int num_elems = sm_input_size;

    float accum_exp_val;
    int dummy = 0;
    int8 * it_exp_in = (int8 *) ifm_data;
    auto it_exp_out   = aie::begin_vector<SM_VEC_LEN>( (bfloat16 *) ofm_data);

    auto it_scale     = aie::cbegin_restrict_vector<16>((bfloat16 *) ofm_data);
    auto it_soft_out  = aie::begin_restrict_vector<16>( (bfloat16 *) ofm_data);

    bfloat16 col_sum_inv;
    aie::vector<bfloat16,16>  in_elems, va;
    aie::accum<accfloat, 16>  out_vals;
    int col_iters    = sm_input_size>>4;
    for(int r=0; r<softmax_batch_size; r++) {
        accum_exp_val = 0;
        accum_exp_val = compute_exp<softmax_batch_size, INT8, 0>(num_elems, dummy, it_exp_out, r, 0, it_exp_in);
 	col_sum_inv = compute_inv(accum_exp_val);
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for(int c=0; c<col_iters; c++){
            in_elems = *it_scale++;
            out_vals = aie::mul(in_elems, col_sum_inv);
            *it_soft_out++ = out_vals.to_vector<bfloat16>();
        }
    }
}


template <int softmax_batch_size, int sm_input_size, int num_softmax_classes, typename dtype_in, typename dtype_out, typename config_ifm, typename config_ofm>
INLINE void softmax(
	    adf::input_buffer_conf<dtype_in, config_ifm>  & __restrict ifm,
        adf::output_buffer_conf<dtype_out, config_ofm> & __restrict ofm
) {
    return softmax<softmax_batch_size, sm_input_size, num_softmax_classes>((int8 *)ifm.data(), (int16 *)ofm.data());
}


//# kernel with input INT16 
template <int softmax_batch_size, int sm_input_size, int num_softmax_classes>
void __attribute__((noinline)) softmax(
        int16 * __restrict ifm_data,
        int16 * __restrict ofm_data
) {
    // TODO:Interleaved batching is not supported in this version of softmax, batching is over-simplified
    // Adding this code for batch size=1, need to work on batching for batch size>1
    #ifndef SM_NULL_VAL
        #define SM_NULL_VAL (-30720) //(-120*256)=-30720
    #endif
    
    // Preprocess inputs. Null out the last k values; k=sm_input_size - num_softmax_classes

    if constexpr(sm_input_size - num_softmax_classes != 0)
    {
        const int num_sm_nullsamp = sm_input_size - num_softmax_classes; 
        int16 * in_ptr  = ((int16    *) ifm_data) + num_softmax_classes*softmax_batch_size;

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for (unsigned k=0;k<num_sm_nullsamp*softmax_batch_size;k++){
            *in_ptr=(int16) SM_NULL_VAL;
            in_ptr++;
        }
    }
    int num_elems = sm_input_size;

    float accum_exp_val;
    int dummy = 0;
    int16 * it_exp_in = (int16 *) ifm_data;
    auto it_exp_out   = aie::begin_vector<SM_VEC_LEN>( (bfloat16 *) ofm_data);

    auto it_scale     = aie::cbegin_restrict_vector<16>((bfloat16 *) ofm_data);
    auto it_soft_out  = aie::begin_restrict_vector<16>( (bfloat16 *) ofm_data);

    bfloat16 col_sum_inv;
    aie::vector<bfloat16,16>  in_elems, va;
    aie::accum<accfloat, 16>  out_vals;
    int col_iters    = sm_input_size>>4;
    for(int r=0; r<softmax_batch_size; r++) {
        accum_exp_val = 0;
        accum_exp_val = compute_exp<softmax_batch_size, INT16, 0>(num_elems, dummy, it_exp_out, r, it_exp_in,0);
 	col_sum_inv = compute_inv(accum_exp_val);
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for(int c=0; c<col_iters; c++){
            in_elems = *it_scale++;
            out_vals = aie::mul(in_elems, col_sum_inv);
            *it_soft_out++ = out_vals.to_vector<bfloat16>();
        }
    }
}

template <int softmax_batch_size, int sm_input_size, int num_softmax_classes, typename config_ifm, typename config_ofm>
INLINE void softmax(
	    adf::input_buffer_conf<int16, config_ifm>  & __restrict ifm,
        adf::output_buffer_conf<int16, config_ofm> & __restrict ofm
) {
    return softmax<softmax_batch_size, sm_input_size, num_softmax_classes>(ifm.data(), ofm.data());
}

//Kernel with input Bfloat16 
template <int rows, int cols, typename dtype_ifm, typename dtype_ofm, uint16_t maxsub_en=0>
void __attribute__((noinline)) softmax(
        dtype_ifm * __restrict ifm_data,
        dtype_ofm * __restrict ofm_data,
        softmax_params_t sm_params
) {
    // TODO:Interleaved batching is not supported in this version of softmax, batching is over-simplified
    // Adding this code for batch size=1, need to work on batching for batch size>1
    int num_elems = rows*cols;

    float accum_exp_val;

    //auto it_exp_in    = aie::cbegin_vector<16>((bfloat16 *) ifm_data);
    auto it_exp_in    = aie::cbegin_vector<16>((bfloat16 *) ( ((bfloat16 *) ifm_data) + sm_params.row_offset ));// pick up row with offset
    auto it_exp_out   = aie::begin_vector<16>( (bfloat16 *) ofm_data);
    auto it_scale     = aie::cbegin_restrict_vector<16>((bfloat16 *) ofm_data);
    auto it_soft_out  = aie::begin_restrict_vector<16>( (bfloat16 *) ofm_data);

    bfloat16 col_sum_inv;
    aie::vector<bfloat16,16>  in_elems, va;
    aie::accum<accfloat, 16>  out_vals;
    int col_iters    = cols>>4;
    for(int r=0; r<rows; r++) {
        accum_exp_val = 0;
        accum_exp_val = compute_exp<rows, BFLOAT16, maxsub_en>(cols, it_exp_in, it_exp_out, 0, 0, 0);
        col_sum_inv = sm_params.enable_exp == 1 ? (bfloat16) 1.0 : compute_inv(accum_exp_val);
	AIE_PREPARE_FOR_PIPELINING 
	AIE_LOOP_RANGE(4,) 
	for(int c=0; c<col_iters; c++ ){
            in_elems = *it_scale++;
            out_vals = aie::mul(in_elems, col_sum_inv);
	    *it_soft_out++ = out_vals.to_vector<bfloat16>();
        }
    }
}

template <int rows, int cols, typename dtype_ifm, typename dtype_ofm, typename config_ifm, typename config_ofm, uint16_t maxsub_en=0>
INLINE void softmax(
	adf::input_buffer_conf<dtype_ifm, config_ifm>  & __restrict ifm,
        adf::output_buffer_conf<dtype_ofm, config_ofm> & __restrict ofm,
        softmax_params_t sm_params
) {
    return softmax<rows, cols, dtype_ifm, dtype_ofm, maxsub_en>(ifm.data(), ofm.data(), sm_params);
}

#endif //__SOFTMAX__
