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
 * 2D BF16 Convolution Kernel for AI Engine
 *
 * Performs 2d bf26 convolution on data present in "ifms" using weights from "wts"
 * Result is stored in "ofms".
 *
 */

#ifndef __CONV2D_BF16_H__
#define __CONV2D_BF16_H__

#include "conv2d_bf16_helper.h"
#include <adf.h>
#include "aie.hpp"
#include "aie_api/utils.hpp"
#include "utils.h"              // Contains some utility functions and definitions related to intrinsics usage
#include "mllib_const.h"        // Constants shared across graphs like activation type
#include "conv2d_bf16_params.h" // Parameters like increments and wraps needed by kernel

using namespace aie;

#if __AIE_ARCH__ == 20
// TODO:(zhonglin) bfloat16 kernel on aie2p/aie2ps
// TODO:(zhonglin) support dtype_partial_sum in case we need fp32 partial sum
template <uint8_t batch_size, act_t act_type>
INLINE void conv2d_generic(bfloat16* __restrict input,
                           bfloat16* __restrict weights,
                           bfloat16* __restrict acc_in,
                           bfloat16* __restrict output,
                           conv2d_bf16_params_t& conv2d_params,
                           out_mode_t out_mode) {
    bfloat16 bar = 0;
    {
#ifdef USE_RTP_ACT
        int out_sign = conv2d_params.out_sign;
#else
        int out_sign = 1 - act_type; // Note enum for act types are: 0,1,2 -> linear, relu, lrelu
#endif
        if (out_sign == 1) {
            bar = -3.38953139e38;
        }
    }

    bfloat16 DM_BANK(b)* p_init16 = (bfloat16 DM_BANK(b)*)acc_in;
    bfloat16 DM_BANK(b) * p_out;
    bfloat16 DM_BANK(b) * p_out2;
    p_out = (conv2d_params.psum_out == 0)
                ? (bfloat16 DM_BANK(b)*)output
                : (bfloat16 DM_BANK(b)*)((int*)output + (conv2d_params.psum_buff_offset >> 1));
    p_out2 = p_out;

    bfloat16 IFM_DM_BANK* p_in = (bfloat16 IFM_DM_BANK*)input;
    bfloat16 IFM_DM_BANK* p_in_start = p_in;
    bfloat16 WT_DM_BANK* p_w = (bfloat16 WT_DM_BANK*)weights;

    bfloat16 WT_DM_BANK* p_bias =
        (bfloat16 WT_DM_BANK*)((int*)p_w - conv2d_params.hdr_len + LAYER_PARAM_WORDS); // Pointer to load bias.
    int psum_in_shift = 0;
    // int psum_in_shift = (1 - conv2d_params.psum_in) * conv2d_params.shift_bias_init;
    int s1, s2, s3, s4; // TODO: better move {s1, s2, s3, s4, bf16_step_align} to parameter setup
    if (conv2d_params.shfl_0 == T512_1x2_lo) {
        s1 = T512_1x2_lo;
        s2 = T512_1x2_hi;
        s3 = T512_1x2_hi;
        s4 = T512_1x2_lo;
    }
    if (conv2d_params.shfl_0 == T64_8x2_lo) {
        s1 = T128_4x2_lo;
        s2 = T128_4x2_lo;
        s3 = T512_1x2_lo;
        s4 = T512_1x2_hi;
    }
    if (conv2d_params.shfl_0 == T128_4x2_lo) {
        s1 = T256_2x2_lo;
        s2 = T256_2x2_lo;
        s3 = T512_1x2_lo;
        s4 = T512_1x2_hi;
    }
    int bf16_step_align = 0;
    if (conv2d_params.step_align == 0) {
        bf16_step_align = 0;
    }
    if (conv2d_params.step_align == 2) {
        bf16_step_align = 3;
    }
    if (conv2d_params.step_align == 3) {
        bf16_step_align = 4;
    }

    int zero_psum = 1 - conv2d_params.psum_in;
    int zero_casc = 1;
    if (out_mode == FULL) {
        zero_casc = 0;
    }

    using mmul = mmul<4, 8, 4, bfloat16, bfloat16>;

    vector<bfloat16, 32> Xbuff_r00, Xbuff_r01, Xbuff_r20, Xbuff_r21;

    SETUP_ITR3D_COUNTERS(iterator_inner);
    SETUP_ITR3D_COUNTERS(iterator_outer);
    SETUP_ITR3D_COUNTERS(iterator_weights);
    SETUP_ITR3D_COUNTERS(iterator_psum);
    SETUP_ITR3D_COUNTERS(iterator_pout);

    int frac = 0;
    int chnd = 32;
    int chnd_T = 64;

    if constexpr (batch_size < 4) {
        p_in += 15;
    }
    int32_t conv12_psum_store_offset = conv2d_params.conv_type == (uint8_t)12 ? -128 : 0;
    DECL_ITR3D(iterator_psum);
    SETUP_ITR3D(iterator_psum, 0, 0, conv2d_params.inc_Xo_T - chnd_T - 32 + conv12_psum_store_offset,
                conv2d_params.num_X, conv2d_params.inc_Co_T - chnd_T - 32 + conv12_psum_store_offset);

    DECL_ITR3D(iterator_pout);
    SETUP_ITR3D(iterator_pout, 0, 0, conv2d_params.inc_Xo_T - chnd_T + conv12_psum_store_offset, conv2d_params.num_X,
                conv2d_params.inc_Co_T - chnd_T + conv12_psum_store_offset);

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2, ) 
    for (unsigned j = 0; j < conv2d_params.outer_g; j++) {
            if constexpr(batch_size < 4) {
                int pdel = ((long)(p_in));
                frac = (pdel & 31) + 34;
            }

            //
            vector<bfloat16, 32> vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3;
            if constexpr (batch_size < 4) {
                vector<bfloat16, 32> Sbuff00, Sbuff01, Sbuff10, Sbuff11;

                Sbuff00 = MY_LOAD_V<32>(p_in);
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);

                Sbuff01 = MY_LOAD_V<32>(p_in);
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);

                Sbuff10 = MY_LOAD_V<32>(p_in);
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                Sbuff11 = MY_LOAD_V<32>(p_in);

                Xbuff_r00 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r00, Sbuff00, bf16_step_align, frac));
                Xbuff_r01 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r01, Sbuff01, bf16_step_align, frac));

                Xbuff_r20 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r20, Sbuff10, bf16_step_align, frac));
                Xbuff_r21 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r21, Sbuff11, bf16_step_align, frac));

                p_in = INCR_ITR3D_PARAM_BF16(p_in, iterator_inner, conv2d_params);

                vector<uint8_t, 64> u00, u01, u20, u21;
                u00 = Xbuff_r00.template cast_to<uint8_t>();
                u01 = Xbuff_r01.template cast_to<uint8_t>();
                u20 = Xbuff_r20.template cast_to<uint8_t>();
                u21 = Xbuff_r21.template cast_to<uint8_t>();

                vector<uint8_t, 64> r0, r1, r2, r3, r1_, r2_;
                r0 = shuffle(u00, u01, s1);
                r1_ = shuffle(u20, u21, s1);
                r2_ = shuffle(u00, u01, s2);
                r3 = shuffle(u20, u21, s2);

                r1 = shuffle(r1_, r2_, s3);
                r2 = shuffle(r1_, r2_, s4);
                vec_Xbuff0 = r0.cast_to<bfloat16>();
                vec_Xbuff1 = r1.cast_to<bfloat16>();
                vec_Xbuff2 = r2.cast_to<bfloat16>();
                vec_Xbuff3 = r3.cast_to<bfloat16>();
            } else {
                vec_Xbuff0 = load_v<32>(p_in);
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                vec_Xbuff1 = load_v<32>(p_in);
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                vec_Xbuff2 = load_v<32>(p_in);
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                vec_Xbuff3 = load_v<32>(p_in);

                p_in = INCR_ITR3D_PARAM_BF16(p_in, iterator_inner, conv2d_params);
            }

            vector<bfloat16, 32> Ybuff00, Ybuff01, Ybuff10, Ybuff11;
            Ybuff00 = load_v<32>(p_w);
            p_w = byte_incr(p_w, 32 * 2);
            Ybuff01 = load_v<32>(p_w);
            p_w = byte_incr(p_w, 32 * 2);
            Ybuff10 = load_v<32>(p_w);
            p_w = byte_incr(p_w, 32 * 2);
            Ybuff11 = load_v<32>(p_w);
            p_w = byte_incr(p_w, 32 * 2);

            //
            aie::accum<accfloat, 16> accum_00, accum_10, accum_20, accum_30, accum_40, accum_50, accum_60, accum_70;
            aie::accum<accfloat, 16> accum_01, accum_11, accum_21, accum_31, accum_41, accum_51, accum_61, accum_71;

            accum_00 = aie::from_vector<accfloat>(aie::load_v<16>(p_init16));
            p_init16 = byte_incr(p_init16, 32);
            accum_01 = aie::from_vector<accfloat>(aie::load_v<16>(p_init16));
            p_init16 = byte_incr(p_init16, 32);

            accum_10 = aie::from_vector<accfloat>(aie::load_v<16>(p_init16));
            p_init16 = byte_incr(p_init16, 32);
            accum_11 = aie::from_vector<accfloat>(aie::load_v<16>(p_init16));
            p_init16 = byte_incr(p_init16, conv2d_params.inc_ST_0 - chnd_T - 32);

            accum_20 = aie::from_vector<accfloat>(aie::load_v<16>(p_init16));
            p_init16 = byte_incr(p_init16, 32);
            accum_21 = aie::from_vector<accfloat>(aie::load_v<16>(p_init16));
            p_init16 = byte_incr(p_init16, 32);

            accum_30 = aie::from_vector<accfloat>(aie::load_v<16>(p_init16));
            p_init16 = byte_incr(p_init16, 32);
            accum_31 = aie::from_vector<accfloat>(aie::load_v<16>(p_init16));
            p_init16 = byte_incr(p_init16, conv2d_params.inc_ST_1 - chnd_T - 32);

            accum_40 = aie::from_vector<accfloat>(aie::load_v<16>(p_init16));
            p_init16 = byte_incr(p_init16, 32);
            accum_41 = aie::from_vector<accfloat>(aie::load_v<16>(p_init16));
            p_init16 = byte_incr(p_init16, 32);

            accum_50 = aie::from_vector<accfloat>(aie::load_v<16>(p_init16));
            p_init16 = byte_incr(p_init16, 32);
            accum_51 = aie::from_vector<accfloat>(aie::load_v<16>(p_init16));
            p_init16 = byte_incr(p_init16, conv2d_params.inc_ST_0 - chnd_T - 32);

            accum_60 = aie::from_vector<accfloat>(aie::load_v<16>(p_init16));
            p_init16 = byte_incr(p_init16, 32);
            accum_61 = aie::from_vector<accfloat>(aie::load_v<16>(p_init16));
            p_init16 = byte_incr(p_init16, 32);

            accum_70 = aie::from_vector<accfloat>(aie::load_v<16>(p_init16));
            p_init16 = byte_incr(p_init16, 32);
            accum_71 = aie::from_vector<accfloat>(aie::load_v<16>(p_init16));
            p_init16 = INCR_ITR3D(p_init16, iterator_psum); // not need BF16, because it was int16

            int32_t bias_incr = 128 * iterator_weights_cnt1;
            bfloat16 WT_DM_BANK* p_bias_tmp = byte_incr(p_bias, bias_incr);
            accum<accfloat, 16> bias_00, bias_01, bias_10, bias_11;
            ld_b(bias_00, bias_01, psum_in_shift, p_bias_tmp);
            p_bias_tmp = byte_incr(p_bias_tmp, 64);
            ld_b(bias_10, bias_11, psum_in_shift, p_bias_tmp);

            mmul Cbuff00(aie::op_zero(accum_00, zero_psum));
            mmul Cbuff10(aie::op_zero(accum_10, zero_psum));
            mmul Cbuff20(aie::op_zero(accum_20, zero_psum));
            mmul Cbuff30(aie::op_zero(accum_30, zero_psum));
            mmul Cbuff40(aie::op_zero(accum_40, zero_psum));
            mmul Cbuff50(aie::op_zero(accum_50, zero_psum));
            mmul Cbuff60(aie::op_zero(accum_60, zero_psum));
            mmul Cbuff70(aie::op_zero(accum_70, zero_psum));

            mmul Cbuff01(aie::op_zero(accum_01, zero_psum));
            mmul Cbuff11(aie::op_zero(accum_11, zero_psum));
            mmul Cbuff21(aie::op_zero(accum_21, zero_psum));
            mmul Cbuff31(aie::op_zero(accum_31, zero_psum));
            mmul Cbuff41(aie::op_zero(accum_41, zero_psum));
            mmul Cbuff51(aie::op_zero(accum_51, zero_psum));
            mmul Cbuff61(aie::op_zero(accum_61, zero_psum));
            mmul Cbuff71(aie::op_zero(accum_71, zero_psum));

            Cbuff00.mac(vec_Xbuff0, Ybuff00);
            Cbuff10.mac(vec_Xbuff1, Ybuff00);
            Cbuff20.mac(vec_Xbuff2, Ybuff00);
            Cbuff30.mac(vec_Xbuff3, Ybuff00);
            Cbuff40.mac(vec_Xbuff0, Ybuff10);
            Cbuff50.mac(vec_Xbuff1, Ybuff10);
            Cbuff60.mac(vec_Xbuff2, Ybuff10);
            Cbuff70.mac(vec_Xbuff3, Ybuff10);

            Cbuff01.mac(vec_Xbuff0, Ybuff01);
            Cbuff11.mac(vec_Xbuff1, Ybuff01);
            Cbuff21.mac(vec_Xbuff2, Ybuff01);
            Cbuff31.mac(vec_Xbuff3, Ybuff01);
            Cbuff41.mac(vec_Xbuff0, Ybuff11);
            Cbuff51.mac(vec_Xbuff1, Ybuff11);
            Cbuff61.mac(vec_Xbuff2, Ybuff11);
            Cbuff71.mac(vec_Xbuff3, Ybuff11);

            Cbuff00 = aie::add(bias_00, Cbuff00.to_accum());
            Cbuff10 = aie::add(bias_00, Cbuff10.to_accum());
            Cbuff20 = aie::add(bias_00, Cbuff20.to_accum());
            Cbuff30 = aie::add(bias_00, Cbuff30.to_accum());
            Cbuff40 = aie::add(bias_10, Cbuff40.to_accum());
            Cbuff50 = aie::add(bias_10, Cbuff50.to_accum());
            Cbuff60 = aie::add(bias_10, Cbuff60.to_accum());
            Cbuff70 = aie::add(bias_10, Cbuff70.to_accum());

            Cbuff01 = aie::add(bias_01, Cbuff01.to_accum());
            Cbuff11 = aie::add(bias_01, Cbuff11.to_accum());
            Cbuff21 = aie::add(bias_01, Cbuff21.to_accum());
            Cbuff31 = aie::add(bias_01, Cbuff31.to_accum());
            Cbuff41 = aie::add(bias_11, Cbuff41.to_accum());
            Cbuff51 = aie::add(bias_11, Cbuff51.to_accum());
            Cbuff61 = aie::add(bias_11, Cbuff61.to_accum());
            Cbuff71 = aie::add(bias_11, Cbuff71.to_accum());

            if constexpr (batch_size < 4) {
                int pdel = ((long)(p_in));
                frac = (pdel & 31) + 34;
            }

            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(4, ) 
            for (int i = 0; i < conv2d_params.inner_g - 1; i++) {             
                    vector<bfloat16, 32> vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3;
                    if constexpr (batch_size < 4) {
                        vector<bfloat16, 32> Sbuff00, Sbuff01, Sbuff10, Sbuff11;

                        Sbuff00 = MY_LOAD_V<32>(p_in);
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);

                        Sbuff01 = MY_LOAD_V<32>(p_in);
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);

                        Sbuff10 = MY_LOAD_V<32>(p_in);
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                        Sbuff11 = MY_LOAD_V<32>(p_in);

                        Xbuff_r00 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r00, Sbuff00, bf16_step_align, frac));
                        Xbuff_r01 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r01, Sbuff01, bf16_step_align, frac));

                        Xbuff_r20 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r20, Sbuff10, bf16_step_align, frac));
                        Xbuff_r21 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r21, Sbuff11, bf16_step_align, frac));

                        p_in = INCR_ITR3D_PARAM_BF16(p_in, iterator_inner, conv2d_params);

                        vector<uint8_t, 64> u00, u01, u20, u21;
                        u00 = Xbuff_r00.template cast_to<uint8_t>();
                        u01 = Xbuff_r01.template cast_to<uint8_t>();
                        u20 = Xbuff_r20.template cast_to<uint8_t>();
                        u21 = Xbuff_r21.template cast_to<uint8_t>();

                        vector<uint8_t, 64> r0, r1, r2, r3, r1_, r2_;
                        r0 = shuffle(u00, u01, s1);
                        r1_ = shuffle(u20, u21, s1);
                        r2_ = shuffle(u00, u01, s2);
                        r3 = shuffle(u20, u21, s2);

                        r1 = shuffle(r1_, r2_, s3);
                        r2 = shuffle(r1_, r2_, s4);
                        vec_Xbuff0 = r0.cast_to<bfloat16>();
                        vec_Xbuff1 = r1.cast_to<bfloat16>();
                        vec_Xbuff2 = r2.cast_to<bfloat16>();
                        vec_Xbuff3 = r3.cast_to<bfloat16>();
                    } else {
                        vec_Xbuff0 = load_v<32>(p_in);
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                        vec_Xbuff1 = load_v<32>(p_in);
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                        vec_Xbuff2 = load_v<32>(p_in);
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                        vec_Xbuff3 = load_v<32>(p_in);

                        p_in = INCR_ITR3D_PARAM_BF16(p_in, iterator_inner, conv2d_params);
                    }

                    vector<bfloat16, 32> Ybuff00, Ybuff01, Ybuff10, Ybuff11;
                    Ybuff00 = load_v<32>(p_w);
                    p_w = byte_incr(p_w, 32 * 2);
                    Ybuff01 = load_v<32>(p_w);
                    p_w = byte_incr(p_w, 32 * 2);
                    Ybuff10 = load_v<32>(p_w);
                    p_w = byte_incr(p_w, 32 * 2);
                    Ybuff11 = load_v<32>(p_w);
                    p_w = byte_incr(p_w, 32 * 2);

                    Cbuff00.mac(vec_Xbuff0, Ybuff00);
                    Cbuff10.mac(vec_Xbuff1, Ybuff00);
                    Cbuff20.mac(vec_Xbuff2, Ybuff00);
                    Cbuff30.mac(vec_Xbuff3, Ybuff00);
                    Cbuff40.mac(vec_Xbuff0, Ybuff10);
                    Cbuff50.mac(vec_Xbuff1, Ybuff10);
                    Cbuff60.mac(vec_Xbuff2, Ybuff10);
                    Cbuff70.mac(vec_Xbuff3, Ybuff10);

                    Cbuff01.mac(vec_Xbuff0, Ybuff01);
                    Cbuff11.mac(vec_Xbuff1, Ybuff01);
                    Cbuff21.mac(vec_Xbuff2, Ybuff01);
                    Cbuff31.mac(vec_Xbuff3, Ybuff01);
                    Cbuff41.mac(vec_Xbuff0, Ybuff11);
                    Cbuff51.mac(vec_Xbuff1, Ybuff11);
                    Cbuff61.mac(vec_Xbuff2, Ybuff11);
                    Cbuff71.mac(vec_Xbuff3, Ybuff11);

                    if constexpr (batch_size < 4) {
                        int pdel = ((long)(p_in));
                        frac = (pdel & 31) + 34;
                    }
                }

            p_in = INCR_ITR3D_PARAM_BF16(p_in, iterator_outer, conv2d_params);
            p_w = INCR_ITR3D_PARAM_BF16(p_w, iterator_weights, conv2d_params);

            // store partial result
            aie::store_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0),
                         Cbuff20.to_accum().to_vector<bfloat16>());
            aie::store_v(byte_incr(p_out, conv12_psum_store_offset + 32 + conv2d_params.inc_ST_0),
                         Cbuff21.to_accum().to_vector<bfloat16>());
            aie::store_v(byte_incr(p_out, conv12_psum_store_offset + 64 + conv2d_params.inc_ST_0),
                         Cbuff30.to_accum().to_vector<bfloat16>());
            aie::store_v(byte_incr(p_out, conv12_psum_store_offset + 96 + conv2d_params.inc_ST_0),
                         Cbuff31.to_accum().to_vector<bfloat16>());
            aie::store_v(p_out, Cbuff00.to_accum().to_vector<bfloat16>());
            aie::store_v(byte_incr(p_out, 32), Cbuff01.to_accum().to_vector<bfloat16>());
            aie::store_v(byte_incr(p_out, 64), Cbuff10.to_accum().to_vector<bfloat16>());
            aie::store_v(byte_incr(p_out, 96), Cbuff11.to_accum().to_vector<bfloat16>());
            p_out = byte_incr(p_out, conv2d_params.inc_ST_0);
            p_out = byte_incr(p_out, conv2d_params.inc_ST_1);

            aie::store_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0),
                         Cbuff60.to_accum().to_vector<bfloat16>());
            aie::store_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0 + 32),
                         Cbuff61.to_accum().to_vector<bfloat16>());
            aie::store_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0 + 64),
                         Cbuff70.to_accum().to_vector<bfloat16>());
            aie::store_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0 + 96),
                         Cbuff71.to_accum().to_vector<bfloat16>());
            aie::store_v(p_out, Cbuff40.to_accum().to_vector<bfloat16>());
            aie::store_v(byte_incr(p_out, 32), Cbuff41.to_accum().to_vector<bfloat16>());
            aie::store_v(byte_incr(p_out, 64), Cbuff50.to_accum().to_vector<bfloat16>());
            aie::store_v(byte_incr(p_out, 96), Cbuff51.to_accum().to_vector<bfloat16>());
            p_out = byte_incr(p_out, conv2d_params.inc_ST_0 + 64);
            p_out = INCR_ITR3D(p_out, iterator_pout);
        }
    // shuffle 4x4 + 4x4 to 4x8
    // if shfl_0 != T512_1x2_lo, then effective write per outer loop iter is 128 elements , not 256 elements
    if (out_mode == FULL) {
        uint8_t* p_out_w = (uint8_t*)p_out2;
        uint8_t* p_out_r = (uint8_t*)p_out2;
        int res_bound = conv2d_params.shfl_0 == T512_1x2_lo ? conv2d_params.outer_g * 2 : conv2d_params.outer_g;
        aie::vector<uint8_t, 64> ts0, ts1, ts2, ts3;

        ts0.insert(0, aie::load_v<32>(p_out_r));
        ts1.insert(0, aie::load_v<32>(p_out_r + 32));
        ts0.insert(1, aie::load_v<32>(p_out_r + 64));
        ts1.insert(1, aie::load_v<32>(p_out_r + 96));
        ts2.insert(0, aie::load_v<32>(p_out_r + 128 + conv12_psum_store_offset));
        ts3.insert(0, aie::load_v<32>(p_out_r + 160 + conv12_psum_store_offset));
        ts2.insert(1, aie::load_v<32>(p_out_r + 192 + conv12_psum_store_offset));
        ts3.insert(1, aie::load_v<32>(p_out_r + 224 + conv12_psum_store_offset));
        p_out_r = byte_incr(p_out_r, 256 + conv12_psum_store_offset);
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(2, ) 
        for (unsigned j = 0; j < res_bound; j++)
            chess_modulo_scheduling_budget_ratio(200) {
                aie::vector<uint8_t, 64> r0, r1, r2, r3;
                r0 = shuffle(ts0, ts1, T64_2x8_lo);
                r1 = shuffle(ts0, ts1, T64_2x8_hi);
                r2 = shuffle(ts2, ts3, T64_2x8_lo);
                r3 = shuffle(ts2, ts3, T64_2x8_hi);

                aie::vector<bfloat16, 32> v0, v1, v2, v3;
                v0 = r0.cast_to<bfloat16>();
                v1 = r1.cast_to<bfloat16>();
                v2 = r2.cast_to<bfloat16>();
                v3 = r3.cast_to<bfloat16>();

                if constexpr (act_type == LINEAR || act_type == RELU) {
                    store_v((bfloat16*)p_out_w, aie::max(v0, bar));
                    p_out_w = byte_incr(p_out_w, 64);
                    store_v((bfloat16*)p_out_w, aie::max(v1, bar));
                    p_out_w = byte_incr(p_out_w, 64 + conv12_psum_store_offset);
                    store_v((bfloat16*)p_out_w, aie::max(v2, bar));
                    p_out_w = byte_incr(p_out_w, 64);
                    store_v((bfloat16*)p_out_w, aie::max(v3, bar));
                    p_out_w = byte_incr(p_out_w, 64);
                } else { // leaky relu
                    bfloat16 alpha = *reinterpret_cast<bfloat16*>(&conv2d_params.lrelu_alpha);
                    leaky_relu_compute<32>(v0, alpha, (bfloat16*)p_out_w);
                    p_out_w = byte_incr(p_out_w, 64);
                    leaky_relu_compute<32>(v1, alpha, (bfloat16*)p_out_w);
                    p_out_w = byte_incr(p_out_w, 64 + conv12_psum_store_offset);
                    leaky_relu_compute<32>(v2, alpha, (bfloat16*)p_out_w);
                    p_out_w = byte_incr(p_out_w, 64);
                    leaky_relu_compute<32>(v3, alpha, (bfloat16*)p_out_w);
                    p_out_w = byte_incr(p_out_w, 64);
                }

                ts0.insert(0, aie::load_v<32>(p_out_r));
                ts1.insert(0, aie::load_v<32>(p_out_r + 32));
                ts0.insert(1, aie::load_v<32>(p_out_r + 64));
                ts1.insert(1, aie::load_v<32>(p_out_r + 96));
                ts2.insert(0, aie::load_v<32>(p_out_r + 128 + conv12_psum_store_offset));
                ts3.insert(0, aie::load_v<32>(p_out_r + 160 + conv12_psum_store_offset));
                ts2.insert(1, aie::load_v<32>(p_out_r + 192 + conv12_psum_store_offset));
                ts3.insert(1, aie::load_v<32>(p_out_r + 224 + conv12_psum_store_offset));
                p_out_r = byte_incr(p_out_r, 256 + conv12_psum_store_offset);
            }
    } else if (conv2d_params.curr_depth_iter == 0) { // the last iter
        uint8_t* p_out_w = (uint8_t*)p_out2;
        uint8_t* p_out_r = (uint8_t*)p_out2;
        int res_bound = conv2d_params.shfl_0 == T512_1x2_lo ? conv2d_params.outer_g * 2 : conv2d_params.outer_g;
        aie::vector<uint8_t, 64> ts0, ts1, ts2, ts3;

        ts0.insert(0, aie::load_v<32>(p_out_r));
        ts1.insert(0, aie::load_v<32>(p_out_r + 32));
        ts0.insert(1, aie::load_v<32>(p_out_r + 64));
        ts1.insert(1, aie::load_v<32>(p_out_r + 96));
        ts2.insert(0, aie::load_v<32>(p_out_r + 128));
        ts3.insert(0, aie::load_v<32>(p_out_r + 160));
        ts2.insert(1, aie::load_v<32>(p_out_r + 192));
        ts3.insert(1, aie::load_v<32>(p_out_r + 224));
        p_out_r = byte_incr(p_out_r, 256);
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(2, ) 
        for (unsigned j = 0; j < res_bound; j++)chess_modulo_scheduling_budget_ratio(200){
                aie::vector<uint8_t, 64> r0, r1, r2, r3;
                r0 = shuffle(ts0, ts1, T64_2x8_lo);
                r1 = shuffle(ts0, ts1, T64_2x8_hi);
                r2 = shuffle(ts2, ts3, T64_2x8_lo);
                r3 = shuffle(ts2, ts3, T64_2x8_hi);
                aie::store_v(p_out_w, r0);
                aie::store_v(p_out_w + 64, r1);
                aie::store_v(p_out_w + 128, r2);
                aie::store_v(p_out_w + 192, r3);
                p_out_w = byte_incr(p_out_w, 256);

                ts0.insert(0, aie::load_v<32>(p_out_r));
                ts1.insert(0, aie::load_v<32>(p_out_r + 32));
                ts0.insert(1, aie::load_v<32>(p_out_r + 64));
                ts1.insert(1, aie::load_v<32>(p_out_r + 96));
                ts2.insert(0, aie::load_v<32>(p_out_r + 128));
                ts3.insert(0, aie::load_v<32>(p_out_r + 160));
                ts2.insert(1, aie::load_v<32>(p_out_r + 192));
                ts3.insert(1, aie::load_v<32>(p_out_r + 224));
                p_out_r = byte_incr(p_out_r, 256);
            }
    }
}

