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


#ifndef __PIXELSHUFFLE_H__
#define __PIXELSHUFFLE_H__

#include "aie.hpp"
#include <adf.h>

#include "pixelshuffle_params.h"

using namespace aie;


template<typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
__attribute__((noinline)) void pixelshuffle
(
    dtype * __restrict ifm_ptr,
    dtype * __restrict ofm_ptr,
    pixelshuffle_params_t  &pixelshuffle_params
){

    dtype __aie_dm_resource_a * input_ptr = (dtype __aie_dm_resource_a *) ifm_ptr;
    dtype __aie_dm_resource_a * input_ptr_orig = (dtype __aie_dm_resource_a *) ifm_ptr;
    dtype __aie_dm_resource_b * output_ptr = (dtype __aie_dm_resource_b *) ofm_ptr;
    dtype __aie_dm_resource_b * output_ptr_orig = (dtype __aie_dm_resource_b *) ofm_ptr;

    SETUP_ITR3D_COUNTERS(itr_ifm)
    SETUP_ITR2D_COUNTERS(itr_ofm)
    SETUP_ITR2D_COUNTERS(itr_ifmBase)
    SETUP_ITR2D_COUNTERS(itr_ofmBase)

    int loop_inner = pixelshuffle_params.loop_inner;
    int loop_outer = pixelshuffle_params.loop_outer;


    for(int i = 0; i < loop_outer; i++){

                input_ptr  = input_ptr_orig;
                output_ptr = output_ptr_orig;

                AIE_LOOP_RANGE(2,) 
                for(int n =0; n < loop_inner; n++){
                    aie::vector<dtype, PXSF_VEC_LEN> vect_ofm;
                    vect_ofm[0] = *input_ptr;
                    input_ptr  = INCR_ITR3D_PARAM(input_ptr,  itr_ifm, pixelshuffle_params) ;
                    vect_ofm[1] = *input_ptr;
                    input_ptr  = INCR_ITR3D_PARAM(input_ptr,  itr_ifm, pixelshuffle_params) ;
                    vect_ofm[2] = *input_ptr;
                    input_ptr  = INCR_ITR3D_PARAM(input_ptr,  itr_ifm, pixelshuffle_params) ;
                    vect_ofm[3] = *input_ptr;
                    input_ptr  = INCR_ITR3D_PARAM(input_ptr,  itr_ifm, pixelshuffle_params) ;
                    vect_ofm[4] = *input_ptr;
                    input_ptr  = INCR_ITR3D_PARAM(input_ptr,  itr_ifm, pixelshuffle_params) ;
                    vect_ofm[5] = *input_ptr;
                    input_ptr  = INCR_ITR3D_PARAM(input_ptr,  itr_ifm, pixelshuffle_params) ;
                    vect_ofm[6] = *input_ptr;
                    input_ptr  = INCR_ITR3D_PARAM(input_ptr,  itr_ifm, pixelshuffle_params) ;
                    vect_ofm[7] = *input_ptr;
                    input_ptr  = INCR_ITR3D_PARAM(input_ptr,  itr_ifm, pixelshuffle_params) ;

                    vect_ofm[8] = *input_ptr;
                    input_ptr  = INCR_ITR3D_PARAM(input_ptr,  itr_ifm, pixelshuffle_params) ;
                    vect_ofm[9] = *input_ptr;
                    input_ptr  = INCR_ITR3D_PARAM(input_ptr,  itr_ifm, pixelshuffle_params) ;
                    vect_ofm[10] = *input_ptr;
                    input_ptr  = INCR_ITR3D_PARAM(input_ptr,  itr_ifm, pixelshuffle_params) ;
                    vect_ofm[11] = *input_ptr;
                    input_ptr  = INCR_ITR3D_PARAM(input_ptr,  itr_ifm, pixelshuffle_params) ;
                    vect_ofm[12] = *input_ptr;
                    input_ptr  = INCR_ITR3D_PARAM(input_ptr,  itr_ifm, pixelshuffle_params) ;
                    vect_ofm[13] = *input_ptr;
                    input_ptr  = INCR_ITR3D_PARAM(input_ptr,  itr_ifm, pixelshuffle_params) ;
                    vect_ofm[14] = *input_ptr;
                    input_ptr  = INCR_ITR3D_PARAM(input_ptr,  itr_ifm, pixelshuffle_params) ;
                    vect_ofm[15] = *input_ptr;
                    input_ptr  = INCR_ITR3D_PARAM(input_ptr,  itr_ifm, pixelshuffle_params) ;

                    store_v(output_ptr, vect_ofm);
                    output_ptr = INCR_ITR2D_PARAM(output_ptr, itr_ofm, pixelshuffle_params) ;

                }
                input_ptr_orig   = INCR_ITR2D_PARAM(input_ptr_orig,  itr_ifmBase, pixelshuffle_params) ;
                output_ptr_orig  = INCR_ITR2D_PARAM(output_ptr_orig, itr_ofmBase, pixelshuffle_params) ;
        }

}

template<typename dtype, typename config_ifm, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
INLINE void pixelshuffle
(
    adf::input_buffer_conf<dtype, config_ifm>  & __restrict ifm,
    adf::output_buffer_conf<dtype, config_ofm> & __restrict ofm,
    pixelshuffle_params_t  &pixelshuffle_params
){
    return pixelshuffle<dtype>(ifm.data(), ofm.data(), pixelshuffle_params);
}

#endif //__PIXELSHUFFLE_H__

