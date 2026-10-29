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


#ifndef __ML_PARAMS_H__
#define __ML_PARAMS_H__

#include <stdint.h>
#include <adf.h>

enum KernelConfig {
    KC_ZERO,
    KC_RESULT4,
    KC_RESULT8,
    KC_RESULT16,
    KC_RESULT32,
    KC_RESULT64,
    KC_CASC,
    KC_TDM16,
    KC_TDM32,
    KC_TDM64,
    KC_TDM16_CASC,
    KC_TDM32_CASC,
    KC_TDM64_CASC,
    KC_RESULT32_CASC,
    KC_CASC2,
    KC_TDM16_CASC2,
    KC_TDM32_CASC2,
};


#ifndef KIR
#define KIR
#endif

union MLKernelControl {
uint32_t control;
struct {
    uint32_t zero_init:1;
    uint32_t sign_N:1;
    uint32_t sign_O:1;
    uint32_t reserved3:3;
    uint32_t skip_casc_in:1;
    uint32_t skip_casc_out:1;
    uint32_t sign_W:1;
    uint32_t sign_A:1;
    uint32_t out_32:1;
    uint32_t add_bias:1;
    uint32_t reserved10:12;
    uint32_t norm_ch_g:8;
} parts;
};


struct MLKernelParams {
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
    int8_t shift_norm;
    int8_t shift_bias;

    uint16_t step_Kx;
    uint16_t step_Ky;
    uint16_t step_Ci;
    uint16_t step_Xi;
    uint16_t step_Yi;
    uint16_t step_Xo;
    uint16_t step_Yo;
    uint16_t step_Co;
    int param_value;
    MLKernelControl ctrl;

    MLKernelParams()
    {
        Kx_g = 0;
        Ky_g = 0;
        Ci_g = 0;
        S_g = 0;
        N_g = 16;
        X_g = 4;
        Y_g = 8;
        Co_g = 0;
        inner_g = 16;
        outer_g = 32;
        shift_tdm = 12;
        shift_res = 12;
        shift_norm = 0;
        shift_bias = 0;
        step_Kx = 1024;
        step_Ky = 8;
        step_Ci = 0;
        step_Xi = 512;
        step_Yi = 8;
        step_Xo = 512;
        step_Yo = 8;
        step_Co = 0;
        param_value = 0;
        ctrl.control = 773;
    }

    // Use the following only for mha gemms (32, 96, 64), (32, 64, 64), (32, 96, 16)
    MLKernelParams(int core_m, int core_k, int core_n, int transpose, uint8_t signA=1, uint8_t signW=1)
    {
        Kx_g = 0;
        Ky_g = 0;
        Ci_g = 0;
        S_g = 0;         
        X_g = 4; 
        Co_g = 0; 

        shift_tdm = 8; 
        shift_res = 8; 
        shift_norm = 0; 
        shift_bias = 0; 
        step_Ci = 0;
        step_Co = 0;

        // The folloing params are used to derive incrs jumps 
        outer_g = (core_m >> 3) * (core_n >> 4);   // (M/8)*(N/16)
        inner_g = (core_k >> 3);                   // (K/8)
        N_g     = inner_g;
        Y_g     = (core_m >> 3);                   // (M/8)

        step_Xi = (core_m << 3);                   // (M*8)
        step_Kx = (transpose == 0) ? (core_k << 3) : 8; // transpose==0:(K*8) transpose==1:8 
        step_Xo = (core_m << 3);                   // (M*8)
        
        step_Yi = 8;
        step_Ky = (transpose == 0) ? 8 : (core_n << 3); // transpose==0:8 transpose==1:(N*8)
        step_Yo = 8;

        S_g = transpose;
        
        param_value = 0;
        ctrl.control = 773;
        ctrl.parts.sign_A = signA;
        ctrl.parts.sign_W = signW;
    }

    void update_params(int Y, int N, int X, int Ygran, int Ngran, int Xgran)
    {
        // compute derived params
        Y_g     = Y / Ygran;
        N_g     = N / Ngran;
        X_g     = X / Xgran;
        inner_g = N_g;
        outer_g = Y_g * X_g;
        step_Xi = Y * 8;
        step_Kx = N * 8;
        step_Xo = Y * 8;
    }

