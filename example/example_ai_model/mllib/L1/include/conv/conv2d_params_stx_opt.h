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

#ifndef __CONV2D_PARAMS_H__
#define __CONV2D_PARAMS_H__

#include "layer_params.h"
#include "utils.h"
#include <adf.h>
#include "aie.hpp"

// Some constants used in the conv2d kernel code

#if ((__AIE_ARCH__ == 21) || (__AIE_ARCH__ == 22))//AIE2p conv2d_generic
#undef USE_OPMODE_1
#define USE_OPMODE_1 0
#endif

// Basic matrix mult dimensions for AIE-ML
#define DIM_M 4
#define DIM_N 8
#define DIM_K 8

// Buffer placement directives for IFM, WTs and OFM buffers.
#ifndef IFM_DM_BANK
#define IFM_DM_BANK __aie_dm_resource_a
#endif

#ifndef WT_DM_BANK
#define WT_DM_BANK __aie_dm_resource_a
#endif

#ifdef CONV2D_S2_OPT1
    #ifndef WT_DM_BANK_2
    #define WT_DM_BANK_2 __aie_dm_resource_b
    #endif
#endif

#ifdef CONV2D_OPT1

   #ifndef OFM_DM_BANK
     #define OFM_DM_BANK __aie_dm_resource_b
   #endif
#else
   #ifndef OFM_DM_BANK
     #define OFM_DM_BANK
   #endif

#endif

#ifndef PSUM_0_DM_BANK
#define PSUM_0_DM_BANK __aie_dm_resource_b
#endif

#ifndef PSUM_1_DM_BANK
#define PSUM_1_DM_BANK __aie_dm_resource_c
#endif

// Bias is 16-bit
#define BIAS_BYTES 2
#define OFM_SETS 2


// number of bytes set aside at the start of weight buffer for conv kernel parameters
#define LAYER_PARAM_BYTES 64
#define LAYER_PARAM_WORDS (LAYER_PARAM_BYTES>>2)
#define ITER_PARAM_BYTES 32
#define ITER_PARAM_WORDS (ITER_PARAM_BYTES>>2)
#define ITER_PARAM_OFFSET 19
#define BIAS_MULTIPLE   2
#define WTS_LEN_OFFSET 50
#define PSUM_IN_OFFSET 61
#define PSUM_OUT_OFFSET 62
#define EXEC_TYPE_OFFSET 63

// Inclusion of opmodes based on Network type
#ifndef USE_OPMODE_1
#define USE_OPMODE_1 1 // 3x3/s1 filter for channel-1, needed in tiny YOLO v2
#endif
// Needed to check for last column, override from top as required
#ifndef LAST_COL
#define LAST_COL 6
#endif

#ifdef CONV2D_OPT1
struct alignas(v32uint16) conv2d_internal_params {
    //Layer params
    uint8_t ifm_xsize;
    uint8_t ifm_ysize;
    #ifndef USE_NUM_CH_SCALE
    uint8_t ifm;
    uint8_t num_ofm_ch;
    #else
    uint8_t ifm_scaled;
    uint8_t num_ofm_ch_scaled;
    #endif
    uint8_t ksize;
    uint8_t stride_bits;
    int8_t shift_out;
    uint8_t out_mode ;
    int8_t shift_bias_init ;
    int8_t shift_psum_in ;
    int8_t shift_psum_out;
    uint8_t psum_buff_offset_scaled;
    int16_t lrelu_alpha;
    int8_t shift_lrelu_alpha;
    int8_t shift_lrelu_input;
    int8_t shift_lrelu_out;
    uint8_t num_chout_iter;
    uint8_t num_ifm_depth_iter;
    uint8_t op_mode ;
    uint16_t ofm_len ;
    uint8_t num_height_iter;
    uint8_t num_width_iter ;
    uint16_t ifm_len ;
    uint8_t ofm_width_pad  ;
    int8_t run_time_act ;
    uint8_t conv_type ;
    uint8_t row_offset;
    uint8_t in0_sign;
    uint8_t ofm_offset_packed ;
    int ifm_sign;  //  IFM sign upper bits
    // Derived params
    int32_t out_sign ;
    int32_t psum_buff_offset;
    uint8_t ofm_offset_left ;
    int32_t psum_in;
    int32_t psum_out;
    int hdr_len ; // larger than 8 bit so retaining
    int8_t batch;
    int8_t batch_log2;
    uint8_t oyp;
    uint8_t oxp;
    uint8_t ky;
    uint8_t kx;
    uint8_t stride;
    uint8_t curr_depth_iter ;
    uint8_t io_mode;
    uint32_t ofm_offset_top;  // number rows of pixels to be left out for padding at top
    uint8_t exec_type; // This has conv2d generic, layer-1, maxpool2d etc
    uint32_t inner_g;
    uint32_t outer_g;
    int    ifm_offset; // IFM offset in 32-bit words to pick subset from superset in FC mode
    int    wts_offset; // WTS offset in 32-bit words to pick subset from superset in FC mode
    DECL_ITR3D(iterator_inner)
    DECL_ITR3D(iterator_outer)
    DECL_ITR3D(iterator_weights)
    #ifdef CONV2D_STX_OPT_BIAS_ITER
        DECL_ITR3D(iterator_bias)
    #endif
    int32_t inc_A;
    int32_t inc_A_1;
    int32_t inc_A_2x8_mode;
    int32_t inc_Co;
    int32_t inc_Xo;
    #ifdef CONV2D_2x8_WIDTH_SUPPORT
        int32_t inc_Yo;
    #endif
    int32_t inc_Co_T;
    int32_t inc_Xo_T;
    #ifdef CONV2D_S2_2x8_SUPPORT
        int32_t inc_Yo_T;
    #endif
    int32_t inc_S_0;
    int32_t inc_S_1;
    int32_t inc_S_2;
    int32_t inc_ST_0;
    int32_t inc_ST_1;
    int32_t inc_ST_2;
    int32_t inc_acc_0;
    int32_t inc_acc_1;
    int32_t inc_acc_2;
    int32_t inc_acc_d0;
    int32_t inc_acc_d1;
    int32_t inc_acc_rev;


    int step_align;
    int shfl_0;
    int shfl_1;
    int shfl_2;
    uint8_t num_Kx;

    uint32_t num_Co;
    uint32_t num_X;
    uint32_t num_Xo;
    uint8_t num_W;
    uint8_t num_BN;
    #ifdef USE_NUM_CH_SCALE
    uint16_t ifm;
    uint16_t num_ofm_ch;
    #endif    
    
    uint32_t sel_S2_opt;

