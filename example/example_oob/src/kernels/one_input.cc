/******************************************************************************
* Copyright (C) 2019 - 2022 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <aie_api/utils.hpp>
#include "config.h"
#define PERF_PROF
void OneInput(input_window_uint32 * dataIn, output_window_uint32 * dataOut,
	      output_window_uint32 * result)
{
	static uint32_t a[NUM_A_ELMNTS_PER_TILE];
	static uint32_t b[NUM_COLS];
	static uint32_t intrmdtResult[WIN_SIZE];
	static uint32_t count = 0;
	static uint32_t currentCol;

	currentCol = (get_coreid() & 0x7F0000) >> 16;

	#ifdef PERF_PROF
		int time1 = get_cycles();
	#endif
	printf("OneInput ---- \n");
	for (unsigned i = 0; i < NUM_A_ELMNTS_PER_TILE / WIN_SIZE; i++) {
		printf("one input kernel - i = %d \n", i);
		window_acquire(dataIn);
		for (unsigned w = 0; w < WIN_SIZE / VECTOR_LENGTH; w++)
		chess_prepare_for_pipelining{
			aie::vector<uint32_t, VECTOR_LENGTH> temp = window_readincr_v<VECTOR_LENGTH>(dataIn);
			aie::store_unaligned_v<VECTOR_LENGTH>(a + (i*WIN_SIZE) + (w*VECTOR_LENGTH), temp);
		}
		window_release(dataIn);
	}
	
	#ifdef PERF_PROF
		int time2 = get_cycles();
		int time = time2 - time1;
		
		printf("one_input::[V]Reading from dataIn into matrix A took %d \n", time);
		time1=get_cycles();
	#endif

	for (unsigned i = 0;
		      i < NUM_A_ELMNTS_PER_TILE *
		      		(NUM_HW_COLS - currentCol - 1) / WIN_SIZE;
		      i++)
	{
		printf("forward packet i = %d \n", i);
		window_acquire(dataOut);
		window_acquire(dataIn);
		for (unsigned w = 0; w < WIN_SIZE / VECTOR_LENGTH; w++)
		chess_prepare_for_pipelining{
			window_writeincr(dataOut, window_readincr_v<VECTOR_LENGTH>(dataIn));
		}
		window_release(dataIn);
		window_release(dataOut);
	}

	#ifdef PERF_PROF
		time2 = get_cycles();
		time = time2 - time1;
		printf("one_input::[V]Reading from dataIn into dataOut took %d \n", time);
	#endif
	/*
	 * read one column of b, pass the same to the next core,
	 * compute matrix multiplication of 'a' rows x 'b' col and
	 * finally output the result
	 */
	for (unsigned i = 0; i < NUM_COLS; i++) {
		/* read 1 entire column of b */
		#ifdef PERF_PROF
			time1 = get_cycles();
		#endif

		for (unsigned w = 0; w < (NUM_COLS/WIN_SIZE); w++) {
			window_acquire(dataOut);
			window_acquire(dataIn);
			printf("forwarding b w = %d\n", w);
			for (unsigned x = 0; x < WIN_SIZE / VECTOR_LENGTH; x++) 
			chess_prepare_for_pipelining{
				aie::vector<uint32_t, VECTOR_LENGTH> temp = window_readincr_v<VECTOR_LENGTH>(dataIn);
				aie::store_unaligned_v<VECTOR_LENGTH>(b + (w*WIN_SIZE) + (x*VECTOR_LENGTH), temp);
				window_writeincr(dataOut, aie::load_v<VECTOR_LENGTH>(b + (w*WIN_SIZE) + (x*VECTOR_LENGTH)));
			}
			window_release(dataIn);
			window_release(dataOut);
		}

		#ifdef PERF_PROF
			time2 = get_cycles();
			time = time2 - time1;
			printf("one_input::[V]Reading from dataIn into b; b into dataOut took %d \n", time);

			time1 = get_cycles();
		#endif

		for (unsigned k = 0; k < NUM_ROWS_PER_TILE; k++)
		{
			uint32_t add_result = 0;
			for (unsigned l = 0; l < NUM_COLS / VECTOR_LENGTH; l++) chess_prepare_for_pipelining{
				aie::vector<uint32_t, VECTOR_LENGTH> va = aie::load_v<VECTOR_LENGTH>(a + (k*NUM_COLS) + (l* VECTOR_LENGTH));
				aie::vector<uint32_t, VECTOR_LENGTH> vb = aie::load_v<VECTOR_LENGTH>(b + (l*VECTOR_LENGTH)); // cuz we just be dealin with one column of b at a time 
				aie::accum<acc64, VECTOR_LENGTH> vres = aie::mul(va, vb);
				aie::vector<uint32_t, VECTOR_LENGTH> result_vector = vres.to_vector<uint32_t>(0);
				add_result = add_result + aie::reduce_add(result_vector);
			}
			intrmdtResult[count++] = add_result;
		}

		#ifdef PERF_PROF
			time2 = get_cycles();
			time = time2 - time1;
			printf("one_input::[V]aie::mul Compute took %d \n", time);
		#endif

		if (count == WIN_SIZE) {
			
			#ifdef PERF_PROF
				time1 = get_cycles();
			#endif

			window_acquire(result);
			for (unsigned z = 0; z < WIN_SIZE/VECTOR_LENGTH; z++) chess_prepare_for_pipelining{
				window_writeincr(result, aie::load_v<VECTOR_LENGTH>(intrmdtResult + (z * VECTOR_LENGTH)));
				//reset 
				for (unsigned i = 0; i < VECTOR_LENGTH; ++i)
				{
					intrmdtResult[(z * VECTOR_LENGTH) + i] = 0;
				}
			}
			window_release(result);
			count = 0;
			
			#ifdef PERF_PROF
				time2=get_cycles();
				time = time2 - time1;
				printf("one_input::[V]Resykt write took %d \n", time);
			#endif
		}
	}
}
