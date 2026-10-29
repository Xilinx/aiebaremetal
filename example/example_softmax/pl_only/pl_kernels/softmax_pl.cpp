/*
Copyright (C) 2026 Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/

// PL (HLS) softmax kernel for the pl_only example. Pure AXI-Stream in/out —
// no m_axi pointer arg, since data arrives from/leaves to mm2s/s2mm over
// streams, not DDR directly. Computes exp(x)/sum(exp(x)) independently per
// TILE_LEN-element tile, matching example_softmax/aie_only/src/softmax.cc's
// math exactly (no max-subtraction) so the two examples are numerically
// comparable.
//
// mm2s/s2mm move raw 32-bit words; the payload here is actually `float`
// reinterpreted through its bit pattern (see bits_to_float / float_to_bits).
//
// Invoked once per host run over the whole buffer; loops over tiles
// internally here so each TILE_LEN-element tile still gets its own
// independent softmax.

#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <math.h>
#include "../src/common.h" // TILE_LEN: compile-time per-tile softmax width

static float bits_to_float(ap_uint<32> bits) {
    union {
        ap_uint<32> u;
        float f;
    } conv;
    conv.u = bits;
    return conv.f;
}

static ap_uint<32> float_to_bits(float val) {
    union {
        ap_uint<32> u;
        float f;
    } conv;
    conv.f = val;
    return conv.u;
}

extern "C" {

void softmax_pl(hls::stream<ap_axis<32, 0, 0, 0>> &in, hls::stream<ap_axis<32, 0, 0, 0>> &out, int size) {
#pragma HLS interface axis port=in
#pragma HLS interface axis port=out
#pragma HLS INTERFACE s_axilite port=size bundle=control
#pragma HLS interface s_axilite port=return bundle=control

    float buf[TILE_LEN];
    float exp_values[TILE_LEN];
#pragma HLS ARRAY_PARTITION variable=buf complete
#pragma HLS ARRAY_PARTITION variable=exp_values complete

    // `size` is the TOTAL element count across all tiles. TILE_LEN is a
    // compile-time constant, so this division is resolved at synthesis
    // time, not a runtime cost.
    int num_tiles = size / TILE_LEN;

    for (int t = 0; t < num_tiles; t++) {
        for (int i = 0; i < TILE_LEN; i++) {
#pragma HLS PIPELINE II=1
            ap_axis<32, 0, 0, 0> x = in.read();
            buf[i] = bits_to_float(x.data);
        }

        float sum = 0.0f;
        for (int i = 0; i < TILE_LEN; i++) {
#pragma HLS PIPELINE II=1
            exp_values[i] = exp(buf[i]);
            sum += exp_values[i];
        }

        for (int i = 0; i < TILE_LEN; i++) {
#pragma HLS PIPELINE II=1
            ap_axis<32, 0, 0, 0> y;
            y.data = float_to_bits(exp_values[i] / sum);
            y.keep = -1;
            y.strb = -1;
            // This stream feeds s2mm directly (PL-to-PL, no PLIO bridge),
            // which reads by fixed count rather than TLAST, but asserting
            // TLAST correctly keeps this kernel AXI4-Stream compliant.
            // Only the last beat of the last tile is the true end.
            y.last = (t == num_tiles - 1 && i == TILE_LEN - 1) ? 1 : 0;
            out.write(y);
        }
    }
}

}
