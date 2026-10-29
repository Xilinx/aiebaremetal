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

#ifndef __CONV2D_OPT_H__
#define __CONV2D_OPT_H__

#include <adf.h>
#include "aie.hpp"

#include "utils.h"         // Contains some utility functions and definitions related to intrinsics usage
#include "mllib_const.h"   // Constants shared across graphs like activation type
#include "adf/io_buffer/io_buffer_access.h" // temporary utility from tools to access ping pong buffers, to be removed

#include "conv2d_opt_params.h"
#include "arch_kernel_helpers.h"
#include "helpers_api.h"

#include "./qdq/wrapper_qdq.cc"


#pragma chess_file_options "showcolor -c"

using namespace aie;

#if __AIE_ARCH__ >= 21

template <int ld = 1, typename T , typename T_wgt, typename T_accs>
ALWAYS_INLINE void loop_body_conv_int8x8(T __aie_dm_resource_a * & pIn,
                                         T_wgt __aie_dm_resource_b * & pW,
                                         const conv3d_params_t &params,
                                         unalign_buffer<ld> &fA, addr_t &cntAL1,
                                         addr_t &cntAL2, addr_t &cntB,
                                         aie::vector<int8_t,64> &Xbuff, T_accs &accs_v,
                                         int zero_acc, int casc) {

    constexpr unsigned w_para = sizeof(T) == 1 ? 8 : 4;
    constexpr unsigned vec_ele_num = sizeof(T) == 1 ? 64 : 32;
    using mac_n = aie::mmul<w_para, 8, 8, T, T_wgt>;
    aie::vector<T, vec_ele_num> Xbuff0;
    load_unaligned<ld, false, T>((T __aie_dm_resource_a *&)pIn, fA, params, cntAL1,
                       cntAL2, cntB, Xbuff0);

    aie::vector<T_wgt, 64> STORAGE_REG_X2 ybuff0_v;
    aie::vector<T_wgt, 64> STORAGE_REG_X3 ybuff1_v;

    ybuff0_v = aie::load_v<64,aie_dm_resource::b>( pW); pW+=64;
    ybuff1_v = aie::load_v<64,aie_dm_resource::b>( pW); pW+=64;
    if constexpr (sizeof(T) == 1)
    {
        auto acc_a0 = accs_v.a0;
        auto acc_a1 = accs_v.a1;
        mac_n D0(aie::op_zero(acc_a0, zero_acc));
        mac_n D1(aie::op_zero(acc_a1, zero_acc));
        D0.mac(Xbuff0, ybuff0_v);
        D1.mac(Xbuff0, ybuff1_v);
        accs_v.a0 = D0.to_accum();
        accs_v.a1 = D1.to_accum();
    }
    else
    {
      auto vec_a0 = accs_v.a0.template to_vector<int32>(0);
      aie::accum<acc64, 32> acc_a0;
      acc_a0.from_vector(vec_a0, 0);

      auto vec_a1 = accs_v.a1.template to_vector<int32>(0);
      aie::accum<acc64, 32> acc_a1;
      acc_a1.from_vector(vec_a1, 0);

      mac_n D0(aie::op_zero(acc_a0, zero_acc));
      mac_n D1(aie::op_zero(acc_a1, zero_acc));
      D0.mac(Xbuff0, ybuff0_v);
      D1.mac(Xbuff0, ybuff1_v);
      auto d0_vec = D0.to_accum().template to_vector<int32_t>(0);
      auto d1_vec = D1.to_accum().template to_vector<int32_t>(0);
      accs_v.a0.template from_vector(d0_vec, 0);
      accs_v.a1.template from_vector(d1_vec, 0);
    }
}

