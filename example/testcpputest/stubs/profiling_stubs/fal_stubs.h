// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "stubs.h"
#include <string>
#include <memory>
#include <atomic>

#include "xaiefal_mock.hpp"
#include "aeg_api_message.h"

namespace abr {

namespace config_manager {

extern void* s_pDevInst;
}

}

namespace fal_util {

class TilePlStub
{
public:
    std::shared_ptr<xaiefal::XAieRsc> perfCounter()
    {
        return std::static_pointer_cast<xaiefal::XAieRsc>(
            std::make_shared<xaiefal::XAiePerfCounter>());
    }
};

class TileStub
{
public:
    TilePlStub pl() { return TilePlStub(); }

    std::shared_ptr<xaiefal::XAieRsc> sswitchPort()
    {
        return std::static_pointer_cast<xaiefal::XAieRsc>(
            std::make_shared<xaiefal::XAieSingleTileRsc>());
    }
};

class DevWrapper
{
public:
    TileStub tile(int , int )
    {
        return TileStub();
    }

    std::shared_ptr<xaiefal::XAieRsc> broadcast(
        const std::vector<XAie_LocType>& locs,
        XAie_ModuleType startMod,
        XAie_ModuleType endMod)
    {
        auto bc = std::make_shared<xaiefal::XAieBroadcast>();
        bc->setChannel(locs, startMod, endMod);
        return std::static_pointer_cast<xaiefal::XAieRsc>(bc);
    }
};

extern DevWrapper* s_pXAieDev;

int  request(const std::shared_ptr<xaiefal::XAieRsc>& rsc);
bool release(const std::shared_ptr<xaiefal::XAieRsc>& rsc);
void setReleaseResult(bool result);
void setReleaseResults(const std::vector<bool>& results);

void setRequestResults(const std::vector<int>& results);

}

namespace stub_controls {

struct PerfCounterCfg {
    bool            valid;
    XAie_LocType    loc;
    XAie_ModuleType module;
    u32             id;
    XAie_ReturnCode status;
};

struct PerfGetCfg {
    bool            valid;
    u8              col;
    u8              row;
    XAie_ModuleType module;
    u8              id;
    u32             value;
    XAie_ReturnCode status;
};

extern PerfCounterCfg g_perfCfg[16];
extern int g_perfCfgUsed;
extern PerfGetCfg g_perfGetCfg[16];
extern std::atomic<int> g_perfGetCallCount;

void ResetAll();

void FalUtil_SetRequestResults(const std::vector<int>& results);
int  FalUtil_GetRequestCallCount();
void FalUtil_SetReleaseResult(bool result);
void FalUtil_SetReleaseResults(const std::vector<bool>& results);

void XAie_SetEventSelectResult(int rc);
void XAie_SetPerfCounterControlResult(int rc);
int  XAie_GetEventSelectCallCount();
int  XAie_GetPerfCounterControlCallCount();

abr::err_code ErrorMsg_GetLastCode();
const char*   ErrorMsg_GetLastMessage();
void          ErrorMsg_Reset();

void Stub_XAiePerfCounter_Reset(void);
void Stub_XAiePerfCounter_SetGetRscIdResult(
    int            index,
    u8             col,
    u8             row,
    int            module,
    u32            id,
    XAie_ReturnCode status);

void Stub_XAie_PerfCounterGet_Reset(void);
void Stub_XAie_PerfCounterGet_SetValue(
    u8              col,
    u8              row,
    XAie_ModuleType module,
    u8              id,
    u32             value,
    XAie_ReturnCode status);
int  Stub_XAie_PerfCounterGet_GetCallCount(void);

extern XAie_DevInst g_stubDevInst;

}