#else // AIE2P and AIE2PS

#ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
vector<bfloat16, 64> mt_8x8(vector<bfloat16, 64> vec) {
    vector<bfloat16, 64> res;

    vector<bfloat16, 32> b0 = vec.template extract<32>(0);
    vector<bfloat16, 32> b1 = vec.template extract<32>(1);

    vector<uint8, 64> v0 = b0.template cast_to<uint8_t>();
    vector<uint8, 64> v1 = b1.template cast_to<uint8_t>();

    vector<uint8, 64> r0 = shuffle(v0, v1, interleave_T16_8x8_lo);
    vector<uint8, 64> r1 = shuffle(v0, v1, interleave_T16_8x8_hi);

    res.insert(0, r0.cast_to<bfloat16>());
    res.insert(1, r1.cast_to<bfloat16>());

    return res;
}

struct m_bfp_888 : public aie::detail::C_block<bfloat16, bfloat16, 32, 64, 1> {
    m_bfp_888(const aie::accum<accfloat, 64>& acc, bool to_zero) : C_block(acc, to_zero) {}

    __aie_inline void mac(const vector<bfloat16, 64>& a, const vector<bfloat16, 64>& b) {
        accum<accfloat, 64> acc_a(a);
        accum<accfloat, 64> acc_b(b);

        v64bfp16ebs8 tmp_a = ::to_v64bfp16ebs8(acc_a);
        v64bfp16ebs8 tmp_b = ::to_v64bfp16ebs8(acc_b);

        this->data = ::mac_8x8_8x8T_conf(tmp_a, tmp_b, this->data, this->zero, 0, 0);
        this->zero = false;
    }

