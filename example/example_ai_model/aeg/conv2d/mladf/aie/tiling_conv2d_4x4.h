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

#ifndef __TILING_CONV2D_4x4_H__
#define __TILING_CONV2D_4x4_H__

// Specify tiling and memory access patterns for single layer graph packaged with MLLib

#include "adf.h"
#include "graph_params.h"   // Holds default parameters, these can be over-ridden from top

#define WT_TENSOR_VOL (FILTER_K_y * FILTER_K_x * IFM_DEPTH * OFM_DEPTH + WT_CONST_DELTA * WTS_FOLDING * IFM_DEPTH / IFM_SV_DEPTH)

// 18 -> 20, ifmsv_dim_g_extra_pad_width[1] should be divisiable by 4. (See kernel constraint ifm_sv_widthBoundaryAlign in Conv2d.yaml)
uint32_t ifm_width_pad = 2;
// 72 -> 80, ofm_dim_g_extra_pad_width[1] should be OFM_SV_WIDTH(16) * wrap(5) = 80
uint32_t ofm_width_pad = 8;
std::vector<uint32_t> ifm_dim_g = { NUM_BATCH * 8, IFM_WIDTH, IFM_DEPTH / (8), IFM_HEIGHT };
std::vector<uint32_t> wts_dim_g = { WT_TENSOR_VOL };
// {8, 72, 64 / 8, 72}, L3
std::vector<uint32_t> ofm_dim_g = { NUM_BATCH * 8, OFM_WIDTH, OFM_DEPTH / (8), OFM_HEIGHT };
// {8, 72 + 8, 64 / 8, 72}, L2
std::vector<uint32_t> ofm_dim_g_extra_pad_width = { NUM_BATCH * 8, OFM_WIDTH + ofm_width_pad, OFM_DEPTH / (8), OFM_HEIGHT };

std::vector<uint32_t> ifmsv_dim_g = { NUM_BATCH * 8, IFM_SV_WIDTH, IFM_SV_DEPTH / (8), IFM_SV_HEIGHT };
// {8, 18 + 2, 16 / 8, 20}, L1
std::vector<uint32_t> ifmsv_dim_g_extra_pad_width = { NUM_BATCH * 8, IFM_SV_WIDTH + ifm_width_pad, IFM_SV_DEPTH / (8), IFM_SV_HEIGHT };
std::vector<uint32_t> wtssv_dim_g = { WT_WIN_SIZE };
std::vector <uint32_t> ofmsv_dim_g = { NUM_BATCH * 8, OFM_SV_WIDTH, OFM_SV_DEPTH / (8), OFM_SV_HEIGHT };

uint32_t ifmsv_size_g = ifmsv_dim_g_extra_pad_width[0] * ifmsv_dim_g_extra_pad_width[1] * ifmsv_dim_g_extra_pad_width[2] * ifmsv_dim_g_extra_pad_width[3];
uint32_t wtssv_size_g = WT_WIN_SIZE;
uint32_t ofmsv_size_g = OFM_WIN_SIZE;

std::vector<location_constraint> ofm_loc_g = { address(1, 0, (IFM_BUF_MAX >> 1)) };
std::vector<location_constraint> wts_loc_g = { address(3 + COL_OFFSET,0,0) };

uint32_t height_iter = ofm_dim_g[3] / ofmsv_dim_g[3] / NUM_COLS;
uint32_t width_iter = ofm_dim_g[1] / ofmsv_dim_g[1] + 1;
uint32_t depth_iter = ifm_dim_g[2] / ifmsv_dim_g_extra_pad_width[2];
uint32_t channel_iter = ofm_dim_g[2] / ofmsv_dim_g[2] / NUM_ROWS;
uint32_t ifm_repetition = channel_iter;

// uint32_t kernel_iter_g = NUM_ITER;
uint32_t kernel_iter_g = height_iter * width_iter * depth_iter * channel_iter;
uint32_t ofm_repetition_g = depth_iter;