    #ifdef USE_ADD2D_LINEAR_ACT
    uint8_t add2d_act_rtp;
    #endif
};
#else
struct alignas(v32uint16) conv2d_internal_params {
    //Layer params
    uint8_t ifm_xsize;
    uint8_t ifm_ysize;
    #ifndef USE_NUM_CH_SCALE
    uint8_t ifm;
    uint8_t num_ofm_ch;
    #else
    uint8_t ifm_scaled;
    uint8_t num_ofm_ch_scaled;
    #endif
    uint8_t ksize;
    uint8_t stride_bits;
    int8_t shift_out;
    uint8_t out_mode ;
    int8_t shift_bias_init ;
    int8_t shift_psum_in ;
    int8_t shift_psum_out;
    uint8_t psum_buff_offset_scaled;
    int16_t lrelu_alpha;
    int8_t shift_lrelu_alpha;
    int8_t shift_lrelu_input;
    int8_t shift_lrelu_out;
    uint8_t num_chout_iter;
    uint8_t num_ifm_depth_iter;
    uint8_t op_mode ;
    uint16_t ofm_len ;
    uint8_t num_height_iter;
    uint8_t num_width_iter ;
    uint16_t ifm_len ;
    uint8_t ofm_width_pad  ;
    int8_t run_time_act ;
    uint8_t conv_type ;
    uint8_t row_offset;
    uint8_t in0_sign;
    uint8_t ofm_offset_packed ;
    int ifm_sign;  //  IFM sign upper bits
    // Derived params
    int32_t out_sign ;
    int32_t psum_buff_offset;
    uint8_t ofm_offset_left ;
    int32_t psum_in;
    int32_t psum_out;
    int hdr_len ; // larger than 8 bit so retaining
    int8_t batch;
    int8_t batch_log2;
    uint8_t oyp;
    uint8_t oxp;
    uint8_t ky;
    uint8_t kx;
    uint8_t stride;
    uint8_t curr_depth_iter ;
    uint8_t io_mode;
    uint32_t ofm_offset_top;  // number rows of pixels to be left out for padding at top
    uint8_t exec_type; // This has conv2d generic, layer-1, maxpool2d etc
    uint32_t inner_g;
    uint32_t outer_g;
    int    ifm_offset; // IFM offset in 32-bit words to pick subset from superset in FC mode
    int    wts_offset; // WTS offset in 32-bit words to pick subset from superset in FC mode
    DECL_ITR3D(iterator_inner)
    DECL_ITR3D(iterator_outer)
    DECL_ITR3D(iterator_weights)
    int32_t inc_A;
    int32_t inc_A_1;
    int32_t inc_A_2x8_mode;
    int32_t inc_Co;
    int32_t inc_Xo;
    int32_t inc_Co_T;
    int32_t inc_Xo_T;
    int32_t inc_S_0;
    int32_t inc_S_1;
    int32_t inc_S_0_2x8; // 2x8 mode
    int32_t inc_S_1_2x8;// 2x8 mode
    int32_t iter_2x8_inc;
    int32_t iter_2x8_inc_T;
    int32_t iter_2x8_wrap;
    int32_t inc_S_2;
    int32_t inc_ST_0;
    int32_t inc_ST_0_2x8;
    int32_t inc_ST_1_2x8;
    int32_t inc_ST_1;
    int32_t inc_ST_2;
    int32_t inc_acc_0;
    int32_t inc_acc_1;
    int32_t inc_acc_2;
    int32_t inc_acc_d0;
    int32_t inc_acc_d1;
    int32_t inc_acc_rev;


    int step_align;
    int shfl_0;
    int shfl_1;
    int shfl_2;
    uint8_t num_Kx;

    uint32_t num_Co;
    uint32_t num_X;
    uint8_t num_W;
    uint8_t num_BN;
    #ifdef USE_NUM_CH_SCALE
    uint16_t ifm;
    uint16_t num_ofm_ch;
    #endif
};
#endif

typedef struct conv2d_internal_params conv2d_params_t ;

static INLINE int min(int a, int b) { return a<b ? a : b; }
static INLINE int max(int a, int b) { return a>b ? a : b; }


#ifdef FC_EN  //Enable generalizationsin FC: support for batch size=1,2,4, int8 in FC, needs out_mode=1 for configuring int16 output
// Conv2d Optimization-0: Load precomputed values for exec_type, psum_in,psum_out, estimated to save 40+ cycles per call and 250+ bytes of PM
static void setup_conv2d_iter_params(conv2d_params_t &param,int8_t* params_ptr) {
/*
    params_ptr+=(PSUM_IN_OFFSET-1);
    uint32_t tmp=*((uint32_t *)params_ptr);
    tmp=tmp>>8;
    param.psum_in=tmp & 0x000000FF;
    tmp=tmp>>8;
    param.psum_out=tmp & 0x000000FF;
    tmp=tmp>>8;
    param.exec_type=tmp & 0x000000FF;*/

    param.psum_in=params_ptr[PSUM_IN_OFFSET];
    param.psum_out=params_ptr[PSUM_OUT_OFFSET];
    param.exec_type=params_ptr[EXEC_TYPE_OFFSET];
}
static void setup_conv2d_iter_params(conv2d_params_t &param) {
    param.curr_depth_iter++;
    param.io_mode = (param.num_ifm_depth_iter>1)*((param.curr_depth_iter==1)+2*(param.curr_depth_iter==param.num_ifm_depth_iter)+3*((param.curr_depth_iter>1)&&(param.curr_depth_iter<param.num_ifm_depth_iter)));
    param.psum_in = (param.io_mode==0 || param.io_mode==1) ? 0 :1;
    #ifdef CONV2D_S1_1x8x16
    uint32_t check_conv_type = (param.conv_type!=2) * (param.conv_type!=8) * (param.conv_type!=CONV2D_7x7S2_LYR1) * (param.conv_type!=5) * (param.conv_type!=7);
    #else
    uint32_t check_conv_type = (param.conv_type!=2) * (param.conv_type!=8) * (param.conv_type!=CONV2D_7x7S2_LYR1) * (param.conv_type!=5);
    #endif
    if((param.out_mode==1) && (param.curr_depth_iter==param.num_ifm_depth_iter)){
        param.psum_out = 1;
        param.exec_type = 3;
    } else {
        param.psum_out   = (param.io_mode==0 || param.io_mode==2) ? 0 :1;
        param.exec_type  = (param.conv_type << 4)*(check_conv_type) + param.io_mode;
    }
    //FC related updates to params
     //uint32_t fc_mode = (param.conv_type==2 || param.conv_type==8) ;
     //param.psum_in = (param.conv_type == 8)? 1*(param.num_ifm_depth_iter==1) : param.psum_in ;


    // TODO: hack for stride=2, add 4 to exec type to pick stride=2 optimized kernel at run time
#ifdef STRIDE2_OPT
     param.exec_type  = param.exec_type+(4*param.stride_bits*(param.batch < 4)*(param.conv_type!=CONV2D_7x7S2_LYR1));
#endif

       if(param.curr_depth_iter==param.num_ifm_depth_iter){
       param.curr_depth_iter=0; // Reset depth_iter
    }
}
#else // last version wthout generalization to be removed after merge test with resnet18 batch_size=1
//INLINE void setup_conv2d_iter_params(conv2d_params_t &param) {
static void setup_conv2d_iter_params(conv2d_params_t &param) {
    param.curr_depth_iter++;
    param.io_mode = (param.num_ifm_depth_iter>1)*((param.curr_depth_iter==1)+2*(param.curr_depth_iter==param.num_ifm_depth_iter)+3*((param.curr_depth_iter>1)&&(param.curr_depth_iter<param.num_ifm_depth_iter)));
    param.psum_in = (param.io_mode==0 || param.io_mode==1) ? 0 :1;

    if((param.out_mode==1) && (param.curr_depth_iter==param.num_ifm_depth_iter)){
        param.psum_out = 1;
        param.exec_type = 1;
    } else {
        param.psum_out   = (param.io_mode==0 || param.io_mode==2) ? 0 :1;
        param.exec_type  = (param.conv_type << 4)*(param.conv_type!=CONV2D_7x7S2_LYR1) + param.io_mode;
    }
    //FC related updates to params
     uint32_t fc_mode = (param.conv_type==2 || param.conv_type==8) ;
     param.psum_in = (param.conv_type == 8)? 1 : param.psum_in ;
     param.exec_type  = (param.conv_type==2)? 1 : ((param.conv_type==8) ? 3: param.exec_type);
     param.psum_out   = fc_mode ? 1 :param.psum_out;

    // TODO: hack for stride=2, add 4 to exec type to pick stride=2 optimized kernel at run time
#ifdef STRIDE2_OPT
     param.exec_type  = param.exec_type+(4*param.stride_bits*(param.batch < 4)*(param.conv_type!=CONV2D_7x7S2_LYR1));
#endif

     //printf("exec_type=%d\n",param.exec_type);
     if(param.curr_depth_iter==param.num_ifm_depth_iter){
       param.curr_depth_iter=0; // Reset depth_iter
    }
}
#endif // end FC_EN

#if ((__AIE_ARCH__ == 21) || (__AIE_ARCH__ == 22))//AIE2p conv2d_generic

#if defined(STRIDE2_OPT) && defined(CONV2D_OPT1) && defined(CONV2D_S2_OPT1)

