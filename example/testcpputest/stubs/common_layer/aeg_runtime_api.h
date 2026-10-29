// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef AEG_RUNTIME_API_H
#define AEG_RUNTIME_API_H

#include "aeg_aie_control_api.h"
#include "aeg_api_config.h"
#include "aeg_api_message.h"
extern "C" {
#include <xaiengine.h>
}
#include <queue>
#include <vector>
namespace abr {

class config_manager {
  public:
    static err_code initialize(XAie_DevInst *devInst, size_t num_reserved_rows,
                               bool broadcast_enable_core) noexcept;

    static XAie_DevInst *s_pDevInst;

    static bool s_bInitialized;
    static size_t s_num_reserved_rows;
    static bool s_broadcast_enable_core;

  private:
    config_manager() = delete;
    ~config_manager() = delete;
    config_manager(const config_manager &) = delete;
    config_manager &operator=(const config_manager &) = delete;
    config_manager(config_manager &&) = delete;
    config_manager &operator=(config_manager &&) = delete;
};

class graph_api {
  public:
    explicit graph_api(const graph_config *pConfig) noexcept;
    graph_api(const graph_api &) = delete;
    graph_api &operator=(const graph_api &) = delete;
    graph_api(graph_api &&) = delete;
    graph_api &operator=(graph_api &&) = delete;
    virtual ~graph_api() = default;

    err_code configure() noexcept;
    err_code run() noexcept;
    err_code run(int32_t testIter) noexcept;
    err_code wait() noexcept;
    err_code wait(uint64_t cycleTimeout) noexcept;
    err_code resume() noexcept;
    err_code end() noexcept;
    err_code end(uint64_t cycleTimeout) noexcept;
    err_code update(const shared_buffer_config *pSharedBufferConfig, const void *pValue, size_t numBytes) const noexcept;
    err_code update(const rtp_config *pRTPConfig, const void *pValue, size_t numBytes) noexcept;
    err_code read(const rtp_config *pRTPConfig, void *pValue, size_t numBytes) const noexcept;

  private:
    const graph_config *pGraphConfig;
    bool isConfigured;
    bool isRunning;
    uint64_t startTime;

    std::vector<XAie_LocType> coreTiles;
    std::vector<XAie_LocType> iterMemTiles;
    std::vector<int32_t> asyncNotFirstTimePorts;
};

class gmio_api {
  public:
    explicit gmio_api(const gmio_config *pConfig) noexcept;
    gmio_api(const gmio_api &) = delete;
    gmio_api &operator=(const gmio_api &) = delete;
    gmio_api(gmio_api &&) = delete;
    gmio_api &operator=(gmio_api &&) = delete;
    virtual ~gmio_api() = default;

    err_code configure() noexcept;
#if !defined(__AIESIM__) && defined(__NOT_BARE_METAL__)
    err_code enqueueBD(XAie_MemInst *memInst, uint64_t offset, size_t size) noexcept;
#else
    err_code enqueueBD(uint64_t address, size_t size) noexcept;
#endif
    err_code wait(bool syncmode = true) noexcept;

  private:
    const gmio_config *pGMIOConfig;

    XAie_DmaDesc shimDmaInst;
    XAie_LocType gmioTileLoc;

    bool isConfigured;
    uint8_t dmaStartQMaxSize;
    std::queue<size_t> enqueuedBDs;
    std::queue<size_t> availableBDs;
};

err_code checkRTPConfigForUpdate(const rtp_config *pRTPConfig, const graph_config *pGraphConfig, size_t numBytes,
                                 bool isRunning = false) noexcept;
err_code checkRTPConfigForRead(const rtp_config *pRTPConfig, const graph_config *pGraphConfig, size_t numBytes) noexcept;
err_code checkSharedBufferConfigForUpdate(const shared_buffer_config *pSharedBufferConfig,
                                          const graph_config *pGraphConfig, size_t numBytes) noexcept;
}
#endif
