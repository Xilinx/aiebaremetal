/*
    Copyright (C) 2014 - 2022 Xilinx, Inc. All rights reserved.
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



#ifndef __TRANSPOSE_PARAMS_H__
#define __TRANSPOSE_PARAMS_H__

#include "aie.hpp"
#include <adf.h>
#include <aie_api/utils.hpp>

#include "utils.h"
#include "mllib_const.h"

using namespace aie;

#include "transpose_base.h"

struct alignas(v32uint16) transpose_3d_params {
    // input shape dimension size with padding
    uint32_t d0_size;
    uint32_t d1_size;
    uint32_t d2_size;

    // permute order
    // 0(ODR_012): d0d1d2(no permute, not allowed) 
    // allowed values {1(ODR_021): d0d2d1; 2(ODR_120): d1d2d0; 3(ODR_102): d1d0d2; 4(ODR_210): d2d1d0; 5(ODR_201): d2d0d1;}
    uint32_t permute_odr;

    // params for loop traversal control
    uint32_t num_iters;
    uint32_t in_stride;
    uint32_t out_stride;
    DECL_ITR3D(iters_in)
    DECL_ITR3D(iters_out)

    // params for padding trimming
    uint32_t trim_loops;
    DECL_ITR2D(trim_iters_in)
    DECL_ITR2D(trim_iters_out)
};
typedef struct transpose_3d_params transpose_3d_params_t;

// setup ITR3D parameters for transposing the d2 and d1 dimension of shape (d0, d1, d2)
template <typename dtype>
void setup_transpose_internal_021(uint32_t d0_sz, uint32_t d1_sz, uint32_t d2_sz, transpose_3d_params_t &param) {
    param.num_iters  = (d0_sz * d1_sz * d2_sz * sizeof(dtype)) >> 8; // (x/256)

    param.in_stride = d2_sz * sizeof(dtype);
    param.iters_in_incr0 = 16 - d2_sz * sizeof(dtype) * 15;
    param.iters_in_incr1 = 16;
    param.iters_in_wrap0 = ((d2_sz * sizeof(dtype)) >> 4) - 1;
    param.iters_in_incr2 = 16;
    param.iters_in_wrap1 = (d1_sz >> 4) - 1;

    param.out_stride = d1_sz * sizeof(dtype);
    param.iters_out_incr0 = d1_sz * sizeof(dtype);
    param.iters_out_incr1 = (1 - d2_sz) * d1_sz * sizeof(dtype) + 16 * sizeof(dtype);
    param.iters_out_wrap0 = ((d2_sz *sizeof(dtype)) >> 4) - 1;
    param.iters_out_incr2 = 16 * sizeof(dtype);
    param.iters_out_wrap1 = (d1_sz >> 4) - 1;
}

// setup ITR3D parameters for transposing the d2 and d0 dimension of shape (d0, d1, d2)
template <typename dtype>
void setup_transpose_internal_210(uint32_t d0_sz, uint32_t d1_sz, uint32_t d2_sz, transpose_3d_params_t &param) {
    param.num_iters  = (d0_sz * d1_sz * d2_sz * sizeof(dtype)) >> 8; // (x/256)

    param.in_stride = d2_sz * d1_sz * sizeof(dtype);
    param.iters_in_incr0 = 16 - d2_sz * d1_sz * sizeof(dtype) * 15;
    param.iters_in_incr1 = d1_sz * d2_sz * sizeof(dtype) - (d2_sz * sizeof(dtype) - 16);
    param.iters_in_wrap0 = ((d2_sz * sizeof(dtype)) >> 4) - 1;
    param.iters_in_incr2 = (1 - d0_sz) * d1_sz * d2_sz * sizeof(dtype) + 16;
    param.iters_in_wrap1 = (d0_sz >> 4) - 1;

    param.out_stride = d0_sz * d1_sz * sizeof(dtype);
    param.iters_out_incr0 = d0_sz * d1_sz * sizeof(dtype);
    param.iters_out_incr1 = (1 - d2_sz) * d0_sz * d1_sz * sizeof(dtype) + 16 * sizeof(dtype);
    param.iters_out_wrap0 = ((d2_sz * sizeof(dtype)) >> 4) - 1;
    param.iters_out_incr2 = (1 - d2_sz) * d0_sz * d1_sz * sizeof(dtype) + 16 * sizeof(dtype);
    param.iters_out_wrap1 = (d0_sz >> 4) - 1;
}

// setup ITR3D parameters for transposing the d1 and d0 dimension of shape (d0, d1, d2)
template <typename dtype>
void setup_transpose_internal_102(uint32_t d0_sz, uint32_t d1_sz, uint32_t d2_sz, transpose_3d_params_t &param) {
    param.num_iters  = (d0_sz * d1_sz * d2_sz * sizeof(dtype)) >> 4; // (x/16)

    param.iters_in_incr0 = 16;
    param.iters_in_wrap0 = ((d2_sz * sizeof(dtype)) >> 4) - 1;
    param.iters_in_incr1 = 16;
    param.iters_in_wrap1 = d1_sz - 1;
    param.iters_in_incr2 = 16;

    param.iters_out_incr0 = 16;
    param.iters_out_wrap0 = ((d2_sz * sizeof(dtype)) >> 4) - 1;
    param.iters_out_incr1 = d0_sz * d2_sz * sizeof(dtype) - (d2_sz * sizeof(dtype) - 16);
    param.iters_out_wrap1 = d1_sz - 1;
    param.iters_out_incr2 = 16 - (d1_sz - 1) * d0_sz * d2_sz * sizeof(dtype);
}

// setup ITR3D parameters for any transpose permute order
template <typename dtype>
void setup_transpose_internal(transpose_3d_params_t &param3d) {
    uint32_t d0_size = param3d.d0_size;
    uint32_t d1_size = param3d.d1_size;
    uint32_t d2_size = param3d.d2_size;
    if (param3d.permute_odr == ODR_021) {
        // d0(d1|d2) => d0d2d1
        setup_transpose_internal_021<dtype>(d0_size, d1_size, d2_size, param3d);
    } else if (param3d.permute_odr == ODR_120) {
        // d0|(d1d2) => d1d2d0
        setup_transpose_internal_021<dtype>(1, d0_size, d1_size * d2_size, param3d);
    } else if (param3d.permute_odr == ODR_102) {
        // (d0|d1)d2 => d1d0d2
        setup_transpose_internal_102<dtype>(d0_size, d1_size, d2_size, param3d);
    } else if (param3d.permute_odr == ODR_210) {
        setup_transpose_internal_210<dtype>(d0_size, d1_size, d2_size, param3d);
    } else if (param3d.permute_odr == ODR_201) {
        // (d0d1)|d2 ==> d2d0d1
        setup_transpose_internal_021<dtype>(1, d0_size * d1_size, d2_size, param3d);
    } 
}

template <typename dtype>
NOINLINE void setup_transpose_3d_params(transpose_3d_params_t &param3d, const uint32_t (&lps)[TRANSPOSE_LP_SIZE]) {
    const uint32_t *__restrict lp = lps;

    // output shape dimension size with padding 
    uint32_t od0_size;
    uint32_t od1_size;
    uint32_t od2_size;
    // output shape dimension size without padding
    uint32_t od0_eff_size;
    uint32_t od1_eff_size;
    uint32_t od2_eff_size;

    //----------------------------------------------
    // ** load dimension settings                 **
    param3d.permute_odr = lp[0];
    // input shape dimension size without padding
    uint32_t d0_eff_size = lp[1];
    uint32_t d1_eff_size = lp[2];
    uint32_t d2_eff_size = lp[3];
    // input shape dimension size with padding
    param3d.d0_size = lp[4];
    param3d.d1_size = lp[5];
    param3d.d2_size = lp[6];
    if (param3d.permute_odr == ODR_021) {
        od0_eff_size = d0_eff_size;
        od1_eff_size = d2_eff_size;
        od2_eff_size = d1_eff_size;
        od0_size = param3d.d0_size;
        od1_size = param3d.d2_size;
        od2_size = param3d.d1_size;
    } else if (param3d.permute_odr == ODR_120) {
        od0_eff_size = d1_eff_size;
        od1_eff_size = d2_eff_size;
        od2_eff_size = d0_eff_size;
        od0_size = param3d.d1_size;
        od1_size = param3d.d2_size;
        od2_size = param3d.d0_size;
    } else if (param3d.permute_odr == ODR_102) {
        od0_eff_size = d1_eff_size;
        od1_eff_size = d0_eff_size;
        od2_eff_size = d2_eff_size;
        od0_size = param3d.d1_size;
        od1_size = param3d.d0_size;
        od2_size = param3d.d2_size;
    } else if (param3d.permute_odr == ODR_210) {
        od0_eff_size = d2_eff_size;
        od1_eff_size = d1_eff_size;
        od2_eff_size = d0_eff_size;
        od0_size = param3d.d2_size;
        od1_size = param3d.d1_size;
        od2_size = param3d.d0_size;
    } else if (param3d.permute_odr == ODR_201) {
        od0_eff_size = d2_eff_size;
        od1_eff_size = d0_eff_size;
        od2_eff_size = d1_eff_size;
        od0_size = param3d.d2_size;
        od1_size = param3d.d0_size;
        od2_size = param3d.d1_size;
    }

    //----------------------------------------------
    // ** setup ITR3D parameters for 3D transpose **
    setup_transpose_internal<dtype>(param3d);

    //----------------------------------------------
    // ** setup parameters for padding trimming   **
    // We do not trim the inner most dimension of the output tensor for the following two reasons:
    //   1. the following on operators still need the inner most dimension be padded,
    //   2. it is too expensive to trim if the address is not aligned.
    // For the outermost dimension, it is not necessary to trim paddings as they are already at the end.  
    // So we actually only need to care about the middle dimension.
    if (od1_eff_size == od1_size) {
        // No paddings
        param3d.trim_loops = 0;
    } else {
        // The innermost dimension has multiple of 16 elements, so we use vector size 16.
        param3d.trim_loops = (od0_eff_size * od1_eff_size * od2_size) >> 4;  // (x / 16)
        // For piplining, we use 8 x vector<dtype, 16>
        param3d.trim_loops = ((param3d.trim_loops >> 3) + ((param3d.trim_loops & 0x07) != 0)); // ceil(trim_loops / 8)
        param3d.trim_iters_in_incr0 = 16 * sizeof(dtype);
        param3d.trim_iters_in_incr1 = (16 + od2_size * (od1_size - od1_eff_size)) * sizeof(dtype);
        param3d.trim_iters_in_wrap0 = ((od1_eff_size * od2_size) >> 4) - 1;
        param3d.trim_iters_out_incr0 = 16 * sizeof(dtype);
        param3d.trim_iters_out_incr1 = 16 * sizeof(dtype);
        param3d.trim_iters_out_wrap0 = param3d.trim_iters_in_wrap0;
    }
} 
#endif // __TRANSPOSE_PARAMS_H__
