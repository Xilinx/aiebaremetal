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
mygraph gradf;
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

//M is starting index
#define M 0
//P is number of GMIO channels to test
#define P 32
//ITER is number of graph iteration
#define ITER 8

#ifdef __AIESIM__
int main()
{

   #if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
	//std::cout << "gradf.a  =" << gradf.a << std::endl;
	gradf.init();
   #else
	Xil_DCacheDisable();
	Xil_ICacheDisable();

	//get time
   XTime tStart, tEnd;

   //init_platform();

   XTime_GetTime(&tStart);
  #endif


	gr.init();
    
    for (int i=M; i<M+P; i++)
    {
        std::string str("].in[1]");
	    std::string port;
	    port = "gradf.k[" + std::to_string(i) + str;
	    int data = i + 1;
	    gr.update(port.c_str(), reinterpret_cast<const char *>(&data), sizeof (int));
    }
    std::cout << "update RTO is done" << std::endl;

    int32_t *inBOMap[N];
    for (int i=M; i<M+P; i++)
    {
        inBOMap[i] = (int32_t*)gr.malloc(256*sizeof(int32_t));
	
	    for (int j = 0; j < 256; j++){
            inBOMap[i][j] = i + 1;
        }
    }

    int32_t *outBOMap[N];
    for (int i=M; i<M+P; i++)
    {
        outBOMap[i] = (int32_t*)gr.malloc(256*sizeof(int32_t));
	
	    for (int j = 0; j < 256; j++){
            outBOMap[i][j] = 0xABCDEF00;
        }
    }
    std::cout << "data is ready" << std::endl;
    
    gr.run(ITER);
    
    for (int j=0; j<ITER; j++)
    {

        for (int i=M; i<M+P; i++)
        {
            std::string gmioport;
            gmioport = "gradf.input["+ std::to_string(i)+"]";
            std::cout<<"Input port: "<<gmioport<<std::endl;
            gr.gm2aie_nb(gmioport.c_str(),reinterpret_cast<char*> (&inBOMap[i][j * 32]), 32 * sizeof (int32_t) );

            gmioport = "gradf.output["+ std::to_string(i)+"]";
            std::cout<<"output port: "<<gmioport<<std::endl;
            gr.aie2gm_nb(gmioport.c_str(),reinterpret_cast<char*>(&outBOMap[i][j * 32] ), 32 * sizeof (int32_t) );
       
        }
    }

    for(int i=M; i<M+P; i++)
    {
        std::string gmioport = "gradf.output["+ std::to_string(i)+"]";
        gr.gmio_wait(gmioport.c_str()); 
    }
    std::cout << "****************************GMIO is done************************************" << std::endl;
    
    gr.wait();
   
    //check results
    int errorCount = 0;
    for (int i=M; i<M+P; i++)
    {
        for (int j=0; j<32*ITER; j++)
        {
            if (outBOMap[i][j] != (1+i)*2) {
	        errorCount++;
	    }
        }
    }
    if (errorCount)
       printf("Test failed with %d errors\n", errorCount);
    else
       printf("Test passed\n");

    gradf.end();

    #if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
    XTime_GetTime(&tEnd);

    printf("Output took %llu clock cycles.\n", 2*(tEnd - tStart));
    printf("Output took %.2f us.\n",
            1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND/1000000));
    #endif
	
    for (int i=M; i<M+P; i++){
        gr.free(inBOMap[i]);
        gr.free(outBOMap[i]);
    }
    std::cout << "TEST " << (errorCount ? "FAILED" : "PASSED") << std::endl; 
    return (errorCount ? EXIT_FAILURE :  EXIT_SUCCESS);

	//gr.end();
    return 0;
}
#endif
