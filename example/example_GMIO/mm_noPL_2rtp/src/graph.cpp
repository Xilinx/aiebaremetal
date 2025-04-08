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
//#ifndef __BAREMETAL_HW__
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include "graph.h"
Mm gradf;
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

#define SAMPLES 256

int main(int argc, char ** argv)
{
    const int input_size_in_bytes = SIZE * sizeof(float);
    const int output_size_in_bytes = SIZE * sizeof(float);

    gradf.init();
    gr.init();

    float increment[2] = {1,2};
    char *inVect = reinterpret_cast< char *>(increment);
    gr.update("gradf.mm0.in[2]", inVect, sizeof (float)*2);

    auto out_bomapped = reinterpret_cast<float*>(gr.malloc(output_size_in_bytes));
    memset(out_bomapped, 0, output_size_in_bytes);

    auto in_bomappedA = reinterpret_cast<float*>(gr.malloc(input_size_in_bytes));
    auto in_bomappedB = reinterpret_cast<float*>(gr.malloc(input_size_in_bytes));
	
    //setting input data
    float *golden = (float*)malloc(output_size_in_bytes);
    for(int i = 0; i < SIZE; i++){
        in_bomappedA[i] = rand() % SIZE;
        in_bomappedB[i] = rand() % SIZE;
    }
    for(int i = 0; i < HEIGHT ; i++) {
        for(int j = 0; j < WIDTH ; j++){
            golden[i*WIDTH+j] = 0;
            for(int k=0; k <WIDTH; k++) golden[i*WIDTH+j] += in_bomappedA[i*WIDTH + k] * in_bomappedB[k*WIDTH + j];
        }
    } 
	gr.run(1);
    int match = 0;
    for (int i = 0; i < SIZE; i++) {
        if (out_bomapped[i] != golden[i]) {
            printf("ERROR: Test failed! Error found in sample %d: golden: %f, hardware: %f\n", i, golden[i], out_bomapped[i]);
            match = 1;
            break;
        }
    }

    float increment_out[1] = {1};
    char *outVect = reinterpret_cast<char *>(increment_out);
    gr.read("gradf.mm0.inout[0]", outVect, sizeof(float));
    if(increment_out[0] != increment[0]+increment[1]){
        printf("ERROR: ReadRTP may failed! %f\n", increment_out[0]);
        match = 1;
    }

    std::cout << "Releasing remaining  objects...\n";
   
    gr.free(in_bomappedA );
    gr.free(in_bomappedB);
    gr.free(out_bomapped);
    	
    std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl; 
    return (match ? EXIT_FAILURE :  EXIT_SUCCESS);
}