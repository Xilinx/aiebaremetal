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

#ifndef __CONV2D_TRANSPOSE_PARAMS_H__
#define __CONV2D_TRANSPOSE_PARAMS_H__

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

struct alignas(v32uint16) conv2d_transpose_internal_params {
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
  uint8_t ofm_offset_left;
  int32_t psum_in;
  int32_t psum_out;
  int hdr_len; // larger than 8 bit so retaining
  int8_t batch;
  int8_t batch_log2;
  uint8_t oyp;
  uint8_t oxp;
  uint8_t Ky;
  uint8_t Kx;
  uint8_t ofm_xsize;
  uint8_t ofm_ysize;
  uint8_t stride;
  uint8_t curr_depth_iter;
  uint8_t io_mode;
  uint32_t
      ofm_offset_top; // number rows of pixels to be left out for padding at top
  uint8_t
      exec_type; // This has conv2d_transpose generic, layer-1, maxpool2d etc
  uint32_t inner_g;
  uint32_t outer_g;
  int ifm_offset; // Starting read pointer of IFM
  int wts_offset; // starting read pointer of WTS
  uint8_t stride_condition;

  DECL_ITR3D(iterator_inner0)
  DECL_ITR3D(iterator_inner1)
  DECL_ITR3D(iterator_outer0)
  DECL_ITR2D(iterator_outer1)
  DECL_ITR3D(iterator_weights)
  DECL_ITR3D(iterator_pinit)
  DECL_ITR3D(iterator_pout)

  uint8_t shuffle[4];
  int32_t inc_S_0;
  int32_t inc_S_1;
  int32_t inc_S_2;

  int32_t inc_ST_0;
  int32_t inc_ST_1;
  int32_t inc_ST_2;

  int32_t inc_A_0;
  int32_t inc_A_1;

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

  uint32_t num_Y;
  uint32_t num_X_Co;
  uint32_t num_Co;
  uint32_t num_X;
  uint32_t num_Ci;
  uint32_t num_Ky;
  uint32_t num_Kx;
};

typedef struct conv2d_transpose_internal_params conv2d_transpose_params_t;


static void setup_conv2d_transpose_iter_params(conv2d_transpose_params_t &param) {
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
  printf("\nexec_type=%d, psum_in=%d, psum_out=%d, "
         "current_iter=%d, num_iter=%d\n",
         param.exec_type, param.psum_in, param.psum_out, param.curr_depth_iter,
         param.num_ifm_depth_iter);
  printf("align_ofm_x=%d\n", align_x);
#endif

  // initialize partial sum iterator
  param.inc_S_0 = (param.psum_in == 0) ? 0 : 64;
  param.inc_S_1 = (param.psum_in == 0) ? 64 : (align_x - 12) * 8 * 2;
  param.inc_S_2 = (param.psum_in == 0) ? 0 : (align_x + 12) * 8 * 2;

  int inc_W = (param.psum_in == 0) ? -64 : (-param.inc_S_2 + 16 * 8 * 2);
  int inc_Co = (param.psum_in == 0) ? -64 : (-param.inc_S_2 - 16 * 8 * 2 * param.num_X + 16 * align_x * 2);
  int inc_H = (param.psum_in == 0) ? -64 : (-param.inc_S_2 - 16 * 8 * 2 * param.num_X - param.num_Co * 16 * align_x * 2 + param.num_ofm_ch * align_x * 2);
  SETUP_ITR3D_PARAM(iterator_pinit, inc_W, param.num_X, inc_Co, param.num_Co,
                    inc_H, param)

#ifdef __X86SIM__
  printf("inner_pinit inc_W=%d,num_X=%d,inc_Co=%d,num_Co=%d,inc_H=%d\n", inc_W,
         param.num_X, inc_Co, param.num_Co, inc_H);
#endif

#if __AIE_ARCH__ == 20

  // initialize ofm iterator
  param.inc_ST_0 = (param.psum_out == 0) ? 32 : 64;
  param.inc_ST_1 = (param.psum_out == 0) ? (align_x - 12) * 8 : (align_x - 12) * 8 * 2;
  param.inc_ST_2 = (param.psum_out == 0) ? (align_x + 12) * 8 : (align_x + 12) * 8 * 2;

  inc_W = (param.psum_out == 0) ? (-param.inc_ST_2 + 16 * 8) : (-param.inc_ST_2 + 16 * 8 * 2);
  inc_Co = (param.psum_out == 0) ? (-param.inc_ST_2 - 16 * 8 * param.num_X + 16 * align_x) : (-param.inc_ST_2 - 16 * 8 * 2 * param.num_X + 16 * align_x * 2);
  inc_H = (param.psum_out == 0) ? (-param.inc_ST_2 - 16 * 8 * param.num_X - param.num_Co * 16 * align_x + param.num_ofm_ch * align_x)
              : (-param.inc_ST_2 - 16 * 8 * 2 * param.num_X - param.num_Co * 16 * align_x * 2 + param.num_ofm_ch * align_x * 2);

#elif __AIE_ARCH__ == 21

  // initialize ofm iterator
  param.inc_ST_0 = (param.psum_out == 0) ? 64 : 128;
  param.inc_ST_1 = (param.psum_out == 0) ? (align_x - 8) * 8 : (align_x - 8) * 8 * 2;
  param.inc_ST_2 = (param.psum_out == 0) ? (align_x + 8) * 8 : (align_x + 8) * 8 * 2;

  inc_W = (param.psum_out == 0) ?  (-param.inc_ST_2 + 16 * 8) : (-param.inc_ST_2 + 16 * 8 * 2);
  inc_Co =  (param.psum_out == 0) ? (-param.inc_ST_2 - 16 * 8 * param.num_X + 16 * align_x) : (-param.inc_ST_2 - 16 * 8 * 2 * param.num_X + 16 * align_x * 2);
  inc_H =  (param.psum_out == 0) ? (-param.inc_ST_2 - 16 * 8 * param.num_X - param.num_Co * 16 * align_x + param.num_ofm_ch * align_x) : 
      (-param.inc_ST_2 - 16 * 8 * 2 * param.num_X - param.num_Co * 16 * align_x * 2 + param.num_ofm_ch * align_x * 2);

#endif
  
  SETUP_ITR3D_PARAM(iterator_pout, inc_W, param.num_X, inc_Co, param.num_Co,
                    inc_H, param)

#ifdef __X86SIM__
  printf("outer_pout inc_W=%d,num_X=%d,inc_Co=%d,num_Co=%d,inc_H=%d\n", inc_W,
         param.num_X, inc_Co, param.num_Co, inc_H);
#endif
}

