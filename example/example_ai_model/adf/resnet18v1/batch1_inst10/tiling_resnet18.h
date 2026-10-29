// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
////////////////////////////////////////////////////////////////////////
//This is per layer parameters
//FlexML can choose to generate this in a separate header file
////////////////////////////////////////////////////////////////////////

#pragma once
#include <adf.h>
#include "overlay_config.h"

//Layer1
//ifm -> {4, 224, 1, 224}
//ofm -> {8, 64, 8, 56}
using namespace adf ;

uint32_t m1_delta0 = (512*1024-224*1024+32*1024*3);
uint32_t m2_delta0 = (32*1024*3);
uint32_t m2_delta1 = (224*1024+32*1024*3);
uint32_t m2_delta00 = m2_delta0+112*1024;
uint32_t m2_delta10 = m2_delta1+112*1024;

class resnet18_tiling {
   public:
      std::vector<access_pattern> ifm_pattern[NUM_COLS];
      std::vector<access_pattern> wts_pattern[NUM_ROWS];
      std::vector<access_pattern> ofm_pattern[NUM_COLS];
      std::vector<access_pattern> ifm2_pattern[NUM_COLS];
      std::vector<access_pattern> wts_ddr_read_pattern;
      std::vector<access_pattern> wts_ddr_write_pattern;
      std::vector<uint32_t> wts_ddr_dim;
      uint32_t ifmsv_size, ifm2sv_size, wtssv_size, ofmsv_size, kernel_iter, ofm_repetition;

   resnet18_tiling(const int filt_k, const std::vector<uint32_t>& ifm_dim, const std::vector<uint32_t>& ifmsv_dim,
		   const std::vector<traversing_parameters> ifm_access_pattern, const std::vector<uint32_t>& ifm_boundary_dim,
		   const std::vector<uint32_t>& wts_dim, const std::vector<uint32_t>& wtssv_dim,
		   const std::vector<uint32_t>& ofm_dim, const std::vector <uint32_t>& ofmsv_dim, const std::vector<traversing_parameters> ofm_access_pattern,
		   const std::vector<uint32_t>& ifm2sv_dim, const std::vector<traversing_parameters> ifm2_access_pattern = {}, const std::vector<uint32_t> channel_split = {},
		   const bool fused_gap=false, const bool is_fc=false, const std::vector<uint32_t>& ifm2_dim = {}, const uint32_t num_ph=1, const int ph_stride=0)
   {
       uint32_t height_iter, width_iter, depth_iter, channel_iter, sub_channel_iter;
       uint32_t rate = 1;
       int32_t ifm_fc_offset = 64*(NUM_COLS-1);
       wtssv_size = wtssv_dim[0];
       ofmsv_size = ofmsv_dim[0]*ofmsv_dim[1]*ofmsv_dim[2]*ofmsv_dim[3];
       ifmsv_size = ifmsv_dim[0]*ifmsv_dim[1]*ifmsv_dim[2]*ifmsv_dim[3];
       int extra_pad = (filt_k==7)?2:0;

       if(is_fc){
          height_iter = 1; width_iter = 1; depth_iter = 1;
          channel_iter = ofm_dim[2]/ofmsv_dim[2]/NUM_ROWS;
       }else{
          height_iter = ofm_dim[3]/ofmsv_dim[3]/NUM_COLS; width_iter = ofm_dim[1]/ofmsv_dim[1]; depth_iter = ifm_dim[2]/ifmsv_dim[2]; channel_iter = ofm_dim[2]/ofmsv_dim[2]/NUM_ROWS;
          rate = ifm_boundary_dim[3]/ofm_dim[3];
       }

       int channel_offset=0;
       if(!channel_split.empty()){
          channel_iter = channel_iter/channel_split[0];
	  channel_offset = channel_split[1];
       }
       kernel_iter = height_iter*width_iter*depth_iter*channel_iter;
       ofm_repetition = depth_iter;

       std::vector<traversing_parameters> wts_access_pattern;
       uint32_t ifm_repetition = channel_iter;
       uint32_t super_iter = is_fc?1:(wtssv_dim[0]*depth_iter*NUM_ROWS*channel_iter/wts_dim[0]);
       sub_channel_iter = is_fc?1:ceil(channel_iter/super_iter);
       std::vector<uint32_t> wts_buffer_dim = {wts_dim[0]/sub_channel_iter, sub_channel_iter};
       std::vector<uint32_t> ofm_dim_int = ofm_dim; std::vector<uint32_t> ofmsv_dim_int = ofmsv_dim;
       std::vector<uint32_t> ifm2_dim_int = ofm_dim;

       wts_ddr_dim = {wts_dim[0]*super_iter};
#if NUM_DMA_CH == 1
       wts_ddr_read_pattern.push_back(tiling({.buffer_dimension = {wts_dim[0]*super_iter}, .tiling_dimension = {wts_dim[0]*super_iter}, .offset = {0}}));
       wts_ddr_write_pattern.push_back(tiling({.buffer_dimension = wts_dim, .tiling_dimension = wts_dim, .offset = {0}}));
#elif NUM_DMA_CH == 2
       wts_ddr_read_pattern.push_back(tiling({.buffer_dimension = {wts_dim[0]*super_iter}, .tiling_dimension = {wts_dim[0]/2}, .offset = {0},
				      .tile_traversal={{.dimension=0, .stride=wts_dim[0], .wrap=super_iter}}, .packet_port_id = -1, .repetition = 1}));

       wts_ddr_read_pattern.push_back(tiling({.buffer_dimension = {wts_dim[0]*super_iter}, .tiling_dimension = {wts_dim[0]/2}, .offset = {int32_t(wts_dim[0]/2)},
				      .tile_traversal={{.dimension=0, .stride=wts_dim[0], .wrap=super_iter}}, .packet_port_id = -1, .repetition = 1}));

       wts_ddr_write_pattern.push_back(tiling({.buffer_dimension = wts_dim, .tiling_dimension = {wts_dim[0]/2}, .offset = {0}}));
       wts_ddr_write_pattern.push_back(tiling({.buffer_dimension = wts_dim, .tiling_dimension = {wts_dim[0]/2}, .offset = {int32_t(wts_dim[0]/2)}}));
#elif NUM_DMA_CH == 4 // TODO: verify that this works for above 2 cases and then remove code above
       for(int i=0; i<NUM_DMA_CH; i++){
	    wts_ddr_read_pattern.push_back(tiling({.buffer_dimension = {wts_dim[0]*super_iter}, .tiling_dimension = {wts_dim[0]/NUM_DMA_CH}, .offset = {int32_t(i*wts_dim[0]/NUM_DMA_CH)},
				      .tile_traversal={{.dimension=0, .stride=wts_dim[0], .wrap=super_iter}}, .packet_port_id = -1, .repetition = 1}));
	    wts_ddr_write_pattern.push_back(tiling({.buffer_dimension = wts_dim, .tiling_dimension = {wts_dim[0]/NUM_DMA_CH}, .offset = {int32_t(i*wts_dim[0]/NUM_DMA_CH)}}));
	}
#endif

       if(sub_channel_iter>1)
          wts_access_pattern = {{.dimension = 0,.stride = wtssv_size,.wrap = ofm_repetition}, {.dimension = 1, .stride = 1, .wrap = sub_channel_iter}};
       else
          wts_access_pattern = {{.dimension = 0,.stride = wtssv_size,.wrap = is_fc?channel_iter:ofm_repetition}};

       if(ifm2sv_dim.empty()){
          ifm2sv_size = 0;
       }else{
          ifm2sv_size = ifm2sv_dim[0]*ifm2sv_dim[1]*ifm2sv_dim[2]*ifm2sv_dim[3];
	  ifm_repetition = 1;  //TODO: ifm1/ifm2 interleaving doesn't support task repeat
	  if(!ifm2_dim.empty())
             ifm2_dim_int = ifm2_dim;
       }

       if(fused_gap){
          ofm_dim_int[1]=1; //Global Avgpool
          ofmsv_dim_int[1]=1; //Global Avgpool
	  ofmsv_size = ofmsv_size/ofmsv_dim[1];
       }

       for(int i=0; i<NUM_COLS; i++){
	  for(uint32_t k=0; k<num_ph; k++){
	     //IFM
             ifm_pattern[i].push_back(tiling({.buffer_dimension = ifm_dim, .tiling_dimension = ifmsv_dim,
					      .offset = {0, is_fc?(ifm_fc_offset+(int)(i*ifmsv_dim[1])):(-((filt_k-1)>>1)-extra_pad+(int)k*ph_stride), 0,
					                 -((filt_k-1)>>1)+(int)(i*height_iter*((is_fc?0:ifmsv_dim[3])-filt_k+1-extra_pad)-extra_pad)},
					      .tile_traversal = ifm_access_pattern, .packet_port_id = -1, .repetition = ifm_repetition, .boundary_dimension = ifm_boundary_dim, .phase=k}));
             //OFM
	     for(int j=0; j<NUM_ROWS; j++)
	        ofm_pattern[i].push_back(tiling({.buffer_dimension = ofm_dim_int, .tiling_dimension = ofmsv_dim_int,
					         .offset = {0, (int)(k*ph_stride/rate), (int)(j*ofmsv_dim[2]*(is_fc?channel_iter:1))+channel_offset, (int)(i*ofmsv_dim[3]*height_iter)},
                                                 .tile_traversal = ofm_access_pattern, .packet_port_id = j, .repetition=1, .boundary_dimension = ofm_dim_int, .phase=k}));
	     //IFM2
	     if(ifm2sv_size>0)
	        ifm2_pattern[i].push_back(tiling({.buffer_dimension = ifm2_dim_int, .tiling_dimension = ifm2sv_dim, .offset = {0, (int)k*ph_stride, channel_offset, (i*(int)(ifm2sv_dim[3]))},
                                                  .tile_traversal = ifm2_access_pattern, .packet_port_id = -1,.repetition = 1, .boundary_dimension = ifm_boundary_dim, .phase=k}));
          }
       }
       //WTS
       // wtssv, offset need to have same dimension as wts_buffer_dim, refer to CR-1141299. Thats why increasing dimension to 2 using push_back
       std::vector<uint32_t> wtssv_tile;
       wtssv_tile = {wtssv_dim[0],1};

       for(int i=0; i<NUM_ROWS; i++){
	  std::vector<int32_t> wts_offset;
          if(is_fc)
	     wts_offset = {(int32_t)(i*wtssv_size*channel_iter)};
          else
	     wts_offset = {(int32_t)(i*wtssv_size*ofm_repetition)};

          wts_offset.push_back(0);

          wts_pattern[i] = {tiling({.buffer_dimension = wts_buffer_dim, .tiling_dimension = wtssv_tile, .offset = wts_offset,
                                    .tile_traversal = wts_access_pattern, .packet_port_id = -1, .repetition = width_iter*height_iter, .boundary_dimension = wts_buffer_dim, .phase=0})};
       }
   }
};


