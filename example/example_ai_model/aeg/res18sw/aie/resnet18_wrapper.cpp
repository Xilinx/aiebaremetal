// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
/*  (c) Copyright 2019 - 2021 Xilinx, Inc. All rights reserved.

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
    PART OF THIS FILE AT ALL TIMES.                       
*/

#include <adf.h>
#include "mllib_config.h"
#include "mllib_kernels.h"

// Parameters used to configure kernels at run time
conv2d_params_t    curr_layer;
maxpool2d_params_t curr_layer_maxpool;
pad2d_params_t curr_layer_pad2d;
add2d_params_t curr_layer_add2d; // params for elt-wise add
globalavgpool2d_params_t curr_layer_gap;

static int curr_iter = 0 ;
static int depth_iter = 0 ;
static int num_depth_iter = 1;  //default
static int num_iter = 1;        //default

// Total iters counter, helps to know inference boundary (very first iter)
static int total_iters = 0;

static int core_id = 0;
static uint16_t core_col = 0;

// Init function for rounding and saturation
void node_init() {
    tile::current().set_saturation(saturation_mode::saturate);  // Fixed: was truncate, causing overflow wrap
    tile::current().set_rounding(rounding_mode::conv_even);
    core_id = get_coreid();
    core_col = (uint16_t) (core_id>>16 & 0xff) ;
}

void resnet18_wrapper
(
        adf::input_buffer_conf<DTYPE_IN, IFM_BPC_TYPE>  & __restrict ifm,
        adf::input_buffer_conf<DTYPE_WTS, WTS_BPC_TYPE>  & __restrict wts,
        adf::output_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> & __restrict ofm
) {

    total_iters++ ;

    if(curr_iter==0){
          
        // Conditionally acquire kernel parameters only at the start of first iteration
        // Setup increments and other kernel params 
        // Do this only in the beginning (first iteration)
        setup_conv2d_params((int8 *)wts.data(), curr_layer, BATCH_SIZE);

        num_depth_iter = curr_layer.num_ifm_depth_iter;

        int num_iter_out = curr_layer.num_width_iter*curr_layer.num_height_iter*curr_layer.num_chout_iter;
        num_iter = num_iter_out*curr_layer.num_ifm_depth_iter;

        if(curr_layer.op_mode == CONV2D_LAYER_1){
            setup_maxpool2d_params(((int8_t *)wts.data())+LAYER_OFFSET, curr_layer_maxpool, BATCH_SIZE); 
            setup_pad2d_params((int8 *)wts.data()+PAD2D_OFFSET, curr_layer_pad2d, BATCH_SIZE);
            //int core_id = get_coreid();
            //uint16_t core_col = (uint16_t) (core_id>>16 & 0xff) ; 
            curr_layer_pad2d.pad_top = (core_col==0)?curr_layer_pad2d.pad_top:0; //no top padding for cores on column>0
        }
        
    }
   
    //if((curr_iter & 0xff)==0) // IFM iteration boundary
    //   config_bd_enq(&ifm,ifm_cfg,mllib_min(num_iter,IFM_ITER_MAX)); // TODO: num_iter is assumed to be multiple of IFM_ITER_MAX or less

    /*   
    int xsize = curr_layer.oxp+2+2;
    int ysize = curr_layer.oyp+2;
    int n_ifms = curr_layer.ifm;
    int temp  = curr_layer.num_Co+1;
    int n_ofms = (curr_layer.conv_type==CONV2D_LAYER1)? (temp<<3) : (temp<<4) ;
    int *rd_ptr = (int *) ifm.data() ;
    int core_id = get_coreid();
    int *wtsrd_ptr = (int *) wts.data() ;
    int *ofm_ptr = (int *) ofm.data() ;

    
    printf("Core(%d,%d): [curr,depth,num_depth,num_iter]=[%d,%d,%d,%d]\n",(core_id>>16 & 0xff),(core_id & 0xff),curr_iter,depth_iter,num_depth_iter,num_iter);
    printf("Core(%d,%d): ifm_sv(h,w,d,c): %dx%dx%dx%d\n",(core_id>>16 & 0xff),(core_id & 0xff),ysize,xsize,n_ifms,n_ofms);	 
    for(int i=0; i<(xsize*ysize*n_ifms)>>2; i++)
       printf("Core(%d,%d): IFM[%d]=%x\n",(core_id>>16 & 0xff),(core_id & 0xff),i,*rd_ptr++);	 

    for(int i=0; i<(9*n_ifms*n_ofms+64+n_ofms*8)>>2; i++)
       printf("Core(%d,%d): WTS[%d]=%x\n",(core_id>>16 & 0xff),(core_id & 0xff),i,*wtsrd_ptr++);
    */	

    depth_iter++;

    // Acquire OFM at the beginning of depth_iter
    if(depth_iter==1)
        ofm.acquire();

    //printf("Core(%d,%d): Obtained OFM ProducerLock!!\n",(core_id>>16 & 0xff),(core_id & 0xff));	

    conv2d<BATCH_SIZE, ACT_TYPE, DTYPE_IN ,DTYPE_WTS, DTYPE_OUT, IFM_BPC_TYPE, WTS_BPC_TYPE, OFM_BPC_TYPE>(ifm, wts, ofm, curr_layer);

    /*
    for(int i=0; i<curr_layer.oxp*curr_layer.oyp*n_ofms/4; i++)
       printf("Core(%d,%d): OFM[%d]=%x\n",(core_id>>16 & 0xff),(core_id & 0xff),i,*ofm_ptr++);	
    */ 

    curr_iter++;

    // Release OFM at the end of depth_iter
    if(depth_iter==num_depth_iter){
       if(curr_layer.op_mode == CONV2D_LAYER_1){
          pad2d<BATCH_SIZE, DTYPE_OUT, OFM_BPC_TYPE, OFM_BPC_TYPE>((adf::input_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> &)ofm, ofm, curr_layer_pad2d);
          maxpool2d<BATCH_SIZE, DTYPE_OUT, OFM_BPC_TYPE, OFM_BPC_TYPE>((adf::input_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> &)ofm, ofm, curr_layer_maxpool);
          // switch off padding for top after the first iteration
          curr_layer_pad2d.pad_top=0;
       }

       ofm.release();
       depth_iter=0; // Reset depth_iter
    }   

    if(curr_iter==num_iter)
       curr_iter=0;  // Reset curr_iter 
    printf("restnet18 rwapper done \n");		
}

