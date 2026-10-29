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


// this kernel inplements (x*sigma'-u')*gamma'+ beta in bfloat16 datatype
#ifndef __LAYER_NORM2P_H__
#define __LAYER_NORM2P_H__

#include "aie.hpp"
#include <adf.h>
#include "utils.h"
#include "layer_norm_params.h"

using namespace aie;
//Output Mantissa LUT when input exponent is even
alignas(aie::vector_decl_align) uint8 LN_inv_sqrt_lut_exp_even[128] =\
{  0,127,126,125,124,123,122,121,120,119,119,118,117,116,115,114,113,113,\
 112,111,110,109,108,108,107,106,105,105,104,103,102,102,101,100,100, 99,\
  98, 97, 97, 96, 95, 95, 94, 93, 93, 92, 92, 91, 90, 90, 89, 88, 88, 87,\
  87, 86, 86, 85, 84, 84, 83, 83, 82, 82, 81, 80, 80, 79, 79, 78, 78, 77,\
  77, 76, 76, 75, 75, 74, 74, 73, 73, 72, 72, 71, 71, 70, 70, 70, 69, 69,\
  68, 68, 67, 67, 66, 66, 66, 65, 65, 64, 64, 63, 63, 63, 62, 62, 61, 61,\
  61, 60, 60, 59, 59, 59, 58, 58, 57, 57, 57, 56, 56, 56, 55, 55, 54, 54,\
  54, 53};

//Output Mantissa LUT when input exponent is odd
alignas(aie::vector_decl_align) uint8 LN_inv_sqrt_lut_exp_odd[128] =\
{53,52,52,51,50,50,49,48,48,47,46,46,45,44,44,43,43,42,41,41,40,40,39,39,\
 38,38,37,36,36,35,35,34,34,33,33,32,32,31,31,30,30,30,29,29,28,28,27,27,\
 26,26,26,25,25,24,24,23,23,23,22,22,21,21,21,20,20,19,19,19,18,18,18,17,\
 17,16,16,16,15,15,15,14,14,14,13,13,13,12,12,12,11,11,11,10,10,10, 9, 9,\
  9, 9, 8, 8, 8, 7, 7, 7, 6, 6, 6, 6, 5, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 2,\
  2, 2, 2, 1, 1, 1, 1, 0};

//LUT used to emulate FLOOR((-1*input_exponent/2) - 0.5*(mantissa_>_1.0))
alignas(aie::vector_decl_align) int8 output_exp_floor_op[8] = { 0, -1, 0, 0, 0, -1, 0, 0 };

//Function to calculate inverse-square-root operation on a single bfloat16 value
static bfloat16 LN_compute_inv_sqrt(bfloat16 input){

    uint16 exponent_mask   = 0x7F80; //Mask to extract exponent bits
    uint16 mantissa_mask   = 0x007F; //Mask to extract mantissa bits

    uint16 *input_ptr = (uint16 *)&input; //interpret bfloat16 values as uint16

    //Extract exponent, mantissa_bits and exponent_last_bit.
    //exponent_last_bit is used to select the correct mantissa LUT.
    //There are 2 separate LUTs. One for even exponent and other for odd exponent.
    //In inverse-square-root operation the mantissa of output depends on whether exponent is even or odd.
    int8  exponent             = ((*input_ptr & exponent_mask) >> 7) - 127; //remove 127 bias.
    uint8 exponent_last_bit    = (uint8)(exponent & 0x01); //last bit of the exponent.
    uint8 mantissa_bits        = (*input_ptr & mantissa_mask); //Get mantissa bits..used as index to mantissa LUT.
    //Select the right mantissa LUT based on exponent_last_bit
    uint8 output_mantissa_bits = exponent_last_bit ? LN_inv_sqrt_lut_exp_odd[mantissa_bits] : LN_inv_sqrt_lut_exp_even[mantissa_bits];

    int8  output_exponent      = (int8)((-1 * exponent) >> 1);

    //Flags used to emulate FLOOR((-1*input_exponent/2) - 0.5*(mantissa_is_>_1.0))
    uint8 output_exponent_sign = (uint8)(output_exponent < 0 ? 1 : 0);
    uint8 neg_0_5_mantissa_adj = (uint8)(mantissa_bits ? 1 : 0); //-0.5 added to exponent to adjust mantissa.

    uint8 output_exp_floor_idx = (uint8)(output_exponent_sign << 2 | exponent_last_bit << 1 | neg_0_5_mantissa_adj); //Index of the floor LUT

    int8 output_exponent_adj = output_exp_floor_op[output_exp_floor_idx]; //Adjustment to be added to exponent to emulate floor operation.

    output_exponent = output_exponent + output_exponent_adj; //adjust the output exponent

    uint8 output_exponent_biased = output_exponent + 127; //add the 127 bias

    uint16 output;
    output = (uint16)0x0000 + (uint16)(output_exponent_biased << 7) + (uint16)output_mantissa_bits;

    bfloat16 *ret_value = (bfloat16 *)&output;//reinterpret uint16 as bfloat16

    return *ret_value;
}



