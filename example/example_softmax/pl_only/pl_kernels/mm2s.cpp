/*
Copyright (C) 2025 Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/


#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>


extern "C" {

void mm2s(ap_int<32>* mem, hls::stream<ap_axis<32, 0, 0, 0>  >& s, int size) {
#pragma HLS INTERFACE m_axi port=mem offset=slave bundle=gmem

#pragma HLS interface axis port=s

#pragma HLS INTERFACE s_axilite port=mem bundle=control
#pragma HLS INTERFACE s_axilite port=size bundle=control
#pragma HLS interface s_axilite port=return bundle=control

	for(int i = 0; i < size; i++) {
#pragma HLS PIPELINE II=1
		ap_axis<32, 0, 0, 0> x;
		x.data = mem[i];
		x.keep = -1; // all bytes valid (full 32-bit word)
		x.strb = -1;
		// TLAST on the final beat: required for the PLIO<->AXI4-Stream
		// bridge v++ inserts between this kernel and the AIE core to close
		// out a window and re-arm correctly.
		x.last = (i == size - 1) ? 1 : 0;
		s.write(x);
	}

}

}
