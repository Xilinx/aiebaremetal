/**
* Copyright (C) 2025 Advanced Micro Devices, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#include <fstream>
#include <xaiengine.h>
#include "../../aiebaremetal.h"
#define NBYTES_DATA 4

#define WIDTH 16
#define HEIGHT 16
#define SIZE (WIDTH * HEIGHT)
#define BLK_DIM 8
#define BLK_SIZE (BLK_DIM * BLK_DIM)

////////////////////////////////////////////////////////////////////////////////
//// Hardcoded unrolled fast 8-point (i)DCT
//////////////////////////////////////////////////////////////////////////////////
#define C_a 1.3870398453221474618216191915664f       //a = sqrt(2) * cos(1 * pi / 16)
#define C_b 1.3065629648763765278566431734272f       //b = sqrt(2) * cos(2 * pi / 16)
#define C_c 1.1758756024193587169744671046113f       //c = sqrt(2) * cos(3 * pi / 16)
#define C_d 0.78569495838710218127789736765722f      //d = sqrt(2) * cos(5 * pi / 16)
#define C_e 0.54119610014619698439972320536639f      //e = sqrt(2) * cos(6 * pi / 16)
#define C_f 0.27589937928294301233595756366937f      //f = sqrt(2) * cos(7 * pi / 16)
#define C_norm 0.35355339059327376220042218105242f   //1 / sqrt(8)

//#ifndef __BAREMETAL_HW__
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include "graph.h"
Dct gradf;
BaremetalGraphSim gr("gradf");
#else
//#include "platform.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "xil_cache.h"
#include "xtime_l.h"
BaremetalGraph gr("gradf");
#endif

void DCT8(float *dst, float *src, int ostride, int istride){
    float X07P = src[0 * istride] + src[7 * istride];
    float X16P = src[1 * istride] + src[6 * istride];
    float X25P = src[2 * istride] + src[5 * istride];
    float X34P = src[3 * istride] + src[4 * istride];

    float X07M = src[0 * istride] - src[7 * istride];
    float X61M = src[6 * istride] - src[1 * istride];
    float X25M = src[2 * istride] - src[5 * istride];
    float X43M = src[4 * istride] - src[3 * istride];

    float X07P34PP = X07P + X34P;
    float X07P34PM = X07P - X34P;
    float X16P25PP = X16P + X25P;
    float X16P25PM = X16P - X25P;

    dst[0 * ostride] = C_norm * (X07P34PP + X16P25PP);
    dst[2 * ostride] = C_norm * (C_b * X07P34PM + C_e * X16P25PM);
    dst[4 * ostride] = C_norm * (X07P34PP - X16P25PP);
    dst[6 * ostride] = C_norm * (C_e * X07P34PM - C_b * X16P25PM);

    dst[1 * ostride] = C_norm * (C_a * X07M - C_c * X61M + C_d * X25M - C_f * X43M);
    dst[3 * ostride] = C_norm * (C_c * X07M + C_f * X61M - C_a * X25M + C_d * X43M);
    dst[5 * ostride] = C_norm * (C_d * X07M + C_a * X61M + C_f * X25M - C_c * X43M);
    dst[7 * ostride] = C_norm * (C_f * X07M + C_d * X61M + C_c * X25M + C_a * X43M);
}

int main(int argc, char **argv)
{
    const int input_size_in_bytes = SIZE * sizeof(float);
    const int output_size_in_bytes = SIZE * sizeof(float);

	gradf.init();
	gr.init();
    

    //xrtKernelHandle dmahls_khdl = xrtPLKernelOpen(dhdl, top->m_header.uuid, "dma_hls");
    // xrtKernelHandle mm2s_hdl = xrtPLKernelOpen(dhdl, top->m_header.uuid, "mm2s");
    // xrtKernelHandle s2mm_hdl = xrtPLKernelOpen(dhdl, top->m_header.uuid, "s2mm");
    
    //const int out_bosz = sizeof(uint32_t) * OUTPUT_SIZE * WORD_LENGTH;
    //xrtBufferHandle out_bohdl = xrtBOAlloc(dhdl, output_size_in_bytes,  0, 0);
    // auto out_bomapped = reinterpret_cast<float*>(gr.malloc(output_size_in_bytes));
    // memset(out_bomapped, 0, output_size_in_bytes);

    // //const int in_bosz = sizeof(input_pl);     
    // //xrtBufferHandle in_bohdl = xrtBOAlloc(dhdl, input_size_in_bytes,  0, 0);
    // auto in_bomapped = reinterpret_cast<float*>(gr.malloc(input_size_in_bytes));
    // //memcpy(in_bomapped, input_pl, input_size_in_bytes);

    // float *golden = (float*)malloc(output_size_in_bytes);
    // for(int i = 0; i < SIZE; i++){
    //     in_bomapped[i] = rand() % SIZE;
    // }
    // for(int i = 0; i + BLK_DIM <= HEIGHT ; i+=BLK_DIM) {
    //     for(int j = 0; j + BLK_DIM <= WIDTH ; j+=BLK_DIM){
    //         //process rows
    //         for(int k = 0; k < BLK_DIM; k++)
    //             DCT8(golden + (i + k) * WIDTH + j, in_bomapped + (i + k) * WIDTH + j, 1, 1);
    //         //process columns
    //         for(int k = 0; k < BLK_DIM; k++)
    //             DCT8(golden + i * WIDTH + (j + k), golden + i * WIDTH + (j + k), WIDTH, WIDTH);
    //     }
    // } 

    // int rval;
    // xrtRunHandle mm2s_rhdl = xrtRunOpen(mm2s_hdl);
    // rval = xrtRunSetArg(mm2s_rhdl, 0, in_bohdl);
    // rval = xrtRunSetArg(mm2s_rhdl, 2, WIDTH);  
    // rval = xrtRunSetArg(mm2s_rhdl, 3, HEIGHT); 
    // rval = xrtRunSetArg(mm2s_rhdl, 4, BLK_DIM);
    
    // xrtRunHandle s2mm_rhdl = xrtRunOpen(s2mm_hdl);
    // rval = xrtRunSetArg(s2mm_rhdl, 0, out_bohdl);
    // rval = xrtRunSetArg(s2mm_rhdl, 2, WIDTH);
    // rval = xrtRunSetArg(s2mm_rhdl, 3, HEIGHT);
    // rval = xrtRunSetArg(s2mm_rhdl, 4, BLK_DIM);
    
    // xrtRunStart(mm2s_rhdl);
    // xrtRunStart(s2mm_rhdl);

//    xrtRunStart(dct8x8_rhdl);
// #if 0
//     xrtRunHandle dmahls_rhdl = xrtKernelRun(dmahls_khdl, in_bohdl, out_bohdl, nullptr, nullptr, INPUT_SIZE, OUTPUT_SIZE);
// #endif
// /*
//     auto state = xrtRunWait(dmahls_rhdl);
//     std::cout << "dma_hls completed with status(" << state << ")\n";
//     xrtRunClose(dmahls_rhdl);
//     xrtKernelClose(dmahls_khdl);
// */
//     auto state1 = xrtRunWait(mm2s_rhdl);
//     std::cout << "mm2s completed with status(" << state1 << ")\n";
//     xrtRunClose(mm2s_rhdl);
//     xrtKernelClose(mm2s_hdl);