    void const print_members() const
    {
    #ifdef _DEBUG_
        printf("Kx_g =%d \n",Kx_g);
        printf("Ky_g =%d \n",Ky_g);
        printf("Ci_g =%d \n",Ci_g);
        printf("S_g  =%d \n",S_g);
        printf("N_g  =%d \n",N_g);
        printf("X_g  =%d \n",X_g);
        printf("Y_g  =%d \n",Y_g);
        printf("Co_g =%d \n",Co_g);
        printf("inner_g     =%d \n",inner_g);
        printf("outer_g     =%d \n",outer_g);
        printf("shift_tdm   =%d \n",shift_tdm);
        printf("shift_res   =%d \n",shift_res);
        printf("shift_norm  =%d \n",shift_norm);
        printf("shift_bias  =%d \n",shift_bias);
        printf("step_Kx     =%d \n",step_Kx);
        printf("step_Ci     =%d \n",step_Ci);
        printf("step_Xi     =%d \n",step_Xi);
        printf("step_Yi     =%d \n",step_Yi);
        printf("step_Xo     =%d \n",step_Xo);
        printf("step_Yo     =%d \n",step_Yo);
        printf("step_Co     =%d \n",step_Co);
        printf("param_value =%d \n",param_value);
        printf("ctrl.zero_init =%d \n",ctrl.parts.zero_init);
        printf("ctrl.sign_N =%d \n",ctrl.parts.sign_N);
        printf("ctrl.sign_O =%d \n",ctrl.parts.sign_O);
        printf("ctrl.skip_casc_in =%d \n",ctrl.parts.skip_casc_in);
        printf("ctrl.skip_casc_out =%d \n",ctrl.parts.skip_casc_out);
        printf("ctrl.sign_W =%d \n",ctrl.parts.sign_W);
        printf("ctrl.sign_A =%d \n",ctrl.parts.sign_A);
        printf("ctrl.norm_ch_g =%d \n",ctrl.parts.norm_ch_g);
    #endif
    }
};

struct MLKernelParams_int16_32x128x64 {
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
    int8_t shift_norm;
    int8_t shift_bias;

    uint16_t step_Kx;
    uint16_t step_Ky;
    uint16_t step_Ci;
    uint16_t step_Xi;
    uint16_t step_Yi;
    uint16_t step_Xo;
    uint16_t step_Yo;
    uint16_t step_Co;
    int param_value;
    MLKernelControl ctrl;

    MLKernelParams_int16_32x128x64()
    {
        Kx_g = 0;
        Ky_g = 0;
        Ci_g = 0;
        S_g = 0;
        N_g = 16;
        X_g = 4;
        Y_g = 8;
        Co_g = 0;
        inner_g = 16;
        outer_g = 32;
        shift_tdm = 12;
        shift_res = 12;
        shift_norm = 0;
        shift_bias = 0;
        step_Kx = 1024;
        step_Ky = 8;
        step_Ci = 0;
        step_Xi = 256;
        step_Yi = 4;
        step_Xo = 256;
        step_Yo = 4;
        step_Co = 0;
        param_value = 0;
        ctrl.control = 773;
    }

    MLKernelParams_int16_32x128x64(int core_m, int core_k, int core_n, int transpose, uint8_t signA=1, uint8_t signW=1)
    {
        Kx_g = 0;
        Ky_g = 0;
        Ci_g = 0;
        S_g = 0;         
        X_g = 4; 
        Co_g = 0; 

        shift_tdm = 8; 
        shift_res = 8; 
        shift_norm = 0; 
        shift_bias = 0; 
        step_Ci = 0;
        step_Co = 0;

        // The folloing params are used to derive incrs jumps 
        outer_g = (core_m >> 3) * (core_n >> 4);   // (M/8)*(N/16)
        inner_g = (core_k >> 3);                   // (K/8)
        N_g     = inner_g;
        Y_g     = (core_m >> 3);                   // (M/8)

        step_Xi = (core_m << 3);                   // (M*8)
        step_Kx = (transpose == 0) ? (core_k << 3) : 8; // transpose==0:(K*8) transpose==1:8 
        step_Xo = (core_m << 3);                   // (M*8)
        
        step_Yi = 8;
        step_Ky = (transpose == 0) ? 8 : (core_n << 3); // transpose==0:8 transpose==1:(N*8)
        step_Yo = 8;

        S_g = transpose;
        
        param_value = 0;
        ctrl.control = 773;
        ctrl.parts.sign_A = signA;
        ctrl.parts.sign_W = signW;
    }
};


struct MLLayerParams {
    MLKernelParams kernel;
    uint8_t iter_cnt;
    uint8_t tdm_cnt;    //0=no tdm (1 iter); 1=accumulate once (2 iters), ...
    uint8_t keep_cnt;   //0=use one time; 1=use two time, ...
    uint8_t keep_data;  //1=keep data; 0=keep weights
    uint8_t casc_setup; //0=no cascade; 1=start; 2=middle; 3=end of cascade
    uint8_t kernel_family;
    uint8_t reserved1;
    uint8_t reserved2;
    int offset_actv;
    int offset_wght;
    int offset_out;
    int offset_interm;
};


struct MLIncrements {
    int incKx;
    int incKy;
    int incCi;

    int incCo_rev;
    int incXi;
    int incYi;

    int incA_0;
    int incA_1;

    int incCo;
    int incXo;
    int incYo;

    int incCo_T;
    int incXo_T;

    int incS_0;
    int incS_1;
    int incS_2;

    int incA_2;
    int incA_3;

    int step_align;
    int shft_0;
    int shft_1;
    int shfl_0;
    int shfl_1;
    int shfl_2;
    int shfl_3;
    int incW_Ci_rev;
    int incW_Co_rev;

    uint8_t numKx;
    uint8_t numKy;
    uint8_t numCo;
    uint8_t numX;
    uint8_t numW;
    uint8_t numBN;
};

#endif //__ML_PARAMS_H__
