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

#include "xil_printf.h"
//#include "xaiengine.h"
#include "xil_io.h"
#include "xil_cache.h"
#include <stdio.h>
//#include <math.h>
#include "xtime_l.h"
#include "common.h"

int main(int argc, char* argv[]) {
	void* baseaddr_p = aligned_alloc(16,2048);
	void* baseaddr_p1 = aligned_alloc(16,2048);
	void* baseaddr_p2 = aligned_alloc(16,2048);
	printf("a baseaddr_p = %p\n", baseaddr_p);
	printf("baseaddr_p1 = %p\n", baseaddr_p1);
	printf("baseaddr_p2 = %p\n", baseaddr_p2);
	log("apu");
	apu_disablecache();
	wsync(0);
	wdata(1);//clear the data
	while(rsync() == 0);
	rdata();
	return 0;
}