#ifdef CONV2D_A16W8_MODE
template <int ld = 1, typename T = int16, typename T_wgt = int8, typename T_psum = int32>
#else
template <int ld = 1, typename T = int8, typename T_wgt = int8, typename T_psum = int32>
#endif
ALWAYS_INLINE void direct_conv_int8x8_tdm_split_ld2_template(
    T *input, T_wgt *weights, T_psum *restrict tdm1_in,
    T_psum *restrict tdm2_in, T *__restrict output,
    const conv3d_params_t &params, const int zero_init,
    const enum KernelConfig kc_in, const enum KernelConfig kc_out) {

    T __aie_dm_resource_a * pIn  = (T __aie_dm_resource_a *) input;
    T_wgt __aie_dm_resource_b * pW  = (T_wgt __aie_dm_resource_b *) weights;

    T_psum __aie_dm_resource_c * restrict pTdm32a = (T_psum __aie_dm_resource_c *) tdm1_in;

    T_psum __aie_dm_resource_d * restrict pTdm32b = (T_psum __aie_dm_resource_d *) tdm2_in;

    T_psum __aie_dm_resource_c * restrict pOut32a = (T_psum __aie_dm_resource_c *) tdm1_in;
    T_psum __aie_dm_resource_d * restrict pOut32b = (T_psum __aie_dm_resource_d *) tdm2_in;

    using accs_type = std::conditional_t<sizeof(T) == 2, acc_struct_16_final, acc_struct>;
    accs_type accs_v;
    aie::vector<int8, 64> Xbuff;
    unalign_buffer<ld> fA;
    // this function call resets the fifo for ld=1, doesnt do anything for ld=0
    fA.init();

    addr_t STORAGE_MOD_D1 cntAL1 = 0;
    addr_t STORAGE_MOD_D5 cntAL2 = 0;
    addr_t STORAGE_MOD_D2 cntAO1 = 0;
    addr_t STORAGE_MOD_D6 cntAO2 = 0;
    addr_t STORAGE_MOD_D0 cntS1 = 0;
    addr_t STORAGE_MOD_D4 cntS2 = 0;
    addr_t STORAGE_MOD_D3 cntB = 0;

    chess_separator();

    int incrT0 = (params.shft == T256_2x2_hi ? 1 : 0);
    setup_pin<ld>((int8 __aie_dm_resource_a *&)pIn);
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2, ) 
    for (int j = 0; j < params.outer_loop; j++){
        int zero_acc = zero_init;
        int kc_in_casc =
            kc_in == KC_CASC || kc_in == KC_TDM16_CASC || kc_in == KC_TDM32_CASC;

        // dpu projects rely on 256 bit alignement, as such 512 loads are not
        // guaranteed to be correct, in addition CRVO-7210 comes into effect for
        // AIE2
        load_accs<ld>((T_psum __aie_dm_resource_c * restrict &)pTdm32a,
                      (T_psum __aie_dm_resource_d * restrict &)pTdm32b, accs_v,
                      incrT0);
        // this function call resets recomputes the frac help variable for ld=0,
        // doesnt do anything for ld=1
        fA.init((int8 __aie_dm_resource_a *&)pIn);
  
        loop_body_conv_int8x8<ld>(pIn, pW, params, fA, cntAL1, cntAL2, cntB,
                                  Xbuff, accs_v, zero_acc, 0);
        zero_acc = 0;
  
        loop_body_conv_int8x8<ld>(pIn, pW, params, fA, cntAL1, cntAL2, cntB,
                                  Xbuff, accs_v, zero_acc, 0);
        zero_acc = 0;

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(6, ) 
        for (int i = 0; i < params.inner_loop - 2; i++){
            loop_body_conv_int8x8<ld>(pIn, pW, params, fA, cntAL1, cntAL2, cntB,
                                      Xbuff, accs_v, zero_acc, 0);
            zero_acc = 0;
          }
  
        pIn = add_3d_byte(pIn, params.incAO3, params.numAO1, cntAO1,
                          params.incAO1, params.numAO2, cntAO2, params.incAO2);
        pW = add_2d_byte(pW, params.incB2, params.numB, cntB, params.incB1);
        store_accs<ld>((T_psum __aie_dm_resource_c * restrict &)pOut32a,
                       (T_psum __aie_dm_resource_d * restrict &)pOut32b, accs_v,
                       incrT0);
        
    }
    chess_separator();
}


