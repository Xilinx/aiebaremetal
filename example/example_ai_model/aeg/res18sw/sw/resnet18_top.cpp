// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <fstream>

// xil_*.h only declare Xil_DCacheInvalidateRange / XTime_GetTime /
// COUNTS_PER_SECOND; on Zephyr the definitions are provided below.
#include "xil_printf.h"
#include "xil_io.h"
#include "xil_cache.h"
#include "xtime_l.h"
#if defined(__ZEPHYR__)
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/device_mmio.h>
#endif

#include "transactionsubmit.h"
#include "./mladfpatch/hex_data.h"
#include "synset.h"
#include <vector>
#include "aiebaremetal.h"

#include "./mladfpatch/baremetal/ml_txn/extern_mapping.h"
#include "stdio.h"
#if !defined(__ZEPHYR__)
#include "netprocess.h"  // lwIP TCP server; Zephyr net path is a separate TODO
#endif

// Unified timestamp rate (A72 generic system counter, ~100 MHz) pinned for both
// runtimes so the Zephyr-vs-baremetal cycles→µs math is apples-to-apples. The
// benchmark also reports raw cycle counts, which are rate-independent.
#define RESNET_TS_HZ      100000000ULL
#define RESNET_TS_PER_US  (RESNET_TS_HZ / 1000000ULL)

// Rounds per 'b' run. 100 keeps the interactive benchmark quick.
#ifndef BENCH_LOOPS
#define BENCH_LOOPS 100
#endif

// With -DBENCH_SECONDS=N the run is bounded by wall-clock instead of round
// count, which is what the power measurements want: a fixed-length plateau
// regardless of how fast a round turns out to be.
#ifndef BENCH_SECONDS
#define BENCH_SECONDS 0
#endif

#if BENCH_SECONDS > 0
#define BENCH_MENU_TEXT "%d s of silent local rounds, then print throughput"
#define BENCH_MENU_VAL  ((int)BENCH_SECONDS)
#else
#define BENCH_MENU_TEXT "%d silent local rounds, then print throughput"
#define BENCH_MENU_VAL  (BENCH_LOOPS)
#endif

// Per-phase timing accumulators (raw counter ticks), summed across process_data
// calls so the benchmark can attribute each round to gr.run() vs the serial A72
// runtime-control transaction. Reset at the start of each `b` run.
static uint64_t g_run_ticks    = 0;
static uint64_t g_ctrl_ticks   = 0;
static uint64_t g_phase_rounds = 0;

// Local sqrt (Newton-Raphson) to compute the per-round std dev without pulling
// in libm: the baremetal A72 link line has no -lm, so <cmath> sqrt won't link.
static double bench_sqrt(double x) {
    if (x <= 0.0) return 0.0;
    double g = x;
    for (int i = 0; i < 40; i++) g = 0.5 * (g + x / g);
    return g;
}

#if defined(__ZEPHYR__)
// Zephyr has no baremetal BSP; back these with kernel equivalents so the shared
// host code builds unchanged.
extern "C" void Xil_DCacheInvalidateRange(INTPTR adr, INTPTR len) {
    sys_cache_data_invd_range((void *)(uintptr_t)adr, (size_t)len);
}
extern "C" void XTime_GetTime(XTime *t) { *t = (XTime)k_cycle_get_64(); }
#undef COUNTS_PER_SECOND
// Booting at EL3 leaves CNTFRQ_EL0 = 0, so use the fixed A72 timestamp rate
// (~100 MHz) rather than sys_clock_hw_cycles_per_sec().
#define COUNTS_PER_SECOND ((u64)100000000ULL)

// Console RX byte source: interrupt-fed queue in uart_rx_fixup.c (which also does
// the one-time SBSA RX bring-up). Returns 0..255, or -1 on timeout. Works because
// of the GICv3 ARE_S fix that lets the UART SPI reach the A72 at EL3.
extern "C" int abr_console_rx_getc(int timeout_ms);

