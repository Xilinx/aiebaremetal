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
 * 2D Convolution Kernel woth cascade connections for AI Engine
 *
 * Performs 2d convolution on data present in "ifms" using weights from "wts"
 * Result is stored in "ofms". Intermedioate results a re stored in 16-bit precision and cascaded out to neighboring core
 *
 */
// TODO: This has implicit reference to LEAKY_RELU and LOAD_WEIGHTS MACROS from conv2d.h, move this to conv2d.h

#ifndef __CONV2D_CASC_H__
#define __CONV2D_CASC_H__

#include <adf.h>
#include "aie.hpp"
#include "aie_adf.hpp"

#include "utils.h"         // Contains some utility functions and definitions related to intrinsics usage
#include "mllib_const.h"   // Constants shared across graphs like activation type
#include "conv2d_params.h" // Parameters like increments and wraps needed by kernel
//#include "aie_api/utils.hpp" // needed for aie:print
#ifndef MIN_LOOP_RANGE
#define MIN_LOOP_RANGE 4
#endif

using namespace aie;

// cascade generic kernel, use casc_mode defined in conv2d_params to configure the CASC_START/MID/END
template <uint8_t batch_size, act_t act_type,  out_mode_t out_mode, casc_mode_t casc_mode, typename dtype_ifm, typename dtype_wts, typename dtype_ofm, typename config_ifm, typename config_wts, typename config_ofm> 
NOINLINE void conv2d_casc_generic
(
        int * __restrict input,
        int * __restrict weights,
        int * __restrict acc_in,
        int * __restrict output,
        conv2d_params_t            &conv2d_params,
        IN_CASC<acc32> *casc_acc_in,
        OUT_CASC<acc32> *casc_acc_out
) { 
    int16 DM_BANK(b) * p_init16  = (int16 DM_BANK(b) *) acc_in;
    dtype_ofm DM_BANK(b) * p_out;
    if constexpr((casc_mode==CASC_START) || (casc_mode == CASC_MID)) { 
       //p_init16   = (int16      DM_BANK(b) *) output;
       p_out   = (dtype_ofm DM_BANK(b) * ) output;
    }
    if constexpr(casc_mode==CASC_END) { 
       //p_init16   = (int16      DM_BANK(b) *) ((int *) output +  (conv2d_params.psum_buff_offset>>2));//output + (conv2d_params.psum_buff_offset>>1));
       p_out   = (conv2d_params.psum_out==0)?(dtype_ofm DM_BANK(b) *)output : (dtype_ofm DM_BANK(b) *)((int *) output +  (conv2d_params.psum_buff_offset>>2));
	//if constexpr(out_mode==FULL)
	//	p_out = (dtype_ofm DM_BANK(b) *)output;
	//else
		//p_out = (dtype_ofm DM_BANK(b) *) output;
    }

    //int16 DM_BANK(b) * psum_out = (int16 DM_BANK(b) *) acc_in;  

    dtype_ifm IFM_DM_BANK   * p_in    = (dtype_ifm  IFM_DM_BANK *) input;
    dtype_wts  WT_DM_BANK   * p_w     = (dtype_wts   WT_DM_BANK  *) weights;

    v8int16 WT_DM_BANK *p_bias = (v8int16 WT_DM_BANK *)((int *)p_w-conv2d_params.hdr_len+LAYER_PARAM_WORDS); //Pointer to load bias.

    vector<dtype_ifm, 32> vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3;
    //int16      DM_BANK(b) * p_init16 = (int16 DM_BANK(b) *) acc_in;
   // int16      IFM_DM_BANK * p_init16 = (int16 IFM_DM_BANK *) acc_in;

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
    SETUP_ITR3D_COUNTERS(iterator_weights)
    SETUP_ITR3D_COUNTERS(iterator_psum)
    SETUP_ITR2D_COUNTERS(iterator_pout)

    int frac; 
    int chnd   = 32;
    int chnd_T = 64;

    if constexpr(batch_size < 4) {
        p_in += 31 ;
    }

    /*int    inc_ST_0       = (conv2d_params.psum_in ==  0) ? chnd_T                              : 2*chnd_T;
    int    inc_ST_1       = (conv2d_params.psum_in ==  0) ? chnd_T                              : conv2d_params.inc_ST_0;
    int    inc_ST_2       = (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_1+chnd_T      : conv2d_params.inc_ST_1;
    int    inc_ST_3       = (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_2+chnd_T      : chnd_T;    
    
    DECL_ITR3D(iterator_psum)
    SETUP_ITR3D(iterator_psum,
                (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_d0 : 0,
                (conv2d_params.psum_in ==  0) ? conv2d_params.num_X : 0,
                (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_d1 : conv2d_params.inc_Xo_T-chnd_T,
                (conv2d_params.psum_in ==  0) ? conv2d_params.num_Co : conv2d_params.num_X,
                (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_rev : conv2d_params.inc_Co_T-chnd_T)
        */

    DECL_ITR3D(iterator_psum)
    SETUP_ITR3D(iterator_psum,
                0,
                0,
                conv2d_params.inc_Xo_T-chnd_T-32,
                conv2d_params.num_X,
                conv2d_params.inc_Co_T-chnd_T-32)

    //int    acc_init_shift = (conv2d_params.psum_in ==  0) ? (int) conv2d_params.shift_bias_init : (int) conv2d_params.shift_psum_in;    
    
    DECL_ITR2D(iterator_pout)
    SETUP_ITR2D(iterator_pout,
               (conv2d_params.psum_out == 0)? conv2d_params.inc_Xo-chnd : conv2d_params.inc_Xo_T-chnd_T,
               conv2d_params.num_X,
               (conv2d_params.psum_out == 0)? conv2d_params.inc_Co-chnd : conv2d_params.inc_Co_T-chnd_T)
    //change : hardcoded for 16bit mode
    /*DECL_ITR2D(iterator_pout)
    SETUP_ITR2D(iterator_pout,
               conv2d_params.inc_Xo_T-chnd_T,
               conv2d_params.num_X,
               conv2d_params.inc_Co_T-chnd_T) */
// Trick to initilize with zeros
/*
    int16 chess_storage(%chess_alignof(v16int16)) zero_vec[32] = {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000} ;//zeros<int16,32>();
    //v32int16 zero_vec = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} ;//zeros<int16,32>();
    int16 * acc_init_ptr = p_init16 ;
    int final_acc_init_shift = acc_init_shift ;
    if constexpr((casc_mode == CASC_END) || (casc_mode == CASC_MID)) { 
        final_acc_init_shift = (conv2d_params.psum_in == 0)?16:acc_init_shift ;
        acc_init_ptr = (conv2d_params.psum_in == 0)?zero_vec:p_init16 ;
    }
*/

    //int * ptr = (int *) &conv2d_params;

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (unsigned j=0; j<conv2d_params.outer_g; j++){
	/*printf("p_in %d\n",p_in);
	printf("p_w %d\n",p_w);
	printf("p_init %d\n",p_init16);
	printf("p_bias %d\n",p_bias);
	printf("p_out %d\n",p_out);*/

	//for(int i=0; i<288/4; i++)
	//   printf("conv2d_params %d\n", ptr[i]);	

	//chess_report(conv2d_params);
	//chess_report(p_w);
	//chess_report(p_bias);
	//chess_report(p_init16);
	//chess_report(p_out);
	
	//printf("PRINT VALUES %d %d %d %d %d %d\n",conv2d_params.inc_ST_0, conv2d_params.inc_ST_1, iterator_weights_cnt1, conv2d_params.inc_A, conv2d_params.inc_S_0, conv2d_params.inc_S_1);
	

        if constexpr(batch_size < 4) {
            v64int8 Sbuff;
            int pdel;
            pdel = (long)p_in;
            frac = (pdel & 31) + 33;
        }
        accum<acc32,32> acc0,acc1,acc2,acc3,acc4,acc5,acc6,acc7;
       
        // Initialize with "bias/2" symmetrically on both cascade nodes (instead of "bias" on one node)
         
        /*acc0.from_vector( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T); 
        acc1.from_vector( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_1-chnd_T); 
        acc2.from_vector( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T); 
        acc3.from_vector( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_2-chnd_T); 
        acc4.from_vector( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T); 
        acc5.from_vector( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_1-chnd_T); 
        acc6.from_vector( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T); 
        acc7.from_vector( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_3-chnd_T); 
        */
   // if constexpr((casc_mode == CASC_END) || (casc_mode == CASC_MID)) 
   //     print(acc0,true,"\nacc0=");
   
        v64int8 vec_Xbuff0_, vec_Xbuff1_, vec_Xbuff2_, vec_Xbuff3_;
       
        if constexpr(batch_size < 4) {
                        
            vector<uint8_t, 64>  Sbuff0_, Sbuff1_,Xbuff0_,Xbuff1_,Xbuff2_,Xbuff3_;

            // Load IFM data, two 4x8 matrices plus additional adjacent data needed for shift alignment
            Sbuff0_.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            Sbuff0_.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            Sbuff1_.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            Sbuff1_.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in)); 

            Xbuff.x0 = shiftx(Xbuff.x0, Sbuff0_, conv2d_params.step_align, frac);
            Xbuff.x2 = shiftx(Xbuff.x2, Sbuff1_, conv2d_params.step_align, frac);
            
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,conv2d_params)
            
            Xbuff0_ = interleave(Xbuff.x0, Xbuff.x2, conv2d_params.shfl_0);
            Xbuff1_ = interleave(Xbuff0_,   unu8, T256_2x2_hi);
            Xbuff2_ = interleave(Xbuff.x0, Xbuff.x2, conv2d_params.shfl_1);
            Xbuff3_ = interleave(Xbuff2_,   unu8, T256_2x2_hi);

            vec_Xbuff0_  = Xbuff0_.cast_to<int8_t>(); 
            vec_Xbuff1_  = Xbuff1_.cast_to<int8_t>(); 
            vec_Xbuff2_  = Xbuff2_.cast_to<int8_t>(); 
            vec_Xbuff3_  = Xbuff3_.cast_to<int8_t>();
        } 
        else {
            
            vec_Xbuff0_ = set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in);
            p_in = byte_incr(p_in, conv2d_params.inc_A);
            
            vec_Xbuff1_ =  set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); 
            p_in = byte_incr(p_in, conv2d_params.inc_A);
            
            vec_Xbuff2_ =  set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in); 
            p_in = byte_incr(p_in, conv2d_params.inc_A);
            
            vec_Xbuff3_ =  set_v64int8(0,*(v32int8 IFM_DM_BANK *) p_in);
            
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner,conv2d_params)        
        }     
            
                        
        v64int8 Ybuff0_ = *(v64int8 IFM_DM_BANK *) p_w; p_w+=64;
        v64int8 Ybuff1_ = *(v64int8 IFM_DM_BANK *) p_w; p_w+=64;

        //mmul Cbuff0(acc0); mmul Cbuff1(acc1); mmul Cbuff2(acc2); mmul Cbuff3(acc3); 
        //mmul Cbuff4(acc4); mmul Cbuff5(acc5); mmul Cbuff6(acc6); mmul Cbuff7(acc7); 

	aie::accum<acc32, 32> accum_0, accum_1, accum_2, accum_3, accum_4, accum_5, accum_6, accum_7;         

        accum_0 = insert(accum_0,0,sups(*(v16int16 DM_BANK(b) *)p_init16,conv2d_params.shift_psum_in,1));
        p_init16 = byte_incr(p_init16, 32);
        accum_0 = insert(accum_0,1,sups(*(v16int16 DM_BANK(b) *)p_init16,conv2d_params.shift_psum_in,1));
        p_init16 = byte_incr(p_init16, 32);

        accum_1 = insert(accum_1,0,sups(*(v16int16 DM_BANK(b) *)p_init16,conv2d_params.shift_psum_in,1));
        p_init16 = byte_incr(p_init16, 32);
        accum_1 = insert(accum_1,1,sups(*(v16int16 DM_BANK(b) *)p_init16,conv2d_params.shift_psum_in,1));
        p_init16 = byte_incr(p_init16, conv2d_params.inc_ST_0 - chnd_T -32);

        accum_2 = insert(accum_2,0,sups(*(v16int16 DM_BANK(b) *)p_init16,conv2d_params.shift_psum_in,1));
        p_init16 = byte_incr(p_init16, 32);
        accum_2 = insert(accum_2,1,sups(*(v16int16 DM_BANK(b) *)p_init16,conv2d_params.shift_psum_in,1));
        p_init16 = byte_incr(p_init16, 32);

        accum_3 = insert(accum_3,0,sups(*(v16int16 DM_BANK(b) *)p_init16,conv2d_params.shift_psum_in,1));
        p_init16 = byte_incr(p_init16, 32);
        accum_3 = insert(accum_3,1,sups(*(v16int16 DM_BANK(b) *)p_init16,conv2d_params.shift_psum_in,1));
        p_init16 = byte_incr(p_init16, conv2d_params.inc_ST_1 - chnd_T -32);

        accum_4 = insert(accum_4,0,sups(*(v16int16 DM_BANK(b) *)p_init16,conv2d_params.shift_psum_in,1));
        p_init16 = byte_incr(p_init16, 32);
        accum_4 = insert(accum_4,1,sups(*(v16int16 DM_BANK(b) *)p_init16,conv2d_params.shift_psum_in,1));
        p_init16 = byte_incr(p_init16, 32);

        accum_5 = insert(accum_5,0,sups(*(v16int16 DM_BANK(b) *)p_init16,conv2d_params.shift_psum_in,1));
        p_init16 = byte_incr(p_init16, 32);
        accum_5 = insert(accum_5,1,sups(*(v16int16 DM_BANK(b) *)p_init16,conv2d_params.shift_psum_in,1));
        p_init16 = byte_incr(p_init16, conv2d_params.inc_ST_0 - chnd_T -32);

        accum_6 = insert(accum_6,0,sups(*(v16int16 DM_BANK(b) *)p_init16,conv2d_params.shift_psum_in,1));
        p_init16 = byte_incr(p_init16, 32);
        accum_6 = insert(accum_6,1,sups(*(v16int16 DM_BANK(b) *)p_init16,conv2d_params.shift_psum_in,1)); 
        p_init16 = byte_incr(p_init16, 32);

        accum_7 = insert(accum_7,0,sups(*(v16int16 DM_BANK(b) *)p_init16,conv2d_params.shift_psum_in,1));
        p_init16 = byte_incr(p_init16, 32);
        accum_7 = insert(accum_7,1,sups(*(v16int16 DM_BANK(b) *)p_init16,conv2d_params.shift_psum_in,1)); 
        //p_init16 = byte_incr(p_init16, -32);                               
        p_init16 = INCR_ITR3D(p_init16, iterator_psum)

	v32uint16 bias_shift = (v32uint16)broadcast_s32((1-conv2d_params.psum_in) << conv2d_params.shift_bias_init);                 

        int32_t chess_storage(dj2) bias_incr = 128*iterator_weights_cnt1;
        
        v32acc32 chess_storage(cm8) bias_0 = mul_4x2_2x8(bias_shift, 0, set_v32int16(0,*byte_incr(p_bias,bias_incr)),1);   
         
        accum_0 = addmac_4x8_8x8_conf(vec_Xbuff0_,conv2d_params.ifm_sign,Ybuff0_,1,accum_0,bias_0,1-conv2d_params.psum_in,0,0,0,0);
        accum_1 = addmac_4x8_8x8_conf(vec_Xbuff1_,conv2d_params.ifm_sign,Ybuff0_,1,accum_1,bias_0,1-conv2d_params.psum_in,0,0,0,0);
        accum_2 = addmac_4x8_8x8_conf(vec_Xbuff2_,conv2d_params.ifm_sign,Ybuff0_,1,accum_2,bias_0,1-conv2d_params.psum_in,0,0,0,0);
        accum_3 = addmac_4x8_8x8_conf(vec_Xbuff3_,conv2d_params.ifm_sign,Ybuff0_,1,accum_3,bias_0,1-conv2d_params.psum_in,0,0,0,0);                

        v32acc32 chess_storage(cm8) bias_1 = mul_4x2_2x8(bias_shift, 0, set_v32int16(0,*byte_incr(p_bias,bias_incr+64)), 1);

        accum_4 = addmac_4x8_8x8_conf(vec_Xbuff0_,conv2d_params.ifm_sign,Ybuff1_,1,accum_4,bias_1,1-conv2d_params.psum_in,0,0,0,0);
        accum_5 = addmac_4x8_8x8_conf(vec_Xbuff1_,conv2d_params.ifm_sign,Ybuff1_,1,accum_5,bias_1,1-conv2d_params.psum_in,0,0,0,0);
        accum_6 = addmac_4x8_8x8_conf(vec_Xbuff2_,conv2d_params.ifm_sign,Ybuff1_,1,accum_6,bias_1,1-conv2d_params.psum_in,0,0,0,0);
        accum_7 = addmac_4x8_8x8_conf(vec_Xbuff3_,conv2d_params.ifm_sign,Ybuff1_,1,accum_7,bias_1,1-conv2d_params.psum_in,0,0,0,0);

	if constexpr(batch_size < 4) {
            int pdel = (long)p_in; 
            frac = (pdel & 31) + 33;
        }

	mmul Cbuff0( accum_0 ); 
        mmul Cbuff1( accum_1 );  
        mmul Cbuff2( accum_2 ); 
        mmul Cbuff3( accum_3 ); 
        mmul Cbuff4( accum_4 ); 
        mmul Cbuff5( accum_5 );  
        mmul Cbuff6( accum_6 ); 
        mmul Cbuff7( accum_7 );

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(MIN_LOOP_RANGE,) 
        for (int i=0; i<conv2d_params.inner_g-1; i++){
            if constexpr(batch_size < 4) {
                vector<uint8_t, 64>  Sbuff0, Sbuff1,Xbuff0,Xbuff1,Xbuff2,Xbuff3;

                // Load IFM data, two 4x8 matrices plus additional adjacent data needed for shift alignment
                Sbuff0.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
                Sbuff0.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
                Sbuff1.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
                Sbuff1.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in)); 
            
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
        //p_init16 = INCR_ITR3D(p_init16, iterator_psum)

        // Write out the final outputs
        if constexpr(casc_mode == CASC_END){ 
           if constexpr(out_mode == FULL){
                  accum<acc32,32> temp_acc0,temp_acc1,temp_acc2,temp_acc3,temp_acc4,temp_acc5,temp_acc6,temp_acc7;
                  accum<acc32,32> final_acc0,final_acc1,final_acc2,final_acc3,final_acc4,final_acc5,final_acc6,final_acc7;
	          temp_acc0 = readincr_v<32>(casc_acc_in) ;
                  temp_acc1 = readincr_v<32>(casc_acc_in) ;
	          temp_acc2 = readincr_v<32>(casc_acc_in) ;
	          temp_acc3 = readincr_v<32>(casc_acc_in) ;
	          temp_acc4 = readincr_v<32>(casc_acc_in) ;
	          temp_acc5 = readincr_v<32>(casc_acc_in) ;
	          temp_acc6 = readincr_v<32>(casc_acc_in) ;
	          temp_acc7 = readincr_v<32>(casc_acc_in) ;
                  //print(temp_acc0,true,"acc0=");
                  
                 vector<dtype_ifm,64> vec0 = zeros<dtype_ifm,64>();
                 final_acc0=add(temp_acc0,Cbuff0.to_accum());
                 final_acc1=add(temp_acc1,Cbuff1.to_accum());
                 final_acc2=add(temp_acc2,Cbuff2.to_accum());
                 final_acc3=add(temp_acc3,Cbuff3.to_accum());
                 final_acc4=add(temp_acc4,Cbuff4.to_accum());
                 final_acc5=add(temp_acc5,Cbuff5.to_accum());
                 final_acc6=add(temp_acc6,Cbuff6.to_accum());
                 final_acc7=add(temp_acc7,Cbuff7.to_accum());
                // print(Cbuff0.to_accum(),true,"\nCbuff0=");

                 mmul Cbuff0_final(final_acc0); mmul Cbuff1_final(final_acc1); mmul Cbuff2_final(final_acc2); mmul Cbuff3_final(final_acc3); 
                 mmul Cbuff4_final(final_acc4); mmul Cbuff5_final(final_acc5); mmul Cbuff6_final(final_acc6); mmul Cbuff7_final(final_acc7); 
               if constexpr(act_type==LINEAR || act_type==RELU){
                   #ifdef USE_RTP_ACT
                     int out_sign=conv2d_params.out_sign;
                   #else
                     int out_sign = 1-act_type; // Note enum for act types are: 0,1,2 -> linear, relu, lrelu
                   #endif
                       store_v(p_out, Cbuff0_final.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32); 
                       store_v(p_out, Cbuff1_final.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
                       store_v(p_out, Cbuff2_final.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                       store_v(p_out, Cbuff3_final.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_1-chnd);
                       store_v(p_out, Cbuff4_final.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                       store_v(p_out, Cbuff5_final.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
                       store_v(p_out, Cbuff6_final.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                       store_v(p_out, Cbuff7_final.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out));
               } else{
                // Code/Macro for Leaky reLU
                   LEAKY_RELU_COMPUTE(Cbuff0_final,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                   p_out = byte_incr(p_out, 32);
                
                   LEAKY_RELU_COMPUTE(Cbuff1_final,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                   p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
                
                   LEAKY_RELU_COMPUTE(Cbuff2_final,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                   p_out = byte_incr(p_out, 32);
                
                   LEAKY_RELU_COMPUTE(Cbuff3_final,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                   p_out = byte_incr(p_out, conv2d_params.inc_S_1-chnd);
                
                   LEAKY_RELU_COMPUTE(Cbuff4_final,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                   p_out = byte_incr(p_out, 32);
                
                   LEAKY_RELU_COMPUTE(Cbuff5_final,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                   p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
                
                   LEAKY_RELU_COMPUTE(Cbuff6_final,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                   p_out = byte_incr(p_out, 32);
                
                   LEAKY_RELU_COMPUTE(Cbuff7_final,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
               }
           }else{ // psum=1,store partial sums as psum iters are not done yet
             // Store partial sum
                store_v((int16 DM_BANK(b) *) p_out, Cbuff0.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(b) *)byte_incr((int16*)p_out, 64); 
                store_v((int16 DM_BANK(b) *) p_out, Cbuff1.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(b) *)byte_incr((int16*)p_out, conv2d_params.inc_ST_0-chnd_T);
                store_v((int16 DM_BANK(b) *) p_out, Cbuff2.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(b) *)byte_incr((int16*)p_out, 64);
                store_v((int16 DM_BANK(b) *) p_out, Cbuff3.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(b) *)byte_incr((int16*)p_out, conv2d_params.inc_ST_1-chnd_T);
                store_v((int16 DM_BANK(b) *) p_out, Cbuff4.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(b) *)byte_incr((int16*)p_out, 64);
                store_v((int16 DM_BANK(b) *) p_out, Cbuff5.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(b) *)byte_incr((int16*)p_out, conv2d_params.inc_ST_0-chnd_T);
                store_v((int16 DM_BANK(b) *) p_out, Cbuff6.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(b) *)byte_incr((int16*)p_out, 64);
                store_v((int16 DM_BANK(b) *) p_out, Cbuff7.template to_vector<int16>(conv2d_params.shift_psum_out));             
           }
       }
        if constexpr((casc_mode == CASC_START) || (casc_mode == CASC_MID)) { 
           if constexpr(out_mode == FULL){
            // Write to cascade
            writeincr(casc_acc_out,Cbuff0.to_accum());p_out = byte_incr(p_out, 32); 
            writeincr(casc_acc_out,Cbuff1.to_accum());p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
            writeincr(casc_acc_out,Cbuff2.to_accum());p_out = byte_incr(p_out, 32); 
            writeincr(casc_acc_out,Cbuff3.to_accum());p_out = byte_incr(p_out, conv2d_params.inc_S_1-chnd);
            writeincr(casc_acc_out,Cbuff4.to_accum());p_out = byte_incr(p_out, 32); 
            writeincr(casc_acc_out,Cbuff5.to_accum());p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
            writeincr(casc_acc_out,Cbuff6.to_accum());p_out = byte_incr(p_out, 32); 
            writeincr(casc_acc_out,Cbuff7.to_accum());
           } else{
           // Store partial sum
              store_v((int16 DM_BANK(b) *) p_out, Cbuff0.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(b) *)byte_incr((int16*)p_out, 64); 
              store_v((int16 DM_BANK(b) *) p_out, Cbuff1.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(b) *)byte_incr((int16*)p_out, conv2d_params.inc_ST_0-chnd_T);
              store_v((int16 DM_BANK(b) *) p_out, Cbuff2.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(b) *)byte_incr((int16*)p_out, 64);
              store_v((int16 DM_BANK(b) *) p_out, Cbuff3.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(b) *)byte_incr((int16*)p_out, conv2d_params.inc_ST_1-chnd_T);
              store_v((int16 DM_BANK(b) *) p_out, Cbuff4.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(b) *)byte_incr((int16*)p_out, 64);
              store_v((int16 DM_BANK(b) *) p_out, Cbuff5.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(b) *)byte_incr((int16*)p_out, conv2d_params.inc_ST_0-chnd_T);
              store_v((int16 DM_BANK(b) *) p_out, Cbuff6.template to_vector<int16>(conv2d_params.shift_psum_out)); p_out = (dtype_ofm DM_BANK(b) *)byte_incr((int16*)p_out, 64);
              store_v((int16 DM_BANK(b) *) p_out, Cbuff7.template to_vector<int16>(conv2d_params.shift_psum_out));             
           }
        }
        p_out = INCR_ITR2D(p_out,iterator_pout)
    }
}

// cascade generic kernel optimized for stride=2, use casc_mode defined in conv2d_params to configure the CASC_START/MID/END
template <uint8_t batch_size, act_t act_type,  out_mode_t out_mode, casc_mode_t casc_mode, typename dtype_ifm, typename dtype_wts, typename dtype_ofm, typename config_ifm, typename config_wts, typename config_ofm> 
INLINE void conv2d_casc_generic_stride2_opt
(
        int * __restrict input,
        int * __restrict weights,
        int * __restrict acc_in,
        int * __restrict output,
        conv2d_params_t            &conv2d_params,
        IN_CASC<acc32> *casc_acc_in,
        OUT_CASC<acc32> *casc_acc_out
) { 
    int16 * p_sum;
    dtype_ofm * p_out;
    if constexpr((casc_mode==CASC_START) || (casc_mode == CASC_MID)) { 
       p_sum   = (int16      DM_BANK(c) *) output;
       p_out   = (dtype_ofm DM_BANK(c) * ) output;
    }
    if constexpr(casc_mode==CASC_END) { 
     p_sum   = ((int16      DM_BANK(c) *) output + (conv2d_params.psum_buff_offset>>1));
     p_out   = (conv2d_params.psum_out==0)?(dtype_ofm DM_BANK(c) *)output : (dtype_ofm DM_BANK(c) *)p_sum;
    }

    dtype_ifm IFM_DM_BANK   * p_in    = (dtype_ifm  IFM_DM_BANK *) input;
    dtype_wts  DM_BANK(b)   * p_w     = (dtype_wts   DM_BANK(b)  *) weights;



    vector<dtype_ifm, 32> vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3;
    int16      DM_BANK(c) * p_init16 = (int16 DM_BANK(c) *) acc_in;
   // int16      IFM_DM_BANK * p_init16 = (int16 IFM_DM_BANK *) acc_in;



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
    SETUP_ITR3D_COUNTERS(iterator_weights)
    SETUP_ITR3D_COUNTERS(iterator_psum)
    SETUP_ITR2D_COUNTERS(iterator_pout)

    // Stride=2 optimization 
    int chnd   = 32;
    int chnd_T = 64;

    int cnt_frac  = 31;
    p_in = byte_incr(p_in, 31);

    int    inc_ST_0       = (conv2d_params.psum_in ==  0) ? chnd_T                              : 2*chnd_T;
    int    inc_ST_1       = (conv2d_params.psum_in ==  0) ? chnd_T                              : conv2d_params.inc_ST_0;
    int    inc_ST_2       = (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_1+chnd_T      : conv2d_params.inc_ST_1;
    int    inc_ST_3       = (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_2+chnd_T      : chnd_T;    
    
    DECL_ITR3D(iterator_psum)
    SETUP_ITR3D(iterator_psum,
                (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_d0 : 0,
                (conv2d_params.psum_in ==  0) ? conv2d_params.num_X : 0,
                (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_d1 : conv2d_params.inc_Xo_T-chnd_T,
                (conv2d_params.psum_in ==  0) ? conv2d_params.num_Co : conv2d_params.num_X,
                (conv2d_params.psum_in ==  0) ? conv2d_params.inc_acc_rev : conv2d_params.inc_Co_T-chnd_T)
        
    int    acc_init_shift = (conv2d_params.psum_in ==  0) ? (int) conv2d_params.shift_bias_init : (int) conv2d_params.shift_psum_in;    
    
    DECL_ITR2D(iterator_pout)
    SETUP_ITR2D(iterator_pout,
               (conv2d_params.psum_out == 0)? conv2d_params.inc_Xo-chnd : conv2d_params.inc_Xo_T-chnd_T,
               conv2d_params.num_X,
               (conv2d_params.psum_out == 0)? conv2d_params.inc_Co-chnd : conv2d_params.inc_Co_T-chnd_T)
     
    
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (unsigned j=0; j<conv2d_params.outer_g; j++){
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
            int frac = (cnt_frac & 31) + 33;
            cnt_frac = (cnt_frac==conv2d_params.num_Kx*((conv2d_params.step_align!=0)<<(conv2d_params.step_align+1))+31 ? 31 : cnt_frac+((conv2d_params.step_align!=0)<<(conv2d_params.step_align+1)));        
        
            vector<uint8_t, 64>  Sbuff0, Sbuff1, Sbuff2, Sbuff3, Xbuff0, Xbuff1, Xbuff2, Xbuff3;

            Sbuff0.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            Sbuff0.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A_1);            
            Sbuff1.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            Sbuff1.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A_1);            
            Sbuff2.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            Sbuff2.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A_1);            
            Sbuff3.insert( 0,  load_v<32>((uint8 IFM_DM_BANK *) p_in));  p_in = byte_incr(p_in, conv2d_params.inc_A);
            Sbuff3.insert( 1,  load_v<32>((uint8 IFM_DM_BANK *) p_in));              
                    
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
        if constexpr(casc_mode == CASC_END){ 
           if constexpr(out_mode == FULL){
                  accum<acc32,32> temp_acc0,temp_acc1,temp_acc2,temp_acc3,temp_acc4,temp_acc5,temp_acc6,temp_acc7;
                  accum<acc32,32> final_acc0,final_acc1,final_acc2,final_acc3,final_acc4,final_acc5,final_acc6,final_acc7;
	          temp_acc0 = readincr_v<32>(casc_acc_in) ;
                  temp_acc1 = readincr_v<32>(casc_acc_in) ;
	          temp_acc2 = readincr_v<32>(casc_acc_in) ;
	          temp_acc3 = readincr_v<32>(casc_acc_in) ;
	          temp_acc4 = readincr_v<32>(casc_acc_in) ;
	          temp_acc5 = readincr_v<32>(casc_acc_in) ;
	          temp_acc6 = readincr_v<32>(casc_acc_in) ;
	          temp_acc7 = readincr_v<32>(casc_acc_in) ;
                  //print(temp_acc0,true,"acc0=");
                  
                 vector<dtype_ifm,64> vec0 = zeros<dtype_ifm,64>();
                 final_acc0=add(temp_acc0,Cbuff0.to_accum());
                 final_acc1=add(temp_acc1,Cbuff1.to_accum());
                 final_acc2=add(temp_acc2,Cbuff2.to_accum());
                 final_acc3=add(temp_acc3,Cbuff3.to_accum());
                 final_acc4=add(temp_acc4,Cbuff4.to_accum());
                 final_acc5=add(temp_acc5,Cbuff5.to_accum());
                 final_acc6=add(temp_acc6,Cbuff6.to_accum());
                 final_acc7=add(temp_acc7,Cbuff7.to_accum());
                // print(Cbuff0.to_accum(),true,"\nCbuff0=");

                 mmul Cbuff0_final(final_acc0); mmul Cbuff1_final(final_acc1); mmul Cbuff2_final(final_acc2); mmul Cbuff3_final(final_acc3); 
                 mmul Cbuff4_final(final_acc4); mmul Cbuff5_final(final_acc5); mmul Cbuff6_final(final_acc6); mmul Cbuff7_final(final_acc7); 
               if constexpr(act_type==LINEAR || act_type==RELU){
                   #ifdef USE_RTP_ACT
                     int out_sign=conv2d_params.out_sign;
                   #else
                     int out_sign = 1-act_type; // Note enum for act types are: 0,1,2 -> linear, relu, lrelu
                   #endif
                       store_v(p_out, Cbuff0_final.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32); 
                       store_v(p_out, Cbuff1_final.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
                       store_v(p_out, Cbuff2_final.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                       store_v(p_out, Cbuff3_final.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_1-chnd);
                       store_v(p_out, Cbuff4_final.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                       store_v(p_out, Cbuff5_final.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
                       store_v(p_out, Cbuff6_final.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out)); p_out = byte_incr(p_out, 32);
                       store_v(p_out, Cbuff7_final.template to_vector_sign<dtype_ofm>(out_sign,conv2d_params.shift_out));
                  
               } else{
                // Code/Macro for Leaky reLU
                   LEAKY_RELU_COMPUTE(Cbuff0_final,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                   p_out = byte_incr(p_out, 32);
                
                   LEAKY_RELU_COMPUTE(Cbuff1_final,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                   p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
                
                   LEAKY_RELU_COMPUTE(Cbuff2_final,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                   p_out = byte_incr(p_out, 32);
                
                   LEAKY_RELU_COMPUTE(Cbuff3_final,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                   p_out = byte_incr(p_out, conv2d_params.inc_S_1-chnd);
                
                   LEAKY_RELU_COMPUTE(Cbuff4_final,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                   p_out = byte_incr(p_out, 32);
                
                   LEAKY_RELU_COMPUTE(Cbuff5_final,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                   p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
                
                   LEAKY_RELU_COMPUTE(Cbuff6_final,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
                   p_out = byte_incr(p_out, 32);
                
                   LEAKY_RELU_COMPUTE(Cbuff7_final,conv2d_params.shift_lrelu_input,conv2d_params.lrelu_alpha,conv2d_params.shift_lrelu_alpha,conv2d_params.shift_lrelu_out,p_out)
               }
           }else{ // psum=1,store partial sums as psum iters are not done yet
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
       }
        if constexpr((casc_mode == CASC_START) || (casc_mode == CASC_MID)) { 
           if constexpr(out_mode == FULL){
            // Write to cascade
            writeincr(casc_acc_out,Cbuff0.to_accum());p_out = byte_incr(p_out, 32); 
            writeincr(casc_acc_out,Cbuff1.to_accum());p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
            writeincr(casc_acc_out,Cbuff2.to_accum());p_out = byte_incr(p_out, 32); 
            writeincr(casc_acc_out,Cbuff3.to_accum());p_out = byte_incr(p_out, conv2d_params.inc_S_1-chnd);
            writeincr(casc_acc_out,Cbuff4.to_accum());p_out = byte_incr(p_out, 32); 
            writeincr(casc_acc_out,Cbuff5.to_accum());p_out = byte_incr(p_out, conv2d_params.inc_S_0-chnd);
            writeincr(casc_acc_out,Cbuff6.to_accum());p_out = byte_incr(p_out, 32); 
            writeincr(casc_acc_out,Cbuff7.to_accum());
           } else{
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
        }
        p_out = INCR_ITR2D(p_out,iterator_pout)
    }
} // end of conv2d_casc_generic_stride2_opt


template <uint8_t batch_size, act_t act_type, casc_mode_t casc_mode, typename dtype_ifm, typename dtype_wts, typename dtype_ofm, typename config_ifm, typename config_wts, typename config_ofm> 
void conv2d
(
        adf::input_buffer_conf<dtype_ifm, config_ifm>  & __restrict ifm,
        adf::input_buffer_conf<dtype_wts, config_wts>  & __restrict wts,
        adf::output_buffer_conf<dtype_ofm, config_ofm> &  ofm,
        conv2d_params_t            &conv2d_params,
        IN_CASC<acc32> *casc_acc_in,
        OUT_CASC<acc32> *casc_acc_out
) {
    // Setup iteration level parameters like IO mode etc
    setup_conv2d_iter_params(conv2d_params) ;
    int * acc_in ;
    int * psum;
    if constexpr(casc_mode == CASC_END) {
          acc_in  = (conv2d_params.psum_in == 0) ? (int*) wts.data() + LAYER_PARAM_WORDS : ((int*) ofm.data() + (conv2d_params.psum_buff_offset>>2));
	  psum  = /*(conv2d_params.psum_in == 0) ? (int*) wts.data() + LAYER_PARAM_WORDS :*/ ((int*) ofm.data() + (conv2d_params.psum_buff_offset>>2));
          // Extra logic for CASC END node to map exec type back to 2 when we have 16-bit output (conv2d_params.out_mode=1)
          // TODO: remove this and update setup_iter_params with casc_type template parameter
         /*
         if((conv2d_params.out_mode==1) && (conv2d_params.curr_depth_iter==0)){
             conv2d_params.exec_type = 1;
             // TODO: Remove this and use a macro to update exec type
             #ifdef STRIDE2_OPT
             conv2d_params.exec_type  = conv2d_params.exec_type+(4*conv2d_params.stride_bits*(conv2d_params.batch < 4)*(conv2d_params.conv_type!=CONV2D_7x7S2_LYR1));
             #endif
         }
         */
    }
    if constexpr(casc_mode == CASC_START || casc_mode == CASC_MID) {
          acc_in  = (conv2d_params.psum_in == 0) ? (int*) wts.data() + LAYER_PARAM_WORDS : ((int*) ofm.data() );
	  psum  = /*(conv2d_params.psum_in == 0) ? (int*) wts.data() + LAYER_PARAM_WORDS :*/ ((int*) ofm.data() );
    }
    int * input   = (int*) ifm.data();
    int * weights = (int*) wts.data() + conv2d_params.hdr_len;
    int * output  = (int*) ofm.data(); 

    switch(conv2d_params.exec_type) {
    case 0:
    case 2:
           conv2d_casc_generic <batch_size, act_type, FULL,casc_mode,dtype_ifm,dtype_wts,dtype_ofm,config_ifm,config_wts,config_ofm>(
           input, weights, psum,output,conv2d_params,casc_acc_in, casc_acc_out);
         break ;
    case 1:
    case 3:
	chess_separator();
           conv2d_casc_generic <batch_size, act_type, PARTIAL,casc_mode,dtype_ifm,dtype_wts,dtype_ofm,config_ifm,config_wts,config_ofm>(
           input, weights, psum,output,conv2d_params,casc_acc_in, casc_acc_out);
	chess_separator();
         break ;
    #ifdef STRIDE2_OPT
    case 4:
    case 6:
           conv2d_casc_generic_stride2_opt <batch_size, act_type, FULL,casc_mode,dtype_ifm,dtype_wts,dtype_ofm,config_ifm,config_wts,config_ofm>(
           input, weights, acc_in,output,conv2d_params,casc_acc_in, casc_acc_out);
         break ;
    case 5:
    case 7:
           conv2d_casc_generic_stride2_opt <batch_size, act_type, PARTIAL,casc_mode,dtype_ifm,dtype_wts,dtype_ofm,config_ifm,config_wts,config_ofm>(
           input, weights, acc_in,output,conv2d_params,casc_acc_in, casc_acc_out);
         break ;
    #endif
    //chess_schedule_separator();
    }
}

#endif //__CONV2D_CASC_H__