#ifdef CONV2D_A16W8_MODE
template <int ld = 1, typename T = int16, typename T_wgt = int8, typename T_psum = int32>
ALWAYS_INLINE void direct_conv_int8x8_tdm_split_ld2_template_final(
    T *input, T_wgt *weights, T_psum *restrict tdm1_in,
    T_psum *restrict tdm2_in, T *__restrict output,
    const conv3d_params_t &params, const int zero_init,
    const enum KernelConfig kc_in, const enum KernelConfig kc_out) {

    T __aie_dm_resource_a * pIn  = (T __aie_dm_resource_a *) input;
    T_wgt __aie_dm_resource_b * pW  = (T_wgt __aie_dm_resource_b *) weights;

    T_psum __aie_dm_resource_c * restrict pTdm32a = (T_psum __aie_dm_resource_c *) tdm1_in;

    T_psum __aie_dm_resource_d * restrict pTdm32b = (T_psum __aie_dm_resource_d *) tdm2_in;

    T_psum __aie_dm_resource_c * restrict pOut32a = (T_psum __aie_dm_resource_c *) tdm1_in;
    T_psum __aie_dm_resource_d * restrict pOut32b = (T_psum __aie_dm_resource_d *) tdm2_in;
    
    T __aie_dm_resource_c * pOut  = (T __aie_dm_resource_c *) output;

    using accs_type = std::conditional_t<sizeof(T) == 2, acc_struct_16_final, acc_struct>;
    accs_type accs_v;
    aie::vector<int8, 64> Xbuff;
    unalign_buffer<ld> fA;
    // this function call resets the fifo for ld=1, doesnt do anything for ld=0
    fA.init();

    addr_t STORAGE_MOD_D1 cntAL1 = 0;
    addr_t STORAGE_MOD_D5 cntAL2 = 0;
    addr_t STORAGE_MOD_D2 cntAO1 = 0;
    addr_t STORAGE_MOD_D6 cntAO2 = 0;
    addr_t STORAGE_MOD_D0 cntS1 = 0;
    addr_t STORAGE_MOD_D4 cntS2 = 0;
    addr_t STORAGE_MOD_D3 cntB = 0;

    chess_separator();

    int incrT0 = (params.shft == T256_2x2_hi ? 1 : 0);
    setup_pin<ld>((int8 __aie_dm_resource_a *&)pIn);
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2, ) 
    for (int j = 0; j < params.outer_loop; j++){
        int zero_acc = zero_init;
        int kc_in_casc =
            kc_in == KC_CASC || kc_in == KC_TDM16_CASC || kc_in == KC_TDM32_CASC;

        // dpu projects rely on 256 bit alignement, as such 512 loads are not
        // guaranteed to be correct, in addition CRVO-7210 comes into effect for
        // AIE2
        load_accs<ld>((T_psum __aie_dm_resource_c * restrict &)pTdm32a,
                      (T_psum __aie_dm_resource_d * restrict &)pTdm32b, accs_v,
                      incrT0);
        // this function call resets recomputes the frac help variable for ld=0,
        // doesnt do anything for ld=1
        fA.init((int8 __aie_dm_resource_a *&)pIn);
  
        loop_body_conv_int8x8<ld>(pIn, pW, params, fA, cntAL1, cntAL2, cntB,
                                  Xbuff, accs_v, zero_acc, 0);
        zero_acc = 0;
  
        loop_body_conv_int8x8<ld>(pIn, pW, params, fA, cntAL1, cntAL2, cntB,
                                  Xbuff, accs_v, zero_acc, 0);
        zero_acc = 0;

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(6, ) 
        for (int i = 0; i < params.inner_loop - 2; i++){
            loop_body_conv_int8x8<ld>(pIn, pW, params, fA, cntAL1, cntAL2, cntB,
                                      Xbuff, accs_v, zero_acc, 0);
            zero_acc = 0;
          }
  
        pIn = add_3d_byte(pIn, params.incAO3, params.numAO1, cntAO1,
                          params.incAO1, params.numAO2, cntAO2, params.incAO2);
        pW = add_2d_byte(pW, params.incB2, params.numB, cntB, params.incB1);
        
        //Only for 16 bit data
        aie::vector<T, 32> chess_storage(x11) Obuff_v, chess_storage(x9) Obuff_vt;
        
        Obuff_v  = accs_v.a0.template to_vector<T>(params.shift_out_1);
        Obuff_vt = accs_v.a1.template to_vector<T>(params.shift_out_1);
        
        aie::store_v((int16 *)pOut, Obuff_v.template extract<16>(0)); pOut = byte_incr(pOut, params.incS0);
        aie::store_v((int16 *)pOut, Obuff_v.template extract<16>(1)); pOut = byte_incr(pOut, params.incCS1);
        aie::store_v((int16 *)pOut, Obuff_vt.template extract<16>(0)); pOut = byte_incr(pOut, params.incS0);
        aie::store_v((int16 *)pOut, Obuff_vt.template extract<16>(1)); pOut = add_3d_byte(pOut, params.incCS3, params.numCS1, cntS1, params.incCS1, params.numCS2, cntS2, params.incCS2);                
        
        //store_accs<ld>((T_psum __aie_dm_resource_c * restrict &)pOut32a,(T_psum __aie_dm_resource_d * restrict &)pOut32b, accs_v,incrT0);
       
    }
    chess_separator();
}
#endif


