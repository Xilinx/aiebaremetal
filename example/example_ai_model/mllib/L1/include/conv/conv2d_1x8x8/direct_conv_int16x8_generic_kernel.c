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


#include "direct_conv_int16x8_generic_kernel.h"
#include "direct_conv_int16x8_generic_params.h"
#include "direct_conv_int16x8_generic_template.h"

// Refer to overlay_gemm.py for these address in L1
#define QDQ_PARAM_ADDR 54272 
#define GEMM_TDM1_ADDR 8192
#define GEMM_TDM2_ADDR 24576 
#define GEMM_IFM_SUM_ADDR 54336 
#define GEMM_A16W8_MSUBV 32
#define GEMM_A16W8_KSUBV 128
#define GEMM_A16W8_NSUBV 64
#define IFM_SIZE (GEMM_A16W8_MSUBV*GEMM_A16W8_KSUBV*2)
#define WGT_SIZE (GEMM_A16W8_KSUBV*GEMM_A16W8_NSUBV*1)
#define OFM_SIZE (GEMM_A16W8_MSUBV*GEMM_A16W8_NSUBV*2)


#ifndef IFM_SUM_OFFSET
#define IFM_SUM_OFFSET (ofm_size)
#endif


#ifndef INLINE

#include "./qdq/qdq_helpers.hpp"
#include "./qdq/qdq.cc"


int col_id = get_coreid() >> 16;
int row_id = get_coreid() & 0xf;


inline void sum_conv_2d_c1( int32 * buffer, int Y, int X, int Ky, int Kx, int Sx, int stepY, int stepKy ) {
    v16int32 * pI = ( v16int32 * ) buffer;
    int32 * restrict pO = buffer;
    constexpr unsigned N = 8;
    fifo_state_t f;
    f.pos = 0;
    addr_t cntI = 0;
    addr_t cntO = 0;
    int reset = -64;
    int inc0 = add_dimension( reset, Kx-1, 4 );
    int inc1 = stepKy + reset;
    reset = -Ky * stepKy;
    int inc2 = add_dimension( reset, X/N-1, 4*N*Sx );
    int inc3 = stepY + reset;
    for ( unsigned pix = 0; pix < Y * X / N; pix++ ) {
        aie::vector<int32,N> sum = aie::zeros<int32,N>( );
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE( 4, ) 
        for ( unsigned k = 0; k < Ky * Kx; k++ ) 
        {
            fifo_ld_fill( pI, f );
            //aie::vector<int32,16> val = fifo_ld_pop_2d_byte( pI, f, inc1, Kx-1, cntI, inc0 );
            aie::vector<int32,16> val = shuffle( fifo_ld_pop_2d_byte( pI, f, inc1, Kx-1, cntI, inc0 ), Sx > 1 ? T32_16x2_lo : T512_1x2_lo );
            sum = aie::add( sum, val.extract<N>( 0 ));
        }
        aie::store_v( pO, sum );
        pO += N;
        pI = add_2d_byte( pI, inc3, X/N-1, cntO, inc2 ); 
    }
}


