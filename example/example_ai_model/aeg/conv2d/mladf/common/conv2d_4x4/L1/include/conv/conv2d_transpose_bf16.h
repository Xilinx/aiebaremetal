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
 * 2D Transposed Convolution Kernel for AI Engine
 *
 * Performs 2d convolution on data present in "ifms" using weights from "wts"
 * Result is stored in "ofms".
 *
 */

#ifndef __CONV2D_TRANSPOSE_BF16_H__
#define __CONV2D_TRANSPOSE_BF16_H__

#include "aie.hpp"
#include "aie_api/utils.hpp" // needed for aie:print
#include <adf.h>

#include "conv2d_transpose_bf16_params.h" // Parameters like increments and wraps needed by kernel
#include "mllib_const.h" // Constants shared across graphs like activation type
#include "utils.h" // Contains some utility functions and definitions related to intrinsics usage

using namespace aie;

#ifdef __X86SIM__
#define CT_BF16_LOAD_V CT_bf16_load_v
//#define DEBUG_CONV2D_TRANSPOSE_BF16 true
// #define DEBUG_CONV2D_TRANSPOSE_ADDR true

#else
#define CT_BF16_LOAD_V aie::load_v
#endif

 // ==================================================
 // ********************************************* //
 //   Bfloat16 version apply dense mothed.        //
 // ********************************************* //
 // ==================================================

#if __AIE_ARCH__ == 20