#ifdef CONV2D_S1_1x8x16
static void setup_conv2d_params (int8_t * params_ptr, conv2d_params_t &param, uint8_t batch_size) {

    #ifdef USE_ADD2D_LINEAR_ACT
    uint8_t add2d_linear_act;
    #endif

    int32_t inc_rev;
    int32_t inc_Kx;
    int32_t inc_Ky;
    uint32_t num_Kx;
    uint32_t num_Ky;
    int32_t inc_Ci;

    int32_t inc_Co_rev;
    int32_t inc_Xi;
    int32_t inc_Yi;   

    int32_t inc_w_Ci_rev;
    int32_t inc_w_Co_rev;

    //Copy 32 bytes of layer params
    aie::vector<int32,8> pars = aie::load_v<8>((int32 *)(params_ptr));
    aie::store_v( (int32  __aie_dm_resource_a *) &param, pars);

    uint8_t ksize         = param.ksize;
    uint8_t stride_bits   = param.stride_bits;

    uint8_t ksize_x       =  ksize;
    uint8_t ksize_y       =  ksize;
    uint8_t stride_bits_x = stride_bits;
    uint8_t stride_bits_y = stride_bits;
    #ifdef USE_NUM_CH_SCALE
    param.ifm             = (param.ifm_scaled)<<3;//Number of channels encoded with scale factor of 8
    uint16_t nifms         = param.ifm;//Number of channels encoded with scale factor of 8
    #else
    uint8_t nifms         = param.ifm;
    #endif
    #ifdef USE_LARGE_IFMSV_WIDTH
    uint16_t ifm_x        = (param.lrelu_alpha==0)?(uint16_t) param.ifm_xsize:(uint16_t)param.lrelu_alpha;
    #else
    uint8_t ifm_x         = param.ifm_xsize;
    #endif

    int calc_edge_output  = 0;
    #ifndef USE_NUM_CH_SCALE 
    if(param.ifm == 4)//Currently only 11x11s4 lyr-1 is supported with this if-condition.TODO bring 7x7s2 lyr-1 under this. 
    {       
        nifms         = 8;
        ksize_x       = (ksize + 1)>>1;        
        ifm_x         = ifm_x >> 1;        
        stride_bits_x = stride_bits>>1;              
    } 
    else
    #endif
    {        
        if(param.conv_type==CONV2D_7x7S2_LYR1_EDGE) {//4
            calc_edge_output = 1;
            param.conv_type  = CONV2D_7x7S2_LYR1;            
        }

        int value = (ksize>>4) & 0x0F;
        ksize_x = (value==0) ? ksize : value;
        ksize_y = ksize & 0x0F;

        ksize_x = (param.conv_type==CONV2D_7x7S2_LYR1) ? (ksize_x + 1)>>1 : ksize_x;
        stride_bits_x = (param.conv_type==CONV2D_7x7S2_LYR1) ? 0 : stride_bits;                
    }

    uint8_t stride_x = 1 << stride_bits_x;
    uint8_t stride_y = 1 << stride_bits_y;       

    // Extract IFM sign packed with ifm2_sign (called in0_sign)
    uint8_t tmp                 = param.in0_sign; // offset 30 input0 sign for add2d
    param.ifm_sign              = (int) ((tmp>>4) & 0x01); // offset 30 input sign for conv2d
    param.in0_sign              = (tmp & 0x01); // offset 30 input0 sign for add2d

    // Fixes related to layer param copy 
    param.psum_buff_offset      = (param.psum_buff_offset_scaled)*128*(1-param.out_mode);
    param.ofm_offset_packed     = 0x00; // hard-coding to zero, OFM offset left and top are not supported (and never used) yet!
    param.ofm_offset_left       = (param.ofm_offset_packed) & 0x0F; 
    #ifdef USE_NUM_CH_SCALE
    param.num_ofm_ch            = (param.num_ofm_ch_scaled)<<3;
    uint16_t nofms              = param.num_ofm_ch;
    #else
    uint8_t nofms               = param.num_ofm_ch;
    #endif   

    uint32_t fc_mode            = (param.conv_type==2 || param.conv_type==8) ;
    param.ifm_offset            = 0 ; // will be set in wrapper
    param.wts_offset            = 0 ;  // will be set in wrapper 
    param.out_sign              = 1 - (int) param.run_time_act; //Run time activation 0: Linear   1: ReLU     

 
    #ifdef USE_ADD2D_LINEAR_ACT
    add2d_linear_act = (int32_t) *(params_ptr+54);
    param.add2d_act_rtp = add2d_linear_act;

    #endif

   
    param.curr_depth_iter       = 0; // initialize depth iter to 0
    param.batch_log2            = (batch_size==4) ? 2: ((batch_size==2) ? 1 : 0);     
    param.batch                 = (int)batch_size;     
    param.oyp                   = (param.ifm_ysize - ksize_y + 1) >> stride_bits_y;     
    param.oxp                   = (ifm_x - ksize_x + 1) >> stride_bits_x;     
    int sv_out_width            = param.oxp + ((calc_edge_output<<4)>>param.batch_log2) ; 
    param.ky                    = ksize_y;     
    param.kx                    = ksize_x;     
    #ifdef CALC_CONV2D_OFM_LEN
    // recalculate OFM length, ignore supplied value, it will be deprecated. Needed for non-uniform tiling
    param.ofm_len               = sv_out_width*param.oyp*nofms*param.batch;
    #endif 
    param.stride                = 1 << stride_bits_y;  

    int align_kx                = max(1, 8 >> param.batch_log2);
    int ixp                     = param.oxp * stride_x + ((param.kx-1+align_kx-1)/align_kx)*align_kx;
    int iyp                     = param.oyp * stride_y +   param.ky-1;
    int step_co                 = (sv_out_width + param.ofm_width_pad + param.ofm_offset_left) * (8 << param.batch_log2);
    int step_ci                 = ixp * (8 << param.batch_log2);
    int step_yo                 = step_co * nofms >> 3;
    int step_yi                 = step_ci * nifms >> 3;
    param.hdr_len               = (LAYER_PARAM_WORDS+nofms*BIAS_MULTIPLE);

    uint32_t width_is_not_16n = (param.oxp & (uint8_t)0x0F) != 0;
    uint32_t height_is_not_2n = (param.oyp & (uint8_t)0x01) != 0;   
    
    param.sel_S2_opt = ((stride_x==1) && width_is_not_16n && height_is_not_2n);
    param.sel_S2_opt = ((param.conv_type == 7) && (stride_x==1)) ? 1 : param.sel_S2_opt;
 
    if((param.sel_S2_opt == 0) && (stride_x != 2))
    { 
        int32_t en2x8_mode             = (int32_t) *(params_ptr+35); 
        int32_t num_x_out_samp         = 16 - 8*en2x8_mode;
        int32_t num_x_out_samp_bits    = 4 - en2x8_mode;    
        int32_t num_y_out_samp         = (stride_x == 1) ? (1+en2x8_mode) : 1;
        int32_t num_y_out_samp_bits    = (stride_x == 1) ? en2x8_mode     : 0;
        int32_t num_ch_out_samp        = (stride_x == 1) ? 16             : (16*(1+en2x8_mode));
        int32_t num_ch_out_samp_bits   = (stride_x == 1) ? 4              : (4+en2x8_mode);         
     
        uint8_t N_g                 = param.batch;
        uint8_t X_g                 = fc_mode ? 1 : ((sv_out_width * param.batch) >> num_x_out_samp_bits) ;// excess width update at edge
        uint8_t Co_g                = (param.conv_type==CONV2D_LAYER1)? (nofms >> 3) : (nofms >> num_ch_out_samp_bits) ;
        uint8_t Y_g                 = param.oyp >> num_y_out_samp_bits;

        param.num_Co                = Co_g-1;
        param.num_X                 = X_g-1;
        uint8_t num_Kx_adj          = (param.conv_type==CONV2D_LAYER1) ? param.conv_type:0;
        num_Kx                      = param.kx-1-(num_Kx_adj<<1);    
        num_Ky                      = param.ky-1;

        uint8_t Ci_g                = nifms >> 3;
        param.inner_g               = param.kx * param.ky * Ci_g;
        uint8_t fc_loop_count       = (Co_g > 4) ? Co_g : 4; 
        param.outer_g               = fc_mode ? fc_loop_count : X_g  * Y_g  * Co_g;

        uint16_t step_Xi            = (2-en2x8_mode)*64*stride_x;
        uint16_t step_Yi            = step_yi * stride_y;
        uint16_t step_Ky            = step_yi;
        uint16_t step_Kx            = 8 << param.batch_log2;
        param.ofm_offset_top        = (param.ofm_offset_packed >>4)*step_yo;    

        // Setup increments    
        param.step_align            = (step_Kx & 15 ? 2 : (step_Kx & 31 ? 3 : (step_Kx & 63 ? 4 : 0)));  
        param.shfl_1                = (N_g == 1) ? T64_8x2_lo : (N_g ==2 ? T128_4x2_lo : T256_2x2_lo);
        param.inc_A                 = fc_mode ? 0 : (en2x8_mode ? step_Yi : 64);

        inc_rev                     = (stride_x == 1) ? -param.inc_A : -4*64;    
        inc_Kx                      = inc_rev + step_Kx;
        inc_Ky                      = inc_rev - step_Kx*(param.kx-1) + step_Ky;
        inc_Ci                      = fc_mode ? 64 : (inc_rev - step_Kx*(param.kx-1) - step_Ky*(param.ky-1) + step_ci); // TODO: change for edge excess width when Ci>8

        inc_Xi                      = -step_ci*Ci_g + step_Xi;
        inc_Co_rev                  = fc_mode ? -(64*(nifms>>3)): -step_ci*Ci_g - step_Xi*(X_g-1);
        inc_Yi                      = -step_ci*Ci_g - step_Xi*(X_g-1) + (1+en2x8_mode*(stride_x == 1))*step_Yi;

        SETUP_ITR3D_PARAM(iterator_inner,inc_Kx,num_Kx,inc_Ky,num_Ky,inc_Ci,param)
        SETUP_ITR3D_PARAM(iterator_outer,inc_Xi,param.num_X,inc_Co_rev,param.num_Co,inc_Yi,param)

        if(en2x8_mode){
            param.inc_S_0               =  (stride_x == 1) ? step_yo            : 0/*TODO*/;
            param.inc_S_1               =  (stride_x == 1) ? (-step_yo+step_co) : 0/*TODO*/;

            #ifdef CONV2D_2x8_WIDTH_SUPPORT
                param.num_Xo            =  X_g-1;
                param.inc_Xo            =  (stride_x == 1) ? -step_yo -step_co + 64        : 0/*TODO*/;
                param.inc_Co            =  (stride_x == 1) ? -step_yo -64*(X_g-1) +step_co : 0/*TODO*/;     
                param.inc_Yo            =  (stride_x == 1) ? 64                            : 0/*TODO*/;          
            #else
                param.num_Xo            =  (stride_x == 1) ? Co_g-1             : 0/*TODO*/;
                param.inc_Xo            =  (stride_x == 1) ? -step_yo + step_co : 0/*TODO*/;
                param.inc_Co            =  (stride_x == 1) ? step_co            : 0/*TODO*/;
            #endif      

        } else {
            param.inc_S_0               =  64;
            param.inc_S_1               =  (-64+step_co);

            #ifdef CONV2D_2x8_WIDTH_SUPPORT
                param.num_Xo            =  X_g-1;
                param.inc_Xo            =  64 - step_co;
                param.inc_Co            =  64 + (param.ofm_width_pad + param.ofm_offset_left)*8;
                param.inc_Yo            =  64 + (param.ofm_width_pad + param.ofm_offset_left)*8;        
            #else
                param.num_Xo            =  X_g-1;
                param.inc_Xo            =  64 - step_co;
                param.inc_Co            =  64 + (param.ofm_width_pad + param.ofm_offset_left)*8;
            #endif   
        }    


        inc_w_Ci_rev                = -param.inner_g*128;   
        inc_w_Co_rev                =  inc_w_Ci_rev*Co_g;  

        SETUP_ITR3D_PARAM(iterator_weights,inc_w_Ci_rev,param.num_X,0,param.num_Co,inc_w_Co_rev,param)
        
        #ifdef CONV2D_STX_OPT_BIAS_ITER
            int inc_bias     = 128;
            int inc_bias_rev = -inc_bias * param.num_Co;
            SETUP_ITR3D_PARAM(iterator_bias, 0, param.num_X, inc_bias, param.num_Co, inc_bias_rev,param)
        #endif
    
    } else {

        uint8_t N_g                 = param.batch;
        uint8_t X_g                 = (sv_out_width * param.batch) >> 3;// excess width update at edge
        uint8_t Co_g                = (param.conv_type == CONV2D_LAYER1)? (nofms >> 3) : (nofms >> 4) ;

        param.num_Co                = Co_g-1;
        param.num_X                 = X_g-1;
        uint8_t num_Kx_adj          = (param.conv_type==CONV2D_LAYER1) ? param.conv_type:0;
        num_Kx                      = param.kx-1-(num_Kx_adj<<1);
        num_Ky                      = param.ky-1;

        uint8_t Ci_g                = nifms >> 3;
        param.inner_g               = param.kx * param.ky * Ci_g;
        param.outer_g               = X_g  * param.oyp  * Co_g;
        
        uint16_t step_Yi            = step_yi * stride_y;
        uint16_t step_Ky            = step_yi;
        uint16_t step_Kx            = 8 << param.batch_log2;

        int stride_2_flag           = (stride_x == 2);
        int inc_Xi_partial          = (1 + stride_2_flag) * 64; 
        
        param.step_align            = (step_Kx & 15 ? 2 : (step_Kx & 31 ? 3 : (step_Kx & 63 ? 4 : 0)));
        param.shfl_1                = ((1-stride_2_flag) ? T512_1x2_lo : (N_g == 1 ? T64_8x2_lo : (N_g ==2 ? T128_4x2_lo : T256_2x2_lo)));
        param.shfl_2                = (1-stride_2_flag) ? T512_1x2_lo : T512_1x2_hi;
        
        inc_Kx                      = -2*64 + step_Kx;
        inc_Ky                      = -2*64 - step_Kx * (param.kx-1) + step_Ky;
        inc_Ci                      = -2*64 - step_Kx * (param.kx-1) - step_Ky * (param.ky-1) + step_ci; // TODO: change for edge excess width when Ci>8
        
        inc_Xi                      = -step_ci * Ci_g + inc_Xi_partial;
        inc_Co_rev                  = -step_ci * Ci_g - inc_Xi_partial * (X_g-1);
        inc_Yi                      = -step_ci * Ci_g - inc_Xi_partial * (X_g-1) + step_Yi;

        SETUP_ITR3D_PARAM(iterator_inner,inc_Kx,num_Kx,inc_Ky,num_Ky,inc_Ci,param)
        SETUP_ITR3D_PARAM(iterator_outer,inc_Xi,param.num_X,inc_Co_rev,param.num_Co,inc_Yi,param)

        param.inc_S_0     =  step_co;
        param.inc_Xo      =  64 - step_co;
        param.inc_Co      =  64 + param.ofm_width_pad*8;

        inc_w_Ci_rev = -param.inner_g * 128;
        inc_w_Co_rev =  inc_w_Ci_rev * Co_g;

        SETUP_ITR3D_PARAM(iterator_weights,inc_w_Ci_rev,param.num_X,0,param.num_Co,inc_w_Co_rev,param)    
        
    }
}