// Read a whitespace/Enter-delimited token, like baremetal's `std::cin >> token`
// (type e.g. `l<Enter>`). Empty => timed out before any input.
static std::string zephyr_console_getline(int timeout_ms) {
    std::string tok;
    for (;;) {
        int c = abr_console_rx_getc(tok.empty() ? timeout_ms : -1);
        if (c < 0) {
            return tok;                             // timeout (only when empty)
        }
        if (c == '\r' || c == '\n' || c == ' ' || c == '\t') {
            if (!tok.empty()) {
                return tok;                         // separator ends the token
            }
            continue;                               // skip leading separators
        }
        tok.push_back((char)c);
    }
}
#endif

//infinite run
#define _INFINITE_RUN_

#ifndef NUM_INST
    #define NUM_INST 10  // batch1_inst10 default
#endif

#define IFM_FILE  "data/ifm32.txt"
#define WTS_FILE  "data/wts32.txt"
#define OFM_FILE  "data/ofm32.txt"
BaremetalGraph gr("compute_graph");

#define NUM_LAYERS 23
std::vector<uint32_t> wts_ddr_offset = {0, 3776, 13760, 23744, 33728, 43712, 64704, 67392, 106816, 146240, 185664, 264512, 275008, 432704, 590400, 748096, 1063488, 1100864, 1416256, 1731648, 2362432, 2677824, 2993216};

uint32_t* ifm_buf[NUM_INST];
uint32_t* wts_buf;
uint32_t* ofm_buf[NUM_INST];

// Set true to silence per-inference UART output during the 100-loop benchmark
// (option 'b'). Network and single-shot 'l' modes leave it false.
bool process_data_silent = false;

//
void ExternSetAddr(BaremetalGraph& gr, char* buffer_name,const uint32_t* addr) {
    //gr.setAddress(buffer_name,addr);
    std::string name(buffer_name);
    external_map[nameIdMapping.getid(buffer_name)] = (u64)addr;
}
#define IFM_DDR_MEM_SIZE 200704
#define WTS_DDR_MEM_SIZE 12632320
#define OFM_DDR_MEM_SIZE 2048

const int ifm_size = IFM_DDR_MEM_SIZE; //8*64*8*52;
const int wts_size = WTS_DDR_MEM_SIZE; //78848;
const int ofm_size = OFM_DDR_MEM_SIZE; //8*32*16*28;

