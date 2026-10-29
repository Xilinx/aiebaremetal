/*
    Copyright (C) 2019 - 2022 Xilinx, Inc. All rights reserved.
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

/* 
 * 
 * 2D Convolution Kernel for AI Engine
 *
 * Performs 2d convolution on data present in "ifms" using weights from "wts"
 * Result is stored in "ofms".
 *
 */

#ifndef __CONV2D_H__
#define __CONV2D_H__

#include <adf.h>
#include "aie.hpp"

#include "utils.h"         // Contains some utility functions and definitions related to intrinsics usage
#include "mllib_const.h"   // Constants shared across graphs like activation type
#include "conv2d_params.h" // Parameters like increments and wraps needed by kernel
#include "adf/io_buffer/io_buffer_access.h" // temporary utility from tools to access ping pong buffers, to be removed

using namespace aie;

//shift_input: Intial shift applied to convert accum<acc32,32> to vec<int16,16>
//alpha: This is the scaled value to be multiplied     
//shift_alpha: shift required to remove the scaling factor of alpha
//shift_out: shift applied before saving to final output
//output_ptr: pointer to the location where data is to be saved    
//input: mmul type Cbuff
#if __AIE_ARCH__ == 20
#define CONV2D_INT8_LEAKY_RELU_COMPUTE(input,shift_input,alpha,shift_alpha,shift_out,output_ptr)            \
    {                                                                                               \
        vector<int16,32> input_vec   = input.template to_vector<int16>(shift_input);                \
        accum<acc32,32> acc         = mul<acc32,vector<int16,32>,int16>(input_vec,alpha);           \
        vector<int16,32> alpha_vec  = acc.to_vector<int16>(shift_alpha);                            \
        vector<int16,32> result_vec = max<vector<int16,32>,vector<int16,32>>(input_vec,alpha_vec);  \
        acc.from_vector(result_vec,0);                                                              \
        store_v(output_ptr,acc.to_vector<int8>(shift_out));                                         \
    }
#else
#define CONV2D_INT8_LEAKY_RELU_COMPUTE(input,shift_input,alpha,shift_alpha,shift_out,output_ptr)            \
{                                                                                               \
    vector<int16,64> input_vec   = input.template to_vector<int16>(shift_input);                \
    accum<acc32,64> acc         = mul<acc32,vector<int16,64>,int16>(input_vec,alpha);           \
    vector<int16,64> alpha_vec  = acc.to_vector<int16>(shift_alpha);                            \
    vector<int16,64> result_vec = max<vector<int16,64>,vector<int16,64>>(input_vec,alpha_vec);  \
    acc.from_vector(result_vec,0);                                                              \
    store_v(output_ptr,acc.to_vector<int8>(shift_out));                                         \
}
#endif


#if __AIE_ARCH__ == 20
//TODO Avoid complete full kernel duplication 
#ifdef ENABLE_STRIDE_4 //Start of stride-4 supported conv2d generic
template <uint8_t batch_size, act_t act_type, out_mode_t out_mode, typename dtype_ifm, typename dtype_wts, typename dtype_ofm> 
INLINE void conv2d_generic
(
        int * __restrict input,
        int * __restrict weights,
        int * __restrict acc_in,
        int * __restrict output,
        conv2d_params_t            &conv2d_params
) {

    int16      OFM_DM_BANK  * p_sum   = ((int16      OFM_DM_BANK *) output + (conv2d_params.psum_buff_offset>>1));
    dtype_ifm IFM_DM_BANK  * p_in    = (dtype_ifm  IFM_DM_BANK *) input; 
    dtype_wts  WT_DM_BANK   * p_w     = (dtype_wts   WT_DM_BANK  *) weights;



    vector<dtype_ifm, 32> vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3;
    int16      OFM_DM_BANK * p_init16 = (int16 OFM_DM_BANK *) acc_in;
    //dtype_ofm OFM_DM_BANK * p_out    = (conv2d_params.psum_out==0)?(dtype_ofm OFM_DM_BANK *)output : (dtype_ofm OFM_DM_BANK *)p_sum;
    dtype_ofm OFM_DM_BANK * p_out    = (conv2d_params.psum_out==0)?((dtype_ofm OFM_DM_BANK *)output+conv2d_params.ofm_offset_top+conv2d_params.ofm_offset_left*8*batch_size) : (dtype_ofm OFM_DM_BANK *)p_sum;



    using mmul = mmul<DIM_M, DIM_N, DIM_K, dtype_ifm, dtype_wts>;

    vector<dtype_wts, 64>  Ybuff0, Ybuff1;

    #define LOAD_WEIGHTS(wts,wbuff,wts_incr) \
    {\
        wbuff.insert(0, load_v<32>(wts));wts+=32;\
        wbuff.insert(1, load_v<32>(wts));wts+=wts_incr;\
    }  
    

    Vecs4_ui8 Xbuff;
    //undef(Xbuff);

    // Following counters are needed for ADD_2D/3D_BYTE intrinsics
    // SETUP_ITR2D_COUNTERS(iterator_name) / SETUP_ITR3D_COUNTERS(iterator_name)
    // type 1: 2D iterator, 2: 3d Iterator
    // e.g. SETUP_ITR3D_COUNTERS(iterator_inner)
    // Resulting count variable is : iterator_inner_cnt0, iterator_inner_cnt_1
    SETUP_ITR3D_COUNTERS(iterator_inner)
    SETUP_ITR3D_COUNTERS(iterator_outer)
    SETUP_ITR3D_COUNTERS(iterator_weights)
    SETUP_ITR3D_COUNTERS(iterator_psum)
    SETUP_ITR2D_COUNTERS(iterator_pout)

    unsigned frac;

    if constexpr(batch_size < 4) {
        p_in += 31 ;
    }

    int    inc_ST_0       = (conv2d_params.psum_in ==  0) ? 0                       : conv2d_params.inc_ST_0;
    int    inc_ST_1       = (conv2d_params.psum_in ==  0) ? 0                       : conv2d_params.inc_ST_1;
    int    inc_ST_2       = (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_1 : conv2d_params.inc_ST_2;
    int    inc_ST_3       = (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_2 : 0;    
    
    DECL_ITR3D(iterator_psum)
    SETUP_ITR3D(iterator_psum,
                (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_d0 : 0,
                (conv2d_params.psum_in ==  0) ? conv2d_params.num_X : 0,
                (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_d1 : conv2d_params.inc_Xo_T,
                (conv2d_params.psum_in ==  0) ? conv2d_params.num_Co : conv2d_params.num_X,
                (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_rev : conv2d_params.inc_Co_T)
        
    int    acc_init_shift = (conv2d_params.psum_in ==  0) ? (int) conv2d_params.shift_bias_init : (int) conv2d_params.shift_psum_in;    
    
    DECL_ITR2D(iterator_pout)
    SETUP_ITR2D(iterator_pout,
               (conv2d_params.psum_out == 0)? conv2d_params.inc_Xo : conv2d_params.inc_Xo_T,
               conv2d_params.num_X,
               (conv2d_params.psum_out == 0)? conv2d_params.inc_Co : conv2d_params.inc_Co_T)
     
    
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (unsigned j=0; j<conv2d_params.outer_g; j++){
        if constexpr(batch_size < 4) {
            v64int8 Sbuff;
            int pdel;
            pdel = (long)p_in;
            frac = (pdel & 31) + 33;
        }
        mmul Cbuff0( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0); 
        mmul Cbuff1( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_1); 
        mmul Cbuff2( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0); 
        mmul Cbuff3( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_2); 
        mmul Cbuff4( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0); 
        mmul Cbuff5( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_1); 
        mmul Cbuff6( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0); 
        mmul Cbuff7( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_3); 

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for (int i=0; i<conv2d_params.inner_g; i++){
            if constexpr(batch_size < 4) {
                vector<uint8_t, 64>  Sbuff0, Sbuff1,Xbuff0,Xbuff1,Xbuff2,Xbuff3;

                // Load IFM data, two 4x8 matrices plus additional adjacent data needed for shift alignment
#ifdef __X86SIM__
                Sbuff0.insert( 0,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
                Sbuff0.insert( 1,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
                Sbuff1.insert( 0,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
                Sbuff1.insert( 1,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in)); 
#else
                Sbuff0.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
                Sbuff0.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
                Sbuff1.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
                Sbuff1.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in)); 
#endif
            
                Xbuff.x0 = shiftx(Xbuff.x0, Sbuff0, conv2d_params.step_align, frac);
                Xbuff.x2 = shiftx(Xbuff.x2, Sbuff1, conv2d_params.step_align, frac);
                
                // TODO: CR-1103776 
                p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,conv2d_params)
                // conv2d_params.iterator_inner_incr0,conv2d_params.iterator_inner_wrap0,conv2d_params.iterator_inner_cnt0
                // conv2d_params.iterator_inner_incr1,conv2d_params.iterator_inner_wrap1,conv2d_params.iterator_inner_cnt1
                
                // conv2d_params.iterator_inner_incr2
                Xbuff0 = interleave(Xbuff.x0, Xbuff.x2, conv2d_params.shfl_0);
                Xbuff1 = interleave(Xbuff0,   unu8, T256_2x2_hi);
                Xbuff2 = interleave(Xbuff.x0, Xbuff.x2, conv2d_params.shfl_1);
                Xbuff3 = interleave(Xbuff2,   unu8, T256_2x2_hi);
                
                            
                Xbuff3 = interleave(Xbuff2,   Xbuff3,   conv2d_params.shfl_2); //for stride of 4
                

                vec_Xbuff0 = Xbuff0.cast_to<dtype_ifm>(). template extract<32>(0); 
                vec_Xbuff1 = Xbuff1.cast_to<dtype_ifm>(). template extract<32>(0); 
                vec_Xbuff2 = Xbuff2.cast_to<dtype_ifm>(). template extract<32>(0); 
                vec_Xbuff3 = Xbuff3.cast_to<dtype_ifm>(). template extract<32>(0);
            }
            else {
                vec_Xbuff0 = load_v<32>((dtype_ifm IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, conv2d_params.inc_A);
                vec_Xbuff1 = load_v<32>((dtype_ifm IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, conv2d_params.inc_A);
                vec_Xbuff2 = load_v<32>((dtype_ifm IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, conv2d_params.inc_A);
                vec_Xbuff3 = load_v<32>((dtype_ifm IFM_DM_BANK *) p_in);
                
                p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,conv2d_params)
            }

            // Load weights -- two matrices 8*8
            LOAD_WEIGHTS(p_w,Ybuff0,32) ;
            LOAD_WEIGHTS(p_w,Ybuff1,32) ;

            #ifdef USE_RTP_SIGN
            int sign_x=conv2d_params.ifm_sign;	
            Cbuff0.mac(op_sign(vec_Xbuff0,sign_x), op_sign(Ybuff0,1));                       
            Cbuff1.mac(op_sign(vec_Xbuff1,sign_x), op_sign(Ybuff0,1));                       
            Cbuff2.mac(op_sign(vec_Xbuff2,sign_x), op_sign(Ybuff0,1));                       
            Cbuff3.mac(op_sign(vec_Xbuff3,sign_x), op_sign(Ybuff0,1));                       
            Cbuff4.mac(op_sign(vec_Xbuff0,sign_x), op_sign(Ybuff1,1));                       
            Cbuff5.mac(op_sign(vec_Xbuff1,sign_x), op_sign(Ybuff1,1));                       
            Cbuff6.mac(op_sign(vec_Xbuff2,sign_x), op_sign(Ybuff1,1));                       
            Cbuff7.mac(op_sign(vec_Xbuff3,sign_x), op_sign(Ybuff1,1));  
            #else
            Cbuff0.mac(vec_Xbuff0, Ybuff0);                       
            Cbuff1.mac(vec_Xbuff1, Ybuff0);                       
            Cbuff2.mac(vec_Xbuff2, Ybuff0);                       
            Cbuff3.mac(vec_Xbuff3, Ybuff0);                       
            Cbuff4.mac(vec_Xbuff0, Ybuff1);                       
            Cbuff5.mac(vec_Xbuff1, Ybuff1);                       
            Cbuff6.mac(vec_Xbuff2, Ybuff1);                       
            Cbuff7.mac(vec_Xbuff3, Ybuff1);  
            #endif                     
            if constexpr(batch_size < 4) {
                int pdel = (long)p_in; 
                frac = (pdel & 31) + 33;
            }
        }

        p_in     = INCR_ITR3D_PARAM(p_in, iterator_outer,conv2d_params)
        p_w      = INCR_ITR3D_PARAM(p_w, iterator_weights,conv2d_params)
        p_init16 = INCR_ITR3D(p_init16, iterator_psum)

        // Write out the final outputs
        if constexpr(out_mode == FULL){
            if constexpr(act_type==LINEAR || act_type==RELU){
                 #ifdef USE_RTP_ACT
                 int out_sign=conv2d_params.out_sign;
                 #else
                 int out_sign = 1-act_type; // Note enum for act types are: 0,1,2 -> linear, relu, lrelu
                 #endif
                 
                 store_v(p_out, Cbuff0.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_0); 
                 store_v(p_out, Cbuff1.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_1);
                 store_v(p_out, Cbuff2.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_0);
                 store_v(p_out, Cbuff3.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_2);                  
                 store_v(p_out, Cbuff4.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_0);
                 store_v(p_out, Cbuff5.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_1);
                 store_v(p_out, Cbuff6.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_0);
                 store_v(p_out, Cbuff7.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out));

            } else {
                // Code/Macro for Leaky reLU
                       
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff0,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, conv2d_params.inc_S_0);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff1,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, conv2d_params.inc_S_1);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff2,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, conv2d_params.inc_S_0);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff3,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, conv2d_params.inc_S_2);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff4,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, conv2d_params.inc_S_0);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff5,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, conv2d_params.inc_S_1);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff6,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, conv2d_params.inc_S_0);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff7,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                        
            }
        }
        else {
        // Store partial sum
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff0.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, conv2d_params.inc_ST_0); 
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff1.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, conv2d_params.inc_ST_1);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff2.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, conv2d_params.inc_ST_0);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff3.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, conv2d_params.inc_ST_2);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff4.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, conv2d_params.inc_ST_0);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff5.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, conv2d_params.inc_ST_1);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff6.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, conv2d_params.inc_ST_0);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff7.template to_vector<int16>(conv2d_params.shift_psum_out));   
        }         
        p_out = INCR_ITR2D(p_out,iterator_pout)
    }
}
#else //(End of stride-4 supported conv2d generic) or (Start of regular conv2d generic)
#ifdef CONV2D_OPT1