//Layer1
uint32_t filt_k7 = 7;
std::vector<uint32_t> ifm_dim_1 = {4, 224, 1, 224}; std::vector<uint32_t> ifm_boundary_dim_1 = ifm_dim_1;
std::vector<uint32_t> wts_dim_1 = {3776*4};
std::vector<uint32_t> ofm_dim_1 = {8, 56, 8, 56};
//ifmsv: width-> 3+2*(1+112)+3=232, height-> 3+2*(1+2)+3=12
std::vector<uint32_t> ifmsv_dim_1 = {4, 232, 1, 12}; std::vector<uint32_t> wtssv_dim_1 = {3776}; std::vector<uint32_t> ofmsv_dim_1 = {8, 56, 2, 1};
uint32_t width_iter_1 = 1; uint32_t depth_iter_1 = 1; uint32_t height_iter_1 = 8; uint32_t channel_iter_1 = 1;
uint32_t extra_pad = 2;


std::vector<traversing_parameters> ifm_access_pattern_1 = {{.dimension=3, .stride=(ifmsv_dim_1[3]-filt_k7+1-extra_pad), .wrap=height_iter_1}};
std::vector<traversing_parameters> ofm_access_pattern_1 = {{.dimension=3, .stride=ofmsv_dim_1[3], .wrap=height_iter_1}};

resnet18_tiling layer1 = resnet18_tiling((int)filt_k7, ifm_dim_1, ifmsv_dim_1, ifm_access_pattern_1, ifm_boundary_dim_1, wts_dim_1, wtssv_dim_1, ofm_dim_1, ofmsv_dim_1, ofm_access_pattern_1, {});

