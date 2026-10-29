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


#ifndef __FULLYCONNECT_H__
#define __FULLYCONNECT_H__

#include "aie.hpp"
#include <adf.h>
#include <stdio.h>
#include "utils.h"
#include "fullyconnect_params.h"
#include "mllib_const.h"
#include <aie_api/utils.hpp>
using namespace aie;

#define __AIE_ARCH__ 20
#if __AIE_ARCH__ == 20

//Start of fullyconnect bfloat16 AIE2

static NOINLINE void fullyconnect_internal_bf16
(
    bfloat16 * __restrict p_a,
    bfloat16 * __restrict p_b,
    bfloat16 * __restrict p_c,
    bfloat16 * __restrict p_bias,
    fullyconnect_params_t &param
){
    bfloat16 MAT_A_DM_BANK * __restrict p_mat_a    = (bfloat16 MAT_A_DM_BANK * __restrict) p_a;
    bfloat16 MAT_B_DM_BANK * __restrict p_mat_b    = (bfloat16 MAT_B_DM_BANK * __restrict) p_b;
    bfloat16 MAT_C_DM_BANK * __restrict p_mat_c    = (bfloat16 MAT_C_DM_BANK * __restrict) p_c;
    bfloat16 MAT_B_DM_BANK * __restrict p_bias_in  = (bfloat16 MAT_B_DM_BANK * __restrict) p_bias;
    // No need partial sum (p_sum) because partial sum can't get performance improved in FC(IO band)
    // In FC one outer loop, we get n column final result.
    // ifm and ofm size are very small compare to wts in FC, so we have >64KB (128kB*(1-1/col_B)) to load wts in aie2p.

    using MMUL = mmul<FC_DIM_M, FC_DIM_K, FC_DIM_N, bfloat16, bfloat16>;

    const int size_bf16 = 2;

    //*********************************************//
    //Load Mat A data and adjust the vectorization.//
    //*********************************************//
    //LOAD_MAT_A_DATA_PADDING0_ROWS3 load one row WHw4 order with padding 0 in next 3 rows
    //Load data in w4 vectorization, load 8 value and insert to the first 8 element of all 0
    //X0 = h0-3w0-7
    //
    #define LOAD_MAT_A_DATA_PADDING0_ROWS3(p_mat_a, X0)\
    {\
        X0.insert(0, load_v<8>(p_mat_a));p_mat_a=byte_incr(p_mat_a, 8*size_bf16);\
    }

    //*********************************************//
    //Load Mat B data and adjust the vectorization.//
    //*********************************************//

    //Y0, Y1, Y2, Y3 must be of type vector<bfloat16,32>
    //
    //Y0 = h0-7w0-3, Y1 = h0-7w4-7, Y2 = h0-7w8-11, Y3 = h0-7w12-15
    //
    #define FC_LOAD_MAT_B_DATA(p_mat_b, iterator_mat_b, param, Y0, Y1, Y2, Y3)\
    {\
        Y0.insert(0, load_v<16>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB1);\
        Y0.insert(1, load_v<16>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB2);\
        Y1.insert(0, load_v<16>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB1);\
        Y1.insert(1, load_v<16>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB2);\
        Y2.insert(0, load_v<16>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB1);\
        Y2.insert(1, load_v<16>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB2);\
        Y3.insert(0, load_v<16>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB1);\
        Y3.insert(1, load_v<16>(p_mat_b));p_mat_b=byte_incr(p_mat_b, 0);\
        p_mat_b = INCR_ITR2D_PARAM(p_mat_b, iterator_mat_b, param);\
    }

    //**************************************************//
    // Bias selection using add_mac_conf()              //
    //**************************************************//

    //4 Bias will feed to the first row of C0 4x4, so its a padding 3 zeros as MatA
    #define BIAS_SELECTION_16_ACC(p_bias_in, zero_vector_0, zero_vector_1, acc_zero_flag, C0_acc, C1_acc, C2_acc, C3_acc)\
    {\
        vector<bfloat16,32>bias_8x4_0,  bias_8x4_1,  bias_8x4_2,  bias_8x4_3;\
        \
        zero_vector_0.insert(0, load_v<8>(p_bias_in)); p_bias_in = byte_incr(p_bias_in, (8*size_bf16));\
        std::tie(bias_8x4_0, bias_8x4_1) = interleave_unzip(zero_vector_0, zero_vector_1, 4);\
        \
        C0_acc.from_vector(bias_8x4_0.extract<16>(0));\
        C1_acc.from_vector(bias_8x4_1.extract<16>(0));\
        C0_acc = addmac_4x8_8x4_conf(X0, Y0, C0_acc, bias_16lanes_0, acc_zero_flag, 0, 0, 0);\
        C1_acc = addmac_4x8_8x4_conf(X0, Y1, C1_acc, bias_16lanes_1, acc_zero_flag, 0, 0, 0);\
        \
        zero_vector_0.insert(0, load_v<8>(p_bias_in)); p_bias_in = byte_incr(p_bias_in, (8*size_bf16));\
        std::tie(bias_8x4_2, bias_8x4_3) = interleave_unzip(zero_vector_0, zero_vector_1, 4);\
        \
        C2_acc.from_vector(bias_8x4_2.extract<16>(0));\
        C3_acc.from_vector(bias_8x4_3.extract<16>(0));\
        C2_acc = addmac_4x8_8x4_conf(X0, Y2, C2_acc, bias_16lanes_2, acc_zero_flag, 0, 0, 0);\
        C3_acc = addmac_4x8_8x4_conf(X0, Y3, C3_acc, bias_16lanes_3, acc_zero_flag, 0, 0, 0);\
    }

    #define FC_STORE_16_ACC_FULL_DATA(p_mat_c, C0, C1, C2, C3) \
    {\
        store_v(p_mat_c, C0.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, 16*size_bf16);\
        store_v(p_mat_c, C1.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, 16*size_bf16);\
        store_v(p_mat_c, C2.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, 16*size_bf16);\
        store_v(p_mat_c, C3.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, 16*size_bf16);\
    }

    SETUP_ITR2D_COUNTERS(iterator_mat_b)

    int acc_zero_flag = param.bias_zero_flag;// because no need p_sum, use this flag for bias

    bfloat16 bfloat16_zero            = 0.0;
    vector<bfloat16,32>zero_vector_0  = (vector<bfloat16,32>)broadcast(bfloat16_zero);
    vector<bfloat16,32>zero_vector_1  = (vector<bfloat16,32>)broadcast(bfloat16_zero);
    vector<bfloat16,32> X0            = (vector<bfloat16,32>)broadcast(bfloat16_zero);

    accum<accfloat,16> bias_16lanes_0, bias_16lanes_1, bias_16lanes_2, bias_16lanes_3;
    vector<bfloat16,16>zero_foracc    = zero_vector_0.extract<16>(0);
    bias_16lanes_0.from_vector(zero_foracc);
    bias_16lanes_1.from_vector(zero_foracc);
    bias_16lanes_2.from_vector(zero_foracc);
    bias_16lanes_3.from_vector(zero_foracc);

    for (int j=0; j<param.outer_count; j++)
        {
        vector<bfloat16,32> Y0, Y1, Y2, Y3;
        accum<accfloat,16> C0_acc, C1_acc, C2_acc, C3_acc;

        bfloat16 *p_A_start = p_mat_a;

        LOAD_MAT_A_DATA_PADDING0_ROWS3(p_A_start, X0);
        FC_LOAD_MAT_B_DATA(p_mat_b, iterator_mat_b, param, Y0, Y1, Y2, Y3);
        BIAS_SELECTION_16_ACC(p_bias_in, zero_vector_0, zero_vector_1, acc_zero_flag, C0_acc, C1_acc, C2_acc, C3_acc);
        //list 4 or 8 MMUL(M*K,K*N)
        MMUL C0(C0_acc), C1(C1_acc), C2(C2_acc), C3(C3_acc);

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(6,) 
        for (int i=0; i<param.inner_count-1; i++){
            LOAD_MAT_A_DATA_PADDING0_ROWS3(p_A_start, X0);
            FC_LOAD_MAT_B_DATA(p_mat_b, iterator_mat_b, param, Y0, Y1, Y2, Y3)
            C0.mac(X0,Y0); C1.mac(X0,Y1); C2.mac(X0,Y2); C3.mac(X0,Y3);
        }

        FC_STORE_16_ACC_FULL_DATA(p_mat_c, C0, C1, C2, C3)

    }
}


