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

#ifndef __DIRECT_CONV_INT16X8_GENERIC_VAR_SIGN_PARAMS_H__
#define __DIRECT_CONV_INT16X8_GENERIC_VAR_SIGN_PARAMS_H__

#include "direct_conv_common.h"
#include "assert.h"

inline void assert_msg( bool tst, const char * msg ) {
    #ifdef FILE_IO
    if (!tst) printf( msg );
    #endif
    assert( tst );
}


ALWAYS_INLINE void setup_parameters_dc_int16x8_generic( const KernelParams &param, Increments &incrs, int delta_tdm=0x8000 ) {
    assert_msg( param.N_g == 1, "needs update" );
    int SN0_g = param.S_g == 1;
    int SN1_g = 4 << SN0_g;
    int reset;

    incrs.step_align = 28 - clb(param.step_Kx ^ (param.step_Kx-1));  // 8->0, 16->1, 32->2
    incrs.shfl_0 = (SN0_g ? T512_1x2_lo : T128_4x2_lo);//(param.N_g == 1 ? T128_4x2_lo : (param.N_g == 2 ? T256_2x2_lo : T512_1x2_lo)));
    incrs.shfl_1 = (SN0_g ? T512_1x2_hi : T512_1x2_lo);//(SN0_g ? T512_1x2_hi : (param.N_g == 1 ? T128_4x2_lo : (param.N_g == 2 ? T256_2x2_lo : T512_1x2_lo)));

    incrs.incA_0 = 128;//param.step_Xi * SN1_g;
    reset        = -incrs.incA_0;
    int incKx = add_dimension( reset, param.Kx_g-1, param.step_Kx );
    int incKy = add_dimension( reset, param.Ky_g-1, param.step_Ky );
    int incCi = reset + param.step_Ci;
    incrs.dimsAI = dims_3d_t( param.Kx_g-1, incKx, param.Ky_g-1, incKy, incCi );

    reset = -param.Ci_g * param.step_Ci;
    int incCo_rev = add_dimension( reset, param.Co_g-1, param.op_mode == OP_DWC || param.op_mode == OP_DWC_SUM ? param.step_Ci : 0 );
    int incXi     = add_dimension( reset, param.X_g-1, incrs.incA_0 );
    int incYi     = reset + param.step_Yi;
    incrs.dimsAO = dims_3d_t( param.Co_g-1, incCo_rev, param.X_g-1, incXi, incYi );

    int incS_0_base = param.step_Xo * 4;
    incrs.incS_0 =  incS_0_base * chess_copy(SN0_g);
    reset = -incrs.incS_0;
    int incCo   = add_dimension( reset, param.Co_g-1, param.step_Co );
    int incXo   = add_dimension( reset, param.X_g-1, incrs.incS_0 + incS_0_base );
    int incYo   = reset + param.step_Yo;
    incrs.dimsO = dims_3d_t( param.Co_g-1, incCo, param.X_g-1, incXo, incYo );
    incrs.incT_0 = SN0_g ? 128 : delta_tdm;
    incrs.incT_1 = SN0_g ? 128 : 128 - delta_tdm;
    
    incrs.dimsW = dims_3d_t( 0, 0, param.inner_g * param.Co_g - 1, 64, -64 * ( param.inner_g * param.Co_g - 1 ));
}

#endif // __DIRECT_CONV_INT16X8_GENERIC_VAR_SIGN_PARAMS_H__