static void __attribute__((noinline)) direct_conv_int16x8_generic
(
        int16_t * input,
        int8_t * weights,
        int8_t * weight_unpack,
        int32_t * tdm1,
        int32_t * tdm2,
        const KernelParams &params_s,
        bool final_tdm_iter=0,
        int op_mode_s = OP_NONE
) {
    Increments incrs;
    KernelParams params = params_s;
    params.op_mode = op_mode_s == OP_NONE ? params.op_mode : op_mode_s;
    bool no_stride = params.S_g == 1;

    setup_parameters_dc_int16x8_generic( params, incrs, ( long ) tdm2 - ( long ) tdm1 );
    
    KernelControl ctrl = params.ctrl;
    int op_mode = params.op_mode;
    int8_t * pW = weights;
    int32_t * pTdm2 = no_stride ? tdm2 : tdm1;
    int Xi_g, Yi_g;
    if ( op_mode == OP_DWC || op_mode == OP_DWC_SUM ) {
        unsigned wghts = params.Ky_g * params.Kx_g * params.Co_g;
        v8int8 * pI = (v8int8*) weights;
        v64int8 * restrict pO = (v64int8*) weight_unpack;
        pW = weight_unpack;
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE( 2, ) 
        for ( unsigned kc = 0; kc < wghts; kc++ )  
        {
            aie::vector<int8, 64> w = broadcast_to_v64int8( *pI++ );
            *pO++ = aie::select( int8( 0 ), w, aie::mask<64>::from_uint64( 0x8040201008040201ull ));
        }
        if ( op_mode == OP_DWC_SUM ) {
            aie::store_v( pW, aie::select( int8( 0 ), params.zp_wght, aie::mask<64>::from_uint64( 0x8040201008040201ull )));
            incrs.dimsW.num1 = 0;
            incrs.dimsW.num2 = 0;
            incrs.dimsW.inc3 = 0;
        }
    } else if ( op_mode == OP_SUM || op_mode == OP_SUM_T ) {
        bool is_transpose = op_mode == OP_SUM_T;
        uint64_t mask = 0x0101010101010101ull;
        aie::vector<int8,64> mask_v = aie::select( int8( 0 ), int8( 1 ), aie::mask<64>::from_uint64( mask ));
        for ( unsigned i=0; i<8; i++ ) {
            aie::store_v( pW + 64*i, mask_v );
            mask_v = aie::shuffle_up_rotate( mask_v, 1 );
        }
        if ( !is_transpose ) {
            incrs.shfl_0 = T512_1x2_lo;
            incrs.shfl_1 = T512_1x2_hi;
    
            Xi_g = params.X_g + ( params.Kx_g > 1 );
            Yi_g = params.Y_g * params.S_g + params.Ky_g - 1;
    
            int reset = -incrs.incA_0;
            int incCi = add_dimension( reset, params.Ci_g-1, params.step_Ci );
            int incXi = add_dimension( reset, Xi_g-1, incrs.incA_0 );
            int incYi = reset + params.step_Ky;
            
            incrs.dimsAI = dims_3d_t( params.Ci_g-1, incCi, Xi_g-1, incXi, incYi );
            incrs.dimsAO.num1 = 0;
            incrs.dimsAO.num2 = 0;
            incrs.dimsAO.inc3 = 0;
            incrs.dimsO.num1 = 0;
            incrs.dimsO.num2 = 0;
            incrs.dimsO.inc3 = 128;
            incrs.dimsW = dims_3d_t( params.Ci_g-1, 0, 7, 64, -7*64 );
            incrs.incT_0 = 256;
            incrs.incT_1 = 256;
            pTdm2 = byte_incr( tdm1, 128 );
            
            //TODO fix me - this can be performance optimized
            params.inner_g = params.Ci_g * 8;
            params.outer_g = std::max( 2, ( Xi_g * Yi_g * params.N_g + 7 ) / 8 );
        } else {
            incrs.shfl_0 = T16_8x8_lo;
            incrs.shfl_1 = T512_1x2_lo;
    
            Xi_g = params.X_g;
            Yi_g = params.Y_g;
    
            int reset  = -128;
            incrs.step_align = 0;
            int incCi1 = add_dimension( reset, 1, 8 );
            int incCi2 = add_dimension( reset, 3, params.step_Ci );
            int incXi  = reset + 128;
            int incCi  = -128 * Xi_g + 4 * params.step_Ci;
        
            incrs.dimsAI = dims_3d_t( 1, incCi1, 3, incCi2, incXi );
            incrs.dimsAO.num1 = 0;
            incrs.dimsAO.num2 = 0;
            incrs.dimsAO.inc3 = incCi;
            incrs.dimsO.num1 = 0;
            incrs.dimsO.num2 = 0;
            incrs.dimsO.inc3 = 128;
            incrs.dimsW = dims_3d_t( 0, 0, 7, 64, -7*64 );
            incrs.incT_0 = 128;
            incrs.incT_1 = 128;
            pTdm2 = tdm1;
            
            //TODO fix me - this can be performance optimized
            params.inner_g = Xi_g * 8;
            params.outer_g = std::min( 2, ( params.Ci_g + 3 ) / 4 );
        }
    }

    direct_conv_int16x8_generic_template<KC_TDM32, KC_TDM32> (
        (int16*)input, pW, tdm1, pTdm2, (int16*)0,
        params, ctrl, incrs
    );
    
    if ( final_tdm_iter && ( op_mode == OP_SUM || op_mode == OP_SUM_T )) {
        v16int32 * pI = ( v16int32 * ) tdm1;
        bool is_2k_mode = op_mode != OP_SUM_T;
        int mode1 = T32_4x8_lo;
        int mode2 = is_2k_mode ? T128_2x4_lo : T512_1x2_lo;
        for ( unsigned pix = 0; pix < ( Yi_g * Xi_g + 7 ) / 8; pix++ ) {
            v16int32 a0 = pI[4*pix];
            v16int32 a1 = pI[4*pix+1];
            v16int32 a2 = pI[4*pix+2];
            v16int32 a3 = pI[4*pix+3];
            v16int32 b0 = shuffle( a0, a1, mode1 );
            v16int32 b1 = shuffle( a0, a1, mode1 + 1 );
            v16int32 b2 = shuffle( a2, a3, mode1 );
            v16int32 b3 = shuffle( a2, a3, mode1 + 1 );
            pI[4*pix]   = shuffle( b0, b2, mode2 );
            pI[4*pix+1+is_2k_mode] = shuffle( b1, b3, mode2 );
            pI[4*pix+2-is_2k_mode] = shuffle( b0, b2, mode2 + 1 );
            pI[4*pix+3] = shuffle( b1, b3, mode2 + 1 );
        }
        if ( params.Kx_g * params.Ky_g > 1 ) {
            int Sy = params.step_Yi / params.step_Ky;
            sum_conv_2d_c1( tdm1, params.Y_g, params.X_g * 8 >> ( params.S_g > 1 ), params.Ky_g, params.Kx_g, params.S_g, Xi_g * 32 * Sy, Xi_g * 32 );
        }
    }
}



