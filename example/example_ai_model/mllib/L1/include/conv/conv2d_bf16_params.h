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

#ifndef __CONV2D_BF16_PARAMS_H__
#define __CONV2D_BF16_PARAMS_H__

#include "layer_params.h"
#include "utils.h"
#include <adf.h>
#include "aie.hpp"

// Basic matrix mult dimensions for AIE-ML
#define DIM_M 4
#define DIM_K 8
#define DIM_N 8

// Bias is 16-bit
#define BIAS_BYTES 2
#define OFM_SETS 2

// Buffer placement directives for IFM, WTs and OFM buffers.
#ifndef IFM_DM_BANK
#define IFM_DM_BANK __aie_dm_resource_a
#endif

#ifndef WT_DM_BANK
#define WT_DM_BANK __aie_dm_resource_a
#endif

// number of bytes set aside at the start of weight buffer for conv kernel parameters
#ifdef LAYER_PARAM_BYTES
#undef LAYER_PARAM_BYTES
#endif
#define LAYER_PARAM_BYTES 64 

#ifdef LAYER_PARAM_WORDS
#undef LAYER_PARAM_WORDS
#endif
#define LAYER_PARAM_WORDS (LAYER_PARAM_BYTES>>2) 

#ifdef ITER_PARAM_BYTES
#undef ITER_PARAM_BYTES
#endif
#define ITER_PARAM_BYTES 32 

#ifdef ITER_PARAM_WORDS
#undef ITER_PARAM_WORDS
#endif
#define ITER_PARAM_WORDS (ITER_PARAM_BYTES>>2) 

#ifdef ITER_PARAM_OFFSET
#undef ITER_PARAM_OFFSET
#endif
#define ITER_PARAM_OFFSET 19

#ifdef BIAS_MULTIPLE
#undef BIAS_MULTIPLE
#endif
#define BIAS_MULTIPLE   2

#ifdef WTS_LEN_OFFSET
#undef WTS_LEN_OFFSET
#endif
#define WTS_LEN_OFFSET 50 

// Needed to check for last column, override from top as required
#ifndef LAST_COL
#define LAST_COL 6
#endif

#if defined(AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16) && (AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16 == 0)
  #undef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
#endif 

struct alignas(v32uint16) conv2d_bf16_internal_params {
    //Layer params
    uint8_t ifm_sv_width;
    uint8_t ifm_sv_height;
    uint16_t ifm_sv_depth;
    uint16_t ofm_sv_depth;
    uint8_t ksize_width; 
    uint8_t ksize_height;
    uint8_t out_mode ;
    uint8_t stride_log2;
    uint8_t psum_buff_offset_scaled;
    uint8_t num_ifm_depth_iters;       
    bfloat16 lrelu_alpha;
    uint8_t ofm_width_pad  ;     
    uint8_t act ;            
    uint8_t conv_type ;          
    uint8_t ofm_offset_packed ;
    uint16_t num_ifm_width_iters;       
    uint16_t num_ifm_height_iters;
    uint16_t num_ofm_depth_iters;
    // Derived params
    int32_t out_sign ;            
    int32_t psum_buff_offset;    
    uint8_t ofm_offset_left ; 
    int32_t psum_in;
    int32_t psum_out;
    int32_t hdr_len ; // larger than 8 bit so retaining
    union {
    uint32_t storage_batch_batch_log2_oyp_oxp;
        struct {
            int8_t batch;
            int8_t batch_log2;
            uint8_t oyp;
            uint8_t oxp;       
        };
    };
    union {
      uint32_t storage_ky_kx_stride_curr_depth_iter;
        struct {
            uint8_t ky;
            uint8_t kx;
            uint8_t stride;
            uint8_t curr_depth_iter;
        };
    };
    uint8_t io_mode;
    uint32_t ofm_offset_top;  // number rows of pixels to be left out for padding at top
    uint8_t exec_type; // This has conv2d generic, layer-1, maxpool2d etc
    uint32_t inner_g; 
    uint32_t outer_g; 
    int32_t  ifm_offset; // IFM offset in 32-bit words to pick subset from superset in FC mode
    int32_t  wts_offset; // WTS offset in 32-bit words to pick subset from superset in FC mode
    DECL_ITR3D(iterator_inner)
    DECL_ITR3D(iterator_outer) 
    DECL_ITR3D(iterator_weights)   
    int32_t inc_A;
    int32_t inc_A_1;
    int32_t inc_Co;
    int32_t inc_Xo;
    int32_t inc_Co_T;
    int32_t inc_Xo_T;
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