template <uint8_t batch_size, act_t act_type, out_mode_t out_mode, typename dtype_ifm, typename dtype_wts, typename dtype_ofm> 
INLINE void conv2d_generic
(
        int * __restrict input,
        int * __restrict weights,
        int * __restrict psum_0,
        int * __restrict psum_1,
        int * __restrict output,
        conv2d_params_t            &conv2d_params
) {

    
    dtype_ifm  IFM_DM_BANK  * p_in      = (dtype_ifm  IFM_DM_BANK *) input;
    dtype_wts  WT_DM_BANK   * p_w       = (dtype_wts   WT_DM_BANK  *) weights;
    
    v8int16    WT_DM_BANK   * p_bias    = (v8int16 WT_DM_BANK *)((int *)p_w-conv2d_params.hdr_len+LAYER_PARAM_WORDS);
    dtype_ofm  OFM_DM_BANK  * __restrict p_out     = (dtype_ofm OFM_DM_BANK * __restrict)output+conv2d_params.ofm_offset_top+conv2d_params.ofm_offset_left*8*batch_size;     

    int16   PSUM_0_DM_BANK * __restrict p_psum_in_0    = (int16 PSUM_0_DM_BANK * __restrict) psum_0;    
    int16   PSUM_1_DM_BANK * __restrict p_psum_in_1    = (int16 PSUM_1_DM_BANK * __restrict) psum_1;
    
    int16   PSUM_0_DM_BANK * __restrict p_psum_out_0   = (int16 PSUM_0_DM_BANK * __restrict) psum_0;    
    int16   PSUM_1_DM_BANK * __restrict p_psum_out_1   = (int16 PSUM_1_DM_BANK * __restrict) psum_1; 
        
    
    v64int8 Xbuff_x0, Xbuff_x1, Xbuff_dummy;
    

    // Following counters are needed for ADD_2D/3D_BYTE intrinsics
    // SETUP_ITR2D_COUNTERS(iterator_name) / SETUP_ITR3D_COUNTERS(iterator_name)
    // type 1: 2D iterator, 2: 3d Iterator
    // e.g. SETUP_ITR3D_COUNTERS(iterator_inner)
    // Resulting count variable is : iterator_inner_cnt0, iterator_inner_cnt_1
    SETUP_ITR3D_COUNTERS(iterator_inner)
    SETUP_ITR3D_COUNTERS(iterator_outer)
    SETUP_ITR3D_COUNTERS(iterator_weights)

    #ifdef CONV2D_2x8_WIDTH_SUPPORT
        SETUP_ITR3D_COUNTERS(iterator_pout)
    #else
        SETUP_ITR2D_COUNTERS(iterator_pout)
    #endif

    unsigned frac;
    int chnd   = 32;
    int chnd_T = 64;

    if constexpr(batch_size < 4) {
        p_in += 31 ;
    }
   
   
    #ifdef CONV2D_2x8_WIDTH_SUPPORT
        DECL_ITR3D(iterator_pout)
        SETUP_ITR3D(iterator_pout,
                   conv2d_params.inc_Xo,
                   conv2d_params.num_Xo,
                   conv2d_params.inc_Co,
                   conv2d_params.num_Co,
                   conv2d_params.inc_Yo
                   )
    #else
        DECL_ITR2D(iterator_pout)
        SETUP_ITR2D(iterator_pout,
                   conv2d_params.inc_Xo,
                   conv2d_params.num_Xo,
                   conv2d_params.inc_Co
                   )
    #endif 
    

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (unsigned j=0; j<conv2d_params.outer_g; j++){
        if constexpr(batch_size < 4) {
            v64int8 Sbuff;
            int pdel;
            pdel = (long)p_in;
            frac = (pdel & 31) + 33;
        }
       
        v64int8 vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3;

        v64int8 Ybuff0, Ybuff1;

        v32acc32 accum_0, accum_1, accum_2, accum_3, accum_4, accum_5, accum_6, accum_7;
       
       
        if constexpr(batch_size < 4) {

            v64int8  Sbuff0, Sbuff1;
            
            Sbuff0 = set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, conv2d_params.inc_A);                
            Sbuff0 = insert(Sbuff0, 1, *(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, conv2d_params.inc_A_2x8_mode);              
            Sbuff1 = set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, conv2d_params.inc_A);
            Sbuff1 = insert(Sbuff1, 1, *(v32int8 IFM_DM_BANK *) p_in);

                     
            Xbuff_x0 = shiftx(Xbuff_x0, Sbuff0, conv2d_params.step_align, frac);
            Xbuff_x1 = shiftx(Xbuff_x1, Sbuff1, conv2d_params.step_align, frac);
            
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,conv2d_params)
            
            vec_Xbuff0 = Xbuff_x0; //interleave(Xbuff_x0, Xbuff_x1, conv2d_params.shfl_0);
            vec_Xbuff1 = interleave(vec_Xbuff0, Xbuff_dummy, T256_2x2_hi);
            vec_Xbuff2 = Xbuff_x1; //interleave(Xbuff_x0, Xbuff_x1, conv2d_params.shfl_1);
            vec_Xbuff3 = interleave(vec_Xbuff2, Xbuff_dummy, T256_2x2_hi);

        } 
        else {
            
            vec_Xbuff0 = set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in);
            p_in = byte_incr(p_in, conv2d_params.inc_A);
            
            vec_Xbuff1 =  set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); 
            p_in = byte_incr(p_in, conv2d_params.inc_A_2x8_mode);
            
            vec_Xbuff2 =  set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); 
            p_in = byte_incr(p_in, conv2d_params.inc_A);
            
            vec_Xbuff3 =  set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in);
            
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,conv2d_params)        
        }                 
                        
        Ybuff0 = *(v64int8 IFM_DM_BANK *) p_w; p_w = byte_incr(p_w, 64);
        Ybuff1 = *(v64int8 IFM_DM_BANK *) p_w; p_w = byte_incr(p_w, 64);
	             

        accum_0 = insert(accum_0,0,sups(*(v16int16 PSUM_0_DM_BANK * __restrict)p_psum_in_0,conv2d_params.shift_psum_in,1)); p_psum_in_0 = byte_incr(p_psum_in_0, 32);
        
        accum_0 = insert(accum_0,1,sups(*(v16int16 PSUM_0_DM_BANK * __restrict)p_psum_in_0,conv2d_params.shift_psum_in,1)); p_psum_in_0 = byte_incr(p_psum_in_0, 32);

        accum_1 = insert(accum_1,0,sups(*(v16int16 PSUM_0_DM_BANK * __restrict)p_psum_in_0,conv2d_params.shift_psum_in,1)); p_psum_in_0 = byte_incr(p_psum_in_0, 32);
        
        accum_1 = insert(accum_1,1,sups(*(v16int16 PSUM_0_DM_BANK * __restrict)p_psum_in_0,conv2d_params.shift_psum_in,1)); p_psum_in_0 = byte_incr(p_psum_in_0, 32);

        accum_2 = insert(accum_2,0,sups(*(v16int16 PSUM_0_DM_BANK * __restrict)p_psum_in_0,conv2d_params.shift_psum_in,1)); p_psum_in_0 = byte_incr(p_psum_in_0, 32);
        
        accum_2 = insert(accum_2,1,sups(*(v16int16 PSUM_0_DM_BANK * __restrict)p_psum_in_0,conv2d_params.shift_psum_in,1)); p_psum_in_0 = byte_incr(p_psum_in_0, 32);

        accum_3 = insert(accum_3,0,sups(*(v16int16 PSUM_0_DM_BANK * __restrict)p_psum_in_0,conv2d_params.shift_psum_in,1)); p_psum_in_0 = byte_incr(p_psum_in_0, 32);
        
        accum_3 = insert(accum_3,1,sups(*(v16int16 PSUM_0_DM_BANK * __restrict)p_psum_in_0,conv2d_params.shift_psum_in,1)); p_psum_in_0 = byte_incr(p_psum_in_0, 32);      
        
        accum_4 = insert(accum_4,0,sups(*(v16int16 PSUM_1_DM_BANK * __restrict)p_psum_in_1,conv2d_params.shift_psum_in,1)); p_psum_in_1 = byte_incr(p_psum_in_1, 32);
        
        accum_4 = insert(accum_4,1,sups(*(v16int16 PSUM_1_DM_BANK * __restrict)p_psum_in_1,conv2d_params.shift_psum_in,1)); p_psum_in_1 = byte_incr(p_psum_in_1, 32);

        accum_5 = insert(accum_5,0,sups(*(v16int16 PSUM_1_DM_BANK * __restrict)p_psum_in_1,conv2d_params.shift_psum_in,1)); p_psum_in_1 = byte_incr(p_psum_in_1, 32);
        
        accum_5 = insert(accum_5,1,sups(*(v16int16 PSUM_1_DM_BANK * __restrict)p_psum_in_1,conv2d_params.shift_psum_in,1)); p_psum_in_1 = byte_incr(p_psum_in_1, 32);

        accum_6 = insert(accum_6,0,sups(*(v16int16 PSUM_1_DM_BANK * __restrict)p_psum_in_1,conv2d_params.shift_psum_in,1)); p_psum_in_1 = byte_incr(p_psum_in_1, 32);
        
        accum_6 = insert(accum_6,1,sups(*(v16int16 PSUM_1_DM_BANK * __restrict)p_psum_in_1,conv2d_params.shift_psum_in,1)); p_psum_in_1 = byte_incr(p_psum_in_1, 32);

        accum_7 = insert(accum_7,0,sups(*(v16int16 PSUM_1_DM_BANK * __restrict)p_psum_in_1,conv2d_params.shift_psum_in,1)); p_psum_in_1 = byte_incr(p_psum_in_1, 32);
        
        accum_7 = insert(accum_7,1,sups(*(v16int16 PSUM_1_DM_BANK * __restrict)p_psum_in_1,conv2d_params.shift_psum_in,1)); p_psum_in_1 = byte_incr(p_psum_in_1, 32);          
        

	    v32uint16 bias_shift = (v32uint16)broadcast_s32((1-conv2d_params.psum_in) << conv2d_params.shift_bias_init);                 

        //int32_t chess_storage(dj2) bias_incr = 128*iterator_weights_cnt1;
        int32_t bias_incr = 128*iterator_weights_cnt1;
        
        v32acc32 chess_storage(cm8) bias_0 = mul_4x2_2x8(bias_shift, 0, set_v32int16(0,*byte_incr(p_bias,bias_incr)),1);   
         
        accum_0 = addmac_4x8_8x8_conf(vec_Xbuff0,conv2d_params.ifm_sign,Ybuff0,1,accum_0,bias_0,1-conv2d_params.psum_in,0,0,0,0);
        accum_1 = addmac_4x8_8x8_conf(vec_Xbuff1,conv2d_params.ifm_sign,Ybuff0,1,accum_1,bias_0,1-conv2d_params.psum_in,0,0,0,0);
        accum_2 = addmac_4x8_8x8_conf(vec_Xbuff2,conv2d_params.ifm_sign,Ybuff0,1,accum_2,bias_0,1-conv2d_params.psum_in,0,0,0,0);
        accum_3 = addmac_4x8_8x8_conf(vec_Xbuff3,conv2d_params.ifm_sign,Ybuff0,1,accum_3,bias_0,1-conv2d_params.psum_in,0,0,0,0);                

        v32acc32 chess_storage(cm8) bias_1 = mul_4x2_2x8(bias_shift, 0, set_v32int16(0,*byte_incr(p_bias,bias_incr+64)), 1);

        accum_4 = addmac_4x8_8x8_conf(vec_Xbuff0,conv2d_params.ifm_sign,Ybuff1,1,accum_4,bias_1,1-conv2d_params.psum_in,0,0,0,0);
        accum_5 = addmac_4x8_8x8_conf(vec_Xbuff1,conv2d_params.ifm_sign,Ybuff1,1,accum_5,bias_1,1-conv2d_params.psum_in,0,0,0,0);
        accum_6 = addmac_4x8_8x8_conf(vec_Xbuff2,conv2d_params.ifm_sign,Ybuff1,1,accum_6,bias_1,1-conv2d_params.psum_in,0,0,0,0);
        accum_7 = addmac_4x8_8x8_conf(vec_Xbuff3,conv2d_params.ifm_sign,Ybuff1,1,accum_7,bias_1,1-conv2d_params.psum_in,0,0,0,0);

	    if constexpr(batch_size < 4) {
                int pdel = (long)p_in; 
                frac = (pdel & 31) + 33;
            }

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for (int i=0; i<conv2d_params.inner_g-1; i++){
        
            if constexpr(batch_size < 4) {
                                  
                v64int8  Sbuff0, Sbuff1;
                
                Sbuff0 = set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, conv2d_params.inc_A);                
                Sbuff0 = insert(Sbuff0, 1, *(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, conv2d_params.inc_A_2x8_mode);              
                Sbuff1 = set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, conv2d_params.inc_A);
                Sbuff1 = insert(Sbuff1, 1, *(v32int8 IFM_DM_BANK *) p_in);

         
                Xbuff_x0 = shiftx(Xbuff_x0, Sbuff0, conv2d_params.step_align, frac);
                Xbuff_x1 = shiftx(Xbuff_x1, Sbuff1, conv2d_params.step_align, frac);
                
                p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,conv2d_params)
                
                vec_Xbuff0 = Xbuff_x0; //interleave(Xbuff_x0, Xbuff_x1, conv2d_params.shfl_0);
                vec_Xbuff1 = interleave(vec_Xbuff0, Xbuff_dummy, T256_2x2_hi);
                vec_Xbuff2 = Xbuff_x1; //interleave(Xbuff_x0, Xbuff_x1, conv2d_params.shfl_1);
                vec_Xbuff3 = interleave(vec_Xbuff2, Xbuff_dummy, T256_2x2_hi);

            } 
            else {
                
                vec_Xbuff0 = set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in);
                p_in = byte_incr(p_in, conv2d_params.inc_A);
                
                vec_Xbuff1 =  set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); 
                p_in = byte_incr(p_in, conv2d_params.inc_A_2x8_mode);
                
                vec_Xbuff2 =  set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); 
                p_in = byte_incr(p_in, conv2d_params.inc_A);
                
                vec_Xbuff3 =  set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in);
                
                p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,conv2d_params)        
            }
            
            Ybuff0 = *(v64int8 IFM_DM_BANK *) p_w; p_w = byte_incr(p_w, 64);
            Ybuff1 = *(v64int8 IFM_DM_BANK *) p_w; p_w = byte_incr(p_w, 64);           
            

            #ifdef USE_RTP_SIGN
            
            int sign_x=conv2d_params.ifm_sign;	            
            accum_0 = mac_4x8_8x8(vec_Xbuff0, sign_x, Ybuff0, 1, accum_0);
            accum_1 = mac_4x8_8x8(vec_Xbuff1, sign_x, Ybuff0, 1, accum_1);
            accum_2 = mac_4x8_8x8(vec_Xbuff2, sign_x, Ybuff0, 1, accum_2);
            accum_3 = mac_4x8_8x8(vec_Xbuff3, sign_x, Ybuff0, 1, accum_3);
            
            accum_4 = mac_4x8_8x8(vec_Xbuff0, sign_x, Ybuff1, 1, accum_4);
            accum_5 = mac_4x8_8x8(vec_Xbuff1, sign_x, Ybuff1, 1, accum_5);
            accum_6 = mac_4x8_8x8(vec_Xbuff2, sign_x, Ybuff1, 1, accum_6);
            accum_7 = mac_4x8_8x8(vec_Xbuff3, sign_x, Ybuff1, 1, accum_7);            
                  
            #else
              
            accum_0 = mac_4x8_8x8(vec_Xbuff0, Ybuff0, accum_0);
            accum_1 = mac_4x8_8x8(vec_Xbuff1, Ybuff0, accum_1);
            accum_2 = mac_4x8_8x8(vec_Xbuff2, Ybuff0, accum_2);
            accum_3 = mac_4x8_8x8(vec_Xbuff3, Ybuff0, accum_3);
            
            accum_4 = mac_4x8_8x8(vec_Xbuff0, Ybuff1, accum_4);
            accum_5 = mac_4x8_8x8(vec_Xbuff1, Ybuff1, accum_5);
            accum_6 = mac_4x8_8x8(vec_Xbuff2, Ybuff1, accum_6);
            accum_7 = mac_4x8_8x8(vec_Xbuff3, Ybuff1, accum_7);            
            
            #endif                     
            
            if constexpr(batch_size < 4) {
                int pdel = (long)p_in; 
                frac = (pdel & 31) + 33;
            }
        }

        p_in     = INCR_ITR3D_PARAM(p_in, iterator_outer,conv2d_params)
        p_w      = INCR_ITR3D_PARAM(p_w, iterator_weights,conv2d_params)

        // Write out the final outputs
           if constexpr(out_mode == FULL){
               
               if constexpr(act_type==LINEAR || act_type==RELU){
                   #ifdef USE_RTP_ACT
                     int out_sign=conv2d_params.out_sign;
                   #else
                     int out_sign = 1-act_type; // Note enum for act types are: 0,1,2 -> linear, relu, lrelu
                   #endif
               
                    *(v32int8 OFM_DM_BANK  * __restrict)p_out = ssrs(accum_0, conv2d_params.shift_out, out_sign); p_out = byte_incr(p_out, 32);
                    *(v32int8 OFM_DM_BANK  * __restrict)p_out = ssrs(accum_1, conv2d_params.shift_out, out_sign); p_out = byte_incr(p_out, conv2d_params.inc_S_0);
                    *(v32int8 OFM_DM_BANK  * __restrict)p_out = ssrs(accum_2, conv2d_params.shift_out, out_sign); p_out = byte_incr(p_out, 32);
                    *(v32int8 OFM_DM_BANK  * __restrict)p_out = ssrs(accum_3, conv2d_params.shift_out, out_sign); p_out = byte_incr(p_out, conv2d_params.inc_S_1);
                    *(v32int8 OFM_DM_BANK  * __restrict)p_out = ssrs(accum_4, conv2d_params.shift_out, out_sign); p_out = byte_incr(p_out, 32);
                    *(v32int8 OFM_DM_BANK  * __restrict)p_out = ssrs(accum_5, conv2d_params.shift_out, out_sign); p_out = byte_incr(p_out, conv2d_params.inc_S_0);
                    *(v32int8 OFM_DM_BANK  * __restrict)p_out = ssrs(accum_6, conv2d_params.shift_out, out_sign); p_out = byte_incr(p_out, 32);
                    *(v32int8 OFM_DM_BANK  * __restrict)p_out = ssrs(accum_7, conv2d_params.shift_out, out_sign);                   
               } else{
               
                //TODO: LeakyRelu
               
               }

               #ifdef CONV2D_2x8_WIDTH_SUPPORT
                    p_out = INCR_ITR3D(p_out,iterator_pout)
                #else
                    p_out = INCR_ITR2D(p_out,iterator_pout)
                #endif
               
           }else{
             // Store partial sum
                
                *(v16int16  PSUM_0_DM_BANK * __restrict)p_psum_out_0 = lsrs(extract_v16acc32(accum_0, 0), conv2d_params.shift_psum_out, 1); p_psum_out_0 = byte_incr(p_psum_out_0, 32); 

                *(v16int16  PSUM_0_DM_BANK * __restrict)p_psum_out_0 = lsrs(extract_v16acc32(accum_0, 1), conv2d_params.shift_psum_out, 1); p_psum_out_0 = byte_incr(p_psum_out_0, 32); 

                *(v16int16  PSUM_0_DM_BANK * __restrict)p_psum_out_0 = lsrs(extract_v16acc32(accum_1, 0), conv2d_params.shift_psum_out, 1); p_psum_out_0 = byte_incr(p_psum_out_0, 32); 

                *(v16int16  PSUM_0_DM_BANK * __restrict)p_psum_out_0 = lsrs(extract_v16acc32(accum_1, 1), conv2d_params.shift_psum_out, 1); p_psum_out_0 = byte_incr(p_psum_out_0, 32); 
                
                *(v16int16  PSUM_0_DM_BANK * __restrict)p_psum_out_0 = lsrs(extract_v16acc32(accum_2, 0), conv2d_params.shift_psum_out, 1); p_psum_out_0 = byte_incr(p_psum_out_0, 32); 

                *(v16int16  PSUM_0_DM_BANK * __restrict)p_psum_out_0 = lsrs(extract_v16acc32(accum_2, 1), conv2d_params.shift_psum_out, 1); p_psum_out_0 = byte_incr(p_psum_out_0, 32); 

                *(v16int16  PSUM_0_DM_BANK * __restrict)p_psum_out_0 = lsrs(extract_v16acc32(accum_3, 0), conv2d_params.shift_psum_out, 1); p_psum_out_0 = byte_incr(p_psum_out_0, 32); 

                *(v16int16  PSUM_0_DM_BANK * __restrict)p_psum_out_0 = lsrs(extract_v16acc32(accum_3, 1), conv2d_params.shift_psum_out, 1); p_psum_out_0 = byte_incr(p_psum_out_0, 32); 

                *(v16int16  PSUM_1_DM_BANK * __restrict)p_psum_out_1 = lsrs(extract_v16acc32(accum_4, 0), conv2d_params.shift_psum_out, 1); p_psum_out_1 = byte_incr(p_psum_out_1, 32); 

                *(v16int16  PSUM_1_DM_BANK * __restrict)p_psum_out_1 = lsrs(extract_v16acc32(accum_4, 1), conv2d_params.shift_psum_out, 1); p_psum_out_1 = byte_incr(p_psum_out_1, 32); 

                *(v16int16  PSUM_1_DM_BANK * __restrict)p_psum_out_1 = lsrs(extract_v16acc32(accum_5, 0), conv2d_params.shift_psum_out, 1); p_psum_out_1 = byte_incr(p_psum_out_1, 32); 

                *(v16int16  PSUM_1_DM_BANK * __restrict)p_psum_out_1 = lsrs(extract_v16acc32(accum_5, 1), conv2d_params.shift_psum_out, 1); p_psum_out_1 = byte_incr(p_psum_out_1, 32); 

                *(v16int16  PSUM_1_DM_BANK * __restrict)p_psum_out_1 = lsrs(extract_v16acc32(accum_6, 0), conv2d_params.shift_psum_out, 1); p_psum_out_1 = byte_incr(p_psum_out_1, 32); 

                *(v16int16  PSUM_1_DM_BANK * __restrict)p_psum_out_1 = lsrs(extract_v16acc32(accum_6, 1), conv2d_params.shift_psum_out, 1); p_psum_out_1 = byte_incr(p_psum_out_1, 32); 

                *(v16int16  PSUM_1_DM_BANK * __restrict)p_psum_out_1 = lsrs(extract_v16acc32(accum_7, 0), conv2d_params.shift_psum_out, 1); p_psum_out_1 = byte_incr(p_psum_out_1, 32); 

                *(v16int16  PSUM_1_DM_BANK * __restrict)p_psum_out_1 = lsrs(extract_v16acc32(accum_7, 1), conv2d_params.shift_psum_out, 1); p_psum_out_1 = byte_incr(p_psum_out_1, 32);              
           }       

    }
      
    if constexpr(out_mode == FULL){
         #if RELU_INT8_ENABLE == 1
            #ifdef USE_RTP_ACT
            	relu_int8_post_process(output,conv2d_params.out_sign,conv2d_params.ofm_len);
            #else
                int act = 1 - act_type;
                relu_int8_post_process(output,act,conv2d_params.ofm_len);   
            #endif
         #endif
    }
}

