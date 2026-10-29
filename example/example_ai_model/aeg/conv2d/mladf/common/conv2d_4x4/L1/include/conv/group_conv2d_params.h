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

#ifndef __GROUP_CONV2D_PARAMS_H__
#define __GROUP_CONV2D_PARAMS_H__


#include "layer_params.h"
#include "utils.h"
#include <adf.h>
#include "aie.hpp"

#ifdef __X86SIM__
#define GC_LOAD_V GC_load_v
#define PRINT_PTR(v, N, pre) print_ptr_impl(v, N, pre)
#define PRINT_VEC(v, pre) print_vec_impl(v, pre)
#define DEBUG_EXEC(...) __VA_ARGS__
#else
#define GC_LOAD_V aie::load_v
#define PRINT_PTR(v, N, pre)
#define PRINT_VEC(v, pre)
#define DEBUG_EXEC(...)
#endif

// Some constants used in the conv2d kernel code

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

#ifndef OFM_DM_BANK
#define OFM_DM_BANK
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
#define BIAS_MULTIPLE   2

struct alignas(v32uint16) group_conv2d_params_t {
    //Layer params of original conv2d
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
    // original conv2d params end

    uint8_t sv_groups;
    uint8_t ssv_groups;
    uint8_t ssv_group_off;
    uint8_t ofm_depth_gran;
    uint8_t sv_ifm_height;
    uint8_t outer_loop_cnt;
    int ifm_h_skip_sz;
    int ifm_g_skip_sz;
    int ofm_h_skip_sz;
    int ofm_c8_skip_sz;    // used for C8 out
    uint32_t psum_num_Co_init;  // when initializating psum using weights, how many times when we increase
    int ofm_iter_inc0;
    uint8_t run_time_enable_post_process;
    uint32_t ofm_depth_per_group;
    uint32_t post_process_loop_cnt;
    DECL_ITR3D(iterator_post_process_load)
    DECL_ITR3D(iterator_post_process_store)
};