void resnet18_wrapper_eltwise
(
        adf::input_buffer_conf<DTYPE_IN, IFM_BPC_TYPE>   & __restrict ifm,
        adf::input_buffer_conf<DTYPE_WTS, WTS_BPC_TYPE>  & __restrict wts,
        adf::input_buffer_conf<DTYPE_IN, IFM2_BPC_TYPE>  & __restrict ifm2,
        adf::output_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> & __restrict ofm        
) {

    total_iters++ ;

    if(curr_iter==0){      
        // Conditionally acquire kernel parameters only at the start of first iteration
        // Setup increments and other kernel params 
        // Do this only in the beginning (first iteration)
        setup_conv2d_params((int8 *)wts.data(), curr_layer, BATCH_SIZE);

        num_depth_iter = curr_layer.num_ifm_depth_iter;

        int num_iter_out = curr_layer.num_width_iter*curr_layer.num_height_iter*curr_layer.num_chout_iter;
        num_iter = num_iter_out*curr_layer.num_ifm_depth_iter;
        
        // Setup elt-wise add params
        setup_add2d_params(curr_layer_add2d, curr_layer) ;
        /*
        curr_layer_add2d.offset = (curr_layer.row_offset)*(curr_layer.ofm_len);       // Pick up data from IFM2 from 0 offset (dedicated IFM2 port for residual input)
        curr_layer_add2d.num_inputs = (curr_layer.ofm_len)>>5; // divide num of elems in OFM by 32 to account for the 32-lane concurrent addition using accumulator 
        curr_layer_add2d.shift_in = curr_layer.shift_lrelu_out;// shift_in is required to bring left side (8-bit) to same scale as right side (16-bit) 
                                                         // re-use shift_lrelu_out field to convey this shift, current layer is in 16-bit output mode
        curr_layer_add2d.shift_out = curr_layer.shift_out; //Final shift from 16-bit to 8-bit 
        curr_layer_add2d.in0_sign = 0; // unsigned input from left (needed for sign extension of accumulator), right side is always signed
        curr_layer_add2d.act =  RELU;  // fixed to ReLU, all elt wise outputs are RELU in ResNet18
        */
        
    }

    depth_iter++;

    // Acquire OFM at the beginning of depth_iter
    if(depth_iter==1)
        ofm.acquire();

    conv2d<BATCH_SIZE, ACT_TYPE, DTYPE_IN, DTYPE_WTS, DTYPE_OUT, IFM_BPC_TYPE, WTS_BPC_TYPE, OFM_BPC_TYPE>(ifm, wts, ofm, curr_layer);

    curr_iter++;

    // Release OFM at the end of depth_iter
    if(depth_iter==num_depth_iter){
       ifm2.acquire();
       //Elt-wise
       // Element-wise add for IFM2 and 16-bit output from OFM buffer
       // DTYPE_IN/OUT are for buffer port type matching and does not affect internal compute
       /*
       printf("\n core=%d, offset=%d\n", get_coreid(),curr_layer_add2d.offset);
       for(unsigned k=0;k<32;k++){
           printf("\n core=%d, index=%d value in residual 8-bit=%x\n", get_coreid(),k,*(((uint8_t*)ifm2.data())+k));
           printf("\n core=%d, index=%d value in conv2d 16-bit=%x\n", get_coreid(),k,*(((int16_t*)ofm.data())+k));
       }*/

       add2d<DTYPE_IN, DTYPE_OUT, IFM2_BPC_TYPE, OFM_BPC_TYPE>(ifm2, ofm, curr_layer_add2d);
       /*
       for(unsigned k=0;k<32;k++){
           printf("\n core=%d, index=%d value out=%x\n", get_coreid(),k,*(((uint8_t*)ofm.data())+k));
       }
       */
       
       ifm2.release();       
       ofm.release();
       depth_iter=0; // Reset depth_iter
    }   

    if(curr_iter==num_iter)
       curr_iter=0;  // Reset curr_iter 

}	