int argmax(uint16_t array[], int size) {
	uint16_t vmax = array[0], idx = 0;
	for (int i = 1; i < size; i++) {
		if(array[i]> vmax) {
			idx = i;
			vmax=array[idx];
		}
	}
	return idx;

}
int16_t process_data(const uint32_t* ifm_data, int ifm_data_len) {
    if(ifm_data == NULL) {
        printf("ifm_data is null\n");
        return -1;
    }
    if (ifm_size != ifm_data_len) {
        printf("ifm_size  %d != ifm_data_len %d\n", ifm_size, ifm_data_len);
    }
    // (bring-up `std::cout` markers removed — were earlier kept as cheap
    //  sanity checks but they cost ~1 ms each via std::endl/UART flush,
    //  which pollutes the `b` benchmark wall-clock.)
#if not ((defined(__AIESIM__) || defined(__ADF_FRONTEND__)))
	XTime tStart, tEnd;
 	XTime_GetTime(&tStart);
#endif
    //for (int i=0; i<ifm_data_len/4; i++) ifm_buf[i]= ifm_data[i];
    //Set DDR base addresses 
    //gr.setAddress("compute_graph.ifm_ddr",ifm_buf);
    //gr.setAddress((char*)("compute_graph.resnet_layers["+std::to_string(i)+"].wts_ddr").c_str(),wts_buf+wts_ddr_offset[i]);
    //gr.setAddress("compute_graph.ofm_ddr",ofm_buf);

    //std::cout << "2" <<std::endl;
    //ExternSetAddr(gr, "compute_graph.ifm_ddr",ifm_buf);

    //set up the input and output buffer, the wts only set once in global
#ifdef BATCH_GRAPH
#if defined(__ZEPHYR__)
    // ifm_data may be the const ifm32[] in read-only rodata; the AIE cache sync
    // DC-invalidates the input (a write-permission cache op) and faults on a
    // read-only page. Copy once into writable ifm_buf[0] and feed it to every
    // instance (all classify the same image).
    for (int i = 0; i < ifm_data_len / 4; i++) ifm_buf[0][i] = ifm_data[i];
    const uint32_t* ifm_in = ifm_buf[0];
#else
    const uint32_t* ifm_in = ifm_data;
#endif
    for (int inst = 0; inst < NUM_INST; inst++) {
        ExternSetAddr(gr, (char*)("compute_graph.resnet18_" + std::to_string(inst) + ".ifm_ddr").c_str(), ifm_in);
        ExternSetAddr(gr, (char*)("compute_graph.resnet18_" + std::to_string(inst) + ".ofm_ddr").c_str(), ofm_buf[inst]);
        Xil_DCacheInvalidateRange((intptr_t)ofm_buf[inst], (intptr_t)ofm_size);
    }
    
#else
#if defined(__ZEPHYR__)
    // ifm_data may point at the const ifm32[] in read-only rodata. The AIE cache
    // sync DC-invalidates the input buffer (a write-permission cache op), which
    // faults on a read-only page under Zephyr. Copy into the writable, cached
    // ifm_buf and hand that to the AIE instead.
    for (int i = 0; i < ifm_data_len / 4; i++) ifm_buf[0][i] = ifm_data[i];
    ExternSetAddr(gr, "compute_graph.ifm_ddr", ifm_buf[0]);
#else
    ExternSetAddr(gr, "compute_graph.ifm_ddr", ifm_data);
#endif
    ExternSetAddr(gr, "compute_graph.ofm_ddr", ofm_buf[0]);
    Xil_DCacheInvalidateRange((intptr_t)ofm_buf[0], (intptr_t)ofm_size);
#endif
    //compute_graph.ifm_ddr.setAddress(ifm_buf);
    //for(int i=0; i<NUM_LAYERS; i++)
    //   compute_graph.resnet_layers[i].wts_ddr.setAddress(wts_buf+wts_ddr_offset[i]);
    //compute_graph.ofm_ddr.setAddress(ofm_buf);

    //std::cout << "3" <<std::endl;
    //run should come with every exe, or do run() without parameter which is infinite run
    //if not the exe will stuck
#if not ((defined(__AIESIM__) || defined(__ADF_FRONTEND__)))
    XTime pr0, pr1, pr2;
    XTime_GetTime(&pr0);
#endif
#ifndef _INFINITE_RUN_
    gr.run(1);
#endif
#if not ((defined(__AIESIM__) || defined(__ADF_FRONTEND__)))
    XTime_GetTime(&pr1);
#endif
    executeRuntimeControl_TXN();
#if not ((defined(__AIESIM__) || defined(__ADF_FRONTEND__)))
    XTime_GetTime(&pr2);
    g_run_ticks    += (uint64_t)(pr1 - pr0);
    g_ctrl_ticks   += (uint64_t)(pr2 - pr1);
    g_phase_rounds += 1;
#endif

    int size = 1000;
    int idx = argmax((uint16_t*)ofm_buf[0], size);
   //printf("compare done count = %d item is different\n", count);

#if not ((defined(__AIESIM__) || defined(__ADF_FRONTEND__)))
	XTime_GetTime(&tEnd);
	if (!process_data_silent) {
		printf("Output took %llu clock cycles.\n", 2*(tEnd - tStart));
		printf("Output 1 took %.2f us.\n",
		       1.0 * (tEnd - tStart) / (double)RESNET_TS_PER_US);
	}
#endif
    if (!process_data_silent) {
#ifdef BATCH_GRAPH
        for (int inst = 0; inst < NUM_INST; inst++) {
            int inst_idx = argmax((uint16_t*)ofm_buf[inst], size);
            printf("inst %d: type is idx %d type name is %s\n",
                   inst, inst_idx, synset_names[inst_idx]);
        }
#else
        printf("type is idx %d type name is %s\n", idx, synset_names[idx]);
#endif
    }
    return idx;
}
std::string get_run_arg(){
    std::string input="";
    printf("Specify any of the below option:\n");
    printf("\tl - run local classification\n");
    printf("\tb - benchmark: " BENCH_MENU_TEXT "\n", BENCH_MENU_VAL);
    printf("\tn - run network server and do remote classification\n");
    printf("\tq - to quit\n");
    while(1){
        std::cin >> input;
        if(input != "l" && input != "b" && input != "n" && input != "q"){
            std::cout << "type is wong" << std::endl;
        } else{
            break;
        }
    }
    return input;
}
int main()
{
    //Xil_DCacheDisable();
    // Force-init the silent flag (don't rely on .bss zeroing for the new global).
    process_data_silent = false;
    // Bail out clearly if init fails (e.g. a stale/wrong graph config that lacks
    // 'compute_graph'): otherwise the malloc()s below return NULL and the first
    // buffer write data-aborts with an opaque FAR=0x0 crash.
    if (gr.init() != aeg_err::ok) {
        printf("[FATAL] gr.init() failed: 'compute_graph' not configured. "
               "Rebuild with a fresh resnet18 graph build (see build_zephyr.sh).\n");
        return -1;
    }
    for (int inst = 0; inst < NUM_INST; inst++) {
        ifm_buf[inst] = (uint32_t*)gr.malloc(ifm_size*sizeof(uint8_t) + 1024);
        ofm_buf[inst] = (uint32_t*)gr.malloc(ofm_size*sizeof(uint8_t));
    }
    wts_buf = (uint32_t*)gr.malloc(wts_size*sizeof(uint8_t));
    if (wts_buf == nullptr) {
        printf("[FATAL] gr.malloc(wts) returned NULL; aborting before buffer init.\n");
        return -1;
    }
    printf("05:13 2026\n");
    //uint32_t* ifm_buf2 = (uint32_t*)GMIO::malloc(ifm_size*sizeof(uint8_t));    
    //uint32_t* wts_buf2 = (uint32_t*)GMIO::malloc(wts_size*sizeof(uint8_t));

	

    //gr.init();
#ifdef _INFINITE_RUN_
    gr.run(-1);//infinite
#endif
    //wts is for alg, only set one time
    for (int i=0; i<wts_size/4; i++) wts_buf[i] = wts32[i];
    for(int i=0; i<NUM_LAYERS; i++) {
#ifdef BATCH_GRAPH
        ExternSetAddr(gr, (char*)("compute_graph.wts_ddr["+std::to_string(i)+"]").c_str(), wts_buf+wts_ddr_offset[i]);
#else
        ExternSetAddr(gr, (char*)("compute_graph.resnet_layers["+std::to_string(i)+"].wts_ddr").c_str(), wts_buf+wts_ddr_offset[i]);
#endif
    }

//#if not ((defined(__AIESIM__) || defined(__ADF_FRONTEND__)))
//	XTime tStart, tEnd;
// 	XTime_GetTime(&tStart);
//#endif

    while(1) {
#if defined(__ZEPHYR__)
        // Menu over the console UART (std::cin doesn't work; see
        // zephyr_console_getline). First prompt is time-bounded so a headless
        // board auto-runs one classification; after a keypress, loop until 'q'.
        static bool zephyr_interactive = false;
        const int first_wait_ms = 10000;
        printf("\nSpecify run option:\n");
        printf("\tl - run local classification\n");
        printf("\tb - benchmark: " BENCH_MENU_TEXT "\n", BENCH_MENU_VAL);
        printf("\tq - to quit\n");
        if (!zephyr_interactive) {
            printf("(auto-run 'l' in %d s if no key pressed)\n",
                   first_wait_ms / 1000);
        }
        // Read a whitespace/Enter-terminated token (baremetal-style: type the
        // letter then Enter). Empty => timed out with no input -> headless auto-run.
        std::string rtype =
            zephyr_console_getline(zephyr_interactive ? -1 : first_wait_ms);
        if (rtype.empty()) {
            printf("> (no input) running one local classification\n");
            rtype = "l";
        } else {
            zephyr_interactive = true;
            printf("> %s\n", rtype.c_str());
        }
#else
        auto rtype = get_run_arg();
#endif
        int itype = 0;
        if(rtype == "l") {
            //after process_data user can select whether run other type test
            itype = process_data(ifm32,ifm_size );
        } else if (rtype == "b") {
            // 100-round benchmark with per-round UART silenced (timing
            // window measures AIE work + A72 runtime control only — no
            // serial-port stalls). Multi-call is supported (see §5 menu
            // callout in README), so we can average over 100 rounds and
            // get a stable per-image number.
            // Run length is build-time settable (-DBENCH_LOOPS rounds, or
            // -DBENCH_SECONDS wall-clock): the power measurements need minutes
            // of steady load, since the board's INA226 rails are read through
            // the system controller at ~1 Hz.
            const int LOOPS = (BENCH_SECONDS > 0) ? INT32_MAX : BENCH_LOOPS;
#ifdef BATCH_GRAPH
            const int images_per_round = NUM_INST;
#else
            const int images_per_round = 1;
#endif
            if (BENCH_SECONDS > 0) {
                printf("Benchmark: %d s of rounds x %d image(s)/round\n",
                       (int)BENCH_SECONDS, images_per_round);
            } else {
                printf("Benchmark: %d rounds x %d image(s)/round = %d images\n",
                       LOOPS, images_per_round, LOOPS * images_per_round);
            }
            process_data_silent = true;
            txn_silent = true;
            // Per-round stats are accumulated in one pass (Welford) rather than
            // kept in an array, so a long -DBENCH_LOOPS run costs no memory.
            uint64_t min_ticks = ~0ULL, max_ticks = 0;
            double mean_ticks = 0.0, m2_ticks = 0.0;
            g_run_ticks = g_ctrl_ticks = g_phase_rounds = 0;
            XTime bStart, bEnd;
            XTime_GetTime(&bStart);
            int rounds = 0;
            for (int loop = 0; loop < LOOPS; loop++) {
                XTime r0, r1;
                XTime_GetTime(&r0);
                itype = process_data(ifm32, ifm_size);
                XTime_GetTime(&r1);
                uint64_t ticks = (uint64_t)(r1 - r0);
                if (ticks < min_ticks) min_ticks = ticks;
                if (ticks > max_ticks) max_ticks = ticks;
                rounds++;
                double delta = (double)ticks - mean_ticks;
                mean_ticks += delta / (double)rounds;
                m2_ticks   += delta * ((double)ticks - mean_ticks);
                if (BENCH_SECONDS > 0 &&
                    (double)(uint64_t)(r1 - bStart) >= (double)BENCH_SECONDS * RESNET_TS_HZ) {
                    break;
                }
            }
            XTime_GetTime(&bEnd);
            const int total_images = rounds * images_per_round;
            process_data_silent = false;
            txn_silent = false;

            // Single explicit timestamp rate for BOTH runtimes (apples-to-apples);
            // raw cycle counts are also printed and are rate-independent.
            const double ticks_per_us = (double)RESNET_TS_PER_US;
            uint64_t total_ticks   = (uint64_t)(bEnd - bStart);
            double us              = total_ticks / ticks_per_us;
            double us_per_image    = us / total_images;
            double images_per_sec  = 1.0e6 * total_images / us;
            // ResNet-18 ~3.64 GOPS / image (Conv+FC, MAC counted as 2 ops).
            double gops_per_sec    = images_per_sec * 3.64;

            double std_ticks = bench_sqrt(m2_ticks / rounds);
            double mean_us = mean_ticks / ticks_per_us;
            double std_us  = std_ticks  / ticks_per_us;
            double min_us  = (double)min_ticks / ticks_per_us;
            double max_us  = (double)max_ticks / ticks_per_us;

            // Sub-phase split: gr.run() vs the serial A72 runtime-control txn.
            // Percentages are of the round wall-clock, not of the two phases, so
            // they don't read as 100% while a chunk of the round is elsewhere.
            // The remainder is the per-round work outside both instrumented
            // regions (Zephyr's input-buffer copy dominates it; see README §5.5).
            double run_us_pr  = g_phase_rounds ? (double)g_run_ticks  / g_phase_rounds / ticks_per_us : 0.0;
            double ctrl_us_pr = g_phase_rounds ? (double)g_ctrl_ticks / g_phase_rounds / ticks_per_us : 0.0;
            double other_us_pr = mean_us - run_us_pr - ctrl_us_pr;

            printf("=== benchmark summary ===\n");
            printf("  rounds x images   : %d x %d = %d\n", rounds, images_per_round, total_images);
            printf("  timestamp rate    : %llu Hz (unified; raw cycles are rate-independent)\n",
                   (unsigned long long)RESNET_TS_HZ);
            printf("  wall-clock total  : %.0f us  (%llu cycles)\n",
                   us, (unsigned long long)total_ticks);
            printf("  per round  mean   : %.1f us  (%.0f cyc)\n", mean_us, mean_ticks);
            printf("  per round  std    : %.1f us  (%.2f%% of mean)\n",
                   std_us, mean_us > 0 ? 100.0 * std_us / mean_us : 0.0);
            printf("  per round  min/max: %.1f / %.1f us\n", min_us, max_us);
            printf("  per image (avg)   : %.1f us\n", us_per_image);
            printf("  throughput        : %.1f img/s\n", images_per_sec);
            printf("  compute (INT8)    : %.2f GOPS\n", gops_per_sec);
            printf("  phase gr.run()    : %.1f us/round (%.1f%% of round)\n",
                   run_us_pr,  mean_us > 0 ? 100.0 * run_us_pr   / mean_us : 0.0);
            printf("  phase ctrl_TXN()  : %.1f us/round (%.1f%% of round)\n",
                   ctrl_us_pr, mean_us > 0 ? 100.0 * ctrl_us_pr  / mean_us : 0.0);
            printf("  phase other       : %.1f us/round (%.1f%% of round)\n",
                   other_us_pr, mean_us > 0 ? 100.0 * other_us_pr / mean_us : 0.0);
            printf("=========================\n");
            // Verification: show top-1 from instance 0 of the last round.
            int vidx = argmax((uint16_t*)ofm_buf[0], 1000);
            printf("verification (inst 0): idx %d (%s)\n",
                   vidx, synset_names[vidx]);
        } else if (rtype == "n") {
#if !defined(__ZEPHYR__)
            net_process_main((void *)process_data);
            break;
#else
            printf("network mode not yet supported on Zephyr\n");
#endif
        } else if (rtype == "q") {
            break;
        }
#if defined(__ZEPHYR__)
        // Headless fallback: if nobody ever pressed a key, exit after the one
        // auto-run. Interactive sessions loop back to the menu until 'q'.
        if (!zephyr_interactive) {
            break;
        }
#endif
    }

//#if not ((defined(__AIESIM__) || defined(__ADF_FRONTEND__)))
//	XTime_GetTime(&tEnd);
//	printf("Output took %llu clock cycles.\n", 2*(tEnd - tStart));
//  	printf("Output 3 took %.2f us.\n",
//           1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND/1000000));
//#endif
    //std::cout << "2 OFM Shim DMA transfer done, ready for file I/O!! itype is "<< itype << std::endl;
	    
    gr.end();
    
    //std::ofstream ofm_ofs;
    //ofm_ofs.open(OFM_FILE, std::ofstream::out | std::ofstream::trunc);
    //std::cout << "Opened file " << OFM_FILE << " for writing OFM!!" << std::endl;
    /*
    int count = 0;
    for (int i=0; i<ofm_size/4; i++) {
       //ofm_ofs<<std::hex<<(uint32_t)ofm_buf[i]<<std::endl;
       if (ofm32_ref[i] != ofm_buf[i]) {
            printf("ref 0x%x buf 0x%x\n", ofm32_ref[i] ,ofm_buf[i]);
            count++;
       }
    }
    */
    //int size = 1000;
    //int idx = argmax((uint16_t*)ofm_buf, size);
    //printf("compare done count = %d item is different\n", count);
    //printf("type is %d max from %d item\n",idx, ofm_size/2);
    
    //ofm_ofs.close();

    for (int inst = 0; inst < NUM_INST; inst++) {
        gr.free(ifm_buf[inst]);
        gr.free(ofm_buf[inst]);
    }
    gr.free(wts_buf);
    std::cout << " graph execution done !" <<std::endl;
    return 0;
}
