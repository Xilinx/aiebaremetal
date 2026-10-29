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

#ifndef __MATMULC8__
#define __MATMULC8__

#include <adf.h>
#include "aie.hpp"

#include "utils.h"         // Contains some utility functions and definitions related to intrinsics usage
#include "mllib_const.h"   // Constants shared across graphs like activation type
// #include "adf/io_buffer/io_buffer_access.h" // temporary utility from tools to access ping pong buffers, to be removed

#include "../conv/conv2d.h"
#include "../conv/conv2d_params.h" // Parameters like increments and wraps needed by kernel

using namespace aie;

template <uint8_t batch_size, act_t act_type, typename dtype_ifm,
          typename dtype_wts, typename dtype_ofm, typename config_ifm,
          typename config_wts, typename config_ofm>
void matmulc8(adf::input_buffer_conf<dtype_ifm, config_ifm> &__restrict ifm,
            adf::input_buffer_conf<dtype_wts, config_wts> &__restrict wts,
            adf::output_buffer_conf<dtype_ofm, config_ofm> &ofm,
            conv2d_params_t &conv2d_params) {
  
    conv2d<batch_size, act_type, dtype_ifm, dtype_wts, dtype_ofm, config_ifm,
           config_wts, config_ofm>(ifm, wts, ofm, conv2d_params);

}

#endif
