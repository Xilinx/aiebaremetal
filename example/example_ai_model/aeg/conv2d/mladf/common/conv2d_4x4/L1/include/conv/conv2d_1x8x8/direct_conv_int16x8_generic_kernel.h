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

#ifndef __DIRECT_CONV_INT16X8_GENERIC_KERNEL_H__
#define __DIRECT_CONV_INT16X8_GENERIC_KERNEL_H__

#include "direct_conv_common.h"

#ifndef IN_MODE
#define IN_MODE KC_ZERO
#endif
#ifndef OUT_MODE
#define OUT_MODE KC_TDM32
#endif

enum OPMode {
    OP_NONE,
    OP_CONV,
    OP_SUM,
    OP_SUM_T,
    OP_DWC,
    OP_DWC_SUM,
    OP_CONV_SYM = 8,
    OP_CONV_ASYM,
    OP_DWC_SYM,
    OP_DWC_ASYM
};
    

struct KernelControl KIR {
    uint32_t zero_init:1;
    uint32_t sign_N:1;
    uint32_t sign_O:1;
    uint32_t reserved3:3;
    uint32_t skip_casc_in:1;
    uint32_t skip_casc_out:1;
    uint32_t sign_W:1;
    uint32_t sign_A:1;
    uint32_t reserved10:14;
    uint32_t norm_ch_g:8;
};


struct KernelParams {
    uint8_t Kx_g;
    uint8_t Ky_g;
    uint8_t Ci_g;
    int8_t  S_g;
    uint8_t N_g;
    uint8_t X_g;
    uint8_t Y_g;
    uint8_t Co_g;
    uint16_t inner_g;
    uint16_t outer_g;
    int8_t shift_tdm;
    int8_t shift_res;
    int8_t zp_wght;
    int8_t op_mode;

    uint16_t step_Kx;
    uint16_t step_Ky;
    uint16_t step_Ci;
    uint16_t step_Xi;
    uint16_t step_Yi;
    uint16_t step_Xo;
    uint16_t step_Yo;
    uint16_t step_Co;
    int param_value;
    KernelControl ctrl;
    KernelParams () {
        // 32 x 128 x 64 subvol
        Kx_g = 1;
        Ky_g = 1;
        Ci_g = 16;
        S_g  = 1;
        N_g  = 1;
        X_g  = 4;
        Y_g  = 1;
        Co_g = 8;
        inner_g = 16; // (128 / 8)
        outer_g = 32; // ( (32/8) * (64/8) )
        // Shift for GEMM output
        shift_tdm = 0;
        // SHift for QDQ output
        shift_res = 0;
        zp_wght = 0;
        op_mode = OP_CONV;

        step_Kx = 16;
        step_Ky = 8192;
        step_Ci = 512;
        step_Xi = 16;
        step_Yi = 8192;
        step_Xo = 16;
        step_Yo = 4096;
        step_Co = 512;
    }
};

struct DirectConvParams {
    KernelParams kernel;
    uint16_t iter_cnt;
    uint8_t tdm_cnt;    //0=no tdm (1 iter); 1=accumulate once (2 iters), ...
    uint8_t keep_cnt;   //0=use one time; 1=use two time, ...
    uint8_t keep_data;  //1=keep data; 0=keep weights
    uint8_t casc_setup; //0=no cascade; 1=start; 2=middle; 3=end of cascade
    uint8_t kernel_family;
    uint8_t reserved1;
    int offset_actv;
    int offset_wght;
    int offset_out;
    int offset_interm;
};

struct Increments {
    int incA_0;
    int incA_1;

    int incS_0;
    int incT_0;
    int incT_1;

    int step_align;
    int shft_0;
    int shft_1;
    int shfl_0;
    int shfl_1;
    int shfl_2;
    int shfl_3;
    
    dims_3d_t dimsAI;
    dims_3d_t dimsAO;
    dims_3d_t dimsW;
    dims_3d_t dimsO;
};


#ifndef INLINE
#ifndef INLINE_DECL
#define INLINE_DECL
#endif


static void direct_conv_int16x8_generic
(
        int * input,
        int * weights,
        int * tdm1,
        int * tdm2,
        int * restrict output,
        const KernelParams &params
);


#else //!INLINE
#  ifndef INLINE_DECL
#   ifndef __llvm__
#     define INLINE_DECL inline
#   else
#     define INLINE_DECL inline __attribute__((always_inline))
#   endif
#  endif
#  undef INLINE
#  include "direct_conv_int16x8_generic_kernel.c"
#  define INLINE
# endif //!INLINE


#endif //__DIRECT_CONV_INT16X8_GENERIC_KERNEL_H__
