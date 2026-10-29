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

   resnet18_tiling(const int filt_k, const std::vector<uint32_t>& iters, const std::vector<uint32_t>& ifm_dim, const std::vector<uint32_t>& ifmsv_dim,
		   const std::vector<std::vector<traversing_parameters>> ifm_access_pattern, const std::vector<uint32_t>& ifm_boundary_dim,
		   const std::vector<uint32_t>& wts_dim, const std::vector<uint32_t>& wtssv_dim,
		   const std::vector<uint32_t>& ofm_dim, const std::vector<uint32_t>& ofmsv_dim, const std::vector<traversing_parameters> ofm_access_pattern,
		   const std::vector<uint32_t>& ifm2sv_dim, const std::vector<traversing_parameters> ifm2_access_pattern = {}, const std::vector<uint32_t> channel_split = {},
		   const bool fused_gap=false, const bool is_fc=false, const std::vector<uint32_t>& ifm2_dim = {}, const uint32_t num_ph=1, const int ph_stride=0)
   {
       uint32_t height_iter, width_iter, depth_iter, channel_iter, sub_channel_iter, super_iter;
       uint32_t rate = 1;
       int32_t ifm_fc_offset = 64*(NUM_COLS-1);
       wtssv_size = wtssv_dim[0];
       ofmsv_size = ofmsv_dim[0]*ofmsv_dim[1]*ofmsv_dim[2]*ofmsv_dim[3];
       ifmsv_size = ifmsv_dim[0]*ifmsv_dim[1]*ifmsv_dim[2]*ifmsv_dim[3];
       int extra_pad = (filt_k==7)?2:0;
       int extra_pad_left = (filt_k==7)?1:0;
       /*
       if(is_fc){
          height_iter = 1; width_iter = 1; depth_iter = 1;
          channel_iter = ofm_dim[2]/ofmsv_dim[2]/NUM_ROWS;
       }else{
          height_iter = ofm_dim[3]/ofmsv_dim[3]/NUM_COLS; width_iter = ofm_dim[1]/ofmsv_dim[1]; depth_iter = ifm_dim[2]/ifmsv_dim[2]; channel_iter = ofm_dim[2]/ofmsv_dim[2]/NUM_ROWS;
          rate = ifm_boundary_dim[3]/ofm_dim[3];
       }
       */

       height_iter = iters[0]; width_iter = iters[1]; depth_iter = iters[2]; channel_iter = iters[3]; super_iter = iters[4]; rate = iters[5];

       int channel_offset=0;
       if(!channel_split.empty()){
          //channel_iter = channel_iter/channel_split[0];
	  channel_offset = channel_split[1];
       }
       kernel_iter = height_iter*width_iter*depth_iter*channel_iter;
       ofm_repetition = depth_iter;

       std::vector<traversing_parameters> wts_access_pattern;
       uint32_t ifm_repetition = channel_iter;
       //uint32_t super_iter = 1; //is_fc?1:(uint32_t)(wtssv_dim[0]/(wts_dim[0]/depth_iter/channel_iter/NUM_ROWS));
       sub_channel_iter = is_fc?1:ceil(channel_iter/super_iter);
       std::vector<uint32_t> wts_buffer_dim = {wts_dim[0]/sub_channel_iter, sub_channel_iter};
       std::vector<uint32_t> ofm_dim_int = ofm_dim; std::vector<uint32_t> ofmsv_dim_int = ofmsv_dim;
       std::vector<uint32_t> ifm2_dim_int = ofm_dim;

       wts_ddr_dim = {wts_dim[0]*super_iter};

       for(int i=0; i<NUM_WTS_SHIM_CH; i++){
	    wts_ddr_read_pattern.push_back(tiling({.buffer_dimension = {wts_dim[0]*super_iter}, .tiling_dimension = {wts_dim[0]/NUM_WTS_SHIM_CH}, .offset = {int32_t(i*wts_dim[0]/NUM_WTS_SHIM_CH)},
				      .tile_traversal={{.dimension=0, .stride=wts_dim[0], .wrap=super_iter}}, .packet_port_id = -1, .repetition = 1}));
	    wts_ddr_write_pattern.push_back(tiling({.buffer_dimension = wts_dim, .tiling_dimension = {wts_dim[0]/NUM_WTS_SHIM_CH}, .offset = {int32_t(i*wts_dim[0]/NUM_WTS_SHIM_CH)}}));
	}

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
          ofm_dim_int[1]=BATCH_SIZE*1;             //Global Avgpool
          ofmsv_dim_int[1]=BATCH_SIZE*1;           //Global Avgpool
	  ofmsv_size = BATCH_SIZE*ofmsv_size/ofmsv_dim[1];
       }

       for(int i=0; i<NUM_COLS; i++){
	  for(uint32_t k=0; k<num_ph; k++){
	     //IFM
	     ifm_pattern[i].push_back(tiling({.buffer_dimension = ifm_dim, .tiling_dimension = ifmsv_dim,
					      .offset = {0, is_fc?(ifm_fc_offset+(int)(i*ifmsv_dim[1])):-(BATCH_SIZE*(((filt_k-1)>>1)+extra_pad+extra_pad_left))+(int)k*ph_stride, 0,
					                 -((filt_k-1)>>1)+(int)(i*height_iter*((is_fc?0:ifmsv_dim[3])-filt_k+1-extra_pad)-extra_pad)},
					      .tile_traversal = ifm_access_pattern[k], .packet_port_id = -1, .repetition = ifm_repetition, .boundary_dimension = ifm_boundary_dim, .phase=k}));
             //OFM
	     for(int j=0; j<NUM_ROWS; j++)
	        ofm_pattern[i].push_back(tiling({.buffer_dimension = ofm_dim_int, .tiling_dimension = ofmsv_dim_int,
					         .offset = {0, (int)(k*ph_stride/rate), (int)(j*ofmsv_dim[2]*(is_fc?channel_iter:1))+(int)k*channel_offset, (int)(i*ofmsv_dim[3]*height_iter)},
                                                 .tile_traversal = {ofm_access_pattern[k]}, .packet_port_id = j, .repetition=1, .boundary_dimension = ofm_dim_int, .phase=k}));
	     //IFM2
	     if(ifm2sv_size>0)
	        ifm2_pattern[i].push_back(tiling({.buffer_dimension = ifm2_dim_int, .tiling_dimension = ifm2sv_dim, .offset = {0, (int)k*ph_stride, (int)k*channel_offset, (i*(int)(ifm2sv_dim[3]))},
                                                  .tile_traversal = {ifm2_access_pattern[k]}, .packet_port_id = -1,.repetition = 1, .boundary_dimension = ifm_boundary_dim, .phase=k}));
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

int filt_k3 = 3; int filt_k1 = 1; uint32_t width_pad = 0; uint32_t rate_r1 = 1; uint32_t rate_r2 = 2; uint32_t rate_r4 = 4;


//Layer1
uint32_t filt_k7 = 7;
std::vector<uint32_t> ifm_dim_1 = {4, BATCH_SIZE*224, 1, 224}; std::vector<uint32_t> ifm_boundary_dim_1 = ifm_dim_1;
std::vector<uint32_t> wts_dim_1 = {3776*4};
std::vector<uint32_t> ofm_dim_1 = {8, BATCH_SIZE*56, 8, 56};
//ifmsv: width-> 3+2*(1+56+7)+3+2=136, height-> 3+2*(1+2)+3=12
std::vector<uint32_t> ifmsv_dim_1 = {4, BATCH_SIZE*136, 1, 12}; std::vector<uint32_t> wtssv_dim_1 = {3776}; std::vector<uint32_t> ofmsv_dim_1 = {8, BATCH_SIZE*28, 2, 1};
uint32_t width_iter_1 = 2; uint32_t depth_iter_1 = 1; uint32_t height_iter_1 = 8; uint32_t channel_iter_1 = 1; uint32_t super_iter_1 = 1;
uint32_t extra_pad = 2;

std::vector<uint32_t> iters_1 = {height_iter_1, width_iter_1, depth_iter_1, channel_iter_1, super_iter_1, rate_r4};


std::vector<std::vector<traversing_parameters>> ifm_access_pattern_1 = {{{.dimension=3, .stride=(ifmsv_dim_1[3]-filt_k7+1-extra_pad), .wrap=height_iter_1}},{{.dimension=3, .stride=(ifmsv_dim_1[3]-filt_k7+1-extra_pad), .wrap=height_iter_1}}};
std::vector<traversing_parameters> ofm_access_pattern_1 = {{.dimension=3, .stride=ofmsv_dim_1[3], .wrap=height_iter_1},{.dimension=3, .stride=ofmsv_dim_1[3], .wrap=height_iter_1}};

resnet18_tiling layer1 = resnet18_tiling(filt_k7, iters_1,ifm_dim_1, ifmsv_dim_1, ifm_access_pattern_1, ifm_boundary_dim_1, wts_dim_1, wtssv_dim_1, ofm_dim_1, ofmsv_dim_1, ofm_access_pattern_1, {}, {}, {}, false, false, {}, 2, 224);//(ifmsv_dim_3[1]-(filt_k3-1)*BATCH_SIZE)*width_iter_ph_3[0]);


//Layer2
uint32_t width_iter_2 = 7; uint32_t depth_iter_2 = 2; uint32_t height_iter_2 = 1; uint32_t channel_iter_2 = 1; uint32_t super_iter_2 = 1;
std::vector<uint32_t> ifm_dim_2 = {8, BATCH_SIZE*56, 8, 56}; std::vector<uint32_t> ifm_boundary_dim_2 = {8, BATCH_SIZE*56, 8, 56};
std::vector<uint32_t> ofm_dim_2 = {8, BATCH_SIZE*56, 8, 56};
std::vector<uint32_t> ifmsv_dim_2 = {8, BATCH_SIZE*10, 4, 10}; std::vector<uint32_t> wtssv_dim_2 = {4800}; std::vector<uint32_t> ofmsv_dim_2 = {8, BATCH_SIZE*8, 2, 8};
std::vector<uint32_t> wts_dim_2 = {wtssv_dim_2[0]*NUM_ROWS*depth_iter_2};
std::vector<uint32_t> iters_2 = {height_iter_2, width_iter_2, depth_iter_2, channel_iter_2, super_iter_2, rate_r1};

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_2 = {{{.dimension=2, .stride=ifmsv_dim_2[2], .wrap=depth_iter_2}, {.dimension=1, .stride=(ifmsv_dim_2[1]-(filt_k3-1)*BATCH_SIZE), .wrap=width_iter_2}}};
std::vector<traversing_parameters> ofm_access_pattern_2 = {{.dimension=1, .stride=ofmsv_dim_2[1], .wrap=width_iter_2}};

resnet18_tiling layer2 = resnet18_tiling(filt_k3, iters_2, ifm_dim_2, ifmsv_dim_2, ifm_access_pattern_2, ifm_boundary_dim_2, wts_dim_2, wtssv_dim_2, ofm_dim_2, ofmsv_dim_2, ofm_access_pattern_2, {});


//Layer3
uint32_t width_iter_3 = 7; uint32_t depth_iter_3 = 2; uint32_t height_iter_3 = 1; uint32_t channel_iter_3 = 1; uint32_t super_iter_3 = 1;
std::vector<uint32_t> width_iter_ph_3 = {4, 3};
std::vector<uint32_t> ifm_dim_3 = ofm_dim_2; std::vector<uint32_t> ifm_boundary_dim_3 = ifm_boundary_dim_2;
std::vector<uint32_t> ofm_dim_3 = ofm_dim_2;
std::vector<uint32_t> ifmsv_dim_3 = ifmsv_dim_2; std::vector<uint32_t> wtssv_dim_3 = wtssv_dim_2; std::vector<uint32_t> ofmsv_dim_3 = ofmsv_dim_2;
std::vector<uint32_t> wts_dim_3 = wts_dim_2;
std::vector<uint32_t> iters_3 = {height_iter_3, width_iter_3, depth_iter_3, channel_iter_3, super_iter_3, rate_r1};

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_3 = {{{.dimension=2, .stride=ifmsv_dim_3[2], .wrap=depth_iter_3}, {.dimension=1, .stride=(ifmsv_dim_3[1]-(filt_k3-1)*BATCH_SIZE), .wrap=width_iter_ph_3[0]}},
                                                           {{.dimension=2, .stride=ifmsv_dim_3[2], .wrap=depth_iter_3}, {.dimension=1, .stride=(ifmsv_dim_3[1]-(filt_k3-1)*BATCH_SIZE), .wrap=width_iter_ph_3[1]}}};
std::vector<traversing_parameters> ofm_access_pattern_3 = {{.dimension=1, .stride=ofmsv_dim_3[1], .wrap=width_iter_ph_3[0]}, {.dimension=1, .stride=ofmsv_dim_3[1], .wrap=width_iter_ph_3[1]}};

std::vector<uint32_t> ifm2_dim_3 = ifm_dim_2;
std::vector<uint32_t> ifm2sv_dim_3 = {8, BATCH_SIZE*8, 8, 8};
std::vector<traversing_parameters> ifm2_access_pattern_3 = {{.dimension=1, .stride=ofmsv_dim_3[1], .wrap=width_iter_ph_3[0]}, {.dimension=1, .stride=ofmsv_dim_3[1], .wrap=width_iter_ph_3[1]}};

resnet18_tiling layer3 = resnet18_tiling(filt_k3, iters_3, ifm_dim_3, ifmsv_dim_3, ifm_access_pattern_3, ifm_boundary_dim_3, wts_dim_3, wtssv_dim_3, ofm_dim_3, ofmsv_dim_3, ofm_access_pattern_3,
		                          ifm2sv_dim_3, ifm2_access_pattern_3, {}, false, false, ifm2_dim_3, 2, (ifmsv_dim_3[1]-(filt_k3-1)*BATCH_SIZE)*width_iter_ph_3[0]);


//Layer4
std::vector<uint32_t> ifm_dim_4 = ofm_dim_3; std::vector<uint32_t> ifm_boundary_dim_4 = ifm_boundary_dim_2;
std::vector<uint32_t> wts_dim_4 = wts_dim_2;
std::vector<uint32_t> ofm_dim_4 = ofm_dim_2;
std::vector<uint32_t> ifmsv_dim_4 = ifmsv_dim_2; std::vector<uint32_t> wtssv_dim_4 = wtssv_dim_2; std::vector<uint32_t> ofmsv_dim_4 = ofmsv_dim_2;
uint32_t width_iter_4 = width_iter_2; uint32_t depth_iter_4 = depth_iter_2; uint32_t height_iter_4 = height_iter_2; uint32_t channel_iter_4 = channel_iter_2;
std::vector<uint32_t> iters_4 = iters_2;

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_4 = ifm_access_pattern_2;
std::vector<traversing_parameters> ofm_access_pattern_4 = ofm_access_pattern_2;

resnet18_tiling layer4 = resnet18_tiling(filt_k3, iters_4, ifm_dim_4, ifmsv_dim_4, ifm_access_pattern_4, ifm_boundary_dim_4, wts_dim_4, wtssv_dim_4, ofm_dim_4, ofmsv_dim_4, ofm_access_pattern_4, {});


//Layer5
std::vector<uint32_t> ifm_dim_5 = ofm_dim_4; std::vector<uint32_t> ifm_boundary_dim_5 = ifm_boundary_dim_3;
std::vector<uint32_t> wts_dim_5 = wts_dim_3;
std::vector<uint32_t> ofm_dim_5 = ofm_dim_3;
std::vector<uint32_t> ifmsv_dim_5 = ifmsv_dim_3; std::vector<uint32_t> wtssv_dim_5 = wtssv_dim_3; std::vector<uint32_t> ofmsv_dim_5 = ofmsv_dim_3;
uint32_t width_iter_5 = width_iter_3; uint32_t depth_iter_5 = depth_iter_3; uint32_t height_iter_5 = height_iter_3; uint32_t channel_iter_5 = channel_iter_3;
std::vector<uint32_t> iters_5 = iters_3;
std::vector<uint32_t> width_iter_ph_5 = width_iter_ph_3;

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_5 = ifm_access_pattern_3;
std::vector<traversing_parameters> ofm_access_pattern_5 = ofm_access_pattern_3;

std::vector<uint32_t> ifm2_dim_5 = ifm_dim_3;
std::vector<uint32_t> ifm2sv_dim_5 = ifm2sv_dim_3;
std::vector<traversing_parameters> ifm2_access_pattern_5 = ifm2_access_pattern_3;

resnet18_tiling layer5 = resnet18_tiling(filt_k3, iters_5, ifm_dim_5, ifmsv_dim_5, ifm_access_pattern_5, ifm_boundary_dim_5, wts_dim_5, wtssv_dim_5, ofm_dim_5, ofmsv_dim_5, ofm_access_pattern_5,
		                          ifm2sv_dim_5, ifm2_access_pattern_5, {}, false, false, ifm2_dim_5, 2, (ifmsv_dim_5[1]-(filt_k3-1)*BATCH_SIZE)*width_iter_ph_5[0]);


//Layer6
uint32_t width_iter_6 = 4; uint32_t depth_iter_6 = 2; uint32_t height_iter_6 = 1; uint32_t channel_iter_6 = 1; uint32_t super_iter_6 = 1;
std::vector<uint32_t> ifm_dim_6 = ofm_dim_5; std::vector<uint32_t> ifm_boundary_dim_6 = ifm_boundary_dim_5;
std::vector<uint32_t> ofm_dim_6 = {8, BATCH_SIZE*32, 16, 28};
std::vector<uint32_t> ifmsv_dim_6 = {8, BATCH_SIZE*18, 4, 10}; std::vector<uint32_t> wtssv_dim_6 = {9536}; std::vector<uint32_t> ofmsv_dim_6 = {8, BATCH_SIZE*8, 4, 4};
std::vector<uint32_t> wts_dim_6 = {wtssv_dim_6[0]*NUM_ROWS*depth_iter_6};
std::vector<uint32_t> iters_6 = {height_iter_6, width_iter_6, depth_iter_6, channel_iter_6, super_iter_6, rate_r2};

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_6 = {{{.dimension=2, .stride=ifmsv_dim_6[2], .wrap=depth_iter_6}, {.dimension=1, .stride=(ifmsv_dim_6[1]-(filt_k3-1)*BATCH_SIZE), .wrap=width_iter_6}}};
std::vector<traversing_parameters> ofm_access_pattern_6 = {{.dimension=1, .stride=ofmsv_dim_6[1], .wrap=width_iter_6}};

resnet18_tiling layer6 = resnet18_tiling(filt_k3, iters_6, ifm_dim_6, ifmsv_dim_6, ifm_access_pattern_6, ifm_boundary_dim_6, wts_dim_6, wtssv_dim_6, ofm_dim_6, ofmsv_dim_6, ofm_access_pattern_6, {});


//Layer6a
uint32_t width_iter_6a = 4; uint32_t depth_iter_6a = 2; uint32_t height_iter_6a = 1; uint32_t channel_iter_6a = 1; uint32_t super_iter_6a = 1;
std::vector<uint32_t> ifm_dim_6a = ofm_dim_5; std::vector<uint32_t> ifm_boundary_dim_6a = ifm_boundary_dim_5;
std::vector<uint32_t> ofm_dim_6a = {8, BATCH_SIZE*32, 16, 28};
std::vector<uint32_t> ifmsv_dim_6a = {8, BATCH_SIZE*16, 4, 8}; std::vector<uint32_t> wtssv_dim_6a = {1344}; std::vector<uint32_t> ofmsv_dim_6a = {8, BATCH_SIZE*8, 4, 4};
std::vector<uint32_t> wts_dim_6a = {wtssv_dim_6a[0]*NUM_ROWS*depth_iter_6a};
std::vector<uint32_t> iters_6a = {height_iter_6a, width_iter_6a, depth_iter_6a, channel_iter_6a, super_iter_6a, rate_r2};

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_6a = {{{.dimension=2, .stride=ifmsv_dim_6a[2], .wrap=depth_iter_6a}, {.dimension=1, .stride=ifmsv_dim_6a[1], .wrap=width_iter_6a}}};
std::vector<traversing_parameters> ofm_access_pattern_6a = {{.dimension=1, .stride=ofmsv_dim_6a[1], .wrap=width_iter_6a}};

resnet18_tiling layer6a = resnet18_tiling(filt_k1, iters_6a, ifm_dim_6a, ifmsv_dim_6a, ifm_access_pattern_6a, ifm_boundary_dim_6a, wts_dim_6a, wtssv_dim_6a, ofm_dim_6a, ofmsv_dim_6a, ofm_access_pattern_6a, {});


//Layer7
uint32_t width_iter_7 = 2; uint32_t depth_iter_7 = 4; uint32_t height_iter_7 = 1; uint32_t channel_iter_7 = 2; uint32_t super_iter_7 = 1;
std::vector<uint32_t> ifm_dim_7 = ofm_dim_6; std::vector<uint32_t> ifm_boundary_dim_7 = {8, BATCH_SIZE*28, 16, 28};
std::vector<uint32_t> ofm_dim_7 = {8, BATCH_SIZE*32, 16, 28};
std::vector<uint32_t> ifmsv_dim_7 = {8, BATCH_SIZE*18, 4, 6}; std::vector<uint32_t> wtssv_dim_7 = {4800}; std::vector<uint32_t> ofmsv_dim_7 = {8, BATCH_SIZE*16, 2, 4};
std::vector<uint32_t> wts_dim_7 = {wtssv_dim_7[0]*NUM_ROWS*depth_iter_7*channel_iter_7};
std::vector<uint32_t> iters_7 = {height_iter_7, width_iter_7, depth_iter_7, channel_iter_7, super_iter_7, rate_r1};

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_7 = {{{.dimension=2, .stride=ifmsv_dim_7[2], .wrap=depth_iter_7}, {.dimension=1, .stride=0, .wrap=channel_iter_7}},
	                                                                {{.dimension=2, .stride=ifmsv_dim_7[2], .wrap=depth_iter_7}, {.dimension=1, .stride=0, .wrap=channel_iter_7}}};
std::vector<traversing_parameters> ofm_access_pattern_7 = {{.dimension=2, .stride=ofmsv_dim_7[2]*NUM_ROWS, .wrap=channel_iter_7}, {.dimension=2, .stride=ofmsv_dim_7[2]*NUM_ROWS, .wrap=channel_iter_7}};

std::vector<uint32_t> ifm2_dim_7 = ofm_dim_6a;
std::vector<uint32_t> ifm2sv_dim_7 = {8, BATCH_SIZE*16, 8, 4};
std::vector<traversing_parameters> ifm2_access_pattern_7 = {{.dimension=2, .stride=ofmsv_dim_7[2]*NUM_ROWS, .wrap=channel_iter_7}, {.dimension=2, .stride=ofmsv_dim_7[2]*NUM_ROWS, .wrap=channel_iter_7}};

resnet18_tiling layer7 = resnet18_tiling(filt_k3, iters_7, ifm_dim_7, ifmsv_dim_7, ifm_access_pattern_7, ifm_boundary_dim_7, wts_dim_7, wtssv_dim_7, ofm_dim_7, ofmsv_dim_7, ofm_access_pattern_7,
                                         ifm2sv_dim_7, ifm2_access_pattern_7, {}, false, false, ifm2_dim_7, 2, (ifmsv_dim_7[1]-(filt_k3-1)*BATCH_SIZE));


//Layer8
uint32_t width_iter_8 = 1; uint32_t depth_iter_8 = 4; uint32_t height_iter_8 = 1; uint32_t channel_iter_8 = 2; uint32_t super_iter_8 = 1;
std::vector<uint32_t> ifm_dim_8 = ofm_dim_7; std::vector<uint32_t> ifm_boundary_dim_8 = ifm_boundary_dim_7;
std::vector<uint32_t> ofm_dim_8 = ofm_dim_7;
std::vector<uint32_t> ifmsv_dim_8 = {8, BATCH_SIZE*34, 4, 6}; std::vector<uint32_t> wtssv_dim_8 = {4800}; std::vector<uint32_t> ofmsv_dim_8 = {8, BATCH_SIZE*32, 2, 4};
std::vector<uint32_t> wts_dim_8 = {wtssv_dim_8[0]*NUM_ROWS*depth_iter_8*channel_iter_8};
std::vector<uint32_t> iters_8 = {height_iter_8, width_iter_8, depth_iter_8, channel_iter_8, super_iter_8, rate_r1};

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_8 = {{{.dimension=2, .stride=ifmsv_dim_8[2], .wrap=depth_iter_8}}};
std::vector<traversing_parameters> ofm_access_pattern_8 = {{.dimension=2, .stride=ofmsv_dim_8[2]*NUM_ROWS, .wrap=channel_iter_8}};

resnet18_tiling layer8 = resnet18_tiling(filt_k3, iters_8, ifm_dim_8, ifmsv_dim_8, ifm_access_pattern_8, ifm_boundary_dim_8, wts_dim_8, wtssv_dim_8, ofm_dim_8, ofmsv_dim_8, ofm_access_pattern_8, {});


//Layer9
uint32_t width_iter_9 = 2; uint32_t depth_iter_9 = 4; uint32_t height_iter_9 = 1; uint32_t channel_iter_9 = 2; uint32_t super_iter_9 = 1;
std::vector<uint32_t> ifm_dim_9 = ofm_dim_8; std::vector<uint32_t> ifm_boundary_dim_9 = ifm_boundary_dim_7;
std::vector<uint32_t> wts_dim_9 = wts_dim_7;
std::vector<uint32_t> ofm_dim_9 = ofm_dim_7;
std::vector<uint32_t> ifmsv_dim_9 = ifmsv_dim_7; std::vector<uint32_t> wtssv_dim_9 = {4800}; std::vector<uint32_t> ofmsv_dim_9 = ofmsv_dim_7;
std::vector<uint32_t> iters_9 = {height_iter_9, width_iter_9, depth_iter_9, channel_iter_9, super_iter_9, rate_r1};

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_9 = {{{.dimension=2, .stride=ifmsv_dim_9[2], .wrap=depth_iter_9}, {.dimension=1, .stride=0, .wrap=channel_iter_9}},
	                                                                {{.dimension=2, .stride=ifmsv_dim_9[2], .wrap=depth_iter_9}, {.dimension=1, .stride=0, .wrap=channel_iter_9}}};
std::vector<traversing_parameters> ofm_access_pattern_9 = {{.dimension=2, .stride=ofmsv_dim_9[2]*NUM_ROWS, .wrap=channel_iter_9}, {.dimension=2, .stride=ofmsv_dim_9[2]*NUM_ROWS, .wrap=channel_iter_9}};

std::vector<uint32_t> ifm2_dim_9 = ifm2_dim_7;
std::vector<uint32_t> ifm2sv_dim_9 = ifm2sv_dim_7;
std::vector<traversing_parameters> ifm2_access_pattern_9 = {{.dimension=2, .stride=ofmsv_dim_9[2]*NUM_ROWS, .wrap=channel_iter_9}, {.dimension=2, .stride=ofmsv_dim_9[2]*NUM_ROWS, .wrap=channel_iter_9}};

resnet18_tiling layer9 = resnet18_tiling(filt_k3, iters_9, ifm_dim_9, ifmsv_dim_9, ifm_access_pattern_9, ifm_boundary_dim_9, wts_dim_9, wtssv_dim_9, ofm_dim_9, ofmsv_dim_9, ofm_access_pattern_9,
                                         ifm2sv_dim_9, ifm2_access_pattern_9, {}, false, false, ifm2_dim_9, 2, (ifmsv_dim_9[1]-(filt_k3-1)*BATCH_SIZE));


//Layer10
uint32_t width_iter_10 = 1; uint32_t depth_iter_10 = 4; uint32_t height_iter_10 = 1; uint32_t channel_iter_10 = 2; uint32_t super_iter_10 = 1;
std::vector<uint32_t> ifm_dim_10 = ofm_dim_9; std::vector<uint32_t> ifm_boundary_dim_10 = ifm_boundary_dim_9;
std::vector<uint32_t> ofm_dim_10 = {8, BATCH_SIZE*16, 32, 14};
std::vector<uint32_t> ifmsv_dim_10 = {8, BATCH_SIZE*34, 4, 6}; std::vector<uint32_t> wtssv_dim_10 = {9536}; std::vector<uint32_t> ofmsv_dim_10 = {8, BATCH_SIZE*16, 4, 2};
std::vector<uint32_t> wts_dim_10 = {wtssv_dim_10[0]*NUM_ROWS*depth_iter_10*channel_iter_10};
std::vector<uint32_t> iters_10 = {height_iter_10, width_iter_10, depth_iter_10, channel_iter_10, super_iter_10, rate_r2};

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_10 = {{{.dimension=2, .stride=ifmsv_dim_10[2], .wrap=depth_iter_10}}};
std::vector<traversing_parameters> ofm_access_pattern_10 = {{.dimension=2, .stride=ofmsv_dim_10[2]*NUM_ROWS, .wrap=channel_iter_10}};

resnet18_tiling layer10 = resnet18_tiling(filt_k3, iters_10, ifm_dim_10, ifmsv_dim_10, ifm_access_pattern_10, ifm_boundary_dim_10, wts_dim_10, wtssv_dim_10, ofm_dim_10, ofmsv_dim_10, ofm_access_pattern_10, {});


//Layer10a
uint32_t width_iter_10a = 1; uint32_t depth_iter_10a = 4; uint32_t height_iter_10a = 1; uint32_t channel_iter_10a = 1; uint32_t super_iter_10a = 1;
std::vector<uint32_t> ifm_dim_10a = ofm_dim_9; std::vector<uint32_t> ifm_boundary_dim_10a = ifm_boundary_dim_9;
std::vector<uint32_t> ofm_dim_10a = {8, BATCH_SIZE*16, 32, 14};
std::vector<uint32_t> ifmsv_dim_10a = {8, BATCH_SIZE*32, 4, 4}; std::vector<uint32_t> wtssv_dim_10a = {2624}; std::vector<uint32_t> ofmsv_dim_10a = {8, BATCH_SIZE*16, 8, 2};
std::vector<uint32_t> wts_dim_10a = {wtssv_dim_10a[0]*depth_iter_10a*NUM_ROWS*channel_iter_10a};
std::vector<uint32_t> iters_10a = {height_iter_10a, width_iter_10a, depth_iter_10a, channel_iter_10a, super_iter_10a, rate_r2};

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_10a = {{{.dimension=2, .stride=ifmsv_dim_10a[2], .wrap=depth_iter_10a}}};
std::vector<traversing_parameters> ofm_access_pattern_10a = {{.dimension=2, .stride=ofmsv_dim_10a[2]*NUM_ROWS, .wrap=channel_iter_10a}}; // dummy

resnet18_tiling layer10a = resnet18_tiling(filt_k1, iters_10a, ifm_dim_10a, ifmsv_dim_10a, ifm_access_pattern_10a, ifm_boundary_dim_10a, wts_dim_10a, wtssv_dim_10a, ofm_dim_10a, ofmsv_dim_10a, ofm_access_pattern_10a, {});


//Layer11
uint32_t width_iter_11 = 1; uint32_t depth_iter_11 = 8; uint32_t height_iter_11 = 1; uint32_t channel_iter_11 = 4; uint32_t super_iter_11 = 1;
std::vector<uint32_t> ifm_dim_11 = ofm_dim_10; std::vector<uint32_t> ifm_boundary_dim_11 = {8, BATCH_SIZE*14, 32, 14};
std::vector<uint32_t> ofm_dim_11 = {8, BATCH_SIZE*16, 32, 14};
std::vector<uint32_t> ifmsv_dim_11 = {8, BATCH_SIZE*18, 4, 4}; std::vector<uint32_t> wtssv_dim_11 = {4800}; std::vector<uint32_t> ofmsv_dim_11 = {8, BATCH_SIZE*16, 2, 2};
std::vector<uint32_t> wts_dim_11 = {wtssv_dim_11[0]*depth_iter_11*NUM_ROWS*channel_iter_11};
std::vector<uint32_t> iters_11 = {height_iter_11, width_iter_11, depth_iter_11, channel_iter_11, super_iter_11, rate_r1};

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_11 = {{{.dimension=2, .stride=ifmsv_dim_11[2], .wrap=depth_iter_11}, {.dimension=1, .stride=0, .wrap=channel_iter_11}}};
std::vector<traversing_parameters> ofm_access_pattern_11 = {{.dimension=2, .stride=ofmsv_dim_11[2]*NUM_ROWS, .wrap=channel_iter_11}};

std::vector<uint32_t> ifm2sv_dim_11 = {8, BATCH_SIZE*16, 8, 2};
std::vector<traversing_parameters> ifm2_access_pattern_11 = {{.dimension=2, .stride=ofmsv_dim_11[2]*NUM_ROWS, .wrap=channel_iter_11}};

resnet18_tiling layer11 = resnet18_tiling(filt_k3, iters_11, ifm_dim_11, ifmsv_dim_11, ifm_access_pattern_11, ifm_boundary_dim_11, wts_dim_11, wtssv_dim_11, ofm_dim_11, ofmsv_dim_11, ofm_access_pattern_11,
		                          ifm2sv_dim_11, ifm2_access_pattern_11);

//Layer12
uint32_t width_iter_12 = 1; uint32_t depth_iter_12 = 8; uint32_t height_iter_12 = 1; uint32_t channel_iter_12 = 2; uint32_t super_iter_12 = 1;
std::vector<uint32_t> ifm_dim_12 = ofm_dim_11; std::vector<uint32_t> ifm_boundary_dim_12 = ifm_boundary_dim_11;
std::vector<uint32_t> ofm_dim_12 = ofm_dim_11;
std::vector<uint32_t> ifmsv_dim_12 = {8, BATCH_SIZE*18, 4, 4}; std::vector<uint32_t> wtssv_dim_12 = {9536}; std::vector<uint32_t> ofmsv_dim_12 = {8, BATCH_SIZE*16, 4, 2};
std::vector<uint32_t> wts_dim_12 = {wtssv_dim_12[0]*depth_iter_12*NUM_ROWS*channel_iter_12};
std::vector<uint32_t> iters_12 = {height_iter_12, width_iter_12, depth_iter_12, channel_iter_12, super_iter_12, rate_r1};

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_12 = {{{.dimension=2, .stride=ifmsv_dim_12[2], .wrap=depth_iter_12}}};
std::vector<traversing_parameters> ofm_access_pattern_12 = {{.dimension=2, .stride=ofmsv_dim_12[2]*NUM_ROWS, .wrap=channel_iter_12}};

resnet18_tiling layer12 = resnet18_tiling(filt_k3, iters_12, ifm_dim_12, ifmsv_dim_12, ifm_access_pattern_12, ifm_boundary_dim_12, wts_dim_12, wtssv_dim_12, ofm_dim_12, ofmsv_dim_12, ofm_access_pattern_12, {});

//Layer13
//Same as layer11 use layer11 tiling etc
std::vector<uint32_t> ofm_dim_13 = ofm_dim_11;

//Layer14
uint32_t width_iter_14 = 1; uint32_t depth_iter_14 = 8; uint32_t height_iter_14 = 1; uint32_t channel_iter_14 = 4; uint32_t super_iter_14 = 4;
std::vector<uint32_t> ifm_dim_14 = ofm_dim_13; std::vector<uint32_t> ifm_boundary_dim_14 = ifm_boundary_dim_12;
std::vector<uint32_t> ofm_dim_14 = {8, BATCH_SIZE*8, 64, 7};
std::vector<uint32_t> ifmsv_dim_14 = {8, BATCH_SIZE*18, 4, 4}; std::vector<uint32_t> wtssv_dim_14 = {9536}; std::vector<uint32_t> ofmsv_dim_14 = {8, BATCH_SIZE*8, 4, 1};
std::vector<uint32_t> wts_ddr_dim_14 = {wtssv_dim_14[0]*depth_iter_14*NUM_ROWS*channel_iter_14};
std::vector<uint32_t> wts_dim_14 = {wtssv_dim_14[0]*depth_iter_14*NUM_ROWS*channel_iter_14/super_iter_14};
std::vector<uint32_t> iters_14 = {height_iter_14, width_iter_14, depth_iter_14, channel_iter_14, super_iter_14, rate_r2};

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_14 = {{{.dimension=2, .stride=ifmsv_dim_14[2], .wrap=depth_iter_14}}};
std::vector<traversing_parameters> ofm_access_pattern_14 = {{.dimension=2, .stride=ofmsv_dim_14[2]*NUM_ROWS, .wrap=channel_iter_14}};

resnet18_tiling layer14 = resnet18_tiling(filt_k3, iters_14, ifm_dim_14, ifmsv_dim_14, ifm_access_pattern_14, ifm_boundary_dim_14, wts_dim_14, wtssv_dim_14, ofm_dim_14, ofmsv_dim_14, ofm_access_pattern_14, {});


//Layer14a
uint32_t width_iter_14a = 1; uint32_t depth_iter_14a = 4; uint32_t height_iter_14a = 1; uint32_t channel_iter_14a = 2; uint32_t super_iter_14a = 1;
std::vector<uint32_t> ifm_dim_14a = ofm_dim_13; std::vector<uint32_t> ifm_boundary_dim_14a = ifm_boundary_dim_12;
std::vector<uint32_t> ofm_dim_14a = {8, BATCH_SIZE*8, 64, 7};
std::vector<uint32_t> ifmsv_dim_14a = {8, BATCH_SIZE*16, 8, 2}; std::vector<uint32_t> wtssv_dim_14a = {4672}; std::vector<uint32_t> ofmsv_dim_14a = {8, BATCH_SIZE*8, 8, 1};
std::vector<uint32_t> wts_dim_14a = {wtssv_dim_14a[0]*depth_iter_14a*NUM_ROWS*channel_iter_14a};
std::vector<uint32_t> iters_14a = {height_iter_14a, width_iter_14a, depth_iter_14a, channel_iter_14a, super_iter_14a, rate_r2};

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_14a = {{{.dimension=2, .stride=ifmsv_dim_14a[2], .wrap=depth_iter_14a}}};
std::vector<traversing_parameters> ofm_access_pattern_14a = {{.dimension=2, .stride=ofmsv_dim_14a[2]*NUM_ROWS, .wrap=channel_iter_14a}};

resnet18_tiling layer14a = resnet18_tiling(filt_k1, iters_14a, ifm_dim_14a, ifmsv_dim_14a, ifm_access_pattern_14a, ifm_boundary_dim_14a, wts_dim_14a, wtssv_dim_14a, ofm_dim_14a, ofmsv_dim_14a, ofm_access_pattern_14a, {});


//Layer15
uint32_t width_iter_15 = 1; uint32_t depth_iter_15 = 16; uint32_t height_iter_15 = 1; uint32_t channel_iter_15 = 4; uint32_t super_iter_15 = 4;
std::vector<uint32_t> channel_iter_ph_15 = {2, 2};
std::vector<uint32_t> ifm_dim_15 = ofm_dim_14; std::vector<uint32_t> ifm_boundary_dim_15 = {8, BATCH_SIZE*7, 64, 7};
std::vector<uint32_t> ofm_dim_15 = {8, BATCH_SIZE*8, 64, 7};
std::vector<uint32_t> ifmsv_dim_15 = {8, BATCH_SIZE*10, 4, 3}; std::vector<uint32_t> wtssv_dim_15 = {9536}; std::vector<uint32_t> ofmsv_dim_15 = {8, BATCH_SIZE*8, 4, 1};
std::vector<uint32_t> wts_ddr_dim_15 = {wtssv_dim_15[0]*depth_iter_15*NUM_ROWS*channel_iter_15};
std::vector<uint32_t> wts_dim_15 = {wtssv_dim_15[0]*depth_iter_15*NUM_ROWS*channel_iter_15/super_iter_15};
std::vector<uint32_t> iters_15 = {height_iter_15, width_iter_15, depth_iter_15, channel_iter_15, super_iter_15, rate_r1};

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_15 = {{{.dimension=2, .stride=ifmsv_dim_15[2], .wrap=depth_iter_15}, {.dimension=1, .stride=0, .wrap=channel_iter_ph_15[0]}},
                                                                         {{.dimension=2, .stride=ifmsv_dim_15[2], .wrap=depth_iter_15}, {.dimension=1, .stride=0, .wrap=channel_iter_ph_15[1]}}};
std::vector<traversing_parameters> ofm_access_pattern_15 = {{.dimension=2, .stride=ofmsv_dim_15[2]*NUM_ROWS, .wrap=channel_iter_ph_15[0]}, {.dimension=2, .stride=ofmsv_dim_15[2]*NUM_ROWS, .wrap=channel_iter_ph_15[1]}};

std::vector<uint32_t> ifm2_dim_15 = ofm_dim_15;
std::vector<uint32_t> ifm2sv_dim_15 = {8, BATCH_SIZE*8, 16, 1};
std::vector<traversing_parameters> ifm2_access_pattern_15 = {{.dimension=2, .stride=ofmsv_dim_15[2]*NUM_ROWS, .wrap=channel_iter_ph_15[0]}, {.dimension=2, .stride=ofmsv_dim_15[2]*NUM_ROWS, .wrap=channel_iter_ph_15[1]}};
std::vector<uint32_t> channel_split_15 = {2, ofmsv_dim_15[2]*NUM_ROWS*channel_iter_15/2};

resnet18_tiling layer15 = resnet18_tiling(filt_k3, iters_15, ifm_dim_15, ifmsv_dim_15, ifm_access_pattern_15, ifm_boundary_dim_15, wts_dim_15, wtssv_dim_15, ofm_dim_15, ofmsv_dim_15, ofm_access_pattern_15,
		                          ifm2sv_dim_15, ifm2_access_pattern_15, channel_split_15, false, false, ifm2_dim_15, 2);


//Layer16
uint32_t width_iter_16 = 1; uint32_t depth_iter_16 = 16; uint32_t height_iter_16 = 1; uint32_t channel_iter_16 = 4; uint32_t super_iter_16 = 4;
std::vector<uint32_t> ifm_dim_16 = ofm_dim_15; std::vector<uint32_t> ifm_boundary_dim_16 = {8, BATCH_SIZE*7, 64, 7};
std::vector<uint32_t> ofm_dim_16 = {8, BATCH_SIZE*8, 64, 7};
std::vector<uint32_t> ifmsv_dim_16 = {8, BATCH_SIZE*10, 4, 3}; std::vector<uint32_t> wtssv_dim_16 = {9536}; std::vector<uint32_t> ofmsv_dim_16 = {8, BATCH_SIZE*8, 4, 1};
std::vector<uint32_t> wts_ddr_dim_16 = {wtssv_dim_16[0]*depth_iter_16*NUM_ROWS*channel_iter_16};
std::vector<uint32_t> wts_dim_16 = {wtssv_dim_16[0]*depth_iter_16*NUM_ROWS*channel_iter_16/super_iter_16};
std::vector<uint32_t> iters_16 = {height_iter_16, width_iter_16, depth_iter_16, channel_iter_16, super_iter_16, rate_r1};

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_16 = {{{.dimension=2, .stride=ifmsv_dim_16[2], .wrap=depth_iter_16}}};
std::vector<traversing_parameters> ofm_access_pattern_16 = {{.dimension=2, .stride=ofmsv_dim_16[2]*NUM_ROWS, .wrap=channel_iter_16}};

resnet18_tiling layer16 = resnet18_tiling(filt_k3, iters_16, ifm_dim_16, ifmsv_dim_16, ifm_access_pattern_16, ifm_boundary_dim_16, wts_dim_16, wtssv_dim_16, ofm_dim_16, ofmsv_dim_16, ofm_access_pattern_16, {});

//Layer17
//Same as layer15 use layer15 tiling etc
std::vector<uint32_t> ofm_dim_17 = {8, BATCH_SIZE*1, 64, 7};

resnet18_tiling layer17 = resnet18_tiling(filt_k3, iters_15, ifm_dim_15, ifmsv_dim_15, ifm_access_pattern_15, ifm_boundary_dim_15, wts_dim_15, wtssv_dim_15, ofm_dim_15, ofmsv_dim_15, ofm_access_pattern_15,
		                          ifm2sv_dim_15, ifm2_access_pattern_15, channel_split_15, true, false, ifm2_dim_15, 2);

//Layer18-FC
//wts is layer17's ofm (1/7th) and ifm is weights of the GEMM
uint32_t width_iter_18 = 1; uint32_t depth_iter_18 = 1; uint32_t height_iter_18 = 1; uint32_t channel_iter_18 = 16; uint32_t super_iter_18 = 1;
std::vector<uint32_t> ifm_dim_18 = {BATCH_SIZE*8, 448, 1, 1}; std::vector<uint32_t> ifm_boundary_dim_18 = {BATCH_SIZE*8, 448, 1, 1};
std::vector<uint32_t> wts_ddr_dim_18 = {659456};
std::vector<uint32_t> wts_dim_18 = {659456};
std::vector<uint32_t> ofm_dim_18 = {BATCH_SIZE*8, 1, 128*2, 7};   //16-bit
std::vector<uint32_t> ifmsv_dim_18 = {32, 10, 1, 1}; std::vector<uint32_t> wtssv_dim_18 = {10304}; std::vector<uint32_t> ofmsv_dim_18 = {BATCH_SIZE*8, 1, 2*2, 1};
std::vector<uint32_t> iters_18 = {height_iter_18, width_iter_18, depth_iter_18, channel_iter_18, super_iter_18, rate_r1};

std::vector<std::vector<traversing_parameters>> ifm_access_pattern_18 = {{}};
std::vector<traversing_parameters> ofm_access_pattern_18 = {{.dimension=2, .stride=ofmsv_dim_18[2], .wrap=channel_iter_18}};

resnet18_tiling layer18 = resnet18_tiling(filt_k1, iters_18, ifm_dim_18, ifmsv_dim_18, ifm_access_pattern_18, ifm_boundary_dim_18, wts_dim_18, wtssv_dim_18, ofm_dim_18, ofmsv_dim_18, ofm_access_pattern_18,
		                          {}, {}, {}, false, true);
//SoftMax overlay
std::vector<uint32_t> ifm_dim_sm = {56*256*BATCH_SIZE};
std::vector<uint32_t> ifmsv_dim_sm = {BATCH_SIZE*1024*2};
std::vector<uint32_t> ofm_dim_sm = {BATCH_SIZE*1024*2,1,1,1};

access_pattern ifm_pattern_sm = tiling({.buffer_dimension = ifm_dim_sm, .tiling_dimension = ifmsv_dim_sm, .offset = {12288*BATCH_SIZE}});
access_pattern ofm_pattern_sm = tiling({.buffer_dimension = ofm_dim_sm, .tiling_dimension = ofm_dim_sm, .offset = {0,0,0,0}});

//Input and Output specs
/*
std::vector<location_constraint> input_loc = ofm_loc_17;
std::vector<uint32_t> input_dim = ofm_dim_17; //D0=0:R, D0=1:G, D0=2:B, D0=3:zero-padded to make D0 4 bytes
std::vector<uint32_t> output_dim = ofm_dim_sm; //ofm_dim_18;
*/

/*
std::vector<location_constraint> input_loc = ifm_loc_1;
std::vector<uint32_t> input_dim = ifm_dim_1; //D0=0:R, D0=1:G, D0=2:B, D0=3:zero-padded to make D0 4 bytes
std::vector<uint32_t> output_dim = ofm_dim_sm;
*/


//WTS DDR Offsets
//std::vector<uint32_t> wts_ddr_offset = {0, 3776, 13760, 23744, 33728, 43712, 64704, 67392, 106816, 146240, 185664, 264512, 275008, 432704, 590400, 748096, 1063488, 1100864, 1416256, 1731648, 2362432, 2677824, 2993216};
std::vector<uint32_t> wts_ddr_offset = {0, 3776, 13376, 22976, 32576, 42176, 61248, 63936, 102336, 140736, 179136, 255424, 265920, 419520, 572096, 725696, 1030848, 1068224, 1678528, 2288832, 2899136};

//Input
      std::vector<uint32_t> input_dim = ifm_dim_1;
      std::vector<uint32_t> output_dim = ofm_dim_sm;

//access_pattern input_ddr_pattern = tiling({.buffer_dimension = input_dim, .tiling_dimension = input_dim, .offset = {0, 0, 0, 0}});
//std::vector<access_pattern> input_ddr_pattern = {tiling({.buffer_dimension = input_dim, .tiling_dimension = {input_dim[0], input_dim[1], input_dim[2], input_dim[3]/2}, .offset = {0, 0, 0, 0}}),
//                                                 tiling({.buffer_dimension = input_dim, .tiling_dimension = {input_dim[0], input_dim[1], input_dim[2], input_dim[3]/2}, .offset = {0, 0, 0, (int)input_dim[3]/2}})};
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
                                        //layer15_1,
                                        layer16,
                                        layer17,
                                       // layer17_1,
                                        layer18
                                     };

template<int COL_OF, int ROW_OF, int memRowId>
class resnet_locations{

   public:

std::vector<location_constraint> ifm_loc_1 = {address(2+COL_OF, memRowId, 129024)};
std::vector<location_constraint> ofm_loc_1 = {address(1+COL_OF, memRowId, 251904)};
std::vector<location_constraint> wts_loc_1 = {address(5+COL_OF,memRowId,0)} ;

      std::vector<location_constraint> ifm_loc_2 = ofm_loc_1; //{address(1+COL_OF, 0, m1_delta0)};
      std::vector<location_constraint> ofm_loc_2 = ifm_loc_1; //{address(2+COL_OF, 0, m2_delta0)};
      std::vector<location_constraint> wts_loc_2 = {address(4+COL_OF, memRowId, 0)};

      std::vector<location_constraint> ofm_loc_3 = ifm_loc_2;
      std::vector<location_constraint> wts_loc_3 = {address(5+COL_OF, memRowId, 0)};


      std::vector<location_constraint> ofm_loc_4 = ofm_loc_2;
      std::vector<location_constraint> wts_loc_4 = {address(6+COL_OF, memRowId, 0)};


      std::vector<location_constraint> ofm_loc_5 = ofm_loc_3;
      std::vector<location_constraint> wts_loc_5 = {address(4+COL_OF, memRowId, 0)};;


      std::vector<location_constraint> ofm_loc_6 = ofm_loc_2;
      std::vector<location_constraint> wts_loc_6 = {address(5+COL_OF, memRowId, 0)};


      std::vector<location_constraint> ofm_loc_6a = {address(2+COL_OF, memRowId, 358400)};
      std::vector<location_constraint> wts_loc_6a = {address(6+COL_OF, memRowId, 0)};


      std::vector<location_constraint> ofm_loc_7 = {address(1+COL_OF, memRowId, m1_delta0)};
      std::vector<location_constraint> wts_loc_7 = {address(4+COL_OF, memRowId, 0)};


      std::vector<location_constraint> ofm_loc_8 = {address(2+COL_OF, memRowId, m2_delta0)} ;
      std::vector<location_constraint> wts_loc_8 = {address(5+COL_OF, memRowId, 0)};


      std::vector<location_constraint> ofm_loc_9 = {address(1+COL_OF, memRowId, m1_delta0)} ;
      std::vector<location_constraint> wts_loc_9 = {address(6+COL_OF, memRowId, 0)};


      std::vector<location_constraint> ofm_loc_10 = {address(2+COL_OF, memRowId, m2_delta0)} ;
      std::vector<location_constraint> wts_loc_10 = {address(4+COL_OF, memRowId, 0)};


      std::vector<location_constraint> ofm_loc_10a = {address(2+COL_OF, memRowId, m2_delta00)} ;
      std::vector<location_constraint> wts_loc_10a = {address(6+COL_OF, memRowId, 0)};


      std::vector<location_constraint> ofm_loc_11 = {address(2+COL_OF, memRowId, m2_delta1)};
      std::vector<location_constraint> wts_loc_11 = {address(4+COL_OF, memRowId, 0)};


      std::vector<location_constraint> ofm_loc_12 = {address(2+COL_OF, memRowId, m2_delta0)};
      std::vector<location_constraint> wts_loc_12 = {address(5+COL_OF, memRowId, 90112)};


      std::vector<location_constraint> ofm_loc_13 = {address(2+COL_OF, memRowId, m2_delta00)};
      std::vector<location_constraint> wts_loc_13 = {address(4+COL_OF, memRowId, 0)};


      std::vector<location_constraint> ofm_loc_14 = {address(2+COL_OF, memRowId, m2_delta0)} ;
      std::vector<location_constraint> wts_loc_14 = {address(5+COL_OF, memRowId, 90112), address(5+COL_OF, memRowId, 395264)};


      std::vector<location_constraint> ofm_loc_14a = {address(2+COL_OF, memRowId, m2_delta1)} ;
      std::vector<location_constraint> wts_loc_14a = {address(6+COL_OF, memRowId, 0)};//374784)};


      std::vector<location_constraint> ofm_loc_15 = {address(2+COL_OF, memRowId, m2_delta00)};
      std::vector<location_constraint> wts_loc_15 = {address(4+COL_OF, memRowId, 0), address(5+COL_OF, memRowId, 86016)};


      std::vector<location_constraint> ofm_loc_16 = {address(2+COL_OF, memRowId, m2_delta0)};
      std::vector<location_constraint> wts_loc_16 = wts_loc_15;


      std::vector<location_constraint> ofm_loc_17 = {address(2+COL_OF, memRowId, m2_delta1)};

      std::vector<location_constraint> wts_loc_18 = {address(4+COL_OF, memRowId, 0)};     //for ifm (wts of FC)
      std::vector<location_constraint> ofm_loc_18 = {address(2+COL_OF, memRowId, m2_delta00)};

      std::vector<location_constraint> ofm_loc_sm = {address(3+COL_OF, memRowId, 0)};

      std::vector<location_constraint> input_loc = ifm_loc_1;

      //resnet_locations(){
      //std::cout<<"This col offset is"<<COL_OF<<std::endl;
      //}
};
