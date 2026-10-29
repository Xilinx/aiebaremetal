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
    
#ifndef __PIXELSHUFFLE_PARAMS_H__
#define __PIXELSHUFFLE_PARAMS_H__

#include "utils.h"
#include "aie_api/utils.hpp"

#ifndef PXSF_VEC_LEN
#define PXSF_VEC_LEN 16
#endif

#ifndef PIXELSHUFFLE_LP_SIZE
#define PIXELSHUFFLE_LP_SIZE 5
#endif

struct alignas(v32uint16) pixelshuffle_internal_params{   

    uint16_t loop_inner;
    uint16_t loop_outer; 

    DECL_ITR3D(itr_ifm)
    DECL_ITR2D(itr_ofm)
    DECL_ITR2D(itr_ifmBase)
    DECL_ITR2D(itr_ofmBase)

};

    
typedef struct pixelshuffle_internal_params pixelshuffle_params_t;


static void setup_pixelshuffle_params(pixelshuffle_params_t &param, const uint32_t (&lp)[PIXELSHUFFLE_LP_SIZE]) 
{
    uint32_t cols      = lp[0];
    uint32_t rows      = lp[1]; 
    uint32_t depths    = lp[2]; 
    uint32_t factor    = lp[3]; 
    uint32_t nbytes    = lp[4];

    int num_tiles = cols * factor;
    int offset_block = depths * cols; 
    int r_sqr = factor * factor;
    int bank_size = depths / r_sqr;

    param.loop_inner = factor * cols * rows / PXSF_VEC_LEN; 
    param.loop_outer = factor * bank_size;

    int ifm_incr0     = cols * nbytes; 
    int ifm_wrap0     = factor - 1;
    int ifm_incr1     = - (factor - 1) * cols * nbytes + nbytes ;
    int ifm_wrap1     = cols - 1;  
    int ifm_incr2     = (offset_block - (factor - 1) * cols - (cols - 1)) * nbytes;  
   
    SETUP_ITR3D_PARAM(itr_ifm,ifm_incr0,ifm_wrap0,ifm_incr1,ifm_wrap1,ifm_incr2,param)  

    int ofm_incr0     = PXSF_VEC_LEN * nbytes; 
    int ofm_wrap0     = num_tiles / PXSF_VEC_LEN - 1;
    int ofm_incr1     = (offset_block - factor * cols + PXSF_VEC_LEN) * nbytes;
   
    SETUP_ITR2D_PARAM(itr_ofm,ofm_incr0,ofm_wrap0,ofm_incr1,param) 


    int ifmBase_incr0     = num_tiles * nbytes; 
    int ifmBase_wrap0     = factor - 1;
    int ifmBase_incr1     = (r_sqr * cols - (factor - 1) * num_tiles) * nbytes; 
   
    SETUP_ITR2D_PARAM(itr_ifmBase,ifmBase_incr0,ifmBase_wrap0,ifmBase_incr1,param) 


    int ofmBase_incr0     = bank_size * num_tiles * nbytes; 
    int ofmBase_wrap0     = factor - 1;
    int ofmBase_incr1     = - (factor - 1) * bank_size * num_tiles * nbytes + num_tiles * nbytes;
   
    SETUP_ITR2D_PARAM(itr_ofmBase,ofmBase_incr0,ofmBase_wrap0,ofmBase_incr1,param) 

}

#endif  //__PIXELSHUFFLE_PARAMS_H__   
