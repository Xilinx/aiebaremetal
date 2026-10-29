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

#ifndef __CONV2D_TRANSPOSE_BF16_PARAMS_H__
#define __CONV2D_TRANSPOSE_BF16_PARAMS_H__

#include "aie.hpp"
#include "layer_params.h"
#include "utils.h"
#include <adf.h>

// Buffer placement directives for IFM, WTs and OFM buffers.
#ifndef IFM_DM_BANK
#define IFM_DM_BANK __aie_dm_resource_a
#endif

#ifndef WT_DM_BANK
#define WT_DM_BANK __aie_dm_resource_a
#endif

#ifndef OFM_DM_BANK
#define OFM_DM_BANK __aie_dm_resource_b
#endif

// Bias is 16-bit
#define BIAS_BYTES 2
#define OFM_SETS 2

// number of bytes set aside at the start of weight buffer for conv kernel
// parameters
#define LAYER_PARAM_BYTES 64
#ifndef LAYER_PARAM_WORDS
#define LAYER_PARAM_WORDS (LAYER_PARAM_BYTES >> 2)
#endif
#define ITER_PARAM_BYTES 32
#ifndef LAYER_PARAM_WORDS
#define ITER_PARAM_WORDS (ITER_PARAM_BYTES >> 2)
#endif
#define ITER_PARAM_OFFSET 19
#define BIAS_MULTIPLE 2
#define WTS_LEN_OFFSET 50

#if defined(AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16) && (AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16 == 0)
  #undef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
#endif

 // ===============================================================
 // bfloat16 version
 // ===============================================================

struct alignas(v32uint16) conv2d_transpose_bf16_internal_params {
  // Layer params
  uint8_t ifm_xsize;
  uint8_t ifm_ysize;
  uint8_t num_ifm_ch;
  uint8_t num_ofm_ch;
  uint8_t ksize;
  uint8_t stride_bits;
  int8_t shift_out;
  uint8_t out_mode;
  int8_t shift_bias_init;
  int8_t shift_psum_in;
  int8_t shift_psum_out;
  uint8_t psum_buff_offset_scaled;
  int16_t lrelu_alpha;
  int8_t shift_lrelu_alpha;
  int8_t shift_lrelu_input;
  int8_t shift_lrelu_out;
  uint8_t num_chout_iter;
  uint8_t num_ifm_depth_iter;
  uint8_t op_mode;
  uint16_t ofm_len;
  uint8_t num_height_iter;
  uint8_t num_width_iter;
  uint16_t ifm_len;
  uint8_t ofm_width_pad;
  int8_t run_time_act;
  uint8_t conv_type;
  uint8_t row_offset;
  uint8_t in0_sign;
  uint8_t ofm_offset_packed;
  int ifm_sign; //  IFM sign upper bits

  // Derived params
  int32_t out_sign;
  int32_t psum_buff_offset;
  uint32_t ofm_offset_left;
  int32_t psum_in;
  int32_t psum_out;
  int hdr_len; // larger than 8 bit so retaining
  int32_t batch;
  int32_t batch_log2;
  // uint8_t oyp;
  // uint8_t oxp;
  // uint8_t Ky;
  // uint8_t Kx;
  uint32_t ofm_xsize;
  uint32_t ofm_ysize;
  uint32_t stride;
  uint32_t curr_depth_iter;
  uint32_t io_mode;
  uint32_t
      ofm_offset_top; // number rows of pixels to be left out for padding at top
  uint32_t
      exec_type; // This has conv2d_transpose generic, layer-1, maxpool2d etc
  uint32_t inner_g;
  uint32_t outer_g;
  int ifm_offset; // Starting read pointer of IFM
  int wts_offset; // starting read pointer of WTS
  uint32_t stride_condition;

  // for bf16
  DECL_ITR3D(iterator_ifm_inner)
  DECL_ITR3D(iterator_wts_inner)
  DECL_ITR3D(iterator_wts_outer)
  DECL_ITR3D(iterator_psum_inner)
  DECL_ITR3D(iterator_psum_outer)
  DECL_ITR3D(iterator_bias)
  DECL_ITR3D(iterator_psum_in_inner)
  DECL_ITR3D(iterator_psum_in_outer)

  // for bf16
  int32_t psum_inc0;
  int32_t psum_inc1;
  int32_t pout_inc2;
  //int32_t inc_A;