    __aie_inline void add(const vector<bfloat16, 64>& a) { this->data = aie::add(this->data, a); }

    __aie_inline accum<accfloat, 64> to_accum() { return this->data; }
};

template <uint8_t batch_size, act_t act_type>
INLINE void conv2d_generic(bfloat16* __restrict input,
                             bfloat16* __restrict weights,
                             bfloat16* __restrict acc_in,
                             bfloat16* __restrict output,
                             conv2d_bf16_params_t& conv2d_params,
                             out_mode_t out_mode) {
    bfloat16 bar = 0;
    {
#ifdef USE_RTP_ACT
        int out_sign = conv2d_params.out_sign;
#else
        int out_sign = 1 - act_type; // Note enum for act types are: 0,1,2 -> linear, relu, lrelu
#endif
        if (out_sign == 1) {
            bar = -3.38953139e38;
        }
    }

    bfloat16 DM_BANK(b)* p_init16 = (bfloat16 DM_BANK(b)*)acc_in;
    bfloat16 DM_BANK(b) * p_out;
    bfloat16 DM_BANK(b) * p_out2;
    p_out = (conv2d_params.psum_out == 0)
                ? (bfloat16 DM_BANK(b)*)output
                : (bfloat16 DM_BANK(b)*)((int*)output + (conv2d_params.psum_buff_offset >> 1));
    p_out2 = p_out;

    bfloat16 IFM_DM_BANK* p_in = (bfloat16 IFM_DM_BANK*)input;
    bfloat16 IFM_DM_BANK* p_in_start = p_in;
    bfloat16 WT_DM_BANK* p_w = (bfloat16 WT_DM_BANK*)weights;

    bfloat16 WT_DM_BANK* p_bias =
        (bfloat16 WT_DM_BANK*)((int*)p_w - conv2d_params.hdr_len + LAYER_PARAM_WORDS); // Pointer to load bias.
    int psum_in_shift = 0;
    // int psum_in_shift = (1 - conv2d_params.psum_in) * conv2d_params.shift_bias_init;
    int s1, s2, s3, s4; // TODO: better move {s1, s2, s3, s4, bf16_step_align} to parameter setup
    if (conv2d_params.shfl_0 == T512_1x2_lo) {
        s1 = T512_1x2_lo;
        s2 = T512_1x2_hi;
        s3 = T512_1x2_hi;
        s4 = T512_1x2_lo;
    }
    if (conv2d_params.shfl_0 == T64_8x2_lo) {
        s1 = T128_4x2_lo;
        s2 = T128_4x2_lo;
        s3 = T512_1x2_lo;
        s4 = T512_1x2_hi;
    }
    if (conv2d_params.shfl_0 == T128_4x2_lo) {
        s1 = T256_2x2_lo;
        s2 = T256_2x2_lo;
        s3 = T512_1x2_lo;
        s4 = T512_1x2_hi;
    }
    int bf16_step_align = 0;
    if (conv2d_params.step_align == 0) {
        bf16_step_align = 0;
    }
    if (conv2d_params.step_align == 2) {
        bf16_step_align = 3;
    }
    if (conv2d_params.step_align == 3) {
        bf16_step_align = 4;
    }

    int zero_psum = 1 - conv2d_params.psum_in;
    int zero_casc = 1;
    if (out_mode == FULL) {
        zero_casc = 0;
    }

    using mmul_888 = m_bfp_888;

    vector<bfloat16, 32> Xbuff_r00, Xbuff_r01, Xbuff_r20, Xbuff_r21;

    SETUP_ITR3D_COUNTERS(iterator_inner);
    SETUP_ITR3D_COUNTERS(iterator_outer);
    SETUP_ITR3D_COUNTERS(iterator_weights);
    SETUP_ITR3D_COUNTERS(iterator_psum);
    SETUP_ITR3D_COUNTERS(iterator_pout);

    int frac = 0;
    int chnd = 32;
    int chnd_T = 64;

    if constexpr (batch_size < 4) {
        p_in += 31; // 64 bytes aligned for aie2p
    }

    int32_t conv12_psum_store_offset = conv2d_params.conv_type == (uint8_t)12 ? -128 : 0;
    DECL_ITR3D(iterator_psum);
    SETUP_ITR3D(iterator_psum, 0, 0, conv2d_params.inc_Xo_T - chnd_T - 32 + conv12_psum_store_offset,
                conv2d_params.num_X, conv2d_params.inc_Co_T - chnd_T - 32 + conv12_psum_store_offset);

    DECL_ITR3D(iterator_pout);
    SETUP_ITR3D(iterator_pout, 0, 0, conv2d_params.inc_Xo_T - chnd_T + conv12_psum_store_offset, conv2d_params.num_X,
                conv2d_params.inc_Co_T - chnd_T + conv12_psum_store_offset);

    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_RANGE(2, )
    for (unsigned j = 0; j < conv2d_params.outer_g; j++) {
            if constexpr (batch_size < 4) {
                int pdel = ((long)(p_in));
                frac = (pdel & 63) + 2;
            }
            // peel one inner loop out for better performance
            vector<bfloat16, 64> vec_Xbuff0, vec_Xbuff1;
            if constexpr (batch_size < 4) {
                vector<bfloat16, 32> Sbuff00, Sbuff01, Sbuff10, Sbuff11;
                Sbuff00 = MY_LOAD_V<32>(p_in);
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                Sbuff01 = MY_LOAD_V<32>(p_in);
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                Sbuff10 = MY_LOAD_V<32>(p_in);
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                Sbuff11 = MY_LOAD_V<32>(p_in);
                Xbuff_r00 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r00, Sbuff00, bf16_step_align, frac));
                Xbuff_r01 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r01, Sbuff01, bf16_step_align, frac));
                Xbuff_r20 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r20, Sbuff10, bf16_step_align, frac));
                Xbuff_r21 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r21, Sbuff11, bf16_step_align, frac));
                p_in = INCR_ITR3D_PARAM_BF16(p_in, iterator_inner, conv2d_params);
                vector<uint8_t, 64> u00, u01, u20, u21;
                u00 = Xbuff_r00.template cast_to<uint8_t>();
                u01 = Xbuff_r01.template cast_to<uint8_t>();
                u20 = Xbuff_r20.template cast_to<uint8_t>();
                u21 = Xbuff_r21.template cast_to<uint8_t>();
                vector<uint8_t, 64> r0, r1, r2, r3, r1_, r2_;
                r0 = shuffle(u00, u01, s1);
                r1_ = shuffle(u20, u21, s1);
                r2_ = shuffle(u00, u01, s2);
                r3 = shuffle(u20, u21, s2);
                r1 = shuffle(r1_, r2_, s3);
                r2 = shuffle(r1_, r2_, s4);
                vec_Xbuff0.insert(0, r0.cast_to<bfloat16>());
                vec_Xbuff0.insert(1, r1.cast_to<bfloat16>());
                vec_Xbuff1.insert(0, r2.cast_to<bfloat16>());
                vec_Xbuff1.insert(1, r3.cast_to<bfloat16>());
            } else {
                vec_Xbuff0.insert(0, load_v<32>(p_in));
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                vec_Xbuff0.insert(1, load_v<32>(p_in));
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                vec_Xbuff1.insert(0, load_v<32>(p_in));
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                vec_Xbuff1.insert(1, load_v<32>(p_in));
                p_in = INCR_ITR3D_PARAM_BF16(p_in, iterator_inner, conv2d_params);
            }
                vector<bfloat16, 32> Ybuff00, Ybuff01, Ybuff10, Ybuff11;
                Ybuff00 = load_v<32>(p_w);
                p_w = byte_incr(p_w, 32 * 2);
                Ybuff01 = load_v<32>(p_w);
                p_w = byte_incr(p_w, 32 * 2);
                Ybuff10 = load_v<32>(p_w);
                p_w = byte_incr(p_w, 32 * 2);
                Ybuff11 = load_v<32>(p_w);
                p_w = byte_incr(p_w, 32 * 2);


            aie::accum<accfloat, 64> accum_0, accum_1, accum_2, accum_3;