static void __attribute__((noinline)) direct_conv_int16x8_generic
(
        int16_t * input,
        int8_t * weights,
        int8_t * weight_unpack,
        int32_t * tdm1,
        int32_t * tdm2,
        int32_t * ifm_sum,
        int64_t * qdq_c0,
        int32_t qdq_c1,
        int32_t qdq_c2,
        int16_t * restrict output,
        //const KernelParams &params,
        KernelParams &params,
        bool final_tdm_iter=1
) {
    int op_mode = params.op_mode;
    int ofm_size = params.X_g * params.Y_g * params.Co_g * params.N_g * 64;
    
    if ( op_mode == OP_CONV_SYM || op_mode == OP_CONV_ASYM ) {
        //printf("\nOP_CONV mode called\n");
        direct_conv_int16x8_generic( input, weights, weight_unpack, tdm1, tdm2, params, final_tdm_iter, OP_CONV );
        if ( op_mode == OP_CONV_ASYM ) {
         //   printf("\nOP_SUM mode called\n");
            uint32_t temp_shift=params.shift_tdm;
            params.shift_tdm=0;
            direct_conv_int16x8_generic( input, weights, weight_unpack, ifm_sum, ifm_sum, params, final_tdm_iter, OP_SUM );
            params.shift_tdm=temp_shift;
        }
        
    } else if ( op_mode == OP_DWC_SYM || op_mode == OP_DWC_ASYM ) {
    
        uint32_t sign_bk      = params.ctrl.sign_O;
        uint32_t zero_init_bk = params.ctrl.zero_init;

        params.ctrl.sign_O = 1;
    
        direct_conv_int16x8_generic( input, weights, weight_unpack, tdm1, tdm2, params, final_tdm_iter, OP_DWC );
        if ( op_mode == OP_DWC_ASYM ) {
        
            params.ctrl.zero_init = 0;
            direct_conv_int16x8_generic( input, weights, weight_unpack, tdm1, tdm2, params, final_tdm_iter, OP_DWC_SUM );
            params.ctrl.zero_init = zero_init_bk;           
        }
        
        params.ctrl.sign_O = sign_bk;
                
    } else if ( op_mode == OP_SUM || op_mode == OP_SUM_T ) {
        direct_conv_int16x8_generic( input, weights, weight_unpack, ifm_sum, ifm_sum, params, final_tdm_iter );
    } else {
        direct_conv_int16x8_generic( input, weights, weight_unpack, tdm1, tdm2, params, final_tdm_iter );
    }
    
    if ( op_mode >= OP_CONV_SYM ) {
        int sum_size = params.X_g * params.Y_g * params.N_g * 8;
        adf::output_buffer<uint16> ofm(( uint16* )output, ofm_size, 0, ofm_size );
        adf::input_buffer<int32> ifm_sm( ifm_sum, sum_size, 0, sum_size );
        
        adf::input_buffer<int64> c0( (int64*)qdq_c0, params.Co_g * 8, 0, params.Co_g * 8 );
        QDQParams qdq_param;
        qdq_param.shift_Qout = params.shift_res;
        qdq_param.Y_g = params.Y_g;
        qdq_param.N_g = params.Co_g;
        if ( params.S_g == 1 ) {
            //'Tdm':  '(XN)2>1YX<1N<1C(XN)4C8',   tile: (XN)4C8, D0: (XN)2>1, D1: YX<1N<1, D2: C
            qdq_param.M_g = 2 * params.X_g;
            qdq_param.wrap0 = 2;
            qdq_param.wrap1 = params.X_g * params.Y_g;
            qdq_param.wrap2 = 1;
            qdq_param.wrap3 = 1;
            qdq_param.step0 = (( long )tdm2 - ( long )tdm1 ) / 4;
            qdq_param.step1 = 32 * qdq_param.N_g;
            qdq_param.step2 = 0;
            qdq_param.step3 = 0;
            qdq_param.step4 = 32;
        } else {
            //'Tdm_S2': 'C2>1YXNC<1(XN)4C8',    tile: (XN)4C8, D0: YXN, D1: C2>1, D2: C<1
            qdq_param.M_g = params.X_g;
            qdq_param.wrap0 = params.X_g * params.Y_g;
            qdq_param.wrap1 = 2;
            qdq_param.wrap2 = 1;
            qdq_param.wrap3 = 1;
            qdq_param.step0 = 16 * qdq_param.N_g;
            qdq_param.step1 = (( long )tdm2 - ( long )tdm1 ) / 4;
            qdq_param.step2 = 0;
            qdq_param.step3 = 0;
            qdq_param.step4 = 32;
        }
        /*
        uint32_t * tmp_ifm_sm = (uint32_t *) ifm_sum;
        if(final_tdm_iter){
           for(unsigned i=0;i<2*16;i++){
              printf("ifm_sum[%d] %d\n",i,tmp_ifm_sm[i]);
           }
        }*/
        qdq<4,8,4,8,3>( tdm1, ifm_sm, c0, qdq_c1, qdq_c2, ofm, qdq_param );
    }
}
/*
#ifdef CONV_QDQ_C0
alignas(64) int64_t some_zeros[64] = {1130994549954};
#else
alignas(64) int64_t some_zeros[64] = {0};
#endif
*/

