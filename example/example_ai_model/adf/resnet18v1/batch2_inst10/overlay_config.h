// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
/*  (c) Copyright 2014 - 2022 Xilinx, Inc. All rights reserved.
   
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

#ifndef _OVERLAY_CONFIG_H_
#define _OVERLAY_CONFIG_H_

// mllib_const.h required for "act_t" definition
#include "mllib_const.h"

// AIE Engine logical array shape NUM_ROWS X NUM_COLS array
#define NUM_COLS        (7)  
#define NUM_ROWS        (4) 
#define NUM_IFM_SHIM_CH (1)
#define NUM_WTS_SHIM_CH (4)

#define LAYER_PARAM_SIZE  64 // Bytes(4n)
#define IFM_DEPTH_GRAN 8

#define IFM_WIN_SIZE_MAX  (8*1024)                            // 8kB- max
#define WTS_WIN_SIZE_MAX  (16*1024-IFM_WIN_SIZE_MAX)           // 8kB+ max
#define OFM_WIN_SIZE_MAX  (14*1024+256)                        // 14kB+
#define IFM_BUF_MAX       (512*1024)        // 512kB
#define OFM_BUF_MAX       (512*1024)        // 512kB
#define WTS_BUF_MAX       (512*1024)        // 512kB

#endif //_OVERLAY_CONFIG_H_

