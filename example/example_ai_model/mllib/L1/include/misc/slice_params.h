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

#ifndef __SLICE_PARAMS_H__
#define __SLICE_PARAMS_H__
#include "utils.h"

// Buffer placement directives for IFM, WTs and OFM buffers.
#ifndef IFM_DM_BANK
#define IFM_DM_BANK __aie_dm_resource_a
#endif
#ifndef WT_DM_BANK
#define WT_DM_BANK __aie_dm_resource_a
#endif
#ifndef OFM_DM_BANK
#define OFM_DM_BANK __aie_dm_resource_b
#endif

#ifndef SLICE_VEC_LEN
#if __AIE_ARCH__ == 20 // AIE2
#define SLICE_VEC_LEN 32
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
#define SLICE_VEC_LEN 64
#endif // AIE_ARCH
#endif // SLICE_VEC_LEN


struct alignas(v8int16) slice_params_t {
    // input shape dimension size without padding HCW
    uint32_t d0_size;
    uint32_t d1_size;
    uint32_t d2_size;

    uint32_t h_st;
    uint32_t c_st;
    uint32_t w_st;

    uint32_t h_ed;
    uint32_t c_ed;
    uint32_t w_ed;

    uint32_t h_sp;
    uint32_t c_sp;
    uint32_t w_sp;

    uint32_t out_num;
    uint32_t iter_num;
    uint32_t offset;
    DECL_ITR3D(iters_in)
};



// Run Time Param Loading

static NOINLINE void load_slice_rtp(slice_params_t &params, const uint32_t (&lp)[16]){
    params.d0_size = lp[0];
    params.d1_size = lp[1];
    params.d2_size = lp[2];
    params.h_st = lp[3];
    params.c_st = lp[4];
    params.w_st = lp[5];
    params.h_ed = lp[6];
    params.c_ed = lp[7];
    params.w_ed = lp[8];
    params.h_sp = lp[9];
    params.c_sp = lp[10];
    params.w_sp = lp[11];
}


static NOINLINE void setup_slice(slice_params_t &params){
    params.out_num = (int((params.h_ed - params.h_st - 1) / params.h_sp) + 1) * (int((params.c_ed - params.c_st - 1) / params.c_sp) + 1) *
                 (int((params.w_ed - params.w_st - 1) / params.w_sp) + 1);
    params.iter_num = params.out_num >> 5;
    params.iter_num = (params.iter_num << 5) == params.out_num ? params.iter_num : params.iter_num + 1;
    params.offset = params.h_st * params.d1_size * params.d2_size + params.c_st * params.d2_size + params.w_st;
}



static NOINLINE void setup_slice_params(slice_params_t &params, const uint32_t (&lp)[16]){
    load_slice_rtp(params, lp);
    setup_slice(params);
}

#endif  //__SLICE_PARAMS_H__