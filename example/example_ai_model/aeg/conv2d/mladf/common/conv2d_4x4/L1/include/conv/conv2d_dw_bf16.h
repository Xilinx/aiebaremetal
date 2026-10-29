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

#pragma once

#include <adf.h>
#include "aie.hpp"
#include "aie_api/utils.hpp" // needed for aie:print

#include "utils.h"              // Contains some utility functions and definitions related to intrinsics usage
#include "mllib_const.h"        // Constants shared across graphs like activation type
#include "conv2d_dw_bf16_params.h" // Parameters like increments and wraps needed by kernel

using namespace aie;

#if __AIE_ARCH__ == 20
template <uint8_t batch_size, typename dtype_ifm, typename dtype_wts, typename dtype_ofm, typename config_ifm, typename config_wts, typename config_ofm>
requires(std::is_same_v<dtype_ifm, bfloat16>)
NOINLINE void conv2d_dw (
    adf::input_buffer_conf<dtype_ifm, config_ifm>  & __restrict input,    // Input-0 this should be from IFM
    adf::input_buffer_conf<dtype_wts, config_wts>  & __restrict weights, // Input-1 this should be from WTS
    adf::output_buffer_conf<dtype_ofm, config_ofm> & __restrict output,  // Output-1 this should be OFM of conv2d output
    conv2d_dw_bf16_params_t &params
) {
    bfloat16 OFM_DM_BANK *p_out = (bfloat16 OFM_DM_BANK *)output.data();
    bfloat16 IFM_DM_BANK *p_in = (bfloat16 IFM_DM_BANK *)input.data();
    bfloat16 WT_DM_BANK *p_weight =
        (bfloat16 WT_DM_BANK *)weights.data() + (LAYER_PARAM_BYTES / 2 + params.ofm * 4);
    bfloat16 WT_DM_BANK *p_bias = (bfloat16 WT_DM_BANK *)weights.data() + (LAYER_PARAM_BYTES / 2);

    // Load weight data
    vector<bfloat16,16> Wbuff0;
    // Load ifm data
    vector<bfloat16,32> Xbuff0, Xbuff1, Xbuff2, Xbuff3, Xbuff4, Xbuff5, Xbuff6, Xbuff7;
    vector<bfloat16,32> Xbuff16, Xbuff17;
    // For xbffuff shuffle
    vector<bfloat16,32> SXbuff0, SXbuff1, SXbuff2, SXbuff3, SXbuff4, SXbuff5, SXbuff6, SXbuff7;
    // For Wbuff shuffle
    vector<bfloat16,16> SWbuff0, SWbuff1;

    DECL_ITR3D(iterator_inner)
    DECL_ITR3D(iterator_outer)
    DECL_ITR3D(iterator_output)
    DECL_ITR2D(iterator_bias)
    DECL_ITR2D(iterator_wts)
    SETUP_ITR3D_COUNTERS(iterator_inner)
    SETUP_ITR3D_COUNTERS(iterator_outer)
    SETUP_ITR3D_COUNTERS(iterator_output)
    SETUP_ITR2D_COUNTERS(iterator_bias)
    SETUP_ITR2D_COUNTERS(iterator_wts)
    SETUP_ITR3D(iterator_inner, params.inc_inner1, params.num_inner1, params.inc_inner2, params.num_inner2, params.inc_inner3)
    SETUP_ITR3D(iterator_outer, params.inc_outer1, params.num_outer1, params.inc_outer2, params.num_outer2, params.inc_outer3)
    SETUP_ITR3D(iterator_output, params.inc_output1, params.num_output1, params.inc_output2, params.num_output2, params.inc_output3)
    SETUP_ITR2D(iterator_wts, params.inc_weight1, params.num_weight, params.inc_weight2)
    SETUP_ITR2D(iterator_bias, params.inc_bias1, params.num_bias, params.inc_bias2)

    #define LOAD_32(ifm,ibuff) \
    {\
        ibuff.insert(0, load_v<16>(ifm));ifm+=16;\
        ibuff.insert(1, load_v<16>(ifm));ifm+=16;\
    }
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (unsigned j = 0; j < params.outer_g; j++){
        aie::accum<accfloat, 16> acc0;
       	acc0.from_vector(load_v<16>(p_bias));
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
            // load ifm and shuffle
            // first row of ifm
            LOAD_32(p_in, Xbuff0)
            LOAD_32(p_in, Xbuff2)
            Xbuff16.insert(0, load_v<16>(p_in));
            p_in = byte_incr(p_in, params.inc_input_next_line);
            // second row of ifm
            LOAD_32(p_in, Xbuff4)
            LOAD_32(p_in, Xbuff6)
            Xbuff17.insert(0, load_v<16>(p_in));
            p_in = INCR_ITR3D(p_in, iterator_inner)

            Xbuff1 = shuffle_down_fill(Xbuff0, Xbuff2,  8);
            Xbuff3 = shuffle_down_fill(Xbuff2, Xbuff16, 8);
            Xbuff5 = shuffle_down_fill(Xbuff4, Xbuff6, 8);
            Xbuff7 = shuffle_down_fill(Xbuff6, Xbuff17, 8);
            std::tie(SXbuff0, SXbuff1) = interleave_zip(Xbuff0, Xbuff1, 8);
            std::tie(SXbuff2, SXbuff3) = interleave_zip(Xbuff2, Xbuff3, 8);
            std::tie(SXbuff4, SXbuff5) = interleave_zip(Xbuff4, Xbuff5, 8);
            std::tie(SXbuff6, SXbuff7) = interleave_zip(Xbuff6, Xbuff7, 8);

            // load wts and shuffle
            Wbuff0=load_v<16>(p_weight);
            p_weight = INCR_ITR2D(p_weight, iterator_wts)
            std::tie(SWbuff0, SWbuff1) = interleave_zip(Wbuff0, Wbuff0, 8);

            acc0 = mac_elem_16_2(SXbuff0, concat(SWbuff0, SWbuff1), acc0);
            acc1 = mac_elem_16_2(SXbuff1, concat(SWbuff0, SWbuff1), acc1);
            acc2 = mac_elem_16_2(SXbuff2, concat(SWbuff0, SWbuff1), acc2);
            acc3 = mac_elem_16_2(SXbuff3, concat(SWbuff0, SWbuff1), acc3);
            acc4 = mac_elem_16_2(SXbuff4, concat(SWbuff0, SWbuff1), acc4);
            acc5 = mac_elem_16_2(SXbuff5, concat(SWbuff0, SWbuff1), acc5);
            acc6 = mac_elem_16_2(SXbuff6, concat(SWbuff0, SWbuff1), acc6);
            acc7 = mac_elem_16_2(SXbuff7, concat(SWbuff0, SWbuff1), acc7);
        }
        p_in = INCR_ITR3D(p_in, iterator_outer)
        p_bias  = INCR_ITR2D(p_bias,  iterator_bias)

        // save output to first line
        aie::vector<bfloat16, 16> OutBuff0, OutBuff1;
        OutBuff0 = acc0.template to_vector<bfloat16>();
        OutBuff1 = acc1.template to_vector<bfloat16>();
        std::tie(OutBuff0, OutBuff1) = interleave_zip(OutBuff0, OutBuff1, params.shift_mode);
        store_v(p_out, aie::max(OutBuff0, params.act_bar)); p_out = byte_incr(p_out, 32);
        store_v(p_out, aie::max(OutBuff1, params.act_bar)); p_out = byte_incr(p_out, params.inc_output_step);
        aie::vector<bfloat16, 16> OutBuff2, OutBuff3;
        OutBuff2 = acc2.template to_vector<bfloat16>();
        OutBuff3 = acc3.template to_vector<bfloat16>();
        std::tie(OutBuff2, OutBuff3) = interleave_zip(OutBuff2, OutBuff3, params.shift_mode);
        store_v(p_out, aie::max(OutBuff2, params.act_bar)); p_out = byte_incr(p_out, params.inc_stride_output_0);
        store_v(p_out, aie::max(OutBuff3, params.act_bar)); p_out = byte_incr(p_out, params.inc_stride_output_1);
        // save output to second line
        aie::vector<bfloat16, 16> OutBuff4, OutBuff5;
        OutBuff4 = acc4.template to_vector<bfloat16>();
        OutBuff5 = acc5.template to_vector<bfloat16>();
        std::tie(OutBuff4, OutBuff5) = interleave_zip(OutBuff4, OutBuff5, params.shift_mode);
        store_v(p_out, aie::max(OutBuff4, params.act_bar)); p_out = byte_incr(p_out, 32);
        store_v(p_out, aie::max(OutBuff5, params.act_bar)); p_out = byte_incr(p_out, params.inc_output_step);
        aie::vector<bfloat16, 16> OutBuff6, OutBuff7;
        OutBuff6 = acc6.template to_vector<bfloat16>();
        OutBuff7 = acc7.template to_vector<bfloat16>();
        std::tie(OutBuff6, OutBuff7) = interleave_zip(OutBuff6, OutBuff7, params.shift_mode);
        // For stride=2, to avoid wrong writing to other address
        OutBuff7 = shuffle_down_fill(OutBuff6, OutBuff7, params.shift_mode_output);
        store_v(p_out, aie::max(OutBuff6, params.act_bar)); p_out = byte_incr(p_out, params.inc_stride_output_2);
        store_v(p_out, aie::max(OutBuff7, params.act_bar)); p_out = INCR_ITR3D(p_out, iterator_output)
    }
    #undef LOAD_32
}
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22
template <uint8_t batch_size, typename dtype_ifm, typename dtype_wts, typename dtype_ofm, typename config_ifm, typename config_wts, typename config_ofm>
requires(std::is_same_v<dtype_ifm, bfloat16>)
NOINLINE void conv2d_dw (
    adf::input_buffer_conf<dtype_ifm, config_ifm>  & __restrict input,    // Input-0 this should be from IFM
    adf::input_buffer_conf<dtype_wts, config_wts>  & __restrict weights, // Input-1 this should be from WTS
    adf::output_buffer_conf<dtype_ofm, config_ofm> & __restrict output,  // Output-1 this should be OFM of conv2d output
    conv2d_dw_bf16_params_t &params
) {
    bfloat16 OFM_DM_BANK *p_out = (bfloat16 OFM_DM_BANK *)output.data();
    bfloat16 IFM_DM_BANK *p_in = (bfloat16 IFM_DM_BANK *)input.data();
    bfloat16 WT_DM_BANK *p_weight =
        (bfloat16 WT_DM_BANK *)weights.data() + (LAYER_PARAM_BYTES / 2 + params.ofm * 4);
    bfloat16 WT_DM_BANK *p_bias = (bfloat16 WT_DM_BANK *)weights.data() + (LAYER_PARAM_BYTES / 2);

    vector<bfloat16, 32> Xbuff0, Xbuff1, Xbuff2, Xbuff3;
    vector<bfloat16, 32> Xbuff4, Xbuff5, Xbuff6, Xbuff7, Xbuff8;
    vector<bfloat16, 32> Wbuff0, Wbuff1, Wbuff2, Wbuff3;

    DECL_ITR3D(iterator_inner)
    DECL_ITR3D(iterator_outer)
    DECL_ITR3D(iterator_output)
    DECL_ITR2D(iterator_bias)
    DECL_ITR2D(iterator_wts)
    SETUP_ITR3D_COUNTERS(iterator_inner)
    SETUP_ITR3D_COUNTERS(iterator_outer)
    SETUP_ITR3D_COUNTERS(iterator_output)
    SETUP_ITR2D_COUNTERS(iterator_bias)
    SETUP_ITR2D_COUNTERS(iterator_wts)
    SETUP_ITR3D(iterator_inner, params.inc_inner1, params.num_inner1, params.inc_inner2, params.num_inner2, params.inc_inner3)
    SETUP_ITR3D(iterator_outer, params.inc_outer1, params.num_outer1, params.inc_outer2, params.num_outer2, params.inc_outer3)
    SETUP_ITR3D(iterator_output, params.inc_output1, params.num_output1, params.inc_output2, params.num_output2, params.inc_output3)
    SETUP_ITR2D(iterator_wts, params.inc_weight1, params.num_weight, params.inc_weight2)
    SETUP_ITR2D(iterator_bias, params.inc_bias1, params.num_bias, params.inc_bias2)

    #define LOAD_32(ifm,ibuff) \
        ibuff.insert(0, load_v<32>(ifm));ifm+=32;

#ifndef __X86SIM__
    #define VMUL_W4C8(ifm,wts,acc) \
        acc = mac_elem_32(ifm, wts, acc);
    #define BROADCAST8to32(from,to,index) \
        to = extract_v8bfloat16_broadcast_to_v32bfloat16(from, index);
#else
    #define VMUL_W4C8(ifm,wts,acc) \
        acc = mac(acc, ifm, wts);
    #define BROADCAST8to32(from,to,index) \
        to = concat(from.template extract<8>(index), from.template extract<8>(index), from.template extract<8>(index), from.template extract<8>(index));
#endif

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (unsigned j = 0; j < params.outer_g; j++){
        aie::accum<accfloat, 32> acc0;
       	acc0.from_vector(load_v<32>(p_bias));
        aie::accum<accfloat, 32> acc1 = acc0;
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(2,) 
        for (unsigned i = 0; i < params.inner_g; i++){
            LOAD_32(p_in, Xbuff0);
            LOAD_32(p_in, Xbuff4);
            Xbuff8 = load_v<32>(p_in);
            p_in = INCR_ITR3D(p_in, iterator_inner);

            Xbuff1 = shuffle_down_fill(Xbuff0, Xbuff4, 8);
            Xbuff2 = shuffle_down_fill(Xbuff0, Xbuff4, 16);
            Xbuff3 = shuffle_down_fill(Xbuff0, Xbuff4, 24);
            Xbuff5 = shuffle_down_fill(Xbuff4, Xbuff8, 8);
            Xbuff6 = shuffle_down_fill(Xbuff4, Xbuff8, 16);
            Xbuff7 = shuffle_down_fill(Xbuff4, Xbuff8, 24);

            Wbuff0 = load_v<32>(p_weight);
            p_weight = INCR_ITR2D(p_weight, iterator_wts)
            BROADCAST8to32(Wbuff0, Wbuff1, 1)
            BROADCAST8to32(Wbuff0, Wbuff2, 2)
            BROADCAST8to32(Wbuff0, Wbuff3, 3)
            BROADCAST8to32(Wbuff0, Wbuff0, 0)

            VMUL_W4C8(Xbuff0, Wbuff0, acc0);
            VMUL_W4C8(Xbuff1, Wbuff1, acc0);
            VMUL_W4C8(Xbuff2, Wbuff2, acc0);
            VMUL_W4C8(Xbuff3, Wbuff3, acc0);
            VMUL_W4C8(Xbuff4, Wbuff0, acc1);
            VMUL_W4C8(Xbuff5, Wbuff1, acc1);
            VMUL_W4C8(Xbuff6, Wbuff2, acc1);
            VMUL_W4C8(Xbuff7, Wbuff3, acc1);
        }
        p_bias  = INCR_ITR2D(p_bias,  iterator_bias)
        p_in = INCR_ITR3D(p_in, iterator_outer)

        vector<bfloat16, 32> Sbuff0 = acc0.template to_vector<bfloat16>();
        vector<bfloat16, 32> Sbuff1 = acc1.template to_vector<bfloat16>();
        store_v(p_out, aie::max(Sbuff0, params.act_bar));
        p_out = byte_incr(p_out, params.inc_output_step);
        Sbuff1 = shuffle(Sbuff0, Sbuff1, params.shift_mode);
        store_v(p_out, aie::max(Sbuff1, params.act_bar));
        p_out = INCR_ITR3D(p_out, iterator_output);
    }
}
#endif // __AIE_ARCH__

