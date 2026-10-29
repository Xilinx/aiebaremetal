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


#ifndef __GEMM_H__
#define __GEMM_H__

#include "aie.hpp"
#include <adf.h>
#include <stdio.h>
#include "utils.h"
#include "gemm_params.h"
#include "mllib_const.h"
using namespace aie;

#if __AIE_ARCH__ == 20

//Start of gemm bfloat16 AIE2

template <out_mode_t out_mode, act_t act_type>
NOINLINE void gemm_generic
(    
    bfloat16 * __restrict p_a,
    bfloat16 * __restrict p_b,
    bfloat16 * __restrict p_c,
    bfloat16 * __restrict p_bias,
    float    * __restrict p_psum,
    gemm_params_t &param
){
    bfloat16 MAT_A_DM_BANK * __restrict p_mat_a    = (bfloat16 MAT_A_DM_BANK * __restrict) p_a;
    bfloat16 MAT_B_DM_BANK * __restrict p_mat_b    = (bfloat16 MAT_B_DM_BANK * __restrict) p_b;
    bfloat16 MAT_C_DM_BANK * __restrict p_mat_c    = (bfloat16 MAT_C_DM_BANK * __restrict) p_c;
    bfloat16 MAT_B_DM_BANK * __restrict p_bias_in  = (bfloat16 MAT_B_DM_BANK * __restrict) p_bias;
    float    PSUM_DM_BANK  * __restrict p_init32   = (float    PSUM_DM_BANK  * __restrict) p_psum;
    float    PSUM_DM_BANK  * __restrict p_psum_out = (float    PSUM_DM_BANK  * __restrict) p_psum; 
        
    using MMUL = mmul<GEMM_DIM_M, GEMM_DIM_K, GEMM_DIM_N, bfloat16, bfloat16>;   

    #define LOAD_DATA(ptr, buff, incr) \
    {\
        buff.insert(0, load_v<16>(ptr));ptr=byte_incr(ptr, 32);\
        buff.insert(1, load_v<16>(ptr));ptr=byte_incr(ptr, incr);\
    }

    #define LOAD_MAT_A_DATA(p_mat_a, iterator_mat_a, param, X0, X1, X2, X3)\
    {\
        LOAD_DATA(p_mat_a, X0, 32);\
        LOAD_DATA(p_mat_a, X1, 32);\
        p_mat_a = byte_incr(p_mat_a, param.incAI);\
        LOAD_DATA(p_mat_a, X2, 32);\
        LOAD_DATA(p_mat_a, X3, 0);\
        p_mat_a = INCR_ITR3D_PARAM(p_mat_a, iterator_mat_a, param)\
    }

    #define MAT_A_DATA_w4_TO_w8(X0, X1, X2, X3, X00, X01, X02, X03)\
    {\
        std::tie(X00, X01) = interleave_zip(X0,X2, 4);\
        std::tie(X02, X03) = interleave_zip(X1,X3, 4);\
    }

    #define LOAD_MAT_B_DATA(p_mat_b, iterator_mat_b, param, Y0, Y1, Y2, Y3)\
    {\
        Y0.insert(0, load_v<16>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB1);\
        Y0.insert(1, load_v<16>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB2);\
        Y1.insert(0, load_v<16>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB1);\
        Y1.insert(1, load_v<16>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB2);\
        Y2.insert(0, load_v<16>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB1);\
        Y2.insert(1, load_v<16>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB2);\
        Y3.insert(0, load_v<16>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB1);\
        Y3.insert(1, load_v<16>(p_mat_b));p_mat_b=byte_incr(p_mat_b, 0);\
        p_mat_b = INCR_ITR3D_PARAM(p_mat_b, iterator_mat_b, param)\
    }

    #define TRANSPOSE_MAT_B_DATA(param, Y0, Y1, Y2, Y3)\
    {\
        Y0 = shuffle(Y0, Y0, param.mat_b_Imode_0);\
        Y1 = shuffle(Y1, Y1, param.mat_b_Imode_0);\
        Y2 = shuffle(Y2, Y2, param.mat_b_Imode_0);\
        Y3 = shuffle(Y3, Y3, param.mat_b_Imode_0);\
    }

    #define LOAD_ACC_DATA(ptr, buff, incr) \
    {\
        buff = insert(buff, 0, *(v8accfloat PSUM_DM_BANK *)ptr); ptr=byte_incr(ptr, 32);\
        buff = insert(buff, 1, *(v8accfloat PSUM_DM_BANK *)ptr); ptr=byte_incr(ptr, incr);\
    }       

    #define LOAD_16_ACC_DATA(p_init32, iterator_psum_in, param, C0_acc, C1_acc, C2_acc, C3_acc, C4_acc, C5_acc, C6_acc, C7_acc, C8_acc, C9_acc, Ca_acc, Cb_acc, Cc_acc, Cd_acc, Ce_acc, Cf_acc)\
    {\
        LOAD_ACC_DATA(p_init32, C0_acc, 32);\
        LOAD_ACC_DATA(p_init32, C1_acc, 32);\
        LOAD_ACC_DATA(p_init32, C2_acc, 32);\
        LOAD_ACC_DATA(p_init32, C3_acc, param.psum_incC1);\
        LOAD_ACC_DATA(p_init32, C4_acc, 32);\
        LOAD_ACC_DATA(p_init32, C5_acc, 32);\
        LOAD_ACC_DATA(p_init32, C6_acc, 32);\
        LOAD_ACC_DATA(p_init32, C7_acc, param.psum_incC1);\
        LOAD_ACC_DATA(p_init32, C8_acc, 32);\
        LOAD_ACC_DATA(p_init32, C9_acc, 32);\
        LOAD_ACC_DATA(p_init32, Ca_acc, 32);\
        LOAD_ACC_DATA(p_init32, Cb_acc, param.psum_incC1);\
        LOAD_ACC_DATA(p_init32, Cc_acc, 32);\
        LOAD_ACC_DATA(p_init32, Cd_acc, 32);\
        LOAD_ACC_DATA(p_init32, Ce_acc, 32);\
        LOAD_ACC_DATA(p_init32, Cf_acc, 0);\
        p_init32 = INCR_ITR2D_PARAM(p_init32, iterator_psum_in, param)\
    }

    #define PARTIAL_SUM_OR_BIAS_SELECTION_16_ACC(p_bias_in, bias_boardcast, zero_vector_0, zero_vector_1, acc_zero_flag, C0_acc, C1_acc, C2_acc, C3_acc, C4_acc, C5_acc, C6_acc, C7_acc, C8_acc, C9_acc, Ca_acc, Cb_acc, Cc_acc, Cd_acc, Ce_acc, Cf_acc)\
    {\
        accum<accfloat,16> bias_16lanes_0, bias_16lanes_1, bias_16lanes_2, bias_16lanes_3;\
        vector<bfloat16,32>bias_4lanes_0,  bias_4lanes_1,  bias_4lanes_2,  bias_4lanes_3;\
        \
        zero_vector_0.insert(0, load_v<8>(p_bias_in)); p_bias_in = byte_incr(p_bias_in, (8*2));\
        std::tie(bias_4lanes_0, bias_4lanes_1) = interleave_unzip(zero_vector_0, zero_vector_1, 4);\
        \
        bias_16lanes_0 = mul_4x8_8x4(bias_boardcast, bias_4lanes_0);\
        C0_acc = addmac_4x8_8x4_conf(X00, Y0, C0_acc, bias_16lanes_0, acc_zero_flag, 0, 0, 0);\
        C1_acc = addmac_4x8_8x4_conf(X01, Y0, C1_acc, bias_16lanes_0, acc_zero_flag, 0, 0, 0);\
        C2_acc = addmac_4x8_8x4_conf(X02, Y0, C2_acc, bias_16lanes_0, acc_zero_flag, 0, 0, 0);\
        C3_acc = addmac_4x8_8x4_conf(X03, Y0, C3_acc, bias_16lanes_0, acc_zero_flag, 0, 0, 0);\
        \
        bias_16lanes_1 = mul_4x8_8x4(bias_boardcast, bias_4lanes_1);\
        C4_acc = addmac_4x8_8x4_conf(X00, Y1, C4_acc, bias_16lanes_1, acc_zero_flag, 0, 0, 0);\
        C5_acc = addmac_4x8_8x4_conf(X01, Y1, C5_acc, bias_16lanes_1, acc_zero_flag, 0, 0, 0);\
        C6_acc = addmac_4x8_8x4_conf(X02, Y1, C6_acc, bias_16lanes_1, acc_zero_flag, 0, 0, 0);\
        C7_acc = addmac_4x8_8x4_conf(X03, Y1, C7_acc, bias_16lanes_1, acc_zero_flag, 0, 0, 0);\
        \
        zero_vector_0.insert(0, load_v<8>(p_bias_in)); p_bias_in = byte_incr(p_bias_in, -(8*2));\
        std::tie(bias_4lanes_2, bias_4lanes_3) = interleave_unzip(zero_vector_0, zero_vector_1, 4);\
        \
        bias_16lanes_2 = mul_4x8_8x4(bias_boardcast, bias_4lanes_2);\
        C8_acc = addmac_4x8_8x4_conf(X00, Y2, C8_acc, bias_16lanes_2, acc_zero_flag, 0, 0, 0);\
        C9_acc = addmac_4x8_8x4_conf(X01, Y2, C9_acc, bias_16lanes_2, acc_zero_flag, 0, 0, 0);\
        Ca_acc = addmac_4x8_8x4_conf(X02, Y2, Ca_acc, bias_16lanes_2, acc_zero_flag, 0, 0, 0);\
        Cb_acc = addmac_4x8_8x4_conf(X03, Y2, Cb_acc, bias_16lanes_2, acc_zero_flag, 0, 0, 0);\
        \
        bias_16lanes_3 = mul_4x8_8x4(bias_boardcast, bias_4lanes_3);\
        Cc_acc = addmac_4x8_8x4_conf(X00, Y3, Cc_acc, bias_16lanes_3, acc_zero_flag, 0, 0, 0);\
        Cd_acc = addmac_4x8_8x4_conf(X01, Y3, Cd_acc, bias_16lanes_3, acc_zero_flag, 0, 0, 0);\
        Ce_acc = addmac_4x8_8x4_conf(X02, Y3, Ce_acc, bias_16lanes_3, acc_zero_flag, 0, 0, 0);\
        Cf_acc = addmac_4x8_8x4_conf(X03, Y3, Cf_acc, bias_16lanes_3, acc_zero_flag, 0, 0, 0);\
    }

    #define STORE_ACC_DATA(ptr, buff, incr) \
    {\
        *(v8accfloat PSUM_DM_BANK *)ptr = buff.extract<8>(0);  ptr = byte_incr(ptr, 32);\
        *(v8accfloat PSUM_DM_BANK *)ptr = buff.extract<8>(1);  ptr = byte_incr(ptr, incr);\
    }

    #define STORE_16_ACC_PARTIAL_DATA(p_psum_out, iterator_psum_out, param, C0, C1, C2, C3, C4, C5, C6, C7, C8, C9, Ca, Cb, Cc, Cd, Ce, Cf, p_bias_in) \
    {\
        STORE_ACC_DATA(p_psum_out, C0.to_accum(), 32);\
        STORE_ACC_DATA(p_psum_out, C1.to_accum(), 32);\
        STORE_ACC_DATA(p_psum_out, C2.to_accum(), 32);\
        STORE_ACC_DATA(p_psum_out, C3.to_accum(), param.psum_incC1);\
        STORE_ACC_DATA(p_psum_out, C4.to_accum(), 32);\
        STORE_ACC_DATA(p_psum_out, C5.to_accum(), 32);\
        STORE_ACC_DATA(p_psum_out, C6.to_accum(), 32);\
        STORE_ACC_DATA(p_psum_out, C7.to_accum(), param.psum_incC1);\
        STORE_ACC_DATA(p_psum_out, C8.to_accum(), 32);\
        STORE_ACC_DATA(p_psum_out, C9.to_accum(), 32);\
        STORE_ACC_DATA(p_psum_out, Ca.to_accum(), 32);\
        STORE_ACC_DATA(p_psum_out, Cb.to_accum(), param.psum_incC1);\
        STORE_ACC_DATA(p_psum_out, Cc.to_accum(), 32);\
        STORE_ACC_DATA(p_psum_out, Cd.to_accum(), 32);\
        STORE_ACC_DATA(p_psum_out, Ce.to_accum(), 32);\
        STORE_ACC_DATA(p_psum_out, Cf.to_accum(), 0);\
        p_psum_out = INCR_ITR2D_PARAM(p_psum_out, iterator_psum_out, param)\
        p_bias_in = byte_incr(p_bias_in, (16*2)*(iterator_psum_out_cnt0 == 0));\
    }

    #define STORE_16_ACC_FULL_DATA(p_mat_c, iterator_mat_c, param, C0, C1, C2, C3, C4, C5, C6, C7, C8, C9, Ca, Cb, Cc, Cd, Ce, Cf, p_bias_in) \
    {\
        store_v(p_mat_c, C0.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, 32);\
        store_v(p_mat_c, C1.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, 32);\
        store_v(p_mat_c, C2.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, 32);\
        store_v(p_mat_c, C3.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, param.incC1);\
        store_v(p_mat_c, C4.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, 32);\
        store_v(p_mat_c, C5.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, 32);\
        store_v(p_mat_c, C6.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, 32);\
        store_v(p_mat_c, C7.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, param.incC1);\
        store_v(p_mat_c, C8.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, 32);\
        store_v(p_mat_c, C9.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, 32);\
        store_v(p_mat_c, Ca.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, 32);\
        store_v(p_mat_c, Cb.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, param.incC1);\
        store_v(p_mat_c, Cc.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, 32);\
        store_v(p_mat_c, Cd.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, 32);\
        store_v(p_mat_c, Ce.template to_vector<bfloat16>(16));  p_mat_c = byte_incr(p_mat_c, 32);\
        store_v(p_mat_c, Cf.template to_vector<bfloat16>(16));  p_mat_c = INCR_ITR2D_PARAM(p_mat_c, iterator_mat_c, param)\
        p_bias_in = byte_incr(p_bias_in, (16*2)*(iterator_mat_c_cnt0 == 0));\
    }

    SETUP_ITR3D_COUNTERS(iterator_mat_a)
    SETUP_ITR3D_COUNTERS(iterator_mat_b)
    SETUP_ITR2D_COUNTERS(iterator_mat_c)
    SETUP_ITR2D_COUNTERS(iterator_psum_in)
    SETUP_ITR2D_COUNTERS(iterator_psum_out)
       
    int acc_zero_flag = 1-param.psum_in;
     
    bfloat16 bfloat16_zero   = 0.0;
    bfloat16 bfloat16_one    = 1.0;
    bfloat16 bias_bcst_value =  (1-param.bias_zero_flag) ? bfloat16_one : bfloat16_zero;
    vector<bfloat16,32>bias_boardcast = (vector<bfloat16,32>)broadcast(bias_bcst_value);
    
    vector<bfloat16,32>zero_vector_0  = (vector<bfloat16,32>)broadcast(bfloat16_zero);
    vector<bfloat16,32>zero_vector_1  = (vector<bfloat16,32>)broadcast(bfloat16_zero);
    
    if constexpr(out_mode == FULL){
        
        for (int j=0; j<param.outer_count; j++)
         {   	 
            vector<bfloat16,32> X0, X1, X2, X3, Y0, Y1, Y2, Y3, X00, X01, X02, X03;         

            accum<accfloat,16> C0_acc, C1_acc, C2_acc, C3_acc, C4_acc, C5_acc, C6_acc, C7_acc, C8_acc, C9_acc, Ca_acc, Cb_acc, Cc_acc, Cd_acc, Ce_acc, Cf_acc;          

            LOAD_MAT_A_DATA(p_mat_a, iterator_mat_a, param, X0, X1, X2, X3)        

            MAT_A_DATA_w4_TO_w8(X0, X1, X2, X3, X00, X01, X02, X03)

            LOAD_MAT_B_DATA(p_mat_b, iterator_mat_b, param, Y0, Y1, Y2, Y3)

            #if GEMM_TRANSPOSE_MAT_B_SUPPORT == 1
            TRANSPOSE_MAT_B_DATA(param, Y0, Y1, Y2, Y3)
            #endif

            LOAD_16_ACC_DATA(p_init32, iterator_psum_in, param, C0_acc, C1_acc, C2_acc, C3_acc, C4_acc, C5_acc, C6_acc, C7_acc, C8_acc, C9_acc, Ca_acc, Cb_acc, Cc_acc, Cd_acc, Ce_acc, Cf_acc)

            PARTIAL_SUM_OR_BIAS_SELECTION_16_ACC(p_bias_in, bias_boardcast, zero_vector_0, zero_vector_1, acc_zero_flag, C0_acc, C1_acc, C2_acc, C3_acc, C4_acc, C5_acc, C6_acc, C7_acc, C8_acc, C9_acc, Ca_acc, Cb_acc, Cc_acc, Cd_acc, Ce_acc, Cf_acc)                

            MMUL C0(C0_acc);
            MMUL C1(C1_acc);        
            MMUL C2(C2_acc);
            MMUL C3(C3_acc);
            MMUL C4(C4_acc);
            MMUL C5(C5_acc);
            MMUL C6(C6_acc);
            MMUL C7(C7_acc);
            MMUL C8(C8_acc);
            MMUL C9(C9_acc);
            MMUL Ca(Ca_acc);
            MMUL Cb(Cb_acc);
            MMUL Cc(Cc_acc);
            MMUL Cd(Cd_acc);
            MMUL Ce(Ce_acc);
            MMUL Cf(Cf_acc);            

            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(4,) 
            for (int i=0; i<param.inner_count-1; i++){
                LOAD_MAT_A_DATA(p_mat_a, iterator_mat_a, param, X0, X1, X2, X3)

                MAT_A_DATA_w4_TO_w8(X0, X1, X2, X3, X00, X01, X02, X03)         

                LOAD_MAT_B_DATA(p_mat_b, iterator_mat_b, param, Y0, Y1, Y2, Y3)

                #if GEMM_TRANSPOSE_MAT_B_SUPPORT == 1
                TRANSPOSE_MAT_B_DATA(param, Y0, Y1, Y2, Y3)            
                #endif            

                C0.mac(X00,Y0);
                C1.mac(X01,Y0); 
                C2.mac(X02,Y0); 
                C3.mac(X03,Y0);
                C4.mac(X00,Y1);
                C5.mac(X01,Y1);
                C6.mac(X02,Y1);
                C7.mac(X03,Y1);
                C8.mac(X00,Y2); 
                C9.mac(X01,Y2);
                Ca.mac(X02,Y2);
                Cb.mac(X03,Y2);
                Cc.mac(X00,Y3); 
                Cd.mac(X01,Y3); 
                Ce.mac(X02,Y3); 
                Cf.mac(X03,Y3);    
            }                

            STORE_16_ACC_FULL_DATA(p_mat_c, iterator_mat_c, param, C0, C1, C2, C3, C4, C5, C6, C7, C8, C9, Ca, Cb, Cc, Cd, Ce, Cf, p_bias_in)                                  
        }
      if constexpr(act_type == RELU){
        if(param.act == 1){
         relu_post_process(p_c, param.outer_count);
        }
      }
    
    } else {

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(2,) 
        for (int j=0; j<param.outer_count; j++){   	 
            vector<bfloat16,32> X0, X1, X2, X3, Y0, Y1, Y2, Y3, X00, X01, X02, X03;         

            accum<accfloat,16> C0_acc, C1_acc, C2_acc, C3_acc, C4_acc, C5_acc, C6_acc, C7_acc, C8_acc, C9_acc, Ca_acc, Cb_acc, Cc_acc, Cd_acc, Ce_acc, Cf_acc;          

            LOAD_MAT_A_DATA(p_mat_a, iterator_mat_a, param, X0, X1, X2, X3)        

            MAT_A_DATA_w4_TO_w8(X0, X1, X2, X3, X00, X01, X02, X03)

            LOAD_MAT_B_DATA(p_mat_b, iterator_mat_b, param, Y0, Y1, Y2, Y3)

            #if GEMM_TRANSPOSE_MAT_B_SUPPORT == 1
            TRANSPOSE_MAT_B_DATA(param, Y0, Y1, Y2, Y3)
            #endif

            LOAD_16_ACC_DATA(p_init32, iterator_psum_in, param, C0_acc, C1_acc, C2_acc, C3_acc, C4_acc, C5_acc, C6_acc, C7_acc, C8_acc, C9_acc, Ca_acc, Cb_acc, Cc_acc, Cd_acc, Ce_acc, Cf_acc)

            PARTIAL_SUM_OR_BIAS_SELECTION_16_ACC(p_bias_in, bias_boardcast, zero_vector_0, zero_vector_1, acc_zero_flag, C0_acc, C1_acc, C2_acc, C3_acc, C4_acc, C5_acc, C6_acc, C7_acc, C8_acc, C9_acc, Ca_acc, Cb_acc, Cc_acc, Cd_acc, Ce_acc, Cf_acc)                

            MMUL C0(C0_acc);
            MMUL C1(C1_acc);        
            MMUL C2(C2_acc);
            MMUL C3(C3_acc);
            MMUL C4(C4_acc);
            MMUL C5(C5_acc);
            MMUL C6(C6_acc);
            MMUL C7(C7_acc);
            MMUL C8(C8_acc);
            MMUL C9(C9_acc);
            MMUL Ca(Ca_acc);
            MMUL Cb(Cb_acc);
            MMUL Cc(Cc_acc);
            MMUL Cd(Cd_acc);
            MMUL Ce(Ce_acc);
            MMUL Cf(Cf_acc);            

            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(4,) 
            for (int i=0; i<param.inner_count-1; i++){
                LOAD_MAT_A_DATA(p_mat_a, iterator_mat_a, param, X0, X1, X2, X3)

                MAT_A_DATA_w4_TO_w8(X0, X1, X2, X3, X00, X01, X02, X03)         

                LOAD_MAT_B_DATA(p_mat_b, iterator_mat_b, param, Y0, Y1, Y2, Y3)

                #if GEMM_TRANSPOSE_MAT_B_SUPPORT == 1
                TRANSPOSE_MAT_B_DATA(param, Y0, Y1, Y2, Y3)            
                #endif            

                C0.mac(X00,Y0);
                C1.mac(X01,Y0); 
                C2.mac(X02,Y0); 
                C3.mac(X03,Y0);
                C4.mac(X00,Y1);
                C5.mac(X01,Y1);
                C6.mac(X02,Y1);
                C7.mac(X03,Y1);
                C8.mac(X00,Y2); 
                C9.mac(X01,Y2);
                Ca.mac(X02,Y2);
                Cb.mac(X03,Y2);
                Cc.mac(X00,Y3); 
                Cd.mac(X01,Y3); 
                Ce.mac(X02,Y3); 
                Cf.mac(X03,Y3);    
            }                

            STORE_16_ACC_PARTIAL_DATA(p_psum_out, iterator_psum_out, param, C0, C1, C2, C3, C4, C5, C6, C7, C8, C9, Ca, Cb, Cc, Cd, Ce, Cf, p_bias_in)                  
            }    
    }
}