// Access pattern for getting IFM SVs from memory tile
std::vector<access_pattern> ifm_pattern_g[] = {
    {tiling({.buffer_dimension = ifm_dim_g, .tiling_dimension = ifmsv_dim_g_extra_pad_width, .offset = {0, -(NUM_BATCH * ((FILTER_K - 1) >> 1)), 0, -1 + (0 * HEIGHT_ITER / WIDTH_FOLDING * (IFM_SV_HEIGHT - FILTER_K + 1)) },.tile_traversal = { {.dimension = 2, .stride = IFM_SV_DEPTH / 8, .wrap = depth_iter}, {.dimension = 1,.stride = IFM_SV_WIDTH - FILTER_K + 1,.wrap = 5} }, .packet_port_id = -1, .repetition = ifm_repetition, .boundary_dimension = ifm_dim_g})},
    {tiling({.buffer_dimension = ifm_dim_g, .tiling_dimension = ifmsv_dim_g_extra_pad_width, .offset = {0, -(NUM_BATCH * ((FILTER_K - 1) >> 1)), 0, -1 + (1 * HEIGHT_ITER / WIDTH_FOLDING * (IFM_SV_HEIGHT - FILTER_K + 1)) },.tile_traversal = { {.dimension = 2, .stride = IFM_SV_DEPTH / 8, .wrap = depth_iter}, {.dimension = 1,.stride = IFM_SV_WIDTH - FILTER_K + 1,.wrap = 5} }, .packet_port_id = -1, .repetition = ifm_repetition, .boundary_dimension = ifm_dim_g})},
    {tiling({.buffer_dimension = ifm_dim_g, .tiling_dimension = ifmsv_dim_g_extra_pad_width, .offset = {0, -(NUM_BATCH * ((FILTER_K - 1) >> 1)), 0, -1 + (2 * HEIGHT_ITER / WIDTH_FOLDING * (IFM_SV_HEIGHT - FILTER_K + 1)) },.tile_traversal = { {.dimension = 2, .stride = IFM_SV_DEPTH / 8, .wrap = depth_iter}, {.dimension = 1,.stride = IFM_SV_WIDTH - FILTER_K + 1,.wrap = 5} }, .packet_port_id = -1, .repetition = ifm_repetition, .boundary_dimension = ifm_dim_g})},
    {tiling({.buffer_dimension = ifm_dim_g, .tiling_dimension = ifmsv_dim_g_extra_pad_width, .offset = {0, -(NUM_BATCH * ((FILTER_K - 1) >> 1)), 0, -1 + (3 * HEIGHT_ITER / WIDTH_FOLDING * (IFM_SV_HEIGHT - FILTER_K + 1)) },.tile_traversal = { {.dimension = 2, .stride = IFM_SV_DEPTH / 8, .wrap = depth_iter}, {.dimension = 1,.stride = IFM_SV_WIDTH - FILTER_K + 1,.wrap = 5} }, .packet_port_id = -1, .repetition = ifm_repetition, .boundary_dimension = ifm_dim_g})}
};

// Access pattern for getting weights SVs from memory tile
std::vector<access_pattern> wts_pattern_g[] = {
    {tiling({.buffer_dimension = wts_dim_g, .tiling_dimension = wtssv_dim_g, .offset = {(int32_t)(0 * wtssv_size_g * ofm_repetition_g)}, .tile_traversal = { {.dimension = 0,.stride = wtssv_size_g,.wrap = ofm_repetition_g } },.packet_port_id = -1,.repetition = width_iter * height_iter, .boundary_dimension = wts_dim_g })},
    {tiling({.buffer_dimension = wts_dim_g, .tiling_dimension = wtssv_dim_g, .offset = {(int32_t)(1 * wtssv_size_g * ofm_repetition_g)}, .tile_traversal = { {.dimension = 0,.stride = wtssv_size_g,.wrap = ofm_repetition_g } },.packet_port_id = -1,.repetition = width_iter * height_iter, .boundary_dimension = wts_dim_g })},
    {tiling({.buffer_dimension = wts_dim_g, .tiling_dimension = wtssv_dim_g, .offset = {(int32_t)(2 * wtssv_size_g * ofm_repetition_g)}, .tile_traversal = { {.dimension = 0,.stride = wtssv_size_g,.wrap = ofm_repetition_g } },.packet_port_id = -1,.repetition = width_iter * height_iter, .boundary_dimension = wts_dim_g })},
    {tiling({.buffer_dimension = wts_dim_g, .tiling_dimension = wtssv_dim_g, .offset = {(int32_t)(3 * wtssv_size_g * ofm_repetition_g)}, .tile_traversal = { {.dimension = 0,.stride = wtssv_size_g,.wrap = ofm_repetition_g } },.packet_port_id = -1,.repetition = width_iter * height_iter, .boundary_dimension = wts_dim_g })}
};

