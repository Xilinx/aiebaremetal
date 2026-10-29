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

// #include "mllib.h"
#include "test_conv2d_4x4.h"
#include <fstream>
#include <iomanip>
#include "transactionsubmit.h"
#ifdef __AIESIM__
#include "../aie_runtime_control.cpp"
#endif

//#define RETRIEVE_FROM_HEADER
#ifdef RETRIEVE_FROM_HEADER
  #include "./binary_data.h"
#else
  #define IFM_FILE  "data/ifm32.txt"
  #define WTS_FILE  "data/wts32.txt"
  #define OFM_FILE  "data/ofm32.txt"
#endif
single_layer compute_graph;

int main()
{
    const int ifm_size = IFM_TENSOR_VOL;
    const int wts_size = WT_TENSOR_VOL ;
    const int ofm_size = OFM_TENSOR_VOL;
    uint32_t* ifm_buf = (uint32_t*)GMIO::malloc(ifm_size*sizeof(uint8_t));
    uint32_t* wts_buf = (uint32_t*)GMIO::malloc(wts_size*sizeof(uint8_t));
    uint32_t* ofm_buf = (uint32_t*)GMIO::malloc(ofm_size*sizeof(uint8_t));

		printf("before init\n");
    compute_graph.init();
#ifdef RETRIEVE_FROM_HEADER
    printf("RETRIEVE_FROM_HEADER\n");
#endif

#ifdef RETRIEVE_FROM_HEADER
    for (int i=0; i<ifm_size/4; i++)
       ifm_buf[i]=ifm32[i];
#else
    std::ifstream ifm_ifs;
    ifm_ifs.open(IFM_FILE);
    std::cout << "Opened file " << IFM_FILE << " for reading IFM!!" << std::endl;
    for (int i=0; i<ifm_size/4; i++)
       ifm_ifs >> std::hex >> ifm_buf[i];
    ifm_ifs.close();
#endif

#ifdef RETRIEVE_FROM_HEADER
    for (int i=0; i<wts_size/4; i++)
       wts_buf[i]=wts32[i]; 
#else
    std::ifstream wts_ifs;
    wts_ifs.open(WTS_FILE);
    std::cout << "Opened file " << WTS_FILE << " for reading WTS!!" << std::endl;
    for (int i=0; i<wts_size/4; i++)
       wts_ifs >> std::hex >> wts_buf[i];
    wts_ifs.close();
#endif

    compute_graph.ifm_ddr.setAddress(ifm_buf);
    compute_graph.layer.wts_ddr.setAddress(wts_buf);
    compute_graph.ofm_ddr.setAddress(ofm_buf);

    compute_graph.run(1);
		printf("after run 2\n");

#ifdef __AIESIM__
    //executeRuntimeControl();
    executeRuntimeControl_TXN();
#endif

		
		printf("after run control\n");

    std::cout << "OFM Shim DMA transfer done, ready for file I/O!!" << std::endl;

    compute_graph.end();

#ifdef RETRIEVE_FROM_HEADER
    //for (int i=0; i<ofm_size/4; i++)
    //   if(ofm_buf[i]!=ofm32[i]){
    //      printf("ofm_buf[%d]=%x, ofm32[%d]=%x mismatch find break\n",i,ofm_buf[i],i,ofm32[i]);
    //      break;
    //   }
#else
    std::ofstream ofm_ofs;
    ofm_ofs.open(OFM_FILE, std::ofstream::out | std::ofstream::trunc);
    std::cout << "Opened file " << OFM_FILE << " for writing OFM!!" << std::endl;
    for (int i=0; i<ofm_size/4; i++)
        ofm_ofs << std::setw(8) << std::hex << std::setfill('0') << (uint32_t)(ofm_buf[i]) << std::endl;
    ofm_ofs.close();
#endif

    GMIO::free(ifm_buf);
    GMIO::free(wts_buf);
    GMIO::free(ofm_buf);

    return 0;
}
