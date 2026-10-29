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
 * 1D Depthwise Convolution Kernel for AI Engine
 *
 * Performs 1D convolution on data present in "ifms" using weights from "wts"
 * Result is stored in "ofms".
 *
 */

#ifndef __CONV1D_DW_H__
#define __CONV1D_DW_H__

#include <adf.h>
#include "aie.hpp"

#include "utils.h"              // Contains some utility functions and definitions related to intrinsics usage
#include "mllib_const.h"        // Constants shared across graphs like activation type
#include "conv1d_dw_params.h" // Parameters like increments and wraps needed by kernel
#include "aie_api/utils.hpp"

using namespace aie;

// Removed reference to windows from this lower level code
template <uint8_t batch_size, typename dtype, typename config_ifm, typename config_wts, typename config_ofm> 
NOINLINE void conv1d_dw
(
    adf::input_buffer_conf<dtype, config_ifm>  & __restrict input,    // Input-0 this should be from IFM
    adf::input_buffer_conf<dtype, config_wts>  & __restrict weights, // Input-1 this should be from WTS
    adf::output_buffer_conf<dtype, config_ofm> & __restrict output,  // Output-1 this should be OFM of conv1d output
    conv1d_dw_params_t &params
) {

    dtype IFM_DM_BANK  * p_in   = (dtype  IFM_DM_BANK *) input.data(); 
    dtype OFM_DM_BANK  * p_out  = (dtype  OFM_DM_BANK *) output.data();
    dtype WTS_DM_BANK  * p_w    = (dtype  WTS_DM_BANK *) weights.data() + (LAYER_PARAM_BYTES + 2 * params.gran_in_width * params.ifm_depth);
    int16 WTS_DM_BANK  * p_b    = (int16  WTS_DM_BANK *) weights.data() + (LAYER_PARAM_BYTES/2);

     //Loading 64-byte values      
#if __AIE_ARCH__ == 20
    #define LOAD_64(ifm,ibuff) \
    {\
        ibuff.insert(0, load_v<32>(ifm));ifm+=32;\
        ibuff.insert(1, load_v<32>(ifm));ifm+=32;\
    }

    // <Output samples, Channel, element of each lane>
    using mac_conv_884 = aie::sliding_mul_ch_x_ops<8, 8, 4, 1, 1, int8, int8, acc32>; //HL-API for depthwise convolution

#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22
    #define LOAD_64(ifm,ibuff) \
    {\
        ibuff.insert(0, load_v<64>(ifm));ifm+=64;\
    }

    // <Output samples, Channel, element of each lane>
    using mac_conv_888 = aie::sliding_mul_ch_x_ops<8, 8, 8, 1, 1, int8, int8, acc32>; //HL-API for depthwise convolution

#endif

    // declare 2D/3D addressing
    DECL_ITR2D(iterator_inner)
    DECL_ITR2D(iterator_outer)
    DECL_ITR2D(iterator_bias)
    DECL_ITR2D(iterator_wts)
    DECL_ITR2D(iterator_result)
    // setup 2D/3D counters
    SETUP_ITR2D_COUNTERS(iterator_inner)
    SETUP_ITR2D_COUNTERS(iterator_outer)
    SETUP_ITR2D_COUNTERS(iterator_bias)
    SETUP_ITR2D_COUNTERS(iterator_wts)
    SETUP_ITR2D_COUNTERS(iterator_result)
    // setup 2D/3D iterator
    SETUP_ITR2D(iterator_inner, params.incInnL1, params.numInnL1, params.incInnL2);
    SETUP_ITR2D(iterator_outer, params.incOutL1, params.numOutL1, params.incOutL2);
    SETUP_ITR2D(iterator_bias, params.incB1, params.numB1, params.incB2);
    SETUP_ITR2D(iterator_wts, params.incK1, params.numK1, params.incK2);
    SETUP_ITR2D(iterator_result, params.incResO1, params.numResO1, params.incResO2);

    vector<dtype, 64> Xbuff0, Xbuff1, Xbuff2, Xbuff3;
    vector<dtype, 64> Xbuff4;
    vector<dtype, 64> Ybuff0 = zeros<dtype, 64>();

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (unsigned j = 0; j < params.outer_g; j++){
        //Load bias values       
#if __AIE_ARCH__ == 20
        aie::accum<acc32,32> tacc0;
        tacc0.from_vector(load_v<32>(p_b), params.shift_bias);
        aie::accum<acc32, 64> acc0 = aie::concat(tacc0, tacc0);
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22
        aie::accum<acc32,64> tacc0;
        tacc0.from_vector(load_v<64>(p_b), params.shift_bias);
        aie::accum<acc32, 64> acc0 = tacc0;
#endif
        aie::accum<acc32, 64> acc1 = acc0;
        aie::accum<acc32, 64> acc2 = acc0;
        aie::accum<acc32, 64> acc3 = acc0;
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(2,) 
        for (unsigned i = 0; i < params.inner_g; i++){
            // row 1
            LOAD_64(p_in, Xbuff0);
            LOAD_64(p_in, Xbuff1);
            LOAD_64(p_in, Xbuff2);
            LOAD_64(p_in, Xbuff3);
#if __AIE_ARCH__ == 20
            Xbuff4.insert(0, load_v<32>(p_in));
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22
            Xbuff4 = load_v<64>(p_in);
#endif
            p_in = INCR_ITR2D(p_in, iterator_inner);

            // load wts
#if __AIE_ARCH__ == 20
            Ybuff0.insert(0, load_v<32>(p_w));
            p_w = byte_incr(p_w, 32);

            acc0 = mac_conv_884::mac(acc0, Ybuff0.template extract<32>(0), 0, aie::concat(Xbuff0, Xbuff1), 0);
            acc1 = mac_conv_884::mac(acc1, Ybuff0.template extract<32>(0), 0, aie::concat(Xbuff1, Xbuff2), 0);
            acc2 = mac_conv_884::mac(acc2, Ybuff0.template extract<32>(0), 0, aie::concat(Xbuff2, Xbuff3), 0);
            acc3 = mac_conv_884::mac(acc3, Ybuff0.template extract<32>(0), 0, aie::concat(Xbuff3, Xbuff4), 0);
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22
            Ybuff0 = load_v<64>(p_w);
            p_w = byte_incr(p_w, 64);

            acc0 = mac_conv_888::mac(acc0, Ybuff0, 0, aie::concat(Xbuff0, Xbuff1), 0);
            acc1 = mac_conv_888::mac(acc1, Ybuff0, 0, aie::concat(Xbuff1, Xbuff2), 0);
            acc2 = mac_conv_888::mac(acc2, Ybuff0, 0, aie::concat(Xbuff2, Xbuff3), 0);
            acc3 = mac_conv_888::mac(acc3, Ybuff0, 0, aie::concat(Xbuff3, Xbuff4), 0);
#endif

        }

        p_b = INCR_ITR2D(p_b, iterator_bias);
        p_w = INCR_ITR2D(p_w, iterator_wts);
        p_in = INCR_ITR2D(p_in, iterator_outer);

        vector<dtype,64> Sbuff0 = acc0.template to_vector<dtype>(params.shift_ofm);
        vector<dtype,64> Sbuff1 = acc1.template to_vector<dtype>(params.shift_ofm);

#if __AIE_ARCH__ == 20
        store_v(p_out, Sbuff0.template extract<32>(0));
        p_out = byte_incr(p_out, params.stepS);
        Sbuff0 = shuffle(Sbuff0, Sbuff1, params.shfMode);
        store_v(p_out, Sbuff0.template extract<32>(0));
        p_out = byte_incr(p_out, 32);
        store_v(p_out, Sbuff1.template extract<32>(0));
        p_out = byte_incr(p_out, params.stepS);
        store_v(p_out, Sbuff0.template extract<32>(1));
        p_out = byte_incr(p_out, 32);

        Sbuff0 = acc2.template to_vector<dtype>(params.shift_ofm);
        Sbuff1 = acc3.template to_vector<dtype>(params.shift_ofm);

        store_v(p_out, Sbuff0.template extract<32>(0));
        p_out = byte_incr(p_out, params.stepS);
        Sbuff0 = shuffle(Sbuff0, Sbuff1, params.shfMode);
        store_v(p_out, Sbuff0.template extract<32>(0));
        p_out = byte_incr(p_out, 32);
        store_v(p_out, Sbuff1.template extract<32>(0));
        p_out = byte_incr(p_out, params.stepS);
        store_v(p_out, Sbuff0.template extract<32>(1));

#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22
        store_v(p_out, Sbuff0);
        p_out = byte_incr(p_out, params.stepS);
        Sbuff1 = shuffle(Sbuff0, Sbuff1, params.shfMode);
        store_v(p_out, Sbuff1);
        p_out = byte_incr(p_out, 64);

        Sbuff0 = acc2.template to_vector<dtype>(params.shift_ofm);
        Sbuff1 = acc3.template to_vector<dtype>(params.shift_ofm);

        store_v(p_out, Sbuff0);
        p_out = byte_incr(p_out, params.stepS);
        Sbuff1 = shuffle(Sbuff0, Sbuff1, params.shfMode);
        store_v(p_out, Sbuff1);
#endif
        p_out = INCR_ITR2D(p_out, iterator_result);
    }
}