//     //auto state2 = xrtRunWait(dct8x8_rhdl);
//     //std::cout << "dct8x8 completed with status(" << state2 << ")\n";
//     //xrtRunClose(dct8x8_rhdl);
//     //xrtKernelClose(dct8x8_rhdl);

//     auto state3 = xrtRunWait(s2mm_rhdl);
//     std::cout << "s2mm completed with status(" << state3 << ")\n";
//     xrtRunClose(s2mm_rhdl);
//     xrtKernelClose(s2mm_hdl);

gr.run(1);
gr.wait();
    // int match = 0;
    // for (int i = 0; i < SIZE; i++) {
    //     if (out_bomapped[i] != golden[i]) {
    //         printf("ERROR: Test failed! Error found in sample %d: golden: %f, hardware: %f\n", i, golden[i], out_bomapped[i]);
    //         match = 1;
    //         break;
    //     }
    // }
/*
    int errFlag = 0;
    int out_count = 0;
    int image_count = 0;

    printf("Comparing output with golden\n");
    for(int i = 0; i < OUTPUT_SIZE * WORD_LENGTH; i++) {
        if( out_bomapped[i] == int32golden[i] ) {
            //printf("%d\n ",((int32_t*)out)[i]);
            errFlag = errFlag || 0;
            out_count++;
        } else {
            errFlag = errFlag || 1;
            printf("Error found in sample 0x%X != to the golden 0x%X\n", out_bomapped[i], int32golden[i]);
        }
        if ( (out_count == OUT_IMAGE_LENGTH) ) {
            if (!errFlag) {
                printf("Pass for image %d \n", image_count);
                    image_count++;
                    out_count = 0;
                    errFlag = 0;
            } else {
                printf("Fail for image %d \n", image_count);
                image_count++;
                out_count = 0;
                errFlag = 0;
            }
        }
    }
*/
    std::cout << "Releasing remaining XRT objects...\n";
    // xrtBOFree(in_bohdl);
    // xrtBOFree(out_bohdl);
    // xrtDeviceClose(dhdl);
	// gr.free(out_bomapped);
	// gr.free(in_bomapped);


    // std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl; 
    // return (match ? EXIT_FAILURE :  EXIT_SUCCESS);
    return 0;
}