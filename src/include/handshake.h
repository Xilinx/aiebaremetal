// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.

#ifndef _HANDSHAKE_H_
#define _HANDSHAKE_H_

#if defined(__KERNEL__)
#include <linux/types.h>
#elif defined(__cplusplus)
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "host_queue.h"

// Common states
#define FW_STATE_HANDSHAKE 0x0000
#define FW_STATE_INIT_BARRIER 0x0001
#define FW_STATE_CONFIGURE_PARTITION 0x0002
#define FW_STATE_HSA_CONFIG 0x0003
#define FW_STATE_EXEC_PAGEIN 0x0004
#define FW_STATE_EXEC_INITIAL_PASS 0x0005
#define FW_STATE_EXEC_EVENTLOOP 0x0006
#define FW_STATE_OOO_OPCODE 0x0007
#define FW_STATE_OOO_PREFETCH 0x0008
#define FW_STATE_EMPTY_PAGE 0x0009
#define FW_STATE_EXIT 0x0FFF

// Leader states
#define FW_STATE_LEADER_HOST_QUEUE_POP 0x1001
#define FW_STATE_LEADER_DISTRIBUTE_WORK 0x1002
#define FW_STATE_LEADER_POST_DIST_BARRIER 0x1003
#define FW_STATE_LEADER_POST_WORK_BARRIER 0x1004
#define FW_STATE_LEADER_RUN 0x1005
#define FW_STATE_LEADER_HOST_QUEUE_FINISH 0x1006

// Worker states
#define FW_STATE_WORKER_PRE_WORK_BARRIER 0x2001
#define FW_STATE_WORKER_POST_WORK_BARRIER 0x2002
#define FW_STATE_WORKER_RUN 0x2003

// #define PDI_LOAD_TEST
/*
 * All data locate at pDM offset 0
 * read/write of all fields are always in 32bits
 */
enum hsa_location_t {
    EXTERNAL_AXI_MM = 0, // by default, hsa queue resides in host ddr
    INTERNAL_AXI_MM = 1, // for bringup test purpose
};

typedef struct {
    volatile uint32_t mpaie_alive;   // 0
    uint32_t partition_base_address; // 4
    /*Changed to apply the offsetof macro*/
    struct {
        uint32_t partition_size : 7; // 8
        uint32_t reserved : 23;      // 8
        uint32_t mode : 1;           // 8
        uint32_t uc_b : 1;           // 8
    } aie_info;
    volatile uint32_t hsa_addr_high; // c
    volatile uint32_t hsa_addr_low;  // 10
    uint32_t ctx_switch_req;         // 14
    uint32_t hsa_location;           // 18
    uint32_t hsa_lite_status;        // 1c
    uint32_t misc_status;            // 20
    volatile uint32_t log_addr_high; // 24
    volatile uint32_t log_addr_low;  // 28
    uint32_t log_buf_size;           // 2c
    uint32_t host_time_high;         // 30
    uint32_t host_time_low;          // 34
    struct {
        volatile uint32_t dtrace_addr_high; // 38
        volatile uint32_t dtrace_addr_low;  // 3c
    } trace;
    struct {
        uint32_t restore_page; // 40
        uint32_t pdi_id;       // 44
        struct {
            uint16_t page_index;
            uint16_t page_len;
        } pdi_page; // 48
    } ctx_save;
    struct {
        volatile uint32_t hsa_addr_high; // 4c
        volatile uint32_t hsa_addr_low;  // 50
    } dbg;
    struct {
        volatile uint32_t dbg_buf_addr_high; // 54
        volatile uint32_t dbg_buf_addr_low;  // 58
        uint32_t size;                       // 5c
    } dbg_buf;
    volatile struct {
        uint32_t c_job_readiness_checked; // number of checks whether there are jobs ready
        uint32_t c_opcode;                // number of opcode run
        uint32_t c_job_launched;
        uint32_t c_job_finished;
        uint32_t c_hsa_pkt;                   // number of hsa pkt handled
        uint32_t c_page;                      // number of pages loaded
        uint32_t c_doorbell;                  // number of hsa doorbell ring
        uint32_t c_uc_scrub;                  // number of uc memory(PM) scrub
        uint32_t c_tct_requested;             // number of tct requested
        uint32_t c_tct_received;              // number of tct received
        uint16_t c_preemption_ucdma;          // run out of wait handle UC_DMA_WRITE_DES opcode
        uint16_t c_preemption_ucdma_sync;     // run out of wait handle UC_DMA_WRITE_DES_SYNC opcode
        uint16_t c_preemption_poll;           // POLL_32 opcode retry times
        uint16_t c_preemption_mask_poll;      // MASK_POLL_32 opcode retry times
        uint16_t c_preemption_remote_barrier; // run out of physical barrier REMOTE_BARRIER opcode
        uint16_t c_preemption_wait_tct;       // actor entry overflow or run out of wait handle WAIT_TCTS opcode
        uint16_t c_block_ucdma;               // block UC_DMA_WRITE_DES opcode
        uint16_t c_block_ucdma_sync;          // block UC_DMA_WRITE_DES_SYNC opcode
        uint16_t c_block_local_barrier;       // block local_barrier opcode
        uint16_t c_block_remote_barrier;      // block REMOTE_BARRIER opcode
        uint16_t c_block_wait_tct;            // block WAIT_TCTS opcode
        uint16_t c_actor_hash_conflict;       // number of slow actor entry lookup
    } counter;
    volatile struct {
        uint32_t fw_state;
        uint32_t abs_page_index; // absolute index of page where current control code are in
        uint32_t ppc; // previous pc (relative address to current page) that drives current_job_context to NULL
    } vm;
    volatile struct {
        uint32_t ear; // exception address
        uint32_t esr; // exception status
        uint32_t pc;  // exception pc
    } exception;
#ifdef PDI_LOAD_TEST
    uint32_t test_pdi_addr_high;
    uint32_t test_pdi_addr_low;
#endif
} handshake_t;

