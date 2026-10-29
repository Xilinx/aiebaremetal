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
    
    
    
#ifndef __CASC_READ_WRITE_H__
#define __CASC_READ_WRITE_H__

#include "aie.hpp"
#include <adf.h>

#include "utils.h"

using namespace aie;
// casc write: read data from memory as v32int32 and write out in cascade as-is 
static void casc_write 
(     
    int data_len,                     // data length in bytes corresponding to the number of elements transferred
    int32 *__restrict buf_ptr,
    OUT_CASC<acc32> *casc_acc_out
){
    
    accum<acc32,32> acc0,acc1;
    unsigned int loop_len=data_len>>8; // ->/32lanes/2accs/4bytes/lane
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(4,) 
    for(unsigned k=0;k<loop_len;k++){
        acc0.from_vector(load_v<32>(buf_ptr),0); buf_ptr+=32 ;
        acc1.from_vector(load_v<32>(buf_ptr),0); buf_ptr+=32 ;
        writeincr(casc_acc_out,acc0);
        writeincr(casc_acc_out,acc1);
    }
}

static void casc_read 
(     
    int data_len,                     // data length in bytes corresponding to the number of elements transferred
    int32 * __restrict buf_ptr,
    IN_CASC<acc32> *casc_acc_in
){
    
    accum<acc32,32> acc0,acc1;
    unsigned int loop_len=data_len>>8; // ->/32lanes/2accs/4bytes/lane
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(4,) 
    for(unsigned k=0;k<loop_len;k++){
        acc0 = readincr_v<32>(casc_acc_in);
        acc1 = readincr_v<32>(casc_acc_in);
        store_v(buf_ptr, acc0.to_vector<int32>(0)); buf_ptr+=32;
        store_v(buf_ptr, acc1.to_vector<int32>(0)); buf_ptr+=32;
    }
}
/*
template <typename dtype>
void casc_write 
(     
    int data_len,
    dtype *__restrict buf_ptr,
    OUT_CASC<acc32> *casc_acc_out
){
    
    accum<acc32,32> acc0,acc1,acc2,acc3 ;
    unsigned int loop_len=data_len>>7; // ->/32/4
    for(unsigned k=0;k<loop_len;k++)
    chess_prepare_for_pipelining
    chess_loop_range(4,)
    {
        acc0.from_vector(load_v<32>(buf_ptr),0); buf_ptr+=32 ;
        acc1.from_vector(load_v<32>(buf_ptr),0); buf_ptr+=32 ;
        acc2.from_vector(load_v<32>(buf_ptr),0); buf_ptr+=32 ;
        acc3.from_vector(load_v<32>(buf_ptr),0); buf_ptr+=32 ;
        writeincr(casc_acc_out,acc0);
        writeincr(casc_acc_out,acc1);
        writeincr(casc_acc_out,acc2);
        writeincr(casc_acc_out,acc3);
    }
}
template <typename dtype>
void casc_read 
(     
    int data_len,
    dtype * __restrict buf_ptr,
    IN_CASC<acc32> *casc_acc_in
){
    
    accum<acc32,32> acc0,acc1,acc2,acc3;
    unsigned int loop_len=data_len>>7; // ->/32/4
    for(unsigned k=0;k<loop_len;k++)
    chess_prepare_for_pipelining
    chess_loop_range(4,)
    {
        acc0 = readincr_v<32>(casc_acc_in);
        acc1 = readincr_v<32>(casc_acc_in);
        acc2 = readincr_v<32>(casc_acc_in);
        acc3 = readincr_v<32>(casc_acc_in);
        store_v(buf_ptr, acc0.to_vector<dtype>(0)); buf_ptr += 32;
        store_v(buf_ptr, acc1.to_vector<dtype>(0)); buf_ptr += 32;
        store_v(buf_ptr, acc2.to_vector<dtype>(0)); buf_ptr += 32;
        store_v(buf_ptr, acc3.to_vector<dtype>(0)); buf_ptr += 32;
    }
}
*/
static void casc_write 
(     
    int data_len,
    int16 * __restrict buf_ptr,
    OUT_CASC<acc32> *casc_acc_out
){
    
    accum<acc32,32> acc0 ;
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(1,) 
    for(unsigned k=0;k<data_len;k++){
        acc0.from_vector(load_v<32>(buf_ptr),0); buf_ptr+=32 ;
        writeincr(casc_acc_out,acc0);
    }
}

