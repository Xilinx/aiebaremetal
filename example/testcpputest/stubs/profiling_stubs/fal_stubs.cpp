// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "fal_stubs.h"
#include "aeg_api_message.h"
#include <atomic>

namespace xaiefal {
    int XAiePerfCounter::s_instanceCounter = 0;

    int XAiePerfCounter::getRscId(XAie_LocType& loc, XAie_ModuleType& module, uint32_t& id) noexcept
    {

        if (m_instanceIndex >= 0 && m_instanceIndex < stub_controls::g_perfCfgUsed) {
            const auto& cfg = stub_controls::g_perfCfg[m_instanceIndex];
            if (cfg.valid) {
                loc = cfg.loc;
                module = cfg.module;
                id = cfg.id;
                return cfg.status;
            }
        }

        loc = XAie_TileLoc(0, 0);
        module = XAIE_PL_MOD;
        id = 0;
        return XAIE_OK;
    }
}

namespace stub_controls {
    extern int g_perfCfgUsed;
    extern PerfCounterCfg g_perfCfg[16];
}

namespace abr {

static std::atomic<err_code> g_lastErrCode{};
static char                  g_lastErrMsg[128];

namespace config_manager {

void* s_pDevInst = nullptr;
}

}

namespace stub_controls {

XAie_DevInst g_stubDevInst = {
    .BaseAddr = 0x20000000000ULL,
    .StartCol = 0,
    .NumRows = 9,
    .NumCols = 50,
    .ShimRow = 0,
    .MemTileRowStart = 1,
    .MemTileNumRows = 1,
    .AieTileRowStart = 2,
    .AieTileNumRows = 6,
    .IsReady = 1,
    .IsProd = 0,
    .EccStatus = 0
};

}

abr::err_code errorMsg(abr::err_code code, const char* msg)
{
    abr::g_lastErrCode.store(code, std::memory_order_relaxed);
    if (msg != nullptr) {
        std::size_t len = 0;
        while (msg[len] != '\0' && len + 1 < sizeof(abr::g_lastErrMsg)) {
            abr::g_lastErrMsg[len] = msg[len];
            ++len;
        }
        abr::g_lastErrMsg[len] = '\0';
    } else {
        abr::g_lastErrMsg[0] = '\0';
    }
    return code;
}

abr::err_code errorMsg(abr::err_code code, std::string msg)
{
    return errorMsg(code, msg.c_str());
}

void debugMsg(const char* )
{

}

void debugMsg(std::string )
{

}

void infoMsg(std::string )
{

}

namespace fal_util {

DevWrapper* s_pXAieDev = nullptr;

std::atomic<int> g_requestCallCount{0};
static int         g_requestResults[8];
static std::size_t g_requestResultsCount = 0U;

int request(std::shared_ptr<xaiefal::XAieRsc> )
{
    int idx = g_requestCallCount.fetch_add(1, std::memory_order_relaxed);
    if (idx >= 0 && static_cast<std::size_t>(idx) < g_requestResultsCount)
        return g_requestResults[static_cast<std::size_t>(idx)];
    return 0;
}

static bool g_releaseResult = true;
static bool g_releaseResultsSeq[8] = {true,true,true,true,true,true,true,true};
static std::size_t g_releaseResultsCount = 0;
static std::atomic<int> g_releaseCallIdx{0};

bool release(std::shared_ptr<xaiefal::XAieRsc> )
{
    if (g_releaseResultsCount == 0U)
        return g_releaseResult;
    int idx = g_releaseCallIdx.fetch_add(1, std::memory_order_relaxed);
    if (idx < 0 || static_cast<std::size_t>(idx) >= g_releaseResultsCount)
        return g_releaseResultsSeq[g_releaseResultsCount - 1U];
    return g_releaseResultsSeq[static_cast<std::size_t>(idx)];
}

void setReleaseResult(bool result)
{
    g_releaseResult = result;
    g_releaseResultsCount = 0;
    g_releaseCallIdx.store(0, std::memory_order_relaxed);
}

void setReleaseResults(const std::vector<bool>& results)
{
    g_releaseResultsCount = (results.size() < 8U) ? results.size() : 8U;
    for (std::size_t i = 0; i < g_releaseResultsCount; ++i)
        g_releaseResultsSeq[i] = results[i];
    g_releaseCallIdx.store(0, std::memory_order_relaxed);
}

void setRequestResults(const std::vector<int>& results)
{
    g_requestResultsCount = (results.size() < 8U) ? results.size() : 8U;
    for (std::size_t i = 0; i < g_requestResultsCount; ++i)
        g_requestResults[i] = results[i];
}

}

