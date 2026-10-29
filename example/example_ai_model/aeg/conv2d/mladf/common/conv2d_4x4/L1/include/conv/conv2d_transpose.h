// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
/*
(c) Copyright 2019 - 2021 Xilinx, Inc. All Rights reserved.
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

/*
 *
 * 2D Convolution Kernel for AI Engine
 *
 * Performs 2d convolution on data present in "ifms" using weights from "wts"
 * Result is stored in "ofms".
 *
 */

#ifndef __CONV2D_TRANSPOSE_H__
#define __CONV2D_TRANSPOSE_H__

#include "aie.hpp"
#include "aie_api/utils.hpp" // needed for aie:print
#include <adf.h>

#include "utils.h" // Contains some utility functions and definitions related to intrinsics usage
#include "mllib_const.h" // Constants shared across graphs like activation type
#include "conv2d_transpose_params.h" // Parameters like increments and wraps needed by kernel

using namespace aie;

#ifdef __X86SIM__
#define CT_LOAD_V CT_load_v
#define DEBUG_CONV2D_TRANSPOSE true
#define DEBUG_CONV2D_TRANSPOSE_PSUM true

#else
#define CT_LOAD_V aie::load_v
#endif

#if __AIE_ARCH__ == 20
template <int N>
__attribute__((always_inline)) aie::vector<int8, N> CT_load_v(int8 *ptr) {
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
__attribute__((always_inline)) aie::vector<uint8, N> CT_load_v(uint8 *ptr) {
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
__attribute__((always_inline)) aie::vector<int16, N> CT_load_v(int16 *ptr) {
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

#if __AIE_ARCH__ == 21
template <int N>
__attribute__((always_inline)) aie::vector<int8, N> CT_load_v(int8 *ptr) {
#ifdef __X86SIM__
  aie::vector<int8, N> res;

  int tmp_val = (long)ptr;
  tmp_val %= 64;
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
__attribute__((always_inline)) aie::vector<uint8, N> CT_load_v(uint8 *ptr) {
#ifdef __X86SIM__
  aie::vector<uint8, N> res;

  int tmp_val = (long)ptr;
  tmp_val %= 64;
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
__attribute__((always_inline)) aie::vector<int16, N> CT_load_v(int16 *ptr) {
#ifdef __X86SIM__
  aie::vector<int16, N> res;

  int tmp_val = (long)ptr;
  tmp_val %= 64;
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


template <typename dtype, uint8_t dim> struct group_data_t {
  vector<dtype, dim> x0, x1;
};

#if __AIE_ARCH__ == 20
template <uint8_t batch_size, act_t act_type, out_mode_t out_mode,
          typename dtype_ifm, typename dtype_wts, typename dtype_ofm>
INLINE void
conv2d_transpose_generic(int *__restrict input, int *__restrict weights,
                         int *__restrict output,
                         conv2d_transpose_params_t &conv2d_transpose_params) {

  // initialize p_sum and bias
  int16 OFM_DM_BANK *p_sum = (int16 OFM_DM_BANK *)output +
                             (conv2d_transpose_params.psum_buff_offset >> 1);
  int16 OFM_DM_BANK *p_bias =
      (int16 OFM_DM_BANK *)(weights) + (LAYER_PARAM_BYTES >> 1);
  int16 OFM_DM_BANK *p_init16 =
      (conv2d_transpose_params.psum_in == 0) ? p_bias : p_sum;

  // initialize ifm, wts and ofm
  dtype_ifm IFM_DM_BANK *p_in = (dtype_ifm IFM_DM_BANK *)(input) + 
                                conv2d_transpose_params.ifm_offset;
  dtype_wts WT_DM_BANK *p_w = (dtype_wts WT_DM_BANK *)(weights) +
                              conv2d_transpose_params.hdr_len +
                              conv2d_transpose_params.wts_offset;
  dtype_ofm OFM_DM_BANK *p_out = (conv2d_transpose_params.psum_out == 0) ? 
                                (dtype_ofm OFM_DM_BANK *)output : 
                                (dtype_ofm OFM_DM_BANK *)p_sum;

  vector<dtype_ifm, 32> vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3;
  vector<dtype_wts, 64> Ybuff0, Ybuff1;
  group_data_t<uint8_t, 64> Xbuff;
  vector<uint8_t, 64> zero_vec = zeros<uint8_t,64>();
  using mmul = mmul<4, 8, 8, dtype_ifm, dtype_wts>;

// input: mmul type Cbuff
// shift_input: Intial shift applied to convert accum<acc32,32> to vec<int16,16>
// alpha: This is the scaled value to be multiplied
// shift_alpha: shift required to remove the scaling factor of alpha
// shift_out: shift applied before saving to final output
// output_ptr: pointer to the location where data is to be saved
#define LEAKY_RELU_COMPUTE(input, shift_input, alpha, shift_alpha, shift_out,  \
                           output_ptr)                                         \
  {                                                                            \
    vector<int16, 32> input_vec =                                              \
        input.template to_vector<int16>(shift_input);                          \
    accum<acc32, 32> acc =                                                     \
        mul<acc32, vector<int16, 32>, int16>(input_vec, alpha);                \
    vector<int16, 32> alpha_vec = acc.to_vector<int16>(shift_alpha);           \
    vector<int16, 32> result_vec =                                             \
        max<vector<int16, 32>, vector<int16, 32>>(input_vec, alpha_vec);       \
    acc.from_vector(result_vec, 0);                                            \
    store_v(output_ptr, acc.to_vector<int8>(shift_out));                       \
  }

  // Following counters are needed for ADD_2D/3D_BYTE intrinsics
  // SETUP_ITR2D_COUNTERS(iterator_name) / SETUP_ITR3D_COUNTERS(iterator_name)
  // type 1: 2D iterator, 2: 3d Iterator
  // e.g. SETUP_ITR3D_COUNTERS(iterator_inner)
  // Resulting count variable is : iterator_inner_cnt0, iterator_inner_cnt_1
  SETUP_ITR3D_COUNTERS(iterator_inner0)
  SETUP_ITR3D_COUNTERS(iterator_inner1)
  SETUP_ITR3D_COUNTERS(iterator_outer0)
  SETUP_ITR2D_COUNTERS(iterator_outer1)
  SETUP_ITR3D_COUNTERS(iterator_weights)
  SETUP_ITR3D_COUNTERS(iterator_pinit)
  SETUP_ITR3D_COUNTERS(iterator_pout)

  addr_t y_cnt = 0;
  addr_t Co_cnt = 0;
  addr_t x_cnt = 0;
  addr_t Ci_cnt = 0;
  addr_t Ky_cnt = 0;
  addr_t Kx_cnt = 0;

#ifdef DEBUG_CONV2D_TRANSPOSE
for(int i =0; i< 16;i++){
print(CT_LOAD_V<32>((uint8 IFM_DM_BANK *)p_in), true, "test: ");
printf("addr:%d\n",p_in);
p_in = byte_incr(p_in, 32);
}
p_in = byte_incr(p_in, -32*16);
#endif

  if
    constexpr(batch_size < 4) { p_in += 31; }

  AIE_PREPARE_FOR_PIPELINING 
  AIE_LOOP_RANGE(2, ) 
  for (unsigned j = 0; j < conv2d_transpose_params.outer_g; j++){
      
// Load OFM -- 16width * 8och
#ifdef DEBUG_CONV2D_TRANSPOSE_PSUM
      printf("curr_depth_iter:%d, num_ifm_depth_iter:%d, psum_in:%d, out_mode:%d\n", 
              conv2d_transpose_params.curr_depth_iter, conv2d_transpose_params.num_ifm_depth_iter, 
              conv2d_transpose_params.psum_in, out_mode);
      printf("pinit:%d\n", p_init16);
#endif
      mmul Cbuff0(CT_LOAD_V<32>((int16 OFM_DM_BANK *)p_init16), // w0-3 oc0-7
                  conv2d_transpose_params.shift_psum_in);
      p_init16 = (int16 OFM_DM_BANK *)byte_incr(
          (int16 *)p_init16, conv2d_transpose_params.inc_S_0);
      mmul Cbuff1(CT_LOAD_V<32>((int16 OFM_DM_BANK *)p_init16),
                  conv2d_transpose_params.shift_psum_in);
      p_init16 = (int16 OFM_DM_BANK *)byte_incr(
          (int16 *)p_init16, conv2d_transpose_params.inc_S_0);

      mmul Cbuff2(CT_LOAD_V<32>((int16 OFM_DM_BANK *)p_init16),
                  conv2d_transpose_params.shift_psum_in);
      p_init16 = (int16 OFM_DM_BANK *)byte_incr(
          (int16 *)p_init16, conv2d_transpose_params.inc_S_0);
      mmul Cbuff3(CT_LOAD_V<32>((int16 OFM_DM_BANK *)p_init16),
                  conv2d_transpose_params.shift_psum_in);
      p_init16 = (int16 OFM_DM_BANK *)byte_incr(
          (int16 *)p_init16, conv2d_transpose_params.inc_S_1);

      mmul Cbuff4(CT_LOAD_V<32>((int16 OFM_DM_BANK *)p_init16),
                  conv2d_transpose_params.shift_psum_in);
      p_init16 = (int16 OFM_DM_BANK *)byte_incr(
          (int16 *)p_init16, conv2d_transpose_params.inc_S_0);
      mmul Cbuff5(CT_LOAD_V<32>((int16 OFM_DM_BANK *)p_init16),
                  conv2d_transpose_params.shift_psum_in);
      p_init16 = (int16 OFM_DM_BANK *)byte_incr(
          (int16 *)p_init16, conv2d_transpose_params.inc_S_0);

      mmul Cbuff6(CT_LOAD_V<32>((int16 OFM_DM_BANK *)p_init16),
                  conv2d_transpose_params.shift_psum_in);
      p_init16 = (int16 OFM_DM_BANK *)byte_incr(
          (int16 *)p_init16, conv2d_transpose_params.inc_S_0);
      mmul Cbuff7(CT_LOAD_V<32>((int16 OFM_DM_BANK *)p_init16),
                  conv2d_transpose_params.shift_psum_in);

#ifdef DEBUG_CONV2D_TRANSPOSE_PSUM
      printf("pinit:%d\n", p_init16);
      
      printf("\n");
      printf("shift_psum_in:%d, shift_psum_out:%d\n",
             conv2d_transpose_params.shift_psum_in,
             conv2d_transpose_params.shift_psum_out);
      print(Cbuff0.template to_vector<int16>(
                conv2d_transpose_params.shift_psum_in),
            true, "Cbuff0_pre: ");
      print(Cbuff1.template to_vector<int16>(
                conv2d_transpose_params.shift_psum_in),
            true, "Cbuff1_pre: ");
      print(Cbuff2.template to_vector<int16>(
                conv2d_transpose_params.shift_psum_in),
            true, "Cbuff2_pre: ");
      print(Cbuff3.template to_vector<int16>(
                conv2d_transpose_params.shift_psum_in),
            true, "Cbuff3_pre: ");
      print(Cbuff4.template to_vector<int16>(
                conv2d_transpose_params.shift_psum_in),
            true, "Cbuff4_pre: ");
      print(Cbuff5.template to_vector<int16>(
                conv2d_transpose_params.shift_psum_in),
            true, "Cbuff5_pre: ");
      print(Cbuff6.template to_vector<int16>(
                conv2d_transpose_params.shift_psum_in),
            true, "Cbuff6_pre: ");
      print(Cbuff7.template to_vector<int16>(
                conv2d_transpose_params.shift_psum_in),
            true, "Cbuff7_pre: ");

      print(Cbuff0.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff0_pre_shift: ");
      print(Cbuff1.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff1_pre_shift: ");
      print(Cbuff2.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff2_pre_shift: ");
      print(Cbuff3.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff3_pre_shift: ");
      print(Cbuff4.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff4_pre_shift: ");
      print(Cbuff5.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff5_pre_shift: ");
      print(Cbuff6.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff6_pre_shift: ");
      print(Cbuff7.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff7_pre_shift: ");
#endif

      AIE_PREPARE_FOR_PIPELINING 
      AIE_LOOP_RANGE(2, ) 
      for (int i = 0; i < conv2d_transpose_params.inner_g; i++){
          // Load IFM data -- 16width * 8ich
          if
            constexpr(batch_size < 4) {
#ifdef DEBUG_CONV2D_TRANSPOSE
              printf("pin:%d\n", p_in);
#endif

              // shift params
              int pdel = (long)p_in;
              bool modKx = Kx_cnt & conv2d_transpose_params.stride_bits;
              bool modKy = (y_cnt+conv2d_transpose_params.Ky-Ky_cnt)&1;
              uint8_t frac = modKx ? 0 : ((pdel & 31) + 33);
              uint8_t step_align =  modKx ? 0 : conv2d_transpose_params.step_align;
              uint8_t shiftKx= modKx ? 0 : 8;
              uint8_t shiftKy= modKy ? 64 : 0;
              uint8_t stride_shift = (conv2d_transpose_params.stride_bits == 0) ? 64:(shiftKy - shiftKx);

#ifdef DEBUG_CONV2D_TRANSPOSE
          printf("\nouter_loop:%d, inner_loop:%d, step_align:%d, frac:%d\n", j,
                 i, step_align, frac);
#endif            
              vector<uint8_t, 64> Sbuff0, Sbuff1, Xbuff0, Xbuff1;
              Sbuff0.insert(0, CT_LOAD_V<32>((uint8 IFM_DM_BANK *)p_in));
              p_in = byte_incr(p_in, conv2d_transpose_params.inc_A_0);
              Sbuff0.insert(1, CT_LOAD_V<32>((uint8 IFM_DM_BANK *)p_in));
              p_in = byte_incr(p_in, conv2d_transpose_params.inc_A_1);
              Sbuff1.insert(0, CT_LOAD_V<32>((uint8 IFM_DM_BANK *)p_in));
              p_in = byte_incr(p_in, conv2d_transpose_params.inc_A_0);
              Sbuff1.insert(1, CT_LOAD_V<32>((uint8 IFM_DM_BANK *)p_in));
#ifdef DEBUG_CONV2D_TRANSPOSE
              printf("pin:%d\n", p_in);
#endif
              Xbuff.x0 = shiftx(Xbuff.x0, Sbuff0, step_align, frac);
              Xbuff.x1 = shiftx(Xbuff.x1, Sbuff1, step_align, frac);

              // shuffle params
              // stride1-batch1 stride2-batch2
              /*
              Xbuff0 = shuffle(Xbuff.x0, zero_vec, shuffle_T512_1x2_lo);  //22
              Xbuff0 = shuffle(Xbuff.x0, zero_vec, shuffle_T512_1x2_lo);  //22

              Xbuff1 = shuffle(Xbuff.x1, zero_vec, shuffle_T512_1x2_lo); //22
              Xbuff1 = shuffle(Xbuff1, zero_vec, shuffle_T512_1x2_lo); //22
              */

              // stride2-batch1 stride4-batch2
              /*
              Xbuff0 = shuffle(Xbuff.x0, zero_vec, shuffle_T64_2x8_lo); //14
              Xbuff0 = shuffle(Xbuff0, zero_vec, shuffle_T512_1x2_lo); //22

              Xbuff1 = shuffle(Xbuff.x1, zero_vec, shuffle_T64_2x8_lo); //14
              Xbuff1 = shuffle(Xbuff1, zero_vec, shuffle_T512_1x2_lo); //22
              */

              // stride4-batch1
              /*
              Xbuff0 = shuffle(Xbuff.x0, zero_vec, shuffle_T64_2x8_lo);
              Xbuff0 = shuffle(Xbuff0, zero_vec, shuffle_T64_2x8_lo);

              Xbuff1 = shuffle(Xbuff.x1, zero_vec, shuffle_T128_4x2_hi);
              Xbuff1 = shuffle(Xbuff1,   zero_vec,   shuffle_T512_1x2_lo);
              */

              Xbuff0 = shuffle(Xbuff.x0, zero_vec, conv2d_transpose_params.shuffle[0]);
              Xbuff0 = shuffle(Xbuff0, zero_vec, conv2d_transpose_params.shuffle[1]);
              Xbuff0 = shift(zero_vec, Xbuff0, stride_shift);

              Xbuff1 = shuffle(Xbuff.x1, zero_vec, conv2d_transpose_params.shuffle[2]);
              Xbuff1 = shuffle(Xbuff1, zero_vec, conv2d_transpose_params.shuffle[3]);
              Xbuff1 = shift(zero_vec, Xbuff1, stride_shift);

              vec_Xbuff0 = Xbuff0.cast_to<dtype_ifm>().template extract<32>(0);
              vec_Xbuff1 = Xbuff0.cast_to<dtype_ifm>().template extract<32>(1);
              vec_Xbuff2 = Xbuff1.cast_to<dtype_ifm>().template extract<32>(0);
              vec_Xbuff3 = Xbuff1.cast_to<dtype_ifm>().template extract<32>(1);

#ifdef DEBUG_CONV2D_TRANSPOSE
             printf("step_align:%d, frac:%d, shift:%d, shiftKx:%d, shiftKy:%d, y:%d, ky:%d, kx:%d, shfl0:%d, shfl1:%d, shfl2:%d, shfl3:%d\n",
                  conv2d_transpose_params.step_align, frac, stride_shift, shiftKx, shiftKy, y_cnt, Ky_cnt, Kx_cnt,
                  conv2d_transpose_params.shuffle[0],
                  conv2d_transpose_params.shuffle[1],
                  conv2d_transpose_params.shuffle[2],
                  conv2d_transpose_params.shuffle[3]);

              print(Sbuff0, true, "Sbuff0: ");
              print(Sbuff1, true, "Sbuff1: ");
              print(Xbuff.x0, true, "Xbuff.x0: ");
              print(Xbuff.x1, true, "Xbuff.x1: ");
              print(Xbuff0, true, "Xbuff0: ");
              print(Xbuff1, true, "Xbuff1: ");
#endif
            }
          else {
            // stride1 batch4
#ifdef DEBUG_CONV2D_TRANSPOSE
          printf("\nouter_loop:%d, inner_loop:%d\n", j, i);
          printf("pin:%d\n", p_in);
#endif    
            vec_Xbuff0 = CT_LOAD_V<32>((dtype_ifm IFM_DM_BANK *)p_in);
            p_in = byte_incr(p_in, conv2d_transpose_params.inc_A_0);
            vec_Xbuff1 = CT_LOAD_V<32>((dtype_ifm IFM_DM_BANK *)p_in);
            p_in = byte_incr(p_in, conv2d_transpose_params.inc_A_1);
            vec_Xbuff2 = CT_LOAD_V<32>((dtype_ifm IFM_DM_BANK *)p_in);
            p_in = byte_incr(p_in, conv2d_transpose_params.inc_A_0);
            vec_Xbuff3 = CT_LOAD_V<32>((dtype_ifm IFM_DM_BANK *)p_in);
#ifdef DEBUG_CONV2D_TRANSPOSE
          printf("pin:%d\n", p_in);
#endif
          }
          p_in = INCR_ITR3D_PARAM(p_in, iterator_inner0, conv2d_transpose_params) 
          p_in = INCR_ITR3D_PARAM(p_in, iterator_inner1, conv2d_transpose_params)

          // update cnt for kx and ky
          dtype_ifm IFM_DM_BANK *p0;
          add_3d_byte(p0,0,conv2d_transpose_params.num_Kx,Kx_cnt,0,conv2d_transpose_params.num_Ky,Ky_cnt,0);

#ifdef DEBUG_CONV2D_TRANSPOSE
          print(vec_Xbuff0, true, "vec_Xbuff0: ");
          print(vec_Xbuff1, true, "vec_Xbuff1: ");
          print(vec_Xbuff2, true, "vec_Xbuff2: ");
          print(vec_Xbuff3, true, "vec_Xbuff3: ");
#endif

#ifdef DEBUG_CONV2D_TRANSPOSE
          printf("p_w:%d\n", p_w);
#endif
          // Load weights -- 8ich * 16och
          Ybuff0.insert(0, CT_LOAD_V<32>(p_w)); // ic0-7 oc0-7
          p_w = byte_incr(p_w, 32);
          Ybuff0.insert(1, CT_LOAD_V<32>(p_w));
          p_w = byte_incr(p_w, 32);

          Ybuff1.insert(0, CT_LOAD_V<32>(p_w)); // ic0-7 oc8-15
          p_w = byte_incr(p_w, 32);
          Ybuff1.insert(1, CT_LOAD_V<32>(p_w));
          // inverse iteration for transposed conv
          p_w = byte_incr(p_w, -224);

#ifdef DEBUG_CONV2D_TRANSPOSE
          printf("p_w:%d\n", p_w);
          print(Ybuff0, true, "vec_Ybuff0: ");
          print(Ybuff1, true, "vec_Ybuff1: ");
#endif

// Update OFM
#ifdef USE_RTP_SIGN
          int sign_x = conv2d_transpose_params.ifm_sign;
          Cbuff0.mac(op_sign(vec_Xbuff0, sign_x), op_sign(Ybuff0, 1)); // oc0-7
          Cbuff1.mac(op_sign(vec_Xbuff1, sign_x), op_sign(Ybuff0, 1));
          Cbuff2.mac(op_sign(vec_Xbuff2, sign_x), op_sign(Ybuff0, 1));
          Cbuff3.mac(op_sign(vec_Xbuff3, sign_x), op_sign(Ybuff0, 1));

          Cbuff4.mac(op_sign(vec_Xbuff0, sign_x), op_sign(Ybuff1, 1)); // oc8-15
          Cbuff5.mac(op_sign(vec_Xbuff1, sign_x), op_sign(Ybuff1, 1));
          Cbuff6.mac(op_sign(vec_Xbuff2, sign_x), op_sign(Ybuff1, 1));
          Cbuff7.mac(op_sign(vec_Xbuff3, sign_x), op_sign(Ybuff1, 1));
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

#ifdef DEBUG_CONV2D_TRANSPOSE
          print(Cbuff0.template to_vector<int32>(0), true, "Cbuff0_update: ");
          print(Cbuff1.template to_vector<int32>(0), true, "Cbuff1_update: ");
          print(Cbuff2.template to_vector<int32>(0), true, "Cbuff2_update: ");
          print(Cbuff3.template to_vector<int32>(0), true, "Cbuff3_update: ");
          print(Cbuff4.template to_vector<int32>(0), true, "Cbuff4_update: ");
          print(Cbuff5.template to_vector<int32>(0), true, "Cbuff5_update: ");
          print(Cbuff6.template to_vector<int32>(0), true, "Cbuff6_update: ");
          print(Cbuff7.template to_vector<int32>(0), true, "Cbuff7_update: ");
#endif
        }

// store OFM
#ifdef DEBUG_CONV2D_TRANSPOSE
      printf("pout:%d\n", p_out);
#endif
      if
        constexpr(out_mode == FULL) {
          if
            constexpr(act_type == LINEAR || act_type == RELU) {
#ifdef USE_RTP_ACT
              int out_sign = conv2d_transpose_params.out_sign;
#else
              int out_sign = 1 - act_type; // Note enum for act types are: 0,1,2
                                           // -> linear, relu, lrelu
#endif
              store_v(
                  (dtype_ofm OFM_DM_BANK *)p_out,
                  Cbuff0.template to_vector<dtype_ofm>(
                      conv2d_transpose_params.shift_out)); // oc0-7 w0-3
              p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
                  (dtype_ofm *)p_out, conv2d_transpose_params.inc_ST_0);
              store_v(
                  (dtype_ofm OFM_DM_BANK *)p_out,
                  Cbuff1.template to_vector<dtype_ofm>(
                      conv2d_transpose_params.shift_out)); // oc0-7 w4-7
              p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
                  (dtype_ofm *)p_out, conv2d_transpose_params.inc_ST_0);

              store_v(
                  (dtype_ofm OFM_DM_BANK *)p_out,
                  Cbuff2.template to_vector<dtype_ofm>(
                      conv2d_transpose_params.shift_out)); // oc0-7 w8-11
              p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
                  (dtype_ofm *)p_out, conv2d_transpose_params.inc_ST_0);
              store_v(
                  (dtype_ofm OFM_DM_BANK *)p_out,
                  Cbuff3.template to_vector<dtype_ofm>(
                      conv2d_transpose_params.shift_out)); // oc0-7 w12-15
              p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
                  (dtype_ofm *)p_out, conv2d_transpose_params.inc_ST_1);

              store_v(
                  (dtype_ofm OFM_DM_BANK *)p_out,
                  Cbuff4.template to_vector<dtype_ofm>(
                      conv2d_transpose_params.shift_out)); // oc8-15 w0-3
              p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
                  (dtype_ofm *)p_out, conv2d_transpose_params.inc_ST_0);
              store_v(
                  (dtype_ofm OFM_DM_BANK *)p_out,
                  Cbuff5.template to_vector<dtype_ofm>(
                      conv2d_transpose_params.shift_out)); // oc8-15 w4-7
              p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
                  (dtype_ofm *)p_out, conv2d_transpose_params.inc_ST_0);

              store_v(
                  (dtype_ofm OFM_DM_BANK *)p_out,
                  Cbuff6.template to_vector<dtype_ofm>(
                      conv2d_transpose_params.shift_out)); // oc8-15 w8-11
              p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
                  (dtype_ofm *)p_out, conv2d_transpose_params.inc_ST_0);
              store_v(
                  (dtype_ofm OFM_DM_BANK *)p_out,
                  Cbuff7.template to_vector<dtype_ofm>(
                      conv2d_transpose_params.shift_out)); // oc8-15 w12-15
#ifdef DEBUG_CONV2D_TRANSPOSE
              printf("pout:%d\n", p_out);
#endif
            }
          else {
            // Code/Macro for LeaKy reLU
            LEAKY_RELU_COMPUTE(Cbuff0,
                               conv2d_transpose_params.shift_lrelu_input,
                               conv2d_transpose_params.lrelu_alpha,
                               conv2d_transpose_params.shift_lrelu_alpha,
                               conv2d_transpose_params.shift_lrelu_out, p_out)
            p_out = byte_incr(p_out, conv2d_transpose_params.inc_ST_0);

            LEAKY_RELU_COMPUTE(Cbuff1,
                               conv2d_transpose_params.shift_lrelu_input,
                               conv2d_transpose_params.lrelu_alpha,
                               conv2d_transpose_params.shift_lrelu_alpha,
                               conv2d_transpose_params.shift_lrelu_out, p_out)
            p_out = byte_incr(p_out, conv2d_transpose_params.inc_ST_0);

            LEAKY_RELU_COMPUTE(Cbuff2,
                               conv2d_transpose_params.shift_lrelu_input,
                               conv2d_transpose_params.lrelu_alpha,
                               conv2d_transpose_params.shift_lrelu_alpha,
                               conv2d_transpose_params.shift_lrelu_out, p_out)
            p_out = byte_incr(p_out, conv2d_transpose_params.inc_ST_0);

            LEAKY_RELU_COMPUTE(Cbuff3,
                               conv2d_transpose_params.shift_lrelu_input,
                               conv2d_transpose_params.lrelu_alpha,
                               conv2d_transpose_params.shift_lrelu_alpha,
                               conv2d_transpose_params.shift_lrelu_out, p_out)
            p_out = byte_incr(p_out, conv2d_transpose_params.inc_ST_1);

            LEAKY_RELU_COMPUTE(Cbuff4,
                               conv2d_transpose_params.shift_lrelu_input,
                               conv2d_transpose_params.lrelu_alpha,
                               conv2d_transpose_params.shift_lrelu_alpha,
                               conv2d_transpose_params.shift_lrelu_out, p_out)
            p_out = byte_incr(p_out, conv2d_transpose_params.inc_ST_0);

            LEAKY_RELU_COMPUTE(Cbuff5,
                               conv2d_transpose_params.shift_lrelu_input,
                               conv2d_transpose_params.lrelu_alpha,
                               conv2d_transpose_params.shift_lrelu_alpha,
                               conv2d_transpose_params.shift_lrelu_out, p_out)
            p_out = byte_incr(p_out, conv2d_transpose_params.inc_ST_0);

            LEAKY_RELU_COMPUTE(Cbuff6,
                               conv2d_transpose_params.shift_lrelu_input,
                               conv2d_transpose_params.lrelu_alpha,
                               conv2d_transpose_params.shift_lrelu_alpha,
                               conv2d_transpose_params.shift_lrelu_out, p_out)
            p_out = byte_incr(p_out, conv2d_transpose_params.inc_ST_0);

            LEAKY_RELU_COMPUTE(Cbuff7,
                               conv2d_transpose_params.shift_lrelu_input,
                               conv2d_transpose_params.lrelu_alpha,
                               conv2d_transpose_params.shift_lrelu_alpha,
                               conv2d_transpose_params.shift_lrelu_out, p_out)
          }
        }
      else {
        // store partial sum
        store_v((int16 OFM_DM_BANK *)p_out,
                Cbuff0.template to_vector<int16>(
                    conv2d_transpose_params.shift_psum_out)); // oc0-7 w0-3
        p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
            (int16 *)p_out, conv2d_transpose_params.inc_ST_0);
        store_v((int16 OFM_DM_BANK *)p_out,
                Cbuff1.template to_vector<int16>(
                    conv2d_transpose_params.shift_psum_out)); // oc0-7 w4-7
        p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
            (int16 *)p_out, conv2d_transpose_params.inc_ST_0);

        store_v((int16 OFM_DM_BANK *)p_out,
                Cbuff2.template to_vector<int16>(
                    conv2d_transpose_params.shift_psum_out)); // oc0-7 w8-11
        p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
            (int16 *)p_out, conv2d_transpose_params.inc_ST_0);
        store_v((int16 OFM_DM_BANK *)p_out,
                Cbuff3.template to_vector<int16>(
                    conv2d_transpose_params.shift_psum_out)); // oc0-7 w12-15
        p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
            (int16 *)p_out, conv2d_transpose_params.inc_ST_1);

        store_v((int16 OFM_DM_BANK *)p_out,
                Cbuff4.template to_vector<int16>(
                    conv2d_transpose_params.shift_psum_out)); // oc8-15 w0-3
        p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
            (int16 *)p_out, conv2d_transpose_params.inc_ST_0);
        store_v((int16 OFM_DM_BANK *)p_out,
                Cbuff5.template to_vector<int16>(
                    conv2d_transpose_params.shift_psum_out)); // oc8-15 w4-7
        p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
            (int16 *)p_out, conv2d_transpose_params.inc_ST_0);

        store_v((int16 OFM_DM_BANK *)p_out,
                Cbuff6.template to_vector<int16>(
                    conv2d_transpose_params.shift_psum_out)); // oc8-15 w8-11
        p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
            (int16 *)p_out, conv2d_transpose_params.inc_ST_0);
        store_v((int16 OFM_DM_BANK *)p_out,
                Cbuff7.template to_vector<int16>(
                    conv2d_transpose_params.shift_psum_out)); // oc8-15 w12-15
#ifdef DEBUG_CONV2D_TRANSPOSE
        printf("pout:%d\n", p_out);
#endif
      }

#ifdef DEBUG_CONV2D_TRANSPOSE
      printf("\n");
      print(Cbuff0.template to_vector<int16>(0), true, "Cbuff0_post: ");
      print(Cbuff1.template to_vector<int16>(0), true, "Cbuff1_post: ");
      print(Cbuff2.template to_vector<int16>(0), true, "Cbuff2_post: ");
      print(Cbuff3.template to_vector<int16>(0), true, "Cbuff3_post: ");
      print(Cbuff4.template to_vector<int16>(0), true, "Cbuff4_post: ");
      print(Cbuff5.template to_vector<int16>(0), true, "Cbuff5_post: ");
      print(Cbuff6.template to_vector<int16>(0), true, "Cbuff6_post: ");
      print(Cbuff7.template to_vector<int16>(0), true, "Cbuff7_post: ");
      print(Cbuff0.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff0_post_shift: ");
      print(Cbuff1.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff1_post_shift: ");
      print(Cbuff2.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff2_post_shift: ");
      print(Cbuff3.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff3_post_shift: ");
      print(Cbuff4.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff4_post_shift: ");
      print(Cbuff5.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff5_post_shift: ");
      print(Cbuff6.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff6_post_shift: ");
      print(Cbuff7.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff7_post_shift: ");
#endif

      // update addr for outer loop
      p_in = INCR_ITR3D_PARAM(p_in, iterator_outer0, conv2d_transpose_params)
      p_in = INCR_ITR2D_PARAM(p_in, iterator_outer1, conv2d_transpose_params)
      p_w = INCR_ITR3D_PARAM(p_w, iterator_weights, conv2d_transpose_params)
      p_init16 = INCR_ITR3D_PARAM(p_init16, iterator_pinit, conv2d_transpose_params) 
      p_out = INCR_ITR3D_PARAM(p_out, iterator_pout, conv2d_transpose_params)
      
      // update cnt for x and y
      dtype_ofm OFM_DM_BANK *p1;
      add_3d_byte(p1,0,conv2d_transpose_params.num_X_Co,Co_cnt,0,conv2d_transpose_params.num_Y,y_cnt,0);

#ifdef DEBUG_CONV2D_TRANSPOSE
      printf("p_in:%d\n", p_in);
      printf("p_w:%d\n", p_w);
      printf("pinit:%d\n", p_init16);
      printf("pout:%d\n", p_out);
#endif
    }

  if
    constexpr(out_mode == FULL) {
#if RELU_INT8_ENABLE == 1
#ifdef USE_RTP_ACT
      relu_int8_post_process(output, conv2d_transpose_params.out_sign,
                             conv2d_transpose_params.ofm_len);
#else
      int act = 1 - act_type;
      relu_int8_post_process(output, act, conv2d_transpose_params.ofm_len);
#endif
#endif
    }
}
#endif

#if __AIE_ARCH__ == 21 // AIE2p conv2d_transpose_generic
template <uint8_t batch_size, act_t act_type, out_mode_t out_mode,
          typename dtype_ifm, typename dtype_wts, typename dtype_ofm>
INLINE void
conv2d_transpose_generic(int *__restrict input, int *__restrict weights,
                         int *__restrict output,
                         conv2d_transpose_params_t &conv2d_transpose_params) {

  // initialize p_sum and bias
  int16 OFM_DM_BANK *p_sum = (int16 OFM_DM_BANK *)output +
                             (conv2d_transpose_params.psum_buff_offset >> 1);
  int16 OFM_DM_BANK *p_bias =
      (int16 OFM_DM_BANK *)(weights) + (LAYER_PARAM_BYTES >> 1);
  int16 OFM_DM_BANK *p_init16 =
      (conv2d_transpose_params.psum_in == 0) ? p_bias : p_sum;

  // initialize ifm, wts and ofm
  dtype_ifm IFM_DM_BANK *p_in = (dtype_ifm IFM_DM_BANK *)(input) + 
                                conv2d_transpose_params.ifm_offset;
  dtype_wts WT_DM_BANK *p_w = (dtype_wts WT_DM_BANK *)(weights) +
                              conv2d_transpose_params.hdr_len +
                              conv2d_transpose_params.wts_offset;
  dtype_ofm OFM_DM_BANK *p_out = (conv2d_transpose_params.psum_out == 0) ? 
                                (dtype_ofm OFM_DM_BANK *)output : 
                                (dtype_ofm OFM_DM_BANK *)p_sum;

  vector<dtype_ifm, 64> vec_Xbuff0, vec_Xbuff1;
  vector<dtype_wts, 64> Ybuff0, Ybuff1;
  group_data_t<uint8_t, 64> Xbuff;
  vector<uint8_t, 64> zero_vec = zeros<uint8_t,64>();
  using mmul = mmul<8, 8, 8, dtype_ifm, dtype_wts>;

// input: mmul type Cbuff
// shift_input: Intial shift applied to convert accum<acc32,32> to vec<int16,16>
// alpha: This is the scaled value to be multiplied
// shift_alpha: shift required to remove the scaling factor of alpha
// shift_out: shift applied before saving to final output
// output_ptr: pointer to the location where data is to be saved
#define LEAKY_RELU_COMPUTE(input, shift_input, alpha, shift_alpha, shift_out,  \
                           output_ptr)                                         \
  {                                                                            \
    vector<int16, 64> input_vec =                                              \
        input.template to_vector<int16>(shift_input);                          \
    vector<int16, 32> input_vec0 =                                             \
        input_vec.template extract<32>(0);                                     \
    vector<int16, 32> input_vec1 =                                             \
        input_vec.template extract<32>(1);                                     \
    accum<acc32, 32> acc0 = mul(input_vec0, alpha);                            \
    accum<acc32, 32> acc1 = mul(input_vec1, alpha);                            \
    vector<int16, 32> alpha_vec0 = acc0.to_vector<int16>(shift_alpha);         \
    vector<int16, 32> alpha_vec1 = acc1.to_vector<int16>(shift_alpha);         \
    vector<int16, 32> result_vec0 = max(input_vec0, alpha_vec0);               \
    vector<int16, 32> result_vec1 = max(input_vec1, alpha_vec1);               \
    acc0.from_vector(result_vec0, 0);                                          \
    acc1.from_vector(result_vec1, 0);                                          \
    mmul Cbuff(concat(acc0, acc1));                                            \
    store_v(output_ptr, Cbuff.template to_vector<int8>(shift_out));            \
  }

  // Following counters are needed for ADD_2D/3D_BYTE intrinsics
  // SETUP_ITR2D_COUNTERS(iterator_name) / SETUP_ITR3D_COUNTERS(iterator_name)
  // type 1: 2D iterator, 2: 3d Iterator
  // e.g. SETUP_ITR3D_COUNTERS(iterator_inner)
  // Resulting count variable is : iterator_inner_cnt0, iterator_inner_cnt_1
  SETUP_ITR3D_COUNTERS(iterator_inner0)
  SETUP_ITR3D_COUNTERS(iterator_inner1)
  SETUP_ITR3D_COUNTERS(iterator_outer0)
  SETUP_ITR2D_COUNTERS(iterator_outer1)
  SETUP_ITR3D_COUNTERS(iterator_weights)
  SETUP_ITR3D_COUNTERS(iterator_pinit)
  SETUP_ITR3D_COUNTERS(iterator_pout)

  addr_t y_cnt = 0;
  addr_t Co_cnt = 0;
  addr_t x_cnt = 0;
  addr_t Ci_cnt = 0;
  addr_t Ky_cnt = 0;
  addr_t Kx_cnt = 0;

#ifdef DEBUG_CONV2D_TRANSPOSE
for(int i =0; i< 16;i++){
  print(CT_LOAD_V<32>((uint8 IFM_DM_BANK *)p_in), true, "test: ");
  printf("addr:%d\n",p_in);
  p_in = byte_incr(p_in, 32);
}
p_in = byte_incr(p_in, -32*16);
#endif

#ifdef __X86SIM__
  p_in += 31;
#else
  p_in += 63;
#endif

  AIE_PREPARE_FOR_PIPELINING 
  AIE_LOOP_RANGE(2, ) 
  for (unsigned j = 0; j < conv2d_transpose_params.outer_g; j++){
      aie::accum<acc32, 32> acc_0;
      aie::accum<acc32, 32> acc_1;

#ifdef DEBUG_CONV2D_TRANSPOSE
    printf("curr_depth_iter:%d, num_ifm_depth_iter:%d, psum_in:%d, out_mode:%d\n", 
              conv2d_transpose_params.curr_depth_iter, conv2d_transpose_params.num_ifm_depth_iter, 
              conv2d_transpose_params.psum_in, out_mode);
      printf("pinit:%d\n", p_init16);
#endif

      acc_0.from_vector(CT_LOAD_V<32>(p_init16), conv2d_transpose_params.shift_psum_in);
      p_init16 = byte_incr(p_init16, conv2d_transpose_params.inc_S_0);
      acc_1.from_vector(CT_LOAD_V<32>(p_init16), conv2d_transpose_params.shift_psum_in);
      p_init16 = byte_incr(p_init16, conv2d_transpose_params.inc_S_0);
      mmul Cbuff0(concat(acc_0, acc_1));

      acc_0.from_vector(CT_LOAD_V<32>(p_init16), conv2d_transpose_params.shift_psum_in);
      p_init16 = byte_incr(p_init16, conv2d_transpose_params.inc_S_0);
      acc_1.from_vector(CT_LOAD_V<32>(p_init16), conv2d_transpose_params.shift_psum_in);
      p_init16 = byte_incr(p_init16, conv2d_transpose_params.inc_S_1);
      mmul Cbuff1(concat(acc_0, acc_1));

      acc_0.from_vector(CT_LOAD_V<32>(p_init16), conv2d_transpose_params.shift_psum_in);
      p_init16 = byte_incr(p_init16, conv2d_transpose_params.inc_S_0);
      acc_1.from_vector(CT_LOAD_V<32>(p_init16), conv2d_transpose_params.shift_psum_in);
      p_init16 = byte_incr(p_init16, conv2d_transpose_params.inc_S_0);
      mmul Cbuff2(concat(acc_0, acc_1));

      acc_0.from_vector(CT_LOAD_V<32>(p_init16), conv2d_transpose_params.shift_psum_in);
      p_init16 = byte_incr(p_init16, conv2d_transpose_params.inc_S_0);
      acc_1.from_vector(CT_LOAD_V<32>(p_init16), conv2d_transpose_params.shift_psum_in);
      mmul Cbuff3(concat(acc_0, acc_1));

#ifdef DEBUG_CONV2D_TRANSPOSE
      print(Cbuff0.template to_vector<int16>(0), true, "Cbuff0_pre: ");
      print(Cbuff1.template to_vector<int16>(0), true, "Cbuff1_pre: ");
      print(Cbuff2.template to_vector<int16>(0), true, "Cbuff2_pre: ");
      print(Cbuff3.template to_vector<int16>(0), true, "Cbuff3_pre: ");
#endif

      AIE_PREPARE_FOR_PIPELINING 
      AIE_LOOP_RANGE(4, ) 
      for (int i = 0; i < conv2d_transpose_params.inner_g; i++){
          vector<uint8_t, 64> Sbuff0, Sbuff1, Xbuff0, Xbuff1;
          // Load IFM data, two 4x8 matrices plus additional adjacent data
          // needed for shift alignment

          // shift params
          int pdel = (long)p_in;
          bool modKx = Kx_cnt & conv2d_transpose_params.stride_bits;
          bool modKy = (y_cnt+conv2d_transpose_params.Ky-Ky_cnt)&1;
#ifdef __X86SIM__      
          uint8_t frac = modKx ? 0 : ((pdel & 31) + 33);
#else
          uint8_t frac = modKx ? 0 : ((pdel & 63) + 1);
#endif         
          uint8_t step_align =  modKx ? 0 : conv2d_transpose_params.step_align;
          uint8_t shiftKx= modKx ? 0 : 8;
          uint8_t shiftKy= modKy ? 64 : 0;
          uint8_t stride_shift = (conv2d_transpose_params.stride_bits == 0) ? 64:(shiftKy - shiftKx);

#ifdef DEBUG_CONV2D_TRANSPOSE
          printf("\nouter_loop:%d, inner_loop:%d, step_align:%d, frac:%d\n", j,
                 i, conv2d_transpose_params.step_align, frac);
          printf("pin:%d\n", p_in);
#endif
          Sbuff0 = CT_LOAD_V<64>((uint8 IFM_DM_BANK *)p_in);
          p_in = byte_incr(p_in, conv2d_transpose_params.inc_A_0);
          Sbuff1 = CT_LOAD_V<64>((uint8 IFM_DM_BANK *)p_in);
          p_in = INCR_ITR3D_PARAM(p_in, iterator_inner0, conv2d_transpose_params)
          p_in = INCR_ITR3D_PARAM(p_in, iterator_inner1, conv2d_transpose_params)

#ifdef DEBUG_CONV2D_TRANSPOSE
              printf("pin:%d\n", p_in);
#endif

          Xbuff.x0 = shiftx(Xbuff.x0, Sbuff0, step_align, frac);
          Xbuff.x1 = shiftx(Xbuff.x1, Sbuff1, step_align, frac);
          
          Xbuff0 = shuffle(Xbuff.x0, zero_vec, conv2d_transpose_params.shuffle[0]);
          Xbuff0 = shuffle(Xbuff0, zero_vec, conv2d_transpose_params.shuffle[1]);
          Xbuff0 = shift(zero_vec, Xbuff0, stride_shift);

          Xbuff1 = shuffle(Xbuff.x1, zero_vec, conv2d_transpose_params.shuffle[2]);
          Xbuff1 = shuffle(Xbuff1, zero_vec, conv2d_transpose_params.shuffle[3]);
          Xbuff1 = shift(zero_vec, Xbuff1, stride_shift);
             
          vec_Xbuff0 = Xbuff0.cast_to<dtype_ifm>();
          vec_Xbuff1 = Xbuff1.cast_to<dtype_ifm>();

#ifdef DEBUG_CONV2D_TRANSPOSE
             printf("step_align:%d, frac:%d, shift:%d, shiftKx:%d, shiftKy:%d, y:%d, ky:%d, kx:%d, shfl0:%d, shfl1:%d, shfl2:%d, shfl3:%d\n",
                  conv2d_transpose_params.step_align, frac, stride_shift, shiftKx, shiftKy, y_cnt, Ky_cnt, Kx_cnt,
                  conv2d_transpose_params.shuffle[0],
                  conv2d_transpose_params.shuffle[1],
                  conv2d_transpose_params.shuffle[2],
                  conv2d_transpose_params.shuffle[3]);

              print(Sbuff0, true, "Sbuff0: ");
              print(Sbuff1, true, "Sbuff1: ");
              print(Xbuff.x0, true, "Xbuff.x0: ");
              print(Xbuff.x1, true, "Xbuff.x1: ");
              print(Xbuff0, true, "Xbuff0: ");
              print(Xbuff1, true, "Xbuff1: ");
#endif

          // update kx and ky cnt
          dtype_ifm IFM_DM_BANK *p0;
          add_3d_byte(p0,0,conv2d_transpose_params.num_Kx,Kx_cnt,0,conv2d_transpose_params.num_Ky,Ky_cnt,0);

#ifdef DEBUG_CONV2D_TRANSPOSE
          print(vec_Xbuff0, true, "vec_Xbuff0: ");
          print(vec_Xbuff1, true, "vec_Xbuff1: ");
#endif

          // Load weights -- two matrices 8*8
          Ybuff0 = CT_LOAD_V<64>((int8 IFM_DM_BANK *)p_w);
          p_w = byte_incr(p_w, 64);
          Ybuff1 = CT_LOAD_V<64>((int8 IFM_DM_BANK *)p_w);
          p_w = byte_incr(p_w, -192);

#ifdef DEBUG_CONV2D_TRANSPOSE
          printf("p_w:%d\n", p_w);
          print(Ybuff0, true, "vec_Ybuff0: ");
          print(Ybuff1, true, "vec_Ybuff1: ");
#endif

#ifdef USE_RTP_SIGN
          int sign_x = conv2d_transpose_params.ifm_sign;
          Cbuff0.mac(op_sign(vec_Xbuff0, sign_x), op_sign(Ybuff0, 1));
          Cbuff1.mac(op_sign(vec_Xbuff1, sign_x), op_sign(Ybuff0, 1));
          Cbuff2.mac(op_sign(vec_Xbuff0, sign_x), op_sign(Ybuff1, 1));
          Cbuff3.mac(op_sign(vec_Xbuff1, sign_x), op_sign(Ybuff1, 1));
#else
          Cbuff0.mac(vec_Xbuff0, Ybuff0);
          Cbuff1.mac(vec_Xbuff1, Ybuff0);
          Cbuff2.mac(vec_Xbuff0, Ybuff1);
          Cbuff3.mac(vec_Xbuff1, Ybuff1);
#endif

#ifdef DEBUG_CONV2D_TRANSPOSE
          print(Cbuff0.template to_vector<int16>(0), true, "Cbuff0_update: ");
          print(Cbuff1.template to_vector<int16>(0), true, "Cbuff1_update: ");
          print(Cbuff2.template to_vector<int16>(0), true, "Cbuff2_update: ");
          print(Cbuff3.template to_vector<int16>(0), true, "Cbuff3_update: ");
#endif
        }

      // store OFM
#ifdef DEBUG_CONV2D_TRANSPOSE
      printf("pout:%d\n", p_out);
#endif

    if constexpr(out_mode == FULL) {
        if
          constexpr(act_type == LINEAR || act_type == RELU) {
#ifdef USE_RTP_ACT
            int out_sign = conv2d_transpose_params.out_sign;
#else
            int out_sign = 1 - act_type; // Note enum for act types are: 0,1,2
                                         // -> linear, relu, lrelu
#endif

            store_v(p_out, Cbuff0.template to_vector<dtype_ofm>(conv2d_transpose_params.shift_out));//w0-7 Oc0-7
            p_out = byte_incr(p_out, conv2d_transpose_params.inc_ST_0);
            
            store_v(p_out, Cbuff1.template to_vector<dtype_ofm>(conv2d_transpose_params.shift_out));//w8-15 Oc0-7
            p_out = byte_incr(p_out, conv2d_transpose_params.inc_ST_1);

            store_v(p_out, Cbuff2.template to_vector<dtype_ofm>(conv2d_transpose_params.shift_out));//w0-7 Oc8-15
            p_out = byte_incr(p_out, conv2d_transpose_params.inc_ST_0);

            store_v(p_out, Cbuff3.template to_vector<dtype_ofm>(conv2d_transpose_params.shift_out));//w8-15 Oc8-15
#ifdef DEBUG_CONV2D_TRANSPOSE
      printf("pout:%d\n", p_out);
#endif

          }
        else {
          LEAKY_RELU_COMPUTE(Cbuff0, conv2d_transpose_params.shift_lrelu_input,
                             conv2d_transpose_params.lrelu_alpha,
                             conv2d_transpose_params.shift_lrelu_alpha,
                             conv2d_transpose_params.shift_lrelu_out, p_out)
          p_out = byte_incr(p_out, conv2d_transpose_params.inc_ST_0);

          LEAKY_RELU_COMPUTE(Cbuff1, conv2d_transpose_params.shift_lrelu_input,
                             conv2d_transpose_params.lrelu_alpha,
                             conv2d_transpose_params.shift_lrelu_alpha,
                             conv2d_transpose_params.shift_lrelu_out, p_out)
          p_out = byte_incr(p_out, conv2d_transpose_params.inc_ST_1);

          LEAKY_RELU_COMPUTE(Cbuff2, conv2d_transpose_params.shift_lrelu_input,
                             conv2d_transpose_params.lrelu_alpha,
                             conv2d_transpose_params.shift_lrelu_alpha,
                             conv2d_transpose_params.shift_lrelu_out, p_out)
          p_out = byte_incr(p_out, conv2d_transpose_params.inc_ST_0);

          LEAKY_RELU_COMPUTE(Cbuff3, conv2d_transpose_params.shift_lrelu_input,
                             conv2d_transpose_params.lrelu_alpha,
                             conv2d_transpose_params.shift_lrelu_alpha,
                             conv2d_transpose_params.shift_lrelu_out, p_out)
        }
      }
      else {
#ifdef DEBUG_CONV2D_TRANSPOSE
      printf("pout:%d\n", p_out);
#endif
          // Store partial sum
        store_v((int16 OFM_DM_BANK *)p_out,
                Cbuff0.template to_vector<int16>(
                    conv2d_transpose_params.shift_psum_out));
        p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
            (int16 *)p_out, conv2d_transpose_params.inc_ST_0);

        store_v((int16 OFM_DM_BANK *)p_out,
                Cbuff1.template to_vector<int16>(
                    conv2d_transpose_params.shift_psum_out));
        p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
            (int16 *)p_out, conv2d_transpose_params.inc_ST_1);

        store_v((int16 OFM_DM_BANK *)p_out,
                Cbuff2.template to_vector<int16>(
                    conv2d_transpose_params.shift_psum_out));
        p_out = (dtype_ofm OFM_DM_BANK *)byte_incr(
            (int16 *)p_out, conv2d_transpose_params.inc_ST_0);

        store_v((int16 OFM_DM_BANK *)p_out,
                Cbuff3.template to_vector<int16>(
                    conv2d_transpose_params.shift_psum_out));

#ifdef DEBUG_CONV2D_TRANSPOSE
      printf("pout:%d\n", p_out);
#endif
      }
      
#ifdef DEBUG_CONV2D_TRANSPOSE
      print(Cbuff0.template to_vector<int16>(0), true, "Cbuff0_post: ");
      print(Cbuff1.template to_vector<int16>(0), true, "Cbuff1_post: ");
      print(Cbuff2.template to_vector<int16>(0), true, "Cbuff2_post: ");
      print(Cbuff3.template to_vector<int16>(0), true, "Cbuff3_post: ");
      print(Cbuff0.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff0_post_shift: ");
      print(Cbuff1.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff1_post_shift: ");
      print(Cbuff2.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff2_post_shift: ");
      print(Cbuff3.template to_vector<dtype_ofm>(
                conv2d_transpose_params.shift_out),
            true, "Cbuff3_post_shift: ");
#endif

      // update addr
      p_in = INCR_ITR3D_PARAM(p_in, iterator_outer0, conv2d_transpose_params)
      p_in = INCR_ITR2D_PARAM(p_in, iterator_outer1, conv2d_transpose_params)
      p_w = INCR_ITR3D_PARAM(p_w, iterator_weights, conv2d_transpose_params)
      p_init16 = INCR_ITR3D_PARAM(p_init16, iterator_pinit, conv2d_transpose_params)
      p_out = INCR_ITR3D_PARAM(p_out, iterator_pout, conv2d_transpose_params)

      // update cnt for x and y
      dtype_ofm OFM_DM_BANK *p1;
      add_3d_byte(p1,0,conv2d_transpose_params.num_X_Co,Co_cnt,0,conv2d_transpose_params.num_Y,y_cnt,0);
    
#ifdef DEBUG_CONV2D_TRANSPOSE
      printf("p_in:%d\n", p_in);
      printf("p_w:%d\n", p_w);
      printf("p_init16:%d\n", p_init16);
      printf("pout:%d\n", p_out);
#endif

    }

#ifdef DEBUG_CONV2D_TRANSPOSE
p_out = (dtype_ofm OFM_DM_BANK *)output;
for(int i =0; i< 16;i++){
  print(CT_LOAD_V<32>((dtype_ofm OFM_DM_BANK *)p_out), true, "test: ");
  printf("addr:%d\n",p_out);
  p_out = byte_incr(p_out, 32);
}
#endif

  if
    constexpr(out_mode == FULL) {
#if RELU_INT8_ENABLE == 1
#ifdef USE_RTP_ACT
      relu_int8_post_process(output, conv2d_transpose_params.out_sign,
                             conv2d_transpose_params.ofm_len);
#else
      int act = 1 - act_type;
      relu_int8_post_process(output, act, conv2d_transpose_params.ofm_len);
#endif
#endif
    }
}
#endif

template <uint8_t batch_size, act_t act_type, typename dtype_ifm,
          typename dtype_wts, typename dtype_ofm, typename config_ifm,
          typename config_wts, typename config_ofm>
void conv2d_transpose(
    adf::input_buffer_conf<dtype_ifm, config_ifm> &__restrict ifm,
    adf::input_buffer_conf<dtype_wts, config_wts> &__restrict wts,
    adf::output_buffer_conf<dtype_ofm, config_ofm> &ofm,
    conv2d_transpose_params_t &conv2d_transpose_params) {

  // Output modes
  const out_mode_t out_mode_full = FULL;
  const out_mode_t out_mode_partial = PARTIAL;

  // Setup iteration level parameters like IO mode etc
  setup_conv2d_transpose_iter_params(conv2d_transpose_params);

  // Extract low level pointers used by the actual kernel code
  int *input = (int *)ifm.data();
  int *weights = (int *)wts.data();
  int *output = (int *)ofm.data();

  switch(conv2d_transpose_params.exec_type) {
    case 0:
    case 1:
    conv2d_transpose_generic<batch_size, act_type, out_mode_full, dtype_ifm,
                             dtype_wts, dtype_ofm>(input, weights, output,
                                                 conv2d_transpose_params);
    break;
    case 2:
    case 3:
    conv2d_transpose_generic<batch_size, act_type, out_mode_partial, dtype_ifm,
                             dtype_wts, dtype_ofm>(input, weights, output,
                                                 conv2d_transpose_params);
    break;
  }
}

#endif //__CONV2D_TRANSPOSE_H__