#if __AIE_ARCH__ == 20
static void setup_conv2d_transpose_params(int8_t *params_ptr,
                                   conv2d_transpose_params_t &param,
                                   uint8_t batch_size = 1,
                                   uint8_t aximm_rtp_enabled = 0) {

  // Copy 32 bytes of layer params
  // get_layer_params((uint32_t *)params_ptr,(uint32_t *)&param);
  aie::vector<int32, 8> pars = aie::load_v<8>((int32 *)(params_ptr));
  aie::store_v((int32 __aie_dm_resource_a *)&param, pars);
  params_ptr += 62;
  param.batch = (uint8_t)*params_ptr++;

  uint8_t ksize = param.ksize;
  uint8_t stride_bits = param.stride_bits;

  uint8_t ksize_x = ksize;
  uint8_t ksize_y = ksize;
  uint8_t stride_bits_x = stride_bits;
  uint8_t stride_bits_y = stride_bits;
  uint8_t nifms = param.num_ifm_ch;
  uint8_t ifm_x = param.ifm_xsize;

  int value = (ksize >> 4) & 0x0F;
  ksize_x = ((value == 0) ? ksize : ((ksize >> 4) & 0x0F));
  ksize_y = ((value == 0) ? ksize : (ksize & 0x0F));

  uint8_t stride_x = 1 << stride_bits_x;
  uint8_t stride_y = 1 << stride_bits_y;

  // Extract IFM sign packed with ifm2_sign (called in0_sign)
  uint8_t tmp = param.in0_sign; // offset 30 input0 sign for add2d
  param.ifm_sign =
      (int)((tmp >> 4) & 0x01);  // offset 30 input sign for conv2d_transpose
  param.in0_sign = (tmp & 0x01); // offset 30 input0 sign for add2d

  // Fixes related to layer param copy
  param.psum_buff_offset = param.psum_buff_offset_scaled * 128;
  param.ofm_offset_left = (param.ofm_offset_packed) & 0x0F;
  uint8_t nofms = param.num_ofm_ch;

  param.out_sign =
      1 - (int)param.run_time_act; // Run time activation 0: Linear   1: ReLU
  param.curr_depth_iter = 0;       // initialize depth iter to 0
  param.batch = (param.batch == 0) ? batch_size : param.batch;
  param.batch_log2 = (param.batch == 4) ? 2 : ((param.batch == 2) ? 1 : 0);
  param.oyp = (param.ifm_ysize - ksize_y + 1) >> stride_bits_y;
  param.oxp = (ifm_x - ksize_x + 1) >> stride_bits_x;

  param.Ky = ksize_y;
  param.Kx = ksize_x;
  param.stride = 1 << stride_bits_y;
  param.stride_condition = param.stride - 1;
  param.hdr_len = (!aximm_rtp_enabled ? LAYER_PARAM_BYTES : 0) + nofms * 4 * 2;

  param.ofm_xsize = (param.ifm_xsize - 1) * stride_x + ksize_x;
  param.ofm_ysize = (param.ifm_ysize - 1) * stride_y + ksize_y;

  uint8_t Kx_g = ((param.Kx + param.stride - 1) >> stride_bits_x)
                 << stride_bits_x;
  uint8_t Ky_g = ((param.Ky + param.stride - 1) >> stride_bits_y)
                 << stride_bits_y;
  uint8_t X_g = ((param.ofm_xsize * param.batch) + 15) >> 4; // X align with 16
  uint8_t Ci_g = nifms >> 3;
  uint8_t Co_g = (nofms >> 4);

  param.num_Y = param.ofm_ysize - 1;
  param.num_X_Co = X_g * Co_g - 1;
  param.num_Co = Co_g - 1;
  param.num_X = X_g - 1;
  param.num_Ci = Ci_g - 1;
  param.num_Kx = Kx_g - 1;
  param.num_Ky = Ky_g - 1;

#ifdef __X86SIM__
  printf("Ci_g=%d,Co_g=%d,Ky_g=%u,Kx_g=%u\n", Ci_g, Co_g, Ky_g, Kx_g);
#endif
  param.inner_g = Ci_g * Kx_g * Ky_g;
  param.outer_g = X_g * Co_g * param.ofm_ysize;

  uint16_t step_Kx = 8 << param.batch_log2;
  param.step_align = (step_Kx & 15 ? 2 : (step_Kx & 31 ? 3 : 0));
  int align_x = (param.ifm_xsize + 2 * (ksize_x - 1) / stride_x + 15) / 16 *
                16 * param.batch; // align_x for ifm
  param.ifm_offset =
      (nifms - 8) * align_x; // for inverse load of ifm in loop Cin
  param.wts_offset = 16 * nifms * Kx_g * Ky_g -
                     128; // for inverse load of wts for loop Ky and Kx
#ifdef __X86SIM__
  printf("align_ifm_x=%d\n", align_x);
#endif

  // ifm shuffle
  if (param.stride == 1) {
    param.shuffle[0] = 22;
    param.shuffle[1] = 22;
    param.shuffle[2] = 22;
    param.shuffle[3] = 22;
  } else if (param.stride == 2) {
    param.shuffle[0] = 14;
    param.shuffle[1] = 22;
    param.shuffle[2] = 14;
    param.shuffle[3] = 22;
  } else { // stride 4
    param.shuffle[0] = 14;
    param.shuffle[1] = 14;
    param.shuffle[2] = 14;
    param.shuffle[3] = 14;
  }

  // initialize ifm iterator
  // stride->Kx->Stride->Ky->off
  int inc_stride_Kx;
  int num_stride_Kx;
  int inc_Kx;
  int num_Kx;
  int inc_stride_Ky;
  int num_stride_Ky;
  int inc_Ky;
  int num_Ky;
  int inc_Ci;
  int off;

  if (param.stride == 1) {
    param.inc_A_0 = 32;
    param.inc_A_1 = 32;

    inc_stride_Kx = 0;
    num_stride_Kx = 0;
    inc_Kx = -96 + 8 * param.batch;
    num_Kx = param.Kx - 1;
    inc_stride_Ky = -96 - 8 * param.batch * num_Kx;
    num_stride_Ky = param.Kx - 1;
    inc_Ky = align_x * param.num_ifm_ch;
    num_Ky = param.Ky - 1;
    inc_Ci = align_x * 8;
    off = -align_x * param.num_ifm_ch * num_Ky - inc_Ci;
  } else if (param.stride == 2) {
    param.inc_A_0 = 32;
    param.inc_A_1 = 0;

    inc_stride_Kx = -64;
    num_stride_Kx = (param.Kx >= param.stride) ? (param.stride - 1) : 0;
    inc_Kx = -64 + 8 * param.batch;
    num_Kx = ((param.Kx + param.stride - 1) >> param.stride_bits) - 1;
    inc_stride_Ky = -64 - 8 * param.batch * num_Kx;
    num_stride_Ky =
        (param.Ky >= param.stride) ? ((Kx_g << param.stride_bits) - 1) : 0;
    inc_Ky = align_x * param.num_ifm_ch;
    num_Ky = (param.Ky >= param.stride)
                 ? (((param.Ky + param.stride - 1) >> param.stride_bits) - 1)
                 : 0;
    inc_Ci = align_x * 8;
    off = -align_x * param.num_ifm_ch * num_Ky - inc_Ci;
  } else { // stride 4
    param.inc_A_0 = 0;
    param.inc_A_1 = 0;

    inc_stride_Kx = 0;
    num_stride_Kx = (param.Kx >= param.stride) ? (param.stride - 1) : 0;
    inc_Kx = 8 * param.batch;
    num_Kx =
        (param.Kx >= param.stride) ? ((param.Kx >> param.stride_bits) - 1) : 0;
    inc_stride_Ky = 0;
    num_stride_Ky =
        (param.Ky >= param.stride) ? ((param.Kx << param.stride_bits) - 1) : 0;
    inc_Ky = -8 * param.batch * num_Kx + align_x * param.num_ifm_ch;
    num_Ky =
        (param.Ky >= param.stride) ? ((param.Ky >> param.stride_bits) - 1) : 0;
    inc_Ci = align_x * 8;
    off = -8 * param.batch * num_Kx - align_x * param.num_ifm_ch * num_Ky -
          inc_Ci;
  }

  SETUP_ITR3D_PARAM(iterator_inner0, inc_stride_Kx, num_stride_Kx, inc_Kx,
                    num_Kx, inc_stride_Ky, param)
  SETUP_ITR3D_PARAM(iterator_inner1, 0, num_stride_Ky, inc_Ky, num_Ky, off,
                    param)

#ifdef __X86SIM__
  printf("inner0 inc_stride_x=%d, num_stride=%d,inc_Kx=%d,num_Kx=%d, off=%d\n",
         inc_stride_Kx, num_stride_Kx, inc_Kx, num_Kx, inc_stride_Ky);
  printf("inner1 inc_stride_y=%d, num_stride=%d,inc_Ky=%d,num_Ky=%d, off=%d\n",
         0, num_stride_Ky, inc_Ky, num_Ky, off);
#endif

  // X->Co->Y
  // Because ifm size is smaller then ofm size, divide stride at dimenstion XY
  int inc_Xi;
  int num_Xi;
  int inc_Co;
  int num_Co;
  int inc_stride_Yi;
  int num_stride_Yi;
  int inc_Yi;

  if (param.stride == 1) {
    inc_Xi = Ci_g * inc_Ci + 8 * 16;
    num_Xi = X_g - 1;
    inc_Co = Ci_g * inc_Ci - num_Xi * 8 * 16;
    num_Co = Co_g - 1;
    inc_stride_Yi = Ci_g * inc_Ci - num_Xi * 8 * 16;
    num_stride_Yi = param.stride * X_g * Co_g - 1;
    inc_Yi = param.num_ifm_ch * align_x;
  } else if (param.stride == 2) {
    inc_Xi = Ci_g * inc_Ci + 8 * 8;
    num_Xi = X_g - 1;
    inc_Co = Ci_g * inc_Ci - num_Xi * 8 * 8;
    num_Co = Co_g - 1;
    inc_stride_Yi = Ci_g * inc_Ci - num_Xi * 8 * 8;
    num_stride_Yi = param.stride * X_g * Co_g - 1;
    inc_Yi = param.num_ifm_ch * align_x;
  } else { // stride 4
    inc_Xi = Ci_g * inc_Ci + 8 * 4;
    num_Xi = X_g - 1;
    inc_Co = Ci_g * inc_Ci - num_Xi * 8 * 4;
    num_Co = Co_g - 1;
    inc_stride_Yi = Ci_g * inc_Ci;
    num_stride_Yi = param.stride * X_g * Co_g - 1;
    inc_Yi = param.num_ifm_ch * align_x;
  }

  SETUP_ITR3D_PARAM(iterator_outer0, inc_Xi, num_Xi, inc_Co, num_Co,
                    inc_stride_Yi, param)
  SETUP_ITR2D_PARAM(iterator_outer1, 0, num_stride_Yi, inc_Yi, param)

#ifdef __X86SIM__
  printf("outer0 inc_Xi=%d,num_Xi=%d,inc_Co=%d,num_Co=%d,inc_stride_Yi=%d\n",
         inc_Xi, num_Xi, inc_Co, num_Co, inc_stride_Yi);
  printf("outer0 inc_stride_Yi=%d,num_stride_Yi=%d,inc_Yi=%d\n", 0,
         num_stride_Yi, inc_Yi);
#endif

  // initilaize wts iterator
  // Kx->Ky->Ci->X->Co->Y
  int inc_w_X;
  int inc_w_Co;
  if (param.stride == 1) {
    inc_w_X = Ci_g * ksize_x * ksize_y * 128;
    inc_w_Co = Ci_g * ksize_x * ksize_y * 128 + 16 * nifms * ksize_x * ksize_y;
    off = Ci_g * ksize_x * ksize_y * 128 -
          param.num_Co * 16 * nifms * ksize_x * ksize_y;
  } else if (param.stride == 2) {
    inc_w_X = Ci_g * ksize_x * ksize_y * 128;
    inc_w_Co = Ci_g * ksize_x * ksize_y * 128 + 16 * nifms * ksize_x * ksize_y;
    off = Ci_g * ksize_x * ksize_y * 128 -
          param.num_Co * 16 * nifms * ksize_x * ksize_y;
  } else { // stride 4
    inc_w_X = Ci_g * ksize_x * ksize_y * 128;
    inc_w_Co = Ci_g * ksize_x * ksize_y * 128 + 16 * nifms * ksize_x * ksize_y;
    off = Ci_g * ksize_x * ksize_y * 128 -
          param.num_Co * 16 * nifms * ksize_x * ksize_y;
  }
  SETUP_ITR3D_PARAM(iterator_weights, inc_w_X, param.num_X, inc_w_Co,
                    param.num_Co, off, param)

#ifdef __X86SIM__
  printf("iterator_weights1 "
         "inc_w_X=%d,num_X=%d,inc_w_Co=%d,num_Ky=%d, off=%d\n",
         inc_w_X, param.num_X, inc_w_Co, param.num_Co, off);
#endif

#ifdef __X86SIM__
  printf("ifm_xsize: %u\n", param.ifm_xsize);
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
  printf("oyp: %u\n", param.oyp);
  printf("oxp: %u\n", param.oxp);
  printf("Ky: %u\n", param.Ky);
  printf("Kx: %u\n", param.Kx);
  printf("stride: %u\n", param.stride);
  printf("curr_depth_iter: %u\n", param.curr_depth_iter);
  printf("io_mode: %u\n", param.io_mode);
  printf("ofm_offset_top: %u\n", param.ofm_offset_top);
  printf("exec_type: %u\n", param.exec_type);
  printf("inner_g: %u\n", param.inner_g);
  printf("outer_g: %u\n", param.outer_g);
  printf("ifm_offset: %d\n", param.ifm_offset);
  printf("wts_offset: %d\n", param.wts_offset);
  printf("inc_S_0: %d\n", param.inc_S_0);
  printf("inc_S_1: %d\n", param.inc_S_1);
  printf("inc_S_2: %d\n", param.inc_S_2);
  printf("inc_ST_0: %d\n", param.inc_ST_0);
  printf("inc_ST_1: %d\n", param.inc_ST_1);
  printf("inc_ST_2: %d\n", param.inc_ST_2);
  printf("step_align: %d\n", param.step_align);
  printf("shfl_0: %d\n", param.shfl_0);
  printf("shfl_1: %d\n", param.shfl_1);
  printf("shfl_2: %d\n", param.shfl_2);
  printf("num_Co: %u\n", param.num_Co);
  printf("num_X: %u\n", param.num_X);
#endif
}
#endif