//Layer2
uint32_t filt_k = 3; uint32_t width_pad = 2;
std::vector<uint32_t> ifm_dim_2 = {8, 56, 8, 56}; std::vector<uint32_t> ifm_boundary_dim_2 = {8, 56, 8, 56};
std::vector<uint32_t> wts_dim_2 = {2496*4*4};
std::vector<uint32_t> ofm_dim_2 = {8, 64, 8, 56};
std::vector<uint32_t> ifmsv_dim_2 = {8, 36, 2, 10}; std::vector<uint32_t> wtssv_dim_2 = {2496}; std::vector<uint32_t> ofmsv_dim_2 = {8, 32, 2, 8};
uint32_t width_iter_2 = 2; uint32_t depth_iter_2 = 4; uint32_t height_iter_2 = 1; uint32_t channel_iter_2 = 1;

std::vector<traversing_parameters> ifm_access_pattern_2 = {{.dimension=2, .stride=ifmsv_dim_2[2], .wrap=depth_iter_2}, {.dimension=1, .stride=(ifmsv_dim_2[1]-filt_k+1-width_pad), .wrap=width_iter_2}};
std::vector<traversing_parameters> ofm_access_pattern_2 = {{.dimension=1, .stride=ofmsv_dim_2[1], .wrap=width_iter_2}};

resnet18_tiling layer2 = resnet18_tiling((int)filt_k, ifm_dim_2, ifmsv_dim_2, ifm_access_pattern_2, ifm_boundary_dim_2, wts_dim_2, wtssv_dim_2, ofm_dim_2, ofmsv_dim_2, ofm_access_pattern_2, {});

//Layer3
std::vector<uint32_t> ifm_dim_3 = ofm_dim_2; std::vector<uint32_t> ifm_boundary_dim_3 = {8, 56, 8, 56};
std::vector<uint32_t> wts_dim_3 = wts_dim_2;
std::vector<uint32_t> ofm_dim_3 = ofm_dim_2;
std::vector<uint32_t> ifmsv_dim_3 = {8, 20, 2, 10}; std::vector<uint32_t> wtssv_dim_3 = {2496}; std::vector<uint32_t> ofmsv_dim_3 = {8, 16, 2, 8};
uint32_t width_iter_3 = 4; uint32_t depth_iter_3 = 4; uint32_t height_iter_3 = 1; uint32_t channel_iter_3 = 1;

std::vector<traversing_parameters> ifm_access_pattern_3 = {{.dimension=2, .stride=ifmsv_dim_3[2], .wrap=depth_iter_3}, {.dimension=1, .stride=(ifmsv_dim_3[1]-filt_k+1-width_pad), .wrap=width_iter_3}};
std::vector<traversing_parameters> ofm_access_pattern_3 = {{.dimension=1, .stride=ofmsv_dim_3[1], .wrap=width_iter_3}};

std::vector<uint32_t> ifm2_dim_3 = {8, 56, 8, 56};
std::vector<uint32_t> ifm2sv_dim_3 = {8, 16, 8, 8};
std::vector<traversing_parameters> ifm2_access_pattern_3 = {{.dimension=1, .stride=ofmsv_dim_3[1], .wrap=width_iter_3}};

resnet18_tiling layer3 = resnet18_tiling((int)filt_k, ifm_dim_3, ifmsv_dim_3, ifm_access_pattern_3, ifm_boundary_dim_3, wts_dim_3, wtssv_dim_3, ofm_dim_3, ofmsv_dim_3, ofm_access_pattern_3,
		                          ifm2sv_dim_3, ifm2_access_pattern_3, {}, false, false, ifm2_dim_3);

//Layer4
std::vector<uint32_t> ifm_dim_4 = ofm_dim_3; std::vector<uint32_t> ifm_boundary_dim_4 = ifm_boundary_dim_2;
std::vector<uint32_t> wts_dim_4 = wts_dim_2;
std::vector<uint32_t> ofm_dim_4 = ofm_dim_2;
std::vector<uint32_t> ifmsv_dim_4 = ifmsv_dim_2; std::vector<uint32_t> wtssv_dim_4 = {2496}; std::vector<uint32_t> ofmsv_dim_4 = {8, 32, 2, 8};
uint32_t width_iter_4 = 2; uint32_t depth_iter_4 = 4; uint32_t height_iter_4 = 1; uint32_t channel_iter_4 = 1;

std::vector<traversing_parameters> ifm_access_pattern_4 = {{.dimension=2, .stride=ifmsv_dim_4[2], .wrap=depth_iter_4}, {.dimension=1, .stride=(ifmsv_dim_4[1]-filt_k+1-width_pad), .wrap=width_iter_4}};
std::vector<traversing_parameters> ofm_access_pattern_4 = {{.dimension=1, .stride=ofmsv_dim_4[1], .wrap=width_iter_4}};

resnet18_tiling layer4 = resnet18_tiling((int)filt_k, ifm_dim_4, ifmsv_dim_4, ifm_access_pattern_4, ifm_boundary_dim_4, wts_dim_4, wtssv_dim_4, ofm_dim_4, ofmsv_dim_4, ofm_access_pattern_4, {});


//Layer5
std::vector<uint32_t> ifm_dim_5 = ofm_dim_4; std::vector<uint32_t> ifm_boundary_dim_5 = ifm_boundary_dim_4;
std::vector<uint32_t> wts_dim_5 = wts_dim_4;
std::vector<uint32_t> ofm_dim_5 = ofm_dim_4;
std::vector<uint32_t> ifmsv_dim_5 = {8, 20, 2, 10}; std::vector<uint32_t> wtssv_dim_5 = {2496}; std::vector<uint32_t> ofmsv_dim_5 = {8, 16, 2, 8};
uint32_t width_iter_5 = 4; uint32_t depth_iter_5 = 4; uint32_t height_iter_5 = 1; uint32_t channel_iter_5 = 1;

std::vector<traversing_parameters> ifm_access_pattern_5 = {{.dimension=2, .stride=ifmsv_dim_5[2], .wrap=depth_iter_5}, {.dimension=1, .stride=(ifmsv_dim_5[1]-filt_k+1-width_pad), .wrap=width_iter_5}};
std::vector<traversing_parameters> ofm_access_pattern_5 = {{.dimension=1, .stride=ofmsv_dim_5[1], .wrap=width_iter_5}};

std::vector<uint32_t> ifm2sv_dim_5 = {8, 16, 8, 8};
std::vector<traversing_parameters> ifm2_access_pattern_5 = {{.dimension=1, .stride=ofmsv_dim_5[1], .wrap=width_iter_5}};

resnet18_tiling layer5 = resnet18_tiling((int)filt_k, ifm_dim_5, ifmsv_dim_5, ifm_access_pattern_5, ifm_boundary_dim_5, wts_dim_5, wtssv_dim_5, ofm_dim_5, ofmsv_dim_5, ofm_access_pattern_5,
		                          ifm2sv_dim_5, ifm2_access_pattern_5);

