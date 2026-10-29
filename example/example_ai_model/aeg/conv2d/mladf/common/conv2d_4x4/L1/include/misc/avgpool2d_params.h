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

#ifndef __AVGPOOL2D_PARAMS_H__
#define __AVGPOOL2D_PARAMS_H__

#include "utils.h"

#define STEP_X_I 16 //Move 16 units in X dimension (8px for batch-of-2 and 4px for batch-of-4)

//TODO: need to check if keep_in_registers is required.

template <typename dtype>
struct alignas(v32uint16) avgpool2d_internal_params{
    uint8_t Kx_g;
    uint8_t Ky_g;
    uint8_t Ci_g;
    uint16_t outer_loop_count;

    DECL_ITR3D(iterator)

    // Changing below to int16
    int16_t inc_Ky;
    int16_t inc_A_0;
    // changing below to int8
    int8_t shft_0;
    int8_t shft_1;
    int8_t shft_2;
    int8_t shft_3;
    int8_t shft_4;
    int8_t shft_5;
    int8_t shft_6;
    int8_t shft_7;
    int8_t shfl_0; //Check
    int8_t shfl_1; //Check

    uint16_t ofm_len ;
    uint8_t nifms;

    using DivFactorType = typename std::conditional<std::is_same<dtype, bfloat16>::value, bfloat16, uint16_t>::type;
    DivFactorType div_factor; //The fixed point representation of 1/4 or 1/9 based on 2x2 or 3x3 filter.
    uint8_t  div_shift;
};
template <typename dtype>
struct avgpool2d_traits;
template <>
struct avgpool2d_traits<uint8> {
    typedef uint16 input_container;
};
template <>
struct avgpool2d_traits<int8> {
    typedef int16 input_container;
};