#if __AIE_ARCH__ == 21 || (__AIE_ARCH__ == 22) // AIE2p conv2d_transpose setup
static void setup_conv2d_transpose_params(int8_t *params_ptr,
                                   conv2d_transpose_params_t &param,
                                   uint8_t batch_size = 1,
                                   uint8_t aximm_rtp_enabled = 0) {

  // Copy 32 bytes of layer params
  // get_layer_params((uint32_t *)params_ptr,(uint32_t *)&param);
  aie::vector<int32, 8> pars = aie::load_v<8>((int32 *)(params_ptr));
  aie::store_v((int32 __aie_dm_resource_a *)&param, pars);
  params_ptr += 62;
  param.batch = (uint8_t)*params_ptr++;

  uint8_t ksize = param.ksize;
  uint8_t stride_bits = param.stride_bits;

  uint8_t ksize_x = ksize;
  uint8_t ksize_y = ksize;
  uint8_t stride_bits_x = stride_bits;
  uint8_t stride_bits_y = stride_bits;
  uint8_t nifms = param.num_ifm_ch;
  uint8_t ifm_x = param.ifm_xsize;

  int value = (ksize >> 4) & 0x0F;
  ksize_x = ((value == 0) ? ksize : ((ksize >> 4) & 0x0F));
  ksize_y = ((value == 0) ? ksize : (ksize & 0x0F));

  uint8_t stride_x = 1 << stride_bits_x;
  uint8_t stride_y = 1 << stride_bits_y;

  // Extract IFM sign packed with ifm2_sign (called in0_sign)
  uint8_t tmp = param.in0_sign; // offset 30 input0 sign for add2d
  param.ifm_sign =
      (int)((tmp >> 4) & 0x01);  // offset 30 input sign for conv2d_transpose
  param.in0_sign = (tmp & 0x01); // offset 30 input0 sign for add2d

  // Fixes related to layer param copy
  param.psum_buff_offset = param.psum_buff_offset_scaled * 128;
  param.ofm_offset_left = (param.ofm_offset_packed) & 0x0F;
  uint8_t nofms = param.num_ofm_ch;

  param.out_sign =
      1 - (int)param.run_time_act; // Run time activation 0: Linear   1: ReLU
  param.curr_depth_iter = 0;       // initialize depth iter to 0
  param.batch = (param.batch == 0) ? batch_size : param.batch;
  param.batch_log2 = (param.batch == 4) ? 2 : ((param.batch == 2) ? 1 : 0);
  param.oyp = (param.ifm_ysize - ksize_y + 1) >> stride_bits_y;
  param.oxp = (ifm_x - ksize_x + 1) >> stride_bits_x;

  param.Ky = ksize_y;
  param.Kx = ksize_x;
  param.stride = 1 << stride_bits_y;
  param.stride_condition = param.stride - 1;
  param.hdr_len = (!aximm_rtp_enabled ? LAYER_PARAM_BYTES : 0) + nofms * 4 * 2;

  param.ofm_xsize = (param.ifm_xsize - 1) * stride_x + ksize_x;
  param.ofm_ysize = (param.ifm_ysize - 1) * stride_y + ksize_y;

  uint8_t Kx_g = ((param.Kx + param.stride - 1) >> stride_bits_x)
                 << stride_bits_x;
  uint8_t Ky_g = ((param.Ky + param.stride - 1) >> stride_bits_y)
                 << stride_bits_y;
  uint8_t X_g = ((param.ofm_xsize * param.batch) + 15) >> 4; // X align with 16
  uint8_t Ci_g = nifms >> 3;
  uint8_t Co_g = (nofms >> 4);

  param.num_Y = param.ofm_ysize - 1;
  param.num_X_Co = X_g * Co_g - 1;
  param.num_Co = Co_g - 1;
  param.num_X = X_g - 1;
  param.num_Ci = Ci_g - 1;
  param.num_Ky = Ky_g - 1;
  param.num_Kx = Kx_g - 1;

#ifdef __X86SIM__
  printf("Ci_g=%d,Co_g=%d,Ky=%u,Kx=%u\n", Ci_g, Co_g, param.Ky, param.Kx);
#endif
  param.inner_g = Ci_g * Kx_g * Ky_g;
  param.outer_g = X_g * Co_g * param.ofm_ysize;

  uint16_t step_Kx = 8 << param.batch_log2;
  param.step_align = (step_Kx & 15 ? 2 : (step_Kx & 31 ? 3 : 0));
  int align_x = (param.ifm_xsize + 2 * (ksize_x - 1) / stride_x + 15) / 16 *
                16 * param.batch; // align_x for ifm
  param.ifm_offset =
      (nifms - 8) * align_x; // for inverse load of ifm in loop Cin
  param.wts_offset = 16 * nifms * Kx_g * Ky_g -
                     128; // for inverse load of wts for loop Ky and Kx
#ifdef __X86SIM__
  printf("align_ifm_x=%d\n", align_x);
#endif

  // ifm shuffle
  if (param.stride == 1) {
    param.shuffle[0] = 22;
    param.shuffle[1] = 22;
    param.shuffle[2] = 22;
    param.shuffle[3] = 22;
  } else if (param.stride == 2) {
    param.shuffle[0] = 14;
    param.shuffle[1] = 22;
    param.shuffle[2] = 11;
    param.shuffle[3] = 14;
  } else { // stride 4
    param.shuffle[0] = 14;
    param.shuffle[1] = 14;
    param.shuffle[2] = 14;
    param.shuffle[3] = 14;
  }

  // initialize ifm iterator
  // stride->Kx->Stride->Ky->off
  int inc_stride_Kx;
  int num_stride_Kx;
  int inc_Kx;
  int num_Kx;
  int inc_stride_Ky;
  int num_stride_Ky;
  int inc_Ky;
  int num_Ky;
  int inc_Ci;
  int off;

  if (param.stride == 1) {
    param.inc_A_0 = 64;
    param.inc_A_1 = 0;

    inc_stride_Kx = 0;
    num_stride_Kx = 0;
    inc_Kx = -64 + 8 * param.batch;
    num_Kx = param.Kx - 1;
    inc_stride_Ky = -64 - 8 * param.batch * num_Kx;
    num_stride_Ky = param.Kx - 1;
    inc_Ky = align_x * param.num_ifm_ch;
    num_Ky = param.Ky - 1;
    inc_Ci = align_x * 8;
    off = -align_x * param.num_ifm_ch * num_Ky - inc_Ci;
  } else if (param.stride == 2) {
    param.inc_A_0 = 0;
    param.inc_A_1 = 0;

    inc_stride_Kx = 0;
    num_stride_Kx = (param.Kx >= param.stride) ? (param.stride - 1) : 0;
    inc_Kx = 64 + 8 * param.batch;
    num_Kx = ((param.Kx + param.stride - 1) >> param.stride_bits) - 1;
    inc_stride_Ky = -(64 + 8 * param.batch) * num_Kx;
    num_stride_Ky =
        (param.Ky >= param.stride) ? ((Kx_g << param.stride_bits) - 1) : 0;
    inc_Ky = align_x * param.num_ifm_ch;
    num_Ky = (param.Ky >= param.stride)
                 ? (((param.Ky + param.stride - 1) >> param.stride_bits) - 1)
                 : 0;
    inc_Ci = align_x * 8;
    off = -align_x * param.num_ifm_ch * num_Ky - inc_Ci;
  } else { // stride 4
    param.inc_A_0 = 0;
    param.inc_A_1 = 0;

    inc_stride_Kx = 0;
    num_stride_Kx = (param.Kx >= param.stride) ? (param.stride - 1) : 0;
    inc_Kx = 8 * param.batch;
    num_Kx =
        (param.Kx >= param.stride) ? ((param.Kx >> param.stride_bits) - 1) : 0;
    inc_stride_Ky = 0;
    num_stride_Ky =
        (param.Ky >= param.stride) ? ((param.Kx << param.stride_bits) - 1) : 0;
    inc_Ky = -8 * param.batch * num_Kx + align_x * param.num_ifm_ch;
    num_Ky =
        (param.Ky >= param.stride) ? ((param.Ky >> param.stride_bits) - 1) : 0;
    inc_Ci = align_x * 8;
    off = -8 * param.batch * num_Kx - align_x * param.num_ifm_ch * num_Ky -
          inc_Ci;
  }

  SETUP_ITR3D_PARAM(iterator_inner0, inc_stride_Kx, num_stride_Kx, inc_Kx,
                    num_Kx, inc_stride_Ky, param)
  SETUP_ITR3D_PARAM(iterator_inner1, 0, num_stride_Ky, inc_Ky, num_Ky, off,
                    param)

#ifdef __X86SIM__
  printf("inner0 inc_stride_x=%d, num_stride=%d,inc_Kx=%d,num_Kx=%d, off=%d\n",
         inc_stride_Kx, num_stride_Kx, inc_Kx, num_Kx, inc_stride_Ky);
  printf("inner1 inc_stride_y=%d, num_stride=%d,inc_Ky=%d,num_Ky=%d, off=%d\n",
         0, num_stride_Ky, inc_Ky, num_Ky, off);
#endif

  // X->Co->Y
  // Because ifm size is smaller then ofm size, divide stride at dimenstion XY
  int inc_Xi;
  int num_Xi;
  int inc_Co;
  int num_Co;
  int inc_stride_Yi;
  int num_stride_Yi;
  int inc_Yi;

  if (param.stride == 1) {
    inc_Xi = Ci_g * inc_Ci + 8 * 16;
    num_Xi = X_g - 1;
    inc_Co = Ci_g * inc_Ci - num_Xi * 8 * 16;
    num_Co = Co_g - 1;
    inc_stride_Yi = Ci_g * inc_Ci - num_Xi * 8 * 16;
    num_stride_Yi = param.stride * X_g * Co_g - 1;
    inc_Yi = param.num_ifm_ch * align_x;
  } else if (param.stride == 2) {
    inc_Xi = Ci_g * inc_Ci + 8 * 8;
    num_Xi = X_g - 1;
    inc_Co = Ci_g * inc_Ci - num_Xi * 8 * 8;
    num_Co = Co_g - 1;
    inc_stride_Yi = Ci_g * inc_Ci - num_Xi * 8 * 8;
    num_stride_Yi = param.stride * X_g * Co_g - 1;
    inc_Yi = param.num_ifm_ch * align_x;
  } else { // stride 4
    inc_Xi = Ci_g * inc_Ci + 8 * 4;
    num_Xi = X_g - 1;
    inc_Co = Ci_g * inc_Ci - num_Xi * 8 * 4;
    num_Co = Co_g - 1;
    inc_stride_Yi = Ci_g * inc_Ci;
    num_stride_Yi = param.stride * X_g * Co_g - 1;
    inc_Yi = param.num_ifm_ch * align_x;
  }

  SETUP_ITR3D_PARAM(iterator_outer0, inc_Xi, num_Xi, inc_Co, num_Co,
                    inc_stride_Yi, param)
  SETUP_ITR2D_PARAM(iterator_outer1, 0, num_stride_Yi, inc_Yi, param)

#ifdef __X86SIM__
  printf("outer0 inc_Xi=%d,num_Xi=%d,inc_Co=%d,num_Co=%d,inc_stride_Yi=%d\n",
         inc_Xi, num_Xi, inc_Co, num_Co, inc_stride_Yi);
  printf("outer0 inc_stride_Yi=%d,num_stride_Yi=%d,inc_Yi=%d\n", 0,
         num_stride_Yi, inc_Yi);
#endif

  // initilaize wts iterator
  // Kx->Ky->Ci->X->Co->Y
  int inc_w_X;
  int inc_w_Co;
  if (param.stride == 1) {
    inc_w_X = Ci_g * ksize_x * ksize_y * 128;
    inc_w_Co = Ci_g * ksize_x * ksize_y * 128 + 16 * nifms * ksize_x * ksize_y;
    off = Ci_g * ksize_x * ksize_y * 128 -
          param.num_Co * 16 * nifms * ksize_x * ksize_y;
  } else if (param.stride == 2) {
    inc_w_X = Ci_g * ksize_x * ksize_y * 128;
    inc_w_Co = Ci_g * ksize_x * ksize_y * 128 + 16 * nifms * ksize_x * ksize_y;
    off = Ci_g * ksize_x * ksize_y * 128 -
          param.num_Co * 16 * nifms * ksize_x * ksize_y;
  } else { // stride 4
    inc_w_X = Ci_g * ksize_x * ksize_y * 128;
    inc_w_Co = Ci_g * ksize_x * ksize_y * 128 + 16 * nifms * ksize_x * ksize_y;
    off = Ci_g * ksize_x * ksize_y * 128 -
          param.num_Co * 16 * nifms * ksize_x * ksize_y;
  }
  SETUP_ITR3D_PARAM(iterator_weights, inc_w_X, param.num_X, inc_w_Co,
                    param.num_Co, off, param)

#ifdef __X86SIM__
  printf("iterator_weights1 "
         "inc_w_X=%d,num_X=%d,inc_w_Co=%d,num_Ky=%d, off=%d\n",
         inc_w_X, param.num_X, inc_w_Co, param.num_Co, off);
#endif

#ifdef __X86SIM__
  printf("ifm_xsize: %u\n", param.ifm_xsize);
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
  printf("oyp: %u\n", param.oyp);
  printf("oxp: %u\n", param.oxp);
  printf("Ky: %u\n", param.Ky);
  printf("Kx: %u\n", param.Kx);
  printf("stride: %u\n", param.stride);
  printf("curr_depth_iter: %u\n", param.curr_depth_iter);
  printf("io_mode: %u\n", param.io_mode);
  printf("ofm_offset_top: %u\n", param.ofm_offset_top);
  printf("exec_type: %u\n", param.exec_type);
  printf("inner_g: %u\n", param.inner_g);
  printf("outer_g: %u\n", param.outer_g);
  printf("ifm_offset: %d\n", param.ifm_offset);
  printf("wts_offset: %d\n", param.wts_offset);
  printf("inc_S_0: %d\n", param.inc_S_0);
  printf("inc_S_1: %d\n", param.inc_S_1);
  printf("inc_S_2: %d\n", param.inc_S_2);
  printf("inc_ST_0: %d\n", param.inc_ST_0);
  printf("inc_ST_1: %d\n", param.inc_ST_1);
  printf("inc_ST_2: %d\n", param.inc_ST_2);
  printf("step_align: %d\n", param.step_align);
  printf("shfl_0: %d\n", param.shfl_0);
  printf("shfl_1: %d\n", param.shfl_1);
  printf("shfl_2: %d\n", param.shfl_2);
  printf("num_Co: %u\n", param.num_Co);
  printf("num_X: %u\n", param.num_X);
#endif
}
#endif

#endif //__CONV2D_TRANSPOSE_PARAMS_H__