//Layer6
std::vector<uint32_t> ifm_dim_6 = ofm_dim_5; std::vector<uint32_t> ifm_boundary_dim_6 = ifm_boundary_dim_5;
std::vector<uint32_t> wts_dim_6 = {2624*8*4};
std::vector<uint32_t> ofm_dim_6 = {8, 32, 16, 28};
std::vector<uint32_t> ifmsv_dim_6 = {8, 68, 1, 10}; std::vector<uint32_t> wtssv_dim_6 = {2624}; std::vector<uint32_t> ofmsv_dim_6 = {8, 32, 4, 4};
uint32_t width_iter_6 = 1; uint32_t depth_iter_6 = 8; uint32_t height_iter_6 = 1; uint32_t channel_iter_6 = 1;

std::vector<traversing_parameters> ifm_access_pattern_6 = {{.dimension=2, .stride=ifmsv_dim_6[2], .wrap=depth_iter_6}};
std::vector<traversing_parameters> ofm_access_pattern_6 = {};

resnet18_tiling layer6 = resnet18_tiling((int)filt_k, ifm_dim_6, ifmsv_dim_6, ifm_access_pattern_6, ifm_boundary_dim_6, wts_dim_6, wtssv_dim_6, ofm_dim_6, ofmsv_dim_6, ofm_access_pattern_6, {});


//Layer6a
std::vector<uint32_t> ifm_dim_6a = ofm_dim_5; std::vector<uint32_t> ifm_boundary_dim_6a = ifm_boundary_dim_5;
std::vector<uint32_t> wts_dim_6a = {1344*2*4};
std::vector<uint32_t> ofm_dim_6a = {8, 32, 16, 28};
std::vector<uint32_t> ifmsv_dim_6a = {8, 32, 4, 8}; std::vector<uint32_t> wtssv_dim_6a = {1344}; std::vector<uint32_t> ofmsv_dim_6a = {8, 16, 4, 4};
uint32_t width_iter_6a = 2; uint32_t depth_iter_6a = 2; uint32_t height_iter_6a = 1; uint32_t channel_iter_6a = 1;

std::vector<traversing_parameters> ifm_access_pattern_6a = {{.dimension=2, .stride=ifmsv_dim_6a[2], .wrap=depth_iter_6a}, {.dimension=1, .stride=ifmsv_dim_6a[1], .wrap=width_iter_6a}};
std::vector<traversing_parameters> ofm_access_pattern_6a = {{.dimension=1, .stride=ofmsv_dim_6a[1], .wrap=width_iter_6a}};

resnet18_tiling layer6a = resnet18_tiling(1, ifm_dim_6a, ifmsv_dim_6a, ifm_access_pattern_6a, ifm_boundary_dim_6a, wts_dim_6a, wtssv_dim_6a, ofm_dim_6a, ofmsv_dim_6a, ofm_access_pattern_6a, {});

//Layer7
std::vector<uint32_t> ifm_dim_7 = ofm_dim_6; std::vector<uint32_t> ifm_boundary_dim_7 = {8, 28, 16, 28};
std::vector<uint32_t> wts_dim_7 = {4928*8*4};
std::vector<uint32_t> ofm_dim_7 = {8, 32, 16, 28};
std::vector<uint32_t> ifmsv_dim_7 = {8, 20, 2, 6}; std::vector<uint32_t> wtssv_dim_7 = {4928}; std::vector<uint32_t> ofmsv_dim_7 = {8, 16, 4, 4};
uint32_t width_iter_7 = 2; uint32_t depth_iter_7 = 8; uint32_t height_iter_7 = 1; uint32_t channel_iter_7 = 1;

std::vector<traversing_parameters> ifm_access_pattern_7 = {{.dimension=2, .stride=ifmsv_dim_7[2], .wrap=depth_iter_7}, {.dimension=1, .stride=(ifmsv_dim_7[1]-filt_k+1-width_pad), .wrap=width_iter_7}};
std::vector<traversing_parameters> ofm_access_pattern_7 = {{.dimension=1, .stride=ofmsv_dim_7[1], .wrap=width_iter_7}};

std::vector<uint32_t> ifm2sv_dim_7 = {8, 16, 16, 4};
std::vector<traversing_parameters> ifm2_access_pattern_7 = {{.dimension=1, .stride=ofmsv_dim_7[1], .wrap=width_iter_7}};

resnet18_tiling layer7 = resnet18_tiling((int)filt_k, ifm_dim_7, ifmsv_dim_7, ifm_access_pattern_7, ifm_boundary_dim_7, wts_dim_7, wtssv_dim_7, ofm_dim_7, ofmsv_dim_7, ofm_access_pattern_7,
		                          ifm2sv_dim_7, ifm2_access_pattern_7);

//Layer8
std::vector<uint32_t> ifm_dim_8 = ofm_dim_7; std::vector<uint32_t> ifm_boundary_dim_8 = ifm_boundary_dim_7;
std::vector<uint32_t> wts_dim_8 = wts_dim_7;
std::vector<uint32_t> ofm_dim_8 = ofm_dim_7;
std::vector<uint32_t> ifmsv_dim_8 = {8, 36, 2, 6}; std::vector<uint32_t> wtssv_dim_8 = {4928}; std::vector<uint32_t> ofmsv_dim_8 = {8, 32, 4, 4};
uint32_t width_iter_8 = 1; uint32_t depth_iter_8 = 8; uint32_t height_iter_8 = 1; uint32_t channel_iter_8 = 1;

std::vector<traversing_parameters> ifm_access_pattern_8 = {{.dimension=2, .stride=ifmsv_dim_8[2], .wrap=depth_iter_8}};
std::vector<traversing_parameters> ofm_access_pattern_8 = {};

resnet18_tiling layer8 = resnet18_tiling((int)filt_k, ifm_dim_8, ifmsv_dim_8, ifm_access_pattern_8, ifm_boundary_dim_8, wts_dim_8, wtssv_dim_8, ofm_dim_8, ofmsv_dim_8, ofm_access_pattern_8, {});