  // int step_align;
  // int shfl_0;
  // int shfl_1;
  // int shfl_2;

  // uint32_t num_Y;
  // uint32_t num_X_Co;
  // uint32_t num_Co;
  // uint32_t num_X;
  // uint32_t num_Ci;
  // uint32_t num_Ky;//
  // uint32_t num_Kx;//
};

typedef struct conv2d_transpose_bf16_internal_params conv2d_transpose_bf16_params_t;


static void setup_conv2d_transpose_bf16_iter_params(conv2d_transpose_bf16_params_t &param) {
  param.curr_depth_iter++;
  param.io_mode = (param.num_ifm_depth_iter > 1) *
                  ((param.curr_depth_iter == 1) +
                   2 * (param.curr_depth_iter == param.num_ifm_depth_iter) +
                   3 * ((param.curr_depth_iter > 1) &&
                        (param.curr_depth_iter < param.num_ifm_depth_iter)));

  if (param.curr_depth_iter < param.num_ifm_depth_iter) {
    // out mode parital
    param.psum_out = 1;
    param.exec_type = param.conv_type + 2;
  } else {
    // out mode full
    param.psum_out = 0;
    param.exec_type = param.conv_type;
  }

  // bias config
  if (param.curr_depth_iter == 1) {
    param.psum_in = 0;
  } else {
    param.psum_in = 1;
  }

  if (param.curr_depth_iter == param.num_ifm_depth_iter) {
    param.curr_depth_iter = 0; // Reset depth_iter
  }

  int align_x =
      (param.ofm_xsize + 15) / 16 * 16 * param.batch; // align_x for ofm

#ifdef __X86SIM__
  printf("\n[iter params] exec_type=%d, psum_in=%d, psum_out=%d, "
         "current_iter=%d, num_iter=%d\n",
         param.exec_type, param.psum_in, param.psum_out, param.curr_depth_iter,
         param.num_ifm_depth_iter);
  printf("align_ofm_x=%d\n", align_x);
#endif

}