template <int N>
__attribute__((always_inline)) aie::vector<bfloat16, N> CT_bf16_load_v(bfloat16 *ptr) {
#ifdef __X86SIM__
  aie::vector<bfloat16, N> res;

  int tmp_val = (long)ptr;
  tmp_val %= 32;//32bytes align
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
__attribute__((always_inline)) aie::vector<float, N> CT_bf16_load_v(float *ptr) {
#ifdef __X86SIM__
  aie::vector<float, N> res;

  int tmp_val = (long)ptr;
  tmp_val %= 32;//32bytes align
  ptr -= (tmp_val / 2);

  for (int i = 0; i < N; i++) {
    res[i] = *(ptr + i);
  }

  return res;
#else
  return aie::load_v<N>(ptr);
#endif
}

/////////////////////////////////////////////////////////
#else // aie2p/aie2ps

template <int N>
__attribute__((always_inline)) aie::vector<bfloat16, N> CT_bf16_load_v(bfloat16 *ptr) {
#ifdef __X86SIM__
  aie::vector<bfloat16, N> res;

  int tmp_val = (long)ptr;
  tmp_val %= 32;//64bytes align
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
__attribute__((always_inline)) aie::vector<float, N> CT_bf16_load_v(float *ptr) {
#ifdef __X86SIM__
  aie::vector<float, N> res;

  int tmp_val = (long)ptr;
  tmp_val %= 32;//64bytes align
  ptr -= (tmp_val / 2);

  for (int i = 0; i < N; i++) {
    res[i] = *(ptr + i);
  }

  return res;
#else
  return aie::load_v<N>(ptr);
#endif
}

#endif // end __AIE_ARCH__


#if __AIE_ARCH__ == 20  // bfloat16 aie2
// Design
// 1. psum layout w4 x co8 to avoid unaligned load
// 2. inner loop w_g -> h_g -> ci_g
// 3. outer loop kx -> ky -> co_g
template <uint8_t batch_size, act_t act_type, //out_mode_t out_mode,
          typename dtype_ifm, typename dtype_wts, typename dtype_ofm>
INLINE void
conv2d_transpose_bf16_generic(int *__restrict input, int *__restrict weights,
                         int *__restrict output,
                         conv2d_transpose_bf16_params_t &param) {

  // initialize p_psum and bias
  float OFM_DM_BANK *p_psum = (float OFM_DM_BANK *)output +
                             (param.psum_buff_offset);

  bfloat16 WT_DM_BANK *__restrict p_bias_in = (bfloat16 WT_DM_BANK *__restrict)(weights) +
                                               (LAYER_PARAM_BYTES >> 1);//bytes to bf16

  float * __restrict p_psum_in = (float * __restrict) p_psum;
  float * __restrict p_psum_out = (float * __restrict) p_psum;
  float * __restrict p_psum_dup = (float * __restrict) p_psum;  

  // initialize ifm, wts and ofm
  dtype_ifm IFM_DM_BANK *p_in = (dtype_ifm IFM_DM_BANK *)(input) + 
                                param.ifm_offset;
  dtype_wts WT_DM_BANK *p_w = (dtype_wts WT_DM_BANK *)(weights) +
                              param.hdr_len +
                              param.wts_offset;
  dtype_ofm OFM_DM_BANK * __restrict p_out = (param.psum_out == 0) ? 
                                (dtype_ofm OFM_DM_BANK * __restrict)output : 
                                (dtype_ofm OFM_DM_BANK * __restrict)p_psum;
#ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
      printf("[check] p_psum:%ld, p_out:%ld , output:%ld, param.psum_buff_offset:%d\n",(long)p_psum, (long)p_out, (long)output, param.psum_buff_offset);
      printf("[check] p_in:%d, weights:%d, param.hdr_len:%d, wts_offset:%d\n", (long)p_in, (long)weights, param.hdr_len, param.wts_offset);
#endif

  using mmul = mmul<4, 8, 4, bfloat16, bfloat16>;


  #define CONVT_LOAD_DATA(ptr, buff, incr) \
  {\
      buff.insert(0, CT_BF16_LOAD_V<16>(ptr));ptr=byte_incr(ptr, 16*sizeof(bfloat16));\
      buff.insert(1, CT_BF16_LOAD_V<16>(ptr));ptr=byte_incr(ptr, incr);\
  }
  //*********************************************//
  //Load psum data w2c8 with stride.             //
  //*********************************************//
  #define LOAD_FLOAT_DATA_WITH_STRIDE(ptr, buff, incr) \
  {\
      buff = insert(buff, 0, CT_BF16_LOAD_V<8>(ptr));ptr=byte_incr(ptr, 8*sizeof(float)*param.stride);\
      buff = insert(buff, 1, CT_BF16_LOAD_V<8>(ptr));ptr=byte_incr(ptr, incr); \
  }   //print(buff, true, "C0_pre: ");\

  //*********************************************//
  //Load psum data w2c8.                         //
  //*********************************************//
  #define LOAD_FLOAT_DATA(ptr, buff, incr) \
  {\
      buff = insert(buff, 0, CT_BF16_LOAD_V<8>(ptr));ptr=byte_incr(ptr, 8*sizeof(float));\
      buff = insert(buff, 1, CT_BF16_LOAD_V<8>(ptr));ptr=byte_incr(ptr, incr);\
  }   //print(buff, true, "C0_pre: ");\

  //*********************************************//
  //Load psum data and shuffle to w4c4.          //
  //*********************************************//
  //where acc is load, acc will be store
  //psum layout is fit for calculation
  #define CONVT_LOAD_16_ACC_DATA(p_psum_in, iterator_psum_in_inner, param, C0_acc, C1_acc, C2_acc, C3_acc)\
  {\
      vector<float, 16> C0in_w2c8, C1in_w2c8, C2in_w2c8, C3in_w2c8;\
      vector<float, 16> C0_v, C1_v, C2_v, C3_v;\
      LOAD_FLOAT_DATA_WITH_STRIDE(p_psum_in, C0in_w2c8, param.psum_inc0);\
      LOAD_FLOAT_DATA_WITH_STRIDE(p_psum_in, C1in_w2c8, param.psum_inc1);\
      LOAD_FLOAT_DATA_WITH_STRIDE(p_psum_in, C2in_w2c8, param.psum_inc0);\
      LOAD_FLOAT_DATA_WITH_STRIDE(p_psum_in, C3in_w2c8, 0);\
      std::tie(C0_v, C1_v) = interleave_unzip(C0in_w2c8, C1in_w2c8, 4);\
      std::tie(C2_v, C3_v) = interleave_unzip(C2in_w2c8, C3in_w2c8, 4);\
      C0_acc.from_vector(C0_v); C1_acc.from_vector(C1_v);\
      C2_acc.from_vector(C2_v); C3_acc.from_vector(C3_v);\
      p_psum_in = INCR_ITR3D_PARAM(p_psum_in, iterator_psum_in_inner, param)\
  }

  //shuffle w4co4 to w4co0~7 & w4co8~15
  #define SHUFFLE_4_FULL_DATA(C0, C1, C2, C3, C0_w2c8, C1_w2c8, C2_w2c8, C3_w2c8)\
  {\
    std::tie(C0_w2c8, C1_w2c8) = interleave_zip(C0.template to_vector<float>(), C1.template to_vector<float>(), 4);\
    std::tie(C2_w2c8, C3_w2c8) = interleave_zip(C2.template to_vector<float>(), C3.template to_vector<float>(), 4);\
  }


// TODO:(yunlei) leaky relu
// #define LEAKY_RELU_COMPUTE(input, shift_input, alpha, shift_alpha, shift_out,  \
//                            output_ptr)                                         \
//   {                                                                            \
//     vector<int16, 32> input_vec =                                              \
//         input.template to_vector<int16>(shift_input);                          \
//     accum<acc32, 32> acc =                                                     \
//         mul<acc32, vector<int16, 32>, int16>(input_vec, alpha);                \
//     vector<int16, 32> alpha_vec = acc.to_vector<int16>(shift_alpha);           \
//     vector<int16, 32> result_vec =                                             \
//         max<vector<int16, 32>, vector<int16, 32>>(input_vec, alpha_vec);       \
//     acc.from_vector(result_vec, 0);                                            \
//     store_v(output_ptr, acc.to_vector<int8>(shift_out));                       \
//   }

  // Following counters are needed for ADD_2D/3D_BYTE intrinsics
  // SETUP_ITR2D_COUNTERS(iterator_name) / SETUP_ITR3D_COUNTERS(iterator_name)
  // type 1: 2D iterator, 2: 3d Iterator
  // e.g. SETUP_ITR3D_COUNTERS(iterator_inner)
  // Resulting count variable is : iterator_inner_cnt0, iterator_inner_cnt_1
  SETUP_ITR3D_COUNTERS(iterator_ifm_inner)
  SETUP_ITR3D_COUNTERS(iterator_wts_inner)
  SETUP_ITR3D_COUNTERS(iterator_wts_outer)
  SETUP_ITR3D_COUNTERS(iterator_psum_inner)
  SETUP_ITR3D_COUNTERS(iterator_psum_outer)
  SETUP_ITR3D_COUNTERS(iterator_bias)
  SETUP_ITR3D_COUNTERS(iterator_psum_in_inner)
  SETUP_ITR3D_COUNTERS(iterator_psum_in_outer)

#ifdef DEBUG_CONV2D_TRANSPOSE_BF16
for(int i =0; i< 4;i++){
print(CT_BF16_LOAD_V<16>((bfloat16 IFM_DM_BANK *)p_in), true, "test: ");
printf("addr:%d\n", (long)p_in);
p_in = byte_incr(p_in, 16*2);
}
p_in = byte_incr(p_in, -16*4*2);
#endif

// initialization of psum zone
int32_t zero_psum = 1 - param.psum_in;
auto vec_init = aie::broadcast<int32_t, 32>(0);
int32_t *p_psum_init = (int32_t *)p_psum_in;
if(zero_psum == 1) {
  AIE_PREPARE_FOR_PIPELINING 
  AIE_LOOP_RANGE(4, ) 
  for(unsigned j=0; j<param.pout_inc2; j++) {
      aie::store_v((int32_t *)p_psum_init, vec_init);
      p_psum_init += 32;
  }
}

dtype_ifm IFM_DM_BANK *p_in_org = p_in;

  AIE_PREPARE_FOR_PIPELINING 
  AIE_LOOP_RANGE(4, ) 
  for (unsigned j = 0; j < param.outer_g; j++) {
      // Load psum, load ifm, load wts, mac, store psum
      AIE_PREPARE_FOR_PIPELINING 
      AIE_LOOP_RANGE(4, ) 
      for (int i = 0; i < param.inner_g; i++){
          #ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
                    printf("\nouter_loop:%d, inner_loop:%d\n", j, i);
                    printf("curr_depth_iter:%d, num_ifm_depth_iter:%d, psum_in:%d, out_mode:%d\n", 
                        param.curr_depth_iter, param.num_ifm_depth_iter, 
                        param.psum_in, param.exec_type);
                    printf("psum_init32:%d\n", (long)p_psum_in);
                    printf("p_in:%d\n", p_in);
                    printf("p_w:%d\n", p_w);
          #endif

          vector<bfloat16,32> X0, X1, Y0, Y1, Y2, Y3;
          accum<accfloat,16> C0_acc, C1_acc, C2_acc, C3_acc;
          // 1. Load IFM data -- 4width * 8ich
          if constexpr(batch_size < 4) {
          
              CONVT_LOAD_DATA(p_in, X0, 16*2);
              p_in = INCR_ITR3D_PARAM(p_in, iterator_ifm_inner, param)

          } 
          // TODO(yunlei) BATCH=4

          // 2. Load weights -- 8ich * 16och
          Y0.insert(0, CT_BF16_LOAD_V<32>(p_w)); p_w = byte_incr(p_w, 32*2);//ic0-7 oc0-3                                                                                                      
          Y1.insert(0, CT_BF16_LOAD_V<32>(p_w)); p_w = byte_incr(p_w, 32*2);//ic0-7 oc4-7
          Y2.insert(0, CT_BF16_LOAD_V<32>(p_w)); p_w = byte_incr(p_w, 32*2);//ic0-7 oc8-11                                                                                                      
          Y3.insert(0, CT_BF16_LOAD_V<32>(p_w)); p_w = byte_incr(p_w, 32*2);
          p_w = INCR_ITR3D_PARAM(p_w, iterator_wts_inner, param)

          // 3. Load/shuffle PSUM -- 4width * 16och
          #ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
                  printf("p_psum_in:%d\n",(long)p_psum_in);
          #endif

          // w4co4  //w4co4-7 //w4co8-11 //w4co12-15
          CONVT_LOAD_16_ACC_DATA(p_psum_in, iterator_psum_in_inner, param, C0_acc, C1_acc, C2_acc, C3_acc)
          mmul C0(C0_acc); mmul C1(C1_acc); mmul C2(C2_acc); mmul C3(C3_acc); 
          C0.mac(X0, Y0);C1.mac(X0, Y1);C2.mac(X0, Y2);C3.mac(X0, Y3);

        #ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
                  printf("p_psum_in push back:%d\n",(long)p_psum_in);
                  printf("p_w:%d\n",(long)p_w);
                  printf("p_psum_out store :%d\n",(long)p_psum_out);
        #endif          

          //4. Store/shuffle PSUM      
          //shuffle w4co4... to w4co0~7 & w4co8~15
          vector<float, 16> C0_w2c8, C1_w2c8, C2_w2c8, C3_w2c8;
          SHUFFLE_4_FULL_DATA(C0, C1, C2, C3, C0_w2c8, C1_w2c8, C2_w2c8, C3_w2c8)

          // och16
          store_v(p_psum_out, C0_w2c8.extract<8>(0)); p_psum_out = byte_incr(p_psum_out, param.psum_inc0);//w0 oc0-7
          store_v(p_psum_out, C0_w2c8.extract<8>(1)); p_psum_out = byte_incr(p_psum_out, param.psum_inc0);//w1 oc0-7
          store_v(p_psum_out, C1_w2c8.extract<8>(0)); p_psum_out = byte_incr(p_psum_out, param.psum_inc0);//keep och16
          store_v(p_psum_out, C1_w2c8.extract<8>(1)); p_psum_out = byte_incr(p_psum_out, param.psum_inc1);
          
          store_v(p_psum_out, C2_w2c8.extract<8>(0)); p_psum_out = byte_incr(p_psum_out, param.psum_inc0);//w0-1 oc8-15
          store_v(p_psum_out, C2_w2c8.extract<8>(1)); p_psum_out = byte_incr(p_psum_out, param.psum_inc0);
          store_v(p_psum_out, C3_w2c8.extract<8>(0)); p_psum_out = byte_incr(p_psum_out, param.psum_inc0);
          store_v(p_psum_out, C3_w2c8.extract<8>(1)); //                   

#ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
          printf("p_psum_out after store:%d\n",(long)p_psum_out);
#endif
          p_psum_out = INCR_ITR3D_PARAM(p_psum_out, iterator_psum_inner, param)

#ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
          printf("p_psum_out inner :%d\n",(long)p_psum_out);             
#endif
          
        }//inner loop
   
        // update addr for outer loop 
        p_in = p_in_org;
        p_w = INCR_ITR3D_PARAM(p_w, iterator_wts_outer, param)
        p_psum_out = INCR_ITR3D_PARAM(p_psum_out, iterator_psum_outer, param)
        p_psum_in = INCR_ITR3D_PARAM(p_psum_in, iterator_psum_in_outer, param)

#ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
          //printf("p_w outer loop:%d\n", p_w);
          printf("p_psum_out outer loop:%d\n",(long)p_psum_out);
          printf("p_psum_in outer loop:%d\n",(long)p_psum_in);
          //printf("p_in:%d\n", p_in);
#endif

    } //outer loop

    // for relu
    bfloat16 bar = 0;
    {
      int out_sign = 1 - act_type; // Note enum for act types are: 0,1,2 -> linear, relu, lrelu(plan)
      if (out_sign == 1) {
          bar = -3.38953139e38;
      }
    }

    //load psum(float) and store ofm
    if (param.out_mode == FULL) {
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4, ) 
        for(unsigned j=0; j<param.pout_inc2; j++)chess_modulo_scheduling_budget_ratio(200){

          aie::vector<float, 16> ps0, ps1;
          aie::accum<accfloat, 8> bias0;
          aie::accum<accfloat, 16> bc0, bc1, pc0, pc1;

  #ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
    printf("\n[check]p_out outer loop:%d\n",(long)p_out);
    printf("[CHECK] p_psum_dup : %d\n", (long)p_psum_dup);
    printf("[CHECK] p_bias_in : %d\n", (long)p_bias_in);
  #endif

          LOAD_FLOAT_DATA(p_psum_dup, ps0, 8*4);//w0-1
          LOAD_FLOAT_DATA(p_psum_dup, ps1, 8*4);//w2-3

          bias0.from_vector(load_v<8>(p_bias_in));  
          p_bias_in = INCR_ITR3D_PARAM(p_bias_in, iterator_bias, param)

  #ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
    printf("[CHECK] p_bias_in : %d\n", (long)p_bias_in);
  #endif
          bc0.insert(0, bias0); bc0.insert(1, bias0);//cp to w4
          bc1.insert(0, bias0); bc1.insert(1, bias0);
          pc0.from_vector(ps0);
          pc1.from_vector(ps1);
          //act_type==LINEAR || act_type==RELU
          store_v(p_out, max(add(pc0, bc0).to_vector<bfloat16>(), bar)); p_out = byte_incr(p_out, 16*2);
          store_v(p_out, max(add(pc1, bc1).to_vector<bfloat16>(), bar)); p_out = byte_incr(p_out, 32);

        }// end loop
    }// end if out_mode

}

#else  // aie2p/aie2ps

// Design
// 1. psum layout w4 x co8, load 256 bit per LD to avoid unaligned load
// 2. inner loop w_g -> h_g -> ci_g
// 3. outer loop kx -> ky -> co_g
template <uint8_t batch_size, act_t act_type, //out_mode_t out_mode,
          typename dtype_ifm, typename dtype_wts, typename dtype_ofm>
INLINE void
conv2d_transpose_bf16_generic(int *__restrict input, int *__restrict weights,
                         int *__restrict output,
                         conv2d_transpose_bf16_params_t &param) {

  // initialize p_psum and bias
  float OFM_DM_BANK *p_psum = (float OFM_DM_BANK *)output + 
                              (param.psum_buff_offset);

  bfloat16 WT_DM_BANK *__restrict p_bias_in = (bfloat16 WT_DM_BANK *__restrict)(weights) +
                                               (LAYER_PARAM_BYTES >> 1);//bytes to bf16

  float OFM_DM_BANK* __restrict p_psum_in = (float OFM_DM_BANK* __restrict) p_psum;
  float OFM_DM_BANK* __restrict p_psum_out = (float OFM_DM_BANK* __restrict) p_psum;
  float OFM_DM_BANK* __restrict p_psum_dup = (float OFM_DM_BANK* __restrict) p_psum;  

  // initialize ifm, wts and ofm
  dtype_ifm IFM_DM_BANK *__restrict p_in = (dtype_ifm IFM_DM_BANK *__restrict)(input) + 
                                param.ifm_offset;
  dtype_wts WT_DM_BANK *__restrict p_w = (dtype_wts WT_DM_BANK *__restrict)(weights) +
                              param.hdr_len +
                              param.wts_offset;
  dtype_ofm OFM_DM_BANK * __restrict p_out = (param.psum_out == 0) ? 
                                (dtype_ofm OFM_DM_BANK * __restrict)output : 
                                (dtype_ofm OFM_DM_BANK * __restrict)p_psum;
#ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
      printf("[check] p_psum:%ld, p_out:%ld , output:%ld, param.psum_buff_offset:%d\n",(long)p_psum, (long)p_out, (long)output, param.psum_buff_offset);
      printf("[check] p_in:%d, weights:%d, param.hdr_len:%d, wts_offset:%d\n", (long)p_in, (long)weights, param.hdr_len, param.wts_offset);
#endif

  using mmul = mmul<4, 8, 8, bfloat16, bfloat16>;

  #define CONVT_LOAD_DATA_4x8(ptr, buff, incr) \
  {\
      buff = CT_BF16_LOAD_V<32>(ptr);ptr=byte_incr(ptr, incr);\
  }
  //*********************************************//
  //Load psum data 256b w2c8 with stride.             //
  //*********************************************//
  #define LOAD_FLOAT_DATA_4x8_WITH_STRIDE(ptr, buff, incr) \
  {\
      buff = insert(buff, 0, CT_BF16_LOAD_V<8>(ptr));ptr=byte_incr(ptr, 8*sizeof(float)*param.stride);\
      buff = insert(buff, 1, CT_BF16_LOAD_V<8>(ptr));ptr=byte_incr(ptr, incr);\
  }

  //*********************************************//
  //Load psum data w4c8.                         //
  //*********************************************//
  #define LOAD_FLOAT_DATA_4x8(ptr, buff, incr) \
  {\
      buff = insert(buff, 0, CT_BF16_LOAD_V<16>(ptr));ptr=byte_incr(ptr, 16*sizeof(float));\
      buff = insert(buff, 1, CT_BF16_LOAD_V<16>(ptr));ptr=byte_incr(ptr, incr);\
  }   //print(buff, true, "C0_pre: ");\

  //*********************************************//
  //Load psum data and shuffle to w4co8.          //
  //*********************************************//
  //where acc is load, acc will be store
  //psum layout is fit for calculation
  #define CONVT_LOAD_16_ACC_DATA_4x8(p_psum_in, iterator_psum_in_inner, param, C0_acc, C1_acc)\
  {\
      vector<float, 16> C0in_w2c8, C1in_w2c8, C2in_w2c8, C3in_w2c8;\
      accum<accfloat, 16> C0_v, C1_v, C2_v, C3_v;\
      LOAD_FLOAT_DATA_4x8_WITH_STRIDE(p_psum_in, C0in_w2c8, param.psum_inc0); C0_v.from_vector(C0in_w2c8);\
      LOAD_FLOAT_DATA_4x8_WITH_STRIDE(p_psum_in, C1in_w2c8, param.psum_inc1); C1_v.from_vector(C1in_w2c8);\
      LOAD_FLOAT_DATA_4x8_WITH_STRIDE(p_psum_in, C2in_w2c8, param.psum_inc0); C2_v.from_vector(C2in_w2c8);\
      LOAD_FLOAT_DATA_4x8_WITH_STRIDE(p_psum_in, C3in_w2c8, 0); C3_v.from_vector(C3in_w2c8);\
      C0_acc = insert(C0_acc, 0, C0_v);\
      C0_acc = insert(C0_acc, 1, C1_v);\
      C1_acc = insert(C1_acc, 0, C2_v);\
      C1_acc = insert(C1_acc, 1, C3_v);\
      p_psum_in = INCR_ITR3D_PARAM(p_psum_in, iterator_psum_in_inner, param)\
  }

  // Following counters are needed for ADD_2D/3D_BYTE intrinsics
  // SETUP_ITR2D_COUNTERS(iterator_name) / SETUP_ITR3D_COUNTERS(iterator_name)
  // type 1: 2D iterator, 2: 3d Iterator
  // e.g. SETUP_ITR3D_COUNTERS(iterator_inner)
  // Resulting count variable is : iterator_inner_cnt0, iterator_inner_cnt_1
  SETUP_ITR3D_COUNTERS(iterator_ifm_inner)
  SETUP_ITR3D_COUNTERS(iterator_wts_inner)
  SETUP_ITR3D_COUNTERS(iterator_wts_outer)
  SETUP_ITR3D_COUNTERS(iterator_psum_inner)
  SETUP_ITR3D_COUNTERS(iterator_psum_outer)
  SETUP_ITR3D_COUNTERS(iterator_bias)
  SETUP_ITR3D_COUNTERS(iterator_psum_in_inner)
  SETUP_ITR3D_COUNTERS(iterator_psum_in_outer)

#ifdef DEBUG_CONV2D_TRANSPOSE_BF16
for(int i =0; i< 4;i++){
print(CT_BF16_LOAD_V<32>((bfloat16 IFM_DM_BANK *)p_in), true, "test: ");
printf("addr:%d\n", (long)p_in);
p_in = byte_incr(p_in, 32*2);
}
p_in = byte_incr(p_in, -32*4*2);
#endif

// initialization of psum zone
int32_t zero_psum = 1 - param.psum_in;
auto vec_init = aie::broadcast<int32_t, 32>(0);
int32_t *p_psum_init = (int32_t *)p_psum_in;
if(zero_psum == 1){
  AIE_PREPARE_FOR_PIPELINING 
  AIE_LOOP_RANGE(4, ) 
  for(unsigned j=0; j<param.pout_inc2; j++) {
    #ifdef __X86SIM__
      for(int i = 0; i < 32; i++)
        *p_psum_init++ = 0;
    #else
      aie::store_v((int32_t *)p_psum_init, vec_init);
      p_psum_init += 32;      
    #endif
  }
}

dtype_ifm IFM_DM_BANK *p_in_org = p_in;

  for (unsigned j = 0; j < param.outer_g; j++)
    //chess_prepare_for_pipelining chess_loop_range(4, ) //pm size increase
    {

      // Load psum, load ifm, load wts, mac, store psum
      AIE_PREPARE_FOR_PIPELINING 
      AIE_LOOP_RANGE(4, ) 
      for (int i = 0; i < param.inner_g; i++){

          #ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
              printf("\nouter_loop:%d, inner_loop:%d\n", j, i);
              printf("curr_depth_iter:%d, num_ifm_depth_iter:%d, psum_in:%d, out_mode:%d\n", 
                  param.curr_depth_iter, param.num_ifm_depth_iter, 
                  param.psum_in, param.exec_type);
              printf("psum_init32:%d\n", (long)p_psum_in);
          #endif

          vector<bfloat16, 32> X0;
          vector<bfloat16, 64> Y0, Y1;
          accum<accfloat, 32> C0_acc, C1_acc;
          // 1. Load IFM data -- 4width * 8ich
          if constexpr(batch_size < 4) {
          
              CONVT_LOAD_DATA_4x8(p_in, X0, 32*2);
              p_in = INCR_ITR3D_PARAM(p_in, iterator_ifm_inner, param)

          } 

          // 2. Load weights -- 8ich * 16och
          Y0.insert(0, CT_BF16_LOAD_V<64>(p_w)); p_w = byte_incr(p_w, 64*2);//ic0-7 oc0-7                                                                                                      
          Y1.insert(0, CT_BF16_LOAD_V<64>(p_w)); p_w = byte_incr(p_w, 64*2);//ic0-7 oc8-15 
          p_w = INCR_ITR3D_PARAM(p_w, iterator_wts_inner, param)

          // 3. Load PSUM -- 4width * 16och
        #ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
            printf("p_psum_in:%d\n",(long)p_psum_in);
        #endif

          // w4co0-7 //w4co8-15
          CONVT_LOAD_16_ACC_DATA_4x8(p_psum_in, iterator_psum_in_inner, param, C0_acc, C1_acc)
          mmul C0(C0_acc); mmul C1(C1_acc);
          #ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
          set_rnd(rnd_sym_inf);
          #endif          
          C0.mac(X0, Y0); C1.mac(X0, Y1);
          #ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
          set_rnd(rnd_conv_even);
          #endif
        #ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
            printf("p_psum_in push back:%d\n",(long)p_psum_in);
            printf("p_w:%d\n",(long)p_w);
            printf("p_psum_out store :%d\n",(long)p_psum_out);
        #endif          

          //4. Store/shuffle PSUM      
          //no need to shuffle
          vector<float, 32> C0_w4c8, C1_w4c8;
          C0_w4c8 = C0.to_vector<float>();
          C1_w4c8 = C1.to_vector<float>();

          // och16
          store_v(p_psum_out, C0_w4c8.extract<8>(0)); p_psum_out = byte_incr(p_psum_out, param.psum_inc0);//w0-1 oc0-7
          store_v(p_psum_out, C0_w4c8.extract<8>(1)); p_psum_out = byte_incr(p_psum_out, param.psum_inc0);//w2-3 oc0-7
          store_v(p_psum_out, C0_w4c8.extract<8>(2)); p_psum_out = byte_incr(p_psum_out, param.psum_inc0);//keep och16
          store_v(p_psum_out, C0_w4c8.extract<8>(3)); p_psum_out = byte_incr(p_psum_out, param.psum_inc1);

          store_v(p_psum_out, C1_w4c8.extract<8>(0)); p_psum_out = byte_incr(p_psum_out, param.psum_inc0);//w0-1 oc0-7
          store_v(p_psum_out, C1_w4c8.extract<8>(1)); p_psum_out = byte_incr(p_psum_out, param.psum_inc0);//w2-3 oc0-7
          store_v(p_psum_out, C1_w4c8.extract<8>(2)); p_psum_out = byte_incr(p_psum_out, param.psum_inc0);//keep och16
          store_v(p_psum_out, C1_w4c8.extract<8>(3));         

        #ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
          printf("p_psum_out after store:%d\n",(long)p_psum_out);
        #endif

          p_psum_out = INCR_ITR3D_PARAM(p_psum_out, iterator_psum_inner, param)

        #ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
          printf("p_psum_out inner :%d\n",(long)p_psum_out);             
        #endif
          
        }//inner loop
   
        // update addr for outer loop 
        p_in = p_in_org;
        p_w = INCR_ITR3D_PARAM(p_w, iterator_wts_outer, param)
        p_psum_out = INCR_ITR3D_PARAM(p_psum_out, iterator_psum_outer, param)
        p_psum_in = INCR_ITR3D_PARAM(p_psum_in, iterator_psum_in_outer, param)

      #ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
        printf("p_psum_out outer loop:%d\n",(long)p_psum_out);
        printf("p_psum_in outer loop:%d\n",(long)p_psum_in);
      #endif

    } //outer loop

    // for relu
    bfloat16 bar = 0;
    {
      int out_sign = 1 - act_type; // Note enum for act types are: 0,1,2 -> linear, relu, lrelu(plan)
      if (out_sign == 1) {
          bar = -3.38953139e38;
      }
    }

    //load psum(float) and store ofm
    if (param.out_mode == FULL) {
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4, ) 
        for(unsigned j=0; j<param.pout_inc2; j++)chess_modulo_scheduling_budget_ratio(200){

          aie::vector<float, 32> ps0, ps1;
          aie::accum<accfloat, 16> bias0;
          aie::accum<accfloat, 32> bc0, bc1, pc0, pc1;

        #ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
          printf("\n[check]p_out outer loop:%d\n",(long)p_out);
          printf("[CHECK] p_psum_dup : %d\n", (long)p_psum_dup);
          printf("[CHECK] p_bias_in : %d\n", (long)p_bias_in);
        #endif

          LOAD_FLOAT_DATA_4x8(p_psum_dup, ps0, 16*4);//w0-3

          bias0.from_vector(load_v<16>(p_bias_in)); //w2 * c8 
          p_bias_in = INCR_ITR3D_PARAM(p_bias_in, iterator_bias, param)

        #ifdef DEBUG_CONV2D_TRANSPOSE_ADDR
          printf("[CHECK] p_bias_in : %d\n", (long)p_bias_in);
        #endif
          bc0.insert(0, bias0); bc0.insert(1, bias0);//cp to w4
          pc0.from_vector(ps0);
        #ifdef __X86SIM__
          for(int i = 0; i < 32; i++)
          *p_out++ = max(add(pc0, bc0).to_vector<bfloat16>(), bar)[i];
        #else
          store_v(p_out, max(add(pc0, bc0).to_vector<bfloat16>(), bar)); p_out = byte_incr(p_out, 32*2);
        #endif

        }// end loop
    }// end if out_mode

}