// This function is a wrapper to interface the generic testbench used in this example.
static void __attribute__((noinline)) direct_conv_int16x8_generic
(
        int * input,
        int * wts,
        int * tdm1,
        int * tdm2,
        int * restrict output,
        //const KernelParams &params_s,
        KernelParams &params_s,
        bool final_tdm_iter=0
) {
    KernelParams params = params_s;
    int op_mode = params.op_mode;
    int wgt_size;
        
    if( (op_mode == OP_DWC_SYM) || (op_mode == OP_DWC_ASYM) ){
        
        wgt_size = ((params.Kx_g * params.Ky_g * params.Co_g * 8) + 63) & ~63; 
    
    } else {
        
        wgt_size = params.Kx_g * params.Ky_g * params.Co_g * 8 * ( op_mode == OP_DWC ? 10 : params.Ci_g * 8 );
    }
    //int wgt_size = params.Kx_g * params.Ky_g * params.Co_g * 8 * ( op_mode == OP_DWC ? 10 : params.Ci_g * 8 );
    int ofm_size = params.X_g * params.Y_g * params.Co_g * params.N_g * 16 * (4 + 4*(params.S_g==1));
    
    int8_t * weights = (int8_t *)wts;
    
    int64_t * c0 = (int64_t*)( weights + wgt_size );
    int32_t c1 = ((int32_t*)( weights + wgt_size + params.Co_g * 64 ))[0];
    int32_t c2 = ((int32_t*)( weights + wgt_size + params.Co_g * 64 ))[1];
    //int8_t * weight_unpack = ( int8_t* )weights + (( params.Ky_g * params.Kx_g * params.Co_g * 8 + 63 ) & ~63 );
    int8_t * weight_unpack = ( int8_t* )weights + (( wgt_size + (params.Co_g * 8 * 8) + 8 + 63 ) & ~63 );
    
    //int32_t * ifm_sum = ( op_mode == OP_SUM || op_mode == OP_SUM_T ? tdm1 : tdm1+ofm_size/2 );
    //int32_t * ifm_sum = ( op_mode == OP_SUM || op_mode == OP_SUM_T ? tdm1 : (int32_t *)(0x78000 + 0x800));
    //int32_t * ifm_sum = ( op_mode == OP_SUM || op_mode == OP_SUM_T ? tdm1 : (int32_t *)(0x78000 + (ofm_size<<1)));
    //int32_t * ifm_sum = ( op_mode == OP_SUM || op_mode == OP_SUM_T ? tdm1 : (int32_t *)(0x7DD40));
    int32_t * ifm_sum = ( op_mode == OP_SUM || op_mode == OP_SUM_T ? tdm1 : (int32_t *)(0x78000 + (IFM_SUM_OFFSET)));
    //printf("\n ifm sum pointer=%i",ifm_sum);
   /* 
    c0 = some_zeros;
    c1 = 0;
    #ifdef CONV_QDQ_C2
    c2 = 113099;
    #else
    c2 = 1;
    params.shift_res -= params.shift_tdm;
    #endif*/
    
    //direct_conv_int16x8_generic(( int16_t* ) input, ( int8_t* )weights, weight_unpack, tdm1, tdm2, ifm_sum, c0, c1, c2, ( int16_t* )output, params );
    direct_conv_int16x8_generic(( int16_t* ) input, ( int8_t* )weights, weight_unpack, tdm1, tdm2, ifm_sum, c0, c1, c2, ( int16_t* )output, params,final_tdm_iter );
    
    if ( op_mode == OP_SUM || op_mode == OP_SUM_T ) {
        if ( OUT_MODE == KC_TDM32 ) {
            //this is not needed in the real design
            v16int32 * pI = ( v16int32 * )( tdm1 + ( params.Y_g * params.X_g * 8 / 2 >> ( params.S_g > 1 )));
            v16int32 * pO = ( v16int32 * ) tdm2;
            fifo_state_t f;
            fifo_ld_reset( pI, f );
            for ( unsigned i = 0; i < (( params.Y_g * params.X_g * 8 >> ( params.S_g > 1 )) + 31 ) / 32; i++ ) {
                *pO++ = fifo_ld_pop( pI, f );
            }
        }
    }
}