#else
template <uint8_t batch_size, act_t act_type, out_mode_t out_mode, typename dtype_ifm, typename dtype_wts, typename dtype_ofm> 
INLINE void conv2d_generic
(
        int * __restrict input,
        int * __restrict weights,
        int * __restrict acc_in,
        int * __restrict output,
        conv2d_params_t            &conv2d_params
) {

    int16      OFM_DM_BANK  * p_sum   = ((int16      OFM_DM_BANK *) output + (conv2d_params.psum_buff_offset>>1));
    dtype_ifm IFM_DM_BANK  * p_in    = (dtype_ifm  IFM_DM_BANK *) input; 
    dtype_wts  WT_DM_BANK   * p_w     = (dtype_wts   WT_DM_BANK  *) weights;



    vector<dtype_ifm, 32> vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3;
    int16      OFM_DM_BANK * p_init16 = (int16 OFM_DM_BANK *) acc_in;
    //dtype_ofm OFM_DM_BANK * p_out    = (conv2d_params.psum_out==0)?(dtype_ofm OFM_DM_BANK *)output : (dtype_ofm OFM_DM_BANK *)p_sum;
    dtype_ofm OFM_DM_BANK * p_out    = (conv2d_params.psum_out==0)?((dtype_ofm OFM_DM_BANK *)output+conv2d_params.ofm_offset_top+conv2d_params.ofm_offset_left*8*batch_size) : (dtype_ofm OFM_DM_BANK *)p_sum;



    using mmul = mmul<DIM_M, DIM_N, DIM_K, dtype_ifm, dtype_wts>;

    vector<dtype_wts, 64>  Ybuff0, Ybuff1;

    #define LOAD_WEIGHTS(wts,wbuff,wts_incr) \
    {\
        wbuff.insert(0, load_v<32>(wts));wts+=32;\
        wbuff.insert(1, load_v<32>(wts));wts+=wts_incr;\
    }  

    Vecs4_ui8 Xbuff;
    //undef(Xbuff);

    // Following counters are needed for ADD_2D/3D_BYTE intrinsics
    // SETUP_ITR2D_COUNTERS(iterator_name) / SETUP_ITR3D_COUNTERS(iterator_name)
    // type 1: 2D iterator, 2: 3d Iterator
    // e.g. SETUP_ITR3D_COUNTERS(iterator_inner)
    // Resulting count variable is : iterator_inner_cnt0, iterator_inner_cnt_1
    SETUP_ITR3D_COUNTERS(iterator_inner)
    SETUP_ITR3D_COUNTERS(iterator_outer)
    SETUP_ITR3D_COUNTERS(iterator_weights)
    SETUP_ITR3D_COUNTERS(iterator_psum)
    SETUP_ITR2D_COUNTERS(iterator_pout)

    unsigned frac;
    int chnd   = 32;
    int chnd_T = 64;

    if constexpr(batch_size < 4) {
        p_in += 31 ;
    }
    /*
    int    inc_ST_0       = (conv2d_params.psum_in ==  0) ? chnd_T                              : 2*chnd_T;
    int    inc_ST_1       = (conv2d_params.psum_in ==  0) ? chnd_T                              : conv2d_params.inc_ST_0;
    int    inc_ST_2       = (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_1+chnd_T      : conv2d_params.inc_ST_1;
    int    inc_ST_3       = (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_2+chnd_T      : chnd_T;    
    */
    // 2x8 mode updates for partial sum
    int    inc_ST_0       = (conv2d_params.psum_in ==  0) ? chnd_T                              : 2*chnd_T;
    int    inc_ST_1       = (conv2d_params.psum_in ==  0) ? chnd_T                              : conv2d_params.inc_ST_0_2x8;
    int    inc_ST_2       = (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_1+chnd_T      : conv2d_params.inc_ST_1_2x8;
    int    inc_ST_3       = (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_2+chnd_T      : chnd_T;    
    
    DECL_ITR3D(iterator_psum)
    SETUP_ITR3D(iterator_psum,
                0,
                (conv2d_params.psum_in ==  0) ? conv2d_params.num_X : 0,
                (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_d1 : conv2d_params.iter_2x8_inc_T,
                (conv2d_params.psum_in ==  0) ? conv2d_params.num_Co : conv2d_params.iter_2x8_wrap,
                (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_rev : conv2d_params.inc_Co_T-chnd_T)
                //(conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_d1 : conv2d_params.inc_Xo_T-chnd_T,
                //(conv2d_params.psum_in ==  0) ? conv2d_params.num_Co : conv2d_params.num_X,
        
    int    acc_init_shift = (conv2d_params.psum_in ==  0) ? (int) conv2d_params.shift_bias_init : (int) conv2d_params.shift_psum_in;    
    
    DECL_ITR2D(iterator_pout)
    if constexpr(out_mode == PARTIAL){
        //SETUP_ITR2D(iterator_pout,conv2d_params.inc_Xo_T-chnd_T, conv2d_params.num_X,conv2d_params.inc_Co_T-chnd_T);
        SETUP_ITR2D(iterator_pout,conv2d_params.iter_2x8_inc_T, conv2d_params.iter_2x8_wrap,conv2d_params.inc_Co_T-chnd_T);
    }else{
        //SETUP_ITR2D(iterator_pout,conv2d_params.inc_Xo-chnd,conv2d_params.num_X,conv2d_params.inc_Co-chnd) ;
        SETUP_ITR2D(iterator_pout,conv2d_params.iter_2x8_inc,conv2d_params.iter_2x8_wrap,conv2d_params.inc_Co-chnd) ;
    }
    /*
    SETUP_ITR2D(iterator_pout,
               (conv2d_params.psum_out == 0)? conv2d_params.inc_Xo-chnd : conv2d_params.inc_Xo_T-chnd_T,
               conv2d_params.num_X,
               (conv2d_params.psum_out == 0)? conv2d_params.inc_Co-chnd : conv2d_params.inc_Co_T-chnd_T)
    */
     
    
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (unsigned j=0; j<conv2d_params.outer_g; j++){
        if constexpr(batch_size < 4) {
            v64int8 Sbuff;
            int pdel;
            pdel = (long)p_in;
            frac = (pdel & 31) + 33;
        }
        mmul Cbuff0( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T); 
        mmul Cbuff1( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_1-chnd_T); 
        mmul Cbuff2( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T); 
        mmul Cbuff3( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_2-chnd_T); 
        mmul Cbuff4( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T); 
        mmul Cbuff5( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_1-chnd_T); 
        mmul Cbuff6( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T); 
        mmul Cbuff7( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_3-chnd_T); 

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for (int i=0; i<conv2d_params.inner_g; i++){
            if constexpr(batch_size < 4) {
                vector<uint8_t, 64>  Sbuff0, Sbuff1,Xbuff0,Xbuff1,Xbuff2,Xbuff3;

                // Load IFM data, two 4x8 matrices plus additional adjacent data needed for shift alignment
#ifdef __X86SIM__                
                Sbuff0.insert( 0,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
                Sbuff0.insert( 1,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A_2x8_mode);//2x8 mode support jump of height=1: -32+8*ifm_sv_depth
                Sbuff1.insert( 0,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
                Sbuff1.insert( 1,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in)); 
#else
                Sbuff0.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
                Sbuff0.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A_2x8_mode);//2x8 mode support jump of height=1: -32+8*ifm_sv_depth
                Sbuff1.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
                Sbuff1.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in)); 
#endif                
            
                Xbuff.x0 = shiftx(Xbuff.x0, Sbuff0, conv2d_params.step_align, frac);
                Xbuff.x2 = shiftx(Xbuff.x2, Sbuff1, conv2d_params.step_align, frac);
                
                // TODO: CR-1103776 
                p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,conv2d_params)
                // conv2d_params.iterator_inner_incr0,conv2d_params.iterator_inner_wrap0,conv2d_params.iterator_inner_cnt0
                // conv2d_params.iterator_inner_incr1,conv2d_params.iterator_inner_wrap1,conv2d_params.iterator_inner_cnt1
                // conv2d_params.iterator_inner_incr2
                Xbuff0 = interleave(Xbuff.x0, Xbuff.x2, conv2d_params.shfl_0);
                Xbuff1 = interleave(Xbuff0,   unu8, T256_2x2_hi);
                Xbuff2 = interleave(Xbuff.x0, Xbuff.x2, conv2d_params.shfl_1);
                Xbuff3 = interleave(Xbuff2,   unu8, T256_2x2_hi);


                vec_Xbuff0 = Xbuff0.cast_to<dtype_ifm>(). template extract<32>(0); 
                vec_Xbuff1 = Xbuff1.cast_to<dtype_ifm>(). template extract<32>(0); 
                vec_Xbuff2 = Xbuff2.cast_to<dtype_ifm>(). template extract<32>(0); 
                vec_Xbuff3 = Xbuff3.cast_to<dtype_ifm>(). template extract<32>(0);
            }
            else {
                vec_Xbuff0 = load_v<32>((dtype_ifm IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, conv2d_params.inc_A);
                vec_Xbuff1 = load_v<32>((dtype_ifm IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, conv2d_params.inc_A);
                vec_Xbuff2 = load_v<32>((dtype_ifm IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, conv2d_params.inc_A);
                vec_Xbuff3 = load_v<32>((dtype_ifm IFM_DM_BANK *) p_in);
                
                p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,conv2d_params)
            }

            // Load weights -- two matrices 8*8
            LOAD_WEIGHTS(p_w,Ybuff0,32) ;
            LOAD_WEIGHTS(p_w,Ybuff1,32) ;

            #ifdef USE_RTP_SIGN
            int sign_x=conv2d_params.ifm_sign;	
            Cbuff0.mac(op_sign(vec_Xbuff0,sign_x), op_sign(Ybuff0,1));                       
            Cbuff1.mac(op_sign(vec_Xbuff1,sign_x), op_sign(Ybuff0,1));                       
            Cbuff2.mac(op_sign(vec_Xbuff2,sign_x), op_sign(Ybuff0,1));                       
            Cbuff3.mac(op_sign(vec_Xbuff3,sign_x), op_sign(Ybuff0,1));                       
            Cbuff4.mac(op_sign(vec_Xbuff0,sign_x), op_sign(Ybuff1,1));                       
            Cbuff5.mac(op_sign(vec_Xbuff1,sign_x), op_sign(Ybuff1,1));                       
            Cbuff6.mac(op_sign(vec_Xbuff2,sign_x), op_sign(Ybuff1,1));                       
            Cbuff7.mac(op_sign(vec_Xbuff3,sign_x), op_sign(Ybuff1,1));  
            #else
            Cbuff0.mac(vec_Xbuff0, Ybuff0);                       
            Cbuff1.mac(vec_Xbuff1, Ybuff0);                       
            Cbuff2.mac(vec_Xbuff2, Ybuff0);                       
            Cbuff3.mac(vec_Xbuff3, Ybuff0);                       
            Cbuff4.mac(vec_Xbuff0, Ybuff1);                       
            Cbuff5.mac(vec_Xbuff1, Ybuff1);                       
            Cbuff6.mac(vec_Xbuff2, Ybuff1);                       
            Cbuff7.mac(vec_Xbuff3, Ybuff1);  
            #endif                     
            if constexpr(batch_size < 4) {
                int pdel = (long)p_in; 
                frac = (pdel & 31) + 33;
            }
        }

        p_in     = INCR_ITR3D_PARAM(p_in, iterator_outer,conv2d_params)
        p_w      = INCR_ITR3D_PARAM(p_w, iterator_weights,conv2d_params)
        p_init16 = INCR_ITR3D(p_init16, iterator_psum)

        // Write out the final outputs
        if constexpr(out_mode == FULL){
            if constexpr(act_type==LINEAR || act_type==RELU){
                 #ifdef USE_RTP_ACT
                 int out_sign=conv2d_params.out_sign;
                 #else
                 int out_sign = 1-act_type; // Note enum for act types are: 0,1,2 -> linear, relu, lrelu
                 #endif
                 store_v(p_out, Cbuff0.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff1.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_0_2x8); // 2x8 mode jump by ht=1
                 store_v(p_out, Cbuff2.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff3.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_1_2x8);
                 store_v(p_out, Cbuff4.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff5.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_0_2x8);
                 store_v(p_out, Cbuff6.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff7.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out));
                 /*
                 store_v(p_out, Cbuff0.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32); 
                 store_v(p_out, Cbuff1.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
                 store_v(p_out, Cbuff2.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff3.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_1-chnd);
                 store_v(p_out, Cbuff4.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff5.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
                 store_v(p_out, Cbuff6.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff7.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out));
                 */

            } else {
                // Code/Macro for Leaky reLU
                       
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff0,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff1,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff2,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff3,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, conv2d_params.inc_S_1-chnd);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff4,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff5,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff6,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff7,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                        
            }
        }
        else {
        // Store partial sum
           /*
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff0.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64); 
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff1.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, conv2d_params.inc_ST_0-chnd_T);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff2.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff3.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, conv2d_params.inc_ST_1-chnd_T);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff4.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff5.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, conv2d_params.inc_ST_0-chnd_T);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff6.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff7.template to_vector<int16>(conv2d_params.shift_psum_out));
           */
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff0.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64); 
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff1.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, conv2d_params.inc_ST_0_2x8-chnd_T);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff2.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff3.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, conv2d_params.inc_ST_1_2x8-chnd_T);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff4.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff5.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, conv2d_params.inc_ST_0_2x8-chnd_T);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff6.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff7.template to_vector<int16>(conv2d_params.shift_psum_out));   
        }         
        p_out = INCR_ITR2D(p_out,iterator_pout)
    }

    if constexpr(out_mode == FULL){
         #if RELU_INT8_ENABLE == 1
            #ifdef USE_RTP_ACT
            	relu_int8_post_process(output,conv2d_params.out_sign,conv2d_params.ofm_len);
            #else
                int act = 1 - act_type;
                relu_int8_post_process(output,act,conv2d_params.ofm_len);   
            #endif
         #endif
    }
}
#endif

#endif //End for regular conv2d generic

#ifdef STRIDE2_OPT  //Start of conv2d stride-2 variant 

#ifdef CONV2D_S2_OPT1