//Layer9
std::vector<uint32_t> ifm_dim_9 = ofm_dim_8; std::vector<uint32_t> ifm_boundary_dim_9 = {8, 28, 16, 28};
std::vector<uint32_t> wts_dim_9 = {4928*8*4};
std::vector<uint32_t> ofm_dim_9 = {8, 32, 16, 28};
std::vector<uint32_t> ifmsv_dim_9 = {8, 20, 2, 6}; std::vector<uint32_t> wtssv_dim_9 = {4928}; std::vector<uint32_t> ofmsv_dim_9 = {8, 16, 4, 4};
uint32_t width_iter_9 = 2; uint32_t depth_iter_9 = 8; uint32_t height_iter_9 = 1; uint32_t channel_iter_9 = 1;

std::vector<traversing_parameters> ifm_access_pattern_9 = {{.dimension=2, .stride=ifmsv_dim_9[2], .wrap=depth_iter_9}, {.dimension=1, .stride=(ifmsv_dim_9[1]-filt_k+1-width_pad), .wrap=width_iter_9}};
std::vector<traversing_parameters> ofm_access_pattern_9 = {{.dimension=1, .stride=ofmsv_dim_9[1], .wrap=width_iter_9}};

std::vector<uint32_t> ifm2sv_dim_9 = {8, 16, 16, 4};
std::vector<traversing_parameters> ifm2_access_pattern_9 = {{.dimension=1, .stride=ofmsv_dim_9[1], .wrap=width_iter_9}};

resnet18_tiling layer9 = resnet18_tiling((int)filt_k, ifm_dim_9, ifmsv_dim_9, ifm_access_pattern_9, ifm_boundary_dim_9, wts_dim_9, wtssv_dim_9, ofm_dim_9, ofmsv_dim_9, ofm_access_pattern_9,
		                          ifm2sv_dim_9, ifm2_access_pattern_9);

//Layer10
uint32_t width_iter_10 = 1; uint32_t depth_iter_10 = 8; uint32_t height_iter_10 = 1; uint32_t channel_iter_10 = 2;
std::vector<uint32_t> ifm_dim_10 = ofm_dim_9; std::vector<uint32_t> ifm_boundary_dim_10 = ifm_boundary_dim_9;
std::vector<uint32_t> wts_dim_10 = {4928*depth_iter_10*NUM_ROWS*channel_iter_10};
std::vector<uint32_t> ofm_dim_10 = {8, 16, 32, 14};
std::vector<uint32_t> ifmsv_dim_10 = {8, 36, 2, 6}; std::vector<uint32_t> wtssv_dim_10 = {4928}; std::vector<uint32_t> ofmsv_dim_10 = {8, 16, 4, 2};

std::vector<traversing_parameters> ifm_access_pattern_10 = {{.dimension=2, .stride=ifmsv_dim_10[2], .wrap=depth_iter_10}};
std::vector<traversing_parameters> ofm_access_pattern_10 = {{.dimension=2, .stride=ofmsv_dim_10[2]*NUM_ROWS, .wrap=channel_iter_10}};

resnet18_tiling layer10 = resnet18_tiling((int)filt_k, ifm_dim_10, ifmsv_dim_10, ifm_access_pattern_10, ifm_boundary_dim_10, wts_dim_10, wtssv_dim_10, ofm_dim_10, ofmsv_dim_10, ofm_access_pattern_10, {});

//Layer10a
uint32_t width_iter_10a = 1; uint32_t depth_iter_10a = 4; uint32_t height_iter_10a = 1; uint32_t channel_iter_10a = 1;
std::vector<uint32_t> ifm_dim_10a = ofm_dim_9; std::vector<uint32_t> ifm_boundary_dim_10a = ifm_boundary_dim_9;
std::vector<uint32_t> wts_dim_10a = {2624*depth_iter_10a*NUM_ROWS*channel_iter_10a};
std::vector<uint32_t> ofm_dim_10a = {8, 16, 32, 14};
std::vector<uint32_t> ifmsv_dim_10a = {8, 32, 4, 4}; std::vector<uint32_t> wtssv_dim_10a = {2624}; std::vector<uint32_t> ofmsv_dim_10a = {8, 16, 8, 2};

std::vector<traversing_parameters> ifm_access_pattern_10a = {{.dimension=2, .stride=ifmsv_dim_10a[2], .wrap=depth_iter_10a}};
std::vector<traversing_parameters> ofm_access_pattern_10a = {};

resnet18_tiling layer10a = resnet18_tiling(1, ifm_dim_10a, ifmsv_dim_10a, ifm_access_pattern_10a, ifm_boundary_dim_10a, wts_dim_10a, wtssv_dim_10a, ofm_dim_10a, ofmsv_dim_10a, ofm_access_pattern_10a, {});

//Layer11
uint32_t width_iter_11 = 1; uint32_t depth_iter_11 = 16; uint32_t height_iter_11 = 1; uint32_t channel_iter_11 = 2; uint32_t super_iter_11 = 2;
std::vector<uint32_t> ifm_dim_11 = ofm_dim_10; std::vector<uint32_t> ifm_boundary_dim_11 = {8, 14, 32, 14};
std::vector<uint32_t> wts_ddr_dim_11 = {4928*depth_iter_11*NUM_ROWS*channel_iter_11};
std::vector<uint32_t> wts_dim_11 = {4928*depth_iter_11*NUM_ROWS*channel_iter_11/super_iter_11};
std::vector<uint32_t> ofm_dim_11 = {8, 16, 32, 14};
std::vector<uint32_t> ifmsv_dim_11 = {8, 20, 2, 4}; std::vector<uint32_t> wtssv_dim_11 = {4928}; std::vector<uint32_t> ofmsv_dim_11 = {8, 16, 4, 2};

std::vector<traversing_parameters> ifm_access_pattern_11 = {{.dimension=2, .stride=ifmsv_dim_11[2], .wrap=depth_iter_11}, {.dimension=1, .stride=0, .wrap=channel_iter_11}};
std::vector<traversing_parameters> ofm_access_pattern_11 = {{.dimension=2, .stride=ofmsv_dim_11[2]*NUM_ROWS, .wrap=channel_iter_11}};

std::vector<uint32_t> ifm2sv_dim_11 = {8, 16, 16, 2};
std::vector<traversing_parameters> ifm2_access_pattern_11 = {{.dimension=2, .stride=ofmsv_dim_11[2]*NUM_ROWS, .wrap=channel_iter_11}};

resnet18_tiling layer11 = resnet18_tiling((int)filt_k, ifm_dim_11, ifmsv_dim_11, ifm_access_pattern_11, ifm_boundary_dim_11, wts_dim_11, wtssv_dim_11, ofm_dim_11, ofmsv_dim_11, ofm_access_pattern_11,
		                          ifm2sv_dim_11, ifm2_access_pattern_11);

