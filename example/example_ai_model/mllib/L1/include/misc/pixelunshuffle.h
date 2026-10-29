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

#ifndef __PIXELUNSHUFFLE_H__
#define __PIXELUNSHUFFLE_H__

#include "aie.hpp"
#include <adf.h>
#include "aie_api/utils.hpp"
#include "pixelunshuffle_params.h"

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
NOINLINE void pixelunshuffle(
    dtype * __restrict ifm_ptr,
    dtype * __restrict ofm_ptr,
    pixelunshuffle_params_t &params)
{
    dtype __aie_dm_resource_a *p_in = (dtype __aie_dm_resource_a *)ifm_ptr;
    dtype __aie_dm_resource_a *p_out = (dtype __aie_dm_resource_a *)ofm_ptr;

    SETUP_ITR2D_COUNTERS(iterator_outer_out)
    for (int i = 0; i < params.outer_loop_nm; i++)
    {
        SETUP_ITR3D_COUNTERS(iterator_inner_in)
        SETUP_ITR2D_COUNTERS(iterator_inner_out)
        for (int j = 0; j < params.inner_loop_nm; j++)
        {
            vector<dtype, ST_VEC_LEN> in_buff;
            in_buff[0] = *p_in;
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner_in, params)
            in_buff[1] = *p_in;
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner_in, params)
            in_buff[2] = *p_in;
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner_in, params)
            in_buff[3] = *p_in;
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner_in, params)
            in_buff[4] = *p_in;
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner_in, params)
            in_buff[5] = *p_in;
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner_in, params)
            in_buff[6] = *p_in;
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner_in, params)
            in_buff[7] = *p_in;
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner_in, params)
            in_buff[8] = *p_in;
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner_in, params)
            in_buff[9] = *p_in;
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner_in, params)
            in_buff[10] = *p_in;
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner_in, params)
            in_buff[11] = *p_in;
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner_in, params)
            in_buff[12] = *p_in;
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner_in, params)
            in_buff[13] = *p_in;
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner_in, params)
            in_buff[14] = *p_in;
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner_in, params)
            in_buff[15] = *p_in;
            p_in = INCR_ITR3D_PARAM(p_in, iterator_inner_in, params)

            store_v(p_out, in_buff);

            p_out = INCR_ITR2D_PARAM(p_out, iterator_inner_out, params)
        }
        p_in = byte_incr(p_in, params.outer_in_inc0);
        p_out = INCR_ITR2D_PARAM(p_out, iterator_outer_out, params)
    }
}

template <typename dtype, typename config_ifm, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
INLINE void pixelunshuffle(adf::input_buffer_conf<dtype, config_ifm> &__restrict ifm,
                                         adf::output_buffer_conf<dtype, config_ofm> &__restrict ofm,
                                         pixelunshuffle_params_t &params)
{
    return pixelunshuffle<dtype>(ifm.data(), ofm.data(), params);
}

#endif //__PIXELUNSHUFFLE_H__