static NOINLINE void layer_norm_coeff_computation
(
    bfloat16 * __restrict p_input,
    bfloat16 * __restrict p_output,
    float    * __restrict p_psum,
    layer_norm_params_t &param
){
    bfloat16 MAT_DM_BANK * __restrict p_mat      = (bfloat16 MAT_DM_BANK * __restrict) p_input;

    //Final output coefficients over-write intermediate partial sum
    bfloat16 PSUM_DM_BANK  * __restrict p_coeff    = (bfloat16 PSUM_DM_BANK  * __restrict) p_output;
    float    PSUM_DM_BANK  * __restrict p_init32   = (float    PSUM_DM_BANK  * __restrict) p_psum;
    float    PSUM_DM_BANK  * __restrict p_psum_out = (float    PSUM_DM_BANK  * __restrict) p_psum;

    #define LOAD_DATA(ptr, buff, incr) \
    {\
        buff.insert(0, load_v<16>(ptr));ptr=byte_incr(ptr, 32);\
        buff.insert(1, load_v<16>(ptr));ptr=byte_incr(ptr, incr);\
    }

    #define LOAD_ACC_DATA(ptr, buff, incr) \
    {\
        accum<accfloat,8> acc_0(load_v<8>(ptr)); buff.insert(0, acc_0); ptr=byte_incr(ptr, 32);\
        accum<accfloat,8> acc_1(load_v<8>(ptr)); buff.insert(1, acc_1); ptr=byte_incr(ptr, incr);\
    }

    #define STORE_ACC_DATA(ptr, buff, incr) \
    {\
        store_v(ptr, buff.template extract<8>(0).template to_vector<float>());  ptr = byte_incr(ptr, 32);\
        store_v(ptr, buff.template extract<8>(1).template to_vector<float>());  ptr = byte_incr(ptr, incr);\
    }

    SETUP_ITR2D_COUNTERS(iterator_mat)

    int acc_zero_flag = 1-param.psum_in; //used to zero-out partial-sum in first iteration.

    vector<bfloat16, 16> div_vector = broadcast((bfloat16)param.div_factor).template extract<16>(0);
    vector<float,    16> epsilon_vector = broadcast((float)LN_EPSILON);
    accum<accfloat,  16> epsilon_acc(epsilon_vector);

    for (int j=0; j<param.outer_count; j++)
     {
        vector<bfloat16,32> input_0, input_1, transpose_0, transpose_1, interleave_0, interleave_1;
        accum<accfloat,16>  sum_x_acc, sum_x2_acc;

        //Load the partial sums from previous iteration. For first iteration this data will be discarded.
        LOAD_ACC_DATA(p_init32, sum_x_acc,  32);
        LOAD_ACC_DATA(p_init32, sum_x2_acc, 32);

        //Load input data of WHw4 format. ex:h0-7w0-3 and h8-15w0-3
        LOAD_DATA(p_mat, input_0, 32);
        LOAD_DATA(p_mat, input_1, 32);
        p_mat = INCR_ITR2D_PARAM(p_mat, iterator_mat, param)


        //Apply transpose to make height the inner dimension
        transpose_0 = transpose(input_0, 8, 4); //8x4 -> 4x8 ex: h0-7w0-3 -> w0-3h0-7
        transpose_1 = transpose(input_1, 8, 4); //8x4 -> 4x8 ex: h8-15w0-3 -> w0-3h8-15

        //Apply interleave to group 16 heights together. 16 heights will fit on 16 lanes of Acc
        //ex: interleave_0 = w0-1h0-15 and interleave_1 w2-3h0-15
        std::tie(interleave_0, interleave_1) = interleave_zip( transpose_0, transpose_1, 8);

        //Now add interleave_0[0], interleave_0[1], interleave_1[0] and interleave_1[1] to sum along width
        //ex: add w0-3 along h0-15
        //Repeat the same for squares.
        accum<accfloat,16> temp_acc0;
        temp_acc0.from_vector(interleave_0.template extract<16>(0));
        sum_x_acc  = add(op_zero(sum_x_acc, acc_zero_flag),temp_acc0);
        sum_x_acc  = add(sum_x_acc, interleave_0.template extract<16>(1));
        sum_x_acc  = add(sum_x_acc, interleave_1.template extract<16>(0));
        sum_x_acc  = add(sum_x_acc, interleave_1.template extract<16>(1));

        accum<accfloat,16> temp_acc1 = mul_square(interleave_0.template extract<16>(0));
        sum_x2_acc = add(op_zero(sum_x2_acc, acc_zero_flag), temp_acc1);
        sum_x2_acc = mac_square(sum_x2_acc, interleave_0.template extract<16>(1));
        sum_x2_acc = mac_square(sum_x2_acc, interleave_1.template extract<16>(0));
        sum_x2_acc = mac_square(sum_x2_acc, interleave_1.template extract<16>(1));

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for (int i=0; i<param.inner_count-1; i++){
            //Load input data of WHw4 format. ex:h0-7w0-3 and h8-15w0-3
            LOAD_DATA(p_mat, input_0, 32);
            LOAD_DATA(p_mat, input_1, 32);
            p_mat = INCR_ITR2D_PARAM(p_mat, iterator_mat, param)
            //Apply transpose to make height the inner dimension
            transpose_0 = transpose(input_0, 8, 4); //8x4 -> 4x8 ex: h0-7w0-3 -> w0-3h0-7
            transpose_1 = transpose(input_1, 8, 4); //8x4 -> 4x8 ex: h8-15w0-3 -> w0-3h8-15

            //Apply interleave to group 16 heights together. 16 heights will fit on 16 lanes of Acc
            //ex: interleave_0 = w0-1h0-15 and interleave_1 w2-3h0-15
            std::tie(interleave_0, interleave_1) = interleave_zip( transpose_0, transpose_1, 8);

            //Now add interleave_0[0], interleave_0[1], interleave_1[0] and interleave_1[1] to sum along width
            //ex: add w0-3 along h0-15
            //Repeat the same for squares.
            sum_x_acc  = add(sum_x_acc, interleave_0.template extract<16>(0));
            sum_x_acc  = add(sum_x_acc, interleave_0.template extract<16>(1));
            sum_x_acc  = add(sum_x_acc, interleave_1.template extract<16>(0));
            sum_x_acc  = add(sum_x_acc, interleave_1.template extract<16>(1));
            sum_x2_acc = mac_square(sum_x2_acc, interleave_0.template extract<16>(0));
            sum_x2_acc = mac_square(sum_x2_acc, interleave_0.template extract<16>(1));
            sum_x2_acc = mac_square(sum_x2_acc, interleave_1.template extract<16>(0));
            sum_x2_acc = mac_square(sum_x2_acc, interleave_1.template extract<16>(1));
        }

        if (param.psum_out == 0){

            //Compute mean
            accum<accfloat,16> mean_x_acc  = mul(sum_x_acc.template to_vector<bfloat16>(16), div_vector);
            vector<bfloat16,16> mean_x_vec = mean_x_acc.template to_vector<bfloat16>(16);

            //Compute (1/sqrt(variance + epsilon))
            accum<accfloat,16> sq_mean_x_acc = mul(mean_x_vec, mean_x_vec);
            accum<accfloat,16> mean_x2_acc = mul(sum_x2_acc.template to_vector<bfloat16>(16), div_vector);
            accum<accfloat,16> variance_acc = sub(mean_x2_acc, sq_mean_x_acc);
            accum<accfloat,16> variance_epsilon_acc = add(variance_acc, epsilon_acc);
            vector<bfloat16,16> variance_epsilon_vec = variance_epsilon_acc.template to_vector<bfloat16>(16);

            vector<bfloat16,16> inv_sqrt_variance_vec;
            #ifdef INV_SQRT_API
               inv_sqrt_variance_vec = invsqrt(variance_epsilon_vec);
            #else
	           for(int k=0; k<16; k++){
                    inv_sqrt_variance_vec[k] = LN_compute_inv_sqrt(variance_epsilon_vec[k]);
                }
            #endif

            //Compute mean_scaled = mean/sqrt(variance + epsilon)
            accum<accfloat,16> mean_x_scaled_acc = mul(mean_x_vec, inv_sqrt_variance_vec);
            vector<bfloat16,16> mean_x_scaled_vec = mean_x_scaled_acc.template to_vector<bfloat16>(16);

            store_v(p_coeff, mean_x_scaled_vec); p_coeff = byte_incr(p_coeff, param.mat_rows*2);
            store_v(p_coeff, inv_sqrt_variance_vec); p_coeff = byte_incr(p_coeff,  (32-param.mat_rows*2));
	} else {
            STORE_ACC_DATA(p_psum_out, sum_x_acc,  32);
            STORE_ACC_DATA(p_psum_out, sum_x2_acc, 32);
        }
    }
}


