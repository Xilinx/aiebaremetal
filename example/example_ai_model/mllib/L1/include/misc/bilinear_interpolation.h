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


#ifndef __BILINEAR_INTERPOLATION_H__
#define __BILINEAR_INTERPOLATION_H__

#include "aie.hpp"
#include <adf.h>

#include "bilinear_interpolation_params.h"
#include "utils.h"

using namespace aie;

template<uint8_t batch_size, typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, uint8_t, bfloat16>)
NOINLINE void bilinear_interpolation
(
    dtype * __restrict ifm_data,
    dtype * __restrict ofm_data,
    bilinear_interpolation_params_t  &bilinear_interpolation_params
){

    //Pre-process the input to copy-paste the right-edge and bottom-edge pixels.

    //Right-edge padding
    v32int8 __aie_dm_resource_a * RP_read_ptr  = (v32int8 __aie_dm_resource_a *) ifm_data;
    v32int8 __aie_dm_resource_a * RP_write_ptr = (v32int8 __aie_dm_resource_a *) ifm_data;
    RP_read_ptr  = byte_incr(RP_read_ptr,  bilinear_interpolation_params.step_to_last_Xi_eff);
    RP_write_ptr = byte_incr(RP_write_ptr, bilinear_interpolation_params.step_to_last_Xi_eff + bilinear_interpolation_params.incr_pad_x);

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(4,) 
    for (int i=0; i< bilinear_interpolation_params.loop_count_x_pad; i++){
        vector<dtype,64> data, data_shifted, last_valid_pixel, data_padded;
        data.insert(0, aie::load_v<32>((dtype __aie_dm_resource_a *)RP_read_ptr));
        last_valid_pixel = shuffle_down_fill(data, zeros<dtype, 64>(), bilinear_interpolation_params.shift_pad_x_0);
        data_shifted = shuffle_up_fill(data, zeros<dtype, 64>(), bilinear_interpolation_params.shift_pad_x_1);
        data_padded  = shuffle_down_fill(data_shifted,last_valid_pixel,bilinear_interpolation_params.shift_pad_x_1);
        store_v((dtype __aie_dm_resource_a *)RP_write_ptr, data_padded.template extract<32>(0));
        RP_read_ptr  = byte_incr(RP_read_ptr,  bilinear_interpolation_params.step_Ci);
        RP_write_ptr = byte_incr(RP_write_ptr, bilinear_interpolation_params.step_Ci);
    }

    //Bottom-edge padding
    v32int8 __aie_dm_resource_a * BP_read_ptr  = (v32int8 __aie_dm_resource_a *) ifm_data;
    v32int8 __aie_dm_resource_a * BP_write_ptr = (v32int8 __aie_dm_resource_a *) ifm_data;
    BP_read_ptr  = byte_incr(BP_read_ptr, bilinear_interpolation_params.step_to_last_Yi);
    BP_write_ptr = byte_incr(BP_write_ptr, bilinear_interpolation_params.step_to_last_Yi + bilinear_interpolation_params.step_Yi);

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(4,) 
    for (int i=0; i< bilinear_interpolation_params.loop_count_y_pad; i++){
        vector<dtype,32> data;
        data = load_v<32>((dtype __aie_dm_resource_a *)BP_read_ptr);
        store_v((dtype __aie_dm_resource_a *)BP_write_ptr, data);
        BP_read_ptr  = byte_incr(BP_read_ptr, 32);
        BP_write_ptr = byte_incr(BP_write_ptr,32);
    }

    //Bilinear Interpolation
    v32int8 __aie_dm_resource_a * input_ptr = (v32int8 __aie_dm_resource_a *) ifm_data;
    v32int8 __aie_dm_resource_a * output_ptr = (v32int8 __aie_dm_resource_a *) ofm_data;

    SETUP_ITR2D_COUNTERS(iterator_input)
    SETUP_ITR2D_COUNTERS(iterator_output)

    //Weights for X-interpolation, Y-interpolation, Diag-interpolation
    //Same wts are used for both X and Y interpolations.
    vector<uint8_t, 64> x_y_interpolation_wts  =  broadcast((uint8_t)1);
    x_y_interpolation_wts = shuffle_down_fill(x_y_interpolation_wts, zeros<uint8_t, 64>(), 48);
    //Wts used for diagonal interpolation.
    vector<uint8_t, 64> diag_interpolation_wts =  broadcast((uint8_t)1);
    diag_interpolation_wts = shuffle_down_fill(diag_interpolation_wts, zeros<uint8_t, 64>(), 32);


    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(4,) 
    for (int i=0; i< bilinear_interpolation_params.loop_count; i++){
        vector<dtype,64> input_row_0, input_row_1; //vectors to store input data
        accum<acc32,32>  x_interpolation_acc, y_interpolation_acc_0, y_interpolation_acc_1, diag_interpolation_acc_0, diag_interpolation_acc_1;
        vector<dtype,64> x_interpolation_res, y_interpolation_res, diag_interpolation_res, y_diag_interpolation_res; //results of various interpolations.
        vector<dtype,64> temp_buff_undef; //just a place holder for shuffle operations.


        //Input data of current row for X-interpolation
        input_row_0.insert(0, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));
        input_ptr = byte_incr(input_ptr, 32);
        input_row_0.insert(1, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));
        input_ptr = byte_incr(input_ptr, bilinear_interpolation_params.incr_Yi);//step to the next row.
        //Input data of next row for Y-interpolation and Diag-interpolation
        input_row_1.insert(0, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));
        input_ptr = byte_incr(input_ptr, 32);
        input_row_1.insert(1, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));
        input_ptr = INCR_ITR2D_PARAM(input_ptr, iterator_input, bilinear_interpolation_params)


        //X-interpolation
        x_interpolation_acc = zeros<acc32, 32>();
        x_interpolation_acc = mac_conv_4x4_8ch(input_row_0, x_y_interpolation_wts, x_interpolation_acc);
        vector<dtype,64> temp_buff_0;
        temp_buff_0.insert(0, x_interpolation_acc.template to_vector<dtype>(1));
        x_interpolation_res = shuffle(input_row_0, temp_buff_0, 14);//Interleave original elements with interpolated elements.


        //Y-interpolation
        //Arrange data for Y-interpolation. Put an element of row_1 next to corresponding element from row_0
        vector<dtype,64> temp_buff_1, temp_buff_2, temp_buff_3;
        temp_buff_1 = shuffle(input_row_0, input_row_1, 14);
        temp_buff_2 = shuffle(input_row_0, input_row_1, 15);
        temp_buff_3 = shuffle_down_fill(temp_buff_1, temp_buff_2, 32);//Shift operation of 32 elements.

        y_interpolation_acc_0 = zeros<acc32, 32>();
        y_interpolation_acc_0 = mac_conv_4x4_8ch(temp_buff_1, x_y_interpolation_wts, y_interpolation_acc_0);
        y_interpolation_acc_1 = zeros<acc32, 32>();
        y_interpolation_acc_1 = mac_conv_4x4_8ch(temp_buff_3, x_y_interpolation_wts, y_interpolation_acc_1);
        vector<dtype,64> temp_buff_4;
        temp_buff_4.insert(0, y_interpolation_acc_0.template to_vector<dtype>(1));
        temp_buff_4.insert(1, y_interpolation_acc_1.template to_vector<dtype>(1));
        y_interpolation_res = shuffle(temp_buff_4,temp_buff_undef,6);//Interleave and throw away unwanted elements

        //Diag-interpolation
        diag_interpolation_acc_0 = zeros<acc32, 32>();
        diag_interpolation_acc_0 = mac_conv_4x4_8ch(temp_buff_1, diag_interpolation_wts, diag_interpolation_acc_0);
        diag_interpolation_acc_1 = zeros<acc32, 32>();
        diag_interpolation_acc_1 = mac_conv_4x4_8ch(temp_buff_3, diag_interpolation_wts, diag_interpolation_acc_1);
        vector<dtype,64> temp_buff_5;
        temp_buff_5.insert(0, diag_interpolation_acc_0.template to_vector<dtype>(2));
        temp_buff_5.insert(1, diag_interpolation_acc_1.template to_vector<dtype>(2));
        diag_interpolation_res = shuffle(temp_buff_5, temp_buff_undef,6);//Interleave and throw away unwanted elements

        //Interleave the Y-interpolation result with Diag-interpolation result.
        y_diag_interpolation_res = shuffle(y_interpolation_res, diag_interpolation_res,14);


        //Store the results of all interpolations.
        store_v((dtype __aie_dm_resource_a *)output_ptr,x_interpolation_res);
        output_ptr = byte_incr(output_ptr, bilinear_interpolation_params.step_Yo);
        store_v((dtype __aie_dm_resource_a *)output_ptr,y_diag_interpolation_res);
        output_ptr = INCR_ITR2D_PARAM(output_ptr, iterator_output, bilinear_interpolation_params)
    }

}

template<uint8_t batch_size, typename dtype, typename config_ifm, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, uint8_t, bfloat16>)
INLINE void bilinear_interpolation
(
    adf::input_buffer_conf<dtype, config_ifm>  & __restrict ifm,
    adf::output_buffer_conf<dtype, config_ofm> & __restrict ofm,
    bilinear_interpolation_params_t  &bilinear_interpolation_params
){
    return bilinear_interpolation<batch_size, dtype>(
        ifm.data(), ofm.data(), bilinear_interpolation_params);
}

#endif //__BILINEAR_INTERPOLATION_H__