#endif // __AIE_ARCH__ == 20 bfloat16


template <uint8_t batch_size, act_t act_type, typename dtype_ifm,
          typename dtype_wts, typename dtype_ofm, typename config_ifm,
          typename config_wts, typename config_ofm>
void conv2d_transpose(
    adf::input_buffer_conf<dtype_ifm, config_ifm> &__restrict ifm,
    adf::input_buffer_conf<dtype_wts, config_wts> &__restrict wts,
    adf::output_buffer_conf<dtype_ofm, config_ofm> &ofm,
    conv2d_transpose_bf16_params_t &conv2d_transpose_bf16_params) {


  // Setup iteration level parameters like IO mode etc
  setup_conv2d_transpose_bf16_iter_params(conv2d_transpose_bf16_params);

  // Setup out_mode
    switch(conv2d_transpose_bf16_params.exec_type) {
    case 0:
    case 1:
        conv2d_transpose_bf16_params.out_mode = FULL;
    break;
    case 2:
    case 3:
        conv2d_transpose_bf16_params.out_mode = PARTIAL;
    break;
  }

  // Extract low level pointers used by the actual kernel code
  int *input = (int *)ifm.data();
  int *weights = (int *)wts.data();
  int *output = (int *)ofm.data();

    conv2d_transpose_bf16_generic<batch_size, act_type, dtype_ifm,
                          dtype_wts, dtype_ofm>(input, weights, output,
                                              conv2d_transpose_bf16_params);

}

#endif //__CONV2D_TRANSPOSE_BF16_H__
