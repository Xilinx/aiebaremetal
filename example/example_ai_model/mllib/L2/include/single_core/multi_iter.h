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

#ifndef __MULTI_ITER_H__
#define __MULTI_ITER_H__

#include "adf.h"

// Single layer structual ADF graph definitions from MLLib ADF library
#include "simple_overlay.h"

// Temporal tiling with access patterns in layer-by-layer fashion
#include "tiling_multi_iter.h"

using namespace adf;
using namespace mllib_graphs;

class multi_iter : public graph
{
public:
	
    simple_overlay multi_iter_layer = {simple_overlay(ifmsv_size_multi_iter, wtssv_size_multi_iter, ofmsv_size_multi_iter, ofm_repetition_multi_iter)};
    external_buffer<uint8_t> ifm_ddr, ofm_ddr, wts_ddr;
    
    multi_iter()
    {
        //External buffers
	//Input from DDR
	ifm_ddr = external_buffer<uint8_t>::create(ifm_dim_multi_iter, 0, 1);
        ofm_ddr = external_buffer<uint8_t>::create(ofm_dim_multi_iter, 1, 0);
        wts_ddr = external_buffer<uint8_t>::create(wts_dim_multi_iter, 0, 1);

	connect<stream>(ifm_ddr.out[0], multi_iter_layer.ifm.in[0]);
        read_access(ifm_ddr.out[0]) = input_ddr_pattern_multi_iter;
        write_access(multi_iter_layer.ifm.in[0]) = input_ddr_pattern_multi_iter;
        location<dma>(ifm_ddr.out[0]) = dma_channel(shim_tile, 2, 0, 0); 

	connect<stream>(wts_ddr.out[0], multi_iter_layer.wts.in[0]);
        read_access(wts_ddr.out[0]) = wts_ddr_pattern_multi_iter;
        write_access(multi_iter_layer.wts.in[0]) = wts_ddr_pattern_multi_iter;
        location<dma>(wts_ddr.out[0]) = dma_channel(shim_tile, 3, 0, 0);

	connect<stream>(multi_iter_layer.ofm.out[0], ofm_ddr.in[0]);
        read_access(multi_iter_layer.ofm.out[0]) = output_ddr_pattern_multi_iter;
        write_access(ofm_ddr.in[0]) = output_ddr_pattern_multi_iter;
        location<dma>(ofm_ddr.in[0]) = dma_channel(shim_tile, 2, 0, 0);

	read_access(multi_iter_layer.wts.out[0]) = wts_pattern_multi_iter ;
	read_access(multi_iter_layer.ifm.out[0]) = ifm_pattern_multi_iter;
        write_access(multi_iter_layer.ofm.in[0]) = ofm_pattern_multi_iter; 	
    }
};

#endif //__MULTI_ITER_H__
