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

#ifndef __GLOBALAVGPOOL2D_OPT_PARAMS_H__
#define __GLOBALAVGPOOL2D_OPT_PARAMS_H__

#include "utils.h"
#include "mllib_const.h" // required for casc mode


struct alignas(v32uint16) globalavgpool2d_opt_internal_params {
    uint32_t ifm_x_eff;
    uint32_t div_factor;
    uint32_t div_shift;
    uint32_t nifms;
    casc_mode_t casc_mode; // new casc mode RTP, this is updated in wrapper after setup based on column

    uint32_t step_Xi;
    uint32_t step_Yi;
    uint32_t step_Ci;
    uint32_t step_Co;

    int64_t  offset;
    uint32_t inner_loop_count;
    uint32_t outer_loop_count;

    DECL_ITR2D(iterator_inner)
};

typedef struct globalavgpool2d_opt_internal_params globalavgpool2d_opt_params_t;

template <>
struct glbavg_traits<int16> {
    typedef acc64 acc_type;
    typedef uint16 div_type;
    typedef globalavgpool2d_opt_params_t param_type;
};

template <>
struct glbavg_traits<uint16> {
    typedef acc64 acc_type;
    typedef uint16 div_type;
    typedef globalavgpool2d_opt_params_t param_type;
};


static NOINLINE void setup_globalavgpool2d_params(uint8_t* params_ptr,
                                           globalavgpool2d_opt_params_t &param,
                                           uint8_t batch_size) {
    param.offset = *((int64_t*)params_ptr); //-50080;
    
    uint16 offsetLL = *((uint16_t*)params_ptr);
    params_ptr+= 2;
    uint16 offsetLH = *((uint16_t*)params_ptr);
    params_ptr+= 2;
    uint16 offsetHL = *((uint16_t*)params_ptr);
    params_ptr+= 2;
    uint16 offsetHH = *((uint16_t*)params_ptr);
    params_ptr+= 2;

    param.offset = offsetLL + ((uint64_t)offsetLH << 16) + ((uint64_t)offsetHL << 32) + ((uint64_t)offsetHH << 48);

    uint32_t ifm_xsize =  (uint32_t)*params_ptr++; // ifm_sv_width
    param.ifm_x_eff = (uint32_t)*params_ptr++; // ifm_sv_width_eff

    uint32_t ifm_ysize = (uint32_t)*params_ptr++; // ifm_sv_height
    param.nifms = (uint32_t)*params_ptr++; // ifm_sv_depth
    param.div_shift = (uint32_t)*params_ptr++; // div_shift
    params_ptr++;
    
    uint16 div_factorL = *((int16_t*)params_ptr);
    params_ptr +=2;
    uint16 div_factorH = *((int16_t*)params_ptr);
    params_ptr +=2;
    param.div_factor = (uint32_t)div_factorL +((uint32_t)div_factorH << 16); // gap_div_factor

    uint32_t N_g = batch_size * 8;
    param.step_Xi = N_g;
    param.step_Yi = (-param.step_Xi * (param.ifm_x_eff - 1)) + (ifm_xsize * param.nifms * batch_size);
    param.step_Ci = ifm_xsize * N_g * sizeof(bfloat16); // Go to the next depth-8
    param.step_Co = N_g * sizeof(bfloat16);

    param.inner_loop_count = param.ifm_x_eff * ifm_ysize;
    param.outer_loop_count = (param.nifms >> 5);

    SETUP_ITR2D_PARAM(iterator_inner, (param.step_Xi * sizeof(bfloat16)), (param.ifm_x_eff - 1),
                      (param.step_Yi * sizeof(bfloat16)), param)
}
#endif // __GLOBALAVGPOOL2D_PARAMS_H__
