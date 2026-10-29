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

//Tiling parameters for single core basic graph 

#ifndef __TILING_BASIC_H__
#define __TILING_BASIC_H__

#include "adf.h"
#include "graph_params.h"   // Holds default parameters, these can be over-ridden from top

std::vector<uint32_t> ifm_dim_basic = {IFM_TENSOR_VOL};
std::vector<uint32_t> wts_dim_basic = {WT_TENSOR_VOL};
std::vector<uint32_t> ofm_dim_basic = {OFM_TENSOR_VOL};

std::vector<uint32_t> ifmsv_dim_basic = {IFM_WIN_SIZE};
std::vector<uint32_t> wtssv_dim_basic = {WT_WIN_SIZE};
std::vector<uint32_t> ofmsv_dim_basic = {OFM_WIN_SIZE};

uint32_t ifmsv_size_basic = IFM_WIN_SIZE;
uint32_t wtssv_size_basic = WT_WIN_SIZE;
uint32_t ofmsv_size_basic = OFM_WIN_SIZE;

uint32_t kernel_iter_basic = 1;
uint32_t ofm_repetition_basic = 1;

// Access pattern for getting IFM SVs from memory tile
access_pattern ifm_pattern_basic = tiling({.buffer_dimension = ifm_dim_basic, .tiling_dimension = ifmsv_dim_basic, .offset = {0}, .tile_traversal = {{.dimension = 0,.stride = IFM_WIN_SIZE, .wrap = 1}},.packet_port_id = -1,.repetition = 1});
     
// Access pattern for getting weights SVs from memory tile
access_pattern wts_pattern_basic = tiling({.buffer_dimension = wts_dim_basic, .tiling_dimension = wtssv_dim_basic, .offset = {0}, .tile_traversal = {{.dimension = 0,.stride = WT_WIN_SIZE, .wrap = 1}},.packet_port_id = -1,.repetition = 1});

// Access pattern for storing OFM SVs to memory tile
access_pattern ofm_pattern_basic = tiling({.buffer_dimension = ofm_dim_basic, .tiling_dimension = ofmsv_dim_basic, .offset = { 0}});
     
// DDR access pattern
access_pattern wts_ddr_pattern_basic = tiling({ .buffer_dimension = wts_dim_basic, .tiling_dimension = wts_dim_basic, .offset = {0} });


//Input
access_pattern input_ddr_pattern_basic = tiling({ .buffer_dimension = ifm_dim_basic,.tiling_dimension = ifm_dim_basic,.offset = { 0 } });

//final output
access_pattern output_ddr_pattern_basic = tiling({ .buffer_dimension = ofm_dim_basic,.tiling_dimension = ofm_dim_basic,.offset = { 0 } });

#endif // __TILING_BASIC_H__