template <uint8_t batch_size, act_t act_type, out_mode_t out_mode, typename dtype_ifm, typename dtype_wts, typename dtype_ofm> 
INLINE void conv2d_generic_stride2_opt
(
        int * __restrict input,
        int * __restrict weights,
        int * __restrict psum_0,
        int * __restrict psum_1,
        int * __restrict output,
        conv2d_params_t            &conv2d_params
) {

    dtype_ifm  IFM_DM_BANK  * p_in      = (dtype_ifm  IFM_DM_BANK *) input;
    dtype_wts  WT_DM_BANK_2 * p_w       = (dtype_wts  WT_DM_BANK_2  *) weights;
    
    v8int16    WT_DM_BANK_2 * p_bias    = (v8int16 WT_DM_BANK_2 *)((int *)p_w-conv2d_params.hdr_len+LAYER_PARAM_WORDS);
    dtype_ofm  OFM_DM_BANK  * p_out     = (dtype_ofm OFM_DM_BANK *)output+conv2d_params.ofm_offset_top+conv2d_params.ofm_offset_left*8*batch_size;     

    int16   PSUM_0_DM_BANK * __restrict p_psum_in_0    = (int16 PSUM_0_DM_BANK * __restrict) psum_0;    
    int16   PSUM_1_DM_BANK * __restrict p_psum_in_1    = (int16 PSUM_1_DM_BANK * __restrict) psum_1;
    
    int16   PSUM_0_DM_BANK * __restrict p_psum_out_0   = (int16 PSUM_0_DM_BANK * __restrict) psum_0;    
    int16   PSUM_1_DM_BANK * __restrict p_psum_out_1   = (int16 PSUM_1_DM_BANK * __restrict) psum_1; 


    #define CONV2D_S2_LOAD_INPUT(p_in, iterator_inner, conv2d_params, Sbuff0, Sbuff1, Sbuff2, Sbuff3)\
        {\
            Sbuff0 = set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, 32);\
            Sbuff0 = insert(Sbuff0, 1, *(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, 32);\
            Sbuff1 = set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, 32);\
            Sbuff1 = insert(Sbuff1, 1, *(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, 32);\
            Sbuff2 = set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, 32);\
            Sbuff2 = insert(Sbuff2, 1, *(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, 32);\
            Sbuff3 = set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, 32);\
            Sbuff3 = insert(Sbuff3, 1, *(v32int8 IFM_DM_BANK *) p_in);\
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,conv2d_params)\
        } 

    #define CONV2D_S2_LOAD_INPUT_2x8(p_in, iterator_inner, conv2d_params, Sbuff0, Sbuff1, Sbuff2, Sbuff3)\
        {\
            Sbuff0 = set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, 32);\
            Sbuff0 = insert(Sbuff0, 1, *(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, 32);\
            Sbuff1 = set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, 32);\
            Sbuff1 = insert(Sbuff1, 1, *(v32int8 IFM_DM_BANK *) p_in);  p_in = byte_incr(p_in, conv2d_params.inc_A_2x8_mode);\
            Sbuff2 = set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, 32);\
            Sbuff2 = insert(Sbuff2, 1, *(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, 32);\
            Sbuff3 = set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, 32);\
            Sbuff3 = insert(Sbuff3, 1, *(v32int8 IFM_DM_BANK *) p_in);\
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,conv2d_params)\
        } 

    #define CONV2D_S2_ALIGN_AND_INTERLEAVE(conv2d_params, vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3, Xbuff_x0, Xbuff_x1, Xbuff_x2, Xbuff_x3, Sbuff0, Sbuff1, Sbuff2, Sbuff3)\
        {\
            Xbuff_x0   = shiftx(Xbuff_x0, Sbuff0, conv2d_params.step_align, frac);\
            Xbuff_x1   = shiftx(Xbuff_x1, Sbuff1, conv2d_params.step_align, frac);\
            Xbuff_x2   = shiftx(Xbuff_x2, Sbuff2, conv2d_params.step_align, frac);\
            Xbuff_x3   = shiftx(Xbuff_x3, Sbuff3, conv2d_params.step_align, frac);\
            vec_Xbuff0 = interleave(Xbuff_x0, Xbuff_dummy, conv2d_params.shfl_0);\
            vec_Xbuff1 = interleave(Xbuff_x1, Xbuff_dummy, conv2d_params.shfl_0);\
            vec_Xbuff2 = interleave(Xbuff_x2, Xbuff_dummy, conv2d_params.shfl_0);\
            vec_Xbuff3 = interleave(Xbuff_x3, Xbuff_dummy, conv2d_params.shfl_0);\
        }

    #define CONV2D_S2_LOAD_WTS(p_w, Ybuff0, Ybuff1)\
        {\
            Ybuff0 = *(v64int8 WT_DM_BANK_2 *) p_w; p_w = byte_incr(p_w, 64);\
            Ybuff1 = *(v64int8 WT_DM_BANK_2 *) p_w; p_w = byte_incr(p_w, 64);\
        }


    #define CONV2D_S2_LOAD_8_ACCUM(p_psum_in_0, p_psum_in_1, accum_0, accum_1, accum_2, accum_3, accum_4, accum_5, accum_6, accum_7)\
        {\
            accum_0 = insert(accum_0,0,sups(*(v16int16 PSUM_0_DM_BANK *)p_psum_in_0,conv2d_params.shift_psum_in,1)); p_psum_in_0 = byte_incr(p_psum_in_0, 32);\
            accum_0 = insert(accum_0,1,sups(*(v16int16 PSUM_0_DM_BANK *)p_psum_in_0,conv2d_params.shift_psum_in,1)); p_psum_in_0 = byte_incr(p_psum_in_0, 32);\
            accum_1 = insert(accum_1,0,sups(*(v16int16 PSUM_0_DM_BANK *)p_psum_in_0,conv2d_params.shift_psum_in,1)); p_psum_in_0 = byte_incr(p_psum_in_0, 32);\
            accum_1 = insert(accum_1,1,sups(*(v16int16 PSUM_0_DM_BANK *)p_psum_in_0,conv2d_params.shift_psum_in,1)); p_psum_in_0 = byte_incr(p_psum_in_0, 32);\
            accum_2 = insert(accum_2,0,sups(*(v16int16 PSUM_0_DM_BANK *)p_psum_in_0,conv2d_params.shift_psum_in,1)); p_psum_in_0 = byte_incr(p_psum_in_0, 32);\
            accum_2 = insert(accum_2,1,sups(*(v16int16 PSUM_0_DM_BANK *)p_psum_in_0,conv2d_params.shift_psum_in,1)); p_psum_in_0 = byte_incr(p_psum_in_0, 32);\
            accum_3 = insert(accum_3,0,sups(*(v16int16 PSUM_0_DM_BANK *)p_psum_in_0,conv2d_params.shift_psum_in,1)); p_psum_in_0 = byte_incr(p_psum_in_0, 32);\
            accum_3 = insert(accum_3,1,sups(*(v16int16 PSUM_0_DM_BANK *)p_psum_in_0,conv2d_params.shift_psum_in,1)); p_psum_in_0 = byte_incr(p_psum_in_0, 32);\
            accum_4 = insert(accum_4,0,sups(*(v16int16 PSUM_1_DM_BANK *)p_psum_in_1,conv2d_params.shift_psum_in,1)); p_psum_in_1 = byte_incr(p_psum_in_1, 32);\
            accum_4 = insert(accum_4,1,sups(*(v16int16 PSUM_1_DM_BANK *)p_psum_in_1,conv2d_params.shift_psum_in,1)); p_psum_in_1 = byte_incr(p_psum_in_1, 32);\
            accum_5 = insert(accum_5,0,sups(*(v16int16 PSUM_1_DM_BANK *)p_psum_in_1,conv2d_params.shift_psum_in,1)); p_psum_in_1 = byte_incr(p_psum_in_1, 32);\
            accum_5 = insert(accum_5,1,sups(*(v16int16 PSUM_1_DM_BANK *)p_psum_in_1,conv2d_params.shift_psum_in,1)); p_psum_in_1 = byte_incr(p_psum_in_1, 32);\
            accum_6 = insert(accum_6,0,sups(*(v16int16 PSUM_1_DM_BANK *)p_psum_in_1,conv2d_params.shift_psum_in,1)); p_psum_in_1 = byte_incr(p_psum_in_1, 32);\
            accum_6 = insert(accum_6,1,sups(*(v16int16 PSUM_1_DM_BANK *)p_psum_in_1,conv2d_params.shift_psum_in,1)); p_psum_in_1 = byte_incr(p_psum_in_1, 32);\
            accum_7 = insert(accum_7,0,sups(*(v16int16 PSUM_1_DM_BANK *)p_psum_in_1,conv2d_params.shift_psum_in,1)); p_psum_in_1 = byte_incr(p_psum_in_1, 32);\
            accum_7 = insert(accum_7,1,sups(*(v16int16 PSUM_1_DM_BANK *)p_psum_in_1,conv2d_params.shift_psum_in,1)); p_psum_in_1 = byte_incr(p_psum_in_1, 32);\
        }


    #define CONV2D_S2_PARTIAL_SUM_OR_BIAS_SELECTION_8_ACCUM(p_bias, conv2d_params, accum_0, accum_1, accum_2, accum_3, accum_4, accum_5, accum_6, accum_7)\
        {\
            v32uint16 bias_shift = (v32uint16)broadcast_s32((1-conv2d_params.psum_in) << conv2d_params.shift_bias_init);\
            int32_t bias_incr = 128*iterator_weights_cnt1;\
            \
            v32acc32 chess_storage(cm8) bias_0 = mul_4x2_2x8(bias_shift, 0, set_v32int16(0,*byte_incr(p_bias,bias_incr)),1);\
            accum_0 = addmac_4x8_8x8_conf(vec_Xbuff0,conv2d_params.ifm_sign,Ybuff0,1,accum_0,bias_0,1-conv2d_params.psum_in,0,0,0,0);\
            accum_1 = addmac_4x8_8x8_conf(vec_Xbuff1,conv2d_params.ifm_sign,Ybuff0,1,accum_1,bias_0,1-conv2d_params.psum_in,0,0,0,0);\
            accum_2 = addmac_4x8_8x8_conf(vec_Xbuff2,conv2d_params.ifm_sign,Ybuff0,1,accum_2,bias_0,1-conv2d_params.psum_in,0,0,0,0);\
            accum_3 = addmac_4x8_8x8_conf(vec_Xbuff3,conv2d_params.ifm_sign,Ybuff0,1,accum_3,bias_0,1-conv2d_params.psum_in,0,0,0,0);\
            \
            v32acc32 chess_storage(cm8) bias_1 = mul_4x2_2x8(bias_shift, 0, set_v32int16(0,*byte_incr(p_bias,bias_incr+64)), 1);\
            \
            accum_4 = addmac_4x8_8x8_conf(vec_Xbuff0,conv2d_params.ifm_sign,Ybuff1,1,accum_4,bias_1,1-conv2d_params.psum_in,0,0,0,0);\
            accum_5 = addmac_4x8_8x8_conf(vec_Xbuff1,conv2d_params.ifm_sign,Ybuff1,1,accum_5,bias_1,1-conv2d_params.psum_in,0,0,0,0);\
            accum_6 = addmac_4x8_8x8_conf(vec_Xbuff2,conv2d_params.ifm_sign,Ybuff1,1,accum_6,bias_1,1-conv2d_params.psum_in,0,0,0,0);\
            accum_7 = addmac_4x8_8x8_conf(vec_Xbuff3,conv2d_params.ifm_sign,Ybuff1,1,accum_7,bias_1,1-conv2d_params.psum_in,0,0,0,0);\
        }


    #define CONV2D_S2_STORE_8_ACCUM_FULL(p_out, conv2d_params, out_sign, accum_0, accum_1, accum_2, accum_3, accum_4, accum_5, accum_6, accum_7)\
        {\
            *(v32int8 OFM_DM_BANK  *)p_out = ssrs(accum_0, conv2d_params.shift_out, out_sign); p_out = byte_incr(p_out, 32);\
            *(v32int8 OFM_DM_BANK  *)p_out = ssrs(accum_1, conv2d_params.shift_out, out_sign); p_out = byte_incr(p_out, conv2d_params.inc_S_0);\
            *(v32int8 OFM_DM_BANK  *)p_out = ssrs(accum_2, conv2d_params.shift_out, out_sign); p_out = byte_incr(p_out, 32);\
            *(v32int8 OFM_DM_BANK  *)p_out = ssrs(accum_3, conv2d_params.shift_out, out_sign); p_out = byte_incr(p_out, conv2d_params.inc_S_1);\
            *(v32int8 OFM_DM_BANK  *)p_out = ssrs(accum_4, conv2d_params.shift_out, out_sign); p_out = byte_incr(p_out, 32);\
            *(v32int8 OFM_DM_BANK  *)p_out = ssrs(accum_5, conv2d_params.shift_out, out_sign); p_out = byte_incr(p_out, conv2d_params.inc_S_0);\
            *(v32int8 OFM_DM_BANK  *)p_out = ssrs(accum_6, conv2d_params.shift_out, out_sign); p_out = byte_incr(p_out, 32);\
            *(v32int8 OFM_DM_BANK  *)p_out = ssrs(accum_7, conv2d_params.shift_out, out_sign);\
        } 


    #define CONV2D_S2_STORE_8_ACCUM_PARTIAL(p_out, conv2d_params, accum_0, accum_1, accum_2, accum_3, accum_4, accum_5, accum_6, accum_7)\
        {\
            *(v16int16  PSUM_0_DM_BANK * __restrict)p_psum_out_0 = lsrs(extract_v16acc32(accum_0, 0), conv2d_params.shift_psum_out, 1); p_psum_out_0 = byte_incr(p_psum_out_0, 32);\
            *(v16int16  PSUM_0_DM_BANK * __restrict)p_psum_out_0 = lsrs(extract_v16acc32(accum_0, 1), conv2d_params.shift_psum_out, 1); p_psum_out_0 = byte_incr(p_psum_out_0, 32);\
            *(v16int16  PSUM_0_DM_BANK * __restrict)p_psum_out_0 = lsrs(extract_v16acc32(accum_1, 0), conv2d_params.shift_psum_out, 1); p_psum_out_0 = byte_incr(p_psum_out_0, 32);\
            *(v16int16  PSUM_0_DM_BANK * __restrict)p_psum_out_0 = lsrs(extract_v16acc32(accum_1, 1), conv2d_params.shift_psum_out, 1); p_psum_out_0 = byte_incr(p_psum_out_0, 32);\
            *(v16int16  PSUM_0_DM_BANK * __restrict)p_psum_out_0 = lsrs(extract_v16acc32(accum_2, 0), conv2d_params.shift_psum_out, 1); p_psum_out_0 = byte_incr(p_psum_out_0, 32);\
            *(v16int16  PSUM_0_DM_BANK * __restrict)p_psum_out_0 = lsrs(extract_v16acc32(accum_2, 1), conv2d_params.shift_psum_out, 1); p_psum_out_0 = byte_incr(p_psum_out_0, 32);\
            *(v16int16  PSUM_0_DM_BANK * __restrict)p_psum_out_0 = lsrs(extract_v16acc32(accum_3, 0), conv2d_params.shift_psum_out, 1); p_psum_out_0 = byte_incr(p_psum_out_0, 32);\
            *(v16int16  PSUM_0_DM_BANK * __restrict)p_psum_out_0 = lsrs(extract_v16acc32(accum_3, 1), conv2d_params.shift_psum_out, 1); p_psum_out_0 = byte_incr(p_psum_out_0, 32);\
            *(v16int16  PSUM_1_DM_BANK * __restrict)p_psum_out_1 = lsrs(extract_v16acc32(accum_4, 0), conv2d_params.shift_psum_out, 1); p_psum_out_1 = byte_incr(p_psum_out_1, 32);\
            *(v16int16  PSUM_1_DM_BANK * __restrict)p_psum_out_1 = lsrs(extract_v16acc32(accum_4, 1), conv2d_params.shift_psum_out, 1); p_psum_out_1 = byte_incr(p_psum_out_1, 32);\
            *(v16int16  PSUM_1_DM_BANK * __restrict)p_psum_out_1 = lsrs(extract_v16acc32(accum_5, 0), conv2d_params.shift_psum_out, 1); p_psum_out_1 = byte_incr(p_psum_out_1, 32);\
            *(v16int16  PSUM_1_DM_BANK * __restrict)p_psum_out_1 = lsrs(extract_v16acc32(accum_5, 1), conv2d_params.shift_psum_out, 1); p_psum_out_1 = byte_incr(p_psum_out_1, 32);\
            *(v16int16  PSUM_1_DM_BANK * __restrict)p_psum_out_1 = lsrs(extract_v16acc32(accum_6, 0), conv2d_params.shift_psum_out, 1); p_psum_out_1 = byte_incr(p_psum_out_1, 32);\
            *(v16int16  PSUM_1_DM_BANK * __restrict)p_psum_out_1 = lsrs(extract_v16acc32(accum_6, 1), conv2d_params.shift_psum_out, 1); p_psum_out_1 = byte_incr(p_psum_out_1, 32);\
            *(v16int16  PSUM_1_DM_BANK * __restrict)p_psum_out_1 = lsrs(extract_v16acc32(accum_7, 0), conv2d_params.shift_psum_out, 1); p_psum_out_1 = byte_incr(p_psum_out_1, 32);\
            *(v16int16  PSUM_1_DM_BANK * __restrict)p_psum_out_1 = lsrs(extract_v16acc32(accum_7, 1), conv2d_params.shift_psum_out, 1); p_psum_out_1 = byte_incr(p_psum_out_1, 32);\
        }


    v64int8 Xbuff_x0, Xbuff_x1, Xbuff_x2, Xbuff_x3, Xbuff_dummy;    

    // Following counters are needed for ADD_2D/3D_BYTE intrinsics
    // SETUP_ITR2D_COUNTERS(iterator_name) / SETUP_ITR3D_COUNTERS(iterator_name)
    // type 1: 2D iterator, 2: 3d Iterator
    // e.g. SETUP_ITR3D_COUNTERS(iterator_inner)
    // Resulting count variable is : iterator_inner_cnt0, iterator_inner_cnt_1
    SETUP_ITR3D_COUNTERS(iterator_inner)
    SETUP_ITR3D_COUNTERS(iterator_outer)
    SETUP_ITR3D_COUNTERS(iterator_weights)
    #ifdef CONV2D_2x8_WIDTH_SUPPORT
        SETUP_ITR3D_COUNTERS(iterator_pout)   
    #else
        SETUP_ITR2D_COUNTERS(iterator_pout) 
    #endif
    

    #ifdef CONV2D_2x8_WIDTH_SUPPORT
        DECL_ITR3D(iterator_pout)
        SETUP_ITR3D(iterator_pout,
                   conv2d_params.inc_Xo,
                   conv2d_params.num_Xo,
                   conv2d_params.inc_Co,
                   conv2d_params.num_Co,
                   conv2d_params.inc_Yo
                   )
    #else
        DECL_ITR2D(iterator_pout)
        SETUP_ITR2D(iterator_pout,
                   conv2d_params.inc_Xo,
                   conv2d_params.num_Xo,
                   conv2d_params.inc_Co
                   )
    #endif 

    int cnt_frac  = 31;
    p_in = byte_incr(p_in, 31);     
    
    int step_align_check=(conv2d_params.step_align!=0)<<(conv2d_params.step_align+1); 

    if constexpr(out_mode == FULL){        
        for (unsigned j=0; j<conv2d_params.outer_g; j++)
        {
            unsigned frac = (cnt_frac & 31) + 33;
            //cnt_frac = (cnt_frac==conv2d_params.num_Kx*((conv2d_params.step_align!=0)<<(conv2d_params.step_align+1))+31 ? 31 : cnt_frac+((conv2d_params.step_align!=0)<<(conv2d_params.step_align+1)));        
            cnt_frac = (cnt_frac==conv2d_params.num_Kx*step_align_check+31) ? 31 : cnt_frac+step_align_check;

            v64int8 vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3;
            v64int8 Sbuff0, Sbuff1, Sbuff2, Sbuff3;
            v64int8 Ybuff0, Ybuff1;
            v32acc32 accum_0, accum_1, accum_2, accum_3, accum_4, accum_5, accum_6, accum_7;

            #ifdef CONV2D_S2_2x8_SUPPORT
                CONV2D_S2_LOAD_INPUT_2x8(p_in, iterator_inner, conv2d_params, Sbuff0, Sbuff1, Sbuff2, Sbuff3)
            #else
                CONV2D_S2_LOAD_INPUT(p_in, iterator_inner, conv2d_params, Sbuff0, Sbuff1, Sbuff2, Sbuff3)  
            #endif
            
            CONV2D_S2_ALIGN_AND_INTERLEAVE(conv2d_params, vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3, Xbuff_x0, Xbuff_x1, Xbuff_x2, Xbuff_x3, Sbuff0, Sbuff1, Sbuff2, Sbuff3)     
            
            CONV2D_S2_LOAD_WTS(p_w, Ybuff0, Ybuff1)

            CONV2D_S2_LOAD_8_ACCUM(p_psum_in_0, p_psum_in_1, accum_0, accum_1, accum_2, accum_3, accum_4, accum_5, accum_6, accum_7)       

            CONV2D_S2_PARTIAL_SUM_OR_BIAS_SELECTION_8_ACCUM(p_bias, conv2d_params, accum_0, accum_1, accum_2, accum_3, accum_4, accum_5, accum_6, accum_7)

            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(4,) 
            for (int i=0; i<conv2d_params.inner_g-1; i++){
                frac = (cnt_frac & 31) + 33;
                //cnt_frac = (cnt_frac==conv2d_params.num_Kx*((conv2d_params.step_align!=0)<<(conv2d_params.step_align+1))+31 ? 31 : cnt_frac+((conv2d_params.step_align!=0)<<(conv2d_params.step_align+1)));        
                cnt_frac = (cnt_frac==conv2d_params.num_Kx*step_align_check+31) ? 31 : cnt_frac+step_align_check; 
                                           
                #ifdef CONV2D_S2_2x8_SUPPORT
                    CONV2D_S2_LOAD_INPUT_2x8(p_in, iterator_inner, conv2d_params, Sbuff0, Sbuff1, Sbuff2, Sbuff3)
                #else
                    CONV2D_S2_LOAD_INPUT(p_in, iterator_inner, conv2d_params, Sbuff0, Sbuff1, Sbuff2, Sbuff3)
                #endif
                
                CONV2D_S2_ALIGN_AND_INTERLEAVE(conv2d_params, vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3, Xbuff_x0, Xbuff_x1, Xbuff_x2, Xbuff_x3, Sbuff0, Sbuff1, Sbuff2, Sbuff3)           
                
                CONV2D_S2_LOAD_WTS(p_w, Ybuff0, Ybuff1)
               
                int sign_x=conv2d_params.ifm_sign;	            
                accum_0 = mac_4x8_8x8(vec_Xbuff0, sign_x, Ybuff0, 1, accum_0);
                accum_1 = mac_4x8_8x8(vec_Xbuff1, sign_x, Ybuff0, 1, accum_1);
                accum_2 = mac_4x8_8x8(vec_Xbuff2, sign_x, Ybuff0, 1, accum_2);
                accum_3 = mac_4x8_8x8(vec_Xbuff3, sign_x, Ybuff0, 1, accum_3);          
                accum_4 = mac_4x8_8x8(vec_Xbuff0, sign_x, Ybuff1, 1, accum_4);
                accum_5 = mac_4x8_8x8(vec_Xbuff1, sign_x, Ybuff1, 1, accum_5);
                accum_6 = mac_4x8_8x8(vec_Xbuff2, sign_x, Ybuff1, 1, accum_6);
                accum_7 = mac_4x8_8x8(vec_Xbuff3, sign_x, Ybuff1, 1, accum_7);    
            }

            p_in     = INCR_ITR3D_PARAM(p_in, iterator_outer,conv2d_params)
            p_w      = INCR_ITR3D_PARAM(p_w, iterator_weights,conv2d_params)

            // Write out the final outputs
            if constexpr(act_type==LINEAR || act_type==RELU){                
                #ifdef USE_RTP_ACT
                    int out_sign=conv2d_params.out_sign;
                #else
                    int out_sign = 1-act_type; // Note enum for act types are: 0,1,2 -> linear, relu, lrelu
                #endif
                
                CONV2D_S2_STORE_8_ACCUM_FULL(p_out, conv2d_params, out_sign, accum_0, accum_1, accum_2, accum_3, accum_4, accum_5, accum_6, accum_7)

            } else {
                //TODO: LeakyRelu                       
            }
                    
            #ifdef CONV2D_2x8_WIDTH_SUPPORT
                p_out = INCR_ITR3D(p_out,iterator_pout)   
            #else
                p_out = INCR_ITR2D(p_out,iterator_pout)
            #endif              
        }
        
    } else {
    
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(2,) 
        for (unsigned j=0; j<conv2d_params.outer_g; j++){
            unsigned frac = (cnt_frac & 31) + 33;
            //cnt_frac = (cnt_frac==conv2d_params.num_Kx*((conv2d_params.step_align!=0)<<(conv2d_params.step_align+1))+31 ? 31 : cnt_frac+((conv2d_params.step_align!=0)<<(conv2d_params.step_align+1)));        
            cnt_frac = (cnt_frac==conv2d_params.num_Kx*step_align_check+31) ? 31 : cnt_frac+step_align_check;

            v64int8 vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3;
            v64int8 Sbuff0, Sbuff1, Sbuff2, Sbuff3;
            v64int8 Ybuff0, Ybuff1;
            v32acc32 accum_0, accum_1, accum_2, accum_3, accum_4, accum_5, accum_6, accum_7;

            #ifdef CONV2D_S2_2x8_SUPPORT
                CONV2D_S2_LOAD_INPUT_2x8(p_in, iterator_inner, conv2d_params, Sbuff0, Sbuff1, Sbuff2, Sbuff3)
            #else
                CONV2D_S2_LOAD_INPUT(p_in, iterator_inner, conv2d_params, Sbuff0, Sbuff1, Sbuff2, Sbuff3)  
            #endif
            
            CONV2D_S2_ALIGN_AND_INTERLEAVE(conv2d_params, vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3, Xbuff_x0, Xbuff_x1, Xbuff_x2, Xbuff_x3, Sbuff0, Sbuff1, Sbuff2, Sbuff3)     
            
            CONV2D_S2_LOAD_WTS(p_w, Ybuff0, Ybuff1)

            CONV2D_S2_LOAD_8_ACCUM(p_psum_in_0, p_psum_in_1, accum_0, accum_1, accum_2, accum_3, accum_4, accum_5, accum_6, accum_7)       

            CONV2D_S2_PARTIAL_SUM_OR_BIAS_SELECTION_8_ACCUM(p_bias, conv2d_params, accum_0, accum_1, accum_2, accum_3, accum_4, accum_5, accum_6, accum_7)

            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(4,) 
            for (int i=0; i<conv2d_params.inner_g-1; i++){
                frac = (cnt_frac & 31) + 33;
                //cnt_frac = (cnt_frac==conv2d_params.num_Kx*((conv2d_params.step_align!=0)<<(conv2d_params.step_align+1))+31 ? 31 : cnt_frac+((conv2d_params.step_align!=0)<<(conv2d_params.step_align+1)));        
                cnt_frac = (cnt_frac==conv2d_params.num_Kx*step_align_check+31) ? 31 : cnt_frac+step_align_check;                                  
                #ifdef CONV2D_S2_2x8_SUPPORT
                    CONV2D_S2_LOAD_INPUT_2x8(p_in, iterator_inner, conv2d_params, Sbuff0, Sbuff1, Sbuff2, Sbuff3)
                #else
                    CONV2D_S2_LOAD_INPUT(p_in, iterator_inner, conv2d_params, Sbuff0, Sbuff1, Sbuff2, Sbuff3)
                #endif
                
                CONV2D_S2_ALIGN_AND_INTERLEAVE(conv2d_params, vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3, Xbuff_x0, Xbuff_x1, Xbuff_x2, Xbuff_x3, Sbuff0, Sbuff1, Sbuff2, Sbuff3)           
                
                CONV2D_S2_LOAD_WTS(p_w, Ybuff0, Ybuff1)
               
                int sign_x=conv2d_params.ifm_sign;	            
                accum_0 = mac_4x8_8x8(vec_Xbuff0, sign_x, Ybuff0, 1, accum_0);
                accum_1 = mac_4x8_8x8(vec_Xbuff1, sign_x, Ybuff0, 1, accum_1);
                accum_2 = mac_4x8_8x8(vec_Xbuff2, sign_x, Ybuff0, 1, accum_2);
                accum_3 = mac_4x8_8x8(vec_Xbuff3, sign_x, Ybuff0, 1, accum_3);          
                accum_4 = mac_4x8_8x8(vec_Xbuff0, sign_x, Ybuff1, 1, accum_4);
                accum_5 = mac_4x8_8x8(vec_Xbuff1, sign_x, Ybuff1, 1, accum_5);
                accum_6 = mac_4x8_8x8(vec_Xbuff2, sign_x, Ybuff1, 1, accum_6);
                accum_7 = mac_4x8_8x8(vec_Xbuff3, sign_x, Ybuff1, 1, accum_7);    
            }

            p_in     = INCR_ITR3D_PARAM(p_in, iterator_outer,conv2d_params)
            p_w      = INCR_ITR3D_PARAM(p_w, iterator_weights,conv2d_params)   
            
            CONV2D_S2_STORE_8_ACCUM_PARTIAL(p_out, conv2d_params, accum_0, accum_1, accum_2, accum_3, accum_4, accum_5, accum_6, accum_7)
    }
    
    }

    if constexpr(out_mode == FULL){
         #if RELU_INT8_ENABLE == 1
            #ifdef USE_RTP_ACT
            	relu_int8_post_process(output,conv2d_params.out_sign,conv2d_params.ofm_len);
            #else
                int act = 1 - act_type;
                relu_int8_post_process(output,act,conv2d_params.ofm_len);   
            #endif
         #endif
    }
}