/*
 * The following function is to compute gemm and ifm sum 
 * While accumulating partial results in TDM buffers
 */
/*void run_a16w8_gemm_tdm(KernelArgs& args)
{
    set_sat();
    set_rnd(rnd_conv_even);
    KernelParams params;
    const int op_mode = params.op_mode;
    uint8_t* args_params = (uint8_t*)args.params_data;
    params.ctrl.zero_init = args_params[0];
    // Both the matrices are unsigned for GEMM
    params.ctrl.sign_A = 0;
    params.ctrl.sign_W = 0;
    int16_t* matA = static_cast<int16_t*>(args.s2mm_ch1_data);
    int8_t* matB = static_cast<int8_t*>(args.s2mm_ch0_data);
    #ifdef __DEBUG__
        if(col_id == 0 && row_id == 4) {
            printf("run_gemm_tdm \n");
            printf("zero_init=%d \n", params.ctrl.zero_init);
            for(int idx = 0; idx < 64; idx++)
                printf("run_gemm_tdm: matA[%d]=%d \n", idx, *(matA+idx) );
            for(int idx = 0; idx < 64; idx++)
                printf("run_gemm_tdm: matB[%d]=%d \n", idx, *(matB+idx) );
        }
    #endif
    int8_t* weight_unpack = nullptr;
    int32_t * tdm1 = static_cast<int32_t*>(conv_to_local_ptr(GEMM_TDM1_ADDR));
    int32_t * tdm2 = static_cast<int32_t*>(conv_to_local_ptr(GEMM_TDM2_ADDR));
    int32_t * ifm_sum = static_cast<int32_t*>(conv_to_local_ptr(GEMM_IFM_SUM_ADDR));
    // Compute GEMM
    #ifdef __DEBUG__
        if(col_id == 0 && row_id == 4)
            printf("gemm_tdm call \n");
    #endif
    direct_conv_int16x8_generic( matA, matB, weight_unpack, tdm1, tdm2, params, 0, OP_CONV );
    // Compute IFM_SUM
    #ifdef __DEBUG__
        if(col_id == 0 && row_id == 4)
            printf("ifm_sum_tdm call \n");
    #endif
    direct_conv_int16x8_generic( matA, matB, weight_unpack, ifm_sum, ifm_sum, params, 0, OP_SUM );
    #ifdef __DEBUG__
        if(col_id == 0 && row_id == 4) {
            printf("run_gemm_tdm: tdm1_addr=%d \n", tdm1);
            printf("run_gemm_tdm: tdm2_addr=%d \n", tdm2);
            printf("run_gemm_tdm: ifm_sum_addr=%d \n", ifm_sum);
            for(int idx = 0; idx < 64; idx++)
                printf("run_gemm_tdm: tdm1[%d]=%d \n", idx, *(tdm1+idx));
            for(int idx = 0; idx < 64; idx++)
                printf("run_gemm_tdm: tdm2[%d]=%d \n", idx, *(tdm2+idx));
            for(int idx = 0; idx < 64; idx++)
                printf("run_gemm_tdm: ifm_sum[%d]=%d \n", idx, *(ifm_sum+idx));
        }
    #endif
}*/