// Access pattern for storing OFM SVs to memory tile
std::vector<access_pattern> ofm_pattern_g[] = {
    {tiling({.buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofmsv_dim_g, .offset = { 0, 0, 0 * 2, 0 * 18 },.tile_traversal = { {.dimension = 1,.stride = OFM_SV_WIDTH,.wrap = 5 } },.packet_port_id = 0, .repetition = 1 }),
     tiling({.buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofmsv_dim_g, .offset = { 0, 0, 1 * 2, 0 * 18 },.tile_traversal = { {.dimension = 1,.stride = OFM_SV_WIDTH,.wrap = 5 } },.packet_port_id = 1, .repetition = 1 }),
     tiling({.buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofmsv_dim_g, .offset = { 0, 0, 2 * 2, 0 * 18 },.tile_traversal = { {.dimension = 1,.stride = OFM_SV_WIDTH,.wrap = 5 } },.packet_port_id = 2, .repetition = 1 }),
     tiling({.buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofmsv_dim_g, .offset = { 0, 0, 3 * 2, 0 * 18 },.tile_traversal = { {.dimension = 1,.stride = OFM_SV_WIDTH,.wrap = 5 } },.packet_port_id = 3, .repetition = 1 })},
    {tiling({.buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofmsv_dim_g, .offset = { 0, 0, 0 * 2, 1 * 18 },.tile_traversal = { {.dimension = 1,.stride = OFM_SV_WIDTH,.wrap = 5 } },.packet_port_id = 0, .repetition = 1 }),
     tiling({.buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofmsv_dim_g, .offset = { 0, 0, 1 * 2, 1 * 18 },.tile_traversal = { {.dimension = 1,.stride = OFM_SV_WIDTH,.wrap = 5 } },.packet_port_id = 1, .repetition = 1 }),
     tiling({.buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofmsv_dim_g, .offset = { 0, 0, 2 * 2, 1 * 18 },.tile_traversal = { {.dimension = 1,.stride = OFM_SV_WIDTH,.wrap = 5 } },.packet_port_id = 2, .repetition = 1 }),
     tiling({.buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofmsv_dim_g, .offset = { 0, 0, 3 * 2, 1 * 18 },.tile_traversal = { {.dimension = 1,.stride = OFM_SV_WIDTH,.wrap = 5 } },.packet_port_id = 3, .repetition = 1 })},
    {tiling({.buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofmsv_dim_g, .offset = { 0, 0, 0 * 2, 2 * 18 },.tile_traversal = { {.dimension = 1,.stride = OFM_SV_WIDTH,.wrap = 5 } },.packet_port_id = 0, .repetition = 1 }),
     tiling({.buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofmsv_dim_g, .offset = { 0, 0, 1 * 2, 2 * 18 },.tile_traversal = { {.dimension = 1,.stride = OFM_SV_WIDTH,.wrap = 5 } },.packet_port_id = 1, .repetition = 1 }),
     tiling({.buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofmsv_dim_g, .offset = { 0, 0, 2 * 2, 2 * 18 },.tile_traversal = { {.dimension = 1,.stride = OFM_SV_WIDTH,.wrap = 5 } },.packet_port_id = 2, .repetition = 1 }),
     tiling({.buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofmsv_dim_g, .offset = { 0, 0, 3 * 2, 2 * 18 },.tile_traversal = { {.dimension = 1,.stride = OFM_SV_WIDTH,.wrap = 5 } },.packet_port_id = 3, .repetition = 1 })},
    {tiling({.buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofmsv_dim_g, .offset = { 0, 0, 0 * 2, 3 * 18 },.tile_traversal = { {.dimension = 1,.stride = OFM_SV_WIDTH,.wrap = 5 } },.packet_port_id = 0, .repetition = 1 }),
     tiling({.buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofmsv_dim_g, .offset = { 0, 0, 1 * 2, 3 * 18 },.tile_traversal = { {.dimension = 1,.stride = OFM_SV_WIDTH,.wrap = 5 } },.packet_port_id = 1, .repetition = 1 }),
     tiling({.buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofmsv_dim_g, .offset = { 0, 0, 2 * 2, 3 * 18 },.tile_traversal = { {.dimension = 1,.stride = OFM_SV_WIDTH,.wrap = 5 } },.packet_port_id = 2, .repetition = 1 }),
     tiling({.buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofmsv_dim_g, .offset = { 0, 0, 3 * 2, 3 * 18 },.tile_traversal = { {.dimension = 1,.stride = OFM_SV_WIDTH,.wrap = 5 } },.packet_port_id = 3, .repetition = 1 })}
};

// DDR access pattern
access_pattern wts_ddr_pattern_g = tiling({ .buffer_dimension = wts_dim_g, .tiling_dimension = wts_dim_g, .offset = {0} });
access_pattern wts_ddr_read_pattern_g = wts_ddr_pattern_g;
access_pattern wts_ddr_write_pattern_g = wts_ddr_pattern_g;

//Input
std::vector<uint32_t> input_dim_g = ifm_dim_g;
std::vector<uint32_t> output_dim_g = ofm_dim_g;
access_pattern input_ddr_pattern_g = tiling({ .buffer_dimension = input_dim_g, .tiling_dimension = input_dim_g, .offset = { 0, 0, 0, 0 } });

//Output
access_pattern output_ddr_read_pattern_g = tiling({ .buffer_dimension = ofm_dim_g_extra_pad_width, .tiling_dimension = ofm_dim_g, .offset = { 0, 0, 0, 0 } });
access_pattern output_ddr_write_pattern_g = tiling({ .buffer_dimension = ofm_dim_g, .tiling_dimension = ofm_dim_g, .offset = { 0, 0, 0, 0 } });

#endif // __TILING_CONV2D_4x4_H__