//Layer12
uint32_t width_iter_12 = 1; uint32_t depth_iter_12 = 16; uint32_t height_iter_12 = 1; uint32_t channel_iter_12 = 2; uint32_t super_iter_12 = 2;
std::vector<uint32_t> ifm_dim_12 = ofm_dim_11; std::vector<uint32_t> ifm_boundary_dim_12 = {8, 14, 32, 14};
std::vector<uint32_t> wts_ddr_dim_12 = {4928*depth_iter_12*NUM_ROWS*channel_iter_12};
std::vector<uint32_t> wts_dim_12 = {4928*depth_iter_12*NUM_ROWS*channel_iter_12/super_iter_12};
std::vector<uint32_t> ofm_dim_12 = {8, 16, 32, 14};
std::vector<uint32_t> ifmsv_dim_12 = {8, 20, 2, 4}; std::vector<uint32_t> wtssv_dim_12 = {4928}; std::vector<uint32_t> ofmsv_dim_12 = {8, 16, 4, 2};

std::vector<traversing_parameters> ifm_access_pattern_12 = {{.dimension=2, .stride=ifmsv_dim_12[2], .wrap=depth_iter_12}};
std::vector<traversing_parameters> ofm_access_pattern_12 = {{.dimension=2, .stride=ofmsv_dim_12[2]*NUM_ROWS, .wrap=channel_iter_12}};

resnet18_tiling layer12 = resnet18_tiling((int)filt_k, ifm_dim_12, ifmsv_dim_12, ifm_access_pattern_12, ifm_boundary_dim_12, wts_dim_12, wtssv_dim_12, ofm_dim_12, ofmsv_dim_12, ofm_access_pattern_12, {});

//Layer13
//Same as layer11 use layer11 tiling etc
std::vector<uint32_t> ofm_dim_13 = {8, 16, 32, 14};

//Layer14
uint32_t width_iter_14 = 1; uint32_t depth_iter_14 = 16; uint32_t height_iter_14 = 1; uint32_t channel_iter_14 = 4; uint32_t super_iter_14 = 4;
std::vector<uint32_t> ifm_dim_14 = ofm_dim_13; std::vector<uint32_t> ifm_boundary_dim_14 = ifm_boundary_dim_12;
std::vector<uint32_t> wts_ddr_dim_14 = {4928*depth_iter_14*NUM_ROWS*channel_iter_14};
std::vector<uint32_t> wts_dim_14 = {4928*depth_iter_14*NUM_ROWS*channel_iter_14/super_iter_14};
std::vector<uint32_t> ofm_dim_14 = {8, 16, 64, 7};
std::vector<uint32_t> ifmsv_dim_14 = {8, 36, 2, 4}; std::vector<uint32_t> wtssv_dim_14 = {4928}; std::vector<uint32_t> ofmsv_dim_14 = {8, 16, 4, 1};

std::vector<traversing_parameters> ifm_access_pattern_14 = {{.dimension=2, .stride=ifmsv_dim_14[2], .wrap=depth_iter_14}};
std::vector<traversing_parameters> ofm_access_pattern_14 = {{.dimension=2, .stride=ofmsv_dim_14[2]*NUM_ROWS, .wrap=channel_iter_14}};

resnet18_tiling layer14 = resnet18_tiling((int)filt_k, ifm_dim_14, ifmsv_dim_14, ifm_access_pattern_14, ifm_boundary_dim_14, wts_dim_14, wtssv_dim_14, ofm_dim_14, ofmsv_dim_14, ofm_access_pattern_14, {});

//Layer14a
uint32_t width_iter_14a = 1; uint32_t depth_iter_14a = 4; uint32_t height_iter_14a = 1; uint32_t channel_iter_14a = 2;
std::vector<uint32_t> ifm_dim_14a = ofm_dim_13; std::vector<uint32_t> ifm_boundary_dim_14a = ifm_boundary_dim_12;
std::vector<uint32_t> wts_dim_14a = {4672*depth_iter_14a*NUM_ROWS*channel_iter_14a};
std::vector<uint32_t> ofm_dim_14a = {8, 16, 64, 7};
std::vector<uint32_t> ifmsv_dim_14a = {8, 32, 8, 2}; std::vector<uint32_t> wtssv_dim_14a = {4672}; std::vector<uint32_t> ofmsv_dim_14a = {8, 16, 8, 1};

std::vector<traversing_parameters> ifm_access_pattern_14a = {{.dimension=2, .stride=ifmsv_dim_14a[2], .wrap=depth_iter_14a}};
std::vector<traversing_parameters> ofm_access_pattern_14a = {{.dimension=2, .stride=ofmsv_dim_14a[2]*NUM_ROWS, .wrap=channel_iter_14a}};

resnet18_tiling layer14a = resnet18_tiling(1, ifm_dim_14a, ifmsv_dim_14a, ifm_access_pattern_14a, ifm_boundary_dim_14a, wts_dim_14a, wtssv_dim_14a, ofm_dim_14a, ofmsv_dim_14a, ofm_access_pattern_14a, {});

//Layer15
uint32_t width_iter_15 = 1; uint32_t depth_iter_15 = 32; uint32_t height_iter_15 = 1; uint32_t channel_iter_15 = 2; uint32_t super_iter_15 = 2;
std::vector<uint32_t> ifm_dim_15 = ofm_dim_14; std::vector<uint32_t> ifm_boundary_dim_15 = {8, 7, 64, 7};
std::vector<uint32_t> wts_ddr_dim_15 = {4928*depth_iter_15*NUM_ROWS*channel_iter_15};
std::vector<uint32_t> wts_dim_15 = {4928*depth_iter_15*NUM_ROWS*channel_iter_15/super_iter_15};
std::vector<uint32_t> ofm_dim_15 = {8, 16, 64, 7};
std::vector<uint32_t> ifmsv_dim_15 = {8, 20, 2, 3}; std::vector<uint32_t> wtssv_dim_15 = {4928}; std::vector<uint32_t> ofmsv_dim_15 = {8, 16, 4, 1};

std::vector<traversing_parameters> ifm_access_pattern_15 = {{.dimension=2, .stride=ifmsv_dim_15[2], .wrap=depth_iter_15}, {.dimension=1, .stride=0, .wrap=channel_iter_15}};
std::vector<traversing_parameters> ofm_access_pattern_15 = {{.dimension=2, .stride=ofmsv_dim_15[2]*NUM_ROWS, .wrap=channel_iter_15}};

