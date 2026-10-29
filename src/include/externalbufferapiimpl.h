/**<!--
 * Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. A copy of the
 * License is located at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 * -->
 */
#ifndef EXTERNALBUFFERAPIIMPL_H_
#define EXTERNALBUFFERAPIIMPL_H_
// This file contains implementation of external buffer runtime APIs.
#include <array>
#include <cinttypes>
#include <cstdint>
#include <map>
#include <memory>
#include <queue>
#include <string_view>
#include "aeg_log.h"
#include "common_layer/aeg_api_config.h"
#include "common_layer/aeg_runtime_api.h"
#include "common_layer/aeg_aie_control_api.h"
#include "xaiemem.h"
namespace externalbufferapiimpl {



struct TransactionInfo {
  public:
    void *virtualAddr = nullptr;
    uint64_t physicalAddr = 0U;
    size_t size = 0U;

    TransactionInfo(void *virtual_addr, uint64_t physical_addr, size_t transaction_size) {
        virtualAddr = virtual_addr;
        physicalAddr = physical_addr;
        size = transaction_size;
    }
    TransactionInfo() {}
};

class ExternalBufferApiImpl {
  private:
    std::string_view ebufferName;
    abr::external_buffer_config config;
    void *ddr_address = nullptr;
    std::array<void*, 2> pingpong_address = {{nullptr, nullptr}};
    std::shared_ptr<xaiemem::XAieMem> xaiemem;
    std::queue<TransactionInfo> aie2gm_buf_queue;
    std::queue<TransactionInfo> gm2aie_buf_queue;
    abr::err_code getAddress(uint64_t &phy_address, void *const ddr_mem) const {
        int32_t ret = 0;
        AEG_LOG("DDR address from getaddress():%p\n", ddr_mem);
        ret = xaiemem->get_dev_addr(const_cast<void *>(ddr_mem), &phy_address);
        if (ret != 0) {
            return abr::err_code::aie_driver_error;
        }
        return abr::err_code::ok;
    }
  public:
    ExternalBufferApiImpl(abr::external_buffer_config ebconfig, std::string_view name, std::shared_ptr<xaiemem::XAieMem> mem)
        : ebufferName(name), config(std::move(ebconfig)), xaiemem(std::move(mem)) {}

    void setAddress(void *ptr) {
        if (ddr_address == nullptr) {
            ddr_address = ptr;
        } else {
            AEG_ERROR("Address is already set\n");
        }
        AEG_LOG("ddr address:%p\n", ddr_address);
    }
    abr::err_code wait(std::string_view port_name) {
        abr::err_code status = abr::err_code::ok;
        bool valid_port = false;
        if (config.shimPortConfigs.empty()) {
            return status;
        }

        abr::shim_port_config port_config{};
        for (auto &t_portConfig : config.shimPortConfigs) {
            if (port_name == t_portConfig.portName) {
                port_config = t_portConfig;
                valid_port = true;
                break;
            }
        }

        if (!valid_port) {
            AEG_ERROR("port_name:%.*s is not a valid port\n", static_cast<int32_t>(port_name.size()), port_name.data());
            return status;
        }

        status = static_cast<abr::err_code>(abr::dma_api::waitDMAChannelDone(1,static_cast<uint8_t>(port_config.column), 0 /*shim row*/,
                        port_config.S2MMOrMM2S, static_cast<uint8_t>(port_config.channel)));
#if defined(__AIESIM__)
        // 0:S2MM, 1:MM2S
        AEG_LOG("This line should only show under SIMULATOR mode--If you see it in HW,SOMETHING WRONG !!! \n");
        if (port_config.S2MMOrMM2S != 0) {
            while (!gm2aie_buf_queue.empty()) {
                auto var = gm2aie_buf_queue.front();
                gm2aie_buf_queue.pop();
                static_cast<void>(xaiemem->sync_for_cpu(reinterpret_cast<void *>(var.virtualAddr), var.size));
            }
        } else {
            while (!aie2gm_buf_queue.empty()) {
                auto var = aie2gm_buf_queue.front();
                aie2gm_buf_queue.pop();
                static_cast<void>(xaiemem->sync_for_cpu(reinterpret_cast<void *>(var.virtualAddr), var.size));
            }
        }
        return status;
#endif
        return status;
    }

