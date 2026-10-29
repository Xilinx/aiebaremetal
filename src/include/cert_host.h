// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef CERT_HOST_H
#define CERT_HOST_H
#include <array>
#include <cstring>
#include <map>
#include <cstdint>
#include <xaiengine.h>
#include "handshake.h"
#include <cert_elf_loader.h>

#define HSA_QUEUE_ENTRY 32
namespace cert_host {

struct hsa_queue {
    struct host_queue_header hq_header;
    std::array<struct host_queue_packet, static_cast<size_t>(HSA_QUEUE_ENTRY)> hq_entry;
    XAie_MemInst *completion_signal;
};


class cert_api {
  public:
    explicit cert_api(XAie_DevInst *DevInst);
    cert_api();
    ~cert_api();
    cert_api(const cert_api &) = delete;
    cert_api(cert_api &&) = delete;
    cert_api &operator=(const cert_api &) = delete;
    cert_api &operator=(cert_api &&) = delete;
    void init_devinst(XAie_DevInst *dev_inst);
    int32_t load_cert() const;
    int32_t load_cert(uint32_t col) const;
    void run_cert(uint32_t cycles) const;
    int32_t wakeup_cert() const;
    int32_t wakeup_cert(uint32_t col) const;
    int32_t init_handshake();
    int32_t submit_command(const char *ctrl_elf, uint32_t ctrl_elf_len,
                                  const std::map<std::string_view, uint64_t> &symbol) const;
    int32_t submit_test_command() const;
    /* xrt_id to sim_id map. Provided by external_buffers_id.json */
    std::map<int64_t, int64_t> external_buffer_map;
    /* sim_id to buffer address mapping */
    std::map<int64_t, uint64_t> external_buffer_addr_map;
    void parse_host_args();

    XAie_DevInst *DevInst = nullptr;
    handshake_t handshake = {};
    uint32_t host_args_low = 0U;
    uint32_t host_args_high = 0U;
    XAie_MemInst *host_args_meminst = nullptr;
    XAie_MemInst *hsa_meminst = nullptr;
    XAie_MemInst *comp_sig_meminst = nullptr;
};

} // namespace cert_host
#endif // CERT_HOST_H