#else
template <uint8_t batch_size, act_t act_type, out_mode_t out_mode, typename dtype_ifm, typename dtype_wts, typename dtype_ofm> 
INLINE void conv2d_generic_stride2_opt
(
        int * __restrict input,
        int * __restrict weights,
        int * __restrict acc_in,
        int * __restrict output,
        conv2d_params_t            &conv2d_params
) {

    int16      DM_BANK(c)  * p_sum   = ((int16      DM_BANK(c) *) output + (conv2d_params.psum_buff_offset>>1));
    dtype_ifm IFM_DM_BANK  * p_in    = (dtype_ifm  IFM_DM_BANK *) input; 
    dtype_wts  DM_BANK(b)   * p_w     = (dtype_wts   DM_BANK(b)  *) weights;



    vector<dtype_ifm, 32> vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3;
    int16      DM_BANK(c) * p_init16 = (int16 DM_BANK(c) *) acc_in;
    //dtype_ofm DM_BANK(c) * p_out    = (conv2d_params.psum_out==0)?(dtype_ofm DM_BANK(c) *)output : (dtype_ofm DM_BANK(c) *)p_sum;
    dtype_ofm DM_BANK(c) * p_out    = (conv2d_params.psum_out==0)?((dtype_ofm DM_BANK(c) *)output+conv2d_params.ofm_offset_top+conv2d_params.ofm_offset_left*8*batch_size) : (dtype_ofm DM_BANK(c) *)p_sum;



    using mmul = mmul<DIM_M, DIM_N, DIM_K, dtype_ifm, dtype_wts>;

    vector<dtype_wts, 64>  Ybuff0, Ybuff1;

    #define LOAD_WEIGHTS(wts,wbuff,wts_incr) \
    {\
        wbuff.insert(0, load_v<32>(wts));wts+=32;\
        wbuff.insert(1, load_v<32>(wts));wts+=wts_incr;\
    }  

    Vecs4_ui8 Xbuff;
    //undef(Xbuff);

    // Following counters are needed for ADD_2D/3D_BYTE intrinsics
    // SETUP_ITR2D_COUNTERS(iterator_name) / SETUP_ITR3D_COUNTERS(iterator_name)
    // type 1: 2D iterator, 2: 3d Iterator
    // e.g. SETUP_ITR3D_COUNTERS(iterator_inner)
    // Resulting count variable is : iterator_inner_cnt0, iterator_inner_cnt_1
    SETUP_ITR3D_COUNTERS(iterator_inner)
    SETUP_ITR3D_COUNTERS(iterator_outer)
    SETUP_ITR3D_COUNTERS(iterator_weights)
    SETUP_ITR3D_COUNTERS(iterator_psum)
    #ifdef CONV2D_S2_2x8_SUPPORT
        #ifdef CONV2D_2x8_WIDTH_SUPPORT
            SETUP_ITR3D_COUNTERS(iterator_pout)
        #else
            SETUP_ITR2D_COUNTERS(iterator_pout)
        #endif
    #else
        SETUP_ITR2D_COUNTERS(iterator_pout)
    #endif
 
    int chnd   = 32;
    int chnd_T = 64;

    int cnt_frac  = 31;
    p_in = byte_incr(p_in, 31);

    int    inc_ST_0       = (conv2d_params.psum_in ==  0) ? chnd_T                              : 2*chnd_T;
    int    inc_ST_1       = (conv2d_params.psum_in ==  0) ? chnd_T                              : conv2d_params.inc_ST_0;
    int    inc_ST_2       = (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_1+chnd_T      : conv2d_params.inc_ST_1;
    int    inc_ST_3       = (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_2+chnd_T      : chnd_T;    

    
     #ifdef CONV2D_S2_2x8_SUPPORT   
        #ifdef CONV2D_2x8_WIDTH_SUPPORT      
            DECL_ITR3D(iterator_psum)
            SETUP_ITR3D(iterator_psum,
                 (conv2d_params.psum_in ==  0) ? 0                         : conv2d_params.inc_Xo_T-chnd_T,
                 (conv2d_params.psum_in ==  0) ? conv2d_params.num_X       : conv2d_params.num_Xo,
                 (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_d1  : conv2d_params.inc_Co_T-chnd_T,
                 (conv2d_params.psum_in ==  0) ? conv2d_params.num_Co      : conv2d_params.num_Co,
                 (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_rev : conv2d_params.inc_Yo_T-chnd_T)
        #else
            DECL_ITR3D(iterator_psum)
            SETUP_ITR3D(iterator_psum,
                  0,
                 (conv2d_params.psum_in ==  0) ? conv2d_params.num_X       : 0,
                 (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_d1  : conv2d_params.inc_Xo_T-chnd_T,
                 (conv2d_params.psum_in ==  0) ? conv2d_params.num_Co      : conv2d_params.num_Xo,
                 (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_rev : conv2d_params.inc_Co_T-chnd_T)
        #endif
    #else
        DECL_ITR3D(iterator_psum)
        SETUP_ITR3D(iterator_psum,
                0,
                (conv2d_params.psum_in ==  0) ? conv2d_params.num_X : 0,
                (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_d1 : conv2d_params.inc_Xo_T-chnd_T,
                (conv2d_params.psum_in ==  0) ? conv2d_params.num_Co : conv2d_params.num_X,
                (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_rev : conv2d_params.inc_Co_T-chnd_T)
    #endif

        
    int    acc_init_shift = (conv2d_params.psum_in ==  0) ? (int) conv2d_params.shift_bias_init : (int) conv2d_params.shift_psum_in;    
    
    #ifdef CONV2D_S2_2x8_SUPPORT
        #ifdef CONV2D_2x8_WIDTH_SUPPORT    
            DECL_ITR3D(iterator_pout)
            if constexpr(out_mode == PARTIAL){
                SETUP_ITR3D(iterator_pout,conv2d_params.inc_Xo_T-chnd_T,conv2d_params.num_Xo,conv2d_params.inc_Co_T-chnd_T,conv2d_params.num_Co,conv2d_params.inc_Yo_T-chnd_T);
            }else{
                SETUP_ITR3D(iterator_pout,conv2d_params.inc_Xo-chnd,conv2d_params.num_Xo,conv2d_params.inc_Co-chnd,conv2d_params.num_Co,conv2d_params.inc_Yo-chnd) ;
            }    
        #else
            DECL_ITR2D(iterator_pout)
            if constexpr(out_mode == PARTIAL){
                SETUP_ITR2D(iterator_pout,conv2d_params.inc_Xo_T-chnd_T,conv2d_params.num_Xo,conv2d_params.inc_Co_T-chnd_T);
            }else{
                SETUP_ITR2D(iterator_pout,conv2d_params.inc_Xo-chnd,conv2d_params.num_Xo,conv2d_params.inc_Co-chnd) ;
            }
        #endif
    #else
        DECL_ITR2D(iterator_pout)
        if constexpr(out_mode == PARTIAL){
            SETUP_ITR2D(iterator_pout,conv2d_params.inc_Xo_T-chnd_T, conv2d_params.num_X,conv2d_params.inc_Co_T-chnd_T);
        }else{
            SETUP_ITR2D(iterator_pout,conv2d_params.inc_Xo-chnd,conv2d_params.num_X,conv2d_params.inc_Co-chnd) ;
        }    
    #endif   
     
    
    int step_align_check=(conv2d_params.step_align!=0)<<(conv2d_params.step_align+1); 

    for (unsigned j=0; j<conv2d_params.outer_g; j++)
    //chess_prepare_for_pipelining
    //chess_loop_range(4,) // saw slightly worse performance in L6 resnet18v1bs2 after enabling these lines, and PM increased by 1 kB
    {
        v64int8 Sbuff;

        mmul Cbuff0( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T); 
        mmul Cbuff1( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_1-chnd_T); 
        mmul Cbuff2( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T); 
        mmul Cbuff3( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_2-chnd_T); 
        mmul Cbuff4( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T); 
        mmul Cbuff5( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_1-chnd_T); 
        mmul Cbuff6( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T); 
        mmul Cbuff7( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_3-chnd_T); 

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for (int i=0; i<conv2d_params.inner_g; i++){
            unsigned frac = (cnt_frac & 31) + 33;
            //cnt_frac = (cnt_frac==conv2d_params.num_Kx*((conv2d_params.step_align!=0)<<(conv2d_params.step_align+1))+31 ? 31 : cnt_frac+((conv2d_params.step_align!=0)<<(conv2d_params.step_align+1)));        
            cnt_frac = (cnt_frac==conv2d_params.num_Kx*step_align_check+31) ? 31 : cnt_frac+step_align_check;        
        
            vector<uint8_t, 64>  Sbuff0, Sbuff1, Sbuff2, Sbuff3, Xbuff0, Xbuff1, Xbuff2, Xbuff3;

#ifdef __X86SIM__               
            Sbuff0.insert( 0,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            Sbuff0.insert( 1,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A_1);            
            Sbuff1.insert( 0,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            
            #ifdef CONV2D_S2_2x8_SUPPORT
                Sbuff1.insert( 1,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A_2x8_mode);
            #else
                Sbuff1.insert( 1,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A_1);
            #endif
                        
            Sbuff2.insert( 0,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            Sbuff2.insert( 1,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A_1);            
            Sbuff3.insert( 0,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            Sbuff3.insert( 1,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));              
                    
#else 
            Sbuff0.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            Sbuff0.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A_1);            
            Sbuff1.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            
            #ifdef CONV2D_S2_2x8_SUPPORT
                Sbuff1.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A_2x8_mode);
            #else
                Sbuff1.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A_1);
            #endif
                        
            Sbuff2.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            Sbuff2.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A_1);            
            Sbuff3.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            Sbuff3.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in));              
#endif
                    
            Xbuff.x0 = shiftx(Xbuff.x0, Sbuff0, conv2d_params.step_align, frac);
            Xbuff.x1 = shiftx(Xbuff.x1, Sbuff1, conv2d_params.step_align, frac);
            Xbuff.x2 = shiftx(Xbuff.x2, Sbuff2, conv2d_params.step_align, frac);
            Xbuff.x3 = shiftx(Xbuff.x3, Sbuff3, conv2d_params.step_align, frac);
            
            // TODO: CR-1103776 
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,conv2d_params)

            Xbuff0 = interleave(Xbuff.x0, unu8, conv2d_params.shfl_0);
            Xbuff1 = interleave(Xbuff.x1, unu8, conv2d_params.shfl_0);
            Xbuff2 = interleave(Xbuff.x2, unu8, conv2d_params.shfl_0);
            Xbuff3 = interleave(Xbuff.x3, unu8, conv2d_params.shfl_0);

            vec_Xbuff0 = Xbuff0.cast_to<dtype_ifm>(). template extract<32>(0); 
            vec_Xbuff1 = Xbuff1.cast_to<dtype_ifm>(). template extract<32>(0); 
            vec_Xbuff2 = Xbuff2.cast_to<dtype_ifm>(). template extract<32>(0); 
            vec_Xbuff3 = Xbuff3.cast_to<dtype_ifm>(). template extract<32>(0);

            // Load weights -- two matrices 8*8
            LOAD_WEIGHTS(p_w,Ybuff0,32) ;
            LOAD_WEIGHTS(p_w,Ybuff1,32) ;

            #ifdef USE_RTP_SIGN
            int sign_x=conv2d_params.ifm_sign;	
            Cbuff0.mac(op_sign(vec_Xbuff0,sign_x), op_sign(Ybuff0,1));                       
            Cbuff1.mac(op_sign(vec_Xbuff1,sign_x), op_sign(Ybuff0,1));                       
            Cbuff2.mac(op_sign(vec_Xbuff2,sign_x), op_sign(Ybuff0,1));                       
            Cbuff3.mac(op_sign(vec_Xbuff3,sign_x), op_sign(Ybuff0,1));                       
            Cbuff4.mac(op_sign(vec_Xbuff0,sign_x), op_sign(Ybuff1,1));                       
            Cbuff5.mac(op_sign(vec_Xbuff1,sign_x), op_sign(Ybuff1,1));                       
            Cbuff6.mac(op_sign(vec_Xbuff2,sign_x), op_sign(Ybuff1,1));                       
            Cbuff7.mac(op_sign(vec_Xbuff3,sign_x), op_sign(Ybuff1,1));  
            #else
            Cbuff0.mac(vec_Xbuff0, Ybuff0);                       
            Cbuff1.mac(vec_Xbuff1, Ybuff0);                       
            Cbuff2.mac(vec_Xbuff2, Ybuff0);                       
            Cbuff3.mac(vec_Xbuff3, Ybuff0);                       
            Cbuff4.mac(vec_Xbuff0, Ybuff1);                       
            Cbuff5.mac(vec_Xbuff1, Ybuff1);                       
            Cbuff6.mac(vec_Xbuff2, Ybuff1);                       
            Cbuff7.mac(vec_Xbuff3, Ybuff1);  
            #endif                     
                        
        }

        p_in     = INCR_ITR3D_PARAM(p_in, iterator_outer,conv2d_params)
        p_w      = INCR_ITR3D_PARAM(p_w, iterator_weights,conv2d_params)
        p_init16 = INCR_ITR3D(p_init16, iterator_psum)

        // Write out the final outputs
        if constexpr(out_mode == FULL){
            if constexpr(act_type==LINEAR || act_type==RELU){
                 #ifdef USE_RTP_ACT
                 int out_sign=conv2d_params.out_sign;
                 #else
                 int out_sign = 1-act_type; // Note enum for act types are: 0,1,2 -> linear, relu, lrelu
                 #endif
                 store_v(p_out, Cbuff0.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32); 
                 store_v(p_out, Cbuff1.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
                 store_v(p_out, Cbuff2.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff3.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_1-chnd);
                 store_v(p_out, Cbuff4.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff5.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
                 store_v(p_out, Cbuff6.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff7.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out));

            } else {
                // Code/Macro for Leaky reLU
                       
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff0,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff1,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff2,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff3,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, conv2d_params.inc_S_1-chnd);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff4,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff5,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff6,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff7,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                        
            }
        }
        else {
        // Store partial sum
            store_v((int16 DM_BANK(c) *) p_out, Cbuff0.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(c) *)byte_incr((int16*)p_out, 64); 
            store_v((int16 DM_BANK(c) *) p_out, Cbuff1.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(c) *)byte_incr((int16*)p_out, conv2d_params.inc_ST_0-chnd_T);
            store_v((int16 DM_BANK(c) *) p_out, Cbuff2.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(c) *)byte_incr((int16*)p_out, 64);
            store_v((int16 DM_BANK(c) *) p_out, Cbuff3.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(c) *)byte_incr((int16*)p_out, conv2d_params.inc_ST_1-chnd_T);
            store_v((int16 DM_BANK(c) *) p_out, Cbuff4.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(c) *)byte_incr((int16*)p_out, 64);
            store_v((int16 DM_BANK(c) *) p_out, Cbuff5.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(c) *)byte_incr((int16*)p_out, conv2d_params.inc_ST_0-chnd_T);
            store_v((int16 DM_BANK(c) *) p_out, Cbuff6.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(c) *)byte_incr((int16*)p_out, 64);
            store_v((int16 DM_BANK(c) *) p_out, Cbuff7.template to_vector<int16>(conv2d_params.shift_psum_out));   
        }         
        
        #ifdef CONV2D_S2_2x8_SUPPORT
            #ifdef CONV2D_2x8_WIDTH_SUPPORT
                p_out = INCR_ITR3D(p_out,iterator_pout)
            #else        
                p_out = INCR_ITR2D(p_out,iterator_pout)
            #endif
        #else
            p_out = INCR_ITR2D(p_out,iterator_pout)
        #endif
    }

    if constexpr(out_mode == FULL){
         #if RELU_INT8_ENABLE == 1
            #ifdef USE_RTP_ACT
            	relu_int8_post_process(output,conv2d_params.out_sign,conv2d_params.ofm_len);
            #else
                int act = 1 - act_type;
                relu_int8_post_process(output,act,conv2d_params.ofm_len);   
            #endif
         #endif
    }
}

#endif

#endif //End of conv2d stride-2 variant 

#if USE_OPMODE_1 == 1
template <uint8_t batch_size, act_t act_type, typename dtype_ifm, typename dtype_wts, typename dtype_ofm> 
void conv2d_gran8
(
        int * __restrict input,
        int * __restrict weights,
        int * __restrict acc_in,
        int * __restrict output,
        conv2d_params_t            &conv2d_params
) 
chess_extra_options("mist2 +Omodo")
{

    int16      OFM_DM_BANK  * p_sum   = ((int16      OFM_DM_BANK *) output + (conv2d_params.psum_buff_offset>>1));
    dtype_ifm IFM_DM_BANK  * p_in    = (dtype_ifm  IFM_DM_BANK *) input;
    dtype_wts  IFM_DM_BANK   * p_w     = (dtype_wts   IFM_DM_BANK  *) weights;


    vector<dtype_ifm, 32> vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3, temp_vec0, temp_vec1,temp_vec2;
    int16      OFM_DM_BANK * p_init16 = (int16 OFM_DM_BANK *) acc_in;
    dtype_ofm OFM_DM_BANK * p_out    = (conv2d_params.psum_out==0)?(dtype_ofm OFM_DM_BANK *)output : (dtype_ofm OFM_DM_BANK *)p_sum;

    using mmul = mmul<DIM_M, DIM_N, DIM_K, dtype_ifm, dtype_wts>;

    vector<dtype_wts, 64>  Ybuff0, Ybuff1;

    Vecs4_ui8 Xbuff;
    //undef(Xbuff);

    // Following counters are needed for ADD_2D/3D_BYTE intrinsics
    // SETUP_ITR2D_COUNTERS(iterator_name) / SETUP_ITR3D_COUNTERS(iterator_name)
    // type 1: 2D iterator, 2: 3d Iterator
    // e.g. SETUP_ITR3D_COUNTERS(iterator_inner)
    // Resulting count variable is : iterator_inner_cnt0, iterator_inner_cnt_1
    SETUP_ITR3D_COUNTERS(iterator_inner)
    SETUP_ITR3D_COUNTERS(iterator_outer)

    unsigned frac;
    int chnd   = 32;
    int chnd_T = 64;

    if constexpr(batch_size < 4) {
        p_in += 31 ;
    }
    int    acc_init_shift = (int) conv2d_params.shift_bias_init ;   
    int    incr = 3*conv2d_params.inc_A ;   
    int    incr1 = 8 ; //this is : 3*incr_A-3*incr_A+8   
    dtype_ifm IFM_DM_BANK *p_temp=p_in;
    
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (unsigned j=0; j< conv2d_params.outer_g; j++){

        mmul Cbuff0( load_v<32>( p_init16 ), acc_init_shift ); 
        mmul Cbuff1( load_v<32>( p_init16 ), acc_init_shift ); 
        mmul Cbuff2( load_v<32>( p_init16 ), acc_init_shift ); 
        mmul Cbuff3( load_v<32>( p_init16 ), acc_init_shift ); 

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(3,) 
        for (int i=0; i<3; i++){
            // iter0 -----------------------------------------------------------------------------------
            vector<uint8_t, 64>  Sbuff0, Sbuff1,Xbuff0,Xbuff1,Xbuff2,Xbuff3;
            // pin is now at p_in_start 
            // Load IFM data, two 4x8 matrices plus additional adjacent data needed for shift alignment
            Xbuff0.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            Xbuff1.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            Xbuff2.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            Xbuff3.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in)); 
                
            p_in = byte_incr(p_in, incr1); 
            vec_Xbuff0 = Xbuff0.cast_to<dtype_ifm>(). template extract<32>(0); 
            vec_Xbuff1 = Xbuff1.cast_to<dtype_ifm>(). template extract<32>(0); 
            vec_Xbuff2 = Xbuff2.cast_to<dtype_ifm>(). template extract<32>(0); 
            vec_Xbuff3 = Xbuff3.cast_to<dtype_ifm>(). template extract<32>(0);

            // Load weights -- two matrices 8*8
            LOAD_WEIGHTS(p_w,Ybuff0,32) ;

            Cbuff0.mac(vec_Xbuff0, Ybuff0);                       
            Cbuff1.mac(vec_Xbuff1, Ybuff0);                       
            Cbuff2.mac(vec_Xbuff2, Ybuff0);                       
            Cbuff3.mac(vec_Xbuff3, Ybuff0);                       
            frac = 40;
            // iter1 -----------------------------------------------------------------------------------
            // Load IFM data, two 4x8 matrices plus additional adjacent data needed for shift alignment
            //p_in is now at p_in_start+8
            Sbuff0.insert( 0,  vec_Xbuff1);  
            Sbuff0.insert( 1,  vec_Xbuff2); 
            Sbuff1.insert( 0,  vec_Xbuff3); 
            Sbuff1.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in)); 
            
            Xbuff.x0 = shiftx(Xbuff0, Sbuff0, conv2d_params.step_align, frac);
            Xbuff.x2 = shiftx(Xbuff2, Sbuff1, conv2d_params.step_align, frac);
                
            Xbuff0 = Xbuff.x0 ;
            Xbuff1 = interleave(Xbuff0,   unu8, T256_2x2_hi);
            Xbuff2 = Xbuff.x2;
            Xbuff3 = interleave(Xbuff2,   unu8, T256_2x2_hi);


            vec_Xbuff0 = Xbuff0.cast_to<dtype_ifm>(). template extract<32>(0); 
            vec_Xbuff1 = Xbuff1.cast_to<dtype_ifm>(). template extract<32>(0); 
            vec_Xbuff2 = Xbuff2.cast_to<dtype_ifm>(). template extract<32>(0); 
            vec_Xbuff3 = Xbuff3.cast_to<dtype_ifm>(). template extract<32>(0);

            // Load weights -- two matrices 8*8
            LOAD_WEIGHTS(p_w,Ybuff0,32) ;

            Cbuff0.mac(vec_Xbuff0, Ybuff0);                       
            Cbuff1.mac(vec_Xbuff1, Ybuff0);                       
            Cbuff2.mac(vec_Xbuff2, Ybuff0);                       
            Cbuff3.mac(vec_Xbuff3, Ybuff0);                       
            frac = 48; 
            // iter2 -----------------------------------------------------------------------------------
            // Load IFM data, two 4x8 matrices plus additional adjacent data needed for shift alignment
            p_in = byte_incr(p_in, incr1);

            Xbuff.x0 = shiftx(Xbuff.x0, Sbuff0, conv2d_params.step_align, frac);
            Xbuff.x2 = shiftx(Xbuff.x2, Sbuff1, conv2d_params.step_align, frac);
                
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,conv2d_params)
            Xbuff0 = Xbuff.x0 ;
            Xbuff1 = interleave(Xbuff0,   unu8, T256_2x2_hi);
            Xbuff2 = Xbuff.x2;
            Xbuff3 = interleave(Xbuff2,   unu8, T256_2x2_hi);

            vec_Xbuff0 = Xbuff0.cast_to<dtype_ifm>(). template extract<32>(0); 
            vec_Xbuff1 = Xbuff1.cast_to<dtype_ifm>(). template extract<32>(0); 
            vec_Xbuff2 = Xbuff2.cast_to<dtype_ifm>(). template extract<32>(0); 
            vec_Xbuff3 = Xbuff3.cast_to<dtype_ifm>(). template extract<32>(0);

            // Load weights -- two matrices 8*8
            LOAD_WEIGHTS(p_w,Ybuff0,32) ;

            Cbuff0.mac(vec_Xbuff0, Ybuff0);                       
            Cbuff1.mac(vec_Xbuff1, Ybuff0);                       
            Cbuff2.mac(vec_Xbuff2, Ybuff0);                       
            Cbuff3.mac(vec_Xbuff3, Ybuff0);                       
        }

        p_in     = INCR_ITR3D_PARAM(p_in, iterator_outer,conv2d_params)
        p_w     = (dtype_wts IFM_DM_BANK *) weights;

        // Write out the final outputs
            if constexpr(act_type==LINEAR || act_type==RELU){
                 store_v(p_out, Cbuff0.template to_vector<dtype_ofm>(conv2d_params.shift_out)); p_out = byte_incr(p_out, 32); 
                 store_v(p_out, Cbuff1.template to_vector<dtype_ofm>(conv2d_params.shift_out)); p_out = byte_incr(p_out, 32); 
                 store_v(p_out, Cbuff2.template to_vector<dtype_ofm>(conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff3.template to_vector<dtype_ofm>(conv2d_params.shift_out)); p_out = byte_incr(p_out, 32); 
            } else {
                // Code/Macro for Leaky reLU
                       
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff0,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff1,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff2,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff3,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);
                
                        
            }
    }
}
#endif // USE_OPMODE_1==1
#endif