#ifdef __X86SIM__
            accum_0.insert(0, aie::from_vector<accfloat>(aie::load_floor_v<32>(p_init16)));
            p_init16 = byte_incr(p_init16, 64);

            accum_0.insert(1, aie::from_vector<accfloat>(aie::load_floor_v<32>(p_init16)));
            p_init16 = byte_incr(p_init16, conv2d_params.inc_ST_0 - chnd_T - 32 + 32);

            accum_1.insert(0, aie::from_vector<accfloat>(aie::load_floor_v<32>(p_init16)));
            p_init16 = byte_incr(p_init16, 64);

            accum_1.insert(1, aie::from_vector<accfloat>(aie::load_floor_v<32>(p_init16)));
            p_init16 = byte_incr(p_init16, conv2d_params.inc_ST_1 - chnd_T - 32 + 32);

            accum_2.insert(0, aie::from_vector<accfloat>(aie::load_floor_v<32>(p_init16)));
            p_init16 = byte_incr(p_init16, 64);

            accum_2.insert(1, aie::from_vector<accfloat>(aie::load_floor_v<32>(p_init16)));
            p_init16 = byte_incr(p_init16, conv2d_params.inc_ST_0 - chnd_T - 32 + 32);

            accum_3.insert(0, aie::from_vector<accfloat>(aie::load_floor_v<32>(p_init16)));
            p_init16 = byte_incr(p_init16, 64);

            accum_3.insert(1, aie::from_vector<accfloat>(aie::load_floor_v<32>(p_init16)));
            p_init16 = byte_incr(p_init16, 32);
