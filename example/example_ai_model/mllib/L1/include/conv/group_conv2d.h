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

#pragma once
#include "mllib_const.h"
#include "group_conv2d_params.h"



// TODO: Replace uses of GC_load_v with load_v; remove the 3 variants of GC_load_v().
// 
// We will change aie_api aie::load_v so that for __X86SIM__ the address will also
// be truncated to a multiple of 32 bytes. After these changes, you can replace
// calls to GC_load_v with calls to load_v. The 3 functions below will no longer be needed.
#if __AIE_ARCH__ == 20
template <int N>
__attribute__((always_inline)) aie::vector<int8, N> GC_load_v(int8 *ptr) {
#ifdef __X86SIM__
  aie::vector<int8, N> res;

  int tmp_val = (long)ptr;
  tmp_val %= 32;
  ptr -= tmp_val;

  for (int i = 0; i < N; i++) {
    res[i] = *(ptr + i);
  }

  return res;
#else
  return aie::load_v<N>(ptr);
#endif
}

template <int N>
__attribute__((always_inline)) aie::vector<uint8, N> GC_load_v(uint8 *ptr) {
#ifdef __X86SIM__
  aie::vector<uint8, N> res;

  int tmp_val = (long)ptr;
  tmp_val %= 32;
  ptr -= tmp_val;

  for (int i = 0; i < N; i++) {
    res[i] = *(ptr + i);
  }

  return res;
#else
  return aie::load_v<N>(ptr);
#endif
}

template <int N>
__attribute__((always_inline)) aie::vector<int16, N> GC_load_v(int16 *ptr) {
#ifdef __X86SIM__
  aie::vector<int16, N> res;

  int tmp_val = (long)ptr;
  tmp_val %= 32;
  ptr -= (tmp_val / 2);

  for (int i = 0; i < N; i++) {
    res[i] = *(ptr + i);
  }

  return res;
#else
  return aie::load_v<N>(ptr);
#endif
}
#endif

