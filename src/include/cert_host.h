// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <stdint.h>
#include <stdio.h>
#include <xaiengine.h>

#include "handshake.h"

#include <cert_elf_loader.h>

#define HSA_QUEUE_ENTRY 32
struct hsa_queue {
    struct host_queue_header hq_header;
    struct host_queue_packet hq_entry[HSA_QUEUE_ENTRY];
    XAie_MemInst *completion_signal;
};

namespace cert_host {

class cert_api {
  public:
    cert_api(XAie_DevInst *DevInst);
    cert_api();
    ~cert_api();
    void init_devinst(XAie_DevInst *DevInst);
    int load_cert();
    int load_cert(u32 col);
    void run_cert(u32 cycles);
    int wakeup_cert();
    int wakeup_cert(u32 col);
    int init_handshake();
    int submit_command(char *ctrl_elf, unsigned int ctrl_elf_len, std::map<std::string_view, uint64_t> &symbol);
    int submit_test_command();
    /* xrt_id to sim_id map. Provided by external_buffers_id.json */
    std::map<int64_t, int64_t> external_buffer_map;
    /* sim_id to buffer address mapping */
    std::map<int64_t, uint64_t> external_buffer_addr_map;
    void parse_host_args();

  private:
    XAie_DevInst *DevInst;
    handshake_t handshake;
    uint32_t host_args_low;
    uint32_t host_args_high;
    XAie_MemInst *host_args_meminst;
    XAie_MemInst *hsa_meminst;
    XAie_MemInst *comp_sig_meminst;
};

} // namespace cert_host