    int32_t step_align;
    int32_t shfl_0;
    int32_t shfl_1;
    int32_t shfl_2;
    uint8_t num_Kx;

    uint32_t num_Co;
    uint32_t num_X;
    uint8_t num_W;
    uint8_t num_BN;
};

typedef struct conv2d_bf16_internal_params conv2d_bf16_params_t ;

static INLINE int min_bf16(int a, int b) { return a<b ? a : b; }
static INLINE int max_bf16(int a, int b) { return a>b ? a : b; }

static void setup_conv2d_bf16_iter_params(conv2d_bf16_params_t &param) {
    param.storage_ky_kx_stride_curr_depth_iter += 0x01000000;
    uint8_t curr_depth_iter = (param.storage_ky_kx_stride_curr_depth_iter >> 24);
    param.io_mode = (param.num_ifm_depth_iters>1)*((curr_depth_iter==1)+2*(curr_depth_iter==param.num_ifm_depth_iters)+3*((curr_depth_iter>1)&&(curr_depth_iter<param.num_ifm_depth_iters)));         
    param.psum_in = (param.io_mode==0 || param.io_mode==1) ? 0 :1; 
    uint32_t check_conv_type = (param.conv_type!=2) * (param.conv_type!=8) * (param.conv_type!=CONV2D_7x7S2_LYR1) * (param.conv_type!=5);
    if((param.out_mode==1) && (curr_depth_iter==param.num_ifm_depth_iters)){
        param.psum_out = 1;
        param.exec_type = 1;    
    } else {        
        param.psum_out   = (param.io_mode==0 || param.io_mode==2) ? 0 :1;       
        param.exec_type  = ((param.conv_type==12?0:param.conv_type) << 4)*(param.conv_type!=3)*(check_conv_type) + param.io_mode;
    }

    // TODO: hack for stride=2, add 4 to exec type to pick stride=2 optimized kernel at run time 
#ifdef STRIDE2_OPT
     param.exec_type  = param.exec_type+(4*param.stride_log2*(param.batch < 4)*(param.conv_type!=3));
#endif

     //printf("exec_type=%d\n",param.exec_type);
     if(curr_depth_iter==param.num_ifm_depth_iters){
       param.storage_ky_kx_stride_curr_depth_iter &= 0x00FFFFFF; // Reset depth_iter
    }       
}