#else //Start of gemm bfloat16/bfp16 AIE2ps / AIE2p kernel


template <out_mode_t out_mode, act_t act_type>
NOINLINE void gemm_generic
(    
    bfloat16 * __restrict p_a,
    bfloat16 * __restrict p_b,
    bfloat16 * __restrict p_c,
    bfloat16 * __restrict p_bias,
    float    * __restrict p_psum,
    gemm_params_t &param
){
    bfloat16 MAT_A_DM_BANK * __restrict p_mat_a    = (bfloat16 MAT_A_DM_BANK * __restrict) p_a;
    bfloat16 MAT_B_DM_BANK * __restrict p_mat_b    = (bfloat16 MAT_B_DM_BANK * __restrict) p_b;
    bfloat16 MAT_C_DM_BANK * __restrict p_mat_c    = (bfloat16 MAT_C_DM_BANK * __restrict) p_c;
    bfloat16 MAT_B_DM_BANK * __restrict p_bias_in  = (bfloat16 MAT_B_DM_BANK * __restrict) p_bias;
    float    PSUM_DM_BANK  * __restrict p_init32   = (float    PSUM_DM_BANK  * __restrict) p_psum;
    float    PSUM_DM_BANK  * __restrict p_psum_out = (float    PSUM_DM_BANK  * __restrict) p_psum; 
        
    using MMUL = mmul<GEMM_DIM_M, GEMM_DIM_K, GEMM_DIM_N, bfloat16, bfloat16>;

    SETUP_ITR3D_COUNTERS(iterator_mat_a)
    SETUP_ITR3D_COUNTERS(iterator_mat_b)
    SETUP_ITR2D_COUNTERS(iterator_mat_c)
    SETUP_ITR2D_COUNTERS(iterator_psum_in)
    SETUP_ITR2D_COUNTERS(iterator_psum_out)
       
    int acc_zero_flag = 1-param.psum_in;
     
    bfloat16 bfloat16_zero   = 0.0;
    bfloat16 bfloat16_one    = 1.0;
    bfloat16 bias_bcst_value =  (1-param.bias_zero_flag) ? bfloat16_one : bfloat16_zero;
    vector<bfloat16,32>bias_broadcast = (vector<bfloat16,32>)broadcast(bias_bcst_value);
    

    //*********************************************//
    //Load Mat A data and adjust the vectorization.//
    //*********************************************//

    //X0, X1, X2, X3 must be of type vector<bfloat16,32>
    //Load data in w4 vectorization and use interleave operation to get w8 vectorization. 
    //
    //tmp0 = h0-7w0-3, tmp1 = h8-15w0-3 
    //tmp2 = h0-7w4-7, tmp3 = h8-15w4-7
    //
    //X0 = h0-3w0-7, X1 = h4-7w0-7, X2 = h8-11w0-7, X3 = h12-15w0-7
    //
    #define LOAD_MAT_A_DATA_4_4x8(p_mat_a, iterator_mat_a, param, X0, X1, X2, X3)\
    {\
        vector<bfloat16,32> tmp0 = load_v<32>(p_mat_a); p_mat_a = byte_incr(p_mat_a, 64);\
        vector<bfloat16,32> tmp1 = load_v<32>(p_mat_a); p_mat_a = byte_incr(p_mat_a, param.incAI);\
        vector<bfloat16,32> tmp2 = load_v<32>(p_mat_a); p_mat_a = byte_incr(p_mat_a, 64);\
        vector<bfloat16,32> tmp3 = load_v<32>(p_mat_a); \
        p_mat_a = INCR_ITR3D_PARAM(p_mat_a, iterator_mat_a, param)\
        \
        std::tie(X0, X1) = interleave_zip(tmp0,tmp2, 4);\
        std::tie(X2, X3) = interleave_zip(tmp1,tmp3, 4);\
    }


    //*********************************************//
    //Load Mat B data and adjust the vectorization.//
    //Apply transpose if required.                 //
    //*********************************************//

    //Y0, Y1 must be of type vector<bfloat16,64>
    //
    // 
    //tmp0 = h0-7w0-3, tmp1 = h0-7w4-7, tmp2 = h0-7w8-11, tmp3 = h0-7w12-15
    //
    #define LOAD_MAT_B_DATA_2_8x8(p_mat_b, iterator_mat_b, param, Y0, Y1)\
    {\
        vector<bfloat16,32> tmp0 = load_v<32>(p_mat_b); p_mat_b=byte_incr(p_mat_b, param.incB1);\
        vector<bfloat16,32> tmp1 = load_v<32>(p_mat_b); p_mat_b=byte_incr(p_mat_b, param.incB2);\
        vector<bfloat16,32> tmp2 = load_v<32>(p_mat_b); p_mat_b=byte_incr(p_mat_b, param.incB1);\
        vector<bfloat16,32> tmp3 = load_v<32>(p_mat_b); \
        p_mat_b = INCR_ITR3D_PARAM(p_mat_b, iterator_mat_b, param)\
        \
        vector<bfloat16,32> tmp4, tmp5, tmp6, tmp7;\
        std::tie(tmp4, tmp5) = interleave_zip(tmp0,tmp1, param.mat_b_Imode_0);\
        std::tie(tmp6, tmp7) = interleave_zip(tmp2,tmp3, param.mat_b_Imode_0);\
        tmp4 = transpose(tmp4, param.mat_b_Imode_1, param.mat_b_Imode_2);\
        tmp5 = transpose(tmp5, param.mat_b_Imode_1, param.mat_b_Imode_2);\
        tmp6 = transpose(tmp6, param.mat_b_Imode_1, param.mat_b_Imode_2);\
        tmp7 = transpose(tmp7, param.mat_b_Imode_1, param.mat_b_Imode_2);\
        Y0.insert(0, tmp4);\
        Y0.insert(1, tmp5);\
        Y1.insert(0, tmp6);\
        Y1.insert(1, tmp7);\
    }

    //********************************************************//
    //Load accumulators with partial sum from the previous    //
    //iteration. For the first iteration this will be ignored.//
    //********************************************************//

    #define LOAD_ACC_4x8_DATA(ptr, buff, incr) \
    {\
        vector<float,32> tmp0 = load_v<32>(ptr);\
        buff.from_vector(tmp0);\
        ptr=byte_incr(ptr, incr+64);\
    }

    #define LOAD_ACC_8_4x8_DATA(p_init32, iterator_psum_in, param, C0_acc, C1_acc, C2_acc, C3_acc, C4_acc, C5_acc, C6_acc, C7_acc)\
    {\
        LOAD_ACC_4x8_DATA(p_init32, C0_acc, 64);\
        LOAD_ACC_4x8_DATA(p_init32, C1_acc, 64);\
        LOAD_ACC_4x8_DATA(p_init32, C2_acc, 64);\
        LOAD_ACC_4x8_DATA(p_init32, C3_acc, 64);\
        LOAD_ACC_4x8_DATA(p_init32, C4_acc, 64);\
        LOAD_ACC_4x8_DATA(p_init32, C5_acc, 64);\
        LOAD_ACC_4x8_DATA(p_init32, C6_acc, 64);\
        LOAD_ACC_4x8_DATA(p_init32, C7_acc, 64);\
    }


    //**************************************************//
    //Partial sum or Bias selection using add_mac_conf()//
    //**************************************************//

    #define ADD_MAC_CONF(vec_0, vec_1, acc_0, acc_1, acc_zero_flag)\
    {\
        MMUL M(op_zero(acc_0, acc_zero_flag));\
        M.mac(vec_0, vec_1);\
        acc_0 = add(M.to_accum(), acc_1);\
    }

    #define PARTIAL_SUM_OR_BIAS_SELECTION_ACC_8_4x8(p_bias_in, bias_broadcast, acc_zero_flag, C0_acc, C1_acc, C2_acc, C3_acc, C4_acc, C5_acc, C6_acc, C7_acc, X0, X1, X2, X3, Y0, Y1)\
    {\
        accum<accfloat,32>  bias_32lanes_0, bias_32lanes_1;\
        vector<bfloat16,64> bias_8lanes_0,  bias_8lanes_1;\
        \
        vector<bfloat16,32> zero_vector = (vector<bfloat16,32>)broadcast(bfloat16_zero);\
        bias_8lanes_0.insert(0, zero_vector);\
        bias_8lanes_0.insert(1, zero_vector);\
        bias_8lanes_1.insert(0, zero_vector);\
        bias_8lanes_1.insert(1, zero_vector);\
        bias_8lanes_0.insert(0, load_v<8>(p_bias_in)); p_bias_in = byte_incr(p_bias_in, (8*2));\
        bias_8lanes_1.insert(0, load_v<8>(p_bias_in)); p_bias_in = byte_incr(p_bias_in, -(8*2));\
        \
        MMUL B0; B0.mul(bias_broadcast, bias_8lanes_0);\
        ADD_MAC_CONF(X0, Y0, C0_acc, B0.to_accum(), acc_zero_flag)\
        ADD_MAC_CONF(X1, Y0, C1_acc, B0.to_accum(), acc_zero_flag)\
        ADD_MAC_CONF(X2, Y0, C2_acc, B0.to_accum(), acc_zero_flag)\
        ADD_MAC_CONF(X3, Y0, C3_acc, B0.to_accum(), acc_zero_flag)\
        \
        MMUL B1; B1.mul(bias_broadcast, bias_8lanes_1);\
        ADD_MAC_CONF(X0, Y1, C4_acc, B1.to_accum(), acc_zero_flag)\
        ADD_MAC_CONF(X1, Y1, C5_acc, B1.to_accum(), acc_zero_flag)\
        ADD_MAC_CONF(X2, Y1, C6_acc, B1.to_accum(), acc_zero_flag)\
        ADD_MAC_CONF(X3, Y1, C7_acc, B1.to_accum(), acc_zero_flag)\
        \
    }


    //***************************************//
    //Store Accumulator data into the memory // 
    //***************************************//

    #define STORE_ACC_4x8_DATA(ptr, buff, incr) \
    {\
        vector<float,32> buff_vec = buff.template to_vector<float>(0);\
        store_v(ptr, buff_vec); ptr = byte_incr(ptr, incr+64);\
    }

    //C0, C1, C2, C3, C4, C5, C6, C7 are of type mmul<4, 8, 8, bfloat16, bfloat16>
    //Please note: Since partial sum here has w8 vectorization and final output is in 
    //w4 vectorization, final output cannot over-write partial sum.
    #define STORE_ACC_8_4x8_PARTIAL_DATA(p_psum_out, iterator_psum_out, param, C0, C1, C2, C3, C4, C5, C6, C7, p_bias_in) \
    {\
        STORE_ACC_4x8_DATA(p_psum_out, C0.to_accum(), 64);\
        STORE_ACC_4x8_DATA(p_psum_out, C1.to_accum(), 64);\
        STORE_ACC_4x8_DATA(p_psum_out, C2.to_accum(), 64);\
        STORE_ACC_4x8_DATA(p_psum_out, C3.to_accum(), 64);\
        STORE_ACC_4x8_DATA(p_psum_out, C4.to_accum(), 64);\
        STORE_ACC_4x8_DATA(p_psum_out, C5.to_accum(), 64);\
        STORE_ACC_4x8_DATA(p_psum_out, C6.to_accum(), 64);\
        STORE_ACC_4x8_DATA(p_psum_out, C7.to_accum(), 64);\
        p_psum_out = INCR_ITR2D_PARAM(p_psum_out, iterator_psum_out, param)\
        \
        p_bias_in = byte_incr(p_bias_in, (16*2)*(iterator_psum_out_cnt0 == 0));\
    }


    //***************************************************************************//
    //Convert float32 accumulator data to bfloat16 data and store in WHw4 format //
    //***************************************************************************//

    //C0, C1, C2, C3, C4, C5, C6, C7 are of type mmul<4, 8, 8, bfloat16, bfloat16>
    #define STORE_ACC_8_4x8_FULL_DATA(p_mat_c, iterator_mat_c, param, C0, C1, C2, C3, C4, C5, C6, C7, p_bias_in) \
    {\
        vector<bfloat16,32> C0_vec = C0.template to_vector<bfloat16>(16);\
        vector<bfloat16,32> C1_vec = C1.template to_vector<bfloat16>(16);\
        vector<bfloat16,32> C2_vec = C2.template to_vector<bfloat16>(16);\
        vector<bfloat16,32> C3_vec = C3.template to_vector<bfloat16>(16);\
        vector<bfloat16,32> C4_vec = C4.template to_vector<bfloat16>(16);\
        vector<bfloat16,32> C5_vec = C5.template to_vector<bfloat16>(16);\
        vector<bfloat16,32> C6_vec = C6.template to_vector<bfloat16>(16);\
        vector<bfloat16,32> C7_vec = C7.template to_vector<bfloat16>(16);\
        \
        std::tie(C0_vec, C1_vec) = interleave_unzip(C0_vec, C1_vec, 4);\
        std::tie(C2_vec, C3_vec) = interleave_unzip(C2_vec, C3_vec, 4);\
        std::tie(C4_vec, C5_vec) = interleave_unzip(C4_vec, C5_vec, 4);\
        std::tie(C6_vec, C7_vec) = interleave_unzip(C6_vec, C7_vec, 4);\
        \
        store_v(p_mat_c, C0_vec);  p_mat_c = byte_incr(p_mat_c, 64);\
        store_v(p_mat_c, C2_vec);  p_mat_c = byte_incr(p_mat_c, param.incC1 + 32);\
        store_v(p_mat_c, C1_vec);  p_mat_c = byte_incr(p_mat_c, 64);\
        store_v(p_mat_c, C3_vec);  p_mat_c = byte_incr(p_mat_c, param.incC1 + 32);\
        store_v(p_mat_c, C4_vec);  p_mat_c = byte_incr(p_mat_c, 64);\
        store_v(p_mat_c, C6_vec);  p_mat_c = byte_incr(p_mat_c, param.incC1 + 32);\
        store_v(p_mat_c, C5_vec);  p_mat_c = byte_incr(p_mat_c, 64);\
        store_v(p_mat_c, C7_vec);  p_mat_c = byte_incr(p_mat_c, 32);\
        p_mat_c = INCR_ITR2D_PARAM(p_mat_c, iterator_mat_c, param)\
        \
        p_bias_in = byte_incr(p_bias_in, (16*2)*(iterator_mat_c_cnt0 == 0));\
    }


    //*************************//
    //Inner loop mac operations//
    //*************************//

    //C0, C1, C2, C3, C4, C5, C6, C7 are of type mmul<4, 8, 8, bfloat16, bfloat16>
    //X0, X1, X2, X3 must be of type vector<bfloat16,32>
    //Y0, Y1 must be of type vector<bfloat16,64> 
    #define MMUL_MAC_8_4x8x8(C0, C1, C2, C3, C4, C5, C6, C7, X0, X1, X2, X3, Y0, Y1)\
    {\
        C0.mac(X0,Y0); C1.mac(X1,Y0); C2.mac(X2,Y0); C3.mac(X3,Y0);\
        C4.mac(X0,Y1); C5.mac(X1,Y1); C6.mac(X2,Y1); C7.mac(X3,Y1);\
    }       
    
    if constexpr(out_mode == FULL){
        
        for (int j=0; j<param.outer_count; j++)
         {   	 
            vector<bfloat16,32> X0, X1, X2, X3;
            vector<bfloat16,64> Y0, Y1;
            
            accum<accfloat, 32> C0_acc, C1_acc, C2_acc, C3_acc, C4_acc, C5_acc, C6_acc, C7_acc;       

            LOAD_MAT_A_DATA_4_4x8(p_mat_a, iterator_mat_a, param, X0, X1, X2, X3)            
            LOAD_MAT_B_DATA_2_8x8(p_mat_b, iterator_mat_b, param, Y0, Y1)            
            LOAD_ACC_8_4x8_DATA(p_init32, iterator_psum_in, param, C0_acc, C1_acc, C2_acc, C3_acc, C4_acc, C5_acc, C6_acc, C7_acc)            
            #ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
            set_rnd(rnd_sym_inf);
            #endif
            PARTIAL_SUM_OR_BIAS_SELECTION_ACC_8_4x8(p_bias_in, bias_broadcast, acc_zero_flag, C0_acc, C1_acc, C2_acc, C3_acc, C4_acc, C5_acc, C6_acc, C7_acc, X0, X1, X2, X3, Y0, Y1)
            #ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
            set_rnd(rnd_conv_even);
            #endif              
            MMUL C0(C0_acc), C1(C1_acc), C2(C2_acc), C3(C3_acc), C4(C4_acc), C5(C5_acc), C6(C6_acc), C7(C7_acc);

            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(4,) 
            for (int i=0; i<param.inner_count-1; i++)
            {
                LOAD_MAT_A_DATA_4_4x8(p_mat_a, iterator_mat_a, param, X0, X1, X2, X3)  
                LOAD_MAT_B_DATA_2_8x8(p_mat_b, iterator_mat_b, param, Y0, Y1) 
                #ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
                set_rnd(rnd_sym_inf);
                #endif                          
                MMUL_MAC_8_4x8x8(C0, C1, C2, C3, C4, C5, C6, C7, X0, X1, X2, X3, Y0, Y1)
                #ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
                set_rnd(rnd_conv_even);
                #endif                                    
            }   
            
            STORE_ACC_8_4x8_FULL_DATA(p_mat_c, iterator_mat_c, param, C0, C1, C2, C3, C4, C5, C6, C7, p_bias_in)
        }
       if constexpr(act_type == RELU){
         if(param.act == 1){
           relu_post_process(p_c, param.outer_count);
         }
       }
    
    } else {

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(2,) 
        for (int j=0; j<param.outer_count; j++){   	 
            vector<bfloat16,32> X0, X1, X2, X3;
            vector<bfloat16,64> Y0, Y1;         

            accum<accfloat, 32> C0_acc, C1_acc, C2_acc, C3_acc, C4_acc, C5_acc, C6_acc, C7_acc;          

            LOAD_MAT_A_DATA_4_4x8(p_mat_a, iterator_mat_a, param, X0, X1, X2, X3)
            LOAD_MAT_B_DATA_2_8x8(p_mat_b, iterator_mat_b, param, Y0, Y1)            
            LOAD_ACC_8_4x8_DATA(p_init32, iterator_psum_in, param, C0_acc, C1_acc, C2_acc, C3_acc, C4_acc, C5_acc, C6_acc, C7_acc)
            #ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
            set_rnd(rnd_sym_inf);
            #endif               
            PARTIAL_SUM_OR_BIAS_SELECTION_ACC_8_4x8(p_bias_in, bias_broadcast, acc_zero_flag, C0_acc, C1_acc, C2_acc, C3_acc, C4_acc, C5_acc, C6_acc, C7_acc, X0, X1, X2, X3, Y0, Y1)
            #ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
            set_rnd(rnd_conv_even);
            #endif
            MMUL C0(C0_acc), C1(C1_acc), C2(C2_acc), C3(C3_acc), C4(C4_acc), C5(C5_acc), C6(C6_acc), C7(C7_acc);            
            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(4,) 
            for (int i=0; i<param.inner_count-1; i++){
                LOAD_MAT_A_DATA_4_4x8(p_mat_a, iterator_mat_a, param, X0, X1, X2, X3)
                LOAD_MAT_B_DATA_2_8x8(p_mat_b, iterator_mat_b, param, Y0, Y1)
                #ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
                set_rnd(rnd_sym_inf);
                #endif                
                MMUL_MAC_8_4x8x8(C0, C1, C2, C3, C4, C5, C6, C7, X0, X1, X2, X3, Y0, Y1)
                #ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
                set_rnd(rnd_conv_even);
                #endif
            }                

            STORE_ACC_8_4x8_PARTIAL_DATA(p_psum_out, iterator_psum_out, param, C0, C1, C2, C3, C4, C5, C6, C7, p_bias_in)                  
            }    
    }
}
#endif