template <uint8_t batch_size, typename config_ifm, typename config_wts, typename config_ofm> 
NOINLINE void conv1d_dw
(
    adf::input_buffer_conf<bfloat16, config_ifm>  & __restrict input,    // Input-0 this should be from IFM
    adf::input_buffer_conf<int8, config_wts>  & __restrict weights, // Input-1 this should be from WTS
    adf::output_buffer_conf<bfloat16, config_ofm> & __restrict output,  // Output-1 this should be OFM of conv1d output
    conv1d_dw_params_t &params
) {

    bfloat16 IFM_DM_BANK  * p_in   = (bfloat16  IFM_DM_BANK *) input.data(); 
    bfloat16 OFM_DM_BANK  * p_out  = (bfloat16  OFM_DM_BANK *) output.data();
    bfloat16 WTS_DM_BANK  * p_w    = (bfloat16  WTS_DM_BANK *) weights.data() + (LAYER_PARAM_BYTES/2 + params.gran_in_width * params.ifm_depth);
    bfloat16 WTS_DM_BANK  * p_b    = (bfloat16 WTS_DM_BANK *) weights.data() + (LAYER_PARAM_BYTES/2);

    // declare 2D/3D addressing
    DECL_ITR2D(iterator_inner)
    DECL_ITR2D(iterator_outer)
    DECL_ITR2D(iterator_bias)
    DECL_ITR2D(iterator_wts)
    DECL_ITR2D(iterator_result)
    // setup 2D/3D counters
    SETUP_ITR2D_COUNTERS(iterator_inner)
    SETUP_ITR2D_COUNTERS(iterator_outer)
    SETUP_ITR2D_COUNTERS(iterator_bias)
    SETUP_ITR2D_COUNTERS(iterator_wts)
    SETUP_ITR2D_COUNTERS(iterator_result)
    // setup 2D/3D iterator
    SETUP_ITR2D(iterator_inner, params.incInnL1, params.numInnL1, params.incInnL2);
    SETUP_ITR2D(iterator_outer, params.incOutL1, params.numOutL1, params.incOutL2);
    SETUP_ITR2D(iterator_bias, params.incB1, params.numB1, params.incB2);
    SETUP_ITR2D(iterator_wts, params.incK1, params.numK1, params.incK2);
    SETUP_ITR2D(iterator_result, params.incResO1, params.numResO1, params.incResO2);

     //Loading 64-byte values      
#if __AIE_ARCH__ == 20
    #define LOAD_32(ifm,ibuff) \
    {\
        ibuff.insert(0, load_v<16>(ifm));ifm+=16;\
        ibuff.insert(1, load_v<16>(ifm));ifm+=16;\
    }
    
    #define MMUL_1x2x1_2C8(ifm,wts,acc) \
    {\
        acc = mac_elem_16_2(ifm, wts, acc); \
    }

    vector<bfloat16, 32> Xbuff0, Xbuff1, Xbuff2, Xbuff3;
    vector<bfloat16, 32> Xbuff4, Xbuff5, Xbuff6, Xbuff7, Xbuff16;
    vector<bfloat16, 16> Ybuff0, Ybuff1, Ybuff2;

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (unsigned j = 0; j < params.outer_g; j++){
        aie::accum<accfloat, 16> acc0;
        acc0.from_vector(load_v<16>( p_b ));
        aie::accum<accfloat, 16> acc1 = acc0;
        aie::accum<accfloat, 16> acc2 = acc0;
        aie::accum<accfloat, 16> acc3 = acc0;
        aie::accum<accfloat, 16> acc4 = acc0;
        aie::accum<accfloat, 16> acc5 = acc0;
        aie::accum<accfloat, 16> acc6 = acc0;
        aie::accum<accfloat, 16> acc7 = acc0;
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(2,) 
        for (unsigned i = 0; i < params.inner_g; i++){
            LOAD_32(p_in, Xbuff0);
            LOAD_32(p_in, Xbuff1);
            LOAD_32(p_in, Xbuff2);
            LOAD_32(p_in, Xbuff3);
            Xbuff16.insert(0, load_v<16>(p_in));
            p_in = INCR_ITR2D(p_in, iterator_inner);

            Xbuff4 = shuffle_down_fill(Xbuff0, Xbuff1, 8);
            Xbuff5 = shuffle_down_fill(Xbuff1, Xbuff2, 8);
            Xbuff6 = shuffle_down_fill(Xbuff2, Xbuff3, 8);
            Xbuff7 = shuffle_down_fill(Xbuff3, Xbuff16, 8);

            std::tie(Xbuff0, Xbuff4) = interleave_zip(Xbuff0, Xbuff4, 8);
            std::tie(Xbuff1, Xbuff5) = interleave_zip(Xbuff1, Xbuff5, 8);
            std::tie(Xbuff2, Xbuff6) = interleave_zip(Xbuff2, Xbuff6, 8);
            std::tie(Xbuff3, Xbuff7) = interleave_zip(Xbuff3, Xbuff7, 8);

            Ybuff0 = load_v<16>(p_w);
            p_w = byte_incr(p_w, 32);
            std::tie(Ybuff1, Ybuff2) = interleave_zip(Ybuff0, Ybuff0, 8);

            MMUL_1x2x1_2C8(Xbuff0, concat(Ybuff1, Ybuff2), acc0);
            MMUL_1x2x1_2C8(Xbuff4, concat(Ybuff1, Ybuff2), acc1);
            MMUL_1x2x1_2C8(Xbuff1, concat(Ybuff1, Ybuff2), acc2);
            MMUL_1x2x1_2C8(Xbuff5, concat(Ybuff1, Ybuff2), acc3);
            MMUL_1x2x1_2C8(Xbuff2, concat(Ybuff1, Ybuff2), acc4);
            MMUL_1x2x1_2C8(Xbuff6, concat(Ybuff1, Ybuff2), acc5);
            MMUL_1x2x1_2C8(Xbuff3, concat(Ybuff1, Ybuff2), acc6);
            MMUL_1x2x1_2C8(Xbuff7, concat(Ybuff1, Ybuff2), acc7);
        }

        p_b = INCR_ITR2D(p_b, iterator_bias); // 2*C8 valid + 2*C8 dummy
        p_w = INCR_ITR2D(p_w, iterator_wts);
        p_in = INCR_ITR2D(p_in, iterator_outer);

        vector<bfloat16, 16> Sbuff0 = acc0.template to_vector<bfloat16>();
        vector<bfloat16, 16> Sbuff1 = acc1.template to_vector<bfloat16>();
        std::tie(Sbuff0, Sbuff1) = interleave_zip(Sbuff0, Sbuff1, params.shfMode);
        store_v(p_out, Sbuff0);
        p_out = byte_incr(p_out, 32);
        store_v(p_out, Sbuff1);
        p_out = byte_incr(p_out, params.stepS);

        Sbuff0 = acc2.template to_vector<bfloat16>();
        Sbuff1 = acc3.template to_vector<bfloat16>();
        std::tie(Sbuff0, Sbuff1) = interleave_zip(Sbuff0, Sbuff1, params.shfMode);
        store_v(p_out, Sbuff0);
        p_out = byte_incr(p_out, 32);
        store_v(p_out, Sbuff1);
        p_out = byte_incr(p_out, params.stepS);

        Sbuff0 = acc4.template to_vector<bfloat16>();
        Sbuff1 = acc5.template to_vector<bfloat16>();
        std::tie(Sbuff0, Sbuff1) = interleave_zip(Sbuff0, Sbuff1, params.shfMode);
        store_v(p_out, Sbuff0);
        p_out = byte_incr(p_out, 32);
        store_v(p_out, Sbuff1);
        p_out = byte_incr(p_out, params.stepS);

        Sbuff0 = acc6.template to_vector<bfloat16>();
        Sbuff1 = acc7.template to_vector<bfloat16>();
        std::tie(Sbuff0, Sbuff1) = interleave_zip(Sbuff0, Sbuff1, params.shfMode);
        store_v(p_out, Sbuff0);
        p_out = byte_incr(p_out, 32);
        store_v(p_out, Sbuff1);
        p_out = INCR_ITR2D(p_out, iterator_result);
    }
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22
    #define LOAD_32(ifm,ibuff) \
    {\
        ibuff.insert(0, load_v<32>(ifm));ifm+=32;\
    }

    #define VMUL_W4C8(ifm,wts,acc) \
    {\
        acc = mac_elem_32(ifm, wts, acc); \
    }

    vector<bfloat16, 32> Xbuff0, Xbuff1, Xbuff2, Xbuff3;
    vector<bfloat16, 32> Xbuff4, Xbuff5, Xbuff6, Xbuff7, Xbuff8;
    vector<bfloat16, 32> Ybuff0, Ybuff1, Ybuff2, Ybuff3;

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (unsigned j = 0; j < params.outer_g; j++){
        aie::accum<accfloat, 32> tacc0;
        tacc0.from_vector(load_v<32>( p_b ));
        aie::accum<accfloat, 32> acc0 = tacc0;
        aie::accum<accfloat, 32> acc1 = acc0;
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(2,) 
        for (unsigned i = 0; i < params.inner_g; i++){
            LOAD_32(p_in, Xbuff0);
            LOAD_32(p_in, Xbuff4);
            Xbuff8 = load_v<32>(p_in);
            p_in = INCR_ITR2D(p_in, iterator_inner);

            Xbuff1 = shuffle_down_fill(Xbuff0, Xbuff4, 8);
            Xbuff2 = shuffle_down_fill(Xbuff0, Xbuff4, 16);
            Xbuff3 = shuffle_down_fill(Xbuff0, Xbuff4, 24);
            Xbuff5 = shuffle_down_fill(Xbuff4, Xbuff8, 8);
            Xbuff6 = shuffle_down_fill(Xbuff4, Xbuff8, 16);
            Xbuff7 = shuffle_down_fill(Xbuff4, Xbuff8, 24);

            Ybuff0 = load_v<32>(p_w);
            p_w = byte_incr(p_w, 64);
            Ybuff1 = extract_v8bfloat16_broadcast_to_v32bfloat16(Ybuff0, 1);
            Ybuff2 = extract_v8bfloat16_broadcast_to_v32bfloat16(Ybuff0, 2);
            Ybuff3 = extract_v8bfloat16_broadcast_to_v32bfloat16(Ybuff0, 3);
            Ybuff0 = extract_v8bfloat16_broadcast_to_v32bfloat16(Ybuff0, 0);

            VMUL_W4C8(Xbuff0, Ybuff0, acc0);
            VMUL_W4C8(Xbuff1, Ybuff1, acc0);
            VMUL_W4C8(Xbuff2, Ybuff2, acc0);
            VMUL_W4C8(Xbuff3, Ybuff3, acc0);
            VMUL_W4C8(Xbuff4, Ybuff0, acc1);
            VMUL_W4C8(Xbuff5, Ybuff1, acc1);
            VMUL_W4C8(Xbuff6, Ybuff2, acc1);
            VMUL_W4C8(Xbuff7, Ybuff3, acc1);
        }

        p_b = INCR_ITR2D(p_b, iterator_bias); // 2*C8 valid + 2*C8 dummy
        p_w = INCR_ITR2D(p_w, iterator_wts);
        p_in = INCR_ITR2D(p_in, iterator_outer);

        vector<bfloat16, 32> Sbuff0 = acc0.template to_vector<bfloat16>();
        vector<bfloat16, 32> Sbuff1 = acc1.template to_vector<bfloat16>();
        store_v(p_out, Sbuff0);
        p_out = byte_incr(p_out, params.stepS);
        Sbuff1 = shuffle(Sbuff0, Sbuff1, params.shfMode);
        store_v(p_out, Sbuff1);
        p_out = INCR_ITR2D(p_out, iterator_result);

    }
#endif

}
#endif //__CONV1D_DW_H__