//Start of fullyconnect int8 AIE2

static NOINLINE void fullyconnect_internal_int8
(
    int8 * __restrict p_a,
    int8 * __restrict p_b,
    int8 * __restrict p_c,
    int8 * __restrict p_bias,
    fullyconnect_params_t &param
){
    int8 MAT_A_DM_BANK * __restrict p_mat_a    = (int8 MAT_A_DM_BANK * __restrict) p_a;
    int8 MAT_B_DM_BANK * __restrict p_mat_b    = (int8 MAT_B_DM_BANK * __restrict) p_b;
    int8 MAT_C_DM_BANK * __restrict p_mat_c    = (int8 MAT_C_DM_BANK * __restrict) p_c;
    int8 MAT_B_DM_BANK * __restrict p_bias_in  = (int8 MAT_B_DM_BANK * __restrict) p_bias;
    // No need partial sum (p_sum) because partial sum can't get performance improved in FC(IO band)
    // In FC one outer loop, we get n column final result.
    // ifm and ofm size are very small compare to wts in FC, so we have >64KB (128kB*(1-1/col_B)) to load wts in aie2p.

    //using MMUL = mmul<4, 8, 8, int8, int8, 48>;
    const int size_int8 = 1;

    //*********************************************//
    //Load Mat A data and adjust the vectorization.//
    //*********************************************//
    //LOAD_MAT_A_DATA_PADDING0_ROWS3 load one row WHw4 order with padding 0 in next 3 rows
    //Load data in w8 vectorization, load 8 value and insert to the first 8 element of all 0
    //Apply WHw4 for int8 assumption
    //X0 = h0-3w0-7 + h4-8 no useful in mac_4x8_8x8 Intrinsics
    //mat_a_0 = h0w0-7 + h1 zeros
    //
    #define LOAD_MAT_A_DATA_8_PADDING0_ROWS3(p_mat_a, X0, X1, zero_vector_16)\
    {\
        vector<int8,16> mat_a_0 = load_v<16>(p_mat_a);\
        vector<int8,16> mat_2x8_0, mat_2x8_1;\
        std::tie(mat_2x8_0, mat_2x8_1) = interleave_zip(mat_a_0, zero_vector_16, 8);\
        X0.insert(0, mat_2x8_0);X1.insert(0, mat_2x8_1);\
        p_mat_a=byte_incr(p_mat_a, 16*size_int8);\
    }


    //*********************************************//
    //Load Mat B data and adjust the vectorization.//
    //*********************************************//

    //Y0, Y1, Y2, Y3 must be of type vector<int8,64>
    //
    //Y0 = h0-7w0-7, Y1 = h0-7w8-15, Y2 = h0-7w16-23, Y3 = h0-7w24-31
    //tmp0 = h0-7w0-3 + h0-7w8-11
    //
    #define FC_LOAD_MAT_B_DATA_INT8(p_mat_b, iterator_mat_b, param, Y0, Y1, Y2, Y3)\
    {\
        vector<int8,64> tmp0, tmp1, tmp2, tmp3;\
        tmp0.insert(0, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB_int8);\
        tmp1.insert(0, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB_int8);\
        tmp0.insert(1, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB_int8);\
        tmp1.insert(1, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB_int8);\
        tmp2.insert(0, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB_int8);\
        tmp3.insert(0, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB_int8);\
        tmp2.insert(1, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB_int8);\
        tmp3.insert(1, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, 0);\
        p_mat_b = INCR_ITR2D_PARAM(p_mat_b, iterator_mat_b, param);\
        std::tie(Y0, Y1) = interleave_zip(tmp0, tmp1, 4);\
        std::tie(Y2, Y3) = interleave_zip(tmp2, tmp3, 4);\
    }

    //**************************************************//
    // Bias selection using add_mac_conf()              //
    //**************************************************//

    //8 Bias will feed to the first row of C0 4x8, so its a padding 3 zeros as MatA
    #define BIAS_SELECTION_32_ACC(p_bias_in, zero_vector_0, zero_vector_1, acc_zero_flag, C0_acc, C1_acc, C2_acc, C3_acc, X0, Y0, Y1, Y2, Y3)\
    {\
        vector<int8,64>bias_8x8_0,  bias_8x8_1,  bias_8x8_2,  bias_8x8_3;\
        \
        zero_vector_0.insert(0, load_v<16>(p_bias_in)); p_bias_in = byte_incr(p_bias_in, (16*size_int8));\
        std::tie(bias_8x8_0, bias_8x8_1) = interleave_unzip(zero_vector_0, zero_vector_1, 8);\
        \
        C0_acc.from_vector(bias_8x8_0.extract<32>(0));\
        C1_acc.from_vector(bias_8x8_1.extract<32>(0));\
        C0_acc = mac_4x8_8x8_conf(X0, 1, Y0, 1, C0_acc, acc_zero_flag, 0, 0, 0);\
        C1_acc = mac_4x8_8x8_conf(X0, 1, Y1, 1, C1_acc, acc_zero_flag, 0, 0, 0);\
        \
        zero_vector_0.insert(0, load_v<16>(p_bias_in)); p_bias_in = byte_incr(p_bias_in, (16*size_int8));\
        std::tie(bias_8x8_2, bias_8x8_3) = interleave_unzip(zero_vector_0, zero_vector_1, 8);\
        \
        C2_acc.from_vector(bias_8x8_2.extract<32>(0));\
        C3_acc.from_vector(bias_8x8_3.extract<32>(0));\
        C2_acc = mac_4x8_8x8_conf(X0, 1, Y2, 1, C2_acc, acc_zero_flag, 0, 0, 0);\
        C3_acc = mac_4x8_8x8_conf(X0, 1, Y3, 1, C3_acc, acc_zero_flag, 0, 0, 0);\
    }

    #define STORE_32_ACC_FULL_DATA(p_mat_c, C0, C1, C2, C3) \
    {\
        store_v(p_mat_c, C0.template to_vector<int8>(param.int8_shift_out));  p_mat_c = byte_incr(p_mat_c, 32*size_int8);\
        store_v(p_mat_c, C1.template to_vector<int8>(param.int8_shift_out));  p_mat_c = byte_incr(p_mat_c, 32*size_int8);\
        store_v(p_mat_c, C2.template to_vector<int8>(param.int8_shift_out));  p_mat_c = byte_incr(p_mat_c, 32*size_int8);\
        store_v(p_mat_c, C3.template to_vector<int8>(param.int8_shift_out));  p_mat_c = byte_incr(p_mat_c, 32*size_int8);\
    }

    SETUP_ITR2D_COUNTERS(iterator_mat_b)

    int acc_zero_flag = param.bias_zero_flag;// because no need p_sum, use this flag for bias

    int8 zero = 0;
    vector<int8,64> zero_vector_0  = (vector<int8,64>)broadcast(zero);
    vector<int8,64> zero_vector_1  = (vector<int8,64>)broadcast(zero);
    vector<int8,64> zero_vector_2  = (vector<int8,64>)broadcast(zero);
    vector<int8,64> zero_vector_3  = (vector<int8,64>)broadcast(zero);
    vector<int8,64> zero_vector_4  = (vector<int8,64>)broadcast(zero);
    vector<int8,64> zero_vector_5  = (vector<int8,64>)broadcast(zero);
    vector<int8,64> X0             = (vector<int8,64>)broadcast(zero);
    vector<int8,64> X1             = (vector<int8,64>)broadcast(zero);

    vector<int8,16> zero_vector_16   = zero_vector_0.extract<16>(0);


        for (int j=0; j<param.outer_count; j++)
         {
            vector<int8,64> Y0, Y1, Y2, Y3, Y10, Y11, Y12, Y13;

            accum<acc32,32> C0_acc, C1_acc, C2_acc, C3_acc, C10_acc, C11_acc, C12_acc, C13_acc;

            int8 *p_A_start = p_mat_a;

            LOAD_MAT_A_DATA_8_PADDING0_ROWS3(p_A_start, X0, X1, zero_vector_16);
            FC_LOAD_MAT_B_DATA_INT8(p_mat_b, iterator_mat_b, param, Y0, Y1, Y2, Y3);
            FC_LOAD_MAT_B_DATA_INT8(p_mat_b, iterator_mat_b, param, Y10, Y11, Y12, Y13);
            BIAS_SELECTION_32_ACC(p_bias_in, zero_vector_0, zero_vector_1, acc_zero_flag, C0_acc, C1_acc, C2_acc, C3_acc, X0, Y0, Y1, Y2, Y3);
            C10_acc = mul_4x8_8x8(X1, 1, Y10, 1);
            C11_acc = mul_4x8_8x8(X1, 1, Y11, 1);
            C12_acc = mul_4x8_8x8(X1, 1, Y12, 1);
            C13_acc = mul_4x8_8x8(X1, 1, Y13, 1);
            //list 8 MMUL(M*K,K*N)

            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(3,) 
            for (int i=0; i<param.inner_count-1; i++){
                LOAD_MAT_A_DATA_8_PADDING0_ROWS3(p_A_start, X0, X1, zero_vector_16);
                FC_LOAD_MAT_B_DATA_INT8(p_mat_b, iterator_mat_b, param, Y0, Y1, Y2, Y3);
                FC_LOAD_MAT_B_DATA_INT8(p_mat_b, iterator_mat_b, param, Y10, Y11, Y12, Y13);

                C0_acc = mac_4x8_8x8(X0, 1, Y0, 1, C0_acc);//C0.mac(X0,Y0);
                C1_acc = mac_4x8_8x8(X0, 1, Y1, 1, C1_acc);//C1.mac(X0,Y1);
                C2_acc = mac_4x8_8x8(X0, 1, Y2, 1, C2_acc);//C2.mac(X0,Y2);
                C3_acc = mac_4x8_8x8(X0, 1, Y3, 1, C3_acc);//C3.mac(X0,Y3);
                C10_acc = mac_4x8_8x8(X1, 1, Y10, 1, C10_acc);
                C11_acc = mac_4x8_8x8(X1, 1, Y11, 1, C11_acc);
                C12_acc = mac_4x8_8x8(X1, 1, Y12, 1, C12_acc);
                C13_acc = mac_4x8_8x8(X1, 1, Y13, 1, C13_acc);
            }

            C0_acc = addmac_4x8_8x8(zero_vector_4, zero_vector_5, C0_acc, C10_acc);
            C1_acc = addmac_4x8_8x8(zero_vector_4, zero_vector_5, C1_acc, C11_acc);
            C2_acc = addmac_4x8_8x8(zero_vector_4, zero_vector_5, C2_acc, C12_acc);
            C3_acc = addmac_4x8_8x8(zero_vector_4, zero_vector_5, C3_acc, C13_acc);

            STORE_32_ACC_FULL_DATA(p_mat_c, C0_acc, C1_acc, C2_acc, C3_acc);
        }

}