#else
static void setup_conv2d_params (int8_t * params_ptr, conv2d_params_t &param, uint8_t batch_size) {
    int32_t inc_rev;
    int32_t inc_Kx;
    int32_t inc_Ky;
    uint32_t num_Kx;
    uint32_t num_Ky;
    int32_t inc_Ci;

    int32_t inc_Co_rev;
    int32_t inc_Xi;
    int32_t inc_Yi;   

    int32_t inc_w_Ci_rev;
    int32_t inc_w_Co_rev;

    //Copy 32 bytes of layer params
    aie::vector<int32,8> pars = aie::load_v<8>((int32 *)(params_ptr));
    aie::store_v( (int32  __aie_dm_resource_a *) &param, pars);

    uint8_t ksize         = param.ksize;
    uint8_t stride_bits   = param.stride_bits;

    uint8_t ksize_x       =  ksize;
    uint8_t ksize_y       =  ksize;
    uint8_t stride_bits_x = stride_bits;
    uint8_t stride_bits_y = stride_bits;
    #ifdef USE_NUM_CH_SCALE
    param.ifm             = (param.ifm_scaled)<<3;//Number of channels encoded with scale factor of 8
    uint16_t nifms         = param.ifm;//Number of channels encoded with scale factor of 8
    #else
    uint8_t nifms         = param.ifm;
    #endif
    #ifdef USE_LARGE_IFMSV_WIDTH
    uint16_t ifm_x        = (param.lrelu_alpha==0)?(uint16_t) param.ifm_xsize:(uint16_t)param.lrelu_alpha;
    #else
    uint8_t ifm_x         = param.ifm_xsize;
    #endif

    int calc_edge_output  = 0;
    #ifndef USE_NUM_CH_SCALE 
    if(param.ifm == 4)//Currently only 11x11s4 lyr-1 is supported with this if-condition.TODO bring 7x7s2 lyr-1 under this. 
    {       
        nifms         = 8;
        ksize_x       = (ksize + 1)>>1;        
        ifm_x         = ifm_x >> 1;        
        stride_bits_x = stride_bits>>1;              
    } 
    else
    #endif
    {        
        if(param.conv_type==CONV2D_7x7S2_LYR1_EDGE) {
            calc_edge_output = 1;
            param.conv_type  = CONV2D_7x7S2_LYR1;            
        }

        int value = (ksize>>4) & 0x0F;
        ksize_x = (value==0) ? ksize : value;
        ksize_y = ksize & 0x0F;

        ksize_x = (param.conv_type==CONV2D_7x7S2_LYR1) ? (ksize_x + 1)>>1 : ksize_x;
        stride_bits_x = (param.conv_type==CONV2D_7x7S2_LYR1) ? 0 : stride_bits;                
    }

    uint8_t stride_x = 1 << stride_bits_x;
    uint8_t stride_y = 1 << stride_bits_y;       

    if(stride_x == 1)
    { 
        int32_t en2x8_mode             = (int32_t) *(params_ptr+35); 
        int32_t num_x_out_samp         = 16 - 8*en2x8_mode;
        int32_t num_x_out_samp_bits    = 4 - en2x8_mode;    
        int32_t num_y_out_samp         = (stride_x == 1) ? (1+en2x8_mode) : 1;
        int32_t num_y_out_samp_bits    = (stride_x == 1) ? en2x8_mode     : 0;
        int32_t num_ch_out_samp        = (stride_x == 1) ? 16             : (16*(1+en2x8_mode));
        int32_t num_ch_out_samp_bits   = (stride_x == 1) ? 4              : (4+en2x8_mode);         

        // Extract IFM sign packed with ifm2_sign (called in0_sign)
        uint8_t tmp                 = param.in0_sign; // offset 30 input0 sign for add2d
        param.ifm_sign              = (int) ((tmp>>4) & 0x01); // offset 30 input sign for conv2d
        param.in0_sign              = (tmp & 0x01); // offset 30 input0 sign for add2d

       // Fixes related to layer param copy 
        param.psum_buff_offset      = (param.psum_buff_offset_scaled)*128*(1-param.out_mode);
        param.ofm_offset_packed     = 0x00; // hard-coding to zero, OFM offset left and top are not supported (and never used) yet!
        param.ofm_offset_left       = (param.ofm_offset_packed) & 0x0F; 
        #ifdef USE_NUM_CH_SCALE
        param.num_ofm_ch            = (param.num_ofm_ch_scaled)<<3;
        uint16_t nofms              = param.num_ofm_ch;
        #else
        uint8_t nofms               = param.num_ofm_ch;
        #endif

        uint32_t fc_mode            = (param.conv_type==2 || param.conv_type==8) ;
        param.ifm_offset            = 0 ; // will be set in wrapper
        param.wts_offset            = 0 ;  // will be set in wrapper 
        param.out_sign              = 1 - (int) param.run_time_act; //Run time activation 0: Linear   1: ReLU 

        param.curr_depth_iter       = 0; // initialize depth iter to 0
        param.batch_log2            = (batch_size==4) ? 2: ((batch_size==2) ? 1 : 0);     
        param.batch                 = (int)batch_size;     
        param.oyp                   = (param.ifm_ysize - ksize_y + 1) >> stride_bits_y;     
        param.oxp                   = (ifm_x - ksize_x + 1) >> stride_bits_x;     
        int sv_out_width            = param.oxp + ((calc_edge_output<<4)>>param.batch_log2) ; 
        param.ky                    = ksize_y;     
        param.kx                    = ksize_x;     
        #ifdef CALC_CONV2D_OFM_LEN
        // recalculate OFM length, ignore supplied value, it will be deprecated. Needed for non-uniform tiling
        param.ofm_len               = sv_out_width*param.oyp*nofms*param.batch;
        #endif 
        param.stride                = 1 << stride_bits_y;       

        int align_kx                = max(1, 8 >> param.batch_log2);
        int ixp                     = param.oxp * stride_x + ((param.kx-1+align_kx-1)/align_kx)*align_kx;
        int iyp                     = param.oyp * stride_y +   param.ky-1;
        int step_co                 = (sv_out_width + param.ofm_width_pad + param.ofm_offset_left) * (8 << param.batch_log2);
        int step_ci                 = ixp * (8 << param.batch_log2);
        int step_yo                 = step_co * nofms >> 3;
        int step_yi                 = step_ci * nifms >> 3;
        param.hdr_len               = (LAYER_PARAM_WORDS+nofms*BIAS_MULTIPLE);

        uint8_t N_g                 = param.batch;
        uint8_t X_g                 = fc_mode ? 1 : ((sv_out_width * param.batch) >> num_x_out_samp_bits) ;// excess width update at edge
        uint8_t Co_g                = (param.conv_type==CONV2D_LAYER1)? (nofms >> 3) : (nofms >> num_ch_out_samp_bits) ;
        uint8_t Y_g                 = param.oyp >> num_y_out_samp_bits;

        param.num_Co                = Co_g-1;
        param.num_X                 = X_g-1;
        uint8_t num_Kx_adj          = (param.conv_type==CONV2D_LAYER1) ? param.conv_type:0;
        num_Kx                      = param.kx-1-(num_Kx_adj<<1);    
        num_Ky                      = param.ky-1;

        uint8_t Ci_g                = nifms >> 3;
        param.inner_g               = param.kx * param.ky * Ci_g;
        uint8_t fc_loop_count       = (Co_g > 4) ? Co_g : 4; 
        param.outer_g               = fc_mode ? fc_loop_count : X_g  * Y_g  * Co_g;

        uint16_t step_Xi            = (2-en2x8_mode)*64*stride_x;
        uint16_t step_Yi            = step_yi * stride_y;
        uint16_t step_Ky            = step_yi;
        uint16_t step_Kx            = 8 << param.batch_log2;
        param.ofm_offset_top        = (param.ofm_offset_packed >>4)*step_yo;    

        // Setup increments    
        param.step_align            = (step_Kx & 15 ? 2 : (step_Kx & 31 ? 3 : (step_Kx & 63 ? 4 : 0)));  
        param.shfl_1                = (N_g == 1) ? T64_8x2_lo : (N_g ==2 ? T128_4x2_lo : T256_2x2_lo);
        param.inc_A                 = fc_mode ? 0 : (en2x8_mode ? step_Yi : 64);

        inc_rev                     = (stride_x == 1) ? -param.inc_A : -4*64;    
        inc_Kx                      = inc_rev + step_Kx;
        inc_Ky                      = inc_rev - step_Kx*(param.kx-1) + step_Ky;
        inc_Ci                      = fc_mode ? 64 : (inc_rev - step_Kx*(param.kx-1) - step_Ky*(param.ky-1) + step_ci); // TODO: change for edge excess width when Ci>8

        inc_Xi                      = -step_ci*Ci_g + step_Xi;
        inc_Co_rev                  = fc_mode ? -(64*(nifms>>3)): -step_ci*Ci_g - step_Xi*(X_g-1);
        inc_Yi                      = -step_ci*Ci_g - step_Xi*(X_g-1) + (1+en2x8_mode*(stride_x == 1))*step_Yi;

        SETUP_ITR3D_PARAM(iterator_inner,inc_Kx,num_Kx,inc_Ky,num_Ky,inc_Ci,param)
        SETUP_ITR3D_PARAM(iterator_outer,inc_Xi,param.num_X,inc_Co_rev,param.num_Co,inc_Yi,param)

        if(en2x8_mode){
            param.inc_S_0               =  (stride_x == 1) ? step_yo            : 0/*TODO*/;
            param.inc_S_1               =  (stride_x == 1) ? (-step_yo+step_co) : 0/*TODO*/;

            #ifdef CONV2D_2x8_WIDTH_SUPPORT
                param.num_Xo            =  X_g-1;
                param.inc_Xo            =  (stride_x == 1) ? -step_yo -step_co + 64        : 0/*TODO*/;
                param.inc_Co            =  (stride_x == 1) ? -step_yo -64*(X_g-1) +step_co : 0/*TODO*/;     
                param.inc_Yo            =  (stride_x == 1) ? 64                            : 0/*TODO*/;          
            #else
                param.num_Xo            =  (stride_x == 1) ? Co_g-1             : 0/*TODO*/;
                param.inc_Xo            =  (stride_x == 1) ? -step_yo + step_co : 0/*TODO*/;
                param.inc_Co            =  (stride_x == 1) ? step_co            : 0/*TODO*/;
            #endif      

        } else {
            param.inc_S_0               =  64;
            param.inc_S_1               =  (-64+step_co);

            #ifdef CONV2D_2x8_WIDTH_SUPPORT
                param.num_Xo            =  X_g-1;
                param.inc_Xo            =  64 - step_co;
                param.inc_Co            =  64 + (param.ofm_width_pad + param.ofm_offset_left)*8;
                param.inc_Yo            =  64 + (param.ofm_width_pad + param.ofm_offset_left)*8;        
            #else
                param.num_Xo            =  X_g-1;
                param.inc_Xo            =  64 - step_co;
                param.inc_Co            =  64 + (param.ofm_width_pad + param.ofm_offset_left)*8;
            #endif   
        }    


        inc_w_Ci_rev                = -param.inner_g*128;   
        inc_w_Co_rev                =  inc_w_Ci_rev*Co_g;  

        SETUP_ITR3D_PARAM(iterator_weights,inc_w_Ci_rev,param.num_X,0,param.num_Co,inc_w_Co_rev,param)
        
        #ifdef CONV2D_STX_OPT_BIAS_ITER
            int inc_bias     = 128;
            int inc_bias_rev = -inc_bias * param.num_Co;
            SETUP_ITR3D_PARAM(iterator_bias, 0, param.num_X, inc_bias, param.num_Co, inc_bias_rev,param)
        #endif
    
    } else {
    
        // Extract IFM sign packed with ifm2_sign (called in0_sign)
        uint8_t tmp                 = param.in0_sign; // offset 30 input0 sign for add2d
        param.ifm_sign              = (int) ((tmp>>4) & 0x01); // offset 30 input sign for conv2d
        param.in0_sign              = (tmp & 0x01); // offset 30 input0 sign for add2d

        param.ofm_offset_packed     = 0x00; // hard-coding to zero, OFM offset left and top are not supported (and never used) yet!
       // Fixes related to layer param copy
        param.psum_buff_offset      = (param.psum_buff_offset_scaled)*128*(1-param.out_mode);
        
        #ifdef USE_NUM_CH_SCALE
        param.num_ofm_ch            = (param.num_ofm_ch_scaled)<<3;
        uint16_t nofms              = param.num_ofm_ch;
        #else
        uint8_t nofms               = param.num_ofm_ch;
        #endif

        uint32_t fc_mode = (param.conv_type==2 || param.conv_type==8) ;
        param.ifm_offset            = 0 ; // will be set in wrapper
        param.wts_offset            = 0 ;  // will be set in wrapper
        param.out_sign              = 1 - (int) param.run_time_act; //Run time activation 0: Linear   1: ReLU
        
        
        param.batch_log2            = (batch_size==4)?2:((batch_size==2)?1:0);
        param.batch                 = (int)batch_size;
        param.oyp                   = (param.ifm_ysize - ksize_y + 1) >> stride_bits_y;
        param.oxp                   = (ifm_x - ksize_x + 1) >> stride_bits_x;

        int sv_out_width            = param.oxp + ((calc_edge_output<<4)>>param.batch_log2) ;

        param.curr_depth_iter       = 0; // initialize depth iter to 0
        param.ky                    = ksize_y;
        param.kx                    = ksize_x;
        #ifdef CALC_CONV2D_OFM_LEN
        // recalculate OFM length, ignore supplied value, it will be deprecated. Needed for non-uniform tiling
        param.ofm_len               = sv_out_width*param.oyp*nofms*param.batch;
        #endif 
        param.stride                = 1 << stride_bits_y;

        //param.conv2d_1x16x16  =  ((param.oxp & (uint8_t)0x0F) == 0) && (param.num_ifm_depth_iter == 1) ? 1 : 0;
        //uint8_t param__conv2d_1x16x16 = param.conv2d_1x16x16;
        
        uint8_t param__conv2d_1x16x16 = 0;

        int align_kx                = max(1, 8 >> param.batch_log2);
        int ixp                     = param.oxp * stride_x + ((param.kx-1+align_kx-1) / align_kx)*align_kx;
        int iyp                     = param.oyp * stride_y +   param.ky-1;
        int step_co                 = (sv_out_width + param.ofm_width_pad) * (8 << param.batch_log2);
        int step_ci                 = ixp * (8 << param.batch_log2);
        int step_yo                 = step_co * nofms >> 3;
        int step_yi                 = step_ci * nifms >> 3;
        param.hdr_len               = (LAYER_PARAM_WORDS + nofms * BIAS_MULTIPLE);
        uint8_t N_g                 = param.batch;
        uint8_t X_g                 = (sv_out_width * param.batch) >> (3 + param__conv2d_1x16x16);// excess width update at edge
        uint8_t Co_g                = (param.conv_type == CONV2D_LAYER1)? (nofms >> 3) : (nofms >> 4) ;

        param.num_Co                = Co_g-1;
        param.num_X                 = X_g-1;
        uint8_t num_Kx_adj          = (param.conv_type==CONV2D_LAYER1) ? param.conv_type:0;
        num_Kx                      = param.kx-1-(num_Kx_adj<<1);
        num_Ky                      = param.ky-1;

        uint8_t Ci_g                = nifms >> 3;
        param.inner_g               = param.kx * param.ky * Ci_g;
        param.outer_g               = X_g  * param.oyp  * Co_g;
        
        uint16_t step_Yi            = step_yi * stride_y;
        uint16_t step_Ky            = step_yi;
        uint16_t step_Kx            = 8 << param.batch_log2;

        int stride_1_flag           = (stride_x == 1) && (param__conv2d_1x16x16 == 0);
        int stride_2_flag           = (stride_x == 2);
        int inc_Xi_partial          = (1 + stride_2_flag) * 64 * (1<<param__conv2d_1x16x16); 
        
        param.step_align            = (step_Kx & 15 ? 2 : (step_Kx & 31 ? 3 : (step_Kx & 63 ? 4 : 0)));
        param.shfl_1                = ((1-stride_2_flag) ? T512_1x2_lo : (N_g == 1 ? T64_8x2_lo : (N_g ==2 ? T128_4x2_lo : T256_2x2_lo)));
        param.shfl_2                = (1-stride_2_flag) ? T512_1x2_lo : T512_1x2_hi;
        
        inc_Kx                      = -(2 + 2*param__conv2d_1x16x16) * 64 + stride_1_flag*64 + step_Kx;
        inc_Ky                      = -(2 + 2*param__conv2d_1x16x16) * 64 + stride_1_flag*64 - step_Kx * (param.kx-1) + step_Ky;
        inc_Ci                      = -(2 + 2*param__conv2d_1x16x16) * 64 + stride_1_flag*64 - step_Kx * (param.kx-1) - step_Ky * (param.ky-1) + step_ci; // TODO: change for edge excess width when Ci>8
        
        inc_Xi                      = -step_ci * Ci_g + inc_Xi_partial;
        inc_Co_rev                  = -step_ci * Ci_g - inc_Xi_partial * (X_g-1);
        inc_Yi                      = -step_ci * Ci_g - inc_Xi_partial * (X_g-1) + step_Yi;

        SETUP_ITR3D_PARAM(iterator_inner,inc_Kx,num_Kx,inc_Ky,num_Ky,inc_Ci,param)
        SETUP_ITR3D_PARAM(iterator_outer,inc_Xi,param.num_X,inc_Co_rev,param.num_Co,inc_Yi,param)

        param.inc_S_0     =  -64*param__conv2d_1x16x16 + step_co;
        param.inc_Xo      =  64 - step_co;
        param.inc_Co      =  64 + param.ofm_width_pad*8;

        inc_w_Ci_rev = -param.inner_g * 128;
        inc_w_Co_rev =  inc_w_Ci_rev * Co_g;

        SETUP_ITR3D_PARAM(iterator_weights,inc_w_Ci_rev,param.num_X,0,param.num_Co,inc_w_Co_rev,param)    
        
    }

}
#endif

