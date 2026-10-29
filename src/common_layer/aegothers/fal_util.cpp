/**<!--
 * Copyright (C) 2021 Xilinx, Inc
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

#include "fal_util.h"
namespace fal {
std::shared_ptr<xaiefal::XAieDev> fal_util::s_pXAieDev;

bool fal_util::initialize(XAie_DevInst *pDevInst) {
    if (!pDevInst)
    {
        return false;
    }
    s_pXAieDev = std::make_shared<xaiefal::XAieDev>(pDevInst);
    return true;
}

int32_t fal_util::request(std::shared_ptr<xaiefal::XAieRsc> pResource) {
    if (!pResource)
    {
        return -1;
    }
    if (auto pBroadcastRsc = std::dynamic_pointer_cast<xaiefal::XAieBroadcast>(pResource)) {
        const auto xaiefalStatus = pBroadcastRsc->reserve(); // Will return error if already reserved
        if (xaiefalStatus == AieRC::XAIE_OK) {
            return pBroadcastRsc->getBc();
        }
        return -1;
    } else if (auto pSingleTileRsc = std::dynamic_pointer_cast<xaiefal::XAieSingleTileRsc>(pResource)) {
        int32_t id = -1;
        uint32_t rsc_id = 0U;
        XAie_LocType loc;
        XAie_ModuleType module{};
        auto xaiefalStatus = pSingleTileRsc->reserve(); // Will return error if already reserved
        if (xaiefalStatus == AieRC::XAIE_OK) {
            xaiefalStatus = pSingleTileRsc->getRscId(loc, module, rsc_id);
        }
        if (xaiefalStatus == AieRC::XAIE_OK) {
            id = static_cast<int32_t>(rsc_id);
        }
        return id;
    } else {
        /* No matching resource type - fall through to default return */
    }

    return -1;
}

bool fal_util::release(std::shared_ptr<xaiefal::XAieRsc> pResource) {
    if (!pResource)
    {
        return false;
    }
    const auto xaiefalStatus = pResource->release(); // If resource is not reserved, release() will not error out
    return (xaiefalStatus == AieRC::XAIE_OK);
}
} // namespace fal