#else
            accum_0.insert(0, aie::from_vector<accfloat>(aie::load_v<32>(p_init16)));
            p_init16 = byte_incr(p_init16, 64);

            accum_0.insert(1, aie::from_vector<accfloat>(aie::load_v<32>(p_init16)));
            p_init16 = byte_incr(p_init16, conv2d_params.inc_ST_0 - chnd_T - 32 + 32);

            accum_1.insert(0, aie::from_vector<accfloat>(aie::load_v<32>(p_init16)));
            p_init16 = byte_incr(p_init16, 64);

            accum_1.insert(1, aie::from_vector<accfloat>(aie::load_v<32>(p_init16)));
            p_init16 = byte_incr(p_init16, conv2d_params.inc_ST_1 - chnd_T - 32 + 32);

            accum_2.insert(0, aie::from_vector<accfloat>(aie::load_v<32>(p_init16)));
            p_init16 = byte_incr(p_init16, 64);

            accum_2.insert(1, aie::from_vector<accfloat>(aie::load_v<32>(p_init16)));
            p_init16 = byte_incr(p_init16, conv2d_params.inc_ST_0 - chnd_T - 32 + 32);

            accum_3.insert(0, aie::from_vector<accfloat>(aie::load_v<32>(p_init16)));
            p_init16 = byte_incr(p_init16, 64);

            accum_3.insert(1, aie::from_vector<accfloat>(aie::load_v<32>(p_init16)));
            p_init16 = byte_incr(p_init16, 32);
#endif
            p_init16 = INCR_ITR3D(p_init16, iterator_psum); // not need BF16, because it was int16

            int32_t bias_incr = 128 * iterator_weights_cnt1;
            bfloat16 WT_DM_BANK* p_bias_tmp = byte_incr(p_bias, bias_incr);

            mmul_888 Cbuff0(accum_0, zero_psum);
            mmul_888 Cbuff1(accum_1, zero_psum);
            mmul_888 Cbuff2(accum_2, zero_psum);
            mmul_888 Cbuff3(accum_3, zero_psum);
                    vector<bfloat16, 64> Y0, Y1;

            Y0.insert(0, Ybuff00);
            Y0.insert(1, Ybuff01);
            Y1.insert(0, Ybuff10);
            Y1.insert(1, Ybuff11);

#ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
            set_rnd(rnd_sym_inf);
#endif
            Cbuff0.mac(Y0, vec_Xbuff0); // row 0-7,   OC0 - OC7
            Cbuff1.mac(Y0, vec_Xbuff1); // row 8-15,  OC0 - OC7
            Cbuff2.mac(Y1, vec_Xbuff0); // row 0-7,   OC8 - OC15
            Cbuff3.mac(Y1, vec_Xbuff1); // row 8-15,  OC8 - OC15
#ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
            set_rnd(rnd_conv_even);
#endif

            vector<bfloat16, 64> bias_0, bias_1;
            bias_0.insert(0, aie::load_v<32>(p_bias_tmp));
            bias_0.insert(1, aie::load_v<32>(p_bias_tmp));
            p_bias_tmp = byte_incr(p_bias_tmp, 64);
            bias_1.insert(0, aie::load_v<32>(p_bias_tmp));
            bias_1.insert(1, aie::load_v<32>(p_bias_tmp));

            Cbuff0.add(mt_8x8(bias_0)); // OC0 - OC7
            Cbuff1.add(mt_8x8(bias_0)); // OC0 - OC7
            Cbuff2.add(mt_8x8(bias_1)); // OC8 - OC15
            Cbuff3.add(mt_8x8(bias_1)); // OC8 - OC15

            if constexpr (batch_size < 4) {
                int pdel = ((long)(p_in));
                frac = (pdel & 63) + 2;
            }

            AIE_PREPARE_FOR_PIPELINING
            AIE_LOOP_RANGE(4, )
            for (int i = 0; i < conv2d_params.inner_g-1; i++) {
                    vector<bfloat16, 64> vec_Xbuff0, vec_Xbuff1;
                    // vector<bfloat16, 32> vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3;
                    if constexpr (batch_size < 4) {
                        vector<bfloat16, 32> Sbuff00, Sbuff01, Sbuff10, Sbuff11;

                        Sbuff00 = MY_LOAD_V<32>(p_in);
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);

                        Sbuff01 = MY_LOAD_V<32>(p_in);
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);

                        Sbuff10 = MY_LOAD_V<32>(p_in);
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                        Sbuff11 = MY_LOAD_V<32>(p_in);

                        Xbuff_r00 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r00, Sbuff00, bf16_step_align, frac));
                        Xbuff_r01 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r01, Sbuff01, bf16_step_align, frac));

                        Xbuff_r20 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r20, Sbuff10, bf16_step_align, frac));
                        Xbuff_r21 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r21, Sbuff11, bf16_step_align, frac));

                        p_in = INCR_ITR3D_PARAM_BF16(p_in, iterator_inner, conv2d_params);

                        vector<uint8_t, 64> u00, u01, u20, u21;
                        u00 = Xbuff_r00.template cast_to<uint8_t>();
                        u01 = Xbuff_r01.template cast_to<uint8_t>();
                        u20 = Xbuff_r20.template cast_to<uint8_t>();
                        u21 = Xbuff_r21.template cast_to<uint8_t>();

                        vector<uint8_t, 64> r0, r1, r2, r3, r1_, r2_;
                        r0 = shuffle(u00, u01, s1);
                        r1_ = shuffle(u20, u21, s1);
                        r2_ = shuffle(u00, u01, s2);
                        r3 = shuffle(u20, u21, s2);

                        r1 = shuffle(r1_, r2_, s3);
                        r2 = shuffle(r1_, r2_, s4);
                        vec_Xbuff0.insert(0, r0.cast_to<bfloat16>());
                        vec_Xbuff0.insert(1, r1.cast_to<bfloat16>());
                        vec_Xbuff1.insert(0, r2.cast_to<bfloat16>());
                        vec_Xbuff1.insert(1, r3.cast_to<bfloat16>());
                    } else {
                        vec_Xbuff0.insert(0, load_v<32>(p_in));
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                        vec_Xbuff0.insert(1, load_v<32>(p_in));
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                        vec_Xbuff1.insert(0, load_v<32>(p_in));
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                        vec_Xbuff1.insert(1, load_v<32>(p_in));

                        p_in = INCR_ITR3D_PARAM_BF16(p_in, iterator_inner, conv2d_params);
                    }

                    vector<bfloat16, 32> Ybuff00, Ybuff01, Ybuff10, Ybuff11;
                    Ybuff00 = load_v<32>(p_w);
                    p_w = byte_incr(p_w, 32 * 2);
                    Ybuff01 = load_v<32>(p_w);
                    p_w = byte_incr(p_w, 32 * 2);
                    Ybuff10 = load_v<32>(p_w);
                    p_w = byte_incr(p_w, 32 * 2);
                    Ybuff11 = load_v<32>(p_w);
                    p_w = byte_incr(p_w, 32 * 2);

                    vector<bfloat16, 64> Y0, Y1;

                    Y0.insert(0, Ybuff00);
                    Y0.insert(1, Ybuff01);
                    Y1.insert(0, Ybuff10);
                    Y1.insert(1, Ybuff11);

#ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
                    set_rnd(rnd_sym_inf);
#endif
                    Cbuff0.mac(Y0, vec_Xbuff0); // row 0-7,   OC0 - OC7
                    Cbuff1.mac(Y0, vec_Xbuff1); // row 8-15,  OC0 - OC7
                    Cbuff2.mac(Y1, vec_Xbuff0); // row 0-7,   OC8 - OC15
                    Cbuff3.mac(Y1, vec_Xbuff1); // row 8-15,  OC8 - OC15
#ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
                    set_rnd(rnd_conv_even);
#endif

                    if constexpr (batch_size < 4) {
                        int pdel = ((long)(p_in));
                        frac = (pdel & 63) + 2;
                    }
                }

            p_in = INCR_ITR3D_PARAM_BF16(p_in, iterator_outer, conv2d_params);
            p_w = INCR_ITR3D_PARAM_BF16(p_w, iterator_weights, conv2d_params);

            if (out_mode == FULL) {
                if constexpr (act_type == LINEAR || act_type == RELU) {
#ifdef __X86SIM__
                    aie::store_unaligned_v(
                        byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0),
                        aie::max(mt_8x8(Cbuff1.to_accum().to_vector<bfloat16>()), bar)); // row8 - row15, OC0 - OC7
                    aie::store_unaligned_v(p_out, aie::max(mt_8x8(Cbuff0.to_accum().to_vector<bfloat16>()),
                                                 bar)); // row0 - row7, OC0 - OC7
                    p_out = byte_incr(p_out, conv2d_params.inc_ST_0);
                    p_out = byte_incr(p_out, conv2d_params.inc_ST_1);

                    aie::store_unaligned_v(
                        byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0),
                        aie::max(mt_8x8(Cbuff3.to_accum().to_vector<bfloat16>()), bar)); // row8 - row15, OC8 - OC15
                    aie::store_unaligned_v(p_out, aie::max(mt_8x8(Cbuff2.to_accum().to_vector<bfloat16>()),
                                                 bar)); // row0 - row7, OC8 - OC15
                    p_out = byte_incr(p_out, conv2d_params.inc_ST_0 + 64);
#else
                    aie::store_v(
                        byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0),
                        aie::max(mt_8x8(Cbuff1.to_accum().to_vector<bfloat16>()), bar)); // row8 - row15, OC0 - OC7
                    aie::store_v(p_out, aie::max(mt_8x8(Cbuff0.to_accum().to_vector<bfloat16>()),
                                                 bar)); // row0 - row7, OC0 - OC7
                    p_out = byte_incr(p_out, conv2d_params.inc_ST_0);
                    p_out = byte_incr(p_out, conv2d_params.inc_ST_1);

                    aie::store_v(
                        byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0),
                        aie::max(mt_8x8(Cbuff3.to_accum().to_vector<bfloat16>()), bar)); // row8 - row15, OC8 - OC15
                    aie::store_v(p_out, aie::max(mt_8x8(Cbuff2.to_accum().to_vector<bfloat16>()),
                                                 bar)); // row0 - row7, OC8 - OC15
                    p_out = byte_incr(p_out, conv2d_params.inc_ST_0 + 64);
