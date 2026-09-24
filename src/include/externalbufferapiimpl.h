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

// This file contains implementation of external buffer runtime APIs.
#include <queue>
#include <map>
#include <cstdint>
#include "xaiemem.h"
#include "aeg_log.h"
#include "common_layer/aeg_api_config.h"
#include "common_layer/aeg_runtime_api.h"
#include "common_layer/aeg_aie_control_api.h"
#include <cstdint>
#include <iostream>
#include <ostream>
using namespace std;
using namespace abr;
typedef struct _TransactionInfo {
  public:
    void *virtualAddr;
    uint64_t physicalAddr;
    size_t size;

    _TransactionInfo(void *virtual_addr, uint64_t physical_addr, size_t transaction_size) {
        virtualAddr = virtual_addr;
        physicalAddr = physical_addr;
        size = transaction_size;
    }
    _TransactionInfo() {}
} TransactionInfo;

class ExternalBufferApiImpl {
  private:
    std::string ebufferName;
    abr::external_buffer_config config;
    void *ddr_address = nullptr;
    void *pingpong_address[2] = {nullptr, nullptr};
    std::shared_ptr<XAieMem> xaiemem;
    std::queue<TransactionInfo> aie2gm_buf_queue;
    std::queue<TransactionInfo> gm2aie_buf_queue;
    abr::err_code getAddress(uint64_t &phy_address, void *const ddr_address) {
        // use physical address for bare metal and AIESIM
        int ret;
        AEG_LOG("DDR address from getaddress():%x\n", ddr_address);
        ret = xaiemem->get_dev_addr(const_cast<void *>(ddr_address), &phy_address);
        if (ret)
            return abr::err_code::aie_driver_error;
        return abr::err_code::ok;
    }

  public:
    ExternalBufferApiImpl(abr::external_buffer_config ebconfig, std::string name, std::shared_ptr<XAieMem> mem)
        : config(ebconfig), ebufferName(name), xaiemem(mem) {}

    void setAddress(void *ptr) {
        if (ddr_address == nullptr) {
            ddr_address = ptr;
        } else {
            std::cout << "Address is already set" << "\n";
        }
        printf("ddr address:%x\n", ddr_address);
    }
    abr::err_code wait(std::string port_name) {
        abr::err_code status = abr::err_code::ok;
        bool valid_port = false;
        if (config.shimPortConfigs.empty())
            return status;

        shim_port_config port_config;
        for (auto &t_portConfig : config.shimPortConfigs) {
            if (port_name == t_portConfig.portName) {
                port_config = t_portConfig;
                valid_port = true;
                break;
            }
        }

        if (!valid_port) {
            std::cout << port_name << " is not a valid port" << std::endl;
            return status;
        }

        status = (abr::err_code)dma_api::waitDMAChannelDone(1, port_config.column, 0 /*shim row*/,
                                                            port_config.S2MMOrMM2S, port_config.channel);
#if defined(__AIESIM__)
        // 0:S2MM, 1:MM2S
        AEG_LOG("This line should only show under SIMULATOR mode--If you see it in HW,SOMETHING WRONG !!! \n");
        if (port_config.S2MMOrMM2S) {
            while (!gm2aie_buf_queue.empty()) {
                auto var = gm2aie_buf_queue.front();
                gm2aie_buf_queue.pop();
                xaiemem->sync_for_cpu(reinterpret_cast<void *>(var.virtualAddr), var.size);
            }
        } else {
            while (!aie2gm_buf_queue.empty()) {
                auto var = aie2gm_buf_queue.front();
                aie2gm_buf_queue.pop();
                xaiemem->sync_for_cpu(reinterpret_cast<void *>(var.virtualAddr), var.size);
            }
        }
        return status;
#endif
        return status;
    }

    abr::err_code gm2aie_nb(std::string port_name) {
        abr::err_code status = abr::err_code::ok;
        bool valid_port = false;
        if (config.shimPortConfigs.empty())
            return status;

        shim_port_config port_config;

        for (auto &t_portConfig : config.shimPortConfigs) {
            if (port_name == t_portConfig.portName) {
                port_config = t_portConfig;
                valid_port = true;
                break;
            }
        }

        if (!valid_port) {
            std::cout << port_name << " is not a valid port" << std::endl;
            return status;
        }

        int start_bd = -1;
        uint64_t transaction_size = 0;
        uint64_t phy_address;
        status = getAddress(phy_address, ddr_address);
        AEG_LOG("physical address:%x\n", (unsigned)phy_address);
        for (const auto &shim_bd_info : port_config.shimBDInfos) {
            auto buf_idx = shim_bd_info.buf_idx;

            uint64_t bd_address =
                phy_address +
                shim_bd_info.offset *
                    4; // adf::dma_buffer_descriptor::offset is in 32bit word, but driver address is in bytes
            status = dma_api::updateBDAddress(1, port_config.column, 0, (uint8_t)shim_bd_info.bd_id, bd_address);

            transaction_size += shim_bd_info.transaction_size;
            if (start_bd < 0)
                start_bd = shim_bd_info.bd_id;
        }
        AEG_LOG("Transaction size:%zu\n", (size_t)transaction_size);
#if defined(__AIESIM__)
        TransactionInfo transaction((void *)((uint64_t)ddr_address), phy_address, transaction_size);
        gm2aie_buf_queue.emplace(transaction);
#endif

        xaiemem->sync_for_dev(reinterpret_cast<void *>((uint64_t)ddr_address), transaction_size);

        status = dma_api::enqueueTask(1, port_config.column, 0, port_config.S2MMOrMM2S, port_config.channel,
                                      port_config.taskRepetition, port_config.enable_task_complete_token,
                                      static_cast<uint8_t>(start_bd));

        return status;
    }

