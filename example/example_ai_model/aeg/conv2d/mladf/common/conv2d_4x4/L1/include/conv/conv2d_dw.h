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
 * 2D Depthwise Convolution Kernel for AI Engine
 *
 * Performs 2d convolution on data present in "ifms" using weights from "wts"
 * Result is stored in "ofms".
 *
 */

#ifndef __CONV2D_DW_H__
#define __CONV2D_DW_H__

#include <adf.h>
#include "aie.hpp"

#include "utils.h"              // Contains some utility functions and definitions related to intrinsics usage
#include "mllib_const.h"        // Constants shared across graphs like activation type
#include "conv2d_dw_params.h" // Parameters like increments and wraps needed by kernel

using namespace aie;

// Removed reference to windows from this lower level code
template <uint8_t batch_size, typename dtype_ifm, typename dtype_wts, typename dtype_ofm, typename config_ifm, typename config_wts, typename config_ofm> 
NOINLINE void conv2d_dw
(
    adf::input_buffer_conf<dtype_ifm, config_ifm>  & __restrict input,    // Input-0 this should be from IFM
    adf::input_buffer_conf<dtype_wts, config_wts>  & __restrict weights, // Input-1 this should be from WTS
    adf::output_buffer_conf<dtype_ofm, config_ofm> & __restrict output,  // Output-1 this should be OFM of conv2d output
    conv2d_dw_params_t &params
) {

    dtype_ofm OFM_DM_BANK  * p_out  = (dtype_ofm  OFM_DM_BANK *) output.data();
    dtype_ifm IFM_DM_BANK  * p_in   = (dtype_ifm  IFM_DM_BANK *) input.data(); 
    dtype_wts WT_DM_BANK   * p_w    = (dtype_wts   WT_DM_BANK  *) weights.data() + (LAYER_PARAM_BYTES + params.ofm*2*4);;
    int16     WT_DM_BANK   * p_b    = (int16   WT_DM_BANK  *) weights.data() + (LAYER_PARAM_BYTES/2);
    //declaration of registers 
    vector<dtype_wts,64> Ybuff0 = zeros<dtype_wts,64>();
    vector<dtype_ofm,64> SS     = zeros<dtype_ofm,64>();
    vector<dtype_ifm,64> Xbuff0, Xbuff2, Xbuff4, Xbuff1, Xbuff3, Xbuff5;
    vector<dtype_ofm,64> Sbuff0, Sbuff1;

    using mac_conv_884 = aie::sliding_mul_ch_x_ops<8, 8, 4, 1, 1, int8, int8, acc32>; //HL-API for depthwise convolution
    DECL_ITR3D(iterator_inner)
    DECL_ITR3D(iterator_outer)
    DECL_ITR3D(iterator_output)
    DECL_ITR2D(iterator_bias)
    DECL_ITR2D(iterator_wts)

    //Loading 64-byte values      
    #define LOAD_64(ifm,ibuff,ifm_incr) \
    {\
        ibuff.insert(0, load_v<32>(ifm));ifm+=32;\
        ibuff.insert(1, load_v<32>(ifm));ifm+=ifm_incr;\
    }  
    //declaration of counters and iterators
    SETUP_ITR3D_COUNTERS(iterator_inner)
    SETUP_ITR3D_COUNTERS(iterator_outer)
    SETUP_ITR3D_COUNTERS(iterator_output)
    SETUP_ITR2D_COUNTERS(iterator_bias)
    SETUP_ITR2D_COUNTERS(iterator_wts)
    SETUP_ITR3D(iterator_inner, params.incAL1, params.numAL1, params.incAL2, params.numAL2, params.incAL3)
    SETUP_ITR3D(iterator_outer, params.incAO1, params.numAO1, params.incAO2, params.numAO2, params.incAO3)
    SETUP_ITR3D(iterator_output, params.incS1, params.numS1, params.incS2, params.numS2, params.incS3)
    SETUP_ITR2D(iterator_wts, params.incB1, params.numB, params.incB2)
    SETUP_ITR2D(iterator_bias, params.ofmB1, params.numofm, params.ofmB2)
   
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2,) 
    for (unsigned j=0; j<params.outer_g; j++){
	//Load bias values       
        aie::accum<acc32,32> tacc0;
       	tacc0.from_vector( load_v<32>( p_b ),params.bias_shift);
        aie::accum<acc32,64> acc0 = aie::concat(tacc0,tacc0);
        aie::accum<acc32,64> acc1 = acc0;
        aie::accum<acc32,64> acc2 = acc0;
        aie::accum<acc32,64> acc3 = acc0;
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(2,) 
        for (int i=0; i<params.inner_g; i++){
            //First row IFM loading  
	    LOAD_64(p_in,Xbuff0,32); 
	    Xbuff1.insert(0, load_v<32>(p_in)); p_in = byte_incr(p_in, 32); 
            Xbuff1.insert(1, load_v<32>(p_in)); p_in = byte_incr(p_in, 32);
	    Xbuff2.insert(0, load_v<32>(p_in)); p_in = byte_incr(p_in, params.incAI1); 

            //Second row IFM loading	
            LOAD_64(p_in,Xbuff3,32);
	    Xbuff4.insert(0, load_v<32>(p_in));  p_in = byte_incr(p_in, 32);  
            Xbuff4.insert(1, load_v<32>(p_in));  p_in = byte_incr(p_in, 32); 
	    Xbuff5.insert(0, load_v<32>(p_in));  p_in = INCR_ITR3D(p_in, iterator_inner) 

	    //Weights are loading row-by-row
            Ybuff0.insert(0, load_v<32>(p_w)); p_w = INCR_ITR2D(p_w, iterator_wts) 
            //Processing of IFM and Weights
            acc0 = mac_conv_884::mac(acc0,Ybuff0. template extract<32>(0), 0, aie::concat(Xbuff0, Xbuff1), 0); 
            acc1 = mac_conv_884::mac(acc1,Ybuff0. template extract<32>(0), 0, aie::concat(Xbuff1, Xbuff2), 0);
            acc2 = mac_conv_884::mac(acc2,Ybuff0. template extract<32>(0), 0, aie::concat(Xbuff3, Xbuff4), 0);
            acc3 = mac_conv_884::mac(acc3,Ybuff0. template extract<32>(0), 0, aie::concat(Xbuff4, Xbuff5), 0);
        }

        p_in = INCR_ITR3D(p_in, iterator_outer) 
        p_b  = INCR_ITR2D(p_b,  iterator_bias) 

        Sbuff0 = acc0.template to_vector_sign<dtype_ofm>(params.result_sign, params.shift_res);
        Sbuff1 = acc1.template to_vector_sign<dtype_ofm>(params.result_sign, params.shift_res);

        store_v(p_out, Sbuff0. template extract<32>(0)); p_out = byte_incr(p_out, params.stepAS);
        Sbuff0 = shuffle(Sbuff0, SS, params.shftAS);
        store_v(p_out, Sbuff0. template extract<32>(0)); p_out = byte_incr(p_out, 32);
        store_v(p_out, Sbuff1. template extract<32>(0)); p_out = byte_incr(p_out, params.stepAS);
        Sbuff1 = shuffle(Sbuff1, SS, params.shftAS);
        store_v(p_out, Sbuff1. template extract<32>(0)); p_out = byte_incr(p_out, params.incS0);

        Sbuff0 = acc2.template to_vector_sign<dtype_ofm>(params.result_sign, params.shift_res);
        Sbuff1 = acc3.template to_vector_sign<dtype_ofm>(params.result_sign, params.shift_res);

        store_v(p_out, Sbuff0. template extract<32>(0)); p_out = byte_incr(p_out, params.stepAS);
        Sbuff0 = shuffle(Sbuff0, SS, params.shftAS);
        store_v(p_out, Sbuff0. template extract<32>(0)); p_out = byte_incr(p_out, 32);
        store_v(p_out, Sbuff1. template extract<32>(0)); p_out = byte_incr(p_out, params.stepAS);
        Sbuff1 = shuffle(Sbuff1, SS, params.shftAS);
        store_v(p_out, Sbuff1. template extract<32>(0)); 
        p_out = INCR_ITR3D(p_out, iterator_output)  
    }
#ifdef DWC_RELU_INT8_ENABLE
    if(params.act == 1) { //RELU
        int OFM_DM_BANK * p_act  = (int OFM_DM_BANK *) output.data();
        relu_int8_post_process(p_act, params.result_sign, params.ofm_elem_size);
    }
#endif    
}
#endif //__CONV2D_DW_H__