#else //__AIE_ARCH__ != 20, Start of fullyconnect bfloat16 AIE2ps / AIEp kernel


static NOINLINE void fullyconnect_internal_bf16
(
    bfloat16 * __restrict p_a,
    bfloat16 * __restrict p_b,
    bfloat16 * __restrict p_c,
    bfloat16 * __restrict p_bias,
    fullyconnect_params_t &param
){
    bfloat16 MAT_A_DM_BANK * __restrict p_mat_a    = (bfloat16 MAT_A_DM_BANK * __restrict) p_a;
    bfloat16 MAT_B_DM_BANK * __restrict p_mat_b    = (bfloat16 MAT_B_DM_BANK * __restrict) p_b;
    bfloat16 MAT_C_DM_BANK * __restrict p_mat_c    = (bfloat16 MAT_C_DM_BANK * __restrict) p_c;
    bfloat16 MAT_B_DM_BANK * __restrict p_bias_in  = (bfloat16 MAT_B_DM_BANK * __restrict) p_bias;

    using MMUL = mmul<FC_DIM_M, FC_DIM_K, FC_DIM_N, bfloat16, bfloat16>;
    SETUP_ITR2D_COUNTERS(iterator_mat_b)

    const int acc_zero_flag = param.bias_zero_flag;// because no need p_sum, use this flag for bias
    const int size_bf16 = 2;

    bfloat16 bfloat16_zero   = 0.0;
    bfloat16 bfloat16_one    = 1.0;

    accum<accfloat,32>  bias_32lanes_0, bias_32lanes_1, bias_32lanes_2, bias_32lanes_3;
    vector<bfloat16,32> zero_foracc    = (vector<bfloat16,32>)broadcast(bfloat16_zero);
    bias_32lanes_0.from_vector(zero_foracc);
    bias_32lanes_1.from_vector(zero_foracc);
    bias_32lanes_2.from_vector(zero_foracc);
    bias_32lanes_3.from_vector(zero_foracc);


    //*********************************************//
    //Load Mat A data and adjust the vectorization.//
    //*********************************************//

    //X0 must be of type vector<bfloat16,32>
    //Load data in w4 vectorization, load 8 value and insert to the first 8 element of all 0
    //X0 = h0-3w0-7
    vector<bfloat16,32> X0  = (vector<bfloat16,32>)broadcast(bfloat16_zero);
    #define LOAD_MAT_A_DATA_PADDING0_ROWS3(p_mat_a, X0)\
    {\
        X0.insert(0, load_v<8>(p_mat_a));p_mat_a=byte_incr(p_mat_a, 8*size_bf16);\
    }

    //*********************************************//
    //Load Mat B data and adjust the vectorization.//
    //*********************************************//

    //Y0, Y1, Y2, Y3 must be of type vector<bfloat16,64>
    //
    //ld0 = h0-7w0-3, ld1 = h0-7w4-7, ld2 = h0-7w8-11, ld3 = h0-7w12-15
    //tmp0 = ld0 + ld2, tmp1 = ld1 + ld3
    //Y0= h0-7w0-7 Y1= h0-7w8-15  (Y0, Y1) = interleave_zip(tmp0, tmp1, 4)
    //
    #define LOAD_MAT_B_DATA_2_8x8(p_mat_b, iterator_mat_b, param, Y0, Y1, Y2, Y3)\
    {\
        vector<bfloat16,64> tmp0, tmp1, tmp2, tmp3;\
        tmp0.insert(0, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB1);\
        tmp1.insert(0, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB1);\
        tmp0.insert(1, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB1);/*printf("\n[address] Y1 : %d\n", p_mat_b);*/\
        tmp1.insert(1, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB1);/*printf("[address] Y2 : %d\n", p_mat_b);*/\
        \
        tmp2.insert(0, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB1);\
        tmp3.insert(0, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB1);\
        tmp2.insert(1, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB1);/*printf("[address] Y3 : %d\n", p_mat_b);*/\
        tmp3.insert(1, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, 0);/*printf("[address] Y3 before ITR2D : %d\n", p_mat_b);*/\
        p_mat_b = INCR_ITR2D_PARAM(p_mat_b, iterator_mat_b, param);/*printf("[address] Y0 ITR2D : %d\n", p_mat_b);*/\
        std::tie(Y0, Y1) = interleave_zip(tmp0, tmp1, 4);\
        std::tie(Y2, Y3) = interleave_zip(tmp2, tmp3, 4);\
    }

    //**************************************************//
    // ADD_MAC_CONF for aie2p and aie2ps                //
    //**************************************************//

    #define ADD_MAC_CONF(vec_0, vec_1, acc_0, acc_1, acc_zero_flag)\
    {\
        MMUL M(op_zero(acc_0, acc_zero_flag));\
        M.mac(vec_0, vec_1);\
        acc_0 = add(M.to_accum(), acc_1);\
    }

    //**************************************************//
    // Bias selection using add_mac_conf()              //
    //**************************************************//

    //4 Bias will feed to the first row of C0 4x4, so its a padding 3 zeros as MatA
    #define BIAS_SELECTION_ACC_8_4x8(p_bias_in, bias_32lanes_0, bias_32lanes_1, bias_32lanes_2, bias_32lanes_3, acc_zero_flag, C0_acc, C1_acc, C2_acc, C3_acc, X0, Y0, Y1, Y2, Y3)\
    {\
        vector<bfloat16,32> bias_4x8_0, bias_4x8_1, bias_4x8_2, bias_4x8_3;\
        \
        vector<bfloat16,32> zero_vector = (vector<bfloat16,32>)broadcast(bfloat16_zero);\
        zero_vector.insert(0, zero_vector);\
        bias_4x8_0 = zero_vector.insert(0, load_v<8>(p_bias_in)); p_bias_in = byte_incr(p_bias_in, (8*2));\
        bias_4x8_1 = zero_vector.insert(0, load_v<8>(p_bias_in)); p_bias_in = byte_incr(p_bias_in, (8*2));\
        bias_4x8_2 = zero_vector.insert(0, load_v<8>(p_bias_in)); p_bias_in = byte_incr(p_bias_in, (8*2));\
        bias_4x8_3 = zero_vector.insert(0, load_v<8>(p_bias_in)); p_bias_in = byte_incr(p_bias_in, (8*2));\
        \
        C0_acc.from_vector(bias_4x8_0);C1_acc.from_vector(bias_4x8_1);\
        C2_acc.from_vector(bias_4x8_2);C3_acc.from_vector(bias_4x8_3);\
        \
        ADD_MAC_CONF(X0, Y0, C0_acc, bias_32lanes_0, acc_zero_flag)\
        ADD_MAC_CONF(X0, Y1, C1_acc, bias_32lanes_1, acc_zero_flag)\
        ADD_MAC_CONF(X0, Y2, C2_acc, bias_32lanes_2, acc_zero_flag)\
        ADD_MAC_CONF(X0, Y3, C3_acc, bias_32lanes_3, acc_zero_flag)\
        \
    }

    //***************************************************************************//
    //Convert float32 accumulator data to bfloat16 data and store in WHw4 format //
    //***************************************************************************//

    //C0, C1, C2, C3 are of type mmul<4, 8, 8, bfloat16, bfloat16>
    //reorder C0 to 2 4x4, so they match WHw4
    #define STORE_ACC_8_4x8_FULL_DATA(p_mat_c, C0, C1, C2, C3) \
    {\
        vector<bfloat16,32> C0_vec = C0.template to_vector<bfloat16>(16);\
        vector<bfloat16,32> C1_vec = C1.template to_vector<bfloat16>(16);\
        vector<bfloat16,32> C2_vec = C2.template to_vector<bfloat16>(16);\
        vector<bfloat16,32> C3_vec = C3.template to_vector<bfloat16>(16);\
        \
        vector<bfloat16,32> tmp0_vec, tmp1_vec, tmp2_vec, tmp3_vec;\
        tmp0_vec.insert(0, filter_even(C0_vec, 4));\
        tmp0_vec.insert(1, filter_odd (C0_vec, 4));\
        tmp1_vec.insert(0, filter_even(C1_vec, 4));\
        tmp1_vec.insert(1, filter_odd (C1_vec, 4));\
        tmp2_vec.insert(0, filter_even(C2_vec, 4));\
        tmp2_vec.insert(1, filter_odd (C2_vec, 4));\
        tmp3_vec.insert(0, filter_even(C3_vec, 4));\
        tmp3_vec.insert(1, filter_odd (C3_vec, 4));\
        \
        store_v(p_mat_c, tmp0_vec);  p_mat_c = byte_incr(p_mat_c, 32*2);\
        store_v(p_mat_c, tmp1_vec);  p_mat_c = byte_incr(p_mat_c, 32*2);\
        store_v(p_mat_c, tmp2_vec);  p_mat_c = byte_incr(p_mat_c, 32*2);\
        store_v(p_mat_c, tmp3_vec);  p_mat_c = byte_incr(p_mat_c, 32*2);\
    }

    //*************************//
    //Inner loop mac operations//
    //*************************//

    //C0, C1, C2, C3, C4, C5, C6, C7 are of type mmul<4, 8, 8, bfloat16, bfloat16>
    //X0, X1, X2, X3 must be of type vector<bfloat16,32>
    //Y0, Y1 must be of type vector<bfloat16,64>
    #define MMUL_MAC_8_4x8x8(C0, C1, C2, C3, X0, Y0, Y1, Y2, Y3)\
    {\
        C0.mac(X0,Y0); C1.mac(X0,Y1); C2.mac(X0,Y2); C3.mac(X0,Y3);\
    }

    //printf("AIE2P 2PS's outer, inner : %d , %d\n", param.outer_count, param.inner_count);

    for (int j=0; j<param.outer_count; j++)
    {
        vector<bfloat16,64> Y0, Y1, Y2, Y3;
        accum<accfloat, 32> C0_acc, C1_acc, C2_acc, C3_acc;

        bfloat16 *p_A_start = p_mat_a;

        LOAD_MAT_A_DATA_PADDING0_ROWS3(p_A_start, X0);
        LOAD_MAT_B_DATA_2_8x8(p_mat_b, iterator_mat_b, param, Y0, Y1, Y2, Y3);
        BIAS_SELECTION_ACC_8_4x8(p_bias_in, bias_32lanes_0, bias_32lanes_1, bias_32lanes_2, bias_32lanes_3,\
                                acc_zero_flag, C0_acc, C1_acc, C2_acc, C3_acc, X0, Y0, Y1, Y2, Y3);

        MMUL C0(C0_acc), C1(C1_acc), C2(C2_acc), C3(C3_acc);

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for (int i=0; i<param.inner_count-1; i++){
            LOAD_MAT_A_DATA_PADDING0_ROWS3(p_A_start, X0);
            LOAD_MAT_B_DATA_2_8x8(p_mat_b, iterator_mat_b, param, Y0, Y1, Y2, Y3)
            MMUL_MAC_8_4x8x8(C0, C1, C2, C3, X0, Y0, Y1, Y2, Y3)
        }

        STORE_ACC_8_4x8_FULL_DATA(p_mat_c, C0, C1, C2, C3);

    }

}


