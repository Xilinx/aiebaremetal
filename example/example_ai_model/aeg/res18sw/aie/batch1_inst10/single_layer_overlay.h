// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
/*  (c) Copyright 2014 - 2020 Xilinx, Inc. All rights reserved.
   
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

#ifndef _SINGLE_LAYER_OVERLAY_H_
#define _SINGLE_LAYER_OVERLAY_H_

#include <adf.h>

// Top level graph parameters like logical array sizes, tensor sizes, sub-volume sizes
#include "overlay_config.h"

// MLLib configuration, kernel signatures, kernel consts, types etc
#include "mllib_config.h"

using namespace adf ;

namespace mllib_graphs {

template <int COL_OF, int ROW_OF, int memRowId>
class single_layer_overlay : public graph
{
public:

    shared_buffer<uint8> ofm;
    shared_buffer<uint8> wts;

    //WTS externer buffer
    //external_buffer<uint8_t> wts_ddr;

    kernel compute_node[NUM_ROWS][NUM_COLS];  // NUM_ROWSxNUM_COLS -> NUM_ROWS rows and NUM_COLS cols
    pktmerge<NUM_ROWS> ofm_merge[NUM_COLS];  // NUM_ROWS-way pkt merge for OFMs

    single_layer_overlay(uint32_t num_ofm_out,
		         std::vector<access_pattern> ofm_pattern[],
                         std::vector<access_pattern> wts_pattern[],
			 std::vector<access_pattern> wts_ddr_read_pattern,
			 std::vector<access_pattern> wts_ddr_write_pattern,
                         const std::vector<uint32_t>& wts_ddr_dim,
                         const std::vector<uint32_t>& wts_tensor_dim,
                         const std::vector<uint32_t>& ofm_tensor_dim,
			 uint32_t ofmsv_size,
		         uint32_t wtssv_size,
                         const std::vector<location_constraint>& ofm_loc,
			 const std::vector<location_constraint>& wts_loc,
		         uint32_t kernel_iter,
			 uint32_t ofm_repetition,
			 resnet18_mode_t wrapper_mode = REGULAR,
			 uint32_t wts_repetition = 1,
			 uint32_t num_wts = 1)
    {

	//Create data nodes
        ofm = shared_buffer<uint8_t>::create(ofm_tensor_dim,NUM_COLS,num_ofm_out) ;
	wts = shared_buffer<uint8_t>::create(wts_tensor_dim,NUM_DMA_CH,NUM_ROWS) ;
        num_buffers(wts) = num_wts;
        repetition_count(wts) = wts_repetition;

        //Dynamic location constraints	
        location<buffer>(ofm) = ofm_loc;  	
        location<buffer>(wts) = wts_loc; 

	//WTS external buffer for DDR
	//wts_ddr = external_buffer<uint8_t>::create(wts_ddr_dim, 0, NUM_DMA_CH);

        for (int j=0; j<NUM_COLS; j++){
            ofm_merge[j] = pktmerge<NUM_ROWS>::create();
            for (int i=0; i<NUM_ROWS; i++){
                switch(wrapper_mode){
		    case REGULAR:{
                        compute_node[i][j] = kernel::create(resnet18_wrapper<COL_OF>); 
                    }break;
	            case ELTWISE_NON_CASC:{		
                        compute_node[i][j] = kernel::create(resnet18_wrapper_eltwise); 
			async_repetition(compute_node[i][j].in[2]) = ofm_repetition; 
		        location<buffer>(compute_node[i][j].in[2]) = {address(j+COL_OF,i+ROW_OF,0x2000), address(j+COL_OF,i+ROW_OF,0x6000)}; //ifm2 in the first bank with 8kB offset (assume ofm <8kB)
                        location<dma>(compute_node[i][j].in[2]) = dma_channel(aie_tile, j+COL_OF, i+ROW_OF, 0); //ifm2 shares the same s2mm0 with ifm
                    }break;
                    case FC_CASC:					  
		    case CASC:{
			if(j==0){                          // cascade start	      
                           compute_node[i][j] = kernel::create(resnet18_wrapper_casc_start<COL_OF>);
		        }else if(j==(NUM_COLS-1)){         // cascade end
                           compute_node[i][j] = kernel::create(resnet18_wrapper_casc_end<COL_OF>);
			   connect<cascade>(compute_node[i][j-1].out[1],compute_node[i][j].in[2]);
			}else{                             // cascade mid
                           compute_node[i][j] = kernel::create(resnet18_wrapper_casc_mid<COL_OF>);
			   connect<cascade>(compute_node[i][j-1].out[1],compute_node[i][j].in[2]);
			}	
                    }break; 
	            case ELTWISE:		
	            case ELTWISE_CASC:{		
                        if(j==0){                          // cascade start	      
                           compute_node[i][j] = kernel::create(resnet18_wrapper_eltwise_casc_start);
		        }else if(j==(NUM_COLS-1)){         // cascade end
                           compute_node[i][j] = kernel::create(resnet18_wrapper_eltwise_casc_end);
			   connect<cascade>(compute_node[i][j-1].out[1],compute_node[i][j].in[3]);
			}else{                             // cascade mid
                           compute_node[i][j] = kernel::create(resnet18_wrapper_eltwise_casc_mid);
			   connect<cascade>(compute_node[i][j-1].out[1],compute_node[i][j].in[3]);
			}
			async_repetition(compute_node[i][j].in[2]) = ofm_repetition; 
		        location<buffer>(compute_node[i][j].in[2]) = {address(j+COL_OF,i+ROW_OF,0x2000), address(j+COL_OF,i+ROW_OF,0x6000)}; //ifm2 in the first bank with 8kB offset (assume ofm <8kB)
                        location<dma>(compute_node[i][j].in[2]) = dma_channel(aie_tile, j+COL_OF, i+ROW_OF, 0); //ifm2 shares the same s2mm0 with ifm
                    }break;		      
		    default:{
                        compute_node[i][j] = kernel::create(resnet18_wrapper<COL_OF>); 
                    }
                }
                source(compute_node[i][j])  = "resnet18_wrapper.cpp";
                initialization_function(compute_node[i][j]) = "node_init" ;
                repetition_count(compute_node[i][j]) = kernel_iter; // Disable multirate analysis in aiecompiler
	        async_repetition(compute_node[i][j].out[0]) = ofm_repetition; 
                runtime<ratio>(compute_node[i][j])  = 0.8;
                location<kernel>(compute_node[i][j]) = tile(j+COL_OF, i+ROW_OF);
                
                connect(compute_node[i][j].out[0], ofm_merge[j].in[i]); dimensions(compute_node[i][j].out[0]) = {ofmsv_size}; 
		connect(wts.out[i], compute_node[i][j].in[1]); dimensions(compute_node[i][j].in[1]) = {wtssv_size}; 
	
	        //IFM ping-pong double buffers placed 16kB apart in two banks	
		if(wrapper_mode==FC_CASC){
		   location<buffer>(compute_node[i][j].in[0]) = {address(j+COL_OF,i+ROW_OF,0x2000), address(j+COL_OF,i+ROW_OF,0x6000)}; // Place it with OFM
		}else{
		   location<buffer>(compute_node[i][j].in[0]) = {address(j+COL_OF,i+ROW_OF,0x8000), address(j+COL_OF,i+ROW_OF,0xC000)}; // Default placement with WTS
		}	
	        //WTS ping-pong double buffers placed 16kB apart in two banks	
		if(wrapper_mode==MAXIFM){
		   location<buffer>(compute_node[i][j].in[1]) = {address(j+COL_OF,i+ROW_OF,0x2000), address(j+COL_OF,i+ROW_OF,0x6000)}; // Place it with OFM
		}else if(wrapper_mode==FC_CASC){
		   location<buffer>(compute_node[i][j].in[1]) = {address(j+COL_OF,i+ROW_OF,0x8000), address(j+COL_OF,i+ROW_OF,0xC000)}; // Place it at IFM location
                }else{			
		   location<buffer>(compute_node[i][j].in[1]) = {address(j+COL_OF,i+ROW_OF,0x9800), address(j+COL_OF,i+ROW_OF,0xD800)}; // Default placement with IFM
                }
		//OFM ping-pong double buffers placed 16kB apart in two banks
                location<buffer>(compute_node[i][j].out[0]) = {address(j+COL_OF,i+ROW_OF,0x0), address(j+COL_OF,i+ROW_OF,0x4000)};
	        //Stack+Heap with IFM	
                location<stack>(compute_node[i][j]) = {address(j+COL_OF,i+ROW_OF,0xEB40)};
		
		location<dma>(compute_node[i][j].in[0]) = dma_channel(aie_tile, j+COL_OF, i+ROW_OF, 0); //s2mm0
	        location<dma>(compute_node[i][j].out[0]) = dma_channel(aie_tile, j+COL_OF, i+ROW_OF, 0); //mm2s0
                location<dma>(compute_node[i][j].in[1]) = dma_channel(aie_tile, j+COL_OF, i+ROW_OF, 1); //s2mm1
		
		if(j==0){
		   read_access(wts.out[i]) = wts_pattern[i]; 
                   location<dma>(wts.out[i]) = dma_channel(memory_tile, 4+COL_OF, memRowId, i);
		}	
            }
            connect<pktstream>(ofm_merge[j].out[0], ofm.in[j]);
	    write_access(ofm.in[j]) =  ofm_pattern[j];
            if(j<2){       //[0:1]	    
               location<dma>(ofm.in[j]) = dma_channel(memory_tile, 1+COL_OF, memRowId, j);
	    }else if(j<5){ //[2:4]   
               location<dma>(ofm.in[j]) = dma_channel(memory_tile, 2+COL_OF, memRowId, (j-2));
	    }else
               location<dma>(ofm.in[j]) = dma_channel(memory_tile, 3+COL_OF, memRowId, (j-5));
        }
	/*for(uint32_t i=0; i<NUM_DMA_CH; i++){
		//int mem_col = 3*(1+(i>>1)); // for first two channels (3,0) and if num channels more than 2 then (6,0) is chosen by this logic
		
		//find nearest 2 shim cols to 4+COL_OF(wts location) and 
		int shim_col = shim_loc[i>>1]; //TO-DO for now fixed for COL_OF=0
		location<dma>(wts.in[i]) = dma_channel(memory_tile, 4+COL_OF, memRowId, i);
        	connect(wts_ddr.out[i], wts.in[i]);
        	read_access(wts_ddr.out[i]) = wts_ddr_read_pattern[i];
        	write_access(wts.in[i]) = wts_ddr_write_pattern[i];
        	location<dma>(wts_ddr.out[i]) = dma_channel(shim_tile, shim_col, 0, i%2);
	}*/
    }

    void connect_layer(shared_buffer<uint8>& ifm, std::vector<access_pattern> ifm_pattern[], uint32_t ifmsv_size, resnet18_mode_t wrapper_mode=REGULAR, int offset=0)
    {
	int fac = (wrapper_mode==ELTWISE) ? 1 : 0 ;    
        for (int j=0; j<NUM_COLS; j++){
            for (int i=0; i<NUM_ROWS; i++){
                connect(ifm.out[j+fac*NUM_COLS+offset], compute_node[i][j].in[2*fac]); dimensions(compute_node[i][j].in[2*fac]) = {ifmsv_size}; 
	    }
            read_access(ifm.out[j+fac*NUM_COLS+offset]) =  ifm_pattern[j];  	    
            if(j<2){        //[0:1]	    
               location<dma>(ifm.out[j+fac*NUM_COLS+offset]) = dma_channel(memory_tile, 1+COL_OF, memRowId, j);
	    }else if(j<5){  //[2:4]   
               location<dma>(ifm.out[j+fac*NUM_COLS+offset]) = dma_channel(memory_tile, 2+COL_OF, memRowId, (j-2));
	    }else
               location<dma>(ifm.out[j+fac*NUM_COLS+offset]) = dma_channel(memory_tile, 3+COL_OF, memRowId, (j-5));
        }
    }

    void connect_wts(external_buffer<uint8>& wts_ddr, std::vector<access_pattern> wts_ddr_write_pattern){
        for(uint32_t i=0; i<NUM_DMA_CH; i++){
            location<dma>(wts.in[i]) = dma_channel(memory_tile, 4+COL_OF, memRowId, i);
            connect(wts_ddr.out[i], wts.in[i]);
            write_access(wts.in[i]) = wts_ddr_write_pattern[i];
        }
    }	  
};

};

#endif // _SINGLE_LAYER_OVERLAY_H_