#ifdef STRIDE2_OPT //Start of stride-2-optimization supported setup_conv2d_bf16_params
static NOINLINE void setup_conv2d_bf16_params (int8_t * params_ptr, conv2d_bf16_params_t &param, uint8_t batch_size, uint8_t aximm_rtp_enabled = 0) {

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
    
    int calc_edge_output = 0;
    if(param.conv_type==CONV2D_7x7S2_LYR1_EDGE) {
      calc_edge_output = 1;
      param.conv_type = 3 ;
    }
    
   // Fixes related to layer param copy 
    param.psum_buff_offset      = (param.psum_buff_offset_scaled)*128*(1-param.out_mode);
    param.ofm_offset_left       = (param.ofm_offset_packed) & 0x0F; 
    uint8_t nofms               = param.ofm_sv_depth;
    uint8_t stride_log2         = param.stride_log2;
    uint8_t nifms               = param.ifm_sv_depth;
    uint32_t fc_mode = (param.conv_type==2 || param.conv_type==8) ;
    param.ifm_offset            = 0 ; // will be set in wrapper
    param.wts_offset            = 0 ;  // will be set in wrapper 
    param.out_sign              = 1 - (int) param.act; //Run time activation 0: Linear   1: ReLU      
   
    uint8_t ksize_x = param.ksize_width;
    uint8_t ksize_y = param.ksize_height;
    
    uint8_t stride_log2_x = (param.conv_type==CONV2D_7x7S2_LYR1) ? 0 : stride_log2;
    uint8_t stride_log2_y = stride_log2;
    
    uint8_t stride_x = 1 << stride_log2_x;
    uint8_t stride_y = 1 << stride_log2_y;    
           

    uint8_t param__batch_log2   = (batch_size==4)?2:((batch_size==2)?1:0);     
    uint8_t param__batch        = (int)batch_size;     
    uint8_t param__oyp          = ((param.ifm_sv_height - ksize_y) >> stride_log2_y) + 1;     
    uint8_t param__oxp          = ((param.ifm_sv_width - ksize_x) >> stride_log2_x) + 1; 
    uint32_t val0 =  (0xFF & param__batch )
                   | (0xFF00 & (param__batch_log2 << 8))
                   | (0xFF0000 & (param__oyp << 16))
                   | (0xFF000000 & (param__oxp << 24));
    param.storage_batch_batch_log2_oyp_oxp = val0;   

    uint8_t param__ky           = ksize_y;     
    uint8_t param__kx           = ksize_x;     
    uint8_t param__stride       = 1 << stride_log2_y;    
    // param.curr_depth_iter       = 0; // initialize depth iter to 0
    uint32_t val1 =  (0xFF & param__ky )
                   | (0xFF00 & (param__kx << 8))
                   | (0xFF0000 & (param__stride << 16))
                   | (0xFF000000 & (0 << 24));
    param.storage_ky_kx_stride_curr_depth_iter = val1;

    int sv_out_width            = param.oxp + ((calc_edge_output<<4)>>param.batch_log2) ; 
    int align_kx                = max_bf16(1, 4 >> param.batch_log2);
    uint32_t align_kx_bits      = align_kx >> 1;
    int ixp                     = param__oxp * stride_x + ((param__kx-1+align_kx-1) >> align_kx_bits)*align_kx;
    int iyp                     = param.oyp * stride_y +   param.ky-1;
    int step_co                 = (sv_out_width + param.ofm_width_pad + param.ofm_offset_left) * (8 << param.batch_log2);
    int step_ci                 = ixp * (8 << param.batch_log2);
    int step_yo                 = step_co * nofms >> 3;
    int step_yi                 = step_ci * nifms >> 3;
    param.hdr_len               = (!aximm_rtp_enabled ? LAYER_PARAM_WORDS : 0) + nofms*BIAS_MULTIPLE;
    uint8_t N_g                 = param.batch;
    uint8_t X_g                 = fc_mode ? 1: (sv_out_width * param.batch  >> 4); // excess width update at edge  
    uint8_t Co_g                = (param.conv_type==CONV2D_LAYER1)? (nofms >> 3) : (nofms >> 4) ;

    param.num_Co                = Co_g-1;
    param.num_X                 = X_g-1;
    uint8_t num_Kx_adj          = (param.conv_type==CONV2D_LAYER1) ? param.conv_type:0;
    num_Kx                      = param.kx-1-(num_Kx_adj<<1);    
    num_Ky                      = param.ky-1;
    
    param.num_Kx                = num_Kx;

    uint8_t Ci_g                = nifms >> 3;
    param.inner_g               = param.kx * param.ky * Ci_g;
    param.outer_g               = (fc_mode==1) ? 4: X_g  * param.oyp  * Co_g;
    uint16_t step_Xo            = 8;
    uint16_t step_Yi            = step_yi * stride_y;
    uint16_t step_Xi            = 8 * stride_x;
    uint16_t step_Ky            = step_yi;
    uint16_t step_Kx            = 8 << param.batch_log2;
    param.ofm_offset_top        = (param.ofm_offset_packed >>4)*step_yo;  
    
    int chnd                    = 32;
    int chnd_T                  = 64;
    param.step_align            = (step_Kx & 15 ? 2 : (step_Kx & 31 ? 3 : 0));    
  
    int bits_O                  =  8;
    int bits_T                  = 16; 
    int inc_S_0_base = step_Xo * 8 * bits_O/8;
    int inc_ST_0_base = step_Xo * 8 * bits_T/8; 
    
    
       
    if((stride_x == 2) && (param.batch < 4)){              
        param.shfl_0                = (param.batch == 1 ? DINTLV_lo_64o128 : DINTLV_lo_128o256);            
        param.inc_A                 = 32;
        param.inc_A_1               = (stride_x == 2 ? 32 : 0);            
        inc_Kx                      = -(3 * param.inc_A_1) - (4 * param.inc_A) + step_Kx;
        inc_Ky                      = -(3 * param.inc_A_1) - (4 * param.inc_A) - step_Kx * (param.kx-1) + step_Ky;
        inc_Ci                      =  (fc_mode) ? 32 : (-(3 * param.inc_A_1) - (4 * param.inc_A) - step_Kx * (param.kx-1) - step_Ky * (param.ky-1) + step_ci); // TODO: change for edge excess width when Ci>8 
    
    } else { 
        param.shfl_0                = T512_1x2_lo;
        param.shfl_1                = T512_1x2_hi;
        param.inc_A                 =   (param.conv_type == 2 || param.conv_type==8) ? 0 : (step_Xi * 4);
        inc_Kx                      = -3 * step_Xi * 4 + step_Kx;
        inc_Ky                      = -3 * step_Xi * 4 - step_Kx * (param.kx-1) + step_Ky; // go to next lower row for procesing next row of filter kernel
        inc_Ci                      =  (fc_mode) ? 32 : (-3 * step_Xi * 4 - step_Kx * (param.kx-1) - step_Ky * (param.ky-1) + step_ci); // TODO: change for edge excess width when Ci>8
        
    }

    inc_Xi                      = -step_ci * Ci_g + 16 * step_Xi ;
    inc_Co_rev                  = (fc_mode) ? -(32 * (nifms >> 3)): -step_ci * Ci_g - 16 * step_Xi * (X_g-1);
    inc_Yi                      = -step_ci * Ci_g - 16 * step_Xi * (X_g-1) + step_Yi;

    param.inc_ST_0  = inc_ST_0_base;
    param.inc_ST_1  = (fc_mode) ? -64 : (-inc_ST_0_base + step_co * bits_T/8);

    param.inc_S_0  =  inc_S_0_base;
    param.inc_S_1  = (fc_mode) ? -32 : -inc_S_0_base + step_co * bits_O/8;

    SETUP_ITR3D_PARAM(iterator_inner,inc_Kx,num_Kx,inc_Ky,num_Ky,inc_Ci,param)
    SETUP_ITR3D_PARAM(iterator_outer,inc_Xi,param.num_X,inc_Co_rev,param.num_Co,inc_Yi,param)

    param.inc_Xo   =  inc_S_0_base                     - step_co * bits_O/8;
    //param.inc_Co   =  inc_S_0_base;
    param.inc_Co   =  inc_S_0_base + (param.ofm_width_pad+param.ofm_offset_left)*8;
    param.inc_Xo_T =  inc_S_0_base                     - step_co * (2*bits_T/8-bits_T/8) + chnd_T; //(1,2) -> 3; (1,4) -> 7; (2,4) -> 6
    param.inc_Co_T =  inc_ST_0_base;

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
// Setup for stride=2, retain stride=1 setup
#else //(End of stride-2-optimization supported setup_conv2d_bf16_params) or (Start of generic setup_conv2d_bf16_params (with stride-4 support using ENABLE_STRIDE_4) )
static void setup_conv2d_bf16_params (int8_t * params_ptr, conv2d_bf16_params_t &param, uint8_t batch_size=1, uint8_t aximm_rtp_enabled = 0) {

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
    params_ptr+=62;

    param.batch       = (uint8_t) *params_ptr++;
    uint8_t stride_log2   = param.stride_log2;
 
    uint8_t ksize_x = param.ksize_width;
    uint8_t ksize_y = param.ksize_height;

    uint8_t stride_log2_x = stride_log2;
    uint8_t stride_log2_y = stride_log2;
    uint8_t nifms         = param.ifm_sv_depth;
    uint8_t ifm_x         = param.ifm_sv_width;

    int calc_edge_output  = 0;
    
    if(param.ifm_sv_depth == 4)//Currently only 11x11s4 lyr-1 is supported with this if-condition.TODO bring 7x7s2 lyr-1 under this. 
    {       
        nifms         = 8;
        ksize_x       = (ksize_x + 1)>>1;        
        ifm_x         = ifm_x >> 1;        
        stride_log2_x = stride_log2>>1;              
    } 
    {        
        if(param.conv_type==CONV2D_7x7S2_LYR1_EDGE) {
            calc_edge_output = 1;
            param.conv_type  = 3;            
        }
        
        //int value = (ksize>>4) & 0x0F;
            
        ksize_x = (param.conv_type==CONV2D_7x7S2_LYR1) ? 4     : ksize_x;
        ksize_y = ksize_y;
        //ksize_y = (param.conv_type==CONV2D_7x7S2_LYR1) ? ksize : ( (value==0) ? ksize : (ksize & 0x0F) );
        
        stride_log2_x = (param.conv_type==CONV2D_7x7S2_LYR1) ? 0 : stride_log2;                
    }
    
    uint8_t stride_x = 1 << stride_log2_x;
    uint8_t stride_y = 1 << stride_log2_y;       
 
   // Fixes related to layer param copy 
    param.psum_buff_offset      = (param.psum_buff_offset_scaled)*128*(1-param.out_mode);
    param.ofm_offset_left       = (param.ofm_offset_packed) & 0x0F; 
    uint8_t nofms               = param.ofm_sv_depth;
    
    uint32_t fc_mode = (param.conv_type==2 || param.conv_type==8) ;
    param.ifm_offset            = 0 ; // will be set in wrapper
    param.wts_offset            = 0 ;  // will be set in wrapper 
    param.out_sign              = 1 - (int) param.act; //Run time activation 0: Linear   1: ReLU 
    
    uint8_t param__batch_log2   = (batch_size==4)?2:((batch_size==2)?1:0);     
    uint8_t param__batch        = (int)batch_size;     
    uint8_t param__oyp          = ((param.ifm_sv_height - ksize_y) >> stride_log2_y) + 1;     
    uint8_t param__oxp          = ((ifm_x - ksize_x) >> stride_log2_x) + 1; 
    uint32_t val0 =  (0xFF & param__batch )
                   | (0xFF00 & (param__batch_log2 << 8))
                   | (0xFF0000 & (param__oyp << 16))
                   | (0xFF000000 & (param__oxp << 24));
    param.storage_batch_batch_log2_oyp_oxp = val0;   

    uint8_t param__ky           = ksize_y;     
    uint8_t param__kx           = ksize_x;     
    uint8_t param__stride       = 1 << stride_log2_y;    
    // param.curr_depth_iter       = 0; // initialize depth iter to 0
    uint32_t val1 =  (0xFF & param__ky )
                   | (0xFF00 & (param__kx << 8))
                   | (0xFF0000 & (param__stride << 16))
                   | (0xFF000000 & (0 << 24));
    param.storage_ky_kx_stride_curr_depth_iter = val1;

    int sv_out_width            = param.oxp + ((calc_edge_output<<4)>>param.batch_log2) ; 
    int align_kx                = max_bf16(1, 4 >> param.batch_log2);
    uint32_t align_kx_bits      = align_kx >> 1;
    int ixp                     = param__oxp * stride_x + ((param__kx-1+align_kx-1) >> align_kx_bits)*align_kx;
    int iyp                     = param.oyp * stride_y +   param.ky-1;
    int step_co                 = (sv_out_width + param.ofm_width_pad + param.ofm_offset_left) * (8 << param.batch_log2);
    int step_ci                 = ixp * (8 << param.batch_log2);
    int step_yo                 = step_co * nofms >> 3;
    int step_yi                 = step_ci * nifms >> 3;
    param.hdr_len               = (!aximm_rtp_enabled ? LAYER_PARAM_WORDS : 0) + nofms*BIAS_MULTIPLE;
    uint8_t N_g                 = param.batch;
    uint8_t X_g                 =  (fc_mode) ? 1 : (sv_out_width * param.batch * (param.batch < 4 ? stride_x : 1) >> (param.conv_type==12?3:4)) ;// excess width update at edge
    uint8_t Co_g                = (param.conv_type==CONV2D_LAYER1)? (nofms >> 3) : (nofms >> 4) ;

    param.num_Co                = Co_g-1;
    param.num_X                 = X_g-1;
    uint8_t num_Kx_adj          = (param.conv_type==CONV2D_LAYER1) ? param.conv_type:0;
    num_Kx                      = param.kx-1-(num_Kx_adj<<1);    
    num_Ky                      = param.ky-1;

    uint8_t Ci_g                = nifms >> 3;
    param.inner_g               = param.kx * param.ky * Ci_g;
    param.outer_g               = X_g  * param.oyp  * Co_g;
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
      int SN0_g                   = !(stride_x == 2 && N_g < 4);
      int SN1_g                   = 2 << SN0_g;
    #endif
    int chnd                    = 32;
    int chnd_T                  = 64;
    param.step_align            = (step_Kx & 15 ? 2 : (step_Kx & 31 ? 3 : 0));
    #ifdef ENABLE_STRIDE_4
      param.shfl_0              = (SN0_g > 1 ? T512_1x2_lo : (N_g == 1 ? T64_8x2_lo : T128_4x2_lo));
      param.shfl_1              = (SN0_g > 1 ? T512_1x2_hi : (N_g == 1 ? T64_8x2_lo : T128_4x2_lo));
      param.shfl_2              = (SN0_g > 0 ? T512_1x2_hi : (N_g == 1 ? T64_8x2_lo : T128_4x2_lo));
    #else
      param.shfl_0                = (SN0_g ? T512_1x2_lo : (N_g == 1 ? T64_8x2_lo : T128_4x2_lo));
      param.shfl_1                = (SN0_g ? T512_1x2_hi : (N_g == 1 ? T64_8x2_lo : T128_4x2_lo));
    #endif
    param.inc_A                 =  (fc_mode) ? 0 : (step_Xi * SN1_g);
    inc_Kx                      = -3 * step_Xi * SN1_g + step_Kx;
    inc_Ky                      = -3 * step_Xi * SN1_g - step_Kx * (param.kx-1) + step_Ky; // go to next lower row for procesing next row of filter kernel
    inc_Ci                      =  (fc_mode) ? 32 : (-3 * step_Xi * SN1_g - step_Kx * (param.kx-1) - step_Ky * (param.ky-1) + step_ci); // TODO: change for edge excess width when Ci>8
    inc_Xi                      = -step_ci * Ci_g + (param.conv_type==12?2:4) * step_Xi * SN1_g;
    inc_Co_rev                  = (fc_mode) ? -(32 * (nifms >> 3)): -step_ci * Ci_g - (param.conv_type==12?2:4) * step_Xi * SN1_g * (X_g-1);
    inc_Yi                      = -step_ci * Ci_g - (param.conv_type==12?2:4) * step_Xi * SN1_g * (X_g-1) + step_Yi;
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

static void update_offsets_bf16 (int8_t * params_ptr, conv2d_bf16_params_t &param, uint16_t core_col) {
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

#endif  //__CONV2D_BF16_PARAMS_H__