static void setup_conv2d_params_for_gconv(int8_t * params_ptr, group_conv2d_params_t &param, uint8_t batch_size=1, uint8_t aximm_rtp_enabled = 0) {
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

    int32_t inc_Co_pp_load;
    uint32_t num_Co_pp_load;
    int32_t inc_X_pp_load;
    uint32_t num_X_pp_load;
    int32_t inc_Y_pp_load;
    int32_t inc_Co_pp_store;
    uint32_t num_Co_pp_store;
    int32_t inc_X_pp_store;
    uint32_t num_X_pp_store;
    int32_t inc_Y_pp_store;
    //Copy 32 bytes of layer params
    //get_layer_params((uint32_t *)params_ptr,(uint32_t *)&param);
    aie::vector<int32,8> pars = aie::load_v<8>((int32 *)(params_ptr));
    aie::store_v( (int32  __aie_dm_resource_a *) &param, pars);

    int32_t en2x8_mode = (int32_t) *(params_ptr+35) ;//EN_2x8; // 2x8 mode //TODO: Update vectors or cleanup based on 2x8 mode
    int32_t num_out_samp=16-8*en2x8_mode;// 2x8 mode
    int32_t num_out_samp_bits=4-en2x8_mode; // 2x8 mode

    uint8_t param__batch = (uint8_t) *(params_ptr+62);
    uint8_t sv_group = (uint8_t)*(params_ptr+61);
    uint8_t ssv_group = (uint8_t)*(params_ptr+60);
    uint8_t ofm_depth_gran = (uint8_t)*(params_ptr+59);
    uint8_t run_time_enable_post_process = (uint8_t)*(params_ptr+58);
    uint8_t ofm_depth_per_group = (uint8_t)*(params_ptr+57);

    uint8_t ksize         = param.ksize;
    uint8_t stride_bits   = param.stride_bits;

    uint8_t ksize_x       =  ksize;
    uint8_t ksize_y       =  ksize;
    uint8_t stride_bits_x = stride_bits;
    uint8_t stride_bits_y = stride_bits;
    #ifdef USE_NUM_CH_SCALE
    param.ifm             = (param.ifm_scaled)<<3;//Number of channels encoded with scaled factor of 8
    uint16_t nifms        = param.ifm;
    #else
    uint8_t nifms        = param.ifm;
    #endif
    uint8_t ifm_x         = param.ifm_xsize;

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
        calc_edge_output = (param.conv_type==CONV2D_7x7S2_LYR1_EDGE) ? 1                 : calc_edge_output;
        param.conv_type  = (param.conv_type==CONV2D_7x7S2_LYR1_EDGE) ? CONV2D_7x7S2_LYR1 : param.conv_type;

        int value = (ksize>>4) & 0x0F;

        ksize_x = (param.conv_type==CONV2D_7x7S2_LYR1) ? 4     : ( (value==0) ? ksize : ((ksize>>4)&0x0F) );
        ksize_y = (param.conv_type==CONV2D_7x7S2_LYR1) ? ksize : ( (value==0) ? ksize : (ksize & 0x0F) );

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
    // DEBUG_EXEC(printf("psum_buff_offset(%d)out_mode(%d)", param.psum_buff_offset, param.out_mode));
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

    param__batch                = (param__batch ==0) ? batch_size : param__batch;
    uint8_t param__batch_log2   = param__batch >> 1;
    uint8_t param__ogp          = sv_group;
    uint8_t param__oyp          = (param.ifm_ysize - ksize_y + 1) >> stride_bits_y; // this is the only place where ifm_ysize (i.e. rtp[1]) get used.
    param.outer_loop_cnt = param__oyp;
    uint8_t param__oxp          = (ifm_x - ksize_x + 1) >> stride_bits_x;
    param.batch                 = param__batch;
    param.batch_log2            = param__batch_log2;
    param.oyp                   = param__oyp;       // this variable seems also not to be used
    param.oxp                   = param__oxp;

    int sv_out_width            = param__oxp + ((calc_edge_output<<4)>>param__batch_log2) ;

    uint8_t param__ky              = ksize_y;
    uint8_t param__kx              = ksize_x;
    uint8_t param__stride          = 1 << stride_bits_y;
    param.ky                       = param__ky;
    param.kx                       = param__kx;
    param.stride                   = param__stride;
    param.curr_depth_iter          = 0;

    uint32_t align_kx           = 4 >> param__batch_log2;
    uint32_t align_kx_bits      = align_kx >> 1;
    int ixp                     = param__oxp * stride_x + ((param__kx-1+align_kx-1)>> align_kx_bits)*align_kx;
    int iyp                     = param__oyp * stride_y +   param__ky-1;    // this variable is not used?
    int step_co                 = (sv_out_width + param.ofm_width_pad + param.ofm_offset_left) * (8 << param__batch_log2);
    //DEBUG_EXEC(printf("step_co(%d)sv_out_width(%d)param.ofm_width_pat(%d)param.ofm_offset_left(%d)\n", step_co, sv_out_width, param.ofm_width_pad, param.ofm_offset_left));
    int step_ci                 = ixp * (8 << param__batch_log2);
    int step_yo                 = step_co * nofms >> 3;
    int step_yi                 = sv_group * step_ci * nifms >> 3;  // skip entire group into next line
    param.hdr_len               = (!aximm_rtp_enabled ? LAYER_PARAM_WORDS : 0) + ssv_group * nofms*BIAS_MULTIPLE;
    uint8_t N_g                 = param__batch;
    //uint8_t sh                  = fc_mode ? 2:4;
    uint8_t X_g                 =  (fc_mode) ? 1 : (sv_out_width * param__batch * (param__batch < 4 ? stride_x : 1) >> num_out_samp_bits) ;// excess width update at edge // 2x8 mode
    uint8_t Co_g                = (param.conv_type==CONV2D_LAYER1)? (nofms >> 3) : (nofms >> 4) ;

    param.num_Co                = Co_g-1;
    param.num_X                 = X_g-1;
    uint8_t num_Kx_adj          = (param.conv_type==CONV2D_LAYER1) ? param.conv_type:0;
    num_Kx                      = param__kx-1-(num_Kx_adj<<1);
    num_Ky                      = param__ky-1;
    //DEBUG_EXEC(printf("param__oyp(%d)param__oxp(%d)sv_out_width(%d)Co_g(%d)X_g(%d)\n", param__oyp, param__oxp, sv_out_width, Co_g, X_g));

    uint8_t Ci_g                = nifms >> 3;
    param.inner_g               = param__kx * param__ky * Ci_g;
    param.outer_g               = (fc_mode==1) ? 4: X_g  * (ssv_group>>en2x8_mode) * Co_g; //2x8 mode
    uint16_t step_Xo            = 8;
    uint16_t step_Yi            = (step_yi * stride_y)<<en2x8_mode; // TBD check for stride>1 for 2x8 mode
    param.ifm_h_skip_sz = step_Yi;
    param.ofm_h_skip_sz = step_yo * ssv_group;
    uint16_t step_Xi            = 8 * stride_x;
    uint16_t step_Ky            = step_yi;
    uint16_t step_Kx            = 8 << param__batch_log2;
    param.ofm_offset_top        = (param.ofm_offset_packed >>4)*step_yo;
    // printf("param.inner_g=%d \n", param.inner_g );
    // printf("param.outer_g=%d \n", param.outer_g );
    // printf("param.outer_loop_cnt=%d \n", param.outer_loop_cnt );

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
    param.inc_A_2x8_mode        =   (en2x8_mode==1) ? (-32+8*nifms): param.inc_A; // 2x8 mode jump with ht=1 for width of 8
    inc_Kx                      = -3 * step_Xi * SN1_g + step_Kx;
    inc_Ky                      = -3 * step_Xi * SN1_g - step_Kx * (param__kx-1) + step_Ky; // go to next lower row for procesing next row of filter kernel
    // DEBUG_EXEC(printf("param__batch_log2(%d)step_Xi(%d)SN1_g(%d)step_Kx(%d)param__kx(%d)step_Ky(%d)step_ci(%d)nifms(%d)sv_groups(%d)\n", param__batch_log2, step_Xi, SN1_g, step_Kx, param__kx, step_Ky, step_ci, nifms, sv_group));
    inc_Ci                      =  (fc_mode) ? 32 : (-3 * step_Xi * SN1_g*(1-en2x8_mode) - step_Kx * (param__kx-1) - step_Ky * (param__ky-1) + step_ci - (8*nifms+32)*en2x8_mode); // TODO: change for edge excess width when Ci>8
    inc_Xi                      = -step_ci * Ci_g + 4 * step_Xi * SN1_g; //TODO:update for en_2x8_mode
    inc_Co_rev                  = (fc_mode) ? -(32 * (nifms >> 3)): -step_ci * Ci_g - 4 * step_Xi * SN1_g * (X_g-1); //TODO:update for en_2x8_mode
    inc_Yi                      = -step_ci * Ci_g - 4 * step_Xi * SN1_g * (X_g-1) + (step_yi/sv_group); //ifm continues to next group for core's outer loop
    int pp_w                    = 4;
    uint8_t nofms_pp            = ofm_depth_gran == 16 ? nofms : nofms >> 1;
    inc_Co_pp_load              = sv_out_width << 3;
    num_Co_pp_load              = (ssv_group * nofms_pp >> 3) - 1;
    inc_X_pp_load               = -inc_Co_pp_load * num_Co_pp_load + (pp_w << 3);
    num_X_pp_load               = sv_out_width * param__batch / pp_w - 1;
    inc_Y_pp_load               = -(inc_Co_pp_load * num_Co_pp_load * (num_X_pp_load + 1)) - inc_X_pp_load * num_X_pp_load + sv_out_width * ssv_group * nofms_pp;
    inc_Co_pp_store             = inc_Co_pp_load;
    num_Co_pp_store             = (ssv_group * ofm_depth_per_group >> 3) - 1;
    inc_X_pp_store              = -inc_Co_pp_store * num_Co_pp_store + (pp_w << 3);
    num_X_pp_store              = sv_out_width * param__batch / pp_w - 1;
    inc_Y_pp_store              = -(inc_Co_pp_store * num_Co_pp_store * (num_X_pp_store + 1)) - inc_X_pp_store * num_X_pp_store + sv_out_width * ssv_group * ofm_depth_per_group;


    SETUP_ITR3D_PARAM(iterator_inner,inc_Kx,num_Kx,inc_Ky,num_Ky,inc_Ci,param)
    SETUP_ITR3D_PARAM(iterator_outer,inc_Xi,param.num_X,inc_Co_rev,param.num_Co,inc_Yi,param)
    SETUP_ITR3D_PARAM(iterator_post_process_load, inc_Co_pp_load, num_Co_pp_load, inc_X_pp_load, num_X_pp_load, inc_Y_pp_load, param)
    SETUP_ITR3D_PARAM(iterator_post_process_store, inc_Co_pp_store, num_Co_pp_store, inc_X_pp_store, num_X_pp_store, inc_Y_pp_store, param)
    // uint8_t Wo_pt_g = param__oxp >> (pp_w >> );
    param.post_process_loop_cnt = (num_Co_pp_load+1) * (num_X_pp_load+1) * param__oyp;
    // DEBUG_EXEC(printf("param.post_process_loop_cnt(%d)param__oyp(%d)inc_Kx(%d)num_Kx(%d)inc_Ky(%d)num_Ky(%d)inc_Ci(%d)inc_Xi(%d)param.num_X(%d)inc_Co_rev(%d)param.num_Co(%d)inc_Yi(%d)\n", param.post_process_loop_cnt, param__oyp, inc_Kx, num_Kx, inc_Ky, num_Ky, inc_Ci, inc_Xi, param.num_X, inc_Co_rev, param.num_Co, inc_Yi));
    // DEBUG_EXEC(printf("inc_Co_pp_load(%d)num_Co_pp_load(%d)inc_X_pp_load(%d)num_X_pp_load(%d)inc_Y_pp_load(%d)\n", inc_Co_pp_load, num_Co_pp_load, inc_X_pp_load, num_X_pp_load, inc_Y_pp_load));
    // DEBUG_EXEC(printf("inc_Co_pp_store(%d)num_Co_pp_store(%d)inc_X_pp_store(%d)num_X_pp_store(%d)inc_Y_pp_store(%d)\n", inc_Co_pp_store, num_Co_pp_store, inc_X_pp_store, num_X_pp_store, inc_Y_pp_store));
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

      param.inc_ST_0_2x8 = (en2x8_mode==1) ? (2*8*nofms): param.inc_ST_0; // 2x8 mode jump with ht=1 for width of 8
      param.inc_ST_1_2x8 = (en2x8_mode==1)?( -2*(nofms*8)+2*step_co ) : param.inc_ST_1; // 2x8 mode

      param.inc_S_0     =  inc_S_0_base * chess_copy(SN0_g);
      param.inc_S_1     = (fc_mode) ? -32 :-inc_S_0_base * chess_copy(SN0_g) + step_co * bits_O/8;

      param.inc_S_0_2x8  = (en2x8_mode==1)?(-32+nofms*8) : (param.inc_S_0-chnd); // 2x8 mode
      param.inc_S_1_2x8  = (en2x8_mode==1)?( -(nofms*8+32)+step_co ) : (param.inc_S_1-chnd); // 2x8 mode

      param.inc_Xo      =  inc_S_0_base - step_co * bits_O/8;
      //param.inc_Co    =  inc_S_0_base;
      param.inc_Co      =  inc_S_0_base + (param.ofm_width_pad+param.ofm_offset_left)*8;
      param.inc_Xo_T    =  inc_S_0_base - step_co * (2*bits_T/8-bits_T/8) + chnd_T; //(1,2) -> 3; (1,4) -> 7; (2,4) -> 6
      param.inc_Co_T    =  inc_ST_0_base;
      param.iter_2x8_inc =  (en2x8_mode==1)?(-nofms*8+chnd):(param.inc_Xo-chnd); // 2x8 mode for 1x1 kernel step back height to next output channel location
      param.iter_2x8_inc_T =  (en2x8_mode==1)?(-2*nofms*8+chnd_T):(param.inc_Xo_T-chnd_T); // 2x8 mode for 1x1 kernel step back height to next output channel location
      param.iter_2x8_wrap =  (en2x8_mode==1)?param.num_Co:param.num_X ;
    #endif

    param.inc_acc_0 = 0;
    param.inc_acc_1 = BIAS_BYTES*DIM_M*DIM_K;
    param.inc_acc_2 = -BIAS_BYTES*DIM_M*DIM_K;
    param.inc_acc_d0 = 0;
    param.inc_acc_d1 =  param.inc_acc_1*OFM_SETS;
    param.inc_acc_rev = (1-Co_g)*param.inc_acc_d1;

    inc_w_Ci_rev = -param.inner_g * 128;
    inc_w_Co_rev =  inc_w_Ci_rev * Co_g * ssv_group;

    SETUP_ITR3D_PARAM(iterator_weights,inc_w_Ci_rev,param.num_X,0,param.num_Co * ssv_group + ssv_group - 1,inc_w_Co_rev,param)
    // DEBUG_EXEC(printf("inc_w_Ci_rev(%d)inc2num(%d)inc_w_Co_rev(%d)\n", inc_w_Ci_rev, param.num_Co * sv_group + sv_group - 1, inc_w_Co_rev));
    param.ofm_depth_gran = ofm_depth_gran;
    param.psum_num_Co_init = Co_g * ssv_group - 1;
    param.ofm_c8_skip_sz = ofm_depth_gran == 16 ? param.inc_S_1_2x8 : -64 - param.inc_S_0_2x8;
    param.ofm_iter_inc0 = ofm_depth_gran == 16 ? param.iter_2x8_inc : 32;   // in C8 output case the weight is actually duplicated in adjacent C8
    param.ifm_g_skip_sz = step_ci * nifms >> 3;
    param.ssv_groups = ssv_group;
    param.run_time_enable_post_process = run_time_enable_post_process;
    param.ofm_depth_per_group = ofm_depth_per_group;
    // DEBUG_EXEC(printf("param.inc_S_0_2x8(%d)param.inc_S_1_2x8(%d)param.iter_2x8_inc(%d)ofm_c8_skip_sz(%d)ofm_iter_inc0(%d)ifm_g_skip_sz(%d)\n", param.inc_S_0_2x8, param.inc_S_1_2x8, param.iter_2x8_inc, param.ofm_c8_skip_sz, param.ofm_iter_inc0, param.ifm_g_skip_sz));
}

static void setup_gconv2d_iter_params(group_conv2d_params_t &param) {
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

     //printf("exec_type=%d\n",param.exec_type);
     if(param.curr_depth_iter==param.num_ifm_depth_iter){
       param.curr_depth_iter=0; // Reset depth_iter
    }
}

static void setup_group_conv2d_params(int8_t *params_ptr, group_conv2d_params_t &param, uint8_t aximm_rtp_enabled = 0) {
    param.sv_groups = params_ptr[61];
    param.sv_ifm_height = params_ptr[1];
    // since the channel is divided into groups, the core shall view only the grouped channels
    // note the passed channel are alloady divided by groups, so we need not to divide again
    //params_ptr_cpy[2] /= param.sv_groups;
    //params_ptr_cpy[3] /= param.sv_groups;
    setup_conv2d_params_for_gconv(params_ptr, param, 1, aximm_rtp_enabled);
}

#endif //__GROUP_CONV2D_PARAMS_H__