static NOINLINE void layer_norm_operation
(
    bfloat16 * __restrict p_input,
    bfloat16 * __restrict p_wts,
    bfloat16 * __restrict p_ofm,
    bfloat16 * __restrict p_lut,
    layer_norm_params_t &param
){
    bfloat16 MAT_DM_BANK * __restrict p_mat     = (bfloat16 MAT_DM_BANK * __restrict) p_input;
    bfloat16 MAT_DM_BANK * __restrict p_gamma   = (bfloat16 MAT_DM_BANK  * __restrict) p_wts + (param.gamma_offset>>1);
    bfloat16 MAT_DM_BANK * __restrict p_beta    = (bfloat16 MAT_DM_BANK  * __restrict) p_wts + (param.beta_offset>>1); //Need to change MAT_COLS to no.of.colums like 768
    bfloat16 PSUM_DM_BANK * __restrict p_out    = (bfloat16 PSUM_DM_BANK * __restrict) p_ofm;

    bfloat16 PSUM_DM_BANK * __restrict p_lut_u = (bfloat16    PSUM_DM_BANK  * __restrict) p_lut; //LUT pointer for mean and variance values
    bfloat16 PSUM_DM_BANK * __restrict p_lut_v = (bfloat16    PSUM_DM_BANK  * __restrict) p_lut + param.mat_rows;
    SETUP_ITR2D_COUNTERS(iterator_mat) //input iterarator
    SETUP_ITR2D_COUNTERS(iterator_out) //output iterator

    for (int j=0; j<param.outer_count; j++)
     {
        vector<bfloat16,16> var0 =  zeros<bfloat16,16>();
        vector<bfloat16,16> var1 =  zeros<bfloat16,16>();
        vector<bfloat16,16> var2 =  zeros<bfloat16,16>();
        vector<bfloat16,16> var3 =  zeros<bfloat16,16>();

        accum<accfloat, 16> mean0 = aie::zeros<accfloat, 16>();
        accum<accfloat, 16> mean1 = aie::zeros<accfloat, 16>();
        accum<accfloat, 16> mean2 = aie::zeros<accfloat, 16>();
        accum<accfloat, 16> mean3 = aie::zeros<accfloat, 16>();

        vector<bfloat16,16> k0, k1, k2, k3;

        //mean value loaded from LUT
        bfloat16 u0 = *p_lut_u; p_lut_u = byte_incr(p_lut_u,2);
        bfloat16 u1 = *p_lut_u; p_lut_u = byte_incr(p_lut_u,2);
        bfloat16 u2 = *p_lut_u; p_lut_u = byte_incr(p_lut_u,2);
        bfloat16 u3 = *p_lut_u; p_lut_u = byte_incr(p_lut_u,2);

        //inv-square root of sigma square loaded from LUT
        bfloat16 v0 = *p_lut_v; p_lut_v = byte_incr(p_lut_v,2);
        bfloat16 v1 = *p_lut_v; p_lut_v = byte_incr(p_lut_v,2);
        bfloat16 v2 = *p_lut_v; p_lut_v = byte_incr(p_lut_v,2);
        bfloat16 v3 = *p_lut_v; p_lut_v = byte_incr(p_lut_v,2);

        //loading mean value to accumulator
        mean0.from_vector(broadcast<bfloat16, 16>((bfloat16)u0));
        mean1.from_vector(broadcast<bfloat16, 16>((bfloat16)u1));
        mean2.from_vector(broadcast<bfloat16, 16>((bfloat16)u2));
        mean3.from_vector(broadcast<bfloat16, 16>((bfloat16)u3));

        var0 = broadcast<bfloat16, 16>(v0);
        var1 = broadcast<bfloat16, 16>(v1);
        var2 = broadcast<bfloat16, 16>(v2);
        var3 = broadcast<bfloat16, 16>(v3);

        vector<bfloat16,32> input_0, input_1, transpose_0, transpose_1, int_temp0, int_temp1, int_temp2, int_temp3, inv_interleave_0, inv_interleave_1;

        vector<bfloat16,32> res0, res1, input_2, input_3, input_4, input_5;

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for (int i=0; i<param.inner_count; i++){
            vector<bfloat16,16> vec_gamma = zeros<bfloat16,16>();
            accum<accfloat,16>  acc_beta = aie::zeros<accfloat, 16>();

            accum<accfloat, 16> mean_acc0 = aie::zeros<accfloat, 16>();
            accum<accfloat, 16> mean_acc1 = aie::zeros<accfloat, 16>();
            accum<accfloat, 16> mean_acc2 = aie::zeros<accfloat, 16>();
            accum<accfloat, 16> mean_acc3 = aie::zeros<accfloat, 16>();
            accum<accfloat, 16> zero_acc  = aie::zeros<accfloat, 16>();

            accum<accfloat, 16> bias_acc0 = aie::zeros<accfloat, 16>();
            accum<accfloat, 16> bias_acc1 = aie::zeros<accfloat, 16>();
            accum<accfloat, 16> bias_acc2 = aie::zeros<accfloat, 16>();
            accum<accfloat, 16> bias_acc3 = aie::zeros<accfloat, 16>();


            //Load input data of WHw4 format.
            input_0.insert(0, load_v<16>(p_mat)); p_mat = byte_incr(p_mat, param.mat_step_W);
            input_1.insert(0, load_v<16>(p_mat)); p_mat = byte_incr(p_mat, param.mat_step_W);
            input_0.insert(1, load_v<16>(p_mat)); p_mat = byte_incr(p_mat, param.mat_step_W);
            input_1.insert(1, load_v<16>(p_mat)); p_mat = INCR_ITR2D_PARAM(p_mat, iterator_mat, param);

            acc_beta.from_vector(load_v<16>(p_beta));
            vec_gamma = load_v<16>(p_gamma);

            chess_separator();
            //shuffling data 4x4 4 to 4 1x16 vectors
            std::tie(transpose_0, transpose_1)   = interleave_zip(input_0, input_1, 4);
            std::tie(input_0, input_1)           = interleave_zip(transpose_0, transpose_1, 8);
            chess_separator();

            //computing the layer norm
            mean_acc0   = mul(input_0.template extract<16>(0), var0);
            mean_acc0   = sub(mean_acc0, mean0);
            bias_acc0   = mac(acc_beta, mean_acc0.template to_vector<bfloat16>(0), vec_gamma);

            mean_acc1   = mul(input_0.template extract<16>(1), var1);
            mean_acc1   = sub(mean_acc1, mean1);
            bias_acc1   = mac(acc_beta, mean_acc1.template to_vector<bfloat16>(0), vec_gamma);

            mean_acc2   = mul(input_1.template extract<16>(0), var2);
            mean_acc2   = sub(mean_acc2, mean2);
            bias_acc2   = mac(acc_beta, mean_acc2.template to_vector<bfloat16>(0), vec_gamma);

            mean_acc3   = mul(input_1.template extract<16>(1), var3);
            mean_acc3   = sub(mean_acc3, mean3);
            bias_acc3   = mac(acc_beta, mean_acc3.template to_vector<bfloat16>(0), vec_gamma);

            res0.insert(0,bias_acc0.template to_vector<bfloat16>(0));
            res0.insert(1,bias_acc1.template to_vector<bfloat16>(0));

            res1.insert(0,bias_acc2.template to_vector<bfloat16>(0));
            res1.insert(1,bias_acc3.template to_vector<bfloat16>(0));

            chess_separator();
            // reshuffling back to original format 4x4 4
            std::tie(transpose_0, transpose_1)   = interleave_unzip(res0, res1, 8);
            std::tie(inv_interleave_0, inv_interleave_1) = interleave_unzip(transpose_0, transpose_1, 4);
            chess_separator();

            vector<bfloat16,16> temp_vec0 = filter_even(inv_interleave_0,16);
            vector<bfloat16,16> temp_vec1 = filter_even(inv_interleave_1,16);
            vector<bfloat16,16> temp_vec2 = filter_odd(inv_interleave_0,16);
            vector<bfloat16,16> temp_vec3 = filter_odd(inv_interleave_1,16);
            store_v(p_out, temp_vec0);  p_out=byte_incr(p_out, param.mat_step_W);
            store_v(p_out, temp_vec1);  p_out=byte_incr(p_out, param.mat_step_W);
            store_v(p_out, temp_vec2);  p_out=byte_incr(p_out, param.mat_step_W);
            store_v(p_out, temp_vec3);  p_out=INCR_ITR2D_PARAM(p_out, iterator_out, param);

            //beta and gamma values increments
            p_beta  = byte_incr(p_beta, 32);
            p_gamma = byte_incr(p_gamma, 32);
        }

        p_beta  = byte_incr(p_beta, -2*param.mat_cols);     //MAT_COLS=64 be sub volume number of columns
        p_gamma = byte_incr(p_gamma, -2*param.mat_cols);

    }
}

