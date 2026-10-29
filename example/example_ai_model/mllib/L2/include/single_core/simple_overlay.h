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

#ifndef __SIMPLE_OVERLAY_H__
#define __SIMPLE_OVERLAY_H__

#include <adf.h>

// Top level parameters like array size and buffer sizes, ping pong buffer locations for graph
#include "graph_params.h"

// Common constants referred by kernels and graphs
#include "mllib_const.h"
#include "mllib_wrappers.h"

using namespace adf ;

namespace mllib_graphs {

class simple_overlay : public graph
{
public:

    shared_buffer<uint8> ifm;
    shared_buffer<uint8> ofm;
    shared_buffer<uint8> wts;

    kernel compute_node;  // NUM_ROWSxNUM_COLS -> NUM_ROWS rows and NUM_COLS cols

    simple_overlay(uint32_t ifmsv_size,
		         uint32_t wtssv_size,
			 uint32_t ofmsv_size,
			 uint32_t ofm_repetition)
    {

	//Create data nodes	
        ifm = shared_buffer<uint8_t>::create({IFM_BUF_MAX},1,1) ;
        ofm = shared_buffer<uint8_t>::create({OFM_BUF_MAX},1,1) ;
        wts = shared_buffer<uint8_t>::create({WTS_BUF_MAX},1,1) ;
        location<buffer>(ifm) = address(0, 0, 0);	
        location<buffer>(ofm) = address(1, 0, 0);	
        location<buffer>(wts) = address(3, 0, 0);       
        
        compute_node = kernel::create(mllib_wrapper<NUM_BATCH,ACT,DTYPE_IN,DTYPE_WTS,DTYPE_OUT,bpc_sync_1d, bpc_sync_1d, bpc_async_1d>) ; 
        source(compute_node)  = "mllib_wrapper.cpp";
        initialization_function(compute_node) = "node_init" ;
        repetition_count(compute_node) = NUM_ITER; // Disable multirate analysis in aiecompiler
        async_repetition(compute_node.out[0]) = ofm_repetition;  
        runtime<ratio>(compute_node)  = 0.8;
        location<kernel>(compute_node) = tile(0, 0);
                
	connect(ifm.out[0], compute_node.in[0]); dimensions(compute_node.in[0]) = {ifmsv_size}; 
        connect(compute_node.out[0], ofm.in[0]); dimensions(compute_node.out[0]) = {ofmsv_size}; 
	connect(wts.out[0], compute_node.in[1]); dimensions(compute_node.in[1]) = {wtssv_size}; 

	location<buffer>(compute_node.in[0]) = {address(0,0,IFM_PING), address(0,0,IFM_PONG)}; //IFM ping-pong double buffers placed 16kB apart in two banks
	location<buffer>(compute_node.in[1]) = {address(0,0,WTS_PING), address(0,0,WTS_PONG)}; //WTS ping-pong double buffers placed 16kB apart in two banks
        location<buffer>(compute_node.out[0]) = {address(0,0,OFM_PING), address(0,0,OFM_PONG)};  //OFM ping-pong double buffers placed 16kB apart in two banks
        location<stack>(compute_node) = {address(0,0,STK_OFFSET)};
	
	location<dma>(compute_node.in[0]) = dma_channel(aie_tile, 0, 0, 0); //s2mm0
	location<dma>(compute_node.out[0]) = dma_channel(aie_tile, 0, 0, 0); //mm2s0
        location<dma>(compute_node.in[1]) = dma_channel(aie_tile, 0, 0, 1); //s2mm1
		
        location<dma>(wts.out[0]) = dma_channel(memory_tile, 3, 0, 0);
        location<dma>(ifm.out[0]) = dma_channel(memory_tile, 1, 0, 0);
        location<dma>(ofm.in[0]) = dma_channel(memory_tile, 1, 0, 0);        

        location<dma>(ifm.in[0]) = dma_channel(memory_tile, 0, 0, 0);
        location<dma>(wts.in[0]) = dma_channel(memory_tile, 3, 0, 0);
        location<dma>(ofm.out[0]) = dma_channel(memory_tile, 2, 0, 0);
    }
};

};

#endif // __SIMPLE_OVERLAY_H__