template <int loop_range = 2, int ld = 1>
ALWAYS_INLINE void biased_conv_int8x8_tdm_split_ld2_template
(
    int8_t *input, int8_t *weights, int8_t *bias, int *restrict tdm1_in,
    int *restrict tdm2_in, int8_t *__restrict output,
    const conv3d_params_t &params, const bool zero_init, const bool gate_bias,
    const enum KernelConfig kc_in, const enum KernelConfig kc_out
) {
    int norm_ch_g = params.tile_ocg * 2 * gate_bias;
    int no_tdm_buffer = ((kc_out == KC_RESULT8) && zero_init);
  
    int8_t __aie_dm_resource_a * pIn  = (int8_t __aie_dm_resource_a *) input;
    int8_t __aie_dm_resource_b * pW  = (int8_t __aie_dm_resource_b *) weights;
    int8_t __aie_dm_resource_b * restrict pBN0 = (int8_t __aie_dm_resource_b *) bias;
    int8_t __aie_dm_resource_b * restrict pBN1 = pBN0 + 32*( (norm_ch_g)>1);
  
    int32_t __aie_dm_resource_c * restrict pTdm32a = (int32_t __aie_dm_resource_c *) tdm1_in;
    int32_t __aie_dm_resource_d * restrict pTdm32b = (int32_t __aie_dm_resource_d *) tdm2_in;
    int8_t __aie_dm_resource_a * pOut8  = (int8_t __aie_dm_resource_a *) output;
    int32_t __aie_dm_resource_c * restrict pOut32a = (int32_t __aie_dm_resource_c *) tdm1_in;
    int32_t __aie_dm_resource_d * restrict pOut32b = (int32_t __aie_dm_resource_d *) tdm2_in;
  
    acc_struct accs_v;

    unalign_buffer<ld> fA;
    fA.init();

    addr_t STORAGE_MOD_D1 cntAL1 = 0;
    addr_t STORAGE_MOD_D5 cntAL2 = 0;
    addr_t STORAGE_MOD_D2 cntAO1 = 0;
    addr_t STORAGE_MOD_D6 cntAO2 = 0;
    addr_t STORAGE_MOD_D0 cntS1 = 0;
    addr_t STORAGE_MOD_D4 cntS2 = 0;
    addr_t STORAGE_MOD_D3 cntB = 0;
    addr_t STORAGE_MOD_D7 cntBN = 0;

    bool sign_res   = (params.act_type_1 == NoRelu) || (params.act_type_1 == Hsigmoid) || (params.act_type_1 == Hswish) || (params.act_type_1 == Leaky_Prelu); 
    bool sign_res_h = !(params.act_type_1 == Hsigmoid);
  
    aie::vector<int8, 64> Xbuff;
    aie::vector<int8,64> min_res_v = aie::broadcast<int8,64>((params.act_type_1 == Relu6) ? 96 : 127 );
    aie::vector<uint16,16> bias_shift_v = aie::broadcast<int32,16>(( (norm_ch_g)>0 ) << params.shift_bias_1 ).template cast_to<uint16>().template extract<16>(0);

    int incrT0 = (params.str_d == 1) | -no_tdm_buffer;
    setup_pin<ld>((int8 __aie_dm_resource_a *&)pIn);
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(loop_range, ) 
    for (int j = 0; j < params.outer_loop; j++)
    //unguard this for R1P8 version of the AIE Core
#ifdef CORE_RELEASE_R1P8
    chess_allocate(DC:8)
    chess_allocate(DJ:8)
    chess_allocate(M:8)
    chess_allocate(R:20)
    chess_allocate(X:12)
    chess_allocate(CM:9)
    chess_keep_free_for_pipelining(R:8)
#endif
    {
        int zero_acc = kc_in == KC_ZERO || zero_init;
  
        if (kc_in != KC_ZERO) {
  
          // dpu projects rely on 256 bit alignement, as such 512 loads are not
          // guaranteed to be correct, in addition CRVO-7210 comes into effect for
          // AIE2
          load_accs<1>((int32 __aie_dm_resource_c * restrict &)pTdm32a,
                       (int32 __aie_dm_resource_d * restrict &)pTdm32b, accs_v,
                       incrT0);
  
          aie::vector<int8, 64>  Xbuff0;
          load_unaligned<ld, true>(pIn, fA, params,
                                   cntAL1, cntAL2, cntB, Xbuff0);
          aie::vector<int8,64>  xbuff0_v = Xbuff0;
          // dpu projects rely on 256 bit alignement, as such 512 loads are not
          // guaranteed to be correct
          aie::vector<int8, 64> STORAGE_REG_X2 ybuff0_v;
          aie::vector<int8, 64> STORAGE_REG_X3 ybuff1_v;
          ybuff0_v = aie::load_v<64,aie_dm_resource::b>( pW); pW+=64;
          ybuff1_v = aie::load_v<64,aie_dm_resource::b>( pW); pW+=64;
          int STORAGE_CNT_R1 cntB_r = cntB;
          int STORAGE_CNT_R1 offBN_r = ((norm_ch_g) > 2) * 64 * cntB_r;
          int STORAGE_MOD_DJ7 offBN = offBN_r;
  
          using bias_mul = aie::mmul<8, 2, 8, uint16, int16>;
          using mac888 = aie::mmul<8, 8, 8, int8, int8>;
  
          aie::accum<acc32, 64> dmac_acc0 = accs_v.a0;
          aie::accum<acc32, 64> dmac_acc1 = accs_v.a1;
  
          aie::vector<int16, 16>  bias_v =
              aie::load_v<32, aie_dm_resource::b>(
                  byte_incr(pBN0, offBN))
                  .unpack().extract<16>(0);
          bias_mul B0;
          B0.mul(bias_shift_v, bias_v);
          //STORAGE_BIAS_A0 maybe needed to apply on B0.accum
  
          mac888 D0(aie::op_zero(dmac_acc0, zero_acc));
          D0.mac(xbuff0_v, ybuff0_v);
          dmac_acc0 = aie::add(B0.to_accum(),
                               D0.to_accum());
  
          bias_v = aie::load_v<32, aie_dm_resource::b>(
                       byte_incr(pBN1, offBN))
                       .unpack().extract<16>(0);
          bias_mul B1;
          B1.mul(bias_shift_v, bias_v);
  
          //STORAGE_BIAS_A1 maybe needed to apply on B1.accum
          mac888 D1(aie::op_zero(dmac_acc1, zero_acc));
          D1.mac(xbuff0_v, ybuff1_v);
          dmac_acc1 = aie::add(B1.to_accum() ,
                               D1.to_accum());
  
          accs_v.a0 = dmac_acc0;
          accs_v.a1 = dmac_acc1;

      }
       else {

          aie::vector<int8, 64> Xbuff0;

          load_unaligned<ld, true>(pIn, fA, params,
                                   cntAL1, cntAL2, cntB, Xbuff0);
          aie::vector<int8, 64> xbuff0_v = Xbuff0;
          Xbuff = Xbuff0;

          // dpu projects rely on 256 bit alignement, as such 512 loads are not
          // guaranteed to be correct
          aie::vector<int8, 64> STORAGE_REG_X2 ybuff0_v;
          aie::vector<int8, 64> STORAGE_REG_X3 ybuff1_v;

          ybuff0_v = aie::load_v<64,aie_dm_resource::b>( pW); pW+=64;
          ybuff1_v = aie::load_v<64,aie_dm_resource::b>( pW); pW+=64;

          using bias_mul = aie::mmul<8, 2, 8, uint16, int16>;
          using mac888 = aie::mmul<8, 8, 8, int8, int8>;
          aie::accum<acc32, 64> dmac_acc0;
          aie::accum<acc32, 64> dmac_acc1;

          aie::vector<int16, 32> bias_v =
              aie::load_v<32, aie_dm_resource::b>(pBN0)
                  .unpack();
          pBN0 = add_2d_ptr( pBN0, 32*(1 - norm_ch_g), norm_ch_g - 1, cntBN, 32 );
          bias_mul B0;
          B0.mul(bias_shift_v, bias_v.extract<16>(0));

          mac888 D0(B0.to_accum());
          D0.mac(xbuff0_v, ybuff0_v);

          bias_v = aie::load_v<32, aie_dm_resource::b>(pBN0)
                       .unpack();
          pBN0 = add_2d_ptr( pBN0, 32*(1 - norm_ch_g), norm_ch_g - 1, cntBN, 32 );
          bias_mul B1;
          B1.mul(bias_shift_v, bias_v.extract<16>(0));

          mac888 D1(B1.to_accum());
          D1.mac(xbuff0_v, ybuff1_v);

          accs_v.a0 = D0.to_accum();
          accs_v.a1 = D1.to_accum();
      }

      zero_acc = 0;
      fA.init(params.numAL1 == 0 ? 64 : params.incAI1 + params.incAL1 + 32);
      // peeling the loop explicitly improves the overhead
      loop_body_conv_int8x8<ld>(pIn, pW, params, fA, cntAL1, cntAL2, cntB,
                                Xbuff, accs_v, 0, 0);

      AIE_PREPARE_FOR_PIPELINING 
      AIE_LOOP_RANGE(6, ) 
      for (int i = 0; i < params.inner_loop - 2; i++){

          loop_body_conv_int8x8<ld>(pIn, pW, params, fA, cntAL1, cntAL2, cntB,
                                    Xbuff, accs_v, 0, 0);
        }

      pIn = add_3d_byte(pIn, params.incAO3, params.numAO1, cntAO1,
                        params.incAO1, params.numAO2, cntAO2, params.incAO2);
      pW = add_2d_byte(pW, params.incB2, params.numB, cntB, params.incB1);

      if (kc_out == KC_RESULT8) {
        aie::vector<int8, 64> chess_storage(x11) Obuff_v, chess_storage(x9) Obuff_vt;

        Obuff_v = accs_v.a0.template to_vector_sign<int8_t>(sign_res, params.shift_out_1);
        Obuff_vt = accs_v.a1.template to_vector_sign<int8_t>(sign_res, params.shift_out_1);

#if (CONV_HSIG_HSWISH)
        aie::vector<int16, 64> hsigmoid_param1_v =
            aie::broadcast<int16, 64>(2731);
        aie::vector<int8, 64> Obuff_p0;
        aie::accum<acc32, 64> coef1;
        aie::accum<acc32, 64> Obuff_acc;
        int16 coef1_scalar[1] = { 8193 };
        int16 * coef1_ptr = coef1_scalar;
        coef1.from_vector(aie::broadcast<int16, 64>(*coef1_ptr),
                          params.hsigmoid_in);
        Obuff_p0 = Obuff_v; // buffer that one

        if (params.act_type_1 == Hsigmoid || params.act_type_1 == Hswish) {
          Obuff_acc = aie::mac(coef1, hsigmoid_param1_v, Obuff_v);
          Obuff_v = Obuff_acc.template to_vector_sign<int8_t>(
              0, params.shift_hsigmoid);
        }
        if (params.act_type_1 == Hswish) {
          Obuff_v = aie::min(Obuff_v, min_res_v, 0);
          Obuff_v =
              aie::mul(Obuff_v, Obuff_p0)
                  .template to_vector_sign<int8_t>(1, params.shift_hswish);
        }
#endif

#if (CONV_HSIG_HSWISH)

        Obuff_p0 = Obuff_vt; // buffer that one

        if (params.act_type_1 == Hsigmoid || params.act_type_1 == Hswish) {
          Obuff_acc = aie::mac(coef1, hsigmoid_param1_v, Obuff_vt);
          Obuff_vt = Obuff_acc.template to_vector_sign<int8_t>(
              0, params.shift_hsigmoid);
        }
        if (params.act_type_1 == Hswish) {
          Obuff_vt = aie::min(Obuff_vt, min_res_v, 0);
          Obuff_vt =
              aie::mul(Obuff_vt, Obuff_p0)
                  .template to_vector_sign<int8_t>(1, params.shift_hswish);
        }
#endif

#if (LEAKY_PRELU_CASE)
        aie::vector<int8,64> Obuff_leaky0, Obuff_leaky1;     
		aie::vector<int32,32> prelu_in_brst = aie::broadcast<int32, 32>(params.prelu_in);
        if (params.act_type_1 == Leaky_Prelu) {
            aie::accum<acc32,32> acc_a0= accs_v.a0.extract<32>(0);
            aie::accum<acc32,32> acc_a1= accs_v.a0.extract<32>(1);
            aie::accum<acc32,32> acc_a2= accs_v.a1.extract<32>(0);
            aie::accum<acc32,32> acc_a3= accs_v.a1.extract<32>(1);
            aie::vector<int32,32> accs_a0v = acc_a0.to_vector_sign<int32>(1,0);
            aie::vector<int32,32> accs_a1v = acc_a1.to_vector_sign<int32>(1,0);
            aie::vector<int32,32> accs_a2v = acc_a2.to_vector_sign<int32>(1,0);
            aie::vector<int32,32> accs_a3v = acc_a3.to_vector_sign<int32>(1,0);
            
            auto  acc_acc32_a0 = aie::mul(accs_a0v, prelu_in_brst);
            auto  acc_acc32_a1 = aie::mul(accs_a1v, prelu_in_brst);
            auto  acc_acc32_a2 = aie::mul(accs_a2v, prelu_in_brst);
            auto  acc_acc32_a3 = aie::mul(accs_a3v, prelu_in_brst);

            Obuff_leaky0.insert(0, acc_acc32_a0. template to_vector_sign<int8>(1, (params.shift_out_1 + params.prelu_shift)));
            Obuff_leaky0.insert(1, acc_acc32_a1. template to_vector_sign<int8>(1, (params.shift_out_1 + params.prelu_shift)));
            Obuff_leaky1.insert(0, acc_acc32_a2. template to_vector_sign<int8>(1, (params.shift_out_1 + params.prelu_shift)));
            Obuff_leaky1.insert(1, acc_acc32_a3. template to_vector_sign<int8>(1, (params.shift_out_1 + params.prelu_shift)));
        }
        if (params.act_type_1 == Leaky_Prelu) {
        	Obuff_v = aie::max( Obuff_v, Obuff_leaky0, sign_res );
        	Obuff_vt = aie::max( Obuff_vt, Obuff_leaky1, sign_res );
        } else {
#endif
            Obuff_v = aie::min( Obuff_v, min_res_v, sign_res&&sign_res_h );
            Obuff_vt = aie::min( Obuff_vt, min_res_v, sign_res&&sign_res_h );
#if (LEAKY_PRELU_CASE)
        }
#endif
        Obuff_v = shuffle(Obuff_v, Obuff_v, params.shfl_o);
        aie::store_v((int8 *)pOut8, Obuff_v.extract<32>(0));
        pOut8 = byte_incr(pOut8, params.incS0);
        aie::store_v((int8 *)pOut8, Obuff_v.extract<32>(1));
        pOut8 = byte_incr(pOut8, params.incCS1);
        
        Obuff_vt = shuffle(Obuff_vt, Obuff_vt, params.shfl_o);
        aie::store_v((int8 *)pOut8, Obuff_vt.extract<32>(0));
        pOut8 = byte_incr(pOut8, params.incS0);
        aie::store_v((int8 *)pOut8, Obuff_vt.extract<32>(1));

        pOut8 = add_3d_byte(pOut8, params.incCS3, params.numCS1, cntS1,
                            params.incCS1, params.numCS2, cntS2, params.incCS2);

      } else if (kc_out == KC_TDM32) {
        // Storing from dircetly from accumulator to memory requires native size
        // store_v CRVO-7210
        store_accs<ld>((int32 __aie_dm_resource_c * restrict &)pOut32a,
                       (int32 __aie_dm_resource_d * restrict &)pOut32b, accs_v,
                       incrT0);
      }
    }
    chess_separator();
}