    abr::err_code aie2gm_nb(std::string port_name) {
        abr::err_code status = abr::err_code::ok;
        bool valid_port = false;

        if (config.shimPortConfigs.empty())
            return status;

        shim_port_config port_config;

        for (auto &t_port_config : config.shimPortConfigs) {
            if (port_name == t_port_config.portName) {
                port_config = t_port_config;
                valid_port = true;
                break;
            }
        }

        if (!valid_port) {
            std::cout << port_name << " is not a valid port" << std::endl;
            return status;
        }

        int start_bd = -1;
        uint64_t transaction_size = 0;
        uint64_t phy_address;
        status = getAddress(phy_address, ddr_address);
        AEG_LOG("physical address:%x\n", (unsigned)phy_address);
        for (const auto &shim_bd_info : port_config.shimBDInfos) {
            auto buf_idx = shim_bd_info.buf_idx;

            uint64_t bd_address =
                phy_address +
                shim_bd_info.offset *
                    4; // adf::dma_buffer_descriptor::offset is in 32bit word, but driver address is in bytes
            status = dma_api::updateBDAddress(1, port_config.column, 0 /*shim row*/, (uint8_t)shim_bd_info.bd_id,
                                              bd_address);
            transaction_size += shim_bd_info.transaction_size;
            if (start_bd < 0)
                start_bd = shim_bd_info.bd_id;
        }

#if defined(__AIESIM__)
        TransactionInfo transaction((void *)((uint64_t)ddr_address), phy_address, transaction_size);
        aie2gm_buf_queue.emplace(transaction);
#else

        xaiemem->sync_for_dev(reinterpret_cast<void *>(phy_address), transaction_size);
#endif
        status = dma_api::enqueueTask(1, port_config.column, 0, port_config.S2MMOrMM2S, port_config.channel,
                                      port_config.taskRepetition, port_config.enable_task_complete_token,
                                      static_cast<uint8_t>(start_bd));

        return status;
    }

    abr::err_code gm2aie(std::string port_name) {
        abr::err_code status = gm2aie_nb(port_name);
        if (status == abr::err_code::ok) {
            wait(port_name);
        }
        return status;
    }

    abr::err_code aie2gm(std::string port_name) {
        abr::err_code status = aie2gm_nb(port_name);
        if (status == abr::err_code::ok) {
            wait(port_name);
        }
        return status;
    }

    abr::err_code setupPingPongBuffers(void *const ptr1, void *const ptr2) {
        abr::err_code status = abr::err_code::ok;
        if ((pingpong_address[0] == nullptr) && (pingpong_address[1] == nullptr)) {
            pingpong_address[0] = ptr1;
            pingpong_address[1] = ptr2;
        } else {
            std::cout << "Address is already set" << "\n";
        }

        if (config.shimPortConfigs.empty())
            return status;

        shim_port_config port_config;

        // All ports of the externalbuffer
        for (auto &port_config : config.shimPortConfigs) {
            int start_bd = -1;
            TransactionInfo transaction;
            uint64_t phy_address;
            uint64_t transaction_size = 0;

            for (const auto &shim_bd_info : port_config.shimBDInfos) {
                auto buf_idx = shim_bd_info.buf_idx;
                status = getAddress(phy_address, pingpong_address[buf_idx]);
                AEG_LOG("physical address:%x\n", (unsigned)phy_address);

                uint64_t bd_address =
                    phy_address +
                    shim_bd_info.offset *
                        4; // adf::dma_buffer_descriptor::offset is in 32bit word, but driver address is in bytes
                status = dma_api::updateBDAddress(1, port_config.column, 0 /*shim row*/, (uint8_t)shim_bd_info.bd_id,
                                                  bd_address);
                transaction_size += shim_bd_info.transaction_size;
                if (start_bd < 0)
                    start_bd = shim_bd_info.bd_id;

                transaction.virtualAddr = (void *)((uint64_t)pingpong_address[buf_idx]);
                transaction.physicalAddr = phy_address;
                transaction.size = transaction_size;
            }

#if defined(__AIESIM__)

            if (port_config.S2MMOrMM2S == 0 /*s2mm*/) /*aie2gm*/
                aie2gm_buf_queue.emplace(transaction);
            else
                gm2aie_buf_queue.emplace(transaction);

#else

            xaiemem->sync_for_dev(reinterpret_cast<void *>(phy_address), transaction_size);
#endif
            status = dma_api::enqueueTask(1, port_config.column, 0, port_config.S2MMOrMM2S, port_config.channel,
                                          port_config.taskRepetition, port_config.enable_task_complete_token,
                                          static_cast<uint8_t>(start_bd));
        }
        return status;
    }
};