//***************************GEMM INT8 version************************
// data layout - WHw8
template <out_mode_t out_mode, typename dtype>
NOINLINE void gemm_generic
(    
    dtype * __restrict p_a,
    dtype * __restrict p_b,
    dtype * __restrict p_c,
    int * __restrict p_bias,
    int * __restrict p_psum,
    gemm_params_t &param
){
    dtype MAT_A_DM_BANK * __restrict p_mat_a = (dtype MAT_A_DM_BANK * __restrict) p_a;
    dtype MAT_B_DM_BANK * __restrict p_mat_b = (dtype MAT_B_DM_BANK * __restrict) p_b;
    dtype MAT_C_DM_BANK * __restrict p_mat_c = (dtype MAT_C_DM_BANK * __restrict) p_c;
    int16 MAT_C_DM_BANK * __restrict p_bias_in = (int16 MAT_C_DM_BANK * __restrict) p_bias;
    int16 PSUM_DM_BANK  * __restrict p_psum_in = (int16 PSUM_DM_BANK  * __restrict) p_psum;
    int16 PSUM_DM_BANK  * __restrict p_psum_out = (int16 PSUM_DM_BANK  * __restrict) p_psum;
        
    using MMUL = mmul<GEMM_INT_DIM_M, GEMM_INT_DIM_K, GEMM_INT_DIM_N, dtype, dtype>;
    using MMUL_BIAS = mmul<GEMM_BIAS_DIM_M, GEMM_BIAS_DIM_K, GEMM_BIAS_DIM_N, int16, int16>;
        
    #define LOAD_MAT_A_DATA_GEMM_INT8(p_mat_a, iterator_mat_a, param, X0, X1)\
    {\
        X0.insert(0, load_v<64>(p_mat_a));p_mat_a=byte_incr(p_mat_a, 64);\
        X1.insert(0, load_v<64>(p_mat_a));p_mat_a=byte_incr(p_mat_a, 0);\
        p_mat_a = INCR_ITR3D_PARAM(p_mat_a, iterator_mat_a, param)\
    }
    
    #define LOAD_MAT_B_DATA_GEMM_INT8(p_mat_b, iterator_mat_b, param, Y0, Y1)\
    {\
        Y0.insert(0, load_v<64>(p_mat_b));p_mat_b=byte_incr(p_mat_b, param.incB1);\
        Y1.insert(0, load_v<64>(p_mat_b));p_mat_b=byte_incr(p_mat_b, 0);\
        p_mat_b = INCR_ITR3D_PARAM(p_mat_b, iterator_mat_b, param)\
        Y0 = transpose(Y0,param.mat_b_Imode_0,param.mat_b_Imode_0);\
        Y1 = transpose(Y1,param.mat_b_Imode_0,param.mat_b_Imode_0);\
    }
    
    #define STORE_ACC_FULL_DATA_GEMM_INT8(p_mat_c, iterator_mat_c, iterator_psum_out, p_bias_in, param, C0, C1, C2, C3)\
    {\
        store_v(p_mat_c, C0.template to_vector_sign<dtype>(param.out_sign,param.shift_out));  p_mat_c = byte_incr(p_mat_c, 64);\
        store_v(p_mat_c, C1.template to_vector_sign<dtype>(param.out_sign,param.shift_out));  p_mat_c = byte_incr(p_mat_c, param.incC1);\
        store_v(p_mat_c, C2.template to_vector_sign<dtype>(param.out_sign,param.shift_out));  p_mat_c = byte_incr(p_mat_c, 64);\
        store_v(p_mat_c, C3.template to_vector_sign<dtype>(param.out_sign,param.shift_out));  p_mat_c = INCR_ITR2D_PARAM(p_mat_c, iterator_mat_c, param)\
        p_bias_in = INCR_ITR2D_PARAM(p_bias_in, iterator_psum_out, param)\
    }
    
    #define LOAD_DATA_GEMM_INT8(ptr, buff, incr) \
    {\
        vector<int16,64> tmp0 = load_v<64>(ptr);\
        buff.from_vector(tmp0); ptr=byte_incr(ptr, incr);\
    }
    
    #define LOAD_ACC_DATA_GEMM_INT8(p_psum_in, param, C0_acc, C1_acc, C2_acc, C3_acc)\
    {\
        LOAD_DATA_GEMM_INT8(p_psum_in, C0_acc, 128)\
        LOAD_DATA_GEMM_INT8(p_psum_in, C1_acc, 128)\
        LOAD_DATA_GEMM_INT8(p_psum_in, C2_acc, 128)\
        LOAD_DATA_GEMM_INT8(p_psum_in, C3_acc, 128)\
    }
    
    #define STORE_ACC_PARTIAL_DATA_GEMM_INT8(p_psum_out, iterator_psum_out, p_bias_in, param, C0, C1, C2, C3) \
    {\
        store_v(p_psum_out, C0.template to_vector<int16>(0));  p_psum_out = byte_incr(p_psum_out, 128);\
        store_v(p_psum_out, C1.template to_vector<int16>(0));  p_psum_out = byte_incr(p_psum_out, 128);\
        store_v(p_psum_out, C2.template to_vector<int16>(0));  p_psum_out = byte_incr(p_psum_out, 128);\
        store_v(p_psum_out, C3.template to_vector<int16>(0));  p_psum_out = byte_incr(p_psum_out, 128);\
        p_bias_in = INCR_ITR2D_PARAM(p_bias_in, iterator_psum_out, param)\
    }
    
    SETUP_ITR3D_COUNTERS(iterator_mat_a);
    SETUP_ITR3D_COUNTERS(iterator_mat_b);
    SETUP_ITR2D_COUNTERS(iterator_mat_c);
    SETUP_ITR2D_COUNTERS(iterator_psum_out);
    
    vector<int16_t,16> B0, B1;
    B0 = zeros<int16_t,16>();
    B1 = zeros<int16_t,16>();
    
    int16_t factor = param.bias_zero_flag ? 0 : 1 << param.bias_init_shift;
    #if __AIE_ARCH__ == 20
    vector<int16_t,8>zero_vector = broadcast<int16_t,8>((int16_t)(factor));
    #else
    vector<int16_t,16>zero_vector = broadcast<int16_t,16>((int16_t)(factor));
    #endif
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for(int j=0; j<param.outer_count; j++){
        vector<dtype,64> X0, X1, Y0, Y1;
        accum<acc32,64> C0_acc, C1_acc, C2_acc, C3_acc, bias_acc;
        
        B0.insert(0,load_v<8>(p_bias_in)); p_bias_in=byte_incr(p_bias_in,16);
        B1.insert(0,load_v<8>(p_bias_in)); p_bias_in=byte_incr(p_bias_in,-16);
        
        LOAD_MAT_A_DATA_GEMM_INT8(p_mat_a, iterator_mat_a, param, X0, X1)
        LOAD_MAT_B_DATA_GEMM_INT8(p_mat_b, iterator_mat_b, param, Y0, Y1)
        LOAD_ACC_DATA_GEMM_INT8(p_psum_in, param, C0_acc, C1_acc, C2_acc, C3_acc)
        
        MMUL C0(op_zero(C0_acc, 1-param.psum_in));
        MMUL C1(op_zero(C1_acc, 1-param.psum_in));
        MMUL C2(op_zero(C2_acc, 1-param.psum_in));
        MMUL C3(op_zero(C3_acc, 1-param.psum_in));
        
        C0.mac(op_sign(X0, param.ifm_sign), op_sign(Y0, param.ifm_sign));
        C1.mac(op_sign(X1, param.ifm_sign), op_sign(Y0, param.ifm_sign));
        C2.mac(op_sign(X0, param.ifm_sign), op_sign(Y1, param.ifm_sign));
        C3.mac(op_sign(X1, param.ifm_sign), op_sign(Y1, param.ifm_sign));
        
        MMUL_BIAS B_add_0;        
        B_add_0.mul(zero_vector, B0);
        
        #if __AIE_ARCH__ == 20
        bias_acc = concat(B_add_0.to_accum(), B_add_0.to_accum());
        #else
        bias_acc = B_add_0.to_accum();
        #endif               
        C0_acc = add(C0.to_accum(), bias_acc);
        C1_acc = add(C1.to_accum(), bias_acc);
        
        B_add_0.mul(zero_vector, B1);
        #if __AIE_ARCH__ == 20
        bias_acc = concat(B_add_0.to_accum(), B_add_0.to_accum());
        #else
        bias_acc = B_add_0.to_accum();
        #endif
        C2_acc = add(C2.to_accum(), bias_acc);
        C3_acc = add(C3.to_accum(), bias_acc);
        
        MMUL Ci0(C0_acc);
        MMUL Ci1(C1_acc);
        MMUL Ci2(C2_acc);
        MMUL Ci3(C3_acc);
        
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for(int i=0; i<param.inner_count-1; i++){
            LOAD_MAT_A_DATA_GEMM_INT8(p_mat_a, iterator_mat_a, param, X0, X1)
            LOAD_MAT_B_DATA_GEMM_INT8(p_mat_b, iterator_mat_b, param, Y0, Y1)
                    
            Ci0.mac(op_sign(X0, param.ifm_sign), op_sign(Y0, param.ifm_sign));
            Ci1.mac(op_sign(X1, param.ifm_sign), op_sign(Y0, param.ifm_sign));
            Ci2.mac(op_sign(X0, param.ifm_sign), op_sign(Y1, param.ifm_sign));
            Ci3.mac(op_sign(X1, param.ifm_sign), op_sign(Y1, param.ifm_sign));
        } 
        if constexpr(out_mode == FULL){
            STORE_ACC_FULL_DATA_GEMM_INT8(p_mat_c, iterator_mat_c, iterator_psum_out, p_bias_in, param, Ci0, Ci1, Ci2, Ci3)
        }else{
            STORE_ACC_PARTIAL_DATA_GEMM_INT8(p_psum_out, iterator_psum_out, p_bias_in, param, Ci0, Ci1, Ci2, Ci3)
        }
    }
}