template <uint8_t batch_size, act_t act_type, out_mode_t out_mode, typename dtype_ifm, typename dtype_wts, typename dtype_ofm>
INLINE void gconv2d_generic
(
        int * __restrict input,
        int * __restrict weights,
        int * __restrict acc_in,
        int * __restrict output,
        int * __restrict output_psum,
        group_conv2d_params_t &params
) {
    // printf("entered generic\n");
    // DEBUG_EXEC(printf("input(%p)weights(%p)acc_in(%p)output(%p)output_psum(%p)\n", input, weights, acc_in, output, output_psum));
    int16     OFM_DM_BANK  * p_sum   = (int16      OFM_DM_BANK *)output_psum; //((int16      OFM_DM_BANK *) output + (params.psum_buff_offset>>1));
    dtype_ifm IFM_DM_BANK  * p_in    = (dtype_ifm  IFM_DM_BANK *) input;
    dtype_wts WT_DM_BANK   * p_w     = (dtype_wts   WT_DM_BANK  *) weights;

    vector<dtype_ifm, 32> vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3;
    int16     OFM_DM_BANK * p_init16 = (int16 OFM_DM_BANK *) acc_in;
    //dtype_ofm OFM_DM_BANK * p_out    = (params.psum_out==0)?(dtype_ofm OFM_DM_BANK *)output : (dtype_ofm OFM_DM_BANK *)p_sum;
    dtype_ofm OFM_DM_BANK * p_out    = (params.psum_out==0)?((dtype_ofm OFM_DM_BANK *)output+params.ofm_offset_top+params.ofm_offset_left*8*batch_size) : (dtype_ofm OFM_DM_BANK *)p_sum;
    // DEBUG_EXEC(printf("p_out_post(%p)params.ofm_offset_top(%d)params.ofm_offset_left(%d)psum_out(%d)p_sum(%p)\n", p_out, params.ofm_offset_top, params.ofm_offset_left, params.psum_out, p_sum));
    // DEBUG_EXEC(printf("p_init16(%p)\n", p_init16));

    using mmul = mmul<DIM_M, DIM_N, DIM_K, dtype_ifm, dtype_wts>;

    vector<dtype_wts, 64>  Ybuff0, Ybuff1;

    #define LOAD_WEIGHTS(wts,wbuff,wts_incr) \
    {\
        wbuff.insert(0, GC_LOAD_V<32>(wts)); /*PRINT_PTR(wts, 32, "w0=");*/ wts+=32;\
        wbuff.insert(1, GC_LOAD_V<32>(wts)); /*PRINT_PTR(wts, 32, "w1=");*/ wts+=wts_incr;\
    }

    //input: mmul type Cbuff
    //shift_input: Intial shift applied to convert accum<acc32,32> to vec<int16,16>
    //alpha: This is the scaled value to be multiplied
    //shift_alpha: shift required to remove the scaling factor of alpha
    //shift_out: shift applied before saving to final output
    //output_ptr: pointer to the location where data is to be saved
    #define LEAKY_RELU_COMPUTE(input,shift_input,alpha,shift_alpha,shift_out,output_ptr)            \
    {                                                                                               \
        vector<int16,32> input_vec   = input.template to_vector<int16>(shift_input);                \
        accum<acc32,32> acc         = mul<acc32,vector<int16,32>,int16>(input_vec,alpha);           \
        vector<int16,32> alpha_vec  = acc.to_vector<int16>(shift_alpha);                            \
        vector<int16,32> result_vec = max<vector<int16,32>,vector<int16,32>>(input_vec,alpha_vec);  \
        acc.from_vector(result_vec,0);                                                              \
        store_v(output_ptr,acc.to_vector<int8>(shift_out));                                         \
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

    int frac;
    int chnd   = 32;
    int chnd_T = 64;

    if constexpr(batch_size < 4) {
        p_in += 31 ;
    }
    /*
    int    inc_ST_0       = (params.psum_in ==  0) ? chnd_T                              : 2*chnd_T;
    int    inc_ST_1       = (params.psum_in ==  0) ? chnd_T                              : params.inc_ST_0;
    int    inc_ST_2       = (params.psum_in ==  0) ? params.inc_acc_1+chnd_T      : params.inc_ST_1;
    int    inc_ST_3       = (params.psum_in ==  0) ? params.inc_acc_2+chnd_T      : chnd_T;
    */
    // 2x8 mode updates for partial sum
    int    inc_ST_0       = (params.psum_in ==  0) ? chnd_T                              : 2*chnd_T;
    int    inc_ST_1       = (params.psum_in ==  0) ? chnd_T                              : params.inc_ST_0_2x8;
    int    inc_ST_2       = (params.psum_in ==  0) ? params.inc_acc_1+chnd_T      : params.inc_ST_1_2x8;
    int    inc_ST_3       = (params.psum_in ==  0) ? params.inc_acc_2+chnd_T      : chnd_T;

    DECL_ITR3D(iterator_psum)
    SETUP_ITR3D(iterator_psum,
                0,
                (params.psum_in ==  0) ? params.num_X : 0,
                (params.psum_in ==  0) ? params.inc_acc_d1 : params.iter_2x8_inc_T,
                (params.psum_in ==  0) ? params.psum_num_Co_init : params.iter_2x8_wrap,
                (params.psum_in ==  0) ? params.inc_acc_rev : params.inc_Co_T-chnd_T)
                //(params.psum_in ==  0) ? params.inc_acc_d1 : params.inc_Xo_T-chnd_T,
                //(params.psum_in ==  0) ? params.num_Co : params.num_X,

    int    acc_init_shift = (params.psum_in ==  0) ? (int) params.shift_bias_init : (int) params.shift_psum_in;

    DECL_ITR2D(iterator_pout)
    if constexpr(out_mode == PARTIAL){
        //SETUP_ITR2D(iterator_pout,params.inc_Xo_T-chnd_T, params.num_X,params.inc_Co_T-chnd_T);
        SETUP_ITR2D(iterator_pout,params.iter_2x8_inc_T, params.iter_2x8_wrap,params.inc_Co_T-chnd_T);
        // DEBUG_EXEC(printf("params.iter_2x8_inc_t(%d)params.iter_2x8_wrap(%d)params.inc_Co_T-chnd_T(%d)\n", params.iter_2x8_inc_T, params.iter_2x8_wrap,params.inc_Co_T-chnd_T));
    }else{
        //SETUP_ITR2D(iterator_pout,params.inc_Xo-chnd,params.num_X,params.inc_Co-chnd) ;
        SETUP_ITR2D(iterator_pout, params.ofm_iter_inc0,params.iter_2x8_wrap,params.inc_Co-chnd);
        // DEBUG_EXEC(printf("params.ofm_iter_inc0(%d)params.iter_2x8_wrap(%d)params.inc_Co-chnd(%d)\n",params.ofm_iter_inc0,params.iter_2x8_wrap,params.inc_Co-chnd));
    }
    /*
    SETUP_ITR2D(iterator_pout,
               (params.psum_out == 0)? params.inc_Xo-chnd : params.inc_Xo_T-chnd_T,
               params.num_X,
               (params.psum_out == 0)? params.inc_Co-chnd : params.inc_Co_T-chnd_T)
    */

    // DEBUG_EXEC(dtype_ifm *pin0, *pin1, *pin2, *pin3);
#ifndef __X86SIM__
    AIE_NO_PREPARE_FOR_PIPELINING 
#endif
    for (unsigned j=0; j<params.outer_g; j++)
    {
        if constexpr(batch_size < 4) {
            v64int8 Sbuff;
            int pdel;
            pdel = (long)p_in;
            frac = (pdel & 31) + 33;
        }
        mmul Cbuff0( GC_LOAD_V<32>( p_init16 ), acc_init_shift ); /*DEBUG_EXEC(printf("%p", p_init16));PRINT_VEC(GC_LOAD_V<32>( p_init16 ), "###p_init16_0:");*/ p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T);
        mmul Cbuff1( GC_LOAD_V<32>( p_init16 ), acc_init_shift ); /*DEBUG_EXEC(printf("%p", p_init16));PRINT_VEC(GC_LOAD_V<32>( p_init16 ), "###p_init16_1:");*/ p_init16 = byte_incr(p_init16, inc_ST_1-chnd_T);
        mmul Cbuff2( GC_LOAD_V<32>( p_init16 ), acc_init_shift ); /*DEBUG_EXEC(printf("%p", p_init16));PRINT_VEC(GC_LOAD_V<32>( p_init16 ), "###p_init16_2:");*/ p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T);
        mmul Cbuff3( GC_LOAD_V<32>( p_init16 ), acc_init_shift ); /*DEBUG_EXEC(printf("%p", p_init16));PRINT_VEC(GC_LOAD_V<32>( p_init16 ), "###p_init16_3:");*/ p_init16 = byte_incr(p_init16, inc_ST_2-chnd_T);
        mmul Cbuff4( GC_LOAD_V<32>( p_init16 ), acc_init_shift ); /*DEBUG_EXEC(printf("%p", p_init16));PRINT_VEC(GC_LOAD_V<32>( p_init16 ), "###p_init16_4:");*/ p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T);
        mmul Cbuff5( GC_LOAD_V<32>( p_init16 ), acc_init_shift ); /*DEBUG_EXEC(printf("%p", p_init16));PRINT_VEC(GC_LOAD_V<32>( p_init16 ), "###p_init16_5:");*/ p_init16 = byte_incr(p_init16, inc_ST_1-chnd_T);
        mmul Cbuff6( GC_LOAD_V<32>( p_init16 ), acc_init_shift ); /*DEBUG_EXEC(printf("%p", p_init16));PRINT_VEC(GC_LOAD_V<32>( p_init16 ), "###p_init16_6:");*/ p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T);
        mmul Cbuff7( GC_LOAD_V<32>( p_init16 ), acc_init_shift ); /*DEBUG_EXEC(printf("%p", p_init16));PRINT_VEC(GC_LOAD_V<32>( p_init16 ), "###p_init16_7:");*/ p_init16 = byte_incr(p_init16, inc_ST_3-chnd_T);

        AIE_PREPARE_FOR_PIPELINING
        AIE_LOOP_RANGE(4,)
        for (int i=0; i<params.inner_g; i++)
        
        
        {
            if constexpr(batch_size < 4) {
                vector<uint8_t, 64>  Sbuff0, Sbuff1,Xbuff0,Xbuff1,Xbuff2,Xbuff3;

                // Load IFM data, two 4x8 matrices plus additional adjacent data needed for shift alignment
                Sbuff0.insert( 0,  GC_LOAD_V<32>((uint8 IFM_DM_BANK *) p_in)); /*DEBUG_EXEC(pin0=p_in);  PRINT_PTR((uint8*)p_in, 32, "p_in0=");*/ p_in = byte_incr(p_in, params.inc_A);
                Sbuff0.insert( 1,  GC_LOAD_V<32>((uint8 IFM_DM_BANK *) p_in)); /*DEBUG_EXEC(pin1=p_in);  PRINT_PTR((uint8*)p_in, 32, "p_in1=");*/ p_in = byte_incr(p_in, params.inc_A_2x8_mode);//2x8 mode support jump of height=1: -32+8*ifm_sv_depth
                Sbuff1.insert( 0,  GC_LOAD_V<32>((uint8 IFM_DM_BANK *) p_in)); /*DEBUG_EXEC(pin2=p_in);  PRINT_PTR((uint8*)p_in, 32, "p_in2=");*/ p_in = byte_incr(p_in, params.inc_A);
                Sbuff1.insert( 1,  GC_LOAD_V<32>((uint8 IFM_DM_BANK *) p_in)); /*DEBUG_EXEC(pin3=p_in);  PRINT_PTR((uint8*)p_in, 32, "p_in3=");*/


                Xbuff.x0 = shiftx(Xbuff.x0, Sbuff0, params.step_align, frac);
                Xbuff.x2 = shiftx(Xbuff.x2, Sbuff1, params.step_align, frac);

                // TODO: CR-1103776
                p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,params)
                // params.iterator_inner_incr0,params.iterator_inner_wrap0,params.iterator_inner_cnt0
                // params.iterator_inner_incr1,params.iterator_inner_wrap1,params.iterator_inner_cnt1
                // params.iterator_inner_incr2
                Xbuff0 = interleave(Xbuff.x0, Xbuff.x2, params.shfl_0);
                Xbuff1 = interleave(Xbuff0,   unu8, T256_2x2_hi);
                Xbuff2 = interleave(Xbuff.x0, Xbuff.x2, params.shfl_1);
                Xbuff3 = interleave(Xbuff2,   unu8, T256_2x2_hi);


                vec_Xbuff0 = Xbuff0.cast_to<dtype_ifm>(). template extract<32>(0);
                vec_Xbuff1 = Xbuff1.cast_to<dtype_ifm>(). template extract<32>(0);
                vec_Xbuff2 = Xbuff2.cast_to<dtype_ifm>(). template extract<32>(0);
                vec_Xbuff3 = Xbuff3.cast_to<dtype_ifm>(). template extract<32>(0);
            }
            else {
                vec_Xbuff0 = GC_LOAD_V<32>((dtype_ifm IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, params.inc_A);
                vec_Xbuff1 = GC_LOAD_V<32>((dtype_ifm IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, params.inc_A);
                vec_Xbuff2 = GC_LOAD_V<32>((dtype_ifm IFM_DM_BANK *) p_in); p_in = byte_incr(p_in, params.inc_A);
                vec_Xbuff3 = GC_LOAD_V<32>((dtype_ifm IFM_DM_BANK *) p_in);

                p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,params)
            }

            // Load weights -- two matrices 8*8
            LOAD_WEIGHTS(p_w,Ybuff0,32) ;
            LOAD_WEIGHTS(p_w,Ybuff1,32) ;

            // DEBUG_EXEC(printf("pin0(%p)pin1(%p)pin2(%p)pin3(%p)\n", pin0, pin1, pin2, pin3));
            // PRINT_VEC(vec_Xbuff0, "xbuff0=");
            // PRINT_VEC(vec_Xbuff1, "xbuff1=");
            // PRINT_VEC(vec_Xbuff2, "xbuff2=");
            // PRINT_VEC(vec_Xbuff3, "xbuff3=");
            // PRINT_VEC(Ybuff0, "ybuff0=");
            // PRINT_VEC(Ybuff1, "ybuff1=");

            #ifdef USE_RTP_SIGN
            int sign_x=params.ifm_sign;
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

        // DEBUG_EXEC(printf("inner loop exited\n"));
        // DEBUG_EXEC(printf("pin before add outer it(%p)\n", p_in));
        p_in     = INCR_ITR3D_PARAM(p_in, iterator_outer,params)
        // DEBUG_EXEC(printf("pin after add outer it(%p)\n", p_in));
        p_w      = INCR_ITR3D_PARAM(p_w, iterator_weights,params)
        p_init16 = INCR_ITR3D(p_init16, iterator_psum)

        // Write out the final outputs
        if constexpr(out_mode == FULL){
            if constexpr(act_type==LINEAR || act_type==RELU){
                 #ifdef USE_RTP_ACT
                 int out_sign=params.out_sign;
                 #else
                 int out_sign = 1-act_type; // Note enum for act types are: 0,1,2 -> linear, relu, lrelu
                 #endif
                 store_v(p_out, Cbuff0.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out)); /*DEBUG_EXEC(printf("%p", p_out)); PRINT_VEC(Cbuff0.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out), "@pout_full0=");*/ p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff1.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out)); /*DEBUG_EXEC(printf("%p", p_out)); PRINT_VEC(Cbuff1.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out), "@pout_full1=");*/ p_out = byte_incr(p_out, params.inc_S_0_2x8); // 2x8 mode jump by ht=1
                 store_v(p_out, Cbuff2.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out)); /*DEBUG_EXEC(printf("%p", p_out)); PRINT_VEC(Cbuff2.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out), "@pout_full2=");*/ p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff3.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out)); /*DEBUG_EXEC(printf("%p", p_out)); PRINT_VEC(Cbuff3.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out), "@pout_full3=");*/ p_out = byte_incr(p_out, params.ofm_c8_skip_sz);
                 store_v(p_out, Cbuff4.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out)); /*DEBUG_EXEC(printf("%p", p_out)); PRINT_VEC(Cbuff4.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out), "@pout_full4=");*/ p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff5.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out)); /*DEBUG_EXEC(printf("%p", p_out)); PRINT_VEC(Cbuff5.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out), "@pout_full5=");*/ p_out = byte_incr(p_out, params.inc_S_0_2x8);
                 store_v(p_out, Cbuff6.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out)); /*DEBUG_EXEC(printf("%p", p_out)); PRINT_VEC(Cbuff6.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out), "@pout_full6=");*/ p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff7.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out)); /*DEBUG_EXEC(printf("%p", p_out)); PRINT_VEC(Cbuff7.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out), "@pout_full7=");*/
                 /*
                 store_v(p_out, Cbuff0.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff1.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out)); p_out = byte_incr(p_out, params.inc_S_0-chnd);
                 store_v(p_out, Cbuff2.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff3.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out)); p_out = byte_incr(p_out, params.inc_S_1-chnd);
                 store_v(p_out, Cbuff4.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff5.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out)); p_out = byte_incr(p_out, params.inc_S_0-chnd);
                 store_v(p_out, Cbuff6.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff7.template to_vector_sign<dtype_ofm>(out_sign,params.shift_out));
                 */

            } else {
                // Code/Macro for Leaky reLU

                LEAKY_RELU_COMPUTE(Cbuff0,params.shift_lrelu_input,params.lrelu_alpha,params.shift_lrelu_alpha,params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);

                LEAKY_RELU_COMPUTE(Cbuff1,params.shift_lrelu_input,params.lrelu_alpha,params.shift_lrelu_alpha,params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, params.inc_S_0-chnd);

                LEAKY_RELU_COMPUTE(Cbuff2,params.shift_lrelu_input,params.lrelu_alpha,params.shift_lrelu_alpha,params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);

                LEAKY_RELU_COMPUTE(Cbuff3,params.shift_lrelu_input,params.lrelu_alpha,params.shift_lrelu_alpha,params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, params.ofm_c8_skip_sz);

                LEAKY_RELU_COMPUTE(Cbuff4,params.shift_lrelu_input,params.lrelu_alpha,params.shift_lrelu_alpha,params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);

                LEAKY_RELU_COMPUTE(Cbuff5,params.shift_lrelu_input,params.lrelu_alpha,params.shift_lrelu_alpha,params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, params.inc_S_0-chnd);

                LEAKY_RELU_COMPUTE(Cbuff6,params.shift_lrelu_input,params.lrelu_alpha,params.shift_lrelu_alpha,params.shift_lrelu_out,p_out)
                p_out = byte_incr(p_out, 32);

                LEAKY_RELU_COMPUTE(Cbuff7,params.shift_lrelu_input,params.lrelu_alpha,params.shift_lrelu_alpha,params.shift_lrelu_out,p_out)

            }
        }
        else {
        // Store partial sum
           /*
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff0.template to_vector<int16>(params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff1.template to_vector<int16>(params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, params.inc_ST_0-chnd_T);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff2.template to_vector<int16>(params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff3.template to_vector<int16>(params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, params.inc_ST_1-chnd_T);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff4.template to_vector<int16>(params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff5.template to_vector<int16>(params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, params.inc_ST_0-chnd_T);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff6.template to_vector<int16>(params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff7.template to_vector<int16>(params.shift_psum_out));
           */
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff0.template to_vector<int16>(params.shift_psum_out)); /*DEBUG_EXEC(printf("%p", p_out)); PRINT_VEC(Cbuff0.template to_vector<int16>(params.shift_psum_out), "@pout_partial0=");*/  p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff1.template to_vector<int16>(params.shift_psum_out)); /*DEBUG_EXEC(printf("%p", p_out)); PRINT_VEC(Cbuff1.template to_vector<int16>(params.shift_psum_out), "@pout_partial1=");*/  p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, params.inc_ST_0_2x8-chnd_T);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff2.template to_vector<int16>(params.shift_psum_out)); /*DEBUG_EXEC(printf("%p", p_out)); PRINT_VEC(Cbuff2.template to_vector<int16>(params.shift_psum_out), "@pout_partial2=");*/  p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff3.template to_vector<int16>(params.shift_psum_out)); /*DEBUG_EXEC(printf("%p", p_out)); PRINT_VEC(Cbuff3.template to_vector<int16>(params.shift_psum_out), "@pout_partial3=");*/  p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, params.inc_ST_1_2x8-chnd_T);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff4.template to_vector<int16>(params.shift_psum_out)); /*DEBUG_EXEC(printf("%p", p_out)); PRINT_VEC(Cbuff4.template to_vector<int16>(params.shift_psum_out), "@pout_partial4=");*/  p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff5.template to_vector<int16>(params.shift_psum_out)); /*DEBUG_EXEC(printf("%p", p_out)); PRINT_VEC(Cbuff5.template to_vector<int16>(params.shift_psum_out), "@pout_partial5=");*/  p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, params.inc_ST_0_2x8-chnd_T);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff6.template to_vector<int16>(params.shift_psum_out)); /*DEBUG_EXEC(printf("%p", p_out)); PRINT_VEC(Cbuff6.template to_vector<int16>(params.shift_psum_out), "@pout_partial6=");*/  p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff7.template to_vector<int16>(params.shift_psum_out)); /*DEBUG_EXEC(printf("%p", p_out)); PRINT_VEC(Cbuff7.template to_vector<int16>(params.shift_psum_out), "@pout_partial7=");*/
        }
        p_out = INCR_ITR2D(p_out,iterator_pout)
    }

    if constexpr(out_mode == FULL){
         #if RELU_INT8_ENABLE == 1
            #ifdef USE_RTP_ACT
            	relu_int8_post_process(output,params.out_sign,params.ofm_len);
            #else
                int act = 1 - act_type;
                relu_int8_post_process(output,act,params.ofm_len);
            #endif
         #endif
    }
}

