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

#include "common.h"
void wsync(uint32_t var) {
	uint32_t* addr = (uint32_t*) SYNC_ADDRESS;
	*addr = var;
	__asm__ __volatile__("dmb ish" : : : "memory");
	Xil_DCacheFlushRange((INTPTR)addr, sizeof(uint32_t));
}
uint32_t rsync() {
	uint32_t* addr = (uint32_t*) SYNC_ADDRESS;
	Xil_DCacheInvalidateRange((INTPTR)addr, sizeof(uint32_t));
	__asm__ __volatile__("dmb ish" : : : "memory");
	uint32_t var = *addr;
}
void wdata(int clear) {
	uint32_t* data = (uint32_t*) (SECTION_ADDRESS);
	for (int i = 0; i < SECTION_COUNT; i++) {
		uint32_t* addr = data + i * SECTION_SIZE;
		uint32_t svar = i << 16;
		printf("data[%d] [0x%x] value start from %x\n", i, addr, (clear ==1 ? 0: svar));
		for (int j = 0; j < SECTION_SIZE; j++) {
			*((uint32_t *)(data + i * SECTION_SIZE + j)) = (clear ==1 ? 0: j + svar);
		}
		// xil api bug, if we not flush address data, next Xil_DCacheInvalidateRange will flush instead invalid
		// enable this flush after write logic can fix invalid not work issue
		//Xil_DCacheFlushRange((INTPTR)addr,SECTION_SIZE * sizeof(uint32_t));
	}

}

void rdata() {
	uint32_t* data = (uint32_t*) SECTION_ADDRESS;
	for (int i = SECTION_COUNT - 1; i >= 0; i--) {
		uint32_t* addr = data + i * SECTION_SIZE;
		// if the addr get used with write data but never flush, this invalid will act as flush
		// then it will flush the old data into ddr instead of read data from ddr.
		Xil_DCacheInvalidateRange((INTPTR)addr, SECTION_SIZE*sizeof(uint32_t));
		printf("data[%d] [0x%x] = ", i , addr);
		for (int j = 0; j < 16; j++) {
			uint32_t var = *((uint32_t *)(addr + j));
			printf(" %x ", var);
		}
		printf("\n");
	}
}


void log(char* app) {
	printf("%s version 13\n", app);
}

void apu_disablecache() {
	//Xil_DCacheDisable();
  //Xil_ICacheDisable();

}
