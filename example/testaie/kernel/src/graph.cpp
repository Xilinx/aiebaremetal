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

#include "aiebaremetal.h"
//#ifndef __BAREMETAL_HW__
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf;
BaremetalGraphSim gr("gradf");
#include "app.h"
extern "C" {
	void ess_Write32(uint64 Addr, uint Data);
	uint32 ess_Read32(uint64 addr);
}
#else
BaremetalGraph gr("gradf");
#endif


int main(int argc, char ** argv)
{
	//trigger the ps_ix.so load and the elf load
	//gradf.init();
	gr.selftestexit();
  return 0;
}