static void casc_read 
(     
    int data_len,
    int16 * __restrict buf_ptr,
    IN_CASC<acc32> *casc_acc_in
){
    
    accum<acc32,32> acc0 ;
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(1,) 
    for(unsigned k=0;k<data_len;k++){
        acc0 = readincr_v<32>(casc_acc_in);
        store_v(buf_ptr, acc0.to_vector<int16>(0)); buf_ptr += 32;
    }
}

static void casc_read
(     
    int data_len,
    int16 * __restrict buf_ptr,
    int shift_out,
    IN_CASC<acc32> *casc_acc_in
){
    
    //using mmul = mmul<DIM_M, DIM_N, DIM_K, int8, int8>;
    accum<acc32,32> acc0 ;
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(1,) 
    for(unsigned k=0;k<data_len;k++){
        accum<acc32,32> temp_acc0,final_acc0;
	temp_acc0 = readincr_v<32>(casc_acc_in) ; // read from cascade port result of conv on left-side core, note we are loading partial sum data without any left shift in acc
        acc0.from_vector(load_v<32>(buf_ptr),0); //buf_ptr+=32 ; // read from local memory, this is the result of conv on this core, dont use any shift due to reason above
        final_acc0=add(temp_acc0,acc0); // add the two to produce final output
        store_v(buf_ptr, final_acc0.to_vector<int16>(shift_out)); buf_ptr += 32; // shift_out=0 for start and mid nodes, non zero for end node
        //mmul Cbuff0_final(final_acc0);
        //acc0 = readincr_v<32>(casc_acc_in);
        //store_v(buf_ptr, acc0.to_vector<int16>(0)); buf_ptr += 32;
    }
}

static void casc_read_write
(     
    int data_len,
    IN_CASC<acc32> *casc_acc_in,
    OUT_CASC<acc32> *casc_acc_out
){
    
    accum<acc32,32> acc0 ;
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(4,) 
    for(unsigned k=0;k<data_len;k++){
        acc0 = readincr_v<32>(casc_acc_in);
        writeincr(casc_acc_out,acc0);
    }
}

static void mem_fill_zero
(
        uint8_t * __restrict out_ptr,
        int out_len,    // length of the broadcast in bytes, shd be multiple of 32 bytes and minimum length is 128 bytes for loop pieplining
        int out_offset
){

    uint8_t *output_ptr = out_ptr+out_offset;
    vector<uint8_t,32> input_vec=zeros<uint8_t,32>();

    uint32_t len = out_len>>5;

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(4,) 
    for (uint32_t j=0; j< len; j++){
        store_v(output_ptr,input_vec);
        output_ptr += 32;
    }
}

static void mem_fill_pat
(
        uint8_t value_in, // byte value to be brocasted and written out to output buffer
        uint8_t * __restrict out_ptr,
        int out_len,    // length of the broadcast in bytes, shd be multiple of 32 bytes and minimum length is 128 bytes for loop pieplining
        int out_offset
){

    uint8_t *output_ptr = out_ptr+out_offset;
    vector<uint8_t,32> input_vec=broadcast<uint8_t,32>(value_in);

    uint32_t len = out_len>>5;

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(4,) 
    for (uint32_t j=0; j< len; j++){
        store_v(output_ptr,input_vec);
        output_ptr += 32;
    }
}

#endif //__CASC_READ_WRITE_H__    