template <typename dtype_ifm, typename dtype_ofm>
void post_process(int * __restrict output,
                  group_conv2d_params_t &params) {
    dtype_ofm OFM_DM_BANK * p_out_load    = (dtype_ofm OFM_DM_BANK *)output;
    dtype_ofm OFM_DM_BANK * p_out_store   = (dtype_ofm OFM_DM_BANK *)output;
    SETUP_ITR3D_COUNTERS(iterator_post_process_load)
    SETUP_ITR3D_COUNTERS(iterator_post_process_store)
    
    int used_slot = 0, empty_slot = 0; // 32 slots in total.
    int shift_before_store = 0; 
    int shift_after_store = 0;
    int valid_depth = params.ofm_depth_per_group;
    int valid_depth_per_load = 0;
    int valid_elems_per_load = 0;
    bool store = false;
    vector<dtype_ifm, 32> vec_load, vec_store;
    for (int i = 0; i < params.post_process_loop_cnt; i++) {
        // DEBUG_EXEC(printf("p_out_load = %p\n", p_out_load));
        // DEBUG_EXEC(printf("p_out_store = %p\n", p_out_store));
  
        valid_depth_per_load = valid_depth <= 8 ? valid_depth : 8; // each round max process 8 channels.
        valid_elems_per_load = valid_depth_per_load * 4; // each round loads 4 column elems (W = 4)
        
        empty_slot = 32 - used_slot;
        shift_before_store = empty_slot >= valid_elems_per_load ? valid_elems_per_load : empty_slot;
        shift_after_store = empty_slot >= valid_elems_per_load ? 0 : valid_elems_per_load - empty_slot;
        store = (used_slot + valid_elems_per_load) >= 32 ? true : false;
        used_slot = (used_slot + valid_elems_per_load) >= 32 ? (used_slot + valid_elems_per_load - 32) : (used_slot + valid_elems_per_load);

        vec_load = GC_LOAD_V<32>(p_out_load);
        vector<dtype_ifm, 32> vec_load_trans = aie::transpose(vec_load, 4, 8);
        vec_store = aie::shuffle_down_fill(vec_store, vec_load_trans, shift_before_store);
        vector<dtype_ifm, 32> vec_store_trans = aie::transpose(vec_store, 8, 4);
        store_v(p_out_store, vec_store_trans);
        // PRINT_VEC(vec_load, "vec_load=");
        // PRINT_VEC(vec_store_trans, "vec_store_trans=");

        // process not stored part
        vec_load_trans = aie::shuffle_down(vec_load_trans, shift_before_store);
        vec_store = aie::shuffle_down_fill(vec_store, vec_load_trans, shift_after_store);

        if (store) {
            p_out_store = INCR_ITR3D_PARAM(p_out_store, iterator_post_process_store, params)
        }
       
        p_out_load = INCR_ITR3D_PARAM(p_out_load, iterator_post_process_load, params)

        valid_depth -= valid_depth_per_load;
        valid_depth = valid_depth == 0 ? params.ofm_depth_per_group : valid_depth;
    }
}

