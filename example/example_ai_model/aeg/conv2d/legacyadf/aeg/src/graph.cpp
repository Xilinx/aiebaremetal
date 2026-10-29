// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <cstdint> // for int32_t, if needed
#include <cstdlib> // for rand(), srand()
#include <cstring> // for memset, memcpy
#include <ctime>   // for time()
#include <fstream>
#include <iostream>
#include <vector>

#include "aiebaremetal.h"
#include "aeg_platform.h"

#if defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph EG;
#endif
#include "config.h"
#include <xaiengine.h>

// #define P_COL_NUM_USED 16
// #define CORE_NUM_PER_COL 8

#ifdef _RPU_
// set rpu support
uint8_t rpu = 1;
#else
uint8_t rpu = 0;
#endif

AbrGraph gr("EG");

using namespace std;
// using namespace adf;

// myGraph EG;

const int InSize_Nopading = IFM_W_SIZE * IFM_W_SIZE * IFM_INPUT_C_SIZE;
const int WgtSize = IFM_K_SIZE * IFM_K_SIZE * IFM_INPUT_C_SIZE * 8;
const int OutSize = OFM_W_SIZE * IFM_K_SIZE * 9 * 8; // OFM_W_SIZE*IFM_INPUT_C_SIZE*8*8

void aie_aietile_check(void *inst, int row, int col);

void aie_memtile_s2mm0_bd_finish_count_set(void *inst, int row, int col);
void aie_perfcount_event_count_set(void *inst, int row, int col, int perf_count_register, int event);
void aie_memtile_check(void *inst, int row, int col);
void aie_check(void *inst, int r = 2, int c = 2);
void aie_dma_transfer_time_count_set_mm2s(void *inst, int r = 4, int c = 0, int channel = 0, int direction = 1);
void aie_dma_transfer_time_count_read_mm2s(void *inst, int rst_after_read = 0, int r = 4, int c = 0, int channel = 0,
                                           int direction = 1); // mm2s default
void aie_check_shim_s2mm(void *inst, int r = 0, int c = 2);
///*
template <typename T> struct TileInfo {
    T *data;
    int tileH;
    int tileW;
};

typedef struct _pkt_info {
    uint32_t pkt_id;
    uint32_t row;
    uint32_t col;
} pkt_info;

pkt_info parse_pkt_header(uint32_t pkt_header) {
    pkt_info pinfo;
    pinfo.pkt_id = pkt_header & ((1 << 5) - 1);
    pinfo.row = (pkt_header & ((1 << 21) - 1)) >> 16;
    pinfo.col = (pkt_header & ((1 << 28) - 1)) >> 21;
    return pinfo;
}

// verify the core 0, each core have 8 item IFM_K_SIZE*IFM_K_SIZE*IFM_INPUT_C_SIZE
void verify(int32_t *output, struct TileInfo<int32_t> tinputdata, int32_t *weights_data[CORE_NUM_PER_COL], int h,
            int numbertile, int kernel, int stride, int pad) {
    int32_t *input_data0 = tinputdata.data;
    int hblock_len = (((h + 2 * pad - IFM_K_SIZE) / stride + 1) / numbertile) * stride + 2 * pad;
    printf("tile w %d tile h %d\n", tinputdata.tileW, tinputdata.tileH);
    int verify_len = 8;
    int32_t *wdata_core0 = weights_data[0];
    for (int i = 0; i < 8 /* */; i++) {
        // printf("weights_data[%d] = 0x%p", i, weights_data[i]);
        int32_t odata = output ? output[i] : 1;
        int32_t *wdata = wdata_core0 + (i * IFM_K_SIZE * IFM_K_SIZE * 3);
        int32_t *input_start = input_data0; // just check first sliding windows result
        int var = 0;
        for (int c = 0; c < IFM_INPUT_C_SIZE; c++) {
            int32_t *cdata_start = input_start + c * (tinputdata.tileW * tinputdata.tileH);
            int32_t *w_cdata_start = wdata + c * (IFM_K_SIZE * IFM_K_SIZE);
            for (int h = 0; h < IFM_K_SIZE; h++) {
                int32_t *hdata_start = cdata_start + h * (IFM_W_SIZE + 2 * 3);
                int32_t *w_hdata_start = w_cdata_start + h * (IFM_K_SIZE);
                for (int w = 0; w < IFM_K_SIZE; w++) {
                    int32_t *wdata_start = hdata_start + w;
                    int32_t *w_wdata_start = w_hdata_start + w;
                    var += (*wdata_start) * (*w_wdata_start);
                    // printf("input[c %d, h %d, w %d] is %d  wgt is %d \n", c, h, w, (*wdata_start) ,
                    // (*w_wdata_start));
                }
            }
        }
        printf("------expect %d get %d %s \n", var, odata, output ? "after init" : "before init");
    }
    return;
}
//*/
template <typename T> void fillDataCHW(T *data, int H, int W, int C) {
    for (int c = 0; c < C; c++) {
        for (int h = 0; h < H; h++) {
            for (int w = 0; w < W; w++) {
                int idx = c * (H * W) + h * W + w;
                data[idx] = 1; //(c + 1) * 1000000 + (h + 1) * 1000 + (w + 1);
            }
        }
    }
}