// Use *wrapper_casc for FC
void resnet18_wrapper_casc_start
(
        adf::input_buffer_conf<DTYPE_IN, IFM_BPC_TYPE>  & __restrict ifm,
        adf::input_buffer_conf<DTYPE_WTS, WTS_BPC_TYPE>  & __restrict wts,
        adf::output_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> & __restrict ofm,
	OUT_CASC<acc32> *casc_acc_out
) {

    total_iters++ ;

    if(curr_iter==0){
          
        // Conditionally acquire kernel parameters only at the start of first iteration
        // Setup increments and other kernel params 
        // Do this only in the beginning (first iteration)
        // IFM and wts are exchanged for this layer
        setup_conv2d_params((int8 *)wts.data(), curr_layer, BATCH_SIZE);
        //int core_id = get_coreid();
        //uint16_t core_col = (uint16_t) ((core_id>>16) & 0xff) ; 
        if(curr_layer.op_mode == FC){
           update_offsets((int8 *)wts.data(), curr_layer,core_col);
        }
        num_depth_iter = curr_layer.num_ifm_depth_iter;

        int num_iter_out = curr_layer.num_width_iter*curr_layer.num_height_iter*curr_layer.num_chout_iter;
        num_iter = num_iter_out*curr_layer.num_ifm_depth_iter;
        
    }
   
    depth_iter++;

    // Acquire OFM at the beginning of depth_iter
    if(depth_iter==1)
        ofm.acquire();

    conv2d<BATCH_SIZE, ACT_TYPE, DTYPE_IN ,DTYPE_WTS, DTYPE_OUT, IFM_BPC_TYPE, WTS_BPC_TYPE, OFM_BPC_TYPE>(ifm, wts, ofm, curr_layer);

    //casc_write(8,(int16 *) ofm.data(),casc_acc_out); // write out 8 accs, only 5 needed for the SV 

    curr_iter++;

    // Release OFM at the end of depth_iter
    if(depth_iter==num_depth_iter){
       if(curr_layer.op_mode == FC){
          casc_write(8,(int16 *) ofm.data(),casc_acc_out);
       }
       ofm.release();
       depth_iter=0; // Reset depth_iter
    }   

    if(curr_iter==num_iter)
       curr_iter=0;  // Reset curr_iter    
}