template <uint32_t batch_size, act_t act_type, uint32_t enable_post_process, typename dtype_ifm, typename dtype_wts, typename dtype_ofm, typename config_ifm, typename config_wts, typename config_ofm>
void group_conv2d
(
    adf::input_buffer_conf<dtype_ifm, config_ifm>  & __restrict ifm,
    adf::input_buffer_conf<dtype_wts, config_wts>  & __restrict wts,
    adf::output_buffer_conf<dtype_ofm, config_ofm> &  ofm,
    group_conv2d_params_t &params
) {
    const out_mode_t out_mode_full     = FULL;
    const out_mode_t out_mode_partial  = PARTIAL;
    // DEBUG_EXEC(printf("Entering group_conv2d\n"));
    // we shall only set 1 time of iter params per invoke, each call of generic function actually does computation for G(C/8)WN_C8
    // such operation shall continue for H(out) times
    setup_gconv2d_iter_params(params) ;

    int * input = (int*)ifm.data();
    // DEBUG_EXEC(printf("input: %p\n", input));
    int * weights = (int*) wts.data() + params.hdr_len + params.wts_offset ;
    // DEBUG_EXEC(printf("weights: %p\n", weights));
    // DEBUG_EXEC(printf("hdr_len(%d)wts_offset(%d)\n", params.hdr_len, params.wts_offset));
    int * output  = (int*) ofm.data();
    int * output_psum = reinterpret_cast<int*>((int16      OFM_DM_BANK *) output + (params.psum_buff_offset>>1));;
    // this is the pointer for psum as input
    int * acc_in  = (params.psum_in == 0) ? (int*) wts.data() + LAYER_PARAM_WORDS + params.wts_offset : output_psum;
    int output_delta = params.ofm_depth_gran == 16 ? params.ofm_h_skip_sz : params.ofm_h_skip_sz / 2;
    input = reinterpret_cast<int*>((uint8_t*)input + params.ifm_g_skip_sz * params.ssv_group_off);
    // DEBUG_EXEC(printf("output_delta(%d)params.ifm_g_skip_sz(%d)params.ssv_group_off(%d)params.psum_buff_offset(%d)\n", output_delta, params.ifm_g_skip_sz, params.ssv_group_off, params.psum_buff_offset));
    // DEBUG_EXEC(printf("entered outer function\n"));
    // DEBUG_EXEC(printf("ifm(%p)wts(%p)ofm(%p)\n", (int*)ifm.data(), (int*)wts.data(), (int*)ofm.data()));
    // DEBUG_EXEC(printf("input_initial(%p)weights_initial(%p)output_initial(%p)acc_in_initial(%p)psum_in(%d)\n", input, weights, output, acc_in, params.psum_in));

    // DEBUG_EXEC(printf("params.outer_loop_cnt = %d,  params.exec_type = %d\n", params.outer_loop_cnt, params.exec_type));
#ifndef __X86SIM__
    AIE_NO_PREPARE_FOR_PIPELINING
#endif
    for (int i = 0; i < params.outer_loop_cnt; ++i) {
        switch(params.exec_type) {
            case 0:
            case 2:
                gconv2d_generic<batch_size, act_type, out_mode_full, dtype_ifm, dtype_wts, dtype_ofm>(input, weights, acc_in, output, output_psum, params);
                break;
            case 1:
            case 3:
                gconv2d_generic<batch_size,act_type,out_mode_partial,dtype_ifm,dtype_wts,dtype_ofm>(input, weights, acc_in, output, output_psum, params);
                break ;
        }
        // we shall update these pointers every iteration
        input = reinterpret_cast<int*>((dtype_ifm*)(input) + params.ifm_h_skip_sz);
        // weights shall not be changed as for each group we will start from begining each time
        // output pointer is always updated with 1H, while output_psum will always update with 2H
        output = reinterpret_cast<int*>((dtype_ofm*)(output) + output_delta);
        output_psum = reinterpret_cast<int*>((int16*)output_psum + params.ofm_h_skip_sz);
        acc_in = (params.psum_in == 0) ? acc_in : reinterpret_cast<int*>((int16*)(acc_in) + params.ofm_h_skip_sz);
        // the weight pointer is not updated, as normally after finishing traversal G the pointer shall return to its initial position
    }
    output  = (int*) ofm.data();

    // Post process for ofm_depth_per_group % 8 != 0
    if constexpr(enable_post_process == 1) {
        if (params.run_time_enable_post_process == 1){
            switch(params.exec_type) {
                case 0:
                case 2:
                    post_process<dtype_ifm, dtype_ofm>(output, params);
                    break;
            }
        }
    }
}
