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


#ifndef __GEMM_4x4_H__
#define __GEMM_4x4_H__

#include "aie.hpp"
#include <adf.h>
#include <stdio.h>
#include "utils.h"
#include "gemm_4x4_params.h"
#include "mllib_const.h"
using namespace aie;


template <out_mode_t out_mode, act_t act_type>
NOINLINE void gemm_4x4_generic
(    
    bfloat16 * __restrict p_a,
    bfloat16 * __restrict p_b,
    bfloat16 * __restrict p_c,
    bfloat16 * __restrict p_bias,
    float    * __restrict p_psum,
    gemm_4x4_params_t &param
){
    bfloat16 GEMM_4x4_MAT_A_DM_BANK * __restrict p_mat_a    = (bfloat16 GEMM_4x4_MAT_A_DM_BANK * __restrict) p_a;
    bfloat16 GEMM_4x4_MAT_B_DM_BANK * __restrict p_mat_b    = (bfloat16 GEMM_4x4_MAT_B_DM_BANK * __restrict) p_b;
    bfloat16 GEMM_4x4_MAT_C_DM_BANK * __restrict p_mat_c    = (bfloat16 GEMM_4x4_MAT_C_DM_BANK * __restrict) p_c;
    bfloat16 GEMM_4x4_MAT_B_DM_BANK * __restrict p_bias_in  = (bfloat16 GEMM_4x4_MAT_B_DM_BANK * __restrict) p_bias;
    float    GEMM_4x4_PSUM_DM_BANK  * __restrict p_init32   = (float    GEMM_4x4_PSUM_DM_BANK  * __restrict) p_psum;
    float    GEMM_4x4_PSUM_DM_BANK  * __restrict p_psum_out = (float    GEMM_4x4_PSUM_DM_BANK  * __restrict) p_psum; 

    using MMUL = mmul<GEMM_4x4_DIM_M, GEMM_4x4_DIM_K, GEMM_4x4_DIM_N, bfloat16, bfloat16>;   

    #define ADD_MAC_CONF(vec_0, vec_1, acc_0, acc_1, acc_zero_flag)\
    {\
        MMUL M(op_zero(acc_0, acc_zero_flag));\
        M.mac(vec_0, vec_1);\
        acc_0 = add(M.to_accum(), acc_1);\
    }

    SETUP_ITR3D_COUNTERS(iterator_mat_a)
    SETUP_ITR3D_COUNTERS(iterator_mat_b)
    SETUP_ITR2D_COUNTERS(iterator_mat_c)
    SETUP_ITR2D_COUNTERS(iterator_bias)

    int acc_zero_flag = 1-param.psum_in;

    bfloat16 bfloat16_zero   = 0.0;
    bfloat16 bfloat16_one    = 1.0;
    bfloat16 bias_bcst_value =  (1-param.bias_zero_flag) ? bfloat16_one : bfloat16_zero;
    vector<bfloat16,32>bias_broadcast = (vector<bfloat16,32>)broadcast(bias_bcst_value);
    uint32 bias_counter      = 0; 
    
    vector<bfloat16,32>zero_vector_0  = (vector<bfloat16,32>)broadcast(bfloat16_zero);
    vector<bfloat16,32>zero_vector_1  = (vector<bfloat16,32>)broadcast(bfloat16_zero);
    

    for (int j=0; j<param.outer_count; j++)
    {   	 
        vector<bfloat16,32> mat_a_vec_0, mat_a_vec_1, mat_a_vec_2, mat_b_vec_0;         
        accum<accfloat,16>  C0_acc;          

        //Load h0-3w0-3 and h0-3w4-7
        mat_a_vec_0.insert(0, load_v<16>(p_mat_a)); p_mat_a = byte_incr(p_mat_a, param.incA1);
        mat_a_vec_1.insert(0, load_v<16>(p_mat_a)); p_mat_a = INCR_ITR3D_PARAM(p_mat_a, iterator_mat_a, param)

        //interleave(h0-3w0-3, h0-3w4-7) = h0-3w0-7. This required for 4x8 8x4 = 4x4 multiplication mode.
        mat_a_vec_2 = interleave(mat_a_vec_0, mat_a_vec_1, 14);

        //Load h0-7w0-3
        mat_b_vec_0.insert(0, load_v<16>(p_mat_b)); p_mat_b = byte_incr(p_mat_b, param.incB1);
        mat_b_vec_0.insert(1, load_v<16>(p_mat_b)); p_mat_b = INCR_ITR3D_PARAM(p_mat_b, iterator_mat_b, param)
        
        //Load the partial sum from previous iteration. For first iteration, this will be junk data.
        C0_acc = insert(C0_acc, 0, *(v8accfloat GEMM_4x4_PSUM_DM_BANK *)p_init32); p_init32=byte_incr(p_init32, 32);
        C0_acc = insert(C0_acc, 1, *(v8accfloat GEMM_4x4_PSUM_DM_BANK *)p_init32); p_init32=byte_incr(p_init32, 32);
        
        //Load and broadcast bias
        zero_vector_0.insert(0, load_v<8>(p_bias_in));
        uint32 bias_flag = (bias_counter & (uint32)0x00000001);
        vector<bfloat16,32>bias_4lanes_0 = interleave(zero_vector_0, zero_vector_1, 6 + bias_flag);
        MMUL B0; B0.mul(bias_broadcast, bias_4lanes_0);
        
        //Bias or Partial sum selection
        ADD_MAC_CONF(mat_a_vec_2, mat_b_vec_0, C0_acc, B0.to_accum(), acc_zero_flag)

        //Intialise the HL-API.
        MMUL C0(C0_acc);      

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for (int i=0; i<param.inner_count-1; i++){
            //Load h0-3w0-3 and h0-3w4-7
            mat_a_vec_0.insert(0, load_v<16>(p_mat_a)); p_mat_a = byte_incr(p_mat_a, param.incA1);
            mat_a_vec_1.insert(0, load_v<16>(p_mat_a)); p_mat_a = INCR_ITR3D_PARAM(p_mat_a, iterator_mat_a, param)
            //interleave(h0-3w0-3, h0-3w4-7) = h0-3w0-7
            mat_a_vec_2 = interleave(mat_a_vec_0, mat_a_vec_1, 14);

            //Load h0-7w0-3
            mat_b_vec_0.insert(0, load_v<16>(p_mat_b)); p_mat_b=byte_incr(p_mat_b, param.incB1);
            mat_b_vec_0.insert(1, load_v<16>(p_mat_b)); p_mat_b = INCR_ITR3D_PARAM(p_mat_b, iterator_mat_b, param)          

            C0.mac(mat_a_vec_2, mat_b_vec_0);
        } //End of inner-loop               

        if constexpr(out_mode == FULL){
            
            //Store final result after the last inner dimension iteration
            store_v(p_mat_c, C0.template to_vector<bfloat16>(16)); p_mat_c = byte_incr(p_mat_c, 32);
        
        } else {        
            
            //Store the partial sums till the last inner dimension iteration
            *(v8accfloat GEMM_4x4_PSUM_DM_BANK *)p_psum_out = C0.to_accum().extract<8>(0);  p_psum_out = byte_incr(p_psum_out, 32);
            *(v8accfloat GEMM_4x4_PSUM_DM_BANK *)p_psum_out = C0.to_accum().extract<8>(1);  p_psum_out = byte_incr(p_psum_out, 32);
            
        }
        
        p_bias_in = INCR_ITR2D_PARAM(p_bias_in, iterator_bias, param)
        
        //Bias counter is used to select lower or upper 8 bytes of 16 byte bias load.
        //Lower 8 bytes when bias counter is even and upper 8 bytes when bias counter is odd.        
        bias_counter = bias_counter + (iterator_bias_cnt0 == 0);                                          
    
    } //End of outer-loop

}