//Start of fullyconnect int8 AIE2ps / AIEp kernel


static NOINLINE void fullyconnect_internal_int8
(
    int8 * __restrict p_a,
    int8 * __restrict p_b,
    int8 * __restrict p_c,
    int8 * __restrict p_bias,
    fullyconnect_params_t &param
){
    int8 MAT_A_DM_BANK * __restrict p_mat_a    = (int8 MAT_A_DM_BANK * __restrict) p_a;
    int8 MAT_B_DM_BANK * __restrict p_mat_b    = (int8 MAT_B_DM_BANK * __restrict) p_b;
    int8 MAT_C_DM_BANK * __restrict p_mat_c    = (int8 MAT_C_DM_BANK * __restrict) p_c;
    int8 MAT_B_DM_BANK * __restrict p_bias_in  = (int8 MAT_B_DM_BANK * __restrict) p_bias;
    // No need partial sum (p_sum) because partial sum can't get performance improved in FC(IO band)
    // In FC one outer loop, we get n column final result.
    // ifm and ofm size are very small compare to wts in FC, so we have >64KB (128kB*(1-1/col_B)) to load wts in aie2p.

    using MMUL = mmul<4, 8, 8, int8, int8>;
    const int size_int8 = 1;

    //*********************************************//
    //Load Mat A data and adjust the vectorization.//
    //*********************************************//
    //LOAD_MAT_A_DATA_PADDING0_ROWS3 load one row WHw8 order with padding 0 in next 3 rows
    //Load data in w8 vectorization, load 8 value and insert to the first 8 element of all 0
    //X0 = h0-7w0-7 in mac_8x8_8x8 high_level API
    //mat_a_0 = h0w0-15 + h1 zeros
    //
    #define LOAD_MAT_A_DATA_8_PADDING0_ROWS3(p_mat_a, X0, X1, zero_vector_16)\
    {\
        vector<int8,16> mat_a_0 = load_v<16>(p_mat_a);\
        vector<int8,16> mat_2x8_0, mat_2x8_1;\
        std::tie(mat_2x8_0, mat_2x8_1) = interleave_zip(mat_a_0, zero_vector_16, 8);\
        X0.insert(0, mat_2x8_0);X1.insert(0, mat_2x8_1);\
        p_mat_a=byte_incr(p_mat_a, 16*size_int8);\
    }


    //*********************************************//
    //Load Mat B data and adjust the vectorization.//
    //*********************************************//

    //Y0, Y1, Y2, Y3 must be of type vector<int8,64>
    //
    //Y0 = h0-7w0-7, Y1 = h0-7w8-15, Y2 = h0-7w16-23, Y3 = h0-7w24-31
    //tmp0 = h0-7w0-3 + h0-7w8-11
    //
    #define FC_LOAD_MAT_B_DATA_INT8(p_mat_b, iterator_mat_b, param, Y0, Y1)\
    {\
        vector<int8,64> tmp0, tmp1, tmp2, tmp3;\
        tmp0.insert(0, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB_int8);\
        tmp1.insert(0, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB_int8);\
        tmp0.insert(1, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB_int8);\
        tmp1.insert(1, load_v<32>(p_mat_b));p_mat_b=byte_incr(p_mat_b, 0);\
        p_mat_b = INCR_ITR2D_PARAM(p_mat_b, iterator_mat_b, param);\
        std::tie(Y0, Y1) = interleave_zip(tmp0, tmp1, 4);\
    }

    //**************************************************//
    // Bias selection using add_mac_conf()              //
    //**************************************************//

    //8 Bias will feed to the first row of C0 4x8, so its a padding 3 zeros as MatA
    #define BIAS_SELECTION_32_ACC(p_bias_in, zero_vector_0, zero_vector_1, acc_zero_flag, C0_acc, C1_acc, X0, Y0, Y1)\
    {\
        vector<int8,64>bias_8x8_0,  bias_8x8_1,  bias_8x8_2,  bias_8x8_3;\
        \
        zero_vector_0.insert(0, load_v<16>(p_bias_in)); p_bias_in = byte_incr(p_bias_in, (16*size_int8));\
        std::tie(bias_8x8_0, bias_8x8_1) = interleave_unzip(zero_vector_0, zero_vector_1, 8);\
        \
        C0_acc.from_vector(bias_8x8_0);\
        C1_acc.from_vector(bias_8x8_1);\
        C0_acc = mac_8x8_8x8_conf(X0, 1, Y0, 1, C0_acc, acc_zero_flag, 0, 0, 0);\
        C1_acc = mac_8x8_8x8_conf(X0, 1, Y1, 1, C1_acc, acc_zero_flag, 0, 0, 0);\
    }

    //only store first 32, not store second 32zeros
    #define STORE_32_ACC_FULL_DATA(p_mat_c, C0, C1) \
    {\
        vector<int8,64> C_8x8_0,  C_8x8_1,  C_8x8_2,  C_8x8_3;\
        C_8x8_0 = C0.template to_vector<int8>(param.int8_shift_out);\
        C_8x8_1 = C1.template to_vector<int8>(param.int8_shift_out);\
        store_v(p_mat_c, C_8x8_0.extract<32>(0));  p_mat_c = byte_incr(p_mat_c, 32*size_int8);\
        store_v(p_mat_c, C_8x8_1.extract<32>(0));  p_mat_c = byte_incr(p_mat_c, 32*size_int8);\
    }

    SETUP_ITR2D_COUNTERS(iterator_mat_b)

    int acc_zero_flag = param.bias_zero_flag;// because no need p_sum, use this flag for bias

    int8 zero = 0;
    vector<int8,64> zero_vector_0  = (vector<int8,64>)broadcast(zero);
    vector<int8,64> zero_vector_1  = (vector<int8,64>)broadcast(zero);
    vector<int8,64> zero_vector_2  = (vector<int8,64>)broadcast(zero);
    vector<int8,64> zero_vector_3  = (vector<int8,64>)broadcast(zero);
    vector<int8,64> zero_vector_4  = (vector<int8,64>)broadcast(zero);
    vector<int8,64> zero_vector_5  = (vector<int8,64>)broadcast(zero);
    vector<int8,64> X0             = (vector<int8,64>)broadcast(zero);
    vector<int8,64> X1             = (vector<int8,64>)broadcast(zero);

    vector<int8,16> zero_vector_16   = zero_vector_0.extract<16>(0);

    for (int j=0; j<param.outer_count; j++)
        {
        vector<int8,64> Y0, Y1, Y10, Y11;

        accum<acc32,64> C0_acc, C1_acc, C10_acc, C11_acc;

        int8 *p_A_start = p_mat_a;

        LOAD_MAT_A_DATA_8_PADDING0_ROWS3(p_A_start, X0, X1, zero_vector_16);
        FC_LOAD_MAT_B_DATA_INT8(p_mat_b, iterator_mat_b, param, Y0, Y1);
        FC_LOAD_MAT_B_DATA_INT8(p_mat_b, iterator_mat_b, param, Y10, Y11);
        BIAS_SELECTION_32_ACC(p_bias_in, zero_vector_0, zero_vector_1, acc_zero_flag, C0_acc, C1_acc, X0, Y0, Y1);
        C10_acc = mul_8x8_8x8(X1, 1, Y10, 1);
        C11_acc = mul_8x8_8x8(X1, 1, Y11, 1);
        //list 8 MMUL(M*K,K*N)

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(3,) 
        for (int i=0; i<param.inner_count-1; i++){
            LOAD_MAT_A_DATA_8_PADDING0_ROWS3(p_A_start, X0, X1, zero_vector_16);
            FC_LOAD_MAT_B_DATA_INT8(p_mat_b, iterator_mat_b, param, Y0, Y1);
            FC_LOAD_MAT_B_DATA_INT8(p_mat_b, iterator_mat_b, param, Y10, Y11);
            C0_acc = mac_8x8_8x8(X0, 1, Y0, 1, C0_acc);//C0.mac(X0,Y0);
            C1_acc = mac_8x8_8x8(X0, 1, Y1, 1, C1_acc);//C1.mac(X0,Y1);
            C10_acc = mac_8x8_8x8(X1, 1, Y10, 1, C10_acc);
            C11_acc = mac_8x8_8x8(X1, 1, Y11, 1, C11_acc);
        }

        C0_acc = addmac_8x8_8x8(zero_vector_4, zero_vector_5, C0_acc, C10_acc);
        C1_acc = addmac_8x8_8x8(zero_vector_4, zero_vector_5, C1_acc, C11_acc);

        STORE_32_ACC_FULL_DATA(p_mat_c, C0_acc, C1_acc);
    }

}

