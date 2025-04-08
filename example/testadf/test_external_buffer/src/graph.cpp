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

#include "graph.h"

#define NITERATIONS 1
#define InSizePerIteration 256
#define OutSizePerIteration 256

using namespace std;
using namespace adf;

myGraph EG;

const int InSize = NITERATIONS*InSizePerIteration;
const int OutSize = NITERATIONS*OutSizePerIteration;

int main(int argc, char ** argv) {

uint32_t* input_data = (uint32_t*)GMIO::malloc(InSize*sizeof(uint32_t));
uint32_t* output_data = (uint32_t*)GMIO::malloc(OutSize*sizeof(uint32_t));

for(uint32_t i = 0;i<InSize;i++) 
  input_data[i] = i;

for(uint32_t i = 0;i<OutSize;i++) // 
  output_data[i] = 99999;

EG.init();

for(int kiter=0;kiter<NITERATIONS;kiter++)
{
  cout << "Iteration: " << kiter << endl;

  EG.ddrin.setAddress(input_data + kiter*InSizePerIteration);
  EG.ddrout.setAddress(output_data + kiter*OutSizePerIteration);
  cout << "\tAddresses set. " << endl;

  EG.ddrin.gm2aie_nb(EG.ddrin.out[0]);
  EG.ddrout.aie2gm_nb(EG.ddrout.in[0]);
  cout << "\tTransactions sent. " << endl;

  EG.run(1);

  EG.ddrin.wait(EG.ddrin.out[0]);
  EG.ddrout.wait(EG.ddrout.in[0]);
  EG.wait();
}


cout << "OFM Interface DMA transfer done, ready for file I/O!!" << endl;

for(uint32_t i = 0;i<InSize;i++)
{ 
  cout<<input_data[i] <<" "<<output_data[i]<<endl;
  if(output_data[i] != input_data[i]*input_data[i])
  {
      cout<<"Test failed\n";
     
      GMIO::free(input_data);
      GMIO::free(output_data);
      return -1;
  }
}

EG.end();
cout<<"Test Passed\n";
GMIO::free(input_data);
GMIO::free(output_data);

return 0;
}