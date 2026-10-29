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

#include <adf.h>
#include "mllib_wrappers.h"

#ifndef SM_BATCH_SIZE
#define SM_BATCH_SIZE 1
#endif
#ifndef NUM_SM_CLASSES
#define NUM_SM_CLASSES (1000)
#endif
#ifndef SM_HEAP_SIZE
#define SM_HEAP_SIZE (8384)
#endif
#ifndef SM_INPUT_SIZE
#define SM_INPUT_SIZE (1024)
#endif


using namespace adf;

namespace mllib_graphs {

class softmax_subgraph: public graph {

    public:
        port<input> in;
        port<output> out;
        
	kernel softmax_inst;

    softmax_subgraph(const std::vector<uint32_t>& sm_loc) 
    {
	    
	softmax_inst = kernel::create(softmax_wrapper<SM_BATCH_SIZE,SM_INPUT_SIZE,NUM_SM_CLASSES,bpc_sync_1d,bpc_sync_1d>) ;

        source(softmax_inst) = "softmax_wrapper.cpp";
        initialization_function(softmax_inst) = "softmax_init" ;
        // Specify mapping constraints
        runtime<ratio>(softmax_inst) = 0.8;
        heap_size(softmax_inst) = SM_HEAP_SIZE;

	// Specify iterations
	// Disable multirate analysis in aiecompiler
        repetition_count(softmax_inst) = 1; 
	location<kernel>(softmax_inst) = tile(sm_loc[0], sm_loc[1]);
	// buffer location constraints
        location<buffer>(softmax_inst.in[0]) = {address(sm_loc[0], sm_loc[1], 0x8000), address(sm_loc[0], sm_loc[1], 0xC000)}; 
        location<buffer>(softmax_inst.out[0]) = {address(sm_loc[0], sm_loc[1], 0x0), address(sm_loc[0], sm_loc[1], 0x4400)};  
        location<stack>(softmax_inst) = {address(sm_loc[0], sm_loc[1], 0x2200)};
        // DMA location constraints
        location<dma>(softmax_inst.in[0]) = dma_channel(aie_tile, sm_loc[0], sm_loc[1], 0); //s2mm0
        location<dma>(softmax_inst.out[0]) = dma_channel(aie_tile, sm_loc[0], sm_loc[1], 0); //mm2s0 

        // Specify core-side DMA transfer lengths 
	// Make connections between data nodes and AIE        
	dimensions(softmax_inst.in[0]) = {SM_INPUT_SIZE*SM_BATCH_SIZE};
        dimensions(softmax_inst.out[0]) = {SM_INPUT_SIZE*SM_BATCH_SIZE};

        connect(in, softmax_inst.in[0]); 
        connect(softmax_inst.out[0], out);

    } 

    void connect_layer(shared_buffer<uint8>& ifm, shared_buffer<uint8>& ofm, access_pattern& ifm_access_pattern, access_pattern& ofm_access_pattern)
    {
	connect(ifm.out[0], in);
        read_access(ifm.out[0]) = ifm_access_pattern;
	connect(out, ofm.in[0]);
        write_access(ofm.in[0]) = ofm_access_pattern;    
    }	    
};
};

