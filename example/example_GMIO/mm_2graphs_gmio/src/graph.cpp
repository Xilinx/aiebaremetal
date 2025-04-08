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
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include "graph.h"
Mm<0> mm1;
Mm<1> mm2;
BaremetalGraphSim gr1("mm1");
BaremetalGraphSim gr2("mm2");
#else
BaremetalGraph gr1("mm1");
BaremetalGraph gr2("mm2");
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
//#include "include.h"
#include <iostream>
#include <cstring>
#include <vector>

#define SAMPLES 256

int main(int argc, char ** argv)
{
    const int input_size_in_bytes = SIZE * sizeof(float);
    const int output_size_in_bytes = SIZE * sizeof(float);

    #if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
	mm1.init();
    mm2.init();
	#else
    //FIXME, should handle the sync in runtime api
	Xil_DCacheDisable();
	Xil_ICacheDisable();

	//get time
    XTime tStart, tEnd;

    //init_platform();

    XTime_GetTime(&tStart);
    #endif

    gr1.init();
    gr2.init();
    gr1.run(1);
    gr2.run(1);
   
    std::cout << "ready to run graph1 \n";
   
    auto out_bomapped1 = reinterpret_cast<float*>(gr1.malloc( output_size_in_bytes));
    memset(out_bomapped1, 0, output_size_in_bytes);
   
    auto out_bomapped2 = reinterpret_cast<float*>(gr2.malloc(output_size_in_bytes));
    memset(out_bomapped2, 0, output_size_in_bytes);
   
    auto in_bomappedA = reinterpret_cast<float*>(gr1.malloc(input_size_in_bytes));
    auto in_bomappedB = reinterpret_cast<float*>(gr1.malloc(input_size_in_bytes));
    auto in_bomappedC = reinterpret_cast<float*>(gr2.malloc(input_size_in_bytes));
	
    //setting input data
    float *golden = (float*)malloc(output_size_in_bytes);
    float *golden1 = (float*)malloc(output_size_in_bytes);
    for(int i = 0; i < SIZE; i++){
        in_bomappedA[i] = rand() % SIZE;
        in_bomappedB[i] = rand() % SIZE;
        in_bomappedC[i] = rand() % SIZE;
    }
    for(int i = 0; i < HEIGHT ; i++) {
        for(int j = 0; j < WIDTH ; j++){
            golden1[i*WIDTH+j] = 0;
            for(int k=0; k <WIDTH; k++) golden1[i*WIDTH+j] += in_bomappedA[i*WIDTH + k] * in_bomappedB[k+WIDTH * j];
        }
    } 
    for(int i = 0; i < HEIGHT ; i++) {
        for(int j = 0; j < WIDTH ; j++){
            golden[i*WIDTH+j] = 0;
            for(int k=0; k <WIDTH; k++) golden[i*WIDTH+j] += in_bomappedC[i*WIDTH + k] * golden1[k+WIDTH * j];
        }
    } 

    std::cout << "data is ready \n";

    int rval;
    int gmio_id = 0;
    
    gr1.gm2aie_nb("mm1.inB", reinterpret_cast<char*>(in_bomappedB), SIZE * sizeof(float));
    std::cout << "syncAIEBO B \n";
    
    gr1.gm2aie_nb("mm1.inA", reinterpret_cast<char*>(in_bomappedA), SIZE * sizeof(float));
    std::cout << "syncAIEBO A \n";
    
    gr1.aie2gm_nb("mm1.out", reinterpret_cast<char*>(out_bomapped1), SIZE * sizeof(float) );
    std::cout << "syncAIEBO Out \n";
	gr1.gmio_wait("mm1.out");
   
    gr2.gm2aie_nb("mm2.inB", reinterpret_cast<char*>(out_bomapped1), SIZE * sizeof(float));
    std::cout << "syncAIEBO Out as input \n";
    
    gr2.gm2aie_nb("mm2.inA", reinterpret_cast<char*>(in_bomappedC), SIZE * sizeof(float));
    std::cout << "syncAIEBO C \n";
   
    gr2.aie2gm_nb("mm2.out",reinterpret_cast<char*>(out_bomapped2), SIZE * sizeof(float) );
    std::cout << "syncAIEBO Out \n";
    gr2.gmio_wait("mm2.out");
    gr2.wait();

    int match = 0;
    for (int i = 0; i < SIZE; i++) {
        if (out_bomapped2[i] != golden[i]) {
            printf("ERROR: Test failed! Error found in sample %d: golden: %f, hardware: %f\n", i, golden[i], out_bomapped2[i]);
            match = 1;
            break;
        }
    }

    std::cout << "Releasing remaining  objects...\n";
   
    gr1.free(in_bomappedA);
    gr1.free(in_bomappedB);
    gr2.free(in_bomappedC);
    gr1.free(out_bomapped1);
    gr2.free(out_bomapped2);
   
    std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl; 
    return (match ? EXIT_FAILURE :  EXIT_SUCCESS);
}