#define PARTITION_START (((lx7_handshake.partition_base_address) >> 25) & 0x7F)
#define PARTITION_SIZE (lx7_handshake.aie_info.partition_size)
#define UC_B (lx7_handshake.aie_info.uc_b)
#define DUAL_APP (lx7_handshake.aie_info.mode)
#define HSA_QUEUE_ADDR ((lx7_handshake.hsa_addr_low) | ((uint64_t)(lx7_handshake.hsa_addr_high) << 32))
#define HSA_QUEUE_ADDR_LOW (lx7_handshake.hsa_addr_low)
#define SLAVE_HSA_ADDR 0xFFFFFFFFFFFFFFFF

/*
 * hsa_lite_status register:
 *  bit 0: cmd completion
 *  bit 1: preemption save completion
 *  bit 2: hsa-lite queue empty
 * misc_status register
 *  bit 0: cert fw exception
 *  bit 1: control code hang
 */
#define SET_COMPLETION_STATUS                                                                                          \
    do {                                                                                                               \
        lx7_handshake.hsa_lite_status = 1;                                                                             \
    } while (0)
#define SET_PREEMPTION_SAVE_STATUS                                                                                     \
    do {                                                                                                               \
        lx7_handshake.hsa_lite_status = 2;                                                                             \
    } while (0)
#define SET_IDLE_STATUS                                                                                                \
    do {                                                                                                               \
        lx7_handshake.hsa_lite_status = 4;                                                                             \
    } while (0)
#define SET_EXCEPTION_STATUS                                                                                           \
    do {                                                                                                               \
        lx7_handshake.misc_status = 1;                                                                                 \
    } while (0)
#define SET_HANG_STATUS                                                                                                \
    do {                                                                                                               \
        lx7_handshake.misc_status = 2;                                                                                 \
    } while (0)