void resnet18_wrapper_casc_end
(
        adf::input_buffer_conf<DTYPE_IN, IFM_BPC_TYPE>  & __restrict ifm,
        adf::input_buffer_conf<DTYPE_WTS, WTS_BPC_TYPE>  & __restrict wts,
	IN_CASC<acc32> *casc_acc_in,
        adf::output_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> & __restrict ofm
) {

    total_iters++ ;

    if(curr_iter==0){
          
        // Conditionally acquire kernel parameters only at the start of first iteration
        // Setup increments and other kernel params 
        // Do this only in the beginning (first iteration)
        // IFM and wts are exchanged for this layer
        setup_conv2d_params((int8 *)wts.data(), curr_layer, BATCH_SIZE);
        //int core_id = get_coreid();
        //uint16_t core_col = (uint16_t) ((core_id>>16) & 0xff) ; 
        if(curr_layer.op_mode == FC){
           update_offsets((int8 *)wts.data(), curr_layer,core_col);
        }
        num_depth_iter = curr_layer.num_ifm_depth_iter;

        int num_iter_out = curr_layer.num_width_iter*curr_layer.num_height_iter*curr_layer.num_chout_iter;
        num_iter = num_iter_out*curr_layer.num_ifm_depth_iter;
        
    }
   
    depth_iter++;

    // Acquire OFM at the beginning of depth_iter
    if(depth_iter==1)
        ofm.acquire();

    //casc_read(8,(int16 *)ofm.data(),casc_acc_in); 

    conv2d<BATCH_SIZE, ACT_TYPE, DTYPE_IN ,DTYPE_WTS, DTYPE_OUT, IFM_BPC_TYPE, WTS_BPC_TYPE, OFM_BPC_TYPE>(ifm, wts, ofm, curr_layer);

    curr_iter++;

    // Release OFM at the end of depth_iter
    if(depth_iter==num_depth_iter){
       // Perform post processing to pick up and join 8+8=16 OFMs into a contiguos block, these are placed in acc0 and acc4
       if(curr_layer.op_mode == FC){
          casc_read(8,(int16 *)ofm.data(),curr_layer.shift_out,casc_acc_in);
          //fc_post_process<DTYPE_OUT>((int *)ofm.data(),curr_layer);
          fc_post_process<DTYPE_OUT,BATCH_SIZE,1>((int *)ofm.data(),curr_layer);
       }
       ofm.release();
       depth_iter=0; // Reset depth_iter
     }

    if(curr_iter==num_iter)
       curr_iter=0;  // Reset curr_iter    
}

void resnet18_wrapper_casc_mid
(
        adf::input_buffer_conf<DTYPE_IN, IFM_BPC_TYPE>  & __restrict ifm,
        adf::input_buffer_conf<DTYPE_WTS, WTS_BPC_TYPE>  & __restrict wts,
	IN_CASC<acc32> *casc_acc_in,
        adf::output_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> & __restrict ofm,
	OUT_CASC<acc32> *casc_acc_out
) {

    total_iters++ ;

    if(curr_iter==0){
          
        // Conditionally acquire kernel parameters only at the start of first iteration
        // Setup increments and other kernel params 
        // Do this only in the beginning (first iteration)
        // IFM and wts are exchanged for this layer
        setup_conv2d_params((int8 *)wts.data(), curr_layer, BATCH_SIZE);
        //int core_id = get_coreid();
        //uint16_t core_col = (uint16_t) ((core_id>>16) & 0xff) ; 
        if(curr_layer.op_mode == FC){
           update_offsets((int8 *)wts.data(), curr_layer,core_col);
        }
        num_depth_iter = curr_layer.num_ifm_depth_iter;

        int num_iter_out = curr_layer.num_width_iter*curr_layer.num_height_iter*curr_layer.num_chout_iter;
        num_iter = num_iter_out*curr_layer.num_ifm_depth_iter;
        
    }
   
    depth_iter++;

    // Acquire OFM at the beginning of depth_iter
    if(depth_iter==1)
        ofm.acquire();

    //casc_read(8,(int16 *)ofm.data(),casc_acc_in); 

    conv2d<BATCH_SIZE, ACT_TYPE, DTYPE_IN ,DTYPE_WTS, DTYPE_OUT, IFM_BPC_TYPE, WTS_BPC_TYPE, OFM_BPC_TYPE>(ifm, wts, ofm, curr_layer);

    //casc_write(8,(int16 *) ofm.data(),casc_acc_out); 

    curr_iter++;

    // Release OFM at the end of depth_iter
    if(depth_iter==num_depth_iter){
       if(curr_layer.op_mode == FC){
          casc_read(8,(int16 *)ofm.data(),0,casc_acc_in);
          casc_write(8,(int16 *) ofm.data(),casc_acc_out);
       }
       ofm.release();
       depth_iter=0; // Reset depth_iter
    }   

    if(curr_iter==num_iter)
       curr_iter=0;  // Reset curr_iter    
}