template <typename dtype>
void setup_avgpool2d_params(dtype* params_ptr, avgpool2d_internal_params<dtype> &param, uint8_t batch_size)
{
    uint8_t* params_ptr_ = (uint8_t *)params_ptr;
    uint8_t ifm_xsize           = (uint8_t) *params_ptr_++;//0
    uint8_t ifm_ysize           = (uint8_t) *params_ptr_++;//1
    param.nifms                 = (uint8_t) *params_ptr_++;//2
                                             params_ptr_++;//3  No need for OFMs so just increment pointer
    uint8_t ksize               = (uint8_t) *params_ptr_++;//4
    uint8_t stride_bits         = (uint8_t) *params_ptr_++;//5
    #ifdef AVGPOOL_NEW_RTP

    uint8_t div_factor_hi;
    uint8_t div_factor_lo;

    params_ptr_ += 2;
    param.div_shift             = (uint8_t) *params_ptr_;
    params_ptr_ += 7;

    div_factor_lo               = (uint8_t) *params_ptr_++;
    div_factor_hi               = (uint8_t) *params_ptr_++;
    param.div_factor            = ((uint16_t) div_factor_hi << 8) +  (uint16_t) div_factor_lo;

    uint8_t manual_ba_pad_val   = (uint8_t) *params_ptr_++; //17
    params_ptr_ += 3; // OFM length is at offset 20
    param.ofm_len               = *( (uint16_t *) params_ptr_);

    #else

    params_ptr_ += 11; // 6-16
    uint8_t manual_ba_pad_val   = (uint8_t) *params_ptr_++; //17
    params_ptr_ += 3; // OFM length is at offset 20
    param.ofm_len               = *( (uint16_t *) params_ptr_);
    params_ptr_ += 5;
    using DivFactorType = typename std::conditional<std::is_same<dtype, bfloat16>::value, bfloat16, uint16_t>::type;
    param.div_factor            = *((DivFactorType *) params_ptr_); // div_factor is at offset 58
    params_ptr_ += 2;
    param.div_shift             = (uint8_t) *params_ptr_; // div_shift is at offset 60

    #endif

    //Variables required to setup the 3D iterator
    uint8_t num_X;
    uint8_t num_Co;
    int16_t inc_Xi;
    int16_t inc_Ci;
    int16_t inc_Yi;

    uint8_t stride = 1 << stride_bits;
    uint8_t X_g;
    uint8_t Co_g = param.nifms >> 3;

    param.Kx_g = ksize;
    param.Ky_g = ksize;

    uint8_t batch_log2 = (batch_size == 4) ? 2 : ((batch_size == 2) ? 1 : 0);

    int8_t ks_diff = ksize - stride;
    #if __AIE_ARCH__ == 21
        uint8_t boundary_align;
        if constexpr(std::is_same<bfloat16, dtype>::value)
        {
            boundary_align = (manual_ba_pad_val != 0) ? manual_ba_pad_val : ((batch_size == 4) ? 0 : (ks_diff != 0 ? ((4 >> batch_log2) - ks_diff) : 0));
        }
        else
        {
    #ifndef AVGPOOL2D_3X3S1N1
            boundary_align = (manual_ba_pad_val != 0) ? manual_ba_pad_val : (ks_diff != 0 ? ((8 >> batch_log2) - ks_diff) : 0);
    #else
            boundary_align = (manual_ba_pad_val != 0) ? manual_ba_pad_val : ((batch_size == 4) ? 0 : (ks_diff != 0 ? ((4 >> batch_log2) - ks_diff) : 0));
    #endif//AVGPOOL2D_3X3S1N1
        }
    #else // #if __AIE_ARCH__ == 21
        uint8_t boundary_align = (manual_ba_pad_val != 0) ? manual_ba_pad_val : ((batch_size == 4) ? 0 : (ks_diff != 0 ? ((4 >> batch_log2) - ks_diff) : 0));
    #endif // #if __AIE_ARCH__ == 21
    unsigned int ifm_xsize_padded = ifm_xsize + boundary_align;

    uint16_t step_Ky = ifm_xsize_padded * param.nifms * batch_size * sizeof(dtype);
    //Based on ifm width format, we have to deduce X_g
    uint8_t factor = ((8 * stride) >> batch_log2);
    uint8_t factor_log2 = (factor == 16) ? 4 : ((factor == 8) ? 3 : ((factor == 4) ? 2 : 1));

    X_g = (uint8_t)((ifm_xsize - ks_diff) >> factor_log2);  // Number of iteration long x dimension

    uint16_t step_Xi = stride > 1 ? STEP_X_I : (STEP_X_I >> 1);
    uint16_t step_Ci = (ifm_xsize_padded * batch_size) << 3;
    uint16_t step_Yi = (ifm_xsize_padded * batch_size) * param.nifms * stride;

    param.outer_loop_count = X_g * Co_g * ((uint8_t)((ifm_ysize - param.Kx_g) >> stride_bits) + 1);

    if constexpr(std::is_same<bfloat16, dtype>::value) {
        param.shfl_0 = (stride == 2 ? (batch_size == 1 ? T128_4x2_lo : T256_2x2_lo) : T512_1x2_lo);
        param.shfl_1 = (stride == 2 ? (batch_size == 1 ? T128_4x2_hi : T256_2x2_hi) : T512_1x2_hi);

        param.shft_0 = 0;
        param.shft_1 = 8 * batch_size * (stride == 1 ? 1 : 4);
        param.shft_2 = 8 * batch_size * (stride == 1 ? 2 * (param.Kx_g > 2) : 0);
        param.shft_3 = 8 * batch_size * (stride == 1 ? 2 * (param.Kx_g > 2) : 4);
        param.shft_4 = 8 * batch_size * (stride == 1 ? 0 : 4);
        param.shft_5 = 8 * batch_size * (stride == 1 ? 0 : 4);
        param.shft_6 = 8 * batch_size * (stride == 1 ? 0 : (param.Kx_g > 2 ? 1 : 0));
        param.shft_7 = 8 * batch_size * (stride == 1 ? 0 : (param.Kx_g > 2 ? 1 : 0));
    } else {
        param.shfl_0 = (stride == 2 ? (batch_size == 1 ? T64_8x2_lo : (batch_size == 2 ? T128_4x2_lo : T256_2x2_lo)) : T512_1x2_lo);
        param.shfl_1 = (stride == 2 ? (batch_size == 1 ? T64_8x2_hi : (batch_size == 2 ? T128_4x2_hi : T256_2x2_hi)) : T512_1x2_lo);

        param.shft_0 = 8 * batch_size * (stride == 1);
        param.shft_1 = 8 * batch_size * (param.Kx_g > 2) * (stride == 2);
        param.shft_2 = 2 * 8 * batch_size * (param.Kx_g > 2) * (stride == 1);
    }
    num_X = X_g - 1;
    num_Co = Co_g - 1;
    
    param.inc_Ky = -128 * sizeof(dtype) + step_Ky;

    param.inc_A_0 = chess_copy(param.Ky_g > 2 ? param.inc_Ky : -128 * sizeof(dtype));

    inc_Xi = -step_Ky * (param.Ky_g - 1) + (stride > 1 ? 0 : -64 * sizeof(dtype));
    inc_Ci = -step_Ky * (param.Ky_g - 1) + (stride > 1 ? 0 : -64 * sizeof(dtype)) - sizeof(dtype) * 8 * step_Xi * X_g + sizeof(dtype) * step_Ci;
    inc_Yi = -step_Ky * (param.Ky_g - 1) + (stride > 1 ? 0 : -64 * sizeof(dtype)) - sizeof(dtype) * 8 * step_Xi * X_g - sizeof(dtype) * step_Ci * num_Co + sizeof(dtype) * step_Yi;

    SETUP_ITR3D_PARAM(iterator, inc_Xi, num_X, inc_Ci, num_Co, inc_Yi, param)
}

#endif  //__AVGPOOL2D_PARAMS_H__