#if __AIE_ARCH__ == 21 //AIE2p conv2d_generic

#if defined(CONV2D_OPT1) && defined(STRIDE2_OPT) && defined(CONV2D_S2_OPT1)

template <uint8_t batch_size, act_t act_type, out_mode_t out_mode, typename dtype_ifm, typename dtype_wts, typename dtype_ofm> 
INLINE void conv2d_generic
(
        int * __restrict input,
        int * __restrict weights,
        int * __restrict psum_0,
        int * __restrict psum_1,
        int * __restrict output,
        conv2d_params_t            &conv2d_params
) {
#ifndef __X86SIM__
    v64int8   IFM_DM_BANK   * __restrict p_in      = (v64int8  IFM_DM_BANK   * __restrict) input;
#else
    v64int8   IFM_DM_BANK   *            p_in      = (v64int8  IFM_DM_BANK   * __restrict) input;
#endif
    v64int8   WT_DM_BANK_2  * __restrict p_w       = (v64int8  WT_DM_BANK_2  * __restrict) weights;  
    v8int16   WT_DM_BANK_2  * __restrict p_bias    = (v8int16  WT_DM_BANK_2  * __restrict) ((int *)p_w-conv2d_params.hdr_len+LAYER_PARAM_WORDS);    
    v64int8   OFM_DM_BANK   * __restrict p_out     = (v64int8  OFM_DM_BANK   * __restrict) output;


    v32int16   PSUM_0_DM_BANK * __restrict p_psum_in_0    = (v32int16 PSUM_0_DM_BANK * __restrict) psum_0;    
    v32int16   PSUM_1_DM_BANK * __restrict p_psum_in_1    = (v32int16 PSUM_1_DM_BANK * __restrict) psum_1;

    v32int16   PSUM_0_DM_BANK * __restrict p_psum_out_0   = (v32int16 PSUM_0_DM_BANK * __restrict) psum_0;    
    v32int16   PSUM_1_DM_BANK * __restrict p_psum_out_1   = (v32int16 PSUM_1_DM_BANK * __restrict) psum_1; 
        
    using mmul = mmul<8, 8, 8, int8, int8>;    

    // Following counters are needed for ADD_2D/3D_BYTE intrinsics
    // SETUP_ITR2D_COUNTERS(iterator_name) / SETUP_ITR3D_COUNTERS(iterator_name)
    // type 1: 2D iterator, 2: 3d Iterator
    // e.g. SETUP_ITR3D_COUNTERS(iterator_inner)
    // Resulting count variable is : iterator_inner_cnt0, iterator_inner_cnt_1
    SETUP_ITR3D_COUNTERS(iterator_inner)
    SETUP_ITR3D_COUNTERS(iterator_outer)
    SETUP_ITR3D_COUNTERS(iterator_weights)
    SETUP_ITR2D_COUNTERS(iterator_pout)

    DECL_ITR2D(iterator_pout) 
    SETUP_ITR2D(iterator_pout,conv2d_params.inc_Xo, conv2d_params.num_X, conv2d_params.inc_Co) ;    
    
    fifo_state_t p_in_fifo;
    p_in_fifo.pos = 0;    
    
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (unsigned j=0; j<conv2d_params.outer_g; j++)
    {     
        v64acc32 bias_0, bias_1, acc_0, acc_1;        
        v64int8  Sbuff0, Ybuff0, Ybuff1;                       
        
        v32uint16 bias_shift = (v32uint16)broadcast_s32((1-conv2d_params.psum_in) << conv2d_params.shift_bias_init);
        int32_t bias_incr = 128*iterator_weights_cnt1;             
        
        bias_0 = mul_8x2_2x8(bias_shift, 0, set_v32int16(0,*byte_incr(p_bias,bias_incr)),1);                
        acc_0  = insert(acc_0, 0, sups(*p_psum_in_0, (int)conv2d_params.shift_psum_in, 1)); p_psum_in_0 = byte_incr(p_psum_in_0, 64);
        acc_0  = insert(acc_0, 1, sups(*p_psum_in_0, (int)conv2d_params.shift_psum_in, 1)); p_psum_in_0 = byte_incr(p_psum_in_0, 64);     
        
        bias_1 = mul_8x2_2x8(bias_shift, 0, set_v32int16(0,*byte_incr(p_bias,bias_incr + 64)),1); 
        acc_1  = insert(acc_1, 0, sups(*p_psum_in_1, (int)conv2d_params.shift_psum_in, 1)); p_psum_in_1 = byte_incr(p_psum_in_1, 64);
        acc_1  = insert(acc_1, 1, sups(*p_psum_in_1, (int)conv2d_params.shift_psum_in, 1)); p_psum_in_1 = byte_incr(p_psum_in_1, 64);                             

        Sbuff0 = fifo_ld_popx(p_in, p_in_fifo, 0, 63); //0 is hard coding for N=1
        
        p_in = add_3d_byte(p_in, conv2d_params.iterator_inner_incr2, conv2d_params.iterator_inner_wrap0, iterator_inner_cnt0, conv2d_params.iterator_inner_incr0, conv2d_params.iterator_inner_wrap1, iterator_inner_cnt1, conv2d_params.iterator_inner_incr1);
        
        Sbuff0 = interleave(Sbuff0, Sbuff0, conv2d_params.shfl_1); //dummy..will be removed later            

        Ybuff0 = *p_w; p_w = byte_incr(p_w, 64);        
        Ybuff1 = *p_w; p_w = byte_incr(p_w, 64);            

        int sign_x = conv2d_params.ifm_sign;

        acc_0  = addmac_8x8_8x8_conf(Sbuff0, sign_x, Ybuff0, 1, acc_0, bias_0, (1-conv2d_params.psum_in), 0, 0, 0, 0);
        acc_1  = addmac_8x8_8x8_conf(Sbuff0, sign_x, Ybuff1, 1, acc_1, bias_1, (1-conv2d_params.psum_in), 0, 0, 0, 0);                                        

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(7,) 
        for (int i=1; i<conv2d_params.inner_g; i++)
        {                                
            Sbuff0 = fifo_ld_popx(p_in, p_in_fifo, 0, 63); //0 is hard coding for N=1            
            
            p_in = add_3d_byte(p_in, conv2d_params.iterator_inner_incr2, conv2d_params.iterator_inner_wrap0, iterator_inner_cnt0, conv2d_params.iterator_inner_incr0, conv2d_params.iterator_inner_wrap1, iterator_inner_cnt1, conv2d_params.iterator_inner_incr1);     
            
            Sbuff0 = interleave(Sbuff0, Sbuff0, conv2d_params.shfl_1);                       

            Ybuff0 = *p_w; p_w = byte_incr(p_w, 64);
            Ybuff1 = *p_w; p_w = byte_incr(p_w, 64);            

            int sign_x = conv2d_params.ifm_sign;
            
            acc_0 = mac_8x8_8x8_conf(Sbuff0, sign_x, Ybuff0, 1, acc_0, 0, 0, 0, 0);
            acc_1 = mac_8x8_8x8_conf(Sbuff0, sign_x, Ybuff1, 1, acc_1, 0, 0, 0, 0);                   
        }

        p_in     = INCR_ITR3D_PARAM(p_in, iterator_outer,conv2d_params)
        p_w      = INCR_ITR3D_PARAM(p_w, iterator_weights,conv2d_params)

        // Write out the final outputs
        if constexpr(out_mode == FULL){
            if constexpr(act_type==LINEAR || act_type==RELU){
                 
                int out_sign=conv2d_params.out_sign;

                *p_out = ssrs(acc_0, (int)conv2d_params.shift_out, (int)out_sign); 
                p_out  = byte_incr(p_out, conv2d_params.inc_S_0);

                *p_out = ssrs(acc_1, (int)conv2d_params.shift_out, (int)out_sign);                

            } else {     
                //TODO: LeakyRelu    
            }
            
            p_out = INCR_ITR2D(p_out,iterator_pout)
        }
        else {
            // Store partial sum
            *p_psum_out_0 = lsrs(extract_v32acc32(acc_0, 0), (int)conv2d_params.shift_psum_out, 1);
            p_psum_out_0 = byte_incr(p_psum_out_0, 64);
            *p_psum_out_0 = lsrs(extract_v32acc32(acc_0, 1), (int)conv2d_params.shift_psum_out, 1);
            p_psum_out_0 = byte_incr(p_psum_out_0, 64);       
            
            *p_psum_out_1 = lsrs(extract_v32acc32(acc_1, 0), (int)conv2d_params.shift_psum_out, 1);
            p_psum_out_1 = byte_incr(p_psum_out_1, 64);
            *p_psum_out_1 = lsrs(extract_v32acc32(acc_1, 1), (int)conv2d_params.shift_psum_out, 1);
            p_psum_out_1 = byte_incr(p_psum_out_1, 64);           
  
        }         
        
    }

    if constexpr(out_mode == FULL){
         #if RELU_INT8_ENABLE == 1
            #ifdef USE_RTP_ACT
            	relu_int8_post_process(output,conv2d_params.out_sign,conv2d_params.ofm_len);
            #else
                int act = 1 - act_type;
                relu_int8_post_process(output,act,conv2d_params.ofm_len);   
            #endif
         #endif
    }
}