namespace stub_controls {

using abr::err_code;

static std::atomic<err_code> g_errCode{static_cast<err_code>(0)};
static char                  g_errMsg[128];

std::atomic<int> g_evtSelectCalls{0};
std::atomic<int> g_perfCtrlCalls{0};
std::atomic<int> g_evtSelectResult{0};
std::atomic<int> g_perfCtrlResult{0};

PerfCounterCfg g_perfCfg[16];
int            g_perfCfgUsed = 0;

PerfGetCfg       g_perfGetCfg[16];
std::atomic<int> g_perfGetCallCount{0};

void ResetAll()
{
    g_errCode.store(static_cast<err_code>(0), std::memory_order_relaxed);
    g_errMsg[0] = '\0';
    g_evtSelectCalls.store(0, std::memory_order_relaxed);
    g_perfCtrlCalls.store(0, std::memory_order_relaxed);
    g_evtSelectResult.store(0, std::memory_order_relaxed);
    g_perfCtrlResult.store(0, std::memory_order_relaxed);
    fal_util::g_requestCallCount.store(0, std::memory_order_relaxed);
    fal_util::setRequestResults({});
    fal_util::setReleaseResult(true);

    xaiefal::XAiePerfCounter::resetInstanceCounter();
    g_perfCfgUsed = 0;
    for (auto &c : g_perfCfg) {
        c.valid  = false;
        c.loc    = XAie_TileLoc(0, 0);
        c.module = XAIE_PL_MOD;
        c.id     = 0u;
        c.status = XAIE_OK;
    }

    g_perfGetCallCount.store(0, std::memory_order_relaxed);
    for (auto &c : g_perfGetCfg) {
        c.valid  = false;
        c.col    = 0;
        c.row    = 0;
        c.module = XAIE_PL_MOD;
        c.id     = 0;
        c.value  = 0u;
        c.status = XAIE_OK;
    }
}

void FalUtil_SetRequestResults(const std::vector<int>& results)
{
    fal_util::setRequestResults(results);
}

int FalUtil_GetRequestCallCount()
{
    return fal_util::g_requestCallCount.load(std::memory_order_relaxed);
}

void FalUtil_SetReleaseResult(bool result)
{
    fal_util::setReleaseResult(result);
}

void FalUtil_SetReleaseResults(const std::vector<bool>& results)
{
    fal_util::setReleaseResults(results);
}

void XAie_SetEventSelectResult(int rc)
{
    g_evtSelectResult.store(rc, std::memory_order_relaxed);
}

void XAie_SetPerfCounterControlResult(int rc)
{
    g_perfCtrlResult.store(rc, std::memory_order_relaxed);
}

int XAie_GetEventSelectCallCount()
{
    return g_evtSelectCalls.load(std::memory_order_relaxed);
}

int XAie_GetPerfCounterControlCallCount()
{
    return g_perfCtrlCalls.load(std::memory_order_relaxed);
}

abr::err_code ErrorMsg_GetLastCode()
{
    return g_errCode.load(std::memory_order_relaxed);
}

const char* ErrorMsg_GetLastMessage()
{
    return g_errMsg;
}

void ErrorMsg_Reset()
{
    g_errCode.store(static_cast<err_code>(0), std::memory_order_relaxed);
    g_errMsg[0] = '\0';
}

void Stub_XAiePerfCounter_Reset(void)
{
    g_perfCfgUsed = 0;

    xaiefal::XAiePerfCounter::resetInstanceCounter();
    for (auto &c : g_perfCfg) {
        c.valid  = false;
        c.loc    = XAie_TileLoc(0, 0);
        c.module = XAIE_PL_MOD;
        c.id     = 0u;
        c.status = XAIE_OK;
    }
}

void Stub_XAiePerfCounter_SetGetRscIdResult(
    int            index,
    u8             col,
    u8             row,
    int            module,
    u32            id,
    XAie_ReturnCode status)
{
    if (index >= 0 && index < 16) {
        g_perfCfg[index].valid  = true;
        g_perfCfg[index].loc    = XAie_TileLoc(col, row);
        g_perfCfg[index].module = static_cast<XAie_ModuleType>(module);
        g_perfCfg[index].id     = id;
        g_perfCfg[index].status = status;
        if (index >= g_perfCfgUsed)
            g_perfCfgUsed = index + 1;
    }
}

void Stub_XAie_PerfCounterGet_Reset(void)
{
    g_perfGetCallCount.store(0, std::memory_order_relaxed);
    for (auto &c : g_perfGetCfg) {
        c.valid  = false;
        c.col    = 0;
        c.row    = 0;
        c.module = XAIE_PL_MOD;
        c.id     = 0;
        c.value  = 0u;
        c.status = XAIE_OK;
    }
}

void Stub_XAie_PerfCounterGet_SetValue(
    u8              col,
    u8              row,
    XAie_ModuleType module,
    u8              id,
    u32             value,
    XAie_ReturnCode status)
{

    for (int i = 0; i < 16; ++i) {
        if (!g_perfGetCfg[i].valid ||
            (g_perfGetCfg[i].col == col &&
             g_perfGetCfg[i].row == row &&
             g_perfGetCfg[i].module == module &&
             g_perfGetCfg[i].id == id)) {
            g_perfGetCfg[i].valid  = true;
            g_perfGetCfg[i].col    = col;
            g_perfGetCfg[i].row    = row;
            g_perfGetCfg[i].module = module;
            g_perfGetCfg[i].id     = id;
            g_perfGetCfg[i].value  = value;
            g_perfGetCfg[i].status = status;
            break;
        }
    }
}

int Stub_XAie_PerfCounterGet_GetCallCount(void)
{
    return g_perfGetCallCount.load(std::memory_order_relaxed);
}

}