#else

static void setup_conv2d_params (int8_t * params_ptr, conv2d_params_t &param, uint8_t batch_size) {
    int32_t inc_Kx;
    int32_t inc_Ky;
    uint32_t num_Kx;
    uint32_t num_Ky;
    int32_t inc_Ci;

    int32_t inc_Co_rev;
    int32_t inc_Xi;
    int32_t inc_Yi;

    int32_t inc_w_Ci_rev;
    int32_t inc_w_Co_rev;

    //Copy 32 bytes of layer params
    //get_layer_params((uint32_t *)params_ptr,(uint32_t *)&param);
    aie::vector<int32,8> pars = aie::load_v<8>((int32 *)(params_ptr));
    aie::store_v( (int32  __aie_dm_resource_a *) &param, pars);

    uint8_t ksize         = param.ksize;
    uint8_t stride_bits   = param.stride_bits;

    uint8_t ksize_x       =  ksize;
    uint8_t ksize_y       =  ksize;
    uint8_t stride_bits_x = stride_bits;
    uint8_t stride_bits_y = stride_bits;
    #ifdef USE_NUM_CH_SCALE
    param.ifm             = (param.ifm_scaled)<<3;//Number of channels encoded with scale factor of 8
    uint16_t nifms         = param.ifm;//Number of channels encoded with scale factor of 8
    #else
    uint8_t nifms         = param.ifm;
    #endif
    #ifdef USE_LARGE_IFMSV_WIDTH
    uint16_t ifm_x        = (param.lrelu_alpha==0)?(uint16_t) param.ifm_xsize:(uint16_t)param.lrelu_alpha;
    #else
    uint8_t ifm_x         = param.ifm_xsize;
    #endif

    int calc_edge_output  = 0;
    #ifndef USE_NUM_CH_SCALE
    if(param.ifm == 4)//Currently only 11x11s4 lyr-1 is supported with this if-condition.TODO bring 7x7s2 lyr-1 under this.
    {
        nifms         = 8;
        ksize_x       = (ksize + 1)>>1;
        ifm_x         = ifm_x >> 1;
        stride_bits_x = stride_bits>>1;
    }
    else
    #endif
    {
        if(param.conv_type==CONV2D_7x7S2_LYR1_EDGE) {
            calc_edge_output = 1;
            param.conv_type  = CONV2D_7x7S2_LYR1;
        }

        int value = (ksize>>4) & 0x0F;

        int value = (ksize>>4) & 0x0F;
        ksize_x = (value==0) ? ksize : value;
        ksize_y = ksize & 0x0F;

        ksize_x = (param.conv_type==CONV2D_7x7S2_LYR1) ? (ksize_x + 1)>>1 : ksize_x;
        stride_bits_x = (param.conv_type==CONV2D_7x7S2_LYR1) ? 0 : stride_bits;
    }

    uint8_t stride_x = 1 << stride_bits_x;
    uint8_t stride_y = 1 << stride_bits_y;

    // Extract IFM sign packed with ifm2_sign (called in0_sign)
    uint8_t tmp                 = param.in0_sign; // offset 30 input0 sign for add2d
    param.ifm_sign              = (int) ((tmp>>4) & 0x01); // offset 30 input sign for conv2d
    param.in0_sign              = (tmp & 0x01); // offset 30 input0 sign for add2d

    param.ofm_offset_packed     = 0x00; // hard-coding to zero, OFM offset left and top are not supported (and never used) yet!
   // Fixes related to layer param copy
    param.psum_buff_offset      = (param.psum_buff_offset_scaled)*128*(1-param.out_mode);
    param.ofm_offset_left       = (param.ofm_offset_packed) & 0x0F;
    #ifdef USE_NUM_CH_SCALE
    param.num_ofm_ch            = (param.num_ofm_ch_scaled)<<3;
    uint16_t nofms              = param.num_ofm_ch;
    #else
    uint8_t nofms               = param.num_ofm_ch;
    #endif

    uint32_t fc_mode = (param.conv_type==2 || param.conv_type==8) ;
    param.ifm_offset            = 0 ; // will be set in wrapper
    param.wts_offset            = 0 ;  // will be set in wrapper
    param.out_sign              = 1 - (int) param.run_time_act; //Run time activation 0: Linear   1: ReLU

    param.curr_depth_iter       = 0; // initialize depth iter to 0
    param.batch_log2            = (batch_size==4)?2:((batch_size==2)?1:0);
    param.batch                 = (int)batch_size;
    param.oyp                   = (param.ifm_ysize - ksize_y + 1) >> stride_bits_y;
    param.oxp                   = (ifm_x - ksize_x + 1) >> stride_bits_x;
    int sv_out_width            = param.oxp + ((calc_edge_output<<4)>>param.batch_log2) ;
    param.ky                    = ksize_y;
    param.kx                    = ksize_x;

    param.stride                = 1 << stride_bits_y;

    #ifdef BYTE_ALIGNMENT_64
        int align_kx            = max(1, 8 >> param.batch_log2);
    #else
        int align_kx            = max(1, 4 >> param.batch_log2);
    #endif

    int ixp                     = param.oxp * stride_x + ((param.kx-1+align_kx-1)/align_kx)*align_kx;
    int iyp                     = param.oyp * stride_y +   param.ky-1;
    int step_co                 = (sv_out_width + param.ofm_width_pad + param.ofm_offset_left) * (8 << param.batch_log2);
    int step_ci                 = ixp * (8 << param.batch_log2);
    int step_yo                 = step_co * nofms >> 3;
    int step_yi                 = step_ci * nifms >> 3;
    param.hdr_len               = (LAYER_PARAM_WORDS+nofms*BIAS_MULTIPLE);
    uint8_t N_g                 = param.batch;
    //uint8_t sh                  = fc_mode ? 2:4;
    uint8_t X_g                 =  (fc_mode) ? 1 : ((sv_out_width * param.batch * stride_x) >> 4) ;// excess width update at edge
    uint8_t Co_g                = (param.conv_type==CONV2D_LAYER1)? (nofms >> 3) : (nofms >> 4) ;

    param.num_Co                = Co_g-1;
    param.num_X                 = X_g-1;
    uint8_t num_Kx_adj          = (param.conv_type==CONV2D_LAYER1) ? param.conv_type:0;
    num_Kx                      = param.kx-1-(num_Kx_adj<<1);
    num_Ky                      = param.ky-1;

    uint8_t Ci_g                = nifms >> 3;
    param.inner_g               = param.kx * param.ky * Ci_g;
    param.outer_g               = (fc_mode==1) ? 4: X_g  * param.oyp  * Co_g;
    uint16_t step_Xo            = 8;
    uint16_t step_Yi            = step_yi * stride_y;
    uint16_t step_Xi            = 8 * stride_x;
    uint16_t step_Ky            = step_yi;
    uint16_t step_Kx            = 8 << param.batch_log2;
    param.ofm_offset_top        = (param.ofm_offset_packed >>4)*step_yo;

// Setup increments

    #ifdef ENABLE_STRIDE_4
      int SN0_g                   = !(stride_x > 1 && N_g < 4) + !(stride_x == 4 && N_g < 4);
                                                                 //0 for stride = 4 , N < 4
                                                                 //1 for stride = 2 , N < 4
                                                                 //2 else
      int SN1_g                   = 1 << SN0_g;
                                             //1 for stride = 4 , N < 4
                                             //2 for stride = 2 , N < 4
                                             //4 else
    #else
      int SN0_g                   = !(stride_x == 2);
      int SN1_g                   = 4 << SN0_g;
    #endif
    int chnd                    = 32;
    int chnd_T                  = 64;
    param.step_align            = (step_Kx & 15 ? 2 : (step_Kx & 31 ? 3 : (step_Kx & 63 ? 4 : 0))); //batch-4 also needs aligment
    #ifdef ENABLE_STRIDE_4
      param.shfl_0              = (SN0_g > 1 ? T512_1x2_lo : (N_g == 1 ? T64_8x2_lo : T128_4x2_lo));
      param.shfl_1              = (SN0_g > 1 ? T512_1x2_hi : (N_g == 1 ? T64_8x2_lo : T128_4x2_lo));
      param.shfl_2              = (SN0_g > 0 ? T512_1x2_hi : (N_g == 1 ? T64_8x2_lo : T128_4x2_lo));
    #else
      param.shfl_1                = (SN0_g ? T512_1x2_hi : (N_g == 1 ? T64_8x2_lo : (N_g ==2 ? T128_4x2_lo : T256_2x2_lo))); //Updated to support batch-4 interleaving.
    #endif
    param.inc_A                 =  (fc_mode) ? 0 : (step_Xi * SN1_g);
    inc_Kx                      = -1 * step_Xi * SN1_g + step_Kx;
    inc_Ky                      = -1 * step_Xi * SN1_g - step_Kx * (param.kx-1) + step_Ky; // go to next lower row for procesing next row of filter kernel
    inc_Ci                      =  (fc_mode) ? 32 : (-1 * step_Xi * SN1_g - step_Kx * (param.kx-1) - step_Ky * (param.ky-1) + step_ci); // TODO: change for edge excess width when Ci>8
    inc_Xi                      = -step_ci * Ci_g + 2 * step_Xi * SN1_g;
    inc_Co_rev                  = (fc_mode) ? -(32 * (nifms >> 3)): -step_ci * Ci_g - 2 * step_Xi * SN1_g * (X_g-1);
    inc_Yi                      = -step_ci * Ci_g - 2 * step_Xi * SN1_g * (X_g-1) + step_Yi;

    SETUP_ITR3D_PARAM(iterator_inner,inc_Kx,num_Kx,inc_Ky,num_Ky,inc_Ci,param)
    SETUP_ITR3D_PARAM(iterator_outer,inc_Xi,param.num_X,inc_Co_rev,param.num_Co,inc_Yi,param)

    int bits_O                  =  8;
    int bits_T                  = 16;

    #ifdef ENABLE_STRIDE_4
      int inc_base_O  =  (32 * bits_O/8);
      int inc_base_T  =  (32 * bits_T/8);
      param.inc_S_0   =  (SN0_g > 0) * inc_base_O;
      param.inc_S_1   =  (SN0_g > 0) * inc_base_O * (SN0_g > 1 ? 1 : -1);
      param.inc_S_2   =  -(2*param.inc_S_0 + param.inc_S_1) + (step_co * bits_O/8);

      param.inc_Xo    =  inc_base_O - (step_co * bits_O/8);
      param.inc_Co    =  inc_base_O + (param.ofm_width_pad+param.ofm_offset_left)*8;

      param.inc_ST_0  = (fc_mode) ? 64   : ((SN0_g > 0) * inc_base_T);
      param.inc_ST_1  = (fc_mode) ? 0    : ((SN0_g > 0) * inc_base_T * (SN0_g > 1 ? 1 : -1));
      param.inc_ST_2  = (fc_mode) ? -64  : (-(2*param.inc_ST_0 + param.inc_ST_1) + (step_co * bits_T/8));

      param.inc_Xo_T  =  inc_base_T - (step_co * bits_T/8);
      param.inc_Co_T  =  inc_base_T + (param.ofm_width_pad+param.ofm_offset_left)*8;
    #else
      int inc_S_0_base  = step_Xo * 8 * bits_O/8;
      int inc_ST_0_base = step_Xo * 8 * bits_T/8;

      param.inc_ST_0    = (inc_ST_0_base * chess_copy(SN0_g) );
      param.inc_ST_1    = (fc_mode) ? -64 : (-inc_ST_0_base * chess_copy(SN0_g) + step_co * bits_T/8);

      param.inc_S_0     =  inc_S_0_base * chess_copy(SN0_g);
      param.inc_S_1     = (fc_mode) ? -32 :-inc_S_0_base * chess_copy(SN0_g) + step_co * bits_O/8;

      param.inc_Xo      =  inc_S_0_base - step_co * bits_O/8;
      //param.inc_Co    =  inc_S_0_base;
      param.inc_Co      =  inc_S_0_base + (param.ofm_width_pad+param.ofm_offset_left)*8;
      param.inc_Xo_T    =  inc_S_0_base - step_co * (2*bits_T/8-bits_T/8) + chnd_T; //(1,2) -> 3; (1,4) -> 7; (2,4) -> 6
      param.inc_Co_T    =  inc_ST_0_base;
    #endif

    param.inc_acc_0 = 0;
    param.inc_acc_1 = BIAS_BYTES*DIM_M*DIM_K;
    param.inc_acc_2 = -BIAS_BYTES*DIM_M*DIM_K;
    param.inc_acc_d0 = 0;
    param.inc_acc_d1 =  param.inc_acc_1*OFM_SETS;
    param.inc_acc_rev = (1-Co_g)*param.inc_acc_d1;

    inc_w_Ci_rev = -param.inner_g * 128;
    inc_w_Co_rev =  inc_w_Ci_rev * Co_g;

    SETUP_ITR3D_PARAM(iterator_weights,inc_w_Ci_rev,param.num_X,0,param.num_Co,inc_w_Co_rev,param)
}