template <uint8_t batch_size, act_t act_type, out_mode_t out_mode, typename dtype_ifm, typename dtype_wts, typename dtype_ofm> 
INLINE void conv2d_generic_stride2_opt
(
        int * __restrict input,
        int * __restrict weights,
        int * __restrict psum_0,
        int * __restrict psum_1,
        int * __restrict output,
        conv2d_params_t            &conv2d_params
) {
#ifndef __X86SIM__
    v64int8   IFM_DM_BANK   * __restrict p_in      = (v64int8  IFM_DM_BANK   * __restrict) input;
#else
    v64int8   IFM_DM_BANK   *            p_in      = (v64int8  IFM_DM_BANK   * __restrict) input;
#endif
    v64int8   WT_DM_BANK_2  * __restrict p_w       = (v64int8  WT_DM_BANK_2  * __restrict) weights;  
    v8int16   WT_DM_BANK_2  * __restrict p_bias    = (v8int16  WT_DM_BANK_2  * __restrict) ((int *)p_w-conv2d_params.hdr_len+LAYER_PARAM_WORDS);    
    v64int8   OFM_DM_BANK   * __restrict p_out     = (v64int8  OFM_DM_BANK   * __restrict) output;


    v32int16   PSUM_0_DM_BANK * __restrict p_psum_in_0    = (v32int16 PSUM_0_DM_BANK * __restrict) psum_0;    
    v32int16   PSUM_1_DM_BANK * __restrict p_psum_in_1    = (v32int16 PSUM_1_DM_BANK * __restrict) psum_1;

    v32int16   PSUM_0_DM_BANK * __restrict p_psum_out_0   = (v32int16 PSUM_0_DM_BANK * __restrict) psum_0;    
    v32int16   PSUM_1_DM_BANK * __restrict p_psum_out_1   = (v32int16 PSUM_1_DM_BANK * __restrict) psum_1; 
        
    using mmul = mmul<8, 8, 8, int8, int8>;    

    // Following counters are needed for ADD_2D/3D_BYTE intrinsics
    // SETUP_ITR2D_COUNTERS(iterator_name) / SETUP_ITR3D_COUNTERS(iterator_name)
    // type 1: 2D iterator, 2: 3d Iterator
    // e.g. SETUP_ITR3D_COUNTERS(iterator_inner)
    // Resulting count variable is : iterator_inner_cnt0, iterator_inner_cnt_1
    SETUP_ITR3D_COUNTERS(iterator_inner)
    SETUP_ITR3D_COUNTERS(iterator_outer)
    SETUP_ITR3D_COUNTERS(iterator_weights)
    SETUP_ITR2D_COUNTERS(iterator_pout)

    DECL_ITR2D(iterator_pout) 
    SETUP_ITR2D(iterator_pout,conv2d_params.inc_Xo, conv2d_params.num_X, conv2d_params.inc_Co) ;    
    
    fifo_state_t p_in_fifo;
    p_in_fifo.pos = 0;    
    
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (unsigned j=0; j<conv2d_params.outer_g; j++)
    {     
        v64acc32 bias_0, bias_1, acc_0, acc_1;        
        v64int8  Sbuff0, Sbuff1, Ybuff0, Ybuff1;                       
        
        v32uint16 bias_shift = (v32uint16)broadcast_s32((1-conv2d_params.psum_in) << conv2d_params.shift_bias_init);
        int32_t bias_incr = 128*iterator_weights_cnt1;             
        
        bias_0 = mul_8x2_2x8(bias_shift, 0, set_v32int16(0,*byte_incr(p_bias,bias_incr)),1);                
        acc_0  = insert(acc_0, 0, sups(*p_psum_in_0, (int)conv2d_params.shift_psum_in, 1)); p_psum_in_0 = byte_incr(p_psum_in_0, 64);
        acc_0  = insert(acc_0, 1, sups(*p_psum_in_0, (int)conv2d_params.shift_psum_in, 1)); p_psum_in_0 = byte_incr(p_psum_in_0, 64);     
        
        bias_1 = mul_8x2_2x8(bias_shift, 0, set_v32int16(0,*byte_incr(p_bias,bias_incr + 64)),1); 
        acc_1  = insert(acc_1, 0, sups(*p_psum_in_1, (int)conv2d_params.shift_psum_in, 1)); p_psum_in_1 = byte_incr(p_psum_in_1, 64);
        acc_1  = insert(acc_1, 1, sups(*p_psum_in_1, (int)conv2d_params.shift_psum_in, 1)); p_psum_in_1 = byte_incr(p_psum_in_1, 64);                             

        Sbuff0 = fifo_ld_popx(p_in, p_in_fifo, 0, 63); //0 is hard coding for N=1
        Sbuff1 = fifo_ld_pop_3d_byte(p_in, p_in_fifo, conv2d_params.iterator_inner_incr2, conv2d_params.iterator_inner_wrap0, iterator_inner_cnt0, conv2d_params.iterator_inner_incr0, conv2d_params.iterator_inner_wrap1, iterator_inner_cnt1, conv2d_params.iterator_inner_incr1);
        Sbuff0 = interleave(Sbuff0, Sbuff1, conv2d_params.shfl_1);                       

        Ybuff0 = *p_w; p_w = byte_incr(p_w, 64);        
        Ybuff1 = *p_w; p_w = byte_incr(p_w, 64);            

        int sign_x = conv2d_params.ifm_sign;

        acc_0  = addmac_8x8_8x8_conf(Sbuff0, sign_x, Ybuff0, 1, acc_0, bias_0, (1-conv2d_params.psum_in), 0, 0, 0, 0);
        acc_1  = addmac_8x8_8x8_conf(Sbuff0, sign_x, Ybuff1, 1, acc_1, bias_1, (1-conv2d_params.psum_in), 0, 0, 0, 0);                                        

        AIE_PREPARE_FOR_PIPELINING
        AIE_LOOP_RANGE(7,) 
        for (int i=1; i<conv2d_params.inner_g; i++)
        {                                
            Sbuff0 = fifo_ld_popx(p_in, p_in_fifo, 0, 63); //0 is hard coding for N=1            
            Sbuff1 = fifo_ld_pop_3d_byte(p_in, p_in_fifo, conv2d_params.iterator_inner_incr2, conv2d_params.iterator_inner_wrap0, iterator_inner_cnt0, conv2d_params.iterator_inner_incr0, conv2d_params.iterator_inner_wrap1, iterator_inner_cnt1, conv2d_params.iterator_inner_incr1);     
            Sbuff0 = interleave(Sbuff0, Sbuff1, conv2d_params.shfl_1);                       

            Ybuff0 = *p_w; p_w = byte_incr(p_w, 64);
            Ybuff1 = *p_w; p_w = byte_incr(p_w, 64);            

            int sign_x = conv2d_params.ifm_sign;
            
            acc_0 = mac_8x8_8x8_conf(Sbuff0, sign_x, Ybuff0, 1, acc_0, 0, 0, 0, 0);
            acc_1 = mac_8x8_8x8_conf(Sbuff0, sign_x, Ybuff1, 1, acc_1, 0, 0, 0, 0);                   
        }

        p_in     = INCR_ITR3D_PARAM(p_in, iterator_outer,conv2d_params)
        p_w      = INCR_ITR3D_PARAM(p_w, iterator_weights,conv2d_params)

        // Write out the final outputs
        if constexpr(out_mode == FULL){
            if constexpr(act_type==LINEAR || act_type==RELU){
                 
                int out_sign=conv2d_params.out_sign;

                *p_out = ssrs(acc_0, (int)conv2d_params.shift_out, (int)out_sign); 
                p_out  = byte_incr(p_out, conv2d_params.inc_S_0);

                *p_out = ssrs(acc_1, (int)conv2d_params.shift_out, (int)out_sign);                

            } else {     
                //TODO: LeakyRelu    
            }
            
            p_out = INCR_ITR2D(p_out,iterator_pout)
        }
        else {
            // Store partial sum
            *p_psum_out_0 = lsrs(extract_v32acc32(acc_0, 0), (int)conv2d_params.shift_psum_out, 1);
            p_psum_out_0 = byte_incr(p_psum_out_0, 64);
            *p_psum_out_0 = lsrs(extract_v32acc32(acc_0, 1), (int)conv2d_params.shift_psum_out, 1);
            p_psum_out_0 = byte_incr(p_psum_out_0, 64);       
            
            *p_psum_out_1 = lsrs(extract_v32acc32(acc_1, 0), (int)conv2d_params.shift_psum_out, 1);
            p_psum_out_1 = byte_incr(p_psum_out_1, 64);
            *p_psum_out_1 = lsrs(extract_v32acc32(acc_1, 1), (int)conv2d_params.shift_psum_out, 1);
            p_psum_out_1 = byte_incr(p_psum_out_1, 64);           
  
        }         
        
    }

    if constexpr(out_mode == FULL){
         #if RELU_INT8_ENABLE == 1
            #ifdef USE_RTP_ACT
            	relu_int8_post_process(output,conv2d_params.out_sign,conv2d_params.ofm_len);
            #else
                int act = 1 - act_type;
                relu_int8_post_process(output,act,conv2d_params.ofm_len);   
            #endif
         #endif
    }
}


template <uint8_t batch_size, act_t act_type, out_mode_t out_mode, typename dtype_ifm, typename dtype_wts, typename dtype_ofm> 
NOINLINE void conv2d_1x16x16_opt
(
        int * __restrict input,
        int * __restrict weights,
        int * __restrict psum_0,
        int * __restrict psum_1,
        int * __restrict output,
        conv2d_params_t            &conv2d_params
) {
#ifndef __X86SIM__
    v64int8   IFM_DM_BANK   * __restrict p_in      = (v64int8  IFM_DM_BANK   * __restrict) input;
#else
    v64int8   IFM_DM_BANK   *            p_in      = (v64int8  IFM_DM_BANK   * __restrict) input;
#endif
    v64int8   WT_DM_BANK_2  * __restrict p_w       = (v64int8  WT_DM_BANK_2  * __restrict) weights;  
    v8int16   WT_DM_BANK_2  * __restrict p_bias    = (v8int16  WT_DM_BANK_2  * __restrict) ((int *)p_w-conv2d_params.hdr_len+LAYER_PARAM_WORDS);    
    v64int8   OFM_DM_BANK   * __restrict p_out     = (v64int8  OFM_DM_BANK   * __restrict) output;
        
    using mmul = mmul<8, 8, 8, int8, int8>;    

    // Following counters are needed for ADD_2D/3D_BYTE intrinsics
    // SETUP_ITR2D_COUNTERS(iterator_name) / SETUP_ITR3D_COUNTERS(iterator_name)
    // type 1: 2D iterator, 2: 3d Iterator
    // e.g. SETUP_ITR3D_COUNTERS(iterator_inner)
    // Resulting count variable is : iterator_inner_cnt0, iterator_inner_cnt_1
    SETUP_ITR3D_COUNTERS(iterator_inner)
    SETUP_ITR3D_COUNTERS(iterator_outer)
    SETUP_ITR3D_COUNTERS(iterator_weights)
    SETUP_ITR2D_COUNTERS(iterator_pout)

    DECL_ITR2D(iterator_pout) 
    SETUP_ITR2D(iterator_pout,conv2d_params.inc_Xo, conv2d_params.num_X, conv2d_params.inc_Co) ;    
    
    fifo_state_t p_in_fifo;
    p_in_fifo.pos = 0;    
    
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (unsigned j=0; j<conv2d_params.outer_g; j++)
    {     
        v64acc32 acc_0, acc_1, acc_2, acc_3;
        v64acc32 bias_0, bias_1;        
        v64int8  Sbuff0, Sbuff1, Sbuff2, Sbuff3;
        v64int8  Vbuff0, Vbuff1; 
        v64int8  Ybuff0, Ybuff1;                       
        
        v32uint16 bias_shift = (v32uint16)broadcast_s32((1-conv2d_params.psum_in) << conv2d_params.shift_bias_init);
        int32_t bias_incr = 128*iterator_weights_cnt1;             
        
        bias_0 = mul_8x2_2x8(bias_shift, 0, set_v32int16(0,*byte_incr(p_bias,bias_incr)),1);             
        bias_1 = mul_8x2_2x8(bias_shift, 0, set_v32int16(0,*byte_incr(p_bias,bias_incr + 64)),1); 

        Sbuff0 = fifo_ld_popx(p_in, p_in_fifo, 0, 63); //0 is hard coding for N=1
        Sbuff1 = fifo_ld_pop(p_in, p_in_fifo);
        Sbuff2 = fifo_ld_pop(p_in, p_in_fifo);
        Sbuff3 = fifo_ld_pop_3d_byte(p_in, p_in_fifo, conv2d_params.iterator_inner_incr2, conv2d_params.iterator_inner_wrap0, iterator_inner_cnt0, conv2d_params.iterator_inner_incr0, conv2d_params.iterator_inner_wrap1, iterator_inner_cnt1, conv2d_params.iterator_inner_incr1);
        
        Vbuff0 = interleave(Sbuff0, Sbuff1, conv2d_params.shfl_1);
        Vbuff1 = interleave(Sbuff2, Sbuff3, conv2d_params.shfl_1);  
        Vbuff1 = interleave(Sbuff1, Vbuff1, conv2d_params.shfl_2);                      

        Ybuff0 = *p_w; p_w = byte_incr(p_w, 64);        
        Ybuff1 = *p_w; p_w = byte_incr(p_w, 64);            

        int sign_x = conv2d_params.ifm_sign;
                    
        acc_0 = addmac_8x8_8x8_conf(Vbuff0, sign_x, Ybuff0, 1, acc_0, bias_0, 1, 0, 0, 0, 0);
        acc_1 = addmac_8x8_8x8_conf(Vbuff1, sign_x, Ybuff0, 1, acc_1, bias_0, 1, 0, 0, 0, 0); 
        acc_2 = addmac_8x8_8x8_conf(Vbuff0, sign_x, Ybuff1, 1, acc_2, bias_1, 1, 0, 0, 0, 0);
        acc_3 = addmac_8x8_8x8_conf(Vbuff1, sign_x, Ybuff1, 1, acc_3, bias_1, 1, 0, 0, 0, 0);       

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for (int i=1; i<conv2d_params.inner_g; i++)
        {                                
            Sbuff0 = fifo_ld_popx(p_in, p_in_fifo, 0, 63); //0 is hard coding for N=1
            Sbuff1 = fifo_ld_pop(p_in, p_in_fifo);
            Sbuff2 = fifo_ld_pop(p_in, p_in_fifo);
            Sbuff3 = fifo_ld_pop_3d_byte(p_in, p_in_fifo, conv2d_params.iterator_inner_incr2, conv2d_params.iterator_inner_wrap0, iterator_inner_cnt0, conv2d_params.iterator_inner_incr0, conv2d_params.iterator_inner_wrap1, iterator_inner_cnt1, conv2d_params.iterator_inner_incr1);
            
            Vbuff0 = interleave(Sbuff0, Sbuff1, conv2d_params.shfl_1);
            Vbuff1 = interleave(Sbuff2, Sbuff3, conv2d_params.shfl_1);  
            Vbuff1 = interleave(Sbuff1, Vbuff1, conv2d_params.shfl_2);                      

            Ybuff0 = *p_w; p_w = byte_incr(p_w, 64);        
            Ybuff1 = *p_w; p_w = byte_incr(p_w, 64);            

            int sign_x = conv2d_params.ifm_sign;
                        
            acc_0 = mac_8x8_8x8_conf(Vbuff0, sign_x, Ybuff0, 1, acc_0, 0, 0, 0, 0);
            acc_1 = mac_8x8_8x8_conf(Vbuff1, sign_x, Ybuff0, 1, acc_1, 0, 0, 0, 0); 
            acc_2 = mac_8x8_8x8_conf(Vbuff0, sign_x, Ybuff1, 1, acc_2, 0, 0, 0, 0);
            acc_3 = mac_8x8_8x8_conf(Vbuff1, sign_x, Ybuff1, 1, acc_3, 0, 0, 0, 0);             
                                     
        }

        p_in     = INCR_ITR3D_PARAM(p_in, iterator_outer,conv2d_params)
        p_w      = INCR_ITR3D_PARAM(p_w, iterator_weights,conv2d_params)

        // Write out the final outputs
        if constexpr(act_type==LINEAR || act_type==RELU){
             
            int out_sign=conv2d_params.out_sign;

            *p_out = ssrs(acc_0, (int)conv2d_params.shift_out, (int)out_sign); p_out  = byte_incr(p_out, 64);
            *p_out = ssrs(acc_1, (int)conv2d_params.shift_out, (int)out_sign); p_out  = byte_incr(p_out, conv2d_params.inc_S_0);

            *p_out = ssrs(acc_2, (int)conv2d_params.shift_out, (int)out_sign); p_out  = byte_incr(p_out, 64);
            *p_out = ssrs(acc_3, (int)conv2d_params.shift_out, (int)out_sign);                

        } else {     
            //TODO: LeakyRelu    
        }
        
        p_out = INCR_ITR2D(p_out,iterator_pout)  
        
    }

     #if RELU_INT8_ENABLE == 1
        #ifdef USE_RTP_ACT
        	relu_int8_post_process(output,conv2d_params.out_sign,conv2d_params.ofm_len);
        #else
            int act = 1 - act_type;
            relu_int8_post_process(output,act,conv2d_params.ofm_len);   
        #endif
     #endif
}

#else //End of CONV2D_OPT1 && STRIDE2_OPT && CONV2D_S2_OPT1 for AIE2p Conv2d. Begin of generic AIE2p Conv2d. 