std::vector<uint32_t> ifm2sv_dim_15 = {8, 16, 16, 1};
std::vector<traversing_parameters> ifm2_access_pattern_15 = {{.dimension=2, .stride=ofmsv_dim_15[2]*NUM_ROWS, .wrap=channel_iter_15}};
std::vector<uint32_t> channel_split_15 = {2, 0};

resnet18_tiling layer15 = resnet18_tiling((int)filt_k, ifm_dim_15, ifmsv_dim_15, ifm_access_pattern_15, ifm_boundary_dim_15, wts_dim_15, wtssv_dim_15, ofm_dim_15, ofmsv_dim_15, ofm_access_pattern_15,
		                          ifm2sv_dim_15, ifm2_access_pattern_15, channel_split_15);

//Layer15_1
std::vector<uint32_t> channel_split_15_1 = {2, ofmsv_dim_15[2]*NUM_ROWS*channel_iter_15};
resnet18_tiling layer15_1 = resnet18_tiling((int)filt_k, ifm_dim_15, ifmsv_dim_15, ifm_access_pattern_15, ifm_boundary_dim_15, wts_dim_15, wtssv_dim_15, ofm_dim_15, ofmsv_dim_15, ofm_access_pattern_15,
		                          ifm2sv_dim_15, ifm2_access_pattern_15, channel_split_15_1);
//Layer16
uint32_t width_iter_16 = 1; uint32_t depth_iter_16 = 32; uint32_t height_iter_16 = 1; uint32_t channel_iter_16 = 4; uint32_t super_iter_16 = 4;
std::vector<uint32_t> ifm_dim_16 = ofm_dim_15; std::vector<uint32_t> ifm_boundary_dim_16 = {8, 7, 64, 7};
std::vector<uint32_t> wts_ddr_dim_16 = {4928*depth_iter_16*NUM_ROWS*channel_iter_16};
std::vector<uint32_t> wts_dim_16 = {4928*depth_iter_16*NUM_ROWS*channel_iter_16/super_iter_16};
std::vector<uint32_t> ofm_dim_16 = {8, 16, 64, 7};
std::vector<uint32_t> ifmsv_dim_16 = {8, 20, 2, 3}; std::vector<uint32_t> wtssv_dim_16 = {4928}; std::vector<uint32_t> ofmsv_dim_16 = {8, 16, 4, 1};

std::vector<traversing_parameters> ifm_access_pattern_16 = {{.dimension=2, .stride=ifmsv_dim_16[2], .wrap=depth_iter_16}};
std::vector<traversing_parameters> ofm_access_pattern_16 = {{.dimension=2, .stride=ofmsv_dim_16[2]*NUM_ROWS, .wrap=channel_iter_16}};

resnet18_tiling layer16 = resnet18_tiling((int)filt_k, ifm_dim_16, ifmsv_dim_16, ifm_access_pattern_16, ifm_boundary_dim_16, wts_dim_16, wtssv_dim_16, ofm_dim_16, ofmsv_dim_16, ofm_access_pattern_16, {});

//Layer17
//Same as layer15 use layer15 tiling etc
std::vector<uint32_t> ofm_dim_17 = {8, 1, 64, 7};
//std::vector<location_constraint> ofm_loc_17 = {address(2+COL_OF, 0, 512*1024-512*6)};
resnet18_tiling layer17 = resnet18_tiling((int)filt_k, ifm_dim_15, ifmsv_dim_15, ifm_access_pattern_15, ifm_boundary_dim_15, wts_dim_15, wtssv_dim_15, ofm_dim_15, ofmsv_dim_15, ofm_access_pattern_15,
		                          ifm2sv_dim_15, ifm2_access_pattern_15, channel_split_15, true);
resnet18_tiling layer17_1 = resnet18_tiling((int)filt_k, ifm_dim_15, ifmsv_dim_15, ifm_access_pattern_15, ifm_boundary_dim_15, wts_dim_15, wtssv_dim_15, ofm_dim_15, ofmsv_dim_15, ofm_access_pattern_15,
		                          ifm2sv_dim_15, ifm2_access_pattern_15, channel_split_15_1, true);

//Layer18-FC
//wts is layer17's ofm (1/7th) and ifm is weights of the GEMM
uint32_t width_iter_18 = 1; uint32_t depth_iter_18 = 1; uint32_t height_iter_18 = 1; uint32_t channel_iter_18 = 16;
std::vector<uint32_t> ifm_dim_18 = {8, 448, 1, 1}; std::vector<uint32_t> ifm_boundary_dim_18 = {8, 448, 1, 1};
std::vector<uint32_t> wts_ddr_dim_18 = {659456};
std::vector<uint32_t> wts_dim_18 = {659456};
std::vector<uint32_t> ofm_dim_18 = {8, 1, 128*2, 7};   //16-bit
std::vector<uint32_t> ifmsv_dim_18 = {32, 10, 1, 1}; std::vector<uint32_t> wtssv_dim_18 = {10304}; std::vector<uint32_t> ofmsv_dim_18 = {8, 1, 2*2, 1};
std::vector<traversing_parameters> ifm_access_pattern_18 = {};
std::vector<traversing_parameters> ofm_access_pattern_18 = {{.dimension=2, .stride=ofmsv_dim_18[2], .wrap=channel_iter_18}};

resnet18_tiling layer18 = resnet18_tiling(1, ifm_dim_18, ifmsv_dim_18, ifm_access_pattern_18, ifm_boundary_dim_18, wts_dim_18, wtssv_dim_18, ofm_dim_18, ofmsv_dim_18, ofm_access_pattern_18,
		                          {}, {}, {}, false, true);

//SoftMax overlay
std::vector<uint32_t> ifm_dim_sm = {56*256};
std::vector<uint32_t> ifmsv_dim_sm = {1024*2};
std::vector<uint32_t> ofm_dim_sm = {1024*2,1,1,1};
access_pattern ifm_pattern_sm = tiling({.buffer_dimension = ifm_dim_sm, .tiling_dimension = ifmsv_dim_sm, .offset = {12288}});
access_pattern ofm_pattern_sm = tiling({.buffer_dimension = ofm_dim_sm, .tiling_dimension = ofm_dim_sm, .offset = {0,0,0,0}});

//Input and Output specs
/*
std::vector<location_constraint> input_loc = ofm_loc_17;
std::vector<uint32_t> input_dim = ofm_dim_17; //D0=0:R, D0=1:G, D0=2:B, D0=3:zero-padded to make D0 4 bytes
std::vector<uint32_t> output_dim = ofm_dim_sm; //ofm_dim_18;
*/