#endif //__AIE_ARCH__ == 20

//top
template <typename dtype, typename config_ifm, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
NOINLINE void fullyconnect( adf::input_buffer_conf<dtype, config_ifm> &__restrict ifm,
                            adf::input_buffer_conf<dtype, config_ifm> &__restrict wts,
                            adf::output_buffer_conf<dtype, config_ofm> &__restrict ofm,
                            fullyconnect_params_t &params) {
    // Output modes
    const out_mode_t out_mode_full     = FULL ;

    // Setup iteration level parameters like IO mode etc
    setup_fullyconnect_iter_params(params);

    // Extract low level pointers used by the actual kernel code
    dtype * input_a      = (dtype *) ifm.data();
    dtype * input_b      = (dtype *) wts.data() + ((params.bias_len + FC_LAYER_PARAM_BYTES)/sizeof(dtype));
    dtype * output_c     = (dtype *) ofm.data();
    dtype * input_bias   = (dtype *) wts.data() + (FC_LAYER_PARAM_BYTES/sizeof(dtype));

    if constexpr(std::is_same<bfloat16, dtype>::value) {
        fullyconnect_internal_bf16(input_a, input_b, output_c, input_bias, params); // out_mode_full
    }else{
        fullyconnect_internal_int8(input_a, input_b, output_c, input_bias, params);
    }

}

#endif //__fullyconnect_H__