template <uint8_t batch_size, act_t act_type, out_mode_t out_mode, typename dtype_ifm, typename dtype_wts, typename dtype_ofm> 
INLINE void conv2d_generic
(
        int * __restrict input,
        int * __restrict weights,
        int * __restrict acc_in,
        int * __restrict output,
        conv2d_params_t            &conv2d_params
) {

    int16      OFM_DM_BANK  * p_sum   = ((int16      OFM_DM_BANK *) output + (conv2d_params.psum_buff_offset>>1));
    dtype_ifm IFM_DM_BANK  * p_in    = (dtype_ifm  IFM_DM_BANK *) input; 
    dtype_wts  WT_DM_BANK   * p_w     = (dtype_wts   WT_DM_BANK  *) weights;



    vector<dtype_ifm, 64> vec_Xbuff0, vec_Xbuff1;
    int16      OFM_DM_BANK * p_init16 = (int16 OFM_DM_BANK *) acc_in;
    //dtype_ofm OFM_DM_BANK * p_out    = (conv2d_params.psum_out==0)?(dtype_ofm OFM_DM_BANK *)output : (dtype_ofm OFM_DM_BANK *)p_sum;
    dtype_ofm OFM_DM_BANK * p_out    = (conv2d_params.psum_out==0)?((dtype_ofm OFM_DM_BANK *)output+conv2d_params.ofm_offset_top+conv2d_params.ofm_offset_left*8*batch_size) : (dtype_ofm OFM_DM_BANK *)p_sum;



    using mmul = mmul<8, 8, 8, dtype_ifm, dtype_wts>;

    vector<dtype_wts, 64>  Ybuff0, Ybuff1;

    #define LOAD_WEIGHTS(wts,wbuff,wts_incr) \
    {\
        wbuff.insert(0, load_v<32>(wts));wts+=32;\
        wbuff.insert(1, load_v<32>(wts));wts+=wts_incr;\
    }  
    
    //Vecs4_ui8 Xbuff;
    //undef(Xbuff);

    vector<uint8, 64> Xbuff_x0;
    vector<uint8, 64> Xbuff_x1;
    vector<uint8, 64> Xbuff_x2;
    vector<uint8, 64> Xbuff_x3;

    // Following counters are needed for ADD_2D/3D_BYTE intrinsics
    // SETUP_ITR2D_COUNTERS(iterator_name) / SETUP_ITR3D_COUNTERS(iterator_name)
    // type 1: 2D iterator, 2: 3d Iterator
    // e.g. SETUP_ITR3D_COUNTERS(iterator_inner)
    // Resulting count variable is : iterator_inner_cnt0, iterator_inner_cnt_1
    SETUP_ITR3D_COUNTERS(iterator_inner)
    SETUP_ITR3D_COUNTERS(iterator_outer)
    SETUP_ITR3D_COUNTERS(iterator_weights)
    SETUP_ITR3D_COUNTERS(iterator_psum)
    SETUP_ITR2D_COUNTERS(iterator_pout)

    int chnd   = 32;
    int chnd_T = 64;

    int    inc_ST_0       = (conv2d_params.psum_in ==  0) ? chnd_T                              : 2*chnd_T;
    int    inc_ST_1       = (conv2d_params.psum_in ==  0) ? chnd_T                              : conv2d_params.inc_ST_0;
    int    inc_ST_2       = (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_1+chnd_T      : conv2d_params.inc_ST_1;
    int    inc_ST_3       = (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_2+chnd_T      : chnd_T;    
    
    DECL_ITR3D(iterator_psum)
    SETUP_ITR3D(iterator_psum,
                0,
                (conv2d_params.psum_in ==  0) ? conv2d_params.num_X : 0,
                (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_d1 : conv2d_params.inc_Xo_T-chnd_T,
                (conv2d_params.psum_in ==  0) ? conv2d_params.num_Co : conv2d_params.num_X,
                (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_rev : conv2d_params.inc_Co_T-chnd_T)
        
    int    acc_init_shift = (conv2d_params.psum_in ==  0) ? (int) conv2d_params.shift_bias_init : (int) conv2d_params.shift_psum_in;    
    
    DECL_ITR2D(iterator_pout)
    if constexpr(out_mode == PARTIAL){
        SETUP_ITR2D(iterator_pout,conv2d_params.inc_Xo_T-chnd_T, conv2d_params.num_X,conv2d_params.inc_Co_T-chnd_T);
    }else{
        SETUP_ITR2D(iterator_pout,conv2d_params.inc_Xo-chnd,conv2d_params.num_X,conv2d_params.inc_Co-chnd) ;
    }
    /*
    SETUP_ITR2D(iterator_pout,
               (conv2d_params.psum_out == 0)? conv2d_params.inc_Xo-chnd : conv2d_params.inc_Xo_T-chnd_T,
               conv2d_params.num_X,
               (conv2d_params.psum_out == 0)? conv2d_params.inc_Co-chnd : conv2d_params.inc_Co_T-chnd_T)
    */
    #ifdef BYTE_ALIGNMENT_64
        p_in += 63; //64 byte aligned
    #else
        p_in += 31;   //32 byte aligned
    #endif
    
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (unsigned j=0; j<conv2d_params.outer_g; j++){   
        aie::accum<acc32, 32> acc_0;
        aie::accum<acc32, 32> acc_1;

        acc_0.from_vector(load_v<32>( p_init16 ),acc_init_shift);
        p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T);
        acc_1.from_vector(load_v<32>( p_init16 ),acc_init_shift);
        p_init16 = byte_incr(p_init16, inc_ST_1-chnd_T); 
        mmul Cbuff0(concat(acc_0,acc_1));        

        acc_0.from_vector(load_v<32>( p_init16 ),acc_init_shift);
        p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T); 
        acc_1.from_vector(load_v<32>( p_init16 ),acc_init_shift);
        p_init16 = byte_incr(p_init16, inc_ST_2-chnd_T);
        mmul Cbuff1(concat(acc_0,acc_1));

        acc_0.from_vector(load_v<32>( p_init16 ),acc_init_shift);
        p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T);
        acc_1.from_vector(load_v<32>( p_init16 ),acc_init_shift);
        p_init16 = byte_incr(p_init16, inc_ST_1-chnd_T); 
        mmul Cbuff2(concat(acc_0,acc_1));

        acc_0.from_vector(load_v<32>( p_init16 ),acc_init_shift);
        p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T);
        acc_1.from_vector(load_v<32>( p_init16 ),acc_init_shift);
        p_init16 = byte_incr(p_init16, inc_ST_3-chnd_T); 
        mmul Cbuff3(concat(acc_0,acc_1));                       

        unsigned frac;

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for (int i=0; i<conv2d_params.inner_g; i++){                                
            vector<uint8_t, 64>  Sbuff0, Sbuff1, Sbuff2;

            // Load IFM data, two 4x8 matrices plus additional adjacent data needed for shift alignment 

            int pdel = (long)p_in;                                   
            #ifdef BYTE_ALIGNMENT_64
                frac = ((pdel) & 63) + 1; //64 byte aligned                
                
                Sbuff0 = load_v<64>((uint8 IFM_DM_BANK *) p_in);  
                p_in = byte_incr(p_in, conv2d_params.inc_A);            
                
                Sbuff1 = load_v<64>((uint8 IFM_DM_BANK *) p_in); 
            #else
                frac = ((pdel) & 31) + 33;  //32 byte aligned
#ifdef __X86SIM__
                Sbuff0.insert( 0, load_floor_v<32>((uint8 IFM_DM_BANK *) p_in, 32));  
                p_in = byte_incr(p_in, 32);
                Sbuff0.insert( 1,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  
                p_in = byte_incr(p_in, conv2d_params.inc_A -32);
            
                Sbuff1.insert( 0, load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  
                p_in = byte_incr(p_in, 32);
                Sbuff1.insert( 1,  load_floor_v<32>((uint8 IFM_DM_BANK *) p_in));  
                p_in = byte_incr(p_in, -32);
#else
                Sbuff0.insert( 0, load_v<32>((uint8 IFM_DM_BANK *) p_in));  
                p_in = byte_incr(p_in, 32);
                Sbuff0.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  
                p_in = byte_incr(p_in, conv2d_params.inc_A -32);
            
                Sbuff1.insert( 0, load_v<32>((uint8 IFM_DM_BANK *) p_in));  
                p_in = byte_incr(p_in, 32);
                Sbuff1.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  
                p_in = byte_incr(p_in, -32);
#endif
            #endif
                                           
            Xbuff_x0 = shiftx(Xbuff_x0, Sbuff0, conv2d_params.step_align, frac);
            Xbuff_x2 = shiftx(Xbuff_x2, Sbuff1, conv2d_params.step_align, frac);                        
            
            // TODO: CR-1103776 
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,conv2d_params)
            // conv2d_params.iterator_inner_incr0,conv2d_params.iterator_inner_wrap0,conv2d_params.iterator_inner_cnt0
            // conv2d_params.iterator_inner_incr1,conv2d_params.iterator_inner_wrap1,conv2d_params.iterator_inner_cnt1
            // conv2d_params.iterator_inner_incr2           
            
            vec_Xbuff0 = Xbuff_x0.cast_to<dtype_ifm>();            
            Sbuff2     = interleave(Xbuff_x0, Xbuff_x2, conv2d_params.shfl_1);
            vec_Xbuff1 = Sbuff2.cast_to<dtype_ifm>();                            

            // Load weights -- two matrices 8*8
            Ybuff0 = load_v<64>((int8 IFM_DM_BANK *) p_w);
            p_w = byte_incr(p_w, 64);
            Ybuff1 = load_v<64>((int8 IFM_DM_BANK *) p_w);
            p_w = byte_incr(p_w, 64);            
            

            #ifdef USE_RTP_SIGN
            int sign_x=conv2d_params.ifm_sign;	
            Cbuff0.mac(op_sign(vec_Xbuff0,sign_x), op_sign(Ybuff0,1));                       
            Cbuff1.mac(op_sign(vec_Xbuff1,sign_x), op_sign(Ybuff0,1));                       
            Cbuff2.mac(op_sign(vec_Xbuff0,sign_x), op_sign(Ybuff1,1));                       
            Cbuff3.mac(op_sign(vec_Xbuff1,sign_x), op_sign(Ybuff1,1));                       
            #else
            Cbuff0.mac(vec_Xbuff0, Ybuff0);                       
            Cbuff1.mac(vec_Xbuff1, Ybuff0);                       
            Cbuff2.mac(vec_Xbuff0, Ybuff1);                       
            Cbuff3.mac(vec_Xbuff1, Ybuff1);                       
            #endif                     
        }

        p_in     = INCR_ITR3D_PARAM(p_in, iterator_outer,conv2d_params)
        p_w      = INCR_ITR3D_PARAM(p_w, iterator_weights,conv2d_params)
        p_init16 = INCR_ITR3D(p_init16, iterator_psum)

        // Write out the final outputs
        if constexpr(out_mode == FULL){
            if constexpr(act_type==LINEAR || act_type==RELU){
                 #ifdef USE_RTP_ACT
                 int out_sign=conv2d_params.out_sign;
                 #else
                 int out_sign = 1-act_type; // Note enum for act types are: 0,1,2 -> linear, relu, lrelu
                 #endif
                 
                 store_v(p_out, Cbuff0.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); 
                 p_out = byte_incr(p_out, conv2d_params.inc_S_0);
                 
                 store_v(p_out, Cbuff1.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out));
                 p_out = byte_incr(p_out, conv2d_params.inc_S_1);
                 
                 store_v(p_out, Cbuff2.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out));
                 p_out = byte_incr(p_out, conv2d_params.inc_S_0);
                 
                 store_v(p_out, Cbuff3.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); 
                 p_out = byte_incr(p_out, 32);

            } else {
                // Code/Macro for Leaky reLU
                       
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff0,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, conv2d_params.inc_S_0);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff1,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, conv2d_params.inc_S_1);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff2,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, conv2d_params.inc_S_0);
                
                CONV2D_INT8_LEAKY_RELU_COMPUTE(Cbuff3,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);
                        
            }
        }
        else {
        // Store partial sum
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff0.template to_vector<int16>(conv2d_params.shift_psum_out)); 
            p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, conv2d_params.inc_ST_0);
            
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff1.template to_vector<int16>(conv2d_params.shift_psum_out)); 
            p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, conv2d_params.inc_ST_1);
            
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff2.template to_vector<int16>(conv2d_params.shift_psum_out)); 
            p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, conv2d_params.inc_ST_0);
            
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff3.template to_vector<int16>(conv2d_params.shift_psum_out)); 
            p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);   
        }         
        p_out = INCR_ITR2D(p_out,iterator_pout)
    }

    if constexpr(out_mode == FULL){
         #if RELU_INT8_ENABLE == 1
            #ifdef USE_RTP_ACT
            	relu_int8_post_process(output,conv2d_params.out_sign,conv2d_params.ofm_len);
            #else
                int act = 1 - act_type;
                relu_int8_post_process(output,act,conv2d_params.ofm_len);   
            #endif
         #endif
    }
}

#endif //ifdef CONV2D_OPT1 
#endif //if __AIE_ARCH__ == 21


//Enable generalizationsin FC: support for batch size=1,2,4, int8 in FC, needs out_mode=1 for configuring int16 output
template <typename dtype_ofm, uint8_t batch_size, uint8_t fc_out16> 
NOINLINE void fc_post_process
(          
           int * restrict output,
           conv2d_params_t &conv2d_param
){
    dtype_ofm  * p_ofm  = (dtype_ofm DM_BANK(a) *) output;
    dtype_ofm  * p_final  = (dtype_ofm DM_BANK(a) *) output;    
    if constexpr(fc_out16 == 1){    // int16 output
       vector<dtype_ofm, 64>Obuff0,Obuff1,Xbuff0,Xbuff1;
       AIE_PREPARE_FOR_PIPELINING 
       AIE_LOOP_RANGE(1,) 
       for(unsigned i=0; i < conv2d_param.outer_g ; i++)chess_keep_sw_loop{
          Obuff0= load_v<64>((dtype_ofm *) p_ofm);
          p_ofm = byte_incr(p_ofm, 64);
          Obuff1= load_v<64>((dtype_ofm *) p_ofm);
          p_ofm = byte_incr(p_ofm, 256);
	  if constexpr(batch_size==4){
            store_v(p_final, Obuff0); p_final = byte_incr(p_final, 64);
            store_v(p_final, Obuff1); p_final = byte_incr(p_final, 64); 
	  }
	  else{
            std::tie(Xbuff0, Xbuff1) = interleave_zip(Obuff0, Obuff1, 16*batch_size);
            store_v(p_final, Xbuff0); p_final = byte_incr(p_final, 32*batch_size);
          } 
       }
    }
    else{ // int8 output 
       vector<dtype_ofm, 32>Obuff0,Obuff1,Xbuff0,Xbuff1;
       AIE_PREPARE_FOR_PIPELINING 
       AIE_LOOP_RANGE(1,) 
       for(unsigned i=0; i < conv2d_param.outer_g ; i++)chess_keep_sw_loop{
         Obuff0= load_v<32>((dtype_ofm *) p_ofm);
         p_ofm = byte_incr(p_ofm, 32);
         Obuff1= load_v<32>((dtype_ofm *) p_ofm);
         p_ofm = byte_incr(p_ofm, 128);
	 if constexpr(batch_size==4){
            store_v(p_final, Obuff0); p_final = byte_incr(p_final, 32);
            store_v(p_final, Obuff1); p_final = byte_incr(p_final, 32); 
	 }
	 else{
             std::tie(Xbuff0, Xbuff1) = interleave_zip(Obuff0, Obuff1, 8*batch_size);
	     if constexpr(batch_size==1){
                vector<dtype_ofm, 32>Obuff2,Obuff3,Xbuff2,Xbuff3,Xbuff4,Xbuff5;
                Obuff2= load_v<32>((dtype_ofm *) p_ofm);
                p_ofm = byte_incr(p_ofm, 32);
                Obuff3= load_v<32>((dtype_ofm *) p_ofm);
                p_ofm = byte_incr(p_ofm, 32);
                std::tie(Xbuff2, Xbuff3) = interleave_zip(Obuff2, Obuff3, 8);
	        std::tie(Xbuff4, Xbuff5) = interleave_zip(Xbuff0, Xbuff2, 16);
	        Xbuff0 = Xbuff4;
                }
             store_v(p_final, Xbuff0); p_final = byte_incr(p_final, 32); //For batch_size 1 - 32, batch_size 2 - 16*batch_size   
         }
       }
    }     
}
// legacy mode works for fixed length vector,batch size=1, int16 only
template <typename dtype_ofm> 
NOINLINE void fc_post_process
(            
           int * __restrict output,
           conv2d_params_t &conv2d_param
){
    vector<dtype_ofm, 32>Obuff0,Obuff1,Xbuff0,Xbuff1;
    dtype_ofm  * p_ofm  = (dtype_ofm *) output;

    Obuff0= load_v<32>((dtype_ofm *) p_ofm);
    p_ofm = byte_incr(p_ofm, 64);
    Obuff1= load_v<32>((dtype_ofm *) p_ofm);
    p_ofm = byte_incr(p_ofm, -64);
	
    std::tie(Xbuff0, Xbuff1) = interleave_zip(Obuff0, Obuff1, 16);
    store_v(p_ofm, Xbuff0);
}

template <typename dtype_ofm, typename config_ofm, uint8_t batch_size, uint8_t fc_out16> 
NOINLINE void fc_post_process
(          
           adf::output_buffer_conf<dtype_ofm, config_ofm> &  __restrict ofm,
           conv2d_params_t &conv2d_param
){
    
    uint32 psum_offset = conv2d_param.ofm_len ;//+ conv2d_param.ofm_offset_top + conv2d_param.ofm_offset_left*8*batch_size;

    int16 * psum_0  = (int16 *)(adf::io_buffer_access::getBuf(ofm, 0) + psum_offset);
    int16 * psum_1  = (int16 *)(adf::io_buffer_access::getBuf(ofm, 1) + psum_offset);   

    int16 * p_final = (int16 *) ofm.data();    
    
    vector<int16, 32> Obuff0, Obuff1, Xbuff0, Xbuff1;
    
    /*for(unsigned i=0; i < conv2d_param.outer_g ; i++)
    chess_no_prepare_for_pipelining
    {*/
        Obuff0 = load_v<32>(psum_0);  psum_0 = byte_incr(psum_0, 256);
        Obuff1 = load_v<32>(psum_1);  psum_1 = byte_incr(psum_1, 256);
        
        std::tie(Xbuff0, Xbuff1) = interleave_zip(Obuff0, Obuff1, 8);

        store_v(p_final, Xbuff0.template extract<16>(0));  p_final = byte_incr(p_final, 32);
    //}   

}


template <uint8_t batch_size, act_t act_type, typename dtype_ifm, typename dtype_wts, typename dtype_ofm, typename config_ifm, typename config_wts, typename config_ofm> 
void conv2d
(
        adf::input_buffer_conf<dtype_ifm, config_ifm>  & __restrict ifm,
        adf::input_buffer_conf<dtype_wts, config_wts>  & __restrict wts,
        adf::output_buffer_conf<dtype_ofm, config_ofm> &  ofm,
        conv2d_params_t            &conv2d_params
) {
    // Output modes
    const out_mode_t out_mode_full     = FULL ;
    const out_mode_t out_mode_partial  = PARTIAL ;

    // Setup iteration level parameters like IO mode etc
    #ifndef CONV2D_OPT0
    setup_conv2d_iter_params(conv2d_params) ;
    #else
    setup_conv2d_iter_params(conv2d_params,wts.data()) ;
    #endif

    // Extract low level pointers used by the actual kernel code
    int * input   = (int*) ifm.data();// + conv2d_params.ifm_offset;
    int * weights = (int*) wts.data() + conv2d_params.hdr_len + conv2d_params.wts_offset ;
    int * output  = (int*) ofm.data(); 
    int * acc_in  = (conv2d_params.psum_in == 0) ? (int*) wts.data() + LAYER_PARAM_WORDS + conv2d_params.wts_offset : ((int*) ofm.data() + (conv2d_params.psum_buff_offset>>2));

    #ifdef CONV2D_OPT1
    
    #if __AIE_ARCH__ == 21
        uint32 psum_offset = conv2d_params.ifm_len ;
        int * psum_0  = (int32 *)(0x70000 + psum_offset);
        int * psum_1  = (int32 *)(0x74000 + psum_offset);         
    #else
        uint32 psum_offset = conv2d_params.ofm_len ;
        int * psum_0  = (int32 *)(adf::io_buffer_access::getBuf(ofm, 0) + psum_offset);
        int * psum_1  = (int32 *)(adf::io_buffer_access::getBuf(ofm, 1) + psum_offset); 
    #endif    
   
    #endif
     
    switch(conv2d_params.exec_type) {
    #ifdef CONV2D_OPT1
    case 0:
    case 2:
            #if __AIE_ARCH__ == 21
            if (conv2d_params.conv2d_1x16x16){
                conv2d_1x16x16_opt<batch_size,act_type,out_mode_full,dtype_ifm,dtype_wts,dtype_ofm>(input, weights, psum_0, psum_1, output,conv2d_params);
            }else 
            #endif
            {
                conv2d_generic<batch_size,act_type,out_mode_full,dtype_ifm,dtype_wts,dtype_ofm>(input, weights, psum_0, psum_1, output,conv2d_params);                  
            }
            break;       
    case 1:
    case 3:
         conv2d_generic<batch_size,act_type,out_mode_partial,dtype_ifm,dtype_wts,dtype_ofm>(input, weights, psum_0, psum_1, output,conv2d_params);
         break ;
    #else
    case 0:
    case 2:
         conv2d_generic<batch_size,act_type,out_mode_full,dtype_ifm,dtype_wts,dtype_ofm>(input, weights, acc_in, output,conv2d_params);
         break ;
    case 1:
    case 3:
         conv2d_generic<batch_size,act_type,out_mode_partial,dtype_ifm,dtype_wts,dtype_ofm>(input, weights, acc_in, output,conv2d_params);
         break ;
    #endif
    
    #ifdef STRIDE2_OPT
    case 4:
    case 6:
            #if __AIE_ARCH__ == 21
            if (conv2d_params.conv2d_1x16x16){
                conv2d_1x16x16_opt<batch_size,act_type,out_mode_full,dtype_ifm,dtype_wts,dtype_ofm>(input, weights, psum_0, psum_1, output,conv2d_params);
            } else
            #endif
            {
                #ifdef CONV2D_S2_OPT1
                    conv2d_generic_stride2_opt <batch_size,act_type,out_mode_full,dtype_ifm,dtype_wts,dtype_ofm>(input, weights, psum_0, psum_1, output,conv2d_params);
                #else
                    conv2d_generic_stride2_opt <batch_size,act_type,out_mode_full,dtype_ifm,dtype_wts,dtype_ofm>(input, weights, acc_in, output,conv2d_params);
                #endif
            }
         break ;
    case 5:
    case 7:
         #ifdef CONV2D_S2_OPT1
            conv2d_generic_stride2_opt <batch_size,act_type,out_mode_partial,dtype_ifm,dtype_wts,dtype_ofm>(input, weights, psum_0, psum_1, output,conv2d_params);
          #else
            conv2d_generic_stride2_opt <batch_size,act_type,out_mode_partial,dtype_ifm,dtype_wts,dtype_ofm>(input, weights, acc_in, output,conv2d_params);
         #endif         
         break ;    
    #endif
        
    
    #if USE_OPMODE_1 == 1
    case 16: // Layer-1, 1<<4+0
         conv2d_gran8<batch_size,act_type,uint8,dtype_wts,dtype_ofm>(input, weights, acc_in, output,conv2d_params);
         break ;
    #endif
    }
}

#endif //__CONV2D_H__