// -----------------------------------------------------------------------------
// tileConv2DHWC<T> (generalized numTiles):
//
//   input: pointer to an (H x W x C) buffer (HWC layout)
//   kernel=k, stride=s, pad=p => typical conv2D parameters
//   numTiles => how many vertical tiles you want (e.g. 16, 8, 2, etc.)
//
//   Steps:
//     1) outH = (H + 2*p - k)/s + 1
//     2) tileOutH = floor(outH / numTiles)  (ignoring leftover coverage)
//     3) tileInH = tileOutH * s
//     4) tileH = tileInH + 2*p
//     5) tileW = W + 2*p
//
//   We produce exactly numTiles tiles. For tile i in [0..numTiles-1]:
//     - it covers input rows [i*tileInH .. i*tileInH + tileInH - 1] plus top/bottom pad
//     - each row is filled in bulk:
//        * if row_in_original out of [0..H-1], memset entire row
//        * else: zero left pad, memcpy W columns, zero right pad
//
//   Returns a vector of new[]-allocated pointers of size (tileH x tileW x C).
//   Caller must free them with delete[] when done.
//
// NOTE: leftover coverage is ignored if (outH % numTiles != 0).
// -----------------------------------------------------------------------------
template <typename T>
std::vector<TileInfo<T>> tileConv2DCHW(AbrGraph *graph, const T *input, int H, int W, int C, int kernel, int stride,
                                       int pad, int numTiles) {
    int outH = (H + 2 * pad - kernel) / stride + 1;
    int tileOutH = outH / numTiles;
    int tileInH = tileOutH * stride;
    int tileH = tileInH + 2 * pad;
    int tileW = W + 2 * pad;
    std::vector<TileInfo<T>> tiles(numTiles);
    for (int tileIdx = 0; tileIdx < numTiles; tileIdx++) {
        // T* block = new T[static_cast<size_t>(C) * tileH * tileW];
        T *block = (T *)graph->malloc(C * tileH * tileW * sizeof(T));
        for (int c = 0; c < C; c++) {
            for (int th = 0; th < tileH; th++) {
                int row_in_original = tileIdx * tileInH + (th - pad);
                int tileRowOffset = c * (tileH * tileW) + th * tileW;
                if (row_in_original < 0 || row_in_original >= H) {
                    std::memset(block + tileRowOffset, 0, tileW * sizeof(T));
                } else {
                    std::memset(block + tileRowOffset, 0, pad * sizeof(T));
                    const T *srcPtr = input + c * (H * W) + row_in_original * W;
                    T *dstPtr = block + tileRowOffset + pad;
                    std::memcpy(dstPtr, srcPtr, W * sizeof(T));
                    std::memset(block + tileRowOffset + (pad + W), 0, pad * sizeof(T));
                }
            }
        }
        tiles[tileIdx].data = block;
        tiles[tileIdx].tileH = tileH;
        tiles[tileIdx].tileW = tileW;
    }
    return tiles;
}