template <typename dtype_ifm, typename dtype_wts, typename dtype_ofm, typename config_ifm, typename config_wts, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype_ifm, int8_t, bfloat16>) &&
            (mllib::utils::is_one_of_v<dtype_wts, int8_t, bfloat16>) &&
            (mllib::utils::is_one_of_v<dtype_ofm, int8_t, bfloat16>)
NOINLINE void layer_norm
(
    adf::input_buffer_conf <dtype_ifm, config_ifm>       & __restrict input_mat,
    adf::input_buffer_conf <dtype_wts, config_wts>       & __restrict input_wts,
    adf::output_buffer_conf<dtype_ofm, config_ofm>       & __restrict output_mat,
    layer_norm_params_t &param
) {
     setup_layer_norm_iter_params(param);
     bfloat16 * p_coeff = (bfloat16 *)output_mat.data() + (param.coeff_offset>>1) ;

     if(param.state == 0){
	     layer_norm_coeff_computation((bfloat16 *)input_mat.data(), p_coeff, (float *) p_coeff, param);
     }
     else{
	     layer_norm_operation((bfloat16 *)(input_mat.data() + param.row_offset), (bfloat16 *)input_wts.data(), (bfloat16 *)output_mat.data(), p_coeff, param);
     }

}

#endif //__LAYER_NORM2P_H__