    abr::err_code gm2aie_nb(std::string_view port_name) {
        abr::err_code status = abr::err_code::ok;
        bool valid_port = false;
        if (config.shimPortConfigs.empty()) {
            return status;
        }

        abr::shim_port_config port_config{};

        for (auto &t_portConfig : config.shimPortConfigs) {
            if (port_name == t_portConfig.portName) {
                port_config = t_portConfig;
                valid_port = true;
                break;
            }
        }

        if (!valid_port) {
            AEG_ERROR("port_name:%.*s is not a valid port\n", static_cast<int32_t>(port_name.size()), port_name.data());
            return status;
        }

        int32_t start_bd = -1;
        uint64_t transaction_size = 0;
        uint64_t phy_address = 0;
        status = getAddress(phy_address, ddr_address);
        if (status != abr::err_code::ok) {
            AEG_ERROR("Failed to get physical address\n");
        }
        AEG_LOG("physical address:0x%" PRIx64 "\n", phy_address);
        for (const auto &shim_bd_info : port_config.shimBDInfos) {
            const uint64_t shim_offset_bytes =
                (static_cast<uint64_t>(shim_bd_info.offset)) * (static_cast<uint64_t>(4U));
            uint64_t bd_address =
                phy_address + shim_offset_bytes; // offset is 32-bit words; address in bytes
            status = abr::dma_api::updateBDAddress(1, static_cast<uint8_t>(port_config.column), 0 /*shim row*/,
                                            static_cast<uint8_t>(shim_bd_info.bd_id), bd_address);
            if (status != abr::err_code::ok) {
                AEG_ERROR("Failed to update BD address\n");
            }

            transaction_size += static_cast<uint64_t>(shim_bd_info.transaction_size);
            if (start_bd < 0){
                start_bd = static_cast<int32_t>(shim_bd_info.bd_id);
            }
        }
        AEG_LOG("Transaction size:%zu\n", static_cast<size_t>(transaction_size));
#if defined(__AIESIM__)
        TransactionInfo transaction(ddr_address, phy_address, static_cast<std::size_t>(transaction_size));
        gm2aie_buf_queue.emplace(transaction);
#endif

        static_cast<void>(xaiemem->sync_for_dev(ddr_address, static_cast<uint64_t>(transaction_size)));

        status = abr::dma_api::enqueueTask(1, static_cast<std::uint8_t>(port_config.column), 0,
                                        port_config.S2MMOrMM2S, static_cast<std::uint8_t>(port_config.channel),
                                        static_cast<uint32_t>(port_config.taskRepetition),
                                        port_config.enable_task_complete_token,
                                        static_cast<uint8_t>(start_bd));

        return status;
    }

    abr::err_code aie2gm_nb(std::string_view port_name) {
        abr::err_code status = abr::err_code::ok;
        bool valid_port = false;

        if (config.shimPortConfigs.empty()) {
            return status;
        }
        abr::shim_port_config port_config{};

        for (auto &t_port_config : config.shimPortConfigs) {
            if (port_name == t_port_config.portName) {
                port_config = t_port_config;
                valid_port = true;
                break;
            }
        }

        if (!valid_port) {
            AEG_ERROR("port_name:%.*s is not a valid port\n", static_cast<int32_t>(port_name.size()), port_name.data());
            return status;
        }

        int32_t start_bd = -1;
        uint64_t transaction_size = 0;
        uint64_t phy_address = 0;
        status = getAddress(phy_address, ddr_address);
        if (status != abr::err_code::ok) {
            AEG_ERROR("Failed to get physical address\n");
        }
        AEG_LOG("physical address:0x%" PRIx64 "\n", phy_address);
        for (const auto &shim_bd_info : port_config.shimBDInfos) {
            const uint64_t shim_offset_bytes =
                (static_cast<uint64_t>(shim_bd_info.offset)) * (static_cast<uint64_t>(4U));
            uint64_t bd_address = phy_address + shim_offset_bytes; // offset is 32-bit words; address in bytes
            status = abr::dma_api::updateBDAddress(1, static_cast<uint8_t>(port_config.column), 0 /*shim row*/,
                                            static_cast<uint8_t>(shim_bd_info.bd_id), bd_address);
            if (status != abr::err_code::ok) {
                AEG_ERROR("Failed to update BD address\n");
            }

            transaction_size += shim_bd_info.transaction_size;
            if (start_bd < 0) {
                start_bd = static_cast<int32_t>(shim_bd_info.bd_id);
            }
        }

#if defined(__AIESIM__)
        TransactionInfo transaction(ddr_address, phy_address, static_cast<size_t>(transaction_size));
        aie2gm_buf_queue.emplace(transaction);
#else

        static_cast<void>(xaiemem->sync_for_dev(reinterpret_cast<void *>(phy_address), static_cast<uint64_t>(transaction_size)));
#endif
        status = abr::dma_api::enqueueTask(1, static_cast<uint8_t>(port_config.column), 0,
                                        port_config.S2MMOrMM2S, static_cast<uint8_t>(port_config.channel),
                                        static_cast<uint32_t>(port_config.taskRepetition),
                                        port_config.enable_task_complete_token,
                                        static_cast<uint8_t>(start_bd));

        return status;
    }