#define DTRACE_ADDR ((lx7_handshake.trace.dtrace_addr_low) | ((uint64_t)(lx7_handshake.trace.dtrace_addr_high) << 32))
#define DTRACE_ADDR_HIGH (lx7_handshake.trace.dtrace_addr_high)
#define DTRACE_ADDR_LOW (lx7_handshake.trace.dtrace_addr_low)
#define DBG_HSA_ADDR ((lx7_handshake.dbg.hsa_addr_low) | ((uint64_t)(lx7_handshake.dbg.hsa_addr_high) << 32))
#define DBG_HSA_ADDR_HIGH (lx7_handshake.dbg.hsa_addr_high)
#define DBG_HSA_ADDR_LOW (lx7_handshake.dbg.hsa_addr_low)
#define HOST_DBG_BUF_ADDR                                                                                              \
    ((lx7_handshake.dbg_buf.dbg_buf_addr_low) | ((uint64_t)(lx7_handshake.dbg_buf.dbg_buf_addr_high) << 32))
#define HOST_DBG_BUF_SIZE (lx7_handshake.dbg_buf.size)
#define LOG_ADDR_HIGH (lx7_handshake.log_addr_high)
#define LOG_ADDR_LOW (lx7_handshake.log_addr_low)
#define LOG_BUF_SIZE (lx7_handshake.log_buf_size)
#define HSA_LOCATION (lx7_handshake.hsa_location)
#define RR_SHARING (1UL << 0)
#define PREEMPTION (1UL << 1)
#define CTX_SWITCH_PENDING(type) (lx7_handshake.ctx_switch_req & type)
#define CTX_SWITCH_FLAG_CLEAR(type)                                                                                    \
    do {                                                                                                               \
        lx7_handshake.ctx_switch_req &= (~(type));                                                                     \
    } while (0)
#define RESTORE_PAGE (lx7_handshake.ctx_save.restore_page)
#define SAVED_PDI_ID (lx7_handshake.ctx_save.pdi_id)
#define SAVED_PDI_PAGE_INDEX (lx7_handshake.ctx_save.pdi_page.page_index)
#define SAVED_PDI_PAGE_LEN (lx7_handshake.ctx_save.pdi_page.page_len)

#define SET_FW_STATE(x)                                                                                                \
    do {                                                                                                               \
        lx7_handshake.vm.fw_state = x;                                                                                 \
    } while (0)
#define PPC (lx7_handshake.vm.ppc)
#define ABS_PAGE_INDEX (lx7_handshake.vm.abs_page_index)
#define EAR (lx7_handshake.exception.ear)
#define ESR (lx7_handshake.exception.esr)
#define UC_PC (lx7_handshake.exception.pc)

#define INCR_PERF_COUNTER(x)                                                                                           \
    do {                                                                                                               \
        ++lx7_handshake.counter.c_##x;                                                                                 \
    } while (0)
#define INCR_PERF_COUNTER_N(x, n)                                                                                      \
    do {                                                                                                               \
        lx7_handshake.counter.c_##x += n;                                                                              \
    } while (0)

#define HS_CLEAR_REGION_SIZE (sizeof(lx7_handshake.counter))
#define DB_CLEAR_REGION_SIZE (sizeof(lx7_handshake.dbg_buf))

#define CTRL_EXTERNAL (HSA_LOCATION == EXTERNAL_AXI_MM)

#define ALIVE_MAGIC 0x404C5645
#define LX7_ALIVE (lx7_handshake.mpaie_alive)

#ifdef SNL_BAREMETAL

#define TEST_DDR_BASE HSA_QUEUE_ADDR
#define TEST_DDR_SIZE 0x2000000 // 32MB
#define TEST_LIVE_POINTER (TEST_DDR_BASE + TEST_DDR_SIZE - 1024)

void claim_cert_active(uint64_t ddr_addr);

#endif

#ifdef HELLO_HSA_TEST
int hello_hsa_test(uint64_t hsa_address);
#endif

#ifdef PDI_LOAD_TEST
#define PDI_LOAD_TEST_ADDR ((lx7_handshake.test_pdi_addr_low) | ((uint64_t)(lx7_handshake.test_pdi_addr_high) << 32))
#define PDI_LOAD_TEST_ADDR_LOW (lx7_handshake.test_pdi_addr_low)
#endif

extern handshake_t lx7_handshake;
#endif