// Use _eltwise_casc_* for eltwise, and global avg pool
void resnet18_wrapper_eltwise_casc_start
(
        adf::input_buffer_conf<DTYPE_IN, IFM_BPC_TYPE>   & __restrict ifm,
        adf::input_buffer_conf<DTYPE_WTS, WTS_BPC_TYPE>  & __restrict wts,
        adf::input_buffer_conf<DTYPE_IN, IFM2_BPC_TYPE>  & __restrict ifm2,
        adf::output_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> & __restrict ofm,
	OUT_CASC<acc32> *casc_acc_out
) {

    total_iters++ ;

    if(curr_iter==0){      
        // Conditionally acquire kernel parameters only at the start of first iteration
        // Setup increments and other kernel params 
        // Do this only in the beginning (first iteration)
        setup_conv2d_params((int8 *)wts.data(), curr_layer, BATCH_SIZE);

        num_depth_iter = curr_layer.num_ifm_depth_iter;

        int num_iter_out = curr_layer.num_width_iter*curr_layer.num_height_iter*curr_layer.num_chout_iter;
        num_iter = num_iter_out*curr_layer.num_ifm_depth_iter;
        
        setup_add2d_params(curr_layer_add2d, curr_layer) ;

        if(curr_layer.op_mode == CONV2D_ADD2D_GAP){
             setup_globalavgpool2d_params((int8 *)wts.data()+GLOBAL_AVG_POOL_OFFSET, curr_layer_gap, BATCH_SIZE);
        }
    }

    depth_iter++;

    // Acquire OFM at the beginning of depth_iter
    if(depth_iter==1)
        ofm.acquire();

    conv2d<BATCH_SIZE, ACT_TYPE, DTYPE_IN, DTYPE_WTS, DTYPE_OUT, IFM_BPC_TYPE, WTS_BPC_TYPE, OFM_BPC_TYPE>(ifm, wts, ofm, curr_layer);

    curr_iter++;

    // Release OFM at the end of depth_iter
    if(depth_iter==num_depth_iter){
       ifm2.acquire();

       add2d<DTYPE_IN, DTYPE_OUT, IFM2_BPC_TYPE, OFM_BPC_TYPE>(ifm2, ofm, curr_layer_add2d);

       if(curr_layer.op_mode == CONV2D_ADD2D_GAP){
           IN_CASC<acc32> *casc_acc_in; // dummy for start node
           globalavgpool2d<BATCH_SIZE, CASC_START, DTYPE_OUT, DTYPE_OUT, OFM_BPC_TYPE, OFM_BPC_TYPE>((adf::input_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> &)ofm,ofm,curr_layer_gap,casc_acc_in,casc_acc_out);
       }

       ifm2.release();       
       ofm.release();
       depth_iter=0; // Reset depth_iter
    }   

    if(curr_iter==num_iter)
       curr_iter=0;  // Reset curr_iter 
}	

