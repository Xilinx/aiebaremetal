// © Copyright 2022 – 2023 Xilinx, Inc. All rights reserved.
// This file contains confidential and proprietary information of Xilinx, Inc. and is protected under U.S. and
// international copyright and other intellectual property laws.
// DISCLAIMER
// This disclaimer is not a license and does not grant any rights to the materials distributed herewith.
// Except as otherwise provided in a valid license issued to you by Xilinx, and to the maximum extent
// permitted by applicable law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL
// FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR
// STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether
// in contract or tort, including negligence, or under any other theory of liability) for any loss or damage of
// any kind or nature related to, arising under or in connection with these materials, including for any
// direct, or any indirect, special, incidental, or consequential loss or damage (including loss of data,
// profits, goodwill, or any type of loss or damage suffered as a result of any action brought by a third
// party) even if such damage or loss was reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-safe, or for use in any application requiring fail-
// safe performance, such as life-support or safety devices or systems, Class III medical devices, nuclear
// facilities, applications related to the deployment of airbags, or any other applications that could lead to
// death, personal injury, or severe property or environmental damage (individually and collectively,

//"Critical Applications"). Customer assumes the sole risk and liability of any use of Xilinx products in
// Critical Applications, subject only to applicable laws and regulations governing limitations on product
// liability.
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT ALL TIMES.

#ifndef _DSPLIB_TEST_HPP_
#define _DSPLIB_TEST_HPP_

#include "kernel.h"
#include <adf.h>
#include <vector>

#define Q(x) #x
#define QUOTE(x) Q(x)
#define GITER 1

#ifndef UUT_GRAPH
#define UUT_GRAPH fft_ifft_dit_1ch_graph
#endif

#define DATA_TYPE_COL cint32
#define DATA_TYPE_ROW cint16
#define TWIDDLE_TYPE cint16

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace dsplib = xf::dsp::aie;
int inst1 = 0;
int inst2 = 0;
int placement_ctrl = 0;
class RangeFFT : public graph {
  private:
  public:
    input_port in;
    output_port out;

    RangeFFT() {
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Point size           = %d \n", 2048);
        printf("FFT/nIFFT            = %d \n", 1);
        printf("Final scaling Shift  = %d \n", 5);
        printf("Number of kernels    = %d \n", 1);
        printf("Dynamic point size   = %d \n", 0);
        printf("Window Size          = %d \n", 2048);
        printf("API_IO               = %d \n", 0);
        printf("PARALLEL_POWER       = %d \n", 0);
        printf("Data type            = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("TWIDDLE type         = ");
        printf(QUOTE(TWIDDLE_TYPE));
        printf("\n");
        printf("PARAMETERS OF TEST:\n-------------------\n");

        printf("========================\n");

        dsplib::fft::dit_1ch::UUT_GRAPH<DATA_TYPE_ROW, TWIDDLE_TYPE, 2048, 1, 5, 1, 0, 2048, 0, 0> fftGraph_2048;
        for (int i = 0; i < ((1 + 0) << 0); i++) {
            runtime<ratio>(*fftGraph_2048.getKernels()) = 0.9;
            connect<>(in, fftGraph_2048.in[i]);
            connect<>(fftGraph_2048.out[i], out);
            location<kernel>((*fftGraph_2048.getKernels())) = tile(inst1 + 2 + placement_ctrl, 0);
            location<buffer>((*fftGraph_2048.getKernels()).in[i]) = {address(inst1 + 2 + placement_ctrl, 0, 0x0000),
                                                                     address(inst1 + 2 + placement_ctrl, 0, 0x4000)};
            location<buffer>((*fftGraph_2048.getKernels()).out[i]) = {address(inst1 + 2 + placement_ctrl, 0, 0x6000),
                                                                      address(inst1 + 2 + placement_ctrl, 0, 0x2000)};
            location<adf::parameter>((fftGraph_2048.FFTwinproc.fft_lut2[0])) =
                address(inst1 + 1 + placement_ctrl, 0, 0xC000);
            location<adf::parameter>((fftGraph_2048.FFTwinproc.fft_lut3[0])) =
                address(inst1 + 1 + placement_ctrl, 0, 0xC800);
            location<adf::parameter>((fftGraph_2048.FFTwinproc.fft_lut4[0])) =
                address(inst1 + 1 + placement_ctrl, 0, 0xEC00);
            location<adf::parameter>((fftGraph_2048.FFTwinproc.fft_buf1[0])) =
                address(inst1 + 2 + placement_ctrl, 0, 0x8000);
            location<adf::parameter>((fftGraph_2048.FFTwinproc.fft_buf2)) =
                address(inst1 + 2 + placement_ctrl, 0, 0xC000);
            location<graph>(*this) =
                area_group({{aie_tile, inst1 + 1 + placement_ctrl, 0, inst1 + 2 + placement_ctrl, 0}});
        }
        inst1 = inst1 + 2;
    };
};
int inst3 = 0;
class cint16_to_cint32 : public graph {
  public:
    input_port in;
    output_port out;

