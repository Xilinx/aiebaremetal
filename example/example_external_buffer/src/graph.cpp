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

//#include "graph.h"
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "graph.h"
myGraph EG;
#else
#include "xil_printf.h"
#include "xil_io.h"
#include "xil_cache.h"
#include "xtime_l.h"
#endif

#include <fstream>
#include <xaiengine.h>
#include "aiebaremetal.h"
#include "config.h"

#ifdef _RPU_
//set rpu support
uint8_t rpu = 1;
#else
uint8_t rpu = 0;
#endif

AbrGraph gr("EG", rpu);

#define input_width 3
#define input_height 3
#define filter_size_1 3
#define in_channels_1 3
#define out_channels_1 2
#define filter_size_2 3
#define in_channels_2 2
#define out_channels_2 1

#define NITERATIONS 1
#define InSizePerIteration input_width*input_height*in_channels_1
#define OutSizePerIteration input_width*input_height*out_channels_2

using namespace std;
//using namespace adf;

//myGraph EG;

const int InSize = NITERATIONS*InSizePerIteration;
const int OutSize = NITERATIONS*OutSizePerIteration;

int main(int argc, char ** argv) {

#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
 EG.init();
#else
#ifdef _RPU_
 //RPU need to disable data cache, without cache diable app will stuck
	Xil_DCacheDisable();
	//Xil_ICacheDisable();
#endif
#endif

//gr.getaiedevinst();

int32_t* input_data = (int32_t*)gr.malloc(InSize*sizeof(int32_t));
int32_t* weights1_data = (int32_t*)gr.malloc(in_channels_1*out_channels_1*filter_size_1*filter_size_1*sizeof(int32_t));
int32_t* weights2_data = (int32_t*)gr.malloc(in_channels_2*out_channels_2*filter_size_1*filter_size_2*sizeof(int32_t));

int32_t* output_data = (int32_t*)gr.malloc(OutSize*sizeof(int32_t));

for(int i = 0;i < in_channels_1;i++)
{
  for(int j = 0; j< input_width*input_height ; j++)
  {
    input_data[i*input_width*input_height + j] = i+1;
  }
}

// First convolution weight
for(int k = 0;k < out_channels_1; k++)
{
  for(int i = 0;i < in_channels_1;i++)
  {
    for(int j = 0; j< filter_size_1*filter_size_1 ; j++)
    {
      weights1_data[k*filter_size_1*filter_size_1*in_channels_1 + i*filter_size_1*filter_size_1 + j] = 1;
    }
  }
}

// second convolution weight
for(int k = 0;k < out_channels_2; k++)
{
  for(int i = 0;i < in_channels_2;i++)
  {
    for(int j = 0; j< filter_size_2*filter_size_2 ; j++)
    {
      weights2_data[k*filter_size_2*filter_size_2*in_channels_2 + i*filter_size_2*filter_size_2 + j] = 1;
    }
  }
}
// for(uint32_t i = 0;i<InSize;i++) 
//   input_data[i] = i;

// Test different tiles with different values
// int row_start = 0;
// for(int i = 0;i < rows; i++)
// {
//   if((i != 0) && ((i % 4) == 0)) row_start+= 3;
//   for(int j = 0; j < cols; j++)
//   {
//     input_data[i*cols + j] = i/4 + j/4 + row_start;
//   }
  
// }

cout << " Input data"<< endl;

for(int k = 0;k < in_channels_1;k++)
{
  for(int i = 0;i < input_height;i++)
  {
    for(int j = 0 ; j < input_width;j++)
    
      {
        cout<<input_data[k*input_width*input_height + i*input_width + j]<<" ";
      }
    cout<<endl;
  } 
  cout<<endl;
}
cout << "weight_1 data"<< endl;

for(int l = 0;l < out_channels_1;l++)
{
  for(int i = 0;i < in_channels_1;i++)
  {
    for(int j = 0;j < filter_size_1;j++)
    {
      for(int k = 0 ; k < filter_size_1;k++)
      
        {
          cout<<weights1_data[l*filter_size_1*filter_size_1*in_channels_1 + i*filter_size_1*filter_size_1 + j*filter_size_1 + k]<<" ";
        }
      cout<<endl;
    } 
    cout <<endl<<endl;
  }
  cout<<"**************************"<<endl;
}
cout << "weight_2 data"<< endl;

for(int l = 0;l < out_channels_2;l++)
{
  for(int i = 0;i < in_channels_2;i++)
  {
    for(int j = 0;j < filter_size_2;j++)
    {
      for(int k = 0 ; k < filter_size_2;k++)
      
        {
          cout<<weights2_data[l*filter_size_2*filter_size_2*in_channels_2 + i*filter_size_2*filter_size_2 + j*filter_size_2 + k]<<" ";
        }
      cout<<endl;
    } 
    cout <<endl<<endl;
  }
  cout<<"**************************"<<endl;
}


for(uint32_t i = 0;i<OutSize;i++) 
  output_data[i] = 99999;

gr.init();

for(int kiter=0;kiter<NITERATIONS;kiter++)
{
  cout << "Iteration: " << kiter << endl;

  gr.setAddress("EG.ddrin",input_data + kiter*InSizePerIteration);
  gr.setAddress("EG.weights_1",weights1_data + kiter*out_channels_1*in_channels_1*filter_size_1*filter_size_1);
  gr.setAddress("EG.weights_2",weights2_data + kiter*out_channels_2*in_channels_2*filter_size_2*filter_size_2);
  gr.setAddress("EG.ddrout",output_data + kiter*OutSizePerIteration);
  cout << "\tAddresses set. " << endl;

  gr.gm2aie_nb("EG.ddrin","EG.ddrin.out[0]");
  gr.gm2aie_nb("EG.weights_1","EG.weights_1.out[0]");
  gr.gm2aie_nb("EG.weights_2","EG.weights_2.out[0]");
  gr.aie2gm_nb("EG.ddrout","EG.ddrout.in[0]");
  cout << "\tTransactions sent. " << endl;

  gr.run(1);

  gr.wait("EG.ddrin","EG.ddrin.out[0]");
  gr.wait("EG.weights_1","EG.weights_1.out[0]");
  gr.wait("EG.weights_2","EG.weights_2.out[0]");
  gr.wait("EG.ddrout","EG.ddrout.in[0]");
  gr.wait();
}


cout << "OFM Interface DMA transfer done, ready for file I/O!!" << endl;
cout << " output data"<< endl;
for(int k = 0;k < out_channels_2;k++)
{
  for(int i = 0;i < input_height;i++)
  {
    for(int j = 0 ; j < input_width;j++)
    {
      cout<<output_data[k*input_width*input_height + i*input_width + j]<<" ";
    }
    cout << endl;
  } 
  // if(output_data[i] != input_data[i]*input_data[i])
  // {
  //     cout<<"Test failed\n";
     
  //     GMIO::free(input_data);
  //     GMIO::free(output_data);
  //     return -1;
  // }
}

gr.end();
//cout << "Test Passed\n";
gr.free(input_data);
gr.free(output_data);
gr.free(weights1_data);
gr.free(weights2_data);

return 0;
}