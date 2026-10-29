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

#ifndef __AVGPOOL2DVARIANT_PARAMS_H__
#define __AVGPOOL2DVARIANT_PARAMS_H__

#include "adf.h"
#include "aie.hpp"
#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"
#include "utils.h"

#define VEC_LEN_AVGPOOL2DVARIANT 32
//TODO: need to check if keep_in_registers is required.

template <typename dtype>
struct alignas(v32uint16) avgpool2dvariant_internal_params{
    //RTPs
    uint8_t ifm_width;
    uint8_t ifm_height;
    uint8_t ifm_depth;
    int8_t ifm_shift;
    uint8_t ifm_shift_biased;
    int8_t ofm_shift;
    uint8_t ofm_shift_biased;
    uint8_t shift_div_factor;
    uint8_t ksize_h;
    uint8_t ksize_w;
    uint8_t stride_h;
    uint8_t stride_w;
    uint8_t manual_ba_pad_val;
    using DivFactorType = typename std::conditional<std::is_same<dtype, bfloat16>::value, bfloat16, uint16_t>::type;
    DivFactorType div_factor; //The fixed point representation of 1/4 or 1/8 based on 1x4 or 2x4 filter.

    uint16_t outer_loop_count;
    uint16_t inner_loop_count;

    DECL_ITR3D(iterator_global)
    DECL_ITR2D(iterator_local)

    uint8_t num_Margin;
    uint16_t offset_Margin;
    uint16_t inc_Margin;

    vector<dtype, VEC_LEN_AVGPOOL2DVARIANT> div_vector;
    vector<dtype, VEC_LEN_AVGPOOL2DVARIANT> div_vector_margin;


};

template <typename dtype>
__attribute__((noinline)) void setup_avgpool2d_params(const uint32_t* params_ptr, avgpool2dvariant_internal_params<dtype> &param, uint8_t batch_size)
{
    uint8_t* params_ptr_        = (uint8_t *)params_ptr;
    param.ifm_width           = (uint8_t) *params_ptr_++;//0
    param.ifm_height           = (uint8_t) *params_ptr_++;//1
    param.ifm_depth               = (uint8_t) *params_ptr_++;//2
                                             params_ptr_++;//3  No need for OFMs so just increment pointer
    params_ptr_++;//4
    param.ifm_shift             = (uint8_t) *params_ptr_++;//5
    param.ifm_shift_biased      = (uint8_t) *params_ptr_++;//6
    param.ofm_shift             = (uint8_t) *params_ptr_++;//5
    param.ofm_shift_biased      = (uint8_t) *params_ptr_++;//8
    param.shift_div_factor      = (uint8_t) *params_ptr_++;//9
    param.ksize_h             = (uint8_t) *params_ptr_++;//10
    param.ksize_w             = (uint8_t) *params_ptr_++;//11
    param.stride_h            = (uint8_t) *params_ptr_++;//12
    param.stride_w            = (uint8_t) *params_ptr_++;//13
    params_ptr_ += 3; // 14-16
    param.manual_ba_pad_val   = (uint8_t) *params_ptr_++; //17
    params_ptr_ += 8; // OFM length is at offset 25
    using DivFactorType = typename std::conditional<std::is_same<dtype, bfloat16>::value, bfloat16, uint16_t>::type;
    param.div_factor            = *((DivFactorType *) params_ptr_); // div_factor is at offset 58

    int elem_per_pixel = 8 * batch_size; //depth_gran * batch_size
    int super_pixel_len = VEC_LEN_AVGPOOL2DVARIANT / elem_per_pixel;
    const int width_align = 32;
    uint8_t num_X;
    uint8_t num_Co;
    int16_t inc_Xi;
    int16_t inc_Ci;
    int16_t inc_Yi;
    uint8_t X_g;
    uint8_t Co_g = param.ifm_depth >> 3;
    int16_t inc_Xl;
    int8_t num_Xl;
    int16_t inc_Yl;    
    uint8_t Y_g;

    uint8_t boundary_align = (param.manual_ba_pad_val != 0) ? param.manual_ba_pad_val : (int((param.ifm_width * elem_per_pixel + width_align - 1) / width_align) * width_align - param.ifm_width * elem_per_pixel) / elem_per_pixel;
    unsigned int ifm_xsize_padded = param.ifm_width + boundary_align;
    uint16_t step_Xi = param.stride_w;
    uint16_t step_Ci = (ifm_xsize_padded) << 3;
    uint16_t step_Yi = (ifm_xsize_padded) * param.ifm_depth;

    int zero_margin = param.ksize_w % super_pixel_len;
    aie::mask<VEC_LEN_AVGPOOL2DVARIANT> mo = aie::mask<32>::from_uint32(-1 << (8 * zero_margin));
    if constexpr(std::is_same<bfloat16, dtype>::value) {
        param.div_vector = broadcast<dtype, VEC_LEN_AVGPOOL2DVARIANT>((dtype)param.div_factor);
    }
    else {
        param.div_vector = broadcast<dtype, VEC_LEN_AVGPOOL2DVARIANT>((dtype)1);
    }
 
    vector<dtype, VEC_LEN_AVGPOOL2DVARIANT> zero_vec = broadcast<dtype, VEC_LEN_AVGPOOL2DVARIANT>((dtype)0);
    param.div_vector_margin = select(param.div_vector, zero_vec, mo);
    X_g = (uint8_t)((param.ifm_width - param.ksize_w) / param.stride_w + 1);  // Number of iteration long x dimension
    Y_g = (uint8_t)((param.ifm_height - param.ksize_h) / param.stride_h + 1); // Number of iteration long y dimension
    
    //Variables required to setup the 3D iterator_global
    num_X = X_g - 1;
    num_Co = Co_g - 1;

    inc_Xi = 8 * step_Xi * sizeof(dtype);
    inc_Ci = 8 * step_Xi * sizeof(dtype) - 8 * step_Xi * X_g * sizeof(dtype) + step_Ci * sizeof(dtype);
    inc_Yi = 8 * step_Xi * sizeof(dtype) - 8 * step_Xi * X_g * sizeof(dtype) - step_Ci * num_Co * sizeof(dtype) + step_Yi * param.stride_h * sizeof(dtype);

    inc_Xl = 8 * super_pixel_len * sizeof(dtype);
    num_Xl = int(param.ksize_w / super_pixel_len) - 1;
    inc_Yl = (- inc_Xl * num_Xl + step_Yi * sizeof(dtype)) ;

    param.num_Margin = (param.ksize_w % super_pixel_len) ? param.ksize_h : 0;
    param.offset_Margin = 8 * super_pixel_len *  (num_Xl + 1) * sizeof(dtype);
    param.inc_Margin = step_Yi * sizeof(dtype);

    param.outer_loop_count = param.ksize_h * (num_Xl + 1);
    param.inner_loop_count = X_g * Y_g * Co_g;

    SETUP_ITR3D_PARAM(iterator_global, inc_Xi, num_X, inc_Ci, num_Co, inc_Yi, param)
    SETUP_ITR2D_PARAM(iterator_local, inc_Xl, (uint8_t)num_Xl, inc_Yl, param)
    
}

#endif  //__AVGPOOL2DVARIANT_PARAMS_H__
