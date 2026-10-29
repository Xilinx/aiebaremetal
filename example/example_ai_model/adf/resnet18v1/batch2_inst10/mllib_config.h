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

#ifndef _MLLIB_CONFIG_H_
#define _MLLIB_CONFIG_H_

// mllib_const.h required for "act_t" definition
#include "mllib_const.h"

//#define MLLIB_WRAPPER_CFG  BATCH_SIZE,ACT_TYPE,DTYPE_IN,DTYPE_WTS,DTYPE_OUT,bpc_sync_1d,bpc_sync_1d,bpc_async_1d

#define ACT_TYPE           LINEAR       // act: Linear/ReLU dynamic mode
#define DTYPE_IN           uint8        // IFM, act: ReLU
#define DTYPE_OUT          uint8        // OFM. act: ReLU
#define DTYPE_WTS          int8         // WTS
#define FILTER_K           3            // WTS: 3x3x16x32
#define IFM_BPC_TYPE       adf::bpc_sync_1d
#define WTS_BPC_TYPE       adf::bpc_sync_1d
#define OFM_BPC_TYPE       adf::bpc_async_1d
#define IFM2_BPC_TYPE      adf::bpc_async_1d

#define BATCH_SIZE         (2)  // Batch

#define SM_BATCH_SIZE      (BATCH_SIZE)

//moved from Makefile
#define STRIDE2_OPT
#define FC_EN
#define USE_OPMODE_1       0
#define PAD_TOP_EN         1
#define PAD_LEFT_EN        1
#define PAD_RIGHT_EN       0
#define PAD_BOT_EN         0 

// Various modes of wrapper
typedef enum {
  REGULAR,
  ELTWISE,
  MAXIFM,
  ELTWISE_CASC,
  CASC,
  ELTWISE_NON_CASC,
  FC_CASC
} resnet18_mode_t ;

typedef enum {
  CONV2D,          //0
  CONV2D_LAYER_1,  //1 
  CONV2D_ADD2D,    //2
  CONV2D_ADD2D_GAP,//3
  FC               //4
} resnet18_ops_t ;

// Various types of resnet18_wrapper
template <int COL_OF>
void resnet18_wrapper
(
        adf::input_buffer_conf<DTYPE_IN, IFM_BPC_TYPE>  & __restrict ifm,
        adf::input_buffer_conf<DTYPE_WTS, WTS_BPC_TYPE>  & __restrict wts,
        adf::output_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> & __restrict ofm
);

template <int COL_OF>
void resnet18_wrapper_casc_start
(
        adf::input_buffer_conf<DTYPE_IN, IFM_BPC_TYPE>  & __restrict ifm,
        adf::input_buffer_conf<DTYPE_WTS, WTS_BPC_TYPE>  & __restrict wts,
        adf::output_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> & __restrict ofm,
	output_cascade<acc32> *casc_acc_out
); 

template <int COL_OF>
void resnet18_wrapper_casc_end
(
        adf::input_buffer_conf<DTYPE_IN, IFM_BPC_TYPE>  & __restrict ifm,
        adf::input_buffer_conf<DTYPE_WTS, WTS_BPC_TYPE>  & __restrict wts,
	input_cascade<acc32> *casc_acc_in,
        adf::output_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> & __restrict ofm
); 

template <int COL_OF>
void resnet18_wrapper_casc_mid
(
        adf::input_buffer_conf<DTYPE_IN, IFM_BPC_TYPE>  & __restrict ifm,
        adf::input_buffer_conf<DTYPE_WTS, WTS_BPC_TYPE>  & __restrict wts,
	input_cascade<acc32> *casc_acc_in,
        adf::output_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> & __restrict ofm,
	output_cascade<acc32> *casc_acc_out
); 

void resnet18_wrapper_eltwise
(
        adf::input_buffer_conf<DTYPE_IN, IFM_BPC_TYPE>   & __restrict ifm,
        adf::input_buffer_conf<DTYPE_WTS, WTS_BPC_TYPE>  & __restrict wts,
        adf::input_buffer_conf<DTYPE_IN, IFM2_BPC_TYPE>  & __restrict ifm2,
        adf::output_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> & __restrict ofm        
);

void resnet18_wrapper_eltwise_casc_start
(
        adf::input_buffer_conf<DTYPE_IN, IFM_BPC_TYPE>   & __restrict ifm,
        adf::input_buffer_conf<DTYPE_WTS, WTS_BPC_TYPE>  & __restrict wts,
        adf::input_buffer_conf<DTYPE_IN, IFM2_BPC_TYPE>  & __restrict ifm2,
        adf::output_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> & __restrict ofm,
        output_cascade<acc32> *casc_acc_out
);

void resnet18_wrapper_eltwise_casc_end
(
        adf::input_buffer_conf<DTYPE_IN, IFM_BPC_TYPE>   & __restrict ifm,
        adf::input_buffer_conf<DTYPE_WTS, WTS_BPC_TYPE>  & __restrict wts,
        adf::input_buffer_conf<DTYPE_IN, IFM2_BPC_TYPE>  & __restrict ifm2,
        input_cascade<acc32> *casc_acc_in,
        adf::output_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> & __restrict ofm
);

void resnet18_wrapper_eltwise_casc_mid
(
        adf::input_buffer_conf<DTYPE_IN, IFM_BPC_TYPE>   & __restrict ifm,
        adf::input_buffer_conf<DTYPE_WTS, WTS_BPC_TYPE>  & __restrict wts,
        adf::input_buffer_conf<DTYPE_IN, IFM2_BPC_TYPE>  & __restrict ifm2,
        input_cascade<acc32> *casc_acc_in,
        adf::output_buffer_conf<DTYPE_OUT, OFM_BPC_TYPE> & __restrict ofm,
        output_cascade<acc32> *casc_acc_out
);

#endif //_MLLIB_CONFIG_H_