    kernel k1;
    cint16_to_cint32() {

        k1 = kernel::create(c16toc32);

        connect<window<256 * sizeof(cint16)>>(in, k1.in[0]);

        connect<window<256 * sizeof(cint32)>>(k1.out[0], out);
        location<buffer>(k1.in[0]) = {address(inst3 + 1 + placement_ctrl, 0, 0x0000),
                                      address(inst3 + 1 + placement_ctrl, 0, 0x4000)};
        runtime<ratio>(k1) = 0.3;
        location<kernel>(k1) = tile(inst3 + 1 + placement_ctrl, 0);
        source(k1) = "src/kernel.cc";
        inst3 = inst3 + 2;
    }
};

class VelocityFFT : public graph {
  private:
  public:
    input_port in;
    output_port out;

    VelocityFFT() {
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Point size           = %d \n", 64);
        printf("FFT/nIFFT            = %d \n", 1);
        printf("Final scaling Shift  = %d \n", 4);
        printf("Number of kernels    = %d \n", 1);
        printf("Dynamic point size   = %d \n", 0);
        printf("Window Size          = %d \n", 256);
        printf("API_IO               = %d \n", 0);
        printf("PARALLEL_POWER       = %d \n", 0);
        printf("Data type            = ");
        printf(QUOTE(DATA_TYPE_COL));
        printf("\n");
        printf("TWIDDLE type         = ");
        printf(QUOTE(TWIDDLE_TYPE));
        printf("\n");
        printf("PARAMETERS OF TEST:\n-------------------\n");

        printf("========================\n");

        dsplib::fft::dit_1ch::UUT_GRAPH<DATA_TYPE_COL, TWIDDLE_TYPE, 64, 1, 4, 1, 0, 256, 0, 0> fftGraph_64;
        for (int i = 0; i < ((1 + 0) << 0); i++) {
            runtime<ratio>(*fftGraph_64.getKernels()) = 0.3;
            connect<>(in, fftGraph_64.in[i]);
            connect<>(fftGraph_64.out[i], out);
            location<kernel>((*fftGraph_64.getKernels())) = tile(1 + inst2 + placement_ctrl, 0);
            location<buffer>((*fftGraph_64.getKernels()).out[i]) = {address(inst2 + 1 + placement_ctrl, 0, 0x7000),
                                                                    address(inst2 + 1 + placement_ctrl, 0, 0x3000)};
            location<adf::parameter>((fftGraph_64.FFTwinproc.fft_buf1[0])) =
                address(inst2 + 1 + placement_ctrl, 0, 0xe000);
            location<buffer>((*fftGraph_64.getKernels()).in[i]) = {address(inst2 + 1 + placement_ctrl, 0, 0x2000),
                                                                   address(inst2 + 1 + placement_ctrl, 0, 0xe000)};
            // location<graph>(*this) = area_group({{aie_tile, inst2+1 + placement_ctrl, 0, inst2+1 + placement_ctrl,
            // 0}});
        }
        inst2 = inst2 + 2;
    };
};

int shimloc[] = {2, 3, 6, 7, 14, 15, 22, 23};

class FFT2D_graphall : public graph {
  public:
    input_gmio gm_in[4];
    output_gmio gm_out[4];
    // input_plio pl_in[1];
    // output_plio pl_out[1];
    input_gmio input_gmaie[4];
    output_gmio output_aiegm[4];
    RangeFFT fft_2048[4];
    cint16_to_cint32 cint16_cint32[4];
    VelocityFFT fft_64[4];
    shared_buffer<cint16> memtile1_buffer[4];
    shared_buffer<cint16> memtile2_buffer[4];

