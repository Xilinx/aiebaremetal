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
 * GEMV kernel for AI Engine gran 32x64x32
 *
 * Performs matrix-vector mult on data present in "ifms" using weights from "wts"
 * Result is stored in "ofms".
 *
 */

#ifndef __GEMV_H__
#define __GEMV_H__

#include <adf.h>
#include "aie.hpp"

#include "utils.h"         // Contains some utility functions and definitions related to intrinsics usage
#include "mllib_const.h"   // Constants shared across graphs like activation type
#include "gemv_params.h" // Parameters like increments and wraps needed by kernel

using namespace aie;


template <uint8_t batch_size, act_t act_type, out_mode_t out_mode, typename dtype_ifm, typename dtype_wts, typename dtype_ofm>
NOINLINE void gemv_generic
(
        int * __restrict input,
        int * __restrict weights,
        int * __restrict acc_in,
        int * __restrict output,
        gemv_params_t            &gemv_params
) {

    int16      OFM_DM_BANK  * p_sum   = ((int16      OFM_DM_BANK *) output + (gemv_params.psum_buff_offset>>1));
    dtype_ifm  IFM_DM_BANK  * p_in    = (dtype_ifm  IFM_DM_BANK *) input;
    dtype_wts  WT_DM_BANK   * p_w     = (dtype_wts   WT_DM_BANK  *) weights;
    vector<dtype_ifm, 32> vec_Xbuff0;
    int16      OFM_DM_BANK * p_init16 = (int16 OFM_DM_BANK *) acc_in;
    dtype_ofm OFM_DM_BANK * p_out    = (gemv_params.psum_out==0)?(dtype_ofm OFM_DM_BANK *)output : (dtype_ofm OFM_DM_BANK *)p_sum;


    using mmul = mmul<DIM_M, DIM_N, DIM_K, dtype_ifm, dtype_wts>;
    vector<dtype_wts, 64>  Ybuff0, Ybuff1,Ybuff2,Ybuff3, Ybuff4,Ybuff5,Ybuff6, Ybuff7;

    #define LOAD_WEIGHTS(wts,wbuff,wts_incr) \
    {\
        wbuff.insert(0, load_v<32>(wts));wts+=32;\
        wbuff.insert(1, load_v<32>(wts));wts+=wts_incr;\
    }

    int frac;
    int chnd   = 32;
    int chnd_T = 64;
    int    acc_init_shift = (gemv_params.psum_in ==  0) ? 0 : (int) gemv_params.shift_psum_in;
    int    inc_ST_0       = (gemv_params.psum_in ==  0) ? chnd_T : 2*chnd_T;

    mmul Cbuff0( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T);
    mmul Cbuff1( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T);
    mmul Cbuff2( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T);
    mmul Cbuff3( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T);
    mmul Cbuff4( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T);
    mmul Cbuff5( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T);
    mmul Cbuff6( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T);
    mmul Cbuff7( load_v<32>( p_init16 ), acc_init_shift ); p_init16 = byte_incr(p_init16, inc_ST_0-chnd_T);


        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4,) 
        for (int i=0; i<gemv_params.inner_g; i++){
            vec_Xbuff0 = load_v<32>((dtype_ifm IFM_DM_BANK *) p_in);
            p_in = p_in+32;

            LOAD_WEIGHTS(p_w,Ybuff0,32) ;
            LOAD_WEIGHTS(p_w,Ybuff1,32) ;
            LOAD_WEIGHTS(p_w,Ybuff2,32) ;
            LOAD_WEIGHTS(p_w,Ybuff3,32) ;
            LOAD_WEIGHTS(p_w,Ybuff4,32) ;
            LOAD_WEIGHTS(p_w,Ybuff5,32) ;
            LOAD_WEIGHTS(p_w,Ybuff6,32) ;
            LOAD_WEIGHTS(p_w,Ybuff7,32) ;

            Cbuff0.mac(vec_Xbuff0, Ybuff0);
            Cbuff1.mac(vec_Xbuff0, Ybuff1);
            Cbuff2.mac(vec_Xbuff0, Ybuff2);
            Cbuff3.mac(vec_Xbuff0, Ybuff3);
            Cbuff4.mac(vec_Xbuff0, Ybuff4);
            Cbuff5.mac(vec_Xbuff0, Ybuff5);
            Cbuff6.mac(vec_Xbuff0, Ybuff6);
            Cbuff7.mac(vec_Xbuff0, Ybuff7);

        }

        if (gemv_params.exec_type == 0){
                 store_v(p_out, Cbuff0.template to_vector<dtype_ofm>(gemv_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff1.template to_vector<dtype_ofm>(gemv_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff2.template to_vector<dtype_ofm>(gemv_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff3.template to_vector<dtype_ofm>(gemv_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff4.template to_vector<dtype_ofm>(gemv_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff5.template to_vector<dtype_ofm>(gemv_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff6.template to_vector<dtype_ofm>(gemv_params.shift_out)); p_out = byte_incr(p_out, 32);
                 store_v(p_out, Cbuff7.template to_vector<dtype_ofm>(gemv_params.shift_out));
	}
        else {
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff0.template to_vector<int16>(gemv_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff1.template to_vector<int16>(gemv_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff2.template to_vector<int16>(gemv_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff3.template to_vector<int16>(gemv_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff4.template to_vector<int16>(gemv_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff5.template to_vector<int16>(gemv_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff6.template to_vector<int16>(gemv_params.shift_psum_out)); p_out = (dtype_ofm OFM_DM_BANK *)byte_incr((int16*)p_out, 64);
            store_v((int16 OFM_DM_BANK *) p_out, Cbuff7.template to_vector<int16>(gemv_params.shift_psum_out));
        }

}


template <uint8_t batch_size, act_t act_type, typename dtype_ifm, typename dtype_wts, typename dtype_ofm, typename config_ifm, typename config_wts, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype_ifm, int8_t, bfloat16>) &&
            (mllib::utils::is_one_of_v<dtype_wts, int8_t, bfloat16>) &&
            (mllib::utils::is_one_of_v<dtype_ofm, int8_t, bfloat16>)
INLINE void gemv
(
        adf::input_buffer_conf<dtype_ifm, config_ifm>  & __restrict ifm,
        adf::input_buffer_conf<dtype_wts, config_wts>  & __restrict wts,
        adf::output_buffer_conf<dtype_ofm, config_ofm> &  ofm,
        gemv_params_t            &gemv_params
) {
    // Output modes
    const out_mode_t out_mode_full     = FULL ;
    const out_mode_t out_mode_partial  = PARTIAL ;

    // Setup iteration level parameters like IO mode etc
    setup_gemv_iter_params(gemv_params) ;

   // Extract low level pointers used by the actual kernel code
    int * input   = (int*) ifm.data();
    int * weights = (int*) wts.data() + gemv_params.hdr_len;
    int * output  = (int*) ofm.data();
    int * acc_in  = (gemv_params.psum_in == 0) ? (int*) wts.data() + LAYER_PARAM_WORDS : ((int*) ofm.data() + (gemv_params.psum_buff_offset>>2));

    gemv_generic<batch_size,act_type,out_mode_full,dtype_ifm,dtype_wts,dtype_ofm>(input, weights, acc_in, output,gemv_params);

    //switch(gemv_params.exec_type) {
    //case 0:
    //case 2:
    //     gemv_generic<batch_size,act_type,out_mode_full,dtype_ifm,dtype_wts,dtype_ofm>(input, weights, acc_in, output,gemv_params);
    //     break ;
    // case 3:
    //      gemv_generic<batch_size,act_type,out_mode_partial,dtype_ifm,dtype_wts,dtype_ofm>(input, weights, acc_in, output,gemv_params);
    //      break ;
    //  }
}

#endif //__CONV2D_H__

