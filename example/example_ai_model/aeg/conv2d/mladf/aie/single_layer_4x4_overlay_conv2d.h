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

#ifndef __SINGLE_LAYER_4x4_OVERLAY_CONV2D_H__
#define __SINGLE_LAYER_4x4_OVERLAY_CONV2D_H__

#include <adf.h>

    // Top level parameters like array size and buffer sizes, ping pong buffer locations for graph
#include "graph_params.h"

// Common constants referred by kernels and graphs
#include "mllib_const.h"

template <uint8 batch_size, act_t act, typename dtype_ifm, typename dtype_wts, typename dtype_ofm, typename config_ifm, typename config_wts, typename config_ofm>
void conv2d_4x4_wrapper(
    adf::input_buffer_conf<dtype_ifm, config_ifm>& __restrict ifm,
    adf::input_buffer_conf<dtype_wts, config_wts>& __restrict wts,
    adf::output_buffer_conf<dtype_ofm, config_ofm>& __restrict ofm
);

using namespace adf;

namespace mllib_graphs {

    class layer_overlay : public graph
    {
    public:

        shared_buffer<uint8> ofm;
        shared_buffer<uint8> wts;

        //WTS external buffer
        external_buffer<uint8_t> wts_ddr;

        kernel compute_node[NUM_ROWS][NUM_COLS];  // Array of NUM_ROWSxNUM_COLS
        pktmerge<NUM_ROWS> ofm_merge[NUM_COLS];   // NUM_ROWS-way pkt merge for OFMs

        layer_overlay(uint32_t num_ofm_out,
            std::vector<access_pattern> ofm_pattern[],
            std::vector<access_pattern> wts_pattern[],
            const access_pattern& wts_ddr_read_pattern,
            const access_pattern& wts_ddr_write_pattern,
            const std::vector<uint32_t>& wts_ddr_dim,
            uint32_t ofmsv_size,
            uint32_t wtssv_size,
            const std::vector<location_constraint>& ofm_loc,
            const std::vector<location_constraint>& wts_loc,
            uint32_t kernel_iter,
            uint32_t ofm_repetition,
            uint32_t wts_repetition = 1,
            uint32_t num_wts = 1)
        {
            //Create data nodes
            ofm = shared_buffer<uint8_t>::create({ OFM_BUF_MAX }, NUM_COLS, num_ofm_out);
            wts = shared_buffer<uint8_t>::create({ WTS_BUF_MAX }, 1, NUM_ROWS);
            num_buffers(wts) = num_wts;
            repetition_count(wts) = wts_repetition;

            //Dynamic location constraints
            location<buffer>(ofm) = ofm_loc;
            location<buffer>(wts) = wts_loc;

            //WTS external buffer for DDR
            wts_ddr = external_buffer<uint8_t>::create(wts_ddr_dim, 0, 1);

            for (int j = 0; j < NUM_COLS; j++) {
                ofm_merge[j] = pktmerge<NUM_ROWS>::create();
                for (int i = 0; i < NUM_ROWS; i++) {
                    // Create kernel, specify wrapper source and how many times kernel will be invoked and location constraints for the kernels
                    compute_node[i][j] = kernel::create(conv2d_4x4_wrapper<NUM_BATCH, LINEAR, DTYPE_IN, int8, int8, bpc_sync_1d, bpc_sync_1d, bpc_async_1d>);
                    source(compute_node[i][j]) = "conv2d_4x4_wrapper.cpp";
                    initialization_function(compute_node[i][j]) = "node_init";
                    // repetition_count(compute_node[i][j]) = IFM_DEPTH / IFM_SV_DEPTH; // Disable multirate analysis in aiecompiler
                    repetition_count(compute_node[i][j]) = kernel_iter; // Disable multirate analysis in aiecompiler
                    async_repetition(compute_node[i][j].out[0]) = ofm_repetition;
                    runtime<ratio>(compute_node[i][j]) = 0.8; // map kernel on one AIE core (80% core utilization)
                    location<kernel>(compute_node[i][j]) = tile(j + COL_OFFSET, i);

                    connect(compute_node[i][j].out[0], ofm_merge[j].in[i]);
                    dimensions(compute_node[i][j].out[0]) = { ofmsv_size };
                    connect(wts.out[i], compute_node[i][j].in[1]);
                    dimensions(compute_node[i][j].in[1]) = { wtssv_size };

                    location<buffer>(compute_node[i][j].in[0]) = { address(j + COL_OFFSET,i,IFM_PING), address(j + COL_OFFSET,i,IFM_PONG) }; //IFM ping-pong double buffers placed 16kB apart in two banks
                    location<buffer>(compute_node[i][j].in[1]) = { address(j + COL_OFFSET,i,WTS_PING), address(j + COL_OFFSET,i,WTS_PONG) }; //WTS ping-pong double buffers placed 16kB apart in two banks
                    location<buffer>(compute_node[i][j].out[0]) = { address(j + COL_OFFSET,i,OFM_PING), address(j + COL_OFFSET,i,OFM_PONG) };  //OFM ping-pong double buffers placed 16kB apart in two banks
                    location<stack>(compute_node[i][j]) = { address(j + COL_OFFSET,i,STK_OFFSET) };

                    location<dma>(compute_node[i][j].in[0]) = dma_channel(aie_tile, j + COL_OFFSET, i, 0); //s2mm0
                    location<dma>(compute_node[i][j].out[0]) = dma_channel(aie_tile, j + COL_OFFSET, i, 0); //mm2s0
                    location<dma>(compute_node[i][j].in[1]) = dma_channel(aie_tile, j + COL_OFFSET, i, 1); //s2mm1

                    if (j == 0) {
                        read_access(wts.out[i]) = wts_pattern[i];
                        location<dma>(wts.out[i]) = dma_channel(memory_tile, 3 + COL_OFFSET, 0, i);
                    }
                }
                connect<pktstream>(ofm_merge[j].out[0], ofm.in[j]);
                write_access(ofm.in[j]) = ofm_pattern[j];
                location<dma>(ofm.in[j]) = dma_channel(memory_tile, 1 + COL_OFFSET, 0, j);
            }

            location<dma>(wts.in[0]) = dma_channel(memory_tile, 3 + COL_OFFSET, 0, 0);
            connect(wts_ddr.out[0], wts.in[0]);
            read_access(wts_ddr.out[0]) = wts_ddr_read_pattern;
            write_access(wts.in[0]) = wts_ddr_write_pattern;
            location<dma>(wts_ddr.out[0]) = dma_channel(shim_tile, 3 + COL_OFFSET, 0, 0);
        }

        void connect_layer(shared_buffer<uint8>& ifm, std::vector<access_pattern> ifm_pattern[], uint32_t ifmsv_size)
        {
            for (int j = 0; j < NUM_COLS; j++) {
                for (int i = 0; i < NUM_ROWS; i++) {
                    connect(ifm.out[j], compute_node[i][j].in[0]);
                    dimensions(compute_node[i][j].in[0]) = { ifmsv_size };
                }
                read_access(ifm.out[j]) = ifm_pattern[j];
                location<dma>(ifm.out[j]) = dma_channel(memory_tile, 1 + COL_OFFSET, 0, j);
            }
        }
    };

};

#endif // __LAYER_OVERLAY_H__