template <typename dtype, typename config_ifm, typename config_wts, typename config_ofm, act_t act_type=LINEAR>
NOINLINE void gemm
(    
    adf::input_buffer_conf <dtype, config_ifm>       & __restrict mat_a,
    adf::input_buffer_conf <dtype, config_wts>       & __restrict mat_b,
    adf::output_buffer_conf<dtype, config_ofm>       &            mat_c,
    gemm_params_t &param
) {

    // Output modes
    const out_mode_t out_mode_full     = FULL ;
    const out_mode_t out_mode_partial  = PARTIAL ;

    // Setup iteration level parameters like IO mode etc
    setup_gemm_iter_params(param) ;
    
    // Extract low level pointers used by the actual kernel code
    
    if constexpr(std::is_same<bfloat16, dtype>::value){
        bfloat16 * input_a      = (bfloat16 *) mat_a.data();
        bfloat16 * input_b      = (bfloat16 *) mat_b.data() + ((param.bias_len + GEMM_LAYER_PARAM_BYTES)>>1);
        bfloat16 * output_c     = (bfloat16 *) mat_c.data();
        bfloat16 * input_bias   = (bfloat16 *) mat_b.data() + (GEMM_LAYER_PARAM_BYTES >> 1); 
        float    * input_psum   = (float    *) mat_c.data() + (param.psum_buff_offset >> 2);
        // Call the kernel with appropriate mode bfloat16 (final store) or float32 (partial store)
        if(param.io_mode==0 || param.io_mode==2) {
            gemm_generic<out_mode_full,act_type>(input_a, input_b, output_c, input_bias, input_psum, param);
        } else {
            gemm_generic<out_mode_partial,act_type>(input_a, input_b, output_c, input_bias, input_psum, param);
        }
    }else{
        dtype * input_a = (dtype *) mat_a.data();
        dtype * input_b = (dtype *) mat_b.data() + param.bias_len;
        dtype * output_c = (dtype *) mat_c.data();
        int * input_bias = (int *) mat_b.data();
        int * psum = (int *) mat_c.data() + (param.psum_buff_offset >> 2);
        // Call the kernel with appropriate mode int8 (final store) or int16 (partial store)
        if(param.io_mode == 0 || param.io_mode == 2) {
            gemm_generic<out_mode_full, dtype>(input_a, input_b, output_c, input_bias, psum, param);
        }else{
            gemm_generic<out_mode_partial, dtype>(input_a, input_b, output_c, input_bias, psum, param);
        }
    }
}

#endif //__GEMM_H__    