void resnet18_wrapper_eltwise_casc_end
(
        adf::input_buffer_conf<DTYPE_IN, IFM_BPC_TYPE>   & __restrict ifm,
        adf::input_buffer_conf<DTYPE_WTS, WTS_BPC_TYPE>  & __restrict wts,
        adf::input_buffer_conf<DTYPE_IN, IFM2_BPC_TYPE>  & __restrict ifm2,
	IN_CASC<acc32> *casc_acc_in,
        adf::output_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> & __restrict ofm
) {

    total_iters++ ;

    if(curr_iter==0){      
        // Conditionally acquire kernel parameters only at the start of first iteration
        // Setup increments and other kernel params 
        // Do this only in the beginning (first iteration)
        setup_conv2d_params((int8 *)wts.data(), curr_layer, BATCH_SIZE);

        num_depth_iter = curr_layer.num_ifm_depth_iter;

        int num_iter_out = curr_layer.num_width_iter*curr_layer.num_height_iter*curr_layer.num_chout_iter;
        num_iter = num_iter_out*curr_layer.num_ifm_depth_iter;
        
        setup_add2d_params(curr_layer_add2d, curr_layer) ;

        if(curr_layer.op_mode == CONV2D_ADD2D_GAP){
             setup_globalavgpool2d_params((int8 *)wts.data()+GLOBAL_AVG_POOL_OFFSET, curr_layer_gap, BATCH_SIZE);
        }
    }

    depth_iter++;

    // Acquire OFM at the beginning of depth_iter
    if(depth_iter==1)
        ofm.acquire();

    conv2d<BATCH_SIZE, ACT_TYPE, DTYPE_IN, DTYPE_WTS, DTYPE_OUT, IFM_BPC_TYPE, WTS_BPC_TYPE, OFM_BPC_TYPE>(ifm, wts, ofm, curr_layer);

    curr_iter++;

    // Release OFM at the end of depth_iter
    if(depth_iter==num_depth_iter){
       ifm2.acquire();

       add2d<DTYPE_IN, DTYPE_OUT, IFM2_BPC_TYPE, OFM_BPC_TYPE>(ifm2, ofm, curr_layer_add2d);

       if(curr_layer.op_mode == CONV2D_ADD2D_GAP){
           OUT_CASC<acc32> *casc_acc_out; // dummy for end node
           globalavgpool2d<BATCH_SIZE, CASC_END, DTYPE_OUT, DTYPE_OUT, OFM_BPC_TYPE, OFM_BPC_TYPE>((adf::input_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> &)ofm,ofm,curr_layer_gap,casc_acc_in,casc_acc_out);
       }

       ifm2.release();       
       ofm.release();
       depth_iter=0; // Reset depth_iter
    }   

    if(curr_iter==num_iter)
       curr_iter=0;  // Reset curr_iter 
}	

void resnet18_wrapper_eltwise_casc_mid
(
        adf::input_buffer_conf<DTYPE_IN, IFM_BPC_TYPE>   & __restrict ifm,
        adf::input_buffer_conf<DTYPE_WTS, WTS_BPC_TYPE>  & __restrict wts,
        adf::input_buffer_conf<DTYPE_IN, IFM2_BPC_TYPE>  & __restrict ifm2,
	IN_CASC<acc32> *casc_acc_in,
        adf::output_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> & __restrict ofm,
	OUT_CASC<acc32> *casc_acc_out
) {

    total_iters++ ;

    if(curr_iter==0){      
        // Conditionally acquire kernel parameters only at the start of first iteration
        // Setup increments and other kernel params 
        // Do this only in the beginning (first iteration)
        setup_conv2d_params((int8 *)wts.data(), curr_layer, BATCH_SIZE);

        num_depth_iter = curr_layer.num_ifm_depth_iter;

        int num_iter_out = curr_layer.num_width_iter*curr_layer.num_height_iter*curr_layer.num_chout_iter;
        num_iter = num_iter_out*curr_layer.num_ifm_depth_iter;
        
        setup_add2d_params(curr_layer_add2d, curr_layer) ;

        if(curr_layer.op_mode == CONV2D_ADD2D_GAP){
             setup_globalavgpool2d_params((int8 *)wts.data()+GLOBAL_AVG_POOL_OFFSET, curr_layer_gap, BATCH_SIZE);
        }
    }

    depth_iter++;

    // Acquire OFM at the beginning of depth_iter
    if(depth_iter==1)
        ofm.acquire();

    conv2d<BATCH_SIZE, ACT_TYPE, DTYPE_IN, DTYPE_WTS, DTYPE_OUT, IFM_BPC_TYPE, WTS_BPC_TYPE, OFM_BPC_TYPE>(ifm, wts, ofm, curr_layer);

    curr_iter++;

    // Release OFM at the end of depth_iter
    if(depth_iter==num_depth_iter){
       ifm2.acquire();

       add2d<DTYPE_IN, DTYPE_OUT, IFM2_BPC_TYPE, OFM_BPC_TYPE>(ifm2, ofm, curr_layer_add2d);

       if(curr_layer.op_mode == CONV2D_ADD2D_GAP){
           globalavgpool2d<BATCH_SIZE, CASC_MID, DTYPE_OUT, DTYPE_OUT, OFM_BPC_TYPE, OFM_BPC_TYPE>((adf::input_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> &)ofm,ofm,curr_layer_gap,casc_acc_in,casc_acc_out);
       }

       ifm2.release();       
       ofm.release();
       depth_iter=0; // Reset depth_iter
    }   

    if(curr_iter==num_iter)
       curr_iter=0;  // Reset curr_iter 
}	



