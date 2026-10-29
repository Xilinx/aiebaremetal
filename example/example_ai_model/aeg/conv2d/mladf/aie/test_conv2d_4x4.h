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

#ifndef __TEST_CONV2D_4x4_H__
#define __TEST_CONV2D_4x4_H__

#include "adf.h"

// Top level parameters like array size and buffer sizes, ping pong buffer locations for graph
#include "graph_params.h"

// Single layer structual ADF graph from MLLib ADF library
#include "single_layer_4x4_overlay_conv2d.h"

// Temporal tiling with access patterns for all memory
#include "tiling_conv2d_4x4.h"


using namespace adf;
using namespace mllib_graphs;

////////////////////////////////////////////////////////////////////////
//This is an example of a simple single layer
////////////////////////////////////////////////////////////////////////

class single_layer : public graph
{
public:

    layer_overlay layer = {layer_overlay(1, ofm_pattern_g, wts_pattern_g, wts_ddr_read_pattern_g, wts_ddr_write_pattern_g,
		                     wts_dim_g, ofmsv_size_g, wtssv_size_g, ofm_loc_g, wts_loc_g, kernel_iter_g, ofm_repetition_g)};


    external_buffer<uint8_t> ifm_ddr, ofm_ddr;
    shared_buffer<uint8_t> ifm;

    single_layer()
    {
        //External buffers
	//Input from DDR
	ifm_ddr = external_buffer<uint8_t>::create(input_dim_g, 0, 1);
        ofm_ddr = external_buffer<uint8_t>::create(output_dim_g, 1, 0);

	ifm = shared_buffer<uint8_t>::create({IFM_BUF_MAX},1,NUM_COLS) ;
	location<buffer>(ifm) = address(0+COL_OFFSET, 0, 0);

	connect(ifm_ddr.out[0], ifm.in[0]);
        read_access(ifm_ddr.out[0]) = input_ddr_pattern_g;
        write_access(ifm.in[0]) = input_ddr_pattern_g;
        location<dma>(ifm_ddr.out[0]) = dma_channel(shim_tile, 2+COL_OFFSET, 0, 0);
	location<dma>(ifm.in[0]) = dma_channel(memory_tile, 0+COL_OFFSET, 0, 0);

	connect(layer.ofm.out[0], ofm_ddr.in[0]);
        read_access(layer.ofm.out[0]) = output_ddr_read_pattern_g;
        write_access(ofm_ddr.in[0]) = output_ddr_write_pattern_g;
        location<dma>(ofm_ddr.in[0]) = dma_channel(shim_tile, 2+COL_OFFSET, 0, 0);

	//Layer connection
	layer.connect_layer(ifm, ifm_pattern_g, ifmsv_size_g);

        location<dma>(layer.ofm.out[0]) = dma_channel(memory_tile, 2+COL_OFFSET, 0, 0);

    }
};

#endif //__TEST_CONV2D_4x4_H__