    FFT2D_graphall() {

        // pl_in[0] = input_plio::create("DataIn0", plio_128_bits, "data/input1.txt");
        //  pl_in[1] = input_plio::create("DataIn1", plio_128_bits, "data/input1.txt");
        //  pl_in[2] = input_plio::create("DataIn2", plio_128_bits, "data/input1.txt");
        //  pl_in[3] = input_plio::create("DataIn3", plio_128_bits, "data/input1.txt");
        // pl_out[0] = output_plio::create("DataOut0", plio_128_bits, "data/output1.txt");
        // pl_out[1] = output_plio::create("DataOut1", plio_128_bits, "data/output2.txt");
        // pl_out[2] = output_plio::create("DataOut2", plio_128_bits, "data/output3.txt");
        // pl_out[3] = output_plio::create("DataOut3", plio_128_bits, "data/output4.txt");

        gm_in[0] = input_gmio::create("gm_in[" + std::to_string(0) + "]", 64, 1000);
        gm_in[1] = input_gmio::create("gm_in[" + std::to_string(1) + "]", 64, 1000);
        gm_in[2] = input_gmio::create("gm_in[" + std::to_string(2) + "]", 64, 1000);
        gm_in[3] = input_gmio::create("gm_in[" + std::to_string(3) + "]", 64, 1000);
        gm_out[0] = output_gmio::create("gm_out[" + std::to_string(0) + "]", 64, 1000);
        gm_out[1] = output_gmio::create("gm_out[" + std::to_string(1) + "]", 64, 1000);
        gm_out[2] = output_gmio::create("gm_out[" + std::to_string(2) + "]", 64, 1000);
        gm_out[3] = output_gmio::create("gm_out[" + std::to_string(3) + "]", 64, 1000);

        input_gmaie[0] = input_gmio::create("DataIn_GMIO" + std::to_string(0), 64, 1);
        input_gmaie[1] = input_gmio::create("DataIn_GMIO" + std::to_string(1), 64, 1);
        input_gmaie[2] = input_gmio::create("DataIn_GMIO" + std::to_string(2), 64, 1);
        input_gmaie[3] = input_gmio::create("DataIn_GMIO" + std::to_string(3), 64, 1);
        output_aiegm[0] = output_gmio::create("DataOut_GMIO" + std::to_string(0), 64, 1);
        output_aiegm[1] = output_gmio::create("DataOut_GMIO" + std::to_string(1), 64, 1);
        output_aiegm[2] = output_gmio::create("DataOut_GMIO" + std::to_string(2), 64, 1);
        output_aiegm[3] = output_gmio::create("DataOut_GMIO" + std::to_string(3), 64, 1);
        // location<PLIO>(pl_in[0]) = shim(5);
        //  location<PLIO>(pl_in[1]) = shim(6);
        //  location<PLIO>(pl_in[2]) = shim(7);
        //  location<PLIO>(pl_in[3]) = shim(8);
        // location<PLIO>(pl_out[0]) = shim(5);
        //  location<PLIO>(pl_out[1]) = shim(6);
        //  location<PLIO>(pl_out[2]) = shim(7);
        //  location<PLIO>(pl_out[3]) = shim(8);

        location<GMIO>(gm_in[0]) = shim(14);
        location<GMIO>(gm_out[0]) = shim(14);

        location<GMIO>(gm_in[1]) = shim(15);
        location<GMIO>(gm_out[1]) = shim(15);
        location<GMIO>(gm_in[2]) = shim(22);
        location<GMIO>(gm_out[2]) = shim(22);
        location<GMIO>(gm_in[3]) = shim(23);
        location<GMIO>(gm_out[3]) = shim(23);

        location<GMIO>(input_gmaie[0]) = shim(shimloc[0]);
        location<GMIO>(input_gmaie[1]) = shim(shimloc[1]);
        location<GMIO>(input_gmaie[2]) = shim(shimloc[2]);
        location<GMIO>(input_gmaie[3]) = shim(shimloc[3]);
        location<GMIO>(output_aiegm[0]) = shim(shimloc[0]);
        location<GMIO>(output_aiegm[1]) = shim(shimloc[1]);
        location<GMIO>(output_aiegm[2]) = shim(shimloc[2]);
        location<GMIO>(output_aiegm[3]) = shim(shimloc[3]);

        memtile1_buffer[0] = shared_buffer<cint16>::create({1024, 48}, 1, 1);
        memtile1_buffer[1] = shared_buffer<cint16>::create({1024, 48}, 1, 1);
        memtile1_buffer[2] = shared_buffer<cint16>::create({1024, 48}, 1, 1);
        memtile1_buffer[3] = shared_buffer<cint16>::create({1024, 48}, 1, 1);
        memtile2_buffer[0] = shared_buffer<cint16>::create({192, 64, 2}, 1, 1);
        memtile2_buffer[1] = shared_buffer<cint16>::create({192, 64, 2}, 1, 1);
        memtile2_buffer[2] = shared_buffer<cint16>::create({192, 64, 2}, 1, 1);
        memtile2_buffer[3] = shared_buffer<cint16>::create({192, 64, 2}, 1, 1);
        num_buffers(memtile1_buffer[0]) = 2;
        num_buffers(memtile1_buffer[1]) = 2;
        num_buffers(memtile1_buffer[2]) = 2;
        num_buffers(memtile1_buffer[3]) = 2;
        num_buffers(memtile2_buffer[0]) = 2;
        num_buffers(memtile2_buffer[1]) = 2;
        num_buffers(memtile2_buffer[2]) = 2;
        num_buffers(memtile2_buffer[3]) = 2;
        location<buffer>(memtile1_buffer[0]) = {address((2 + (2 * 0) - 1), 1, 0),
                                                address((2 + (2 * 0) - 1), 1, 262144)};
        location<buffer>(memtile1_buffer[1]) = {address((2 + (2 * 1) - 1), 1, 0),
                                                address((2 + (2 * 1) - 1), 1, 262144)};
        location<buffer>(memtile1_buffer[2]) = {address((2 + (2 * 2) - 1), 1, 0),
                                                address((2 + (2 * 2) - 1), 1, 262144)};
        location<buffer>(memtile1_buffer[3]) = {address((2 + (2 * 3) - 1), 1, 0),
                                                address((2 + (2 * 3) - 1), 1, 262144)};
        location<buffer>(memtile2_buffer[0]) = {address((2 + (2 * 0)), 1, 0), address((2 + (2 * 0)), 1, 262144)};
        location<buffer>(memtile2_buffer[1]) = {address((2 + (2 * 1)), 1, 0), address((2 + (2 * 1)), 1, 262144)};
        location<buffer>(memtile2_buffer[2]) = {address((2 + (2 * 2)), 1, 0), address((2 + (2 * 2)), 1, 262144)};
        location<buffer>(memtile2_buffer[3]) = {address((2 + (2 * 3)), 1, 0), address((2 + (2 * 3)), 1, 262144)};

        connect<window<2048 * sizeof(cint16)>>(gm_in[0].out[0], fft_2048[0].in);
        connect<window<2048 * sizeof(cint16)>>(gm_in[1].out[0], fft_2048[1].in);
        connect<window<2048 * sizeof(cint16)>>(gm_in[2].out[0], fft_2048[2].in);
        connect<window<2048 * sizeof(cint16)>>(gm_in[3].out[0], fft_2048[3].in);

        connect<window<2048 * sizeof(cint16)>>(fft_2048[0].out, memtile1_buffer[0].in[0]);
        connect<window<2048 * sizeof(cint16)>>(fft_2048[1].out, memtile1_buffer[1].in[0]);
        connect<window<2048 * sizeof(cint16)>>(fft_2048[2].out, memtile1_buffer[2].in[0]);
        connect<window<2048 * sizeof(cint16)>>(fft_2048[3].out, memtile1_buffer[3].in[0]);

        connect<stream>(memtile1_buffer[0].out[0], output_aiegm[0].in[0]);
        connect<stream>(memtile1_buffer[1].out[0], output_aiegm[1].in[0]);
        connect<stream>(memtile1_buffer[2].out[0], output_aiegm[2].in[0]);
        connect<stream>(memtile1_buffer[3].out[0], output_aiegm[3].in[0]);

        connect<stream>(input_gmaie[0].out[0], memtile2_buffer[0].in[0]);
        connect<stream>(input_gmaie[1].out[0], memtile2_buffer[1].in[0]);
        connect<stream>(input_gmaie[2].out[0], memtile2_buffer[2].in[0]);
        connect<stream>(input_gmaie[3].out[0], memtile2_buffer[3].in[0]);

        connect<window<256 * sizeof(cint16)>>(memtile2_buffer[0].out[0], cint16_cint32[0].in);
        connect<window<256 * sizeof(cint16)>>(memtile2_buffer[1].out[0], cint16_cint32[1].in);
        connect<window<256 * sizeof(cint16)>>(memtile2_buffer[2].out[0], cint16_cint32[2].in);
        connect<window<256 * sizeof(cint16)>>(memtile2_buffer[3].out[0], cint16_cint32[3].in);

        connect<window<256 * sizeof(cint32)>>(cint16_cint32[0].out, fft_64[0].in);
        connect<window<256 * sizeof(cint32)>>(cint16_cint32[1].out, fft_64[1].in);
        connect<window<256 * sizeof(cint32)>>(cint16_cint32[2].out, fft_64[2].in);
        connect<window<256 * sizeof(cint32)>>(cint16_cint32[3].out, fft_64[3].in);

        connect<>(fft_64[0].out, gm_out[0].in[0]);
        connect<>(fft_64[1].out, gm_out[1].in[0]);
        connect<>(fft_64[2].out, gm_out[2].in[0]);
        connect<>(fft_64[3].out, gm_out[3].in[0]);
        /////////////////////////////
        /////////////////////////////
        tiling_parameters memtile1_in = {
            .buffer_dimension = {1024, 48}, .tiling_dimension = {1024, 48}, .offset = {0, 0}};
        tiling_parameters memtile1_out = {
            .buffer_dimension = {1024, 48},
            .tiling_dimension = {1, 48},
            .offset = {0, 0},
            .tile_traversal = {{.dimension = 0, .stride = 1, .wrap = 1024}, {.dimension = 1, .stride = 1, .wrap = 1}}};
        tiling_parameters memtile2_in = {.buffer_dimension = {192, 64, 2},
                                         .tiling_dimension = {48, 1, 2},
                                         .offset = {0, 0, 0},
                                         .tile_traversal = {{.dimension = 0, .stride = 48, .wrap = 4},
                                                            {.dimension = 1, .stride = 1, .wrap = 64},
                                                            {.dimension = 2, .stride = 1, .wrap = 1}}};
        tiling_parameters memtile2_out = {.buffer_dimension = {192, 64, 2},
                                          .tiling_dimension = {1, 64, 1},
                                          .offset = {0, 0, 0},
                                          .tile_traversal = {{.dimension = 0, .stride = 1, .wrap = 192},
                                                             {.dimension = 1, .stride = 1, .wrap = 1},
                                                             {.dimension = 2, .stride = 1, .wrap = 2}}};
        write_access(memtile1_buffer[0].in[0]) = tiling(memtile1_in);
        write_access(memtile1_buffer[1].in[0]) = tiling(memtile1_in);
        write_access(memtile1_buffer[2].in[0]) = tiling(memtile1_in);
        write_access(memtile1_buffer[3].in[0]) = tiling(memtile1_in);
        read_access(memtile1_buffer[0].out[0]) = tiling(memtile1_out);
        read_access(memtile1_buffer[1].out[0]) = tiling(memtile1_out);
        read_access(memtile1_buffer[2].out[0]) = tiling(memtile1_out);
        read_access(memtile1_buffer[3].out[0]) = tiling(memtile1_out);
        write_access(memtile2_buffer[0].in[0]) = tiling(memtile2_in);
        write_access(memtile2_buffer[1].in[0]) = tiling(memtile2_in);
        write_access(memtile2_buffer[2].in[0]) = tiling(memtile2_in);
        write_access(memtile2_buffer[3].in[0]) = tiling(memtile2_in);
        read_access(memtile2_buffer[0].out[0]) = tiling(memtile2_out);
        read_access(memtile2_buffer[1].out[0]) = tiling(memtile2_out);
        read_access(memtile2_buffer[2].out[0]) = tiling(memtile2_out);
        read_access(memtile2_buffer[3].out[0]) = tiling(memtile2_out);
    };
};

#endif // _DSPLIB_TEST_HPP_