    abr::err_code gm2aie(std::string_view port_name) {
        abr::err_code status = gm2aie_nb(port_name);
        if (status == abr::err_code::ok) {
            static_cast<void>(wait(port_name));
        }
        return status;
    }

    abr::err_code aie2gm(std::string_view port_name) {
        abr::err_code status = aie2gm_nb(port_name);
        if (status == abr::err_code::ok) {
            static_cast<void>(wait(port_name));
        }
        return status;
    }

    abr::err_code setupPingPongBuffers(void *const ptr1, void *const ptr2, bool sync_cache = true) {
        abr::err_code status = abr::err_code::ok;
        if ((pingpong_address[0] == nullptr) && (pingpong_address[1] == nullptr)) {
            pingpong_address[0] = ptr1;
            pingpong_address[1] = ptr2;
        } else {
            AEG_ERROR("Address is already set\n");
        }

        if (config.shimPortConfigs.empty()) {
            return status;
        }

        // All ports of the externalbuffer
        for (auto &port_config : config.shimPortConfigs) {
            int32_t start_bd = -1;
#if defined(__AIESIM__)
            TransactionInfo transaction{};
#endif
            uint64_t phy_address = 0;
            uint64_t transaction_size = 0;

            for (const auto &shim_bd_info : port_config.shimBDInfos) {
                auto buf_idx = shim_bd_info.buf_idx;
                status = getAddress(phy_address, pingpong_address[buf_idx]);
                if (status != abr::err_code::ok) {
                    AEG_ERROR("Failed to get physical address\n");
                }
                AEG_LOG("physical address:0x%" PRIx64 "\n", phy_address);

                const uint64_t shim_offset_bytes =
                    (static_cast<uint64_t>(shim_bd_info.offset)) * (static_cast<uint64_t>(4U));
                uint64_t const bd_address = phy_address + shim_offset_bytes; // words → bytes
                status = abr::dma_api::updateBDAddress(1, static_cast<uint8_t>(port_config.column), 0 /*shim row*/,
                                                    static_cast<uint8_t>(shim_bd_info.bd_id), bd_address);
                if (status != abr::err_code::ok) {
                    AEG_ERROR("Failed to update BD address\n");
                }
                transaction_size += shim_bd_info.transaction_size;
                if (start_bd < 0) {
                    start_bd = static_cast<int32_t>(shim_bd_info.bd_id);
                }

#if defined(__AIESIM__)
                transaction.virtualAddr = pingpong_address[static_cast<size_t>(buf_idx)];
                transaction.physicalAddr = phy_address;
                transaction.size = transaction_size;
#endif
            }

#if defined(__AIESIM__)

            if (port_config.S2MMOrMM2S == 0 /*s2mm*/) /*aie2gm*/{
                aie2gm_buf_queue.emplace(transaction);
            } else {
                gm2aie_buf_queue.emplace(transaction);
            }

#else
            if (sync_cache) {
                static_cast<void>(xaiemem->sync_for_dev(reinterpret_cast<void *>(phy_address), static_cast<uint64_t>(transaction_size)));
            }
#endif

            status = abr::dma_api::enqueueTask(static_cast<int32_t>(1), static_cast<uint8_t>(port_config.column), static_cast<uint8_t>(0),
                                            static_cast<int32_t>(port_config.S2MMOrMM2S), static_cast<uint8_t>(port_config.channel),
                                            static_cast<uint32_t>(port_config.taskRepetition),
                                            port_config.enable_task_complete_token,
                                            static_cast<uint8_t>(start_bd));
            if (status != abr::err_code::ok) {
                AEG_ERROR("Failed to enqueue task\n");
            }
        }
        return status;
    }
};
} // namespace externalbufferapiimpl
#endif /* EXTERNALBUFFERAPIIMPL_H_ */
