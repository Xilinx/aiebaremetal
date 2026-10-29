// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022-2023 Advanced Micro Devices, Inc. All Rights Reserved.
#pragma once
#include "adf.h"
#include "multi_layer_resnet18.h"

#ifndef NUM_INST
    #define NUM_INST 10  //defualt run one instance
#endif

#ifndef COL_OFFSET
    #define COL_OFFSET 0
#endif
#ifndef ROW_OFFSET
    #define ROW_OFFSET 0
#endif
#ifndef MEM_ROW
    #define MEM_ROW 0
#endif
#ifndef SHIM_COL
    #define SHIM_COL 2
#endif
#ifndef SHIM_CH
    #define SHIM_CH 0
#endif
#ifndef SM_COL
    #define SM_COL 7
#endif
#ifndef SM_ROW
    #define SM_ROW 0
#endif

std::vector<uint32_t> shim_loc = {15,22}; //shim locations used by 4 channel mm2s shim dma. 

// Co-ordinates
/*
    {0, 0, 0, 2, 7, 0},
    {0, 4, 1, 3, 7, 1},
    {8, 0, 0, 6, 7, 2},
    {8, 4, 1, 7, 15, 0},
    {16, 0, 0, 14, 15, 1},
    {16, 4, 1, 23, 15, 2},
    {23, 0, 0, 30, 30, 0},
    {23, 4, 1, 31, 30, 1},
    {31, 0, 0, 34, 30, 2},
    {31, 4, 1, 35, 30, 3}
*/

using namespace adf ;

class TopLevel : public graph
{
public:
	ResNet18v1<COL_OFFSET, ROW_OFFSET, MEM_ROW, SHIM_COL, SHIM_CH, SM_COL, SM_ROW> resnet18_0;
#if NUM_INST == 10
	ResNet18v1<0,4,1,3,0,7,1> resnet18_1;
	ResNet18v1<8,0,0,6,0,7,2> resnet18_2;
	ResNet18v1<8,4,1,7,0,15,0> resnet18_3;
	ResNet18v1<16,0,0,14,0,15,1> resnet18_4;
	ResNet18v1<16,4,1,23,0,15,2> resnet18_5;
	ResNet18v1<23,0,0,30,0,30,0> resnet18_6;
	ResNet18v1<23,4,1,31,0,30,1> resnet18_7;
	ResNet18v1<31,0,0,34,0,30,2> resnet18_8;
	ResNet18v1<31,4,1,35,0,30,3> resnet18_9;
#endif	

	external_buffer<uint8_t> wts_ddr[NUM_LAYERS];

	TopLevel(){
		//wts_ddr buffer shim dma loc and read pattern
		for(int i=0; i<NUM_LAYERS; i++){
			wts_ddr[i] = external_buffer<uint8_t>::create(res_tiling[i].wts_ddr_dim, 0, NUM_DMA_CH);
			for(int j=0; j<NUM_DMA_CH; j++){
				int shim_col = shim_loc[j>>1];
				read_access(wts_ddr[i].out[j]) = res_tiling[i].wts_ddr_read_pattern[j];
				location<dma>(wts_ddr[i].out[j]) = dma_channel(shim_tile, shim_col, 0, j%2);
			}
		}

		//calling member function of single layer overlay for each resnet to connect wts buffer to wts_ddr and write pattern
		for(int i=0; i<NUM_LAYERS; i++){
		    resnet18_0.resnet_layers[i].connect_wts(wts_ddr[i],res_tiling[i].wts_ddr_write_pattern);
#if NUM_INST == 10
		    resnet18_1.resnet_layers[i].connect_wts(wts_ddr[i],res_tiling[i].wts_ddr_write_pattern);
		    resnet18_2.resnet_layers[i].connect_wts(wts_ddr[i],res_tiling[i].wts_ddr_write_pattern);
		    resnet18_3.resnet_layers[i].connect_wts(wts_ddr[i],res_tiling[i].wts_ddr_write_pattern);
		    resnet18_4.resnet_layers[i].connect_wts(wts_ddr[i],res_tiling[i].wts_ddr_write_pattern);
		    resnet18_5.resnet_layers[i].connect_wts(wts_ddr[i],res_tiling[i].wts_ddr_write_pattern);
		    resnet18_6.resnet_layers[i].connect_wts(wts_ddr[i],res_tiling[i].wts_ddr_write_pattern);
		    resnet18_7.resnet_layers[i].connect_wts(wts_ddr[i],res_tiling[i].wts_ddr_write_pattern);
		    resnet18_8.resnet_layers[i].connect_wts(wts_ddr[i],res_tiling[i].wts_ddr_write_pattern);
		    resnet18_9.resnet_layers[i].connect_wts(wts_ddr[i],res_tiling[i].wts_ddr_write_pattern);
#endif
		}

	}

};