template <typename dtype, typename config_ifm, typename config_wts, typename config_ofm, act_t act_type=LINEAR>
NOINLINE void gemm_4x4
(    
    adf::input_buffer_conf <dtype, config_ifm>       & __restrict mat_a,
    adf::input_buffer_conf <dtype, config_wts>       & __restrict mat_b,
    adf::output_buffer_conf<dtype, config_ofm>       &            mat_c,
    gemm_4x4_params_t &param
) {

    // Output modes
    const out_mode_t out_mode_full     = FULL ;
    const out_mode_t out_mode_partial  = PARTIAL ;

    // Setup iteration level parameters like IO mode etc
    setup_gemm_4x4_iter_params(param) ;

    // Extract low level pointers used by the actual kernel code
    bfloat16 * input_a      = (bfloat16 *) mat_a.data();
    bfloat16 * input_b      = (bfloat16 *) mat_b.data() + ((param.bias_len + GEMM_4x4_LAYER_PARAM_BYTES)>>1);
    bfloat16 * output_c     = (bfloat16 *) mat_c.data();
    bfloat16 * input_bias   = (bfloat16 *) mat_b.data() + (GEMM_4x4_LAYER_PARAM_BYTES >> 1); 
    float    * input_psum   = (float    *) mat_c.data() + (param.psum_buff_offset >> 2);   

    // Call the kernel with appropriate mode bfloat16 (final store) or float32 (partial store)
    if(param.io_mode==0 || param.io_mode==2) {
         gemm_4x4_generic<out_mode_full,act_type>(input_a, input_b, output_c, input_bias, input_psum, param);
     } else {
         gemm_4x4_generic<out_mode_partial,act_type>(input_a, input_b, output_c, input_bias, input_psum, param);
    }
}

#endif //__GEMM_4x4_H__    

