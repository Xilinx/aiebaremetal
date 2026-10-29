// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef FAL_UTIL_H
#define FAL_UTIL_H

#define FAL_UTIL_MOCK_CLASS_BASED 1

#include <cstdint>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace xaiefal {
class XAieRsc;
class XAieDev;
}

class fal_util {
  public:
    static bool initialize(XAie_DevInst *pDevInst);
    static int32_t request(std::shared_ptr<xaiefal::XAieRsc> pResource);
    static bool release(std::shared_ptr<xaiefal::XAieRsc> pResource);
    static std::shared_ptr<xaiefal::XAieDev> s_pXAieDev;
};

#endif