static void setup_conv2d_transpose_bf16_params(int8_t *params_ptr,
                                   conv2d_transpose_bf16_params_t &param,
                                   uint8_t batch_size = 1,
                                   uint8_t aximm_rtp_enabled = 0) {

  // Copy 32 bytes of layer params
  // get_layer_params((uint32_t *)params_ptr,(uint32_t *)&param);
  aie::vector<int32, 8> pars = aie::load_v<8>((int32 *)(params_ptr));
  aie::store_v((int32 __aie_dm_resource_a *)&param, pars);
  params_ptr += 62;
  param.batch = (uint8_t)*params_ptr++;

  int ksize = param.ksize;
  int stride_bits_x = param.stride_bits;
  int stride_bits_y = param.stride_bits;
  int nifms = param.num_ifm_ch;
  int ifm_x_gran = 4;
  int ofm_x_gran = 4;
  int ifm_x_pad = (param.ifm_xsize+(ifm_x_gran-1))/ifm_x_gran *ifm_x_gran;

  int calc_edge_output = 0;

  // get ksize
  int value = (ksize >> 4) & 0x0F;
  int ksize_x = ((value == 0) ? ksize : ((ksize >> 4) & 0x0F));
  int ksize_y = ((value == 0) ? ksize : (ksize & 0x0F));

  // Extract IFM sign packed with ifm2_sign (called in0_sign)
  int tmp = param.in0_sign; // offset 30 input0 sign for add2d
  param.ifm_sign =
      (int)((tmp >> 4) & 0x01);  // offset 30 input sign for conv2d_transpose
  param.in0_sign = (tmp & 0x01); // offset 30 input0 sign for add2d

  // Fixes related to layer param copy
  param.psum_buff_offset = (param.psum_buff_offset_scaled) * 128;//diff from conv2d "(1 - param.out_mode)"
  param.ofm_offset_left = (param.ofm_offset_packed) & 0x0F;
  int nofms = param.num_ofm_ch;

  param.ifm_offset = 0; // will be set in wrapper
  param.wts_offset = 0; // will be set in wrapper
  param.out_sign =
      1 - (int)param.run_time_act; // Run time activation 0: Linear   1: ReLU

  param.curr_depth_iter = 0; // initialize depth iter to 0
  param.batch = (param.batch == 0) ? batch_size : param.batch;
  param.batch_log2 = (param.batch == 4) ? 2 : ((param.batch == 2) ? 1 : 0);

  // ofm size //L2 will deal with the padding jobs
  param.stride = 1 << stride_bits_x;
  param.ofm_xsize = (param.ifm_xsize - 1) * param.stride + ksize_x;
  param.ofm_ysize = (param.ifm_ysize - 1) * param.stride + ksize_y;
  int ofm_x_pad = ((param.ofm_xsize + (ofm_x_gran-1)) / ofm_x_gran) * ofm_x_gran;

#ifdef __X86SIM__
  printf("[check para] ofm_xsize:%d, ofm_x_pad:%d\n", param.ofm_xsize, ofm_x_pad);
#endif

  //define by kernel
  int Ci_gran = 8;
  int Co_gran = 16;//Psum_st_gran = 4, Co_st_gran = 8
  // int Xi_gran = 4;//org 16
  // int Xo_gran = param.stride * Xi_gran;//org 16

  param.hdr_len =
      (int)(!aximm_rtp_enabled ? LAYER_PARAM_BYTES/sizeof(bfloat16) : 0) + nofms * 4 * 1;//
  
  int N_g = param.batch;
  int X_g = ((param.ifm_xsize * param.batch) + 3) >> 2; // excess width update at edge
  int Ci_g = nifms >> 3;//Ci_gran=8
  int Co_g = (nofms >> 4);//Co_gran=16

#ifdef __X86SIM__
  printf("Ci_g=%d,Co_g=%d,ky=%u,kx=%u, X_g=%u,\n",Ci_g, Co_g, ksize_y, ksize_x, X_g);
#endif
  // param.inner_g = ksize_y * ksize_x * Ci_g;
  // param.outer_g = X_g * param.ifm_ysize * Co_g;
  param.inner_g = X_g * param.ifm_ysize * Ci_g;
  param.outer_g = ksize_y * ksize_x * Co_g;

#ifdef __X86SIM__
  printf("ofm_depth=%u, ofm_ysize=%u, ofm_xsize=%u\n", nofms, param.ofm_ysize, param.ofm_xsize);
  printf("ifm_depth=%u, ifm_ysize=%u, ifm_xsize=%u\n", nifms, param.ifm_ysize, param.ifm_xsize);
#endif

// ifm and wts is the same in aie2 and 2p/2ps
  // =================================================================================
  // iterator_ifm
  int incA1            = 0; //sequential 
  int numA1            = (ifm_x_pad >> 2) - 1; 
  int incA2            = -32 * 2 * (1 + numA1) + ifm_x_pad*nifms*2;//next h // (Ci_g-1) * X_g *32*sizeof(bfloat16);//
  int numA2            = (param.ifm_ysize >> 0) - 1; 
  int incA3            = -32 * 2 * (1+numA1) - numA2*ifm_x_pad*nifms*2 + ifm_x_pad* 8*2;
  //int incA3            = (-X_g - numA2*Ci_g * X_g  + X_g)*32*sizeof(bfloat16);// back to next ci
  SETUP_ITR3D_PARAM(iterator_ifm_inner, incA1, numA1, incA2, numA2, incA3, param)
#ifdef __X86SIM__
  printf("iterator_ifm_inner incA1=%d, numA1=%d, incA2=%d, numA2=%d, incA3=%d\n",incA1, numA1, incA2, numA2, incA3);
#endif

  // =================================================================================
  // iterator_weights
  int incW1            = -4* 32*2; // push back (8ich * 16och) //-2* 64*2 same with aie2p/2ps
  int numW1            = X_g * param.ifm_ysize - 1;//after w h
  int incW2            = -4* 32*2 + ((ksize_x) * (ksize_y)  * Co_gran * Ci_gran) * sizeof(bfloat16); //for Ci
  int numW2            = Ci_g - 1;
  int incW3            = -numW2 * ((ksize_x) * (ksize_y)  * Co_gran * Ci_gran) * sizeof(bfloat16);// back
  SETUP_ITR3D_PARAM(iterator_wts_inner, incW1, numW1, incW2, numW2, incW3, param)
#ifdef __X86SIM__
  printf("iterator_wts_inner incW1=%d, numW1=%d, incW2=%d, numW2=%d, incW3=%d\n",incW1, numW1, incW2, numW2, incW3);
#endif

  int incW4            = 0;
  int numW4            = (ksize_x) * (ksize_y)-1;
  int incW5            = numW2 * ((ksize_x) * (ksize_y)  * Co_gran * Ci_gran) * sizeof(bfloat16); //for Co
  int numW5            = Co_g - 1;
  int incW6            = 0;
  SETUP_ITR3D_PARAM(iterator_wts_outer, incW4, numW4, incW5, numW5, incW6, param)
#ifdef __X86SIM__
  printf("iterator_wts_outer incW4=%d, numW4=%d, incW5=%d, numW5=%d, incW6=%d\n",incW4, numW4, incW5, numW5, incW6);
#endif


  // =================================================================================
  // ifm(p_in) and ofm(p_out) BW is NOT the same in aie2 and 2p/2ps
  // #if __AIE_ARCH__ == 20
  //   param.inc_A = 32;//256 bit aligned 16 * sizeof(bf16)
  // #else // aie2p/aie2ps
  //   param.inc_A = 32*2;//512 bit aligned 32 * sizeof(bf16)
  // #endif// __AIE_ARCH__ == 20 end

  // =================================================================================
  // psum inner loop adjust p_out and p_init
  // (b, h , oc/och16, w_pad/w4, och16/wts_col, w4, wts_col=4)
  int psum_w1c8 = 8* sizeof(float); // w1c8 // sizeof(float)*w1*Co8=4*1*8 
  param.psum_inc0 = param.stride * 8* sizeof(float);  
  param.psum_inc1 = param.psum_inc0 * (-3) + psum_w1c8 * ofm_x_pad; //to write Co8-15(w4) float

  int psum_line = sizeof(float) * ofm_x_pad * nofms;// one line(h)

  // iterator_psum
  // offset is in byte in the add3D_byte API
  // each W4, we need to store Psum to/back to 0~7och and 8~16 och, each has psum_w1c8 * ofm_x_pad bytes 
  int incP1            = param.psum_inc0 * (1) - psum_w1c8 * ofm_x_pad; //next w4
  int numP1            = (ifm_x_pad >> 2) - 1; 
  int incP2            = param.psum_inc0 * (-numP1*4 - 3) - psum_w1c8 * ofm_x_pad + param.stride * psum_line;//next h
  int numP2            = (param.ifm_ysize >> 0) - 1;  
  int incP3            = param.psum_inc0 * (-numP1*4 - 3) - psum_w1c8 * ofm_x_pad - param.stride * psum_line * numP2;//back to zero
  SETUP_ITR3D_PARAM(iterator_psum_inner, incP1, numP1, incP2, numP2, incP3, param)
#ifdef __X86SIM__
  printf("inner_psum incP1=%d, numP1=%d, incP2=%d, numP2=%d, incP3=%d | while psum_line=%d\n",incP1, numP1, incP2, numP2, incP3, psum_line);
#endif

 
  int next_oc16 = ofm_x_pad * 16 * sizeof(float);
  // psum outerloop adjust p_init32
  int incP4            = 8 * sizeof(float); //8 = 1 * psum_gran // min align 256bits start from W1
  int numP4            = ksize_x - 1; 
  int incP5            = incP4 * (-numP4) + psum_line;//next h1w1
  int numP5            = ksize_y - 1;
  int incP6            = -incP4 * numP4 - psum_line * numP5 + next_oc16;//next co/16 h0w0
  SETUP_ITR3D_PARAM(iterator_psum_outer, incP4, numP4, incP5, numP5, incP6, param)
#ifdef __X86SIM__
  printf("outer_psum incP4=%d, numP4=%d, incP5=%d, numP5=%d, incP6=%d\n",incP4, numP4, incP5, numP5, incP6);
#endif

  // =================================================================================
  // p_psum_in same with the p_psum_out
  SETUP_ITR3D_PARAM(iterator_psum_in_inner, incP1, numP1, incP2, numP2, incP3, param)
  SETUP_ITR3D_PARAM(iterator_psum_in_outer, incP4, numP4, incP5, numP5, incP6, param)
  
  // =================================================================================
  // bias layout is {co0~7} dup 4, {co8~15} dup 4, ... same with Conv2d bf16
  param.pout_inc2 = (ofm_x_pad/4) * (nofms /8) * param.ofm_ysize;
  // iterator_bias
  int incB1            = 0;//-3 * 8*sizeof(bfloat16); //back 
  int numB1            = (ofm_x_pad/4) - 1; 
  int incB2            = 4*8*sizeof(bfloat16);//next co/8 : 4 copy* c8
  int numB2            = (nofms /8) - 1;  
  int incB3            = -numB2*(4)*8*sizeof(bfloat16);//back to zero
  SETUP_ITR3D_PARAM(iterator_bias, incB1, numB1, incB2, numB2, incB3, param)
#ifdef __X86SIM__
  printf("iterator_bias incB1=%d, numB1=%d, incB2=%d, numB2=%d, incB3=%d\n",incB1, numB1, incB2, numB2, incB3);
#endif

// =================================================================================


#ifdef __X86SIM__
printf("ifm_xsize: %u\n", param.ifm_xsize);
printf("ifm_x_pad: %u\n", ifm_x_pad);
printf("ifm_ysize: %u\n", param.ifm_ysize);
printf("num_ifm_ch: %u\n", param.num_ifm_ch);
printf("num_ofm_ch: %u\n", param.num_ofm_ch);
printf("ksize: %u\n", param.ksize);
printf("stride_bits: %u\n", param.stride_bits);
printf("shift_out: %d\n", param.shift_out);
printf("out_mode: %u\n", param.out_mode);
printf("shift_bias_init: %d\n", param.shift_bias_init);
printf("shift_psum_in: %d\n", param.shift_psum_in);
printf("shift_psum_out: %d\n", param.shift_psum_out);
printf("psum_buff_offset_scaled: %u\n", param.psum_buff_offset_scaled);
printf("lrelu_alpha: %d\n", param.lrelu_alpha);
printf("shift_lrelu_alpha: %d\n", param.shift_lrelu_alpha);
printf("shift_lrelu_input: %d\n", param.shift_lrelu_input);
printf("shift_lrelu_out: %d\n", param.shift_lrelu_out);
printf("num_chout_iter: %u\n", param.num_chout_iter);
printf("num_ifm_depth_iter: %u\n", param.num_ifm_depth_iter);
printf("op_mode: %u\n", param.op_mode);
printf("ofm_len: %u\n", param.ofm_len);
printf("num_height_iter: %u\n", param.num_height_iter);
printf("num_width_iter: %u\n", param.num_width_iter);
printf("ifm_len: %u\n", param.ifm_len);
printf("ofm_width_pad: %u\n", param.ofm_width_pad);
printf("run_time_act: %d\n", param.run_time_act);
printf("conv_type: %u\n", param.conv_type);
printf("row_offset: %u\n", param.row_offset);
printf("in0_sign: %u\n", param.in0_sign);
printf("ofm_offset_packed: %u\n", param.ofm_offset_packed);
printf("ifm_sign: %d\n", param.ifm_sign);
printf("out_sign: %d\n", param.out_sign);
printf("psum_buff_offset: %d\n", param.psum_buff_offset);
printf("ofm_offset_left: %u\n", param.ofm_offset_left);
printf("psum_in: %d\n", param.psum_in);
printf("psum_out: %d\n", param.psum_out);
printf("hdr_len: %d\n", param.hdr_len);
printf("batch: %d\n", param.batch);
printf("batch_log2: %d\n", param.batch_log2);
printf("stride: %u\n", param.stride);
printf("curr_depth_iter: %u\n", param.curr_depth_iter);
printf("io_mode: %u\n", param.io_mode);
printf("ofm_offset_top: %u\n", param.ofm_offset_top);
printf("exec_type: %u\n", param.exec_type);
printf("inner_g: %u\n", param.inner_g);
printf("Ci_g: %u\n", Ci_g);
printf("Kx_g: %u\n", ksize_x);
printf("Ky_g: %u\n", ksize_y);
printf("outer_g: %u\n", param.outer_g);
printf("X_g: %u\n", X_g);
printf("Co_g: %u\n", Co_g);
printf("param.ifm_ysize: %u\n", param.ifm_ysize);
printf("ifm_offset: %d\n", param.ifm_offset);
printf("wts_offset: %d\n", param.wts_offset);
printf("psum_inc0: %d\n", param.psum_inc0);
printf("psum_inc1: %d\n", param.psum_inc1);
printf("pout_inc2: %d\n", param.pout_inc2);
#endif
}
//#endif// AIE2 bf16

#endif //__CONV2D_TRANSPOSE_BF16_PARAMS_H__