int gdebug = 1;
void gdb() {
    while (gdebug) {
        usleep(10000);
        printf("gdebug\n");
    }
    return;
}

int main(int argc, char **argv) {
// gdb();
#ifdef _RPU_
    // RPU need to disable data cache, without cache diable app will stuck
    Xil_DCacheDisable();
    // Xil_ICacheDisable();
#endif

    auto inst = gr.getaiedevinst();
    aie_check(inst);

    // gr.getaiedevinst();

    int32_t *input_data_no_padding = (int32_t *)malloc(InSize_Nopading * sizeof(int32_t));
    int32_t *weights_data[CORE_NUM_PER_COL];
    int32_t *output_data[P_COL_NUM_USED];

    // int32_t* weights2_data =
    // (int32_t*)gr.malloc(in_channels_2*out_channels_2*filter_size_1*filter_size_2*sizeof(int32_t));

    for (int i = 0; i < P_COL_NUM_USED; i++) {
        output_data[i] = (int32_t *)gr.malloc(OutSize * sizeof(int32_t));
        for (uint32_t j = 0; j < OutSize; j++)
            output_data[i][j] = 99999;
    }

    fillDataCHW<int32_t>(input_data_no_padding, IFM_W_SIZE, IFM_W_SIZE, IFM_INPUT_C_SIZE);
    auto input_data_vec = tileConv2DCHW<int32_t>(&gr, input_data_no_padding, IFM_W_SIZE, IFM_W_SIZE, IFM_INPUT_C_SIZE,
                                                 IFM_K_SIZE, 2, 3, P_COL_NUM_USED);

    auto idata = input_data_vec[0].data;
    /*
    for (int c = 0; c < IFM_INPUT_C_SIZE; c++) {
      for (int h = 0; h < IFM_K_SIZE; h++) {
        for (int w = 0; w < IFM_K_SIZE; w++) {
          printf("inputdata[%d][%d][%d] = %d\n", c,h, w, idata[c*230*20 + h*230 + w]);
        }
      }
    }
    /**/
    for (int k = 0; k < CORE_NUM_PER_COL; k++) {
        weights_data[k] = (int32_t *)gr.malloc(WgtSize * sizeof(int32_t));
        // printf(" alloc weights_data[%d]=0x%p\n", k, weights_data[k]);
        for (int o = 0; o < 8; o++) {
            for (int i = 0; i < IFM_INPUT_C_SIZE; i++) {
                for (int l = 0; l < IFM_K_SIZE; l++) {
                    for (int m = 0; m < IFM_K_SIZE; m++) {
                        weights_data[k][o * IFM_K_SIZE * IFM_K_SIZE * IFM_INPUT_C_SIZE + i * IFM_K_SIZE * IFM_K_SIZE +
                                        l * IFM_K_SIZE + m] =
                            (o + 1); //(k +1)* 10000 + (o + 1)*1000+(i + 1) *100 + (l +1) *10 + m +1;
                                     // weights_data[k][i*IFM_K_SIZE*IFM_K_SIZE + l *IFM_K_SIZE +m] =  (k + 1)*10+m +1;
                    }
                }
            }
#ifdef XAIE_DEBUG
            printf("weight is  ");
            for (int i = 0; i < 16; i++) {
                printf(" %d ", weights_data[k][i]);
            }
            printf("\n");
#endif
        }
    }
    // verify(nullptr, input_data_vec[0], weights_data,IFM_W_SIZE, P_COL_NUM_USED, IFM_K_SIZE, 2,IFM_PAD_SIZE);
    gr.init();
#ifdef XAIE_DEBUG
    aie_dma_transfer_time_count_set_mm2s(inst, 5, 0);
    aie_dma_transfer_time_count_set_mm2s(inst, 4, 0);
    aie_memtile_s2mm0_bd_finish_count_set(inst, 1, 3);
#endif
    for (int j = 0; j < P_COL_NUM_USED; j++) {
        // cout << "Iteration: " << kiter << endl;
        int param = 8;

        // std::string port = "EG.conv[0].in[2]",port2 = "EG.conv[1].in[2]";

        for (int i = 0; i < CORE_NUM_PER_COL; i++) {
            gr.update(("EG.conv[" + std::to_string(j) + "][" + std::to_string(i) + "].in[2]"),
                      reinterpret_cast<char *>(&param), sizeof(int));
        }
        gr.setAddress((char *)("EG.ddrin[" + std::to_string(j) + "]").c_str(), input_data_vec[j].data);
        gr.gm2aie_nb((char *)("EG.ddrin[" + std::to_string(j) + "]").c_str(),
                     (char *)("EG.ddrin[" + std::to_string(j) + "].out[0]").c_str());

        gr.setAddress((char *)("EG.ddrout[" + std::to_string(j) + "]").c_str(), output_data[j]);
        gr.aie2gm_nb((char *)("EG.ddrout[" + std::to_string(j) + "]").c_str(),
                     (char *)("EG.ddrout[" + std::to_string(j) + "].in[0]").c_str());
        // cout << "\tTransactions sent. " << endl;

        // aie_dma_transfer_time_count_set_mm2s(inst, 4, 0);
        // aie_dma_transfer_time_count_set_mm2s(inst, 5, 0);
        // aie_check_shim_s2mm(inst);
    }
    // broadcast weights
    for (int j = 0; j < CORE_NUM_PER_COL; j++) {

        gr.setAddress((char *)("EG.ddr_weights[" + std::to_string(j) + "]").c_str(), weights_data[j]);
        // cout << "\tAddresses set. " << endl;
        gr.gm2aie_nb((char *)("EG.ddr_weights[" + std::to_string(j) + "]").c_str(),
                     (char *)("EG.ddr_weights[" + std::to_string(j) + "].out[0]").c_str());
        // gr.gm2aie_nb("EG.weights_2","EG.weights_2.out[0]");
    }
    XTime tStart, tEnd;
    XTime_GetTime(&tStart);
    gr.run(1); // 112*IFM_K_SIZE);
#ifdef XAIE_DEBUG
    usleep(10000);
    // aie_dma_transfer_time_count_read_mm2s(inst, 0, 4, 0);
    aie_dma_transfer_time_count_read_mm2s(inst, 0, 5, 0);
    aie_check_shim_s2mm(inst);
    char *log = gr.dumpcorelog(3, 0); // row 3 col 13
    // printf("1 core log %s\n", log);
    log = gr.dumpcorelog(4, 0); // row 3 col 13
    // printf("2 core log %s\n", log);
#endif

    for (int j = 0; j < P_COL_NUM_USED; j++) {
        gr.wait((char *)("EG.ddrin[" + std::to_string(j) + "]").c_str(),
                (char *)("EG.ddrin[" + std::to_string(j) + "].out[0]").c_str());
        // cout << "ddr in done" <<endl;
        if (j < CORE_NUM_PER_COL) {
            gr.wait((char *)("EG.ddr_weights[" + std::to_string(j) + "]").c_str(),
                    (char *)("EG.ddr_weights[" + std::to_string(j) + "].out[0]").c_str());
        }

        // cout << "ddr_weights done" <<endl;
        // aie_memtile_check(inst, 1, 3);
        gr.wait((char *)("EG.ddrout[" + std::to_string(j) + "]").c_str(),
                (char *)("EG.ddrout[" + std::to_string(j) + "].in[0]").c_str());
        // cout << "ddr out done in number "  << j << "" <<endl;
        // sleep(100000);
    }

    XTime_GetTime(&tEnd);
    printf("Output took %llu clock cycles.\n", 2 * (tEnd - tStart));
    printf("Output took %.2f us.\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND / 1000000));

    auto odata = &output_data[0][8];
    for (int k = 0; k < OFM_W_SIZE * 7; k++) {
        for (int j = 0; j < 8; j++) {
            for (int i = 0; i < 9; i++) {
                auto var = odata[j * 9 + i];
                // auto prev_var = ((uint32_t)(&output_data[0][0]) -4);
                if (i == 0) {
                    auto pinfo = parse_pkt_header(var);
                    // printf("output[%d]=0x%x pkdid %d row %d col %d ", j*9+i,var, pinfo.pkt_id, pinfo.row, pinfo.col);
                    // aie_aietile_check(inst, pinfo.row, pinfo.col);
                } else {
                    // printf("output[%d] =  %d ", j*9+i, var);
                }
            }
            ////printf("\n");
        }
        break;
    }
// verify the computation result
// verify(output_data[0], input_data_vec[0], weights_data, IFM_W_SIZE, P_COL_NUM_USED, IFM_K_SIZE, 2,IFM_PAD_SIZE);
#ifdef XAIE_DEBUG
    aie_dma_transfer_time_count_read_mm2s(inst, 0, 4, 0);
    aie_dma_transfer_time_count_read_mm2s(inst, 0, 5, 0);
    aie_check_shim_s2mm(inst);
#endif
    // return 0;
    gr.wait();
    // cout << "OFM Interface DMA transfer done, ready for file I/O!!" << endl;
    // cout << " output data"<< endl;
    /// for(int k = 0;k < OutSize;k++)
    //{

    // cout<<output_data[k]<<" ";
    // cout << endl;
    //}
    gr.end();
    cout << "Test Passed\n";

    for (int i = 0; i < P_COL_NUM_USED; i++) {
        gr.free(input_data_vec[i].data);
        gr.free(output_data[i]);
    }
    for (int i = 0; i < CORE_NUM_PER_COL; i++)
        gr.free(weights_data[i]);

    return 0;
}

void aie_check(void *inst, int r, int c) {
#if not defined(__ADF_FRONTEND__) && defined(XAIE_DEBUG)
    uint32_t c_r_offset = (c << 25) + (r << 20);
    uint32_t start_queue_oft = 8;
    uint32_t s2m_0_bd_start_queue_memtile = 0x000A0604 + c_r_offset;
    // uint32_t s2m_1_bd_start_queue = 0x000A060C+ c_r_offset;
    uint32_t m2s_0_bd_start_queue_memtile = 0x000A0634 + c_r_offset;
    // uint32_t m2s_1_bd_start_queue = 0x000A063C+ c_r_offset;

    uint32_t DMA_S2MM_Status_0 = 0x0001DF00;

    XAie_DevInst *devinst = (XAie_DevInst *)inst;
    uint32_t dcm, dcs;
    for (int i = 0; i < 6; i++) {
        XAie_Read32(devinst, s2m_0_bd_start_queue_memtile + i * start_queue_oft, &dcs);
        XAie_Read32(devinst, m2s_0_bd_start_queue_memtile + i * start_queue_oft, &dcm);

        printf("%d S2M channel start bd is %d\n", i, dcs & ((1 << 6) - 1));
        printf("%d M2S channel start bd is %d\n ", i, dcm & ((1 << 6) - 1));
    }

    XAie_LocType Loc = XAie_TileLoc(2, 0);
    u8 ChNum = 0;
    XAie_DmaDirection Dir = DMA_S2MM;
    AieRC rc = XAie_DmaWaitForDone(devinst, Loc, ChNum, Dir, 1000);
    printf("rc = %d\n", rc);
#endif
}

void aie_dma_transfer_time_count_set_mm2s(void *inst, int r, int c, int channel, int direction) {
#if not defined(__ADF_FRONTEND__)
    XAie_DevInst *devinst = (XAie_DevInst *)inst;
    aie_dma_transfer_time_count_read_mm2s(inst, 1, r, c); // reset the counter

    uint32_t c_r_offset = (c << 25) + (r << 20);
    uint32_t Performance_Control0 = 0x00011000;
    uint32_t start_event = 25; // DMA_MM2S_0_finished_BD
    uint32_t start_event_value = ((1 << 7) - 1) & start_event;

    uint32_t stop_event = 25 /*same event with the start event use such method to measure how many bd done*/
        ;                    // 29;//DMA_MM2S_0_finished_task
    uint32_t stop_event_value = (((1 << 7) - 1) & stop_event) << 8;

    uint32_t per_count_control_value = (stop_event_value | start_event_value);

    uint32_t per_count_control_addr = c_r_offset + Performance_Control0;

    XAie_Write32(devinst, per_count_control_addr, per_count_control_value);

    uint32_t value;
    XAie_Read32(devinst, per_count_control_addr, &value);
    printf("row %d col %d mm2s channel 0 perf count event setis %d\n", r, c, value);
#endif
}

void aie_dma_transfer_time_count_read_mm2s(void *inst, int rst_after_read, int r, int c, int channel, int direction) {
#if not defined(__ADF_FRONTEND__)
    XAie_DevInst *devinst = (XAie_DevInst *)inst;
    uint32_t c_r_offset = (c << 25) + (r << 20);
    uint32_t Performance_Counter0 = 0x00011020;
    uint32_t DMA_MM2S_Status_0 = 0x0001DF10;
    uint32_t Event_Status0 = 0x00014200;
    uint32_t per_counter_addr = c_r_offset + Performance_Counter0;

    uint32_t event_status_addr = c_r_offset + Event_Status0;

    uint32_t dma_mm2s_status_addr = c_r_offset + DMA_MM2S_Status_0;

    uint32_t value;
    XAie_Read32(devinst, per_counter_addr, &value);
    printf("row %d col %d mm2s channel 0 bd finishnumber is %d\n", r, c, value);
    if (rst_after_read) {
        XAie_Write32(devinst, per_counter_addr, 0);
        XAie_Read32(devinst, per_counter_addr, &value);
        printf("after reset bd finishnumber is %d\n", value);
    }

    for (int i = 0; i < 4; i++) {
        XAie_Read32(devinst, event_status_addr + i * 4, &value);
        printf("event status(%d) value is %d\n", i, value);
    }
    XAie_Read32(devinst, dma_mm2s_status_addr, &value);
    printf("mm2s status is 0x%x\n", value);

#endif
}

void aie_check_shim_s2mm(void *inst, int r, int c) {
#if not defined(__ADF_FRONTEND__)
    uint32_t c_r_offset = (c << 25) + (r << 20);
    uint32_t DMA_S2MM_Status_0 = 0x0001D220 + c_r_offset;
    uint32_t DMA_S2MM_Current_Write_Count_0 = 0x0001D230 + c_r_offset;
    XAie_DevInst *devinst = (XAie_DevInst *)inst;
    uint32_t value1, value2, v3, v4;
    XAie_Read32(devinst, DMA_S2MM_Status_0, &value1);
    XAie_Read32(devinst, DMA_S2MM_Current_Write_Count_0, &value2);

    int32_t BD1_0 = 0x1d020 + c_r_offset, DMA_S2MM_0_Ctrl = 0x0001D200 + c_r_offset;
    XAie_Read32(devinst, BD1_0, &v3);
    XAie_Read32(devinst, DMA_S2MM_0_Ctrl, &v4);
    printf(
        " S2MM row %d col %d status addr 0x%x status is 0x%x writ count = %d BD1_0 0x%x is %d DMA_S2MM_0_Ctrl = 0x%x\n",
        r, c, DMA_S2MM_Status_0, value1, value2, BD1_0, v3, v4);

    uint32_t err_v[4];
    uint32_t event0 = 0x00034200 + c_r_offset;
    for (int i = 0; i < 4; i++) {
        XAie_Read32(devinst, event0 + i * 4, &err_v[i]);
        printf("err_v[%d] = 0x%x\n", i, err_v[i]);
    }
    uint32_t bd0_0 = 0x0001D000 + c_r_offset;
    uint32_t v1;
    XAie_Read32(devinst, bd0_0, &v1);
    printf("bd0_0 = 0x%x\n", v1);
#endif
}
void aie_perfcount_event_count_set(void *inst, int row, int col, int perf_count_register, int event) {
#if not defined(__ADF_FRONTEND__)
    XAie_DevInst *devinst = (XAie_DevInst *)inst;

    uint32_t c_r_offset = (col << 25) + (row << 20);
    // uint32_t Performance_Control0 = 0x00091000;
    uint32_t start_event = event; // 25;//DMA_MM2S_0_finished_BD
    uint32_t start_event_value = ((1 << 7) - 1) & start_event;

    uint32_t stop_event = event; // 25/*same event with the start event use such method to measure how many bd
                                 // done*/;//29;//DMA_MM2S_0_finished_task
    uint32_t stop_event_value = (((1 << 7) - 1) & stop_event) << 8;

    uint32_t per_count_control_value = (stop_event_value | start_event_value);

    uint32_t per_count_control_addr = c_r_offset + perf_count_register;

    XAie_Write32(devinst, per_count_control_addr, per_count_control_value);

    uint32_t value;
    XAie_Read32(devinst, per_count_control_addr, &value);
    printf("row %d col %d mm2s channel 0 perf count event setis %d\n", row, col, value);
#endif
}

void aie_memtile_s2mm0_bd_finish_count_set(void *inst, int row, int col) {
    uint32_t Performance_Control0 = 0x00091000;
    uint32_t start_event = 25; // DMA_MM2S_0_finished_BD
    aie_perfcount_event_count_set(inst, row, col, Performance_Control0, start_event);
}
// 0x73d80
void aie_aietile_check(void *inst, int row, int col) {
#if not defined(__ADF_FRONTEND__) and defined(XAIE_DEBUG)
    XAie_DevInst *devinst = (XAie_DevInst *)inst;
    XAie_LocType loc = XAie_TileLoc(col, row);
    uint32_t buffer[WgtSize];
    AieRC RC = XAie_DataMemBlockRead((XAie_DevInst *)devinst, loc, 0x3d80, (void *)buffer, WgtSize * sizeof(uint32_t));
    for (int i = 0; i < 1; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < IFM_K_SIZE; k++) {
                for (int l = 0; l < IFM_K_SIZE; l++) {
                    printf(" %d ",
                           buffer[i * 3 * IFM_K_SIZE * IFM_K_SIZE + j * IFM_K_SIZE * IFM_K_SIZE + k * IFM_K_SIZE + l]);
                }
                printf("\n");
            }
            printf("\n\n");
        }
        printf("\n\n---aie tile----group %d end---------------\n\n", i);
    }
#endif
}
void aie_memtile_check(void *inst, int row, int col) {
#if not defined(__ADF_FRONTEND__) and defined(XAIE_DEBUG)
    XAie_DevInst *devinst = (XAie_DevInst *)inst;
    XAie_LocType loc = XAie_TileLoc(col, row);
    uint32_t buffer[WgtSize];
    AieRC RC = XAie_DataMemBlockRead((XAie_DevInst *)devinst, loc, 0x6da0, (void *)buffer, WgtSize * sizeof(uint32_t));
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < IFM_K_SIZE; k++) {
                for (int l = 0; l < IFM_K_SIZE; l++) {
                    // printf(" %d ", buffer[i * 3 * IFM_K_SIZE* IFM_K_SIZE + j * IFM_K_SIZE* IFM_K_SIZE + k *
                    // IFM_K_SIZE + l]);
                }
                // printf("\n");
            }
            // printf("\n\n");
        }
        // printf("\n\n-------group %d end---------------\n\n", i);
    }
#endif
}