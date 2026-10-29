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
#ifndef GMIO_API_IMPL_H
#define GMIO_API_IMPL_H

#include <cstdint>
#include <memory>
#include <queue>
#include <string_view>

#include "aeg_baremetal_api.h"
#include "common_layer/aeg_api_config.h"
#include "common_layer/aeg_runtime_api.h"
#include "xaiemem.h"
namespace gmioapiimpl {

enum class AIE_GMIO_DIR : std::int32_t {
    AIE2GM = 0,
    GM2AIE,
};

struct MemMap {
    MemMap(std::uint64_t paddr, std::uint64_t vaddr, std::uint64_t dlen) {
        sim_phy_addr = paddr;
        host_virtual_addr = vaddr;
        len = dlen;
    };
    std::uint64_t sim_phy_addr = 0U;
    std::uint64_t host_virtual_addr = 0U;
    std::uint64_t len = 0U;
};

class GmioApiImpl {
  public:
    GmioApiImpl(std::shared_ptr<abr::gmio_api> gapi, abr::gmio_config gconfig, std::shared_ptr<xaiemem::XAieMem> mem,
                std::string_view name)
        : gmio_api(std::move(gapi)), gmioconfig(gconfig), xaiemem(std::move(mem)), gmioName(name) {}
    std::int32_t wait(bool syncmode = true) const {
        std::int32_t ret = static_cast<std::int32_t>(gmio_api->wait(syncmode));
#if defined(__AIESIM__)
        while (!aie2gm_buf_queue.empty()) {
            auto var = aie2gm_buf_queue.front();
            aie2gm_buf_queue.pop();
            xaiemem->sync_for_cpu(reinterpret_cast<void *>(var.host_virtual_addr), var.len);
        }
#endif
        return ret;
    }

    abr::err_code gm2aie_nb(char *data, std::int32_t len) const {
        std::uint64_t phy_data = 0;
        std::int32_t ret = 0;

        ret = xaiemem->get_dev_addr(data, &phy_data);
        if (ret != 0) {
            return abr::err_code::aie_driver_error;
        }
        static_cast<void>(xaiemem->sync_for_dev(static_cast<void *>(data), static_cast<std::uint64_t>(len)));
        static_cast<void>(gmio_sync_nb(AIE_GMIO_DIR::GM2AIE, phy_data, len));
        return abr::err_code::ok;
    }

    abr::err_code aie2gm_nb(char *data, std::int32_t len) const {
        std::uint64_t phy_data = 0;
        std::int32_t ret = 0;

        ret = xaiemem->get_dev_addr(data, &phy_data);
        if (ret != 0) {
            return abr::err_code::aie_driver_error;
        }
#if defined(__AIESIM__)
        aie2gm_buf_queue.emplace(phy_data, reinterpret_cast<std::uint64_t>(data), len);
#else
        // due the arm8 strong cache coherent feature, before to invalid the ddr memory
        // for reading the data from DDR,we need to clean the cache, if not the next
        // invalid will clean the dirty cache then invalid it, that means, if the ddr
        // is dirty in current time, future we only get a old value
        static_cast<void>(xaiemem->sync_for_cpu(static_cast<void *>(data), static_cast<std::uint64_t>(len)));
#endif
        return gmio_sync_nb(AIE_GMIO_DIR::AIE2GM, phy_data, len);
    }

    abr::err_code aie2gm(char *data, std::int32_t len) const {
        abr::err_code status = aie2gm_nb(data, len);
        if (status == abr::err_code::ok) {
            static_cast<void>(wait());
        }
        return status;
    }

    abr::err_code gm2aie(char *data, std::int32_t len) const {
        abr::err_code status = gm2aie_nb(data, len);
        if (status == abr::err_code::ok) {
            static_cast<void>(wait());
        }
        return status;
    }

    abr::err_code gmio_sync_nb(AIE_GMIO_DIR dir, std::uint64_t data, std::int32_t len) const {
        switch (dir) {
        case AIE_GMIO_DIR::AIE2GM:
            if (static_cast<std::int32_t>(gmioconfig.type) != 1) {
                AEG_ERROR("gmio_config.type should be 1 for aie2gm\n");
            }
            break;
        case AIE_GMIO_DIR::GM2AIE:
            if (static_cast<std::int32_t>(gmioconfig.type) != 0) {
                AEG_ERROR("gmio_config.type should be 0 for gm2aie\n");
            }
            break;
        default:
            AEG_ERROR(" the direction is wrong\n");
            break;
        }
        return gmio_api->enqueueBD(data, static_cast<std::uint64_t>(len));
    }

  private:
  mutable std::queue<MemMap> aie2gm_buf_queue;
    std::shared_ptr<abr::gmio_api> gmio_api;
    abr::gmio_config gmioconfig;
    std::shared_ptr<xaiemem::XAieMem> xaiemem;
    std::string_view gmioName;
};
} // namespace gmioapiimpl
#endif /* GMIO_API_IMPL_H */
