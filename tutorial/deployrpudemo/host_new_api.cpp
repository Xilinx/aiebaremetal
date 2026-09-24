// Copyright (C) 2026 Advanced Micro Devices, Inc.
// Host application using the New (inheritance-based) API.

#include <fstream>
#include "xil_printf.h"
#include "xil_io.h"
#include "xil_cache.h"
#include <xaiengine.h>
#include "aeg_baremetal_api.h"
#include "generated_graphs.h"

#define DATA_LENGTH 128

AEGGraph<aeg::gradf> gr;

int main(int argc, char ** argv) {
	gr.init();

	int32_t* inputArray = (int32_t*)gr.malloc(DATA_LENGTH * sizeof(int32_t));
	int32_t* outputArray = (int32_t*)gr.malloc(DATA_LENGTH * sizeof(int32_t));
	int32_t* outputArray_gold = (int32_t*)gr.malloc(DATA_LENGTH * sizeof(int32_t));

	int param = 5;
	gr.sq.in[1].update(reinterpret_cast<char*>(&param), sizeof(int));

	gr.run(1);

	for (int i = 0; i < DATA_LENGTH; i++) {
		inputArray[i] = i + 1;
		outputArray_gold[i] = inputArray[i] * inputArray[i] + param;
	}

	gr.in.gm2aie_nb(reinterpret_cast<char*>(&inputArray[0]), DATA_LENGTH * sizeof(int32_t));
	gr.out.aie2gm_nb(reinterpret_cast<char*>(&outputArray[0]), DATA_LENGTH * sizeof(int32_t));
	gr.out.gmio_wait();
	gr.wait();
	gr.end();

	for (int i = 0; i < DATA_LENGTH; i++) {
		std::cout << "value " << outputArray[i] << ", ";
		if (outputArray[i] != outputArray_gold[i]) {
			std::cout << "\n at data[" << i << "] incorrect Test Failed \n";
			return -1;
		}
	}

	std::cout << "\nTest Passed (New API)\n";

	gr.free(inputArray);
	gr.free(outputArray);
	gr.free(outputArray_gold);
	return 0;
}
