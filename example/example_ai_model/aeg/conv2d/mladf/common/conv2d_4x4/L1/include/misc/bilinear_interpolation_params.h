// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
/*
(c) Copyright 2014 - 2021 Xilinx, Inc. All Rights reserved.
(c) Copyright 2022 - 2024 Advanced Micro Devices, Inc. All Rights reserved.
   
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
    
#ifndef __BILINEAR_INTERPOLATION_PARAMS_H__
#define __BILINEAR_INTERPOLATION_PARAMS_H__

#include "utils.h"

struct alignas(v16uint16) bilinear_interpolation_internal_params{   
    uint16_t loop_count;
    uint16_t loop_count_x_pad;
    uint16_t loop_count_y_pad;
    int16_t  step_Ci;
    int16_t  step_Yi;
    int16_t  step_Yo;
    int16_t  incr_Yi;
    int16_t  step_to_last_Xi_eff;
    int16_t  step_to_last_Yi;
    int8_t   shift_pad_x_0;
    int8_t   shift_pad_x_1;
    int16_t  incr_pad_x;
    DECL_ITR2D(iterator_input);
    DECL_ITR2D(iterator_output);
};

    
typedef struct bilinear_interpolation_internal_params bilinear_interpolation_params_t;


static void setup_bilinear_interpolation_params(int8_t * params_ptr, bilinear_interpolation_params_t &param, uint8_t batch_size) 
{
    uint8_t ifm_xsize           = (uint8_t) *params_ptr++;//width of sub-volume.
    uint8_t ifm_xsize_eff       = (uint8_t) *params_ptr++;//required to know where to do sample repeat padding.
    uint8_t ifm_ysize           = (uint8_t) *params_ptr++;//height of sub-volume.
    uint8_t nifms               = (uint8_t) *params_ptr++;//number of channels. 
             
    uint8_t X_g      = ifm_xsize >> 2;//Number of iterations long x dimension. width/width_vectorization
    uint8_t Ci_g     = nifms >> 3;//Number of iterations along channels. channels/channel_vectorization    
    uint8_t pad      = 4; 
       
    param.loop_count = X_g * Ci_g * ifm_ysize;
        
    //Input pointer increment and iterator.
    param.step_Yi        = (ifm_xsize + pad) * nifms; //size of step in height direction for input.
    param.incr_Yi        = -32 + param.step_Yi;   //Move input_ptr to the next row.
    uint8_t input_num_0  = X_g - 1; //Covers:width complete. 
    int16_t input_incr_0 = -param.step_Yi;
    int16_t input_incr_1 = -param.step_Yi + pad*8; //Jump to next depth of 8 by skipping padded data.
    SETUP_ITR2D_PARAM(iterator_input,input_incr_0,input_num_0,input_incr_1,param)
    
    //Output pointer increment and iterator.
    uint8_t scale_factor  = 2;
    param.step_Yo         = (ifm_xsize*scale_factor*nifms); //size of step in height direction for output.  
    uint8_t output_num_0  = (((ifm_xsize*scale_factor*nifms)>>6) - 1);//Covers:width,depth complete,height of 2
    int16_t output_incr_0 = 64 - param.step_Yo;
    int16_t output_incr_1 = 64; //Jump to next height,  
    SETUP_ITR2D_PARAM(iterator_output,output_incr_0,output_num_0,output_incr_1,param) 
    
    
    //Variables for right padding
    param.step_Ci              = (ifm_xsize + pad) * 8;
    param.loop_count_x_pad     = ifm_ysize * Ci_g;
    uint8_t Xeff_g             = ifm_xsize_eff >> 2;    
    uint8_t ifm_xsize_eff_ceil = (ifm_xsize_eff - Xeff_g*4) == 0 ? ifm_xsize_eff : (Xeff_g+1)*4;      
    uint8_t tail               = ifm_xsize_eff_ceil-ifm_xsize_eff;
    
    param.step_to_last_Xi_eff  = (Xeff_g - (tail==0))*32;    
    param.shift_pad_x_0        = (3-tail) * 8;
    param.shift_pad_x_1        = tail==0 ? 64 : (tail*8) + 32;
    param.incr_pad_x           = 32 * (tail==0);
                
    //Variables for bottom padding
    param.loop_count_y_pad = (X_g + (pad==4)) * Ci_g;//Read and write blocks of 32 bytes.
    param.step_to_last_Yi  = (ifm_xsize + pad) * (ifm_ysize - 1) * nifms;
            
}

#endif  //__BILINEAR_INTERPOLATION_PARAMS_H__   