#endif
                } else {
                    bfloat16 alpha = *reinterpret_cast<bfloat16*>(&conv2d_params.lrelu_alpha);

                    vector<bfloat16, 64> vec_Cbuff0 = mt_8x8(Cbuff0.to_accum().to_vector<bfloat16>());
                    vector<bfloat16, 64> vec_Cbuff1 = mt_8x8(Cbuff1.to_accum().to_vector<bfloat16>());
                    vector<bfloat16, 64> vec_Cbuff2 = mt_8x8(Cbuff2.to_accum().to_vector<bfloat16>());
                    vector<bfloat16, 64> vec_Cbuff3 = mt_8x8(Cbuff3.to_accum().to_vector<bfloat16>());

                    leaky_relu_compute<64>(
                        vec_Cbuff1, alpha,
                        byte_incr((bfloat16*)p_out,
                                  conv12_psum_store_offset + conv2d_params.inc_ST_0)); // row8 - row15, OC0 - OC7
                    leaky_relu_compute<64>(vec_Cbuff0, alpha, (bfloat16*)p_out);       // row0 - row7, OC0 - OC7
                    p_out = byte_incr(p_out, conv2d_params.inc_ST_0);
                    p_out = byte_incr(p_out, conv2d_params.inc_ST_1);

                    leaky_relu_compute<64>(
                        vec_Cbuff3, alpha,
                        byte_incr((bfloat16*)p_out,
                                  conv12_psum_store_offset + conv2d_params.inc_ST_0)); // row8 - row15, OC8 - OC15
                    leaky_relu_compute<64>(vec_Cbuff2, alpha, (bfloat16*)p_out);       // row0 - row7, OC8 - OC15
                    p_out = byte_incr(p_out, conv2d_params.inc_ST_0 + 64);
                }
            } else {
#ifdef __X86SIM__
                aie::store_unaligned_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0),
                             Cbuff1.to_accum().to_vector<bfloat16>());        // row8 - row15, OC0 - OC7
                aie::store_unaligned_v(p_out, Cbuff0.to_accum().to_vector<bfloat16>()); // row0 - row7, OC0 - OC7
                p_out = byte_incr(p_out, conv2d_params.inc_ST_0);
                p_out = byte_incr(p_out, conv2d_params.inc_ST_1);

                aie::store_unaligned_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0),
                             Cbuff3.to_accum().to_vector<bfloat16>());        // row8 - row15, OC8 - OC15
                aie::store_unaligned_v(p_out, Cbuff2.to_accum().to_vector<bfloat16>()); // row0 - row7, OC8 - OC15
                p_out = byte_incr(p_out, conv2d_params.inc_ST_0 + 64);        // row8 - row11, OC8 - OC15
#else
                aie::store_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0),
                             Cbuff1.to_accum().to_vector<bfloat16>());        // row8 - row15, OC0 - OC7
                aie::store_v(p_out, Cbuff0.to_accum().to_vector<bfloat16>()); // row0 - row7, OC0 - OC7
                p_out = byte_incr(p_out, conv2d_params.inc_ST_0);
                p_out = byte_incr(p_out, conv2d_params.inc_ST_1);

                aie::store_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0),
                             Cbuff3.to_accum().to_vector<bfloat16>());        // row8 - row15, OC8 - OC15
                aie::store_v(p_out, Cbuff2.to_accum().to_vector<bfloat16>()); // row0 - row7, OC8 - OC15
                p_out = byte_incr(p_out, conv2d_params.inc_ST_0 + 64);        // row8 - row11, OC8 - OC15
#endif
            }
            p_out = INCR_ITR3D(p_out, iterator_pout);
        }
}

#else
// TODO:(tianmin) bfloat16 kernel on aie2p/aie2ps
// TODO:(tianmin) support dtype_partial_sum in case we need fp32 partial sum
// TODO: INNER-MAC4
template <uint8_t batch_size, act_t act_type>
INLINE void conv2d_generic(bfloat16* __restrict input,
                           bfloat16* __restrict weights,
                           bfloat16* __restrict acc_in,
                           bfloat16* __restrict output,
                           conv2d_bf16_params_t& conv2d_params,
                           out_mode_t out_mode) {
    bfloat16 bar = 0;
    {
#ifdef USE_RTP_ACT
        int out_sign = conv2d_params.out_sign;
#else
        int out_sign = 1 - act_type; // Note enum for act types are: 0,1,2 -> linear, relu, lrelu
#endif
        if (out_sign == 1) {
            bar = -3.38953139e38;
        }
    }

    bfloat16 DM_BANK(b)* p_init16 = (bfloat16 DM_BANK(b)*)acc_in;
    bfloat16 DM_BANK(b) * p_out;
    bfloat16 DM_BANK(b) * p_out2;
    p_out = (conv2d_params.psum_out == 0)
                ? (bfloat16 DM_BANK(b)*)output
                : (bfloat16 DM_BANK(b)*)((int*)output + (conv2d_params.psum_buff_offset >> 1));
    p_out2 = p_out;

    bfloat16 IFM_DM_BANK* p_in = (bfloat16 IFM_DM_BANK*)input;
    bfloat16 IFM_DM_BANK* p_in_start = p_in;
    bfloat16 WT_DM_BANK* p_w = (bfloat16 WT_DM_BANK*)weights;

    bfloat16 WT_DM_BANK* p_bias =
        (bfloat16 WT_DM_BANK*)((int*)p_w - conv2d_params.hdr_len + LAYER_PARAM_WORDS); // Pointer to load bias.
    int psum_in_shift = 0;
    // int psum_in_shift = (1 - conv2d_params.psum_in) * conv2d_params.shift_bias_init;
    int s1, s2, s3, s4; // TODO: better move {s1, s2, s3, s4, bf16_step_align} to parameter setup
    if (conv2d_params.shfl_0 == T512_1x2_lo) {
        s1 = T512_1x2_lo;
        s2 = T512_1x2_hi;
        s3 = T512_1x2_hi;
        s4 = T512_1x2_lo;
    }
    if (conv2d_params.shfl_0 == T64_8x2_lo) {
        s1 = T128_4x2_lo;
        s2 = T128_4x2_lo;
        s3 = T512_1x2_lo;
        s4 = T512_1x2_hi;
    }
    if (conv2d_params.shfl_0 == T128_4x2_lo) {
        s1 = T256_2x2_lo;
        s2 = T256_2x2_lo;
        s3 = T512_1x2_lo;
        s4 = T512_1x2_hi;
    }
    int bf16_step_align = 0;
    if (conv2d_params.step_align == 0) {
        bf16_step_align = 0;
    }
    if (conv2d_params.step_align == 2) {
        bf16_step_align = 3;
    }
    if (conv2d_params.step_align == 3) {
        bf16_step_align = 4;
    }

    int zero_psum = 1 - conv2d_params.psum_in;
    int zero_casc = 1;
    if (out_mode == FULL) {
        zero_casc = 0;
    }

    using mmul_488 = mmul<4, 8, 8, bfloat16, bfloat16>;

    vector<bfloat16, 32> Xbuff_r00, Xbuff_r01, Xbuff_r20, Xbuff_r21;

    SETUP_ITR3D_COUNTERS(iterator_inner);
    SETUP_ITR3D_COUNTERS(iterator_outer);
    SETUP_ITR3D_COUNTERS(iterator_weights);
    SETUP_ITR3D_COUNTERS(iterator_psum);
    SETUP_ITR3D_COUNTERS(iterator_pout);

    int frac = 0;
    int chnd = 32;
    int chnd_T = 64;

    if constexpr (batch_size < 4) {
        p_in += 31; // 64 bytes aligned for aie2p
    }

    int32_t conv12_psum_store_offset = conv2d_params.conv_type == (uint8_t)12 ? -128 : 0;
    DECL_ITR3D(iterator_psum);
    SETUP_ITR3D(iterator_psum, 0, 0, conv2d_params.inc_Xo_T - chnd_T - 32 + conv12_psum_store_offset,
                conv2d_params.num_X, conv2d_params.inc_Co_T - chnd_T - 32 + conv12_psum_store_offset);

    DECL_ITR3D(iterator_pout);
    SETUP_ITR3D(iterator_pout, 0, 0, conv2d_params.inc_Xo_T - chnd_T + conv12_psum_store_offset, conv2d_params.num_X,
                conv2d_params.inc_Co_T - chnd_T + conv12_psum_store_offset);

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2, ) 
    for (unsigned j = 0; j < conv2d_params.outer_g; j++) {     
            if constexpr(batch_size < 4) {
                int pdel = ((long)(p_in));
                frac = (pdel & 63) + 2; // 64 bytes aligned for aie2p
            }

            vector<bfloat16, 32> vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3;
            if constexpr (batch_size < 4) {
                vector<bfloat16, 32> Sbuff00, Sbuff01, Sbuff10, Sbuff11;

                Sbuff00 = MY_LOAD_V<32>(p_in);
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);

                Sbuff01 = MY_LOAD_V<32>(p_in);
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);

                Sbuff10 = MY_LOAD_V<32>(p_in);
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                Sbuff11 = MY_LOAD_V<32>(p_in);

                Xbuff_r00 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r00, Sbuff00, bf16_step_align, frac));
                Xbuff_r01 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r01, Sbuff01, bf16_step_align, frac));

                Xbuff_r20 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r20, Sbuff10, bf16_step_align, frac));
                Xbuff_r21 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r21, Sbuff11, bf16_step_align, frac));

                p_in = INCR_ITR3D_PARAM_BF16(p_in, iterator_inner, conv2d_params);

                vector<uint8_t, 64> u00, u01, u20, u21;
                u00 = Xbuff_r00.template cast_to<uint8_t>();
                u01 = Xbuff_r01.template cast_to<uint8_t>();
                u20 = Xbuff_r20.template cast_to<uint8_t>();
                u21 = Xbuff_r21.template cast_to<uint8_t>();

                vector<uint8_t, 64> r0, r1, r2, r3, r1_, r2_;
                r0 = shuffle(u00, u01, s1);
                r1_ = shuffle(u20, u21, s1);
                r2_ = shuffle(u00, u01, s2);
                r3 = shuffle(u20, u21, s2);

                r1 = shuffle(r1_, r2_, s3);
                r2 = shuffle(r1_, r2_, s4);
                vec_Xbuff0 = r0.cast_to<bfloat16>();
                vec_Xbuff1 = r1.cast_to<bfloat16>();
                vec_Xbuff2 = r2.cast_to<bfloat16>();
                vec_Xbuff3 = r3.cast_to<bfloat16>();
            } else {
                vec_Xbuff0 = load_v<32>(p_in);
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                vec_Xbuff1 = load_v<32>(p_in);
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                vec_Xbuff2 = load_v<32>(p_in);
                p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                vec_Xbuff3 = load_v<32>(p_in);

                p_in = INCR_ITR3D_PARAM_BF16(p_in, iterator_inner, conv2d_params);
            }

            vector<bfloat16, 32> Ybuff00, Ybuff01, Ybuff10, Ybuff11;
            Ybuff00 = load_v<32>(p_w);
            p_w = byte_incr(p_w, 32 * 2);
            Ybuff01 = load_v<32>(p_w);
            p_w = byte_incr(p_w, 32 * 2);
            Ybuff10 = load_v<32>(p_w);
            p_w = byte_incr(p_w, 32 * 2);
            Ybuff11 = load_v<32>(p_w);
            p_w = byte_incr(p_w, 32 * 2);

            aie::accum<accfloat, 32> accum_0, accum_1, accum_2, accum_3, accum_4, accum_5, accum_6, accum_7;

            accum_0 = aie::from_vector<accfloat>(aie::load_v<32>(p_init16));
            p_init16 = byte_incr(p_init16, 64);

            accum_1 = aie::from_vector<accfloat>(aie::load_v<32>(p_init16));
            p_init16 = byte_incr(p_init16, conv2d_params.inc_ST_0 - chnd_T - 32 + 32);

            accum_2 = aie::from_vector<accfloat>(aie::load_v<32>(p_init16));
            p_init16 = byte_incr(p_init16, 64);

            accum_3 = aie::from_vector<accfloat>(aie::load_v<32>(p_init16));
            p_init16 = byte_incr(p_init16, conv2d_params.inc_ST_1 - chnd_T - 32 + 32);

            accum_4 = aie::from_vector<accfloat>(aie::load_v<32>(p_init16));
            p_init16 = byte_incr(p_init16, 64);

            accum_5 = aie::from_vector<accfloat>(aie::load_v<32>(p_init16));
            p_init16 = byte_incr(p_init16, conv2d_params.inc_ST_0 - chnd_T - 32 + 32);

            accum_6 = aie::from_vector<accfloat>(aie::load_v<32>(p_init16));
            p_init16 = byte_incr(p_init16, 64);

            accum_7 = aie::from_vector<accfloat>(aie::load_v<32>(p_init16));
            p_init16 = byte_incr(p_init16, 32);
            p_init16 = INCR_ITR3D(p_init16, iterator_psum); // not need BF16, because it was int16

            int32_t bias_incr = 128 * iterator_weights_cnt1;
            bfloat16 WT_DM_BANK* p_bias_tmp = byte_incr(p_bias, bias_incr);

            accum<accfloat, 32> bias_0, bias_1;
            bias_0 = aie::from_vector<accfloat>(aie::load_v<32>(p_bias_tmp));
            p_bias_tmp = byte_incr(p_bias_tmp, 64);
            bias_1 = aie::from_vector<accfloat>(aie::load_v<32>(p_bias_tmp));

            mmul_488 Cbuff0(aie::op_zero(accum_0, zero_psum));
            mmul_488 Cbuff1(aie::op_zero(accum_1, zero_psum));
            mmul_488 Cbuff2(aie::op_zero(accum_2, zero_psum));
            mmul_488 Cbuff3(aie::op_zero(accum_3, zero_psum));
            mmul_488 Cbuff4(aie::op_zero(accum_4, zero_psum));
            mmul_488 Cbuff5(aie::op_zero(accum_5, zero_psum));
            mmul_488 Cbuff6(aie::op_zero(accum_6, zero_psum));
            mmul_488 Cbuff7(aie::op_zero(accum_7, zero_psum));

            vector<bfloat16, 64> Y0, Y1;

            Y0.insert(0, Ybuff00);
            Y0.insert(1, Ybuff01);
            Y1.insert(0, Ybuff10);
            Y1.insert(1, Ybuff11);

#ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
            set_rnd(rnd_sym_inf);
#endif
            Cbuff0.mac(vec_Xbuff0, Y0); // row 0-3,   OC0 - OC7
            Cbuff1.mac(vec_Xbuff1, Y0); // row 4-7,   OC0 - OC7
            Cbuff2.mac(vec_Xbuff2, Y0); // row 8-11,  OC0 - OC7
            Cbuff3.mac(vec_Xbuff3, Y0); // row 12-15, OC0 - OC7

            Cbuff4.mac(vec_Xbuff0, Y1); // row 0-3,   OC8 - OC15
            Cbuff5.mac(vec_Xbuff1, Y1); // row 4-7,   OC8 - OC15
            Cbuff6.mac(vec_Xbuff2, Y1); // row 8-11,  OC8 - OC15
            Cbuff7.mac(vec_Xbuff3, Y1); // row 12-15, OC8 - OC15
#ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
            set_rnd(rnd_conv_even);
#endif

            Cbuff0 = aie::add(bias_0, Cbuff0.to_accum()); // OC0 - OC7
            Cbuff1 = aie::add(bias_0, Cbuff1.to_accum()); // OC0 - OC7
            Cbuff2 = aie::add(bias_0, Cbuff2.to_accum()); // OC0 - OC7
            Cbuff3 = aie::add(bias_0, Cbuff3.to_accum()); // OC0 - OC7
            Cbuff4 = aie::add(bias_1, Cbuff4.to_accum()); // OC8 - OC15
            Cbuff5 = aie::add(bias_1, Cbuff5.to_accum()); // OC8 - OC15
            Cbuff6 = aie::add(bias_1, Cbuff6.to_accum()); // OC8 - OC15
            Cbuff7 = aie::add(bias_1, Cbuff7.to_accum()); // OC8 - OC15

            if constexpr (batch_size < 4) {
                int pdel = ((long)(p_in));
                frac = (pdel & 63) + 2;
            }
            
            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(4, ) 
            for (int i = 0; i < conv2d_params.inner_g - 1; i++) {
                    vector<bfloat16, 32> vec_Xbuff0, vec_Xbuff1, vec_Xbuff2, vec_Xbuff3;
                    if constexpr (batch_size < 4) {
                        vector<bfloat16, 32> Sbuff00, Sbuff01, Sbuff10, Sbuff11;

                        Sbuff00 = MY_LOAD_V<32>(p_in);
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);

                        Sbuff01 = MY_LOAD_V<32>(p_in);
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);

                        Sbuff10 = MY_LOAD_V<32>(p_in);
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                        Sbuff11 = MY_LOAD_V<32>(p_in);

                        Xbuff_r00 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r00, Sbuff00, bf16_step_align, frac));
                        Xbuff_r01 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r01, Sbuff01, bf16_step_align, frac));

                        Xbuff_r20 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r20, Sbuff10, bf16_step_align, frac));
                        Xbuff_r21 = aie::vector<bfloat16, 32>(shiftx(Xbuff_r21, Sbuff11, bf16_step_align, frac));

                        p_in = INCR_ITR3D_PARAM_BF16(p_in, iterator_inner, conv2d_params);

                        vector<uint8_t, 64> u00, u01, u20, u21;
                        u00 = Xbuff_r00.template cast_to<uint8_t>();
                        u01 = Xbuff_r01.template cast_to<uint8_t>();
                        u20 = Xbuff_r20.template cast_to<uint8_t>();
                        u21 = Xbuff_r21.template cast_to<uint8_t>();

                        vector<uint8_t, 64> r0, r1, r2, r3, r1_, r2_;
                        r0 = shuffle(u00, u01, s1);
                        r1_ = shuffle(u20, u21, s1);
                        r2_ = shuffle(u00, u01, s2);
                        r3 = shuffle(u20, u21, s2);

                        r1 = shuffle(r1_, r2_, s3);
                        r2 = shuffle(r1_, r2_, s4);
                        vec_Xbuff0 = r0.cast_to<bfloat16>();
                        vec_Xbuff1 = r1.cast_to<bfloat16>();
                        vec_Xbuff2 = r2.cast_to<bfloat16>();
                        vec_Xbuff3 = r3.cast_to<bfloat16>();
                    } else {
                        vec_Xbuff0 = load_v<32>(p_in);
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                        vec_Xbuff1 = load_v<32>(p_in);
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                        vec_Xbuff2 = load_v<32>(p_in);
                        p_in = byte_incr(p_in, conv2d_params.inc_A * 2);
                        vec_Xbuff3 = load_v<32>(p_in);

                        p_in = INCR_ITR3D_PARAM_BF16(p_in, iterator_inner, conv2d_params);
                    }

                    vector<bfloat16, 32> Ybuff00, Ybuff01, Ybuff10, Ybuff11;
                    Ybuff00 = load_v<32>(p_w);
                    p_w = byte_incr(p_w, 32 * 2);
                    Ybuff01 = load_v<32>(p_w);
                    p_w = byte_incr(p_w, 32 * 2);
                    Ybuff10 = load_v<32>(p_w);
                    p_w = byte_incr(p_w, 32 * 2);
                    Ybuff11 = load_v<32>(p_w);
                    p_w = byte_incr(p_w, 32 * 2);

                    vector<bfloat16, 64> Y0, Y1;

                    Y0.insert(0, Ybuff00);
                    Y0.insert(1, Ybuff01);
                    Y1.insert(0, Ybuff10);
                    Y1.insert(1, Ybuff11);

#ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
                    set_rnd(rnd_sym_inf);
#endif
                    Cbuff0.mac(vec_Xbuff0, Y0); // row 0-3,   OC0 - OC7
                    Cbuff1.mac(vec_Xbuff1, Y0); // row 4-7,   OC0 - OC7
                    Cbuff2.mac(vec_Xbuff2, Y0); // row 8-11,  OC0 - OC7
                    Cbuff3.mac(vec_Xbuff3, Y0); // row 12-15, OC0 - OC7

                    Cbuff4.mac(vec_Xbuff0, Y1); // row 0-3,   OC8 - OC11
                    Cbuff5.mac(vec_Xbuff1, Y1); // row 4-7,   OC8 - OC11
                    Cbuff6.mac(vec_Xbuff2, Y1); // row 8-11,  OC8 - OC11
                    Cbuff7.mac(vec_Xbuff3, Y1); // row 12-15, OC8 - OC11
#ifdef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
                    set_rnd(rnd_conv_even);