std::vector<uint32_t> input_dim = ifm_dim_1; //D0=0:R, D0=1:G, D0=2:B, D0=3:zero-padded to make D0 4 bytes
std::vector<uint32_t> output_dim = ofm_dim_sm;

//WTS DDR Offsets
std::vector<uint32_t> wts_ddr_offset = {0, 3776, 13760, 23744, 33728, 43712, 64704, 67392, 106816, 146240, 185664, 264512, 275008, 432704, 590400, 748096, 1063488, 1100864, 1416256, 1731648, 2362432, 2677824, 2993216};

//Input
access_pattern input_ddr_pattern = tiling({.buffer_dimension = input_dim, .tiling_dimension = input_dim, .offset = {0, 0, 0, 0}});
//final output
access_pattern output_ddr_pattern = tiling({.buffer_dimension = output_dim, .tiling_dimension = output_dim, .offset = {0, 0, 0, 0}});

std::vector<resnet18_tiling> res_tiling = {  layer1,
                                        layer2,
                                        layer3,
                                        layer4,
                                        layer5,
                                        layer6,
                                        layer6a,
                                        layer7,
                                        layer8,
                                        layer9,
                                        layer10,
                                        layer10a,
                                        layer11,
                                        layer12,
                                        layer11,
                                        layer14,
                                        layer14a,
                                        layer15,
                                        layer15_1,
                                        layer16,
                                        layer17,
                                        layer17_1,
                                        layer18
                                     };

template<int COL_OF, int ROW_OF, int memRowId>
class resnet_locations{

   public:
      std::vector<location_constraint> ifm_loc_1 = {address(2+COL_OF, memRowId, m2_delta0)};
      std::vector<location_constraint> ofm_loc_1 = {address(1+COL_OF, memRowId, m1_delta0)};
      std::vector<location_constraint> wts_loc_1 = {address(5+COL_OF,memRowId,0)} ;


      std::vector<location_constraint> ifm_loc_2 = {address(1+COL_OF, memRowId, m1_delta0)};
      std::vector<location_constraint> ofm_loc_2 = {address(2+COL_OF, memRowId, m2_delta0)};
      std::vector<location_constraint> wts_loc_2 = {address(5+COL_OF, memRowId, 0)};


      std::vector<location_constraint> ofm_loc_3 = {address(2+COL_OF, memRowId, m2_delta1)} ;
      std::vector<location_constraint> wts_loc_3 = wts_loc_2 ;


      std::vector<location_constraint> ofm_loc_4 = {address(1+COL_OF, memRowId, m1_delta0)} ;
      std::vector<location_constraint> wts_loc_4 = wts_loc_3 ;


      std::vector<location_constraint> ofm_loc_5 = {address(2+COL_OF, memRowId, m2_delta0)} ;
      std::vector<location_constraint> wts_loc_5 = wts_loc_4 ;


      std::vector<location_constraint> ofm_loc_6 = {address(2+COL_OF, memRowId, m2_delta1)} ;
      std::vector<location_constraint> wts_loc_6 = wts_loc_5 ;


      std::vector<location_constraint> ofm_loc_6a = {address(2+COL_OF, memRowId, m2_delta10)} ;
      std::vector<location_constraint> wts_loc_6a = wts_loc_5 ;


      std::vector<location_constraint> ofm_loc_7 = {address(2+COL_OF, memRowId, m2_delta0)} ;
      std::vector<location_constraint> wts_loc_7 = wts_loc_6 ;


      std::vector<location_constraint> ofm_loc_8 = {address(2+COL_OF, memRowId, m2_delta00)} ;
      std::vector<location_constraint> wts_loc_8 = wts_loc_7 ;


      std::vector<location_constraint> ofm_loc_9 = {address(2+COL_OF, memRowId, m2_delta1)} ;
      std::vector<location_constraint> wts_loc_9 = wts_loc_6 ;


      std::vector<location_constraint> ofm_loc_10 = {address(2+COL_OF, memRowId, m2_delta0)} ;
      std::vector<location_constraint> wts_loc_10 = wts_loc_9 ;


      std::vector<location_constraint> ofm_loc_10a = {address(2+COL_OF, memRowId, m2_delta00)} ;
      std::vector<location_constraint> wts_loc_10a = wts_loc_9 ;


      std::vector<location_constraint> ofm_loc_11 = {address(2+COL_OF, memRowId, m2_delta1)};
      std::vector<location_constraint> wts_loc_11 = {address(4+COL_OF, memRowId, 0), address(5+COL_OF, memRowId, 0)};


      std::vector<location_constraint> ofm_loc_12 = {address(2+COL_OF, memRowId, m2_delta0)};
      std::vector<location_constraint> wts_loc_12 = wts_loc_11;


      std::vector<location_constraint> ofm_loc_13 = {address(2+COL_OF, memRowId, m2_delta00)};


      std::vector<location_constraint> ofm_loc_14 = {address(2+COL_OF, memRowId, m2_delta0)} ;
      std::vector<location_constraint> wts_loc_14 = wts_loc_11 ;


      std::vector<location_constraint> ofm_loc_14a = {address(2+COL_OF, memRowId, m2_delta1)} ;
      std::vector<location_constraint> wts_loc_14a = wts_loc_9 ;


      std::vector<location_constraint> ofm_loc_15 = {address(2+COL_OF, memRowId, m2_delta00)};
      std::vector<location_constraint> wts_loc_15 = {address(3+COL_OF, memRowId, 311296), address(4+COL_OF, memRowId, 417792)}; //{address(4+COL_OF, memRowId, 311296)}; //


      std::vector<location_constraint> ofm_loc_16 = {address(2+COL_OF, memRowId, m2_delta0)};
      std::vector<location_constraint> wts_loc_16 = wts_loc_15;


      std::vector<location_constraint> ofm_loc_17 = {address(2+COL_OF, memRowId, m2_delta1)};


      std::vector<location_constraint> wts_loc_18 = {address(3+COL_OF, memRowId, 256*1024)};     //for ifm (wts of FC)
      std::vector<location_constraint> ofm_loc_18 = {address(2+COL_OF, memRowId, m2_delta0)};


      //std::vector<uint32_t> core_loc_sm = {COL_OF+NUM_COLS,ROW_OF};
      std::vector<location_constraint> ofm_loc_sm = {address(3+COL_OF, memRowId, 0)};

      std::vector<location_constraint> input_loc = ifm_loc_1;

};