#endif

#endif

static void update_offsets (int8_t * params_ptr, conv2d_params_t &param, uint16_t core_col) {
    // Add conv mode for FC based on cascade mode. This glue is needed if conv2d non-cascade kernel is used
    param.conv_type = (core_col==0)?2:8;
    #ifndef FC_EN // use legacy mode to use shift_out for the last column
    param.shift_psum_out = (core_col==LAST_COL)?param.shift_out:param.shift_psum_out;
    #endif
    //uint16_t core_row = (uint16_t) ((core_id & 0xff)-3) ;
    //param.ifm_offset            = (param.row_offset*param.ifm_len)>>2;// Note row offset is now a misnomer, interpret as column number for FC when wts sent over columns
    uint32_t wts_sv_len          = (uint32_t) (*((uint16_t*) (params_ptr+WTS_LEN_OFFSET) )); // extract wts SV length
    param.wts_offset            = (wts_sv_len*core_col )>>2; // generate offet for wts subset from superset based on the row
}

static void update_tiling (int8_t * params_ptr) {
    // Update ifm_sv_width/height,ksize and 2x8 mode in case of non-uniform tiling
    params_ptr[0]=params_ptr[50];//ifm sv width
    params_ptr[1]=params_ptr[51];//ifm sv height
    params_ptr[4]=params_ptr[52];//ksize
    uint32_t temp=(uint32_t) params_ptr[53];// [psum_buff_offset | conv2d 2x8 mode]
    uint32_t en_2x8_mode=temp & 0x0000000F;// conv2d 2x8 mode
    uint32_t psum_buff_offset_scaled=(temp & 0x000000FF)>>4;// conv2d 2x8 mode
    params_ptr[35]=(uint8_t) en_2x8_mode;//conv2d 2x8 mode
    params_ptr[11]=(uint8_t) psum_buff_offset_scaled;
    int ksize_fused_layer=params_ptr[36]; // 36: corresponds to ksize for fused layer (avgpool/maxpool)
    int has_fused_layer=(ksize_fused_layer==0)?0:1; // 36: corresponds to ksize for fused layer (avgpool/maxpool)
    if(has_fused_layer){
       params_ptr[32]=params_ptr[50]+ksize_fused_layer-1;
       params_ptr[33]=params_ptr[51]+ksize_fused_layer-1;
    }
}
#endif  //__CONV2D_PARAMS_H__