/*
 * The following function is to compute gemm + ifm sum + QDQ
 * And stream out the final output 
 */
/*void run_a16w8_gemm_qdq(KernelArgs& args)
{
    set_sat();
    set_rnd(rnd_conv_even);
    KernelParams params;
    // Op code to Compute GEMM + sum + qdq
    params.op_mode = OP_CONV_ASYM;
    const int op_mode = params.op_mode;
    // Both the matrices are unsigned for GEMM
    params.ctrl.sign_A = 0;
    params.ctrl.sign_W = 0;
    #ifdef __DEBUG__
        if(col_id == 0 && row_id == 4)
            printf("final subvol \n");
    #endif
    // NOTE: Refer to overlay_gemm.py to align the QDQ params address in L1
    void* qdq_param_addr = static_cast<void*>(conv_to_local_ptr(QDQ_PARAM_ADDR));
    // Load the qdq params from L1 buffer to struct on stack
    alignas(64) static QDQKernelParams gemm_qdq_params;
    v64uint8 * pI = (v64uint8*)qdq_param_addr;
    v64uint8 * restrict pO = (v64uint8*)(&gemm_qdq_params);
    pO[0] = pI[0];
    // Shift the output of QDQ
    params.shift_res = gemm_qdq_params.shift_Qout;
    // Shift the GEMM acc64 result to acc32 to be fed to qdq kernel
    params.shift_tdm = gemm_qdq_params.shift_tdm;
    int16_t* matA = static_cast<int16_t*>(args.s2mm_ch1_data);
    int8_t* matB = static_cast<int8_t*>(args.s2mm_ch0_data);
    int64_t* c0 = (int64_t*)( matB + WGT_SIZE );
    #ifdef __DEBUG__
        if(col_id == 0 && row_id == 4) {
            for(int idx = 0; idx < 64; idx++)
                printf("run_gemm_qdq: matA[%d]=%d \n", idx, *(matA+idx) );
            for(int idx = 0; idx < 64; idx++)
                printf("run_gemm_qdq: matB[%d]=%d \n", idx, *(matB+idx) );
        }
    #endif
    int16_t* output = static_cast<int16_t*>(args.mm2s_ch0_data);
    // NOTE: WGT size is compile time constant for a fixed dimension of 32 x 128 x 64 
    //  This is to avoid any scalar Ops that might increase PM
    //int wgt_size = params.Kx_g * params.Ky_g * params.Co_g * 8 * ( op_mode == OP_DWC ? 10 : params.Ci_g * 8 );
    #ifdef __DEBUG__
        if(col_id == 0 && row_id == 4)
            printf("wgt_size=%d \n", WGT_SIZE);
    #endif
    // Pointer to C0 vector coefficients that is part of the weights subvol
    #ifdef __DEBUG__
        if(col_id == 0 && row_id == 4) {
            printf("matB_addr: %d \n", matB);
            printf("qdq_co_addr: %d \n", c0);
            for(int idx = 0; idx < 64; idx++)
                printf("matB[%d]=%ld \n", idx, *(matB+idx));
            for(int idx = 0; idx < 64; idx++)
                printf("qdq_c0[%d]=%ld \n", idx, *(c0+idx));
            printf("gemm_qdq_params.c1 = %d \n", gemm_qdq_params.c1);
            printf("gemm_qdq_params.c2 = %d \n", gemm_qdq_params.c2);
            printf("gemm_qdq_params.shift_Qb = %d \n", gemm_qdq_params.shift_Qb);
            printf("gemm_qdq_params.shift_Qout = %d \n", gemm_qdq_params.shift_Qout);
            printf("gemm_qdq_params.shift_tdm = %d \n", gemm_qdq_params.shift_tdm);
        }
    #endif
    int8_t* weight_unpack = nullptr;
    // Compute GEMM and IFM sum 
    // This is the accumulation wrapper 
    // Intermediate output is stored to TDM
    int32_t * tdm1 = static_cast<int32_t*>(conv_to_local_ptr(GEMM_TDM1_ADDR));
    int32_t * tdm2 = static_cast<int32_t*>(conv_to_local_ptr(GEMM_TDM2_ADDR));
    int32_t * ifm_sum = static_cast<int32_t*>(conv_to_local_ptr(GEMM_IFM_SUM_ADDR));
    direct_conv_int16x8_generic( matA, matB, weight_unpack, tdm1, tdm2, ifm_sum, c0, gemm_qdq_params.c1, gemm_qdq_params.c2, output, params, 1);
    #ifdef __DEBUG__
        if(col_id == 0 && row_id == 4) {
            printf("run_gemm_qdq: tdm1_addr=%d \n", tdm1);
            printf("run_gemm_qdq: tdm2_addr=%d \n", tdm2);
            printf("run_gemm_qdq: ifm_sum_addr=%d \n", ifm_sum);
            for(int idx = 0; idx < 64; idx++)
                printf("run_gemm_qdq: tdm1[%d]=%d \n", idx, *(tdm1+idx));
            for(int idx = 0; idx < 64; idx++)
                printf("run_gemm_qdq: tdm2[%d]=%d \n", idx, *(tdm2+idx));
            for(int idx = 0; idx < 64; idx++)
                printf("run_gemm_qdq: ifm_sum[%d]=%d \n", idx, *(ifm_sum+idx));
            for(int idx = 0; idx < 64; idx++)
                printf("run_gemm_qdq: ofm[%d]=%d \n", idx, *(output+idx));
        }
    #endif
}*/

#endif