#ifdef CONV2D_A16W8_QdQ_MODE 
template <uint8_t batch_size, act_t act_type, typename dtype_ifm, typename dtype_wts, typename dtype_ofm, typename config_ifm, typename config_wts, typename config_ofm> 
void conv2d
(
        adf::input_buffer_conf<dtype_ifm, config_ifm>  & __restrict ifm,
        adf::input_buffer_conf<dtype_wts, config_wts>  & __restrict wts,
        adf::output_buffer_conf<dtype_ofm, config_ofm> &  ofm,
        conv3d_params_t            &conv2d_params,
        QDQParams                  &qdq_params
) 
#else
template <uint8_t batch_size, act_t act_type, typename dtype_ifm, typename dtype_wts, typename dtype_ofm, typename config_ifm, typename config_wts, typename config_ofm> 
void conv2d
(
        adf::input_buffer_conf<dtype_ifm, config_ifm>  & __restrict ifm,
        adf::input_buffer_conf<dtype_wts, config_wts>  & __restrict wts,
        adf::output_buffer_conf<dtype_ofm, config_ofm> &  ofm,
        conv3d_params_t            &conv2d_params
)
#endif
{

    uint32 psum_offset = conv2d_params.psum_offset_a ;//+ conv2d_params.ofm_offset_top + conv2d_params.ofm_offset_left*8*batch_size;
    //int * psum_0  = (int32 *)(adf::io_buffer_access::getBuf(ofm, 0) + psum_offset);
    //int * psum_1  = (int32 *)(adf::io_buffer_access::getBuf(ofm, 1) + psum_offset);  
    //int * psum_0  = (int32 *)(0x70000 + 8192);
    //int * psum_1  = (int32 *)(0x74000 + 8192);   
    int * psum_0  = (int32 *)(0x70000 + psum_offset);
    int * psum_1  = (int32 *)(0x74000 + psum_offset);   
    
    conv2d_params.curr_iter_cnt++;
    
    #ifdef CONV2D_A16W8_MODE        
       
        #ifdef CONV2D_A16W8_QdQ_MODE 
           
        direct_conv_int8x8_tdm_split_ld2_template<1, dtype_ifm, dtype_wts, int32>(
                            (dtype_ifm *)ifm.data(),
                            (dtype_wts*)wts.data() + conv2d_params.wgt_offset,
                            (int32 *)psum_0,
                            (int32 *)psum_1,
                            (dtype_ofm*)ofm.data(), conv2d_params, conv2d_params.curr_iter_cnt==1,
                            KC_TDM32, KC_TDM32);        
        
        if(conv2d_params.curr_iter_cnt == conv2d_params.iter_cnt) {                
            
            adf::input_buffer<int32>  ifm_port({ (int *)psum_0,       (uint32_t)conv2d_params.conv_ofm_h*conv2d_params.conv_ofm_w*conv2d_params.conv_ofm_c/2, 0, (uint32_t)conv2d_params.conv_ofm_h*conv2d_params.conv_ofm_w*conv2d_params.conv_ofm_c/2 });
            
            adf::input_buffer<int32>  cof_port({ (int *)wts.data()+16,   (uint32_t)(16 + 2*conv2d_params.conv_ofm_c + conv2d_params.conv_ofm_c*2), 0, (uint32_t)(16 + 2*conv2d_params.conv_ofm_c + conv2d_params.conv_ofm_c*2) });
            adf::output_buffer<dtype_ofm> ofm_port({ (dtype_ofm *)ofm.data(), (uint32_t)conv2d_params.conv_ofm_h*conv2d_params.conv_ofm_w*conv2d_params.conv_ofm_c, 0, (uint32_t)conv2d_params.conv_ofm_h*conv2d_params.conv_ofm_w*conv2d_params.conv_ofm_c });
              
            wrapper_sym<int32, int32, int64, dtype_ofm, M_gran, N_gran, M_tile, N_tile, 1, 1, 1>(ifm_port, cof_port, ofm_port, qdq_params);
            
            conv2d_params.curr_iter_cnt = 0;                
        }                    
       
        #else //if not CONV2D_A16W8_QdQ_MODE
        if(conv2d_params.curr_iter_cnt != conv2d_params.iter_cnt){            
            direct_conv_int8x8_tdm_split_ld2_template<1, dtype_ifm, dtype_wts, int32>(
                                (dtype_ifm *)ifm.data(),
                                (dtype_wts*)wts.data() + conv2d_params.wgt_offset,
                                (int32 *)psum_0,
                                (int32 *)psum_1,
                                (dtype_ofm*)ofm.data(), conv2d_params, conv2d_params.curr_iter_cnt==1,
                                KC_TDM32, KC_TDM32);    
        } else {                
            direct_conv_int8x8_tdm_split_ld2_template_final<1, dtype_ifm, dtype_wts, int32>(
                            (dtype_ifm *)ifm.data(),
                            (dtype_wts*)wts.data() + conv2d_params.wgt_offset,
                            (int32 *)psum_0,
                            (int32 *)psum_1,
                            (dtype_ofm*)ofm.data(), conv2d_params, conv2d_params.curr_iter_cnt==1,
                            KC_TDM32, KC_TDM32); 
            conv2d_params.curr_iter_cnt = 0;        
        }
    
        #endif  //endif of CONV2D_A16W8_QdQ_MODE   
                               
    #else //if not CONV2D_A16W8_MODE
    if(conv2d_params.iter_cnt==1){
        biased_conv_int8x8_tdm_split_ld2_template<3, 1>(
                    (int8 *)ifm.data(),
                    (int8*)wts.data() + conv2d_params.wgt_offset,
                    (int8*)wts.data() + 64,
                    (int32 *)psum_0,
                    (int32 *)psum_1,
                    (int8*)ofm.data(), conv2d_params, 1, 1, KC_ZERO,
                    KC_RESULT8);     

    
    } else {
    
        if(conv2d_params.curr_iter_cnt != conv2d_params.iter_cnt)
        {

            direct_conv_int8x8_tdm_split_ld2_template<1, int8, int8, int32>(
                            (int8 *)ifm.data(),
                            (int8*)wts.data() + conv2d_params.wgt_offset,
                            (int32 *)psum_0,
                            (int32 *)psum_1,
                            (int8*)ofm.data(), conv2d_params, conv2d_params.curr_iter_cnt==1,
                            KC_TDM32, KC_TDM32);   
                                
        } else {
            biased_conv_int8x8_tdm_split_ld2_template<3, 1>(
                            (int8 *)ifm.data(),
                            (int8*)wts.data() + conv2d_params.wgt_offset,
                            (int8*)wts.data() + 64,
                            (int32 *)psum_0,
                            (int32 *)psum_1,
                            (int8*)ofm.data(), conv2d_params, conv2d_params.curr_iter_cnt==1, 1, KC_TDM32,
                            KC_RESULT8); 
            conv2d_params.curr_iter_cnt = 0;     

        }
    }
    #endif //endif of CONV2D_A16W8_MODE
}
#endif //#if __AIE_ARCH__ >= 21

#endif //__CONV2D_OPT_H__