#endif

                    if constexpr (batch_size < 4) {
                        int pdel = ((long)(p_in));
                        frac = (pdel & 63) + 2;
                    }
                }

            p_in = INCR_ITR3D_PARAM_BF16(p_in, iterator_outer, conv2d_params);
            p_w = INCR_ITR3D_PARAM_BF16(p_w, iterator_weights, conv2d_params);

            if (out_mode == FULL) {
                if constexpr (act_type == LINEAR || act_type == RELU) {
                    aie::store_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0),
                                 aie::max(Cbuff2.to_accum().to_vector<bfloat16>(), bar)); // row8 - row11, OC0 - OC7
                    aie::store_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0 + 64),
                                 aie::max(Cbuff3.to_accum().to_vector<bfloat16>(), bar)); // row12 - row15, OC0 - OC7
                    aie::store_v(p_out,
                                 aie::max(Cbuff0.to_accum().to_vector<bfloat16>(), bar)); // row0 - row3, OC0 - OC7
                    aie::store_v(byte_incr(p_out, 64),
                                 aie::max(Cbuff1.to_accum().to_vector<bfloat16>(), bar)); // row4 - row7, OC0 - OC7
                    p_out = byte_incr(p_out, conv2d_params.inc_ST_0);
                    p_out = byte_incr(p_out, conv2d_params.inc_ST_1);

                    aie::store_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0),
                                 aie::max(Cbuff6.to_accum().to_vector<bfloat16>(), bar)); // row8 - row11, OC8 - OC15
                    aie::store_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0 + 64),
                                 aie::max(Cbuff7.to_accum().to_vector<bfloat16>(), bar)); // row12 - row15, OC8 - OC15
                    aie::store_v(p_out,
                                 aie::max(Cbuff4.to_accum().to_vector<bfloat16>(), bar)); // row0 - row3, OC8 - OC15
                    p_out = byte_incr(p_out, 64);
                    aie::store_v(p_out,
                                 aie::max(Cbuff5.to_accum().to_vector<bfloat16>(), bar)); // row4 - row7, OC8 - OC15
                    p_out = byte_incr(p_out, conv2d_params.inc_ST_0);
                } else {
                    bfloat16 alpha = *reinterpret_cast<bfloat16*>(&conv2d_params.lrelu_alpha);

                    vector<bfloat16, 32> vec_Cbuff0 = Cbuff0.to_accum().to_vector<bfloat16>();
                    vector<bfloat16, 32> vec_Cbuff1 = Cbuff1.to_accum().to_vector<bfloat16>();
                    vector<bfloat16, 32> vec_Cbuff2 = Cbuff2.to_accum().to_vector<bfloat16>();
                    vector<bfloat16, 32> vec_Cbuff3 = Cbuff3.to_accum().to_vector<bfloat16>();

                    vector<bfloat16, 32> vec_Cbuff4 = Cbuff4.to_accum().to_vector<bfloat16>();
                    vector<bfloat16, 32> vec_Cbuff5 = Cbuff5.to_accum().to_vector<bfloat16>();
                    vector<bfloat16, 32> vec_Cbuff6 = Cbuff6.to_accum().to_vector<bfloat16>();
                    vector<bfloat16, 32> vec_Cbuff7 = Cbuff7.to_accum().to_vector<bfloat16>();

                    leaky_relu_compute<32>(
                        vec_Cbuff2, alpha,
                        byte_incr((bfloat16*)p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0));
                    leaky_relu_compute<32>(
                        vec_Cbuff3, alpha,
                        byte_incr((bfloat16*)p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0 + 64));
                    leaky_relu_compute<32>(vec_Cbuff0, alpha, (bfloat16*)p_out);
                    leaky_relu_compute<32>(vec_Cbuff1, alpha, byte_incr((bfloat16*)p_out, 64));
                    p_out = byte_incr(p_out, conv2d_params.inc_ST_0);
                    p_out = byte_incr(p_out, conv2d_params.inc_ST_1);

                    leaky_relu_compute<32>(
                        vec_Cbuff6, alpha,
                        byte_incr((bfloat16*)p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0));
                    leaky_relu_compute<32>(
                        vec_Cbuff7, alpha,
                        byte_incr((bfloat16*)p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0 + 64));
                    leaky_relu_compute<32>(vec_Cbuff4, alpha, (bfloat16*)p_out);
                    p_out = byte_incr(p_out, 64);
                    leaky_relu_compute<32>(vec_Cbuff5, alpha, (bfloat16*)p_out);
                    p_out = byte_incr(p_out, conv2d_params.inc_ST_0);
                }
            } else {
                aie::store_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0),
                             Cbuff2.to_accum().to_vector<bfloat16>()); // row8 - row11, OC0 - OC7
                aie::store_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0 + 64),
                             Cbuff3.to_accum().to_vector<bfloat16>());                       // row12 - row15, OC0 - OC7
                aie::store_v(p_out, Cbuff0.to_accum().to_vector<bfloat16>());                // row0 - row3, OC0 - OC7
                aie::store_v(byte_incr(p_out, 64), Cbuff1.to_accum().to_vector<bfloat16>()); // row4 - row7, OC0 - OC7
                p_out = byte_incr(p_out, conv2d_params.inc_ST_0);
                p_out = byte_incr(p_out, conv2d_params.inc_ST_1);

                aie::store_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0),
                             Cbuff6.to_accum().to_vector<bfloat16>());
                aie::store_v(byte_incr(p_out, conv12_psum_store_offset + conv2d_params.inc_ST_0 + 64),
                             Cbuff7.to_accum().to_vector<bfloat16>());        // row12 - row15, OC8 - OC15
                aie::store_v(p_out, Cbuff4.to_accum().to_vector<bfloat16>()); // row0 - row3, OC8 - OC15
                p_out = byte_incr(p_out, 64);
                aie::store_v(p_out, Cbuff5.to_accum().to_vector<bfloat16>()); // row4 - row7, OC8 - OC15
                p_out = byte_incr(p_out, conv2d_params.inc_ST_0);             // row8 - row11, OC8 - OC15
            }
            p_out = INCR_ITR3D(p_out, iterator_pout);
        }
}
#endif // end of AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
#endif // end of AIE2P/AIE2PS

template <uint8_t batch_size,
          act_t act_type,
          typename dtype_ifm,
          typename dtype_wts,
          typename dtype_ofm,
          typename config_ifm,
          typename config_wts,
          typename config_ofm,
          bool enable_FC = false>
void conv2d_bf16(adf::input_buffer_conf<dtype_ifm, config_ifm>& __restrict ifm,
                 adf::input_buffer_conf<dtype_wts, config_wts>& __restrict wts,
                 adf::output_buffer_conf<dtype_ofm, config_ofm>& ofm,
                 conv2d_bf16_params_t& conv2d_params) {
    // Setup iteration level parameters like IO mode etc
    setup_conv2d_bf16_iter_params(conv2d_params);

    // Extract low level pointers used by the actual kernel code
    int* input = (int*)ifm.data() + conv2d_params.ifm_offset;
    int* weights = (int*)wts.data() + conv2d_params.hdr_len + conv2d_params.wts_offset;
    int* output = (int*)ofm.data();
    int* acc_in;
    acc_in = ((int*)ofm.data() + (conv2d_params.psum_buff_offset >> 1));
    // Call the kernel with appropriate mode int8 (final store) or int16 (partial store)
    // printf("\n exec_type = %d", conv2d_params.exec_type);

    // Output modes
    out_mode_t out_mode;
    switch (conv2d_params.exec_type) {
        case 0:
        case 2:
            out_mode = FULL;
            break;
        case 1:
        case 3:
            out_mode = PARTIAL;
            break;
    }
    conv2d_generic<batch_size, act_type>((bfloat16*)input, (bfloat16*)weights, (bfloat16*)acc_in, (bfloat16*)output,
                                         conv2d_params, out_mode);

    if constexpr(enable_FC == true) { // have FC in graph
        if (conv2d_params.conv_type == 2 && out_mode == FULL) { // FC
            fc_post_process<dtype_ofm, batch_size>((int *)ofm.data(), conv2d_params);
        }
    }
}

template <typename dtype_ofm, uint8_t batch_size>
NOINLINE void fc_post_process
(
    int* restrict output,
    conv2d_bf16_params_t& conv2d_params
) {
    dtype_ofm* p_ofm = (dtype_ofm DM_BANK(a)*) output;
    dtype_ofm* p_final = (dtype_ofm DM_BANK(a)*) output;
    vector<dtype_ofm, 32> Obuff0, Obuff1, Xbuff0, Xbuff1;
    for (unsigned i = 0; i < conv2d_params.outer_g; i++)
        chess_prepare_for_pipelining
        chess_keep_sw_loop
        chess_loop_range(1, )
    {
        Obuff0 = load_v<32>((dtype_ofm*)p_ofm);
        p_ofm = byte_incr(p_ofm, 64);
        Obuff1 = load_v<32>((dtype_ofm*)p_ofm);
        p_ofm = byte_incr(p_ofm, 256);
        if constexpr (batch_size == 4) {
            store_v(p_final, Obuff0); p_final = byte_incr(p_final, 64);
            store_v(p_final, Obuff1); p_final = byte_incr(p_final, 64);
        }
        else {
            std::tie(Xbuff0, Xbuff1) = interleave_zip(Obuff0, Obuff1, 8 * batch_size);
            if constexpr (batch_size == 1) {
                vector<dtype_ofm, 32>Obuff2, Obuff3, Xbuff2, Xbuff3, Xbuff4, Xbuff5;
                Obuff2 = load_v<32>((dtype_ofm*)p_ofm);
                p_ofm = byte_incr(p_ofm, 64);
                Obuff3 = load_v<32>((dtype_ofm*)p_ofm);
                p_ofm = byte_incr(p_ofm, 256);
                std::tie(Xbuff2, Xbuff3) = interleave_zip(Obuff2, Obuff3, 8);
                std::tie(Xbuff4, Xbuff5) = interleave_zip(Xbuff0, Xbuff2, 16);
                Xbuff0 = Xbuff4;
            }
            store_v(p_final, Xbuff0); p_final = byte_incr(p_final, 64);
        }
    }
}

#endif //__CONV2D_BF16_H__
