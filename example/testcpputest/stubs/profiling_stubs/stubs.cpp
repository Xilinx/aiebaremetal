#include "stubs.h"
#include "fal_stubs.h"
#include <string>
#include <memory>
#include <atomic>
#include "CppUTestExt/MockSupport_c.h"

// External control variables from stub_controls namespace in fal_stubs.cpp
namespace stub_controls {
    extern std::atomic<int> g_evtSelectCalls;
    extern std::atomic<int> g_perfCtrlCalls;
    extern std::atomic<int> g_evtSelectResult;
    extern std::atomic<int> g_perfCtrlResult;
    extern PerfGetCfg g_perfGetCfg[16];
    extern std::atomic<int> g_perfGetCallCount;
}

// Local control for reset functions
namespace {
    std::atomic<int> g_perfCounterResetResult{XAIE_OK};
    std::atomic<int> g_perfCounterResetCtrlResult{XAIE_OK};
    std::atomic<int> g_eventSelectStrmPortResetResult{XAIE_OK};
    std::atomic<int> g_eventPCResetResult{XAIE_OK};
}

XAie_ReturnCode XAie_PerfCounterReset(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              counterId)
{
    (void)devInst;
    (void)loc;
    (void)module;
    (void)counterId;
    return static_cast<XAie_ReturnCode>(g_perfCounterResetResult.load());
}
XAie_ReturnCode XAie_EventPCReset(
    void*        devInst,
    XAie_LocType loc,
    u8           pcId)
{
    (void)devInst;
    (void)loc;
    (void)pcId;
    return static_cast<XAie_ReturnCode>(g_eventPCResetResult.load());
}
XAie_ReturnCode XAie_PerfCounterResetControlReset(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              counterId)
{
    (void)devInst;
    (void)loc;
    (void)module;
    (void)counterId;
    return static_cast<XAie_ReturnCode>(g_perfCounterResetCtrlResult.load());
}
XAie_ReturnCode XAie_EventSelectStrmPortReset(
    void*        devInst,
    XAie_LocType loc,
    u8           portId)
{
    (void)devInst;
    (void)loc;
    (void)portId;
    return static_cast<XAie_ReturnCode>(g_eventSelectStrmPortResetResult.load());
}

XAie_ReturnCode XAie_EventGenerate(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    XAie_Events     event)
{
    (void)devInst;
    (void)loc;
    (void)module;
    (void)event;
    return XAIE_OK;
}

XAie_ReturnCode XAie_PerfCounterEventValueSet(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              counterId,
    u32             value)
{
    (void)devInst;
    (void)loc;
    (void)module;
    (void)counterId;
    (void)value;
    return XAIE_OK;
}

XAie_ReturnCode XAie_EventBroadcastBlockMapDir(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    XAie_Events     switchEvent,
    u8              broadcastId,
    u32             directionsMask)
{
    (void)devInst;
    (void)loc;
    (void)module;
    (void)switchEvent;
    (void)broadcastId;
    (void)directionsMask;
    return XAIE_OK;
}

XAie_ReturnCode XAie_EventBroadcast(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              broadcastId,
    XAie_Events     event)
{
    (void)devInst;
    (void)loc;
    (void)module;
    (void)broadcastId;
    (void)event;
    return XAIE_OK;
}

// Add XAie_PerfCounterGet stub
XAie_ReturnCode XAie_PerfCounterGet(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              counterId,
    u32*            value)
{
    (void)devInst;

    stub_controls::g_perfGetCallCount.fetch_add(1, std::memory_order_relaxed);

    // Search for matching configuration
    for (int i = 0; i < 16; ++i) {
        if (stub_controls::g_perfGetCfg[i].valid &&
            stub_controls::g_perfGetCfg[i].col == loc.Col &&
            stub_controls::g_perfGetCfg[i].row == loc.Row &&
            stub_controls::g_perfGetCfg[i].module == module &&
            stub_controls::g_perfGetCfg[i].id == counterId) {
            if (value) {
                *value = stub_controls::g_perfGetCfg[i].value;
            }
            return stub_controls::g_perfGetCfg[i].status;
        }
    }

    // Default if not configured
    if (value) *value = 0;
    return XAIE_OK;
}

XAie_ReturnCode XAie_EventBroadcastReset(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              broadcastId)
{
    (void)devInst;
    (void)loc;
    (void)module;
    (void)broadcastId;
    return XAIE_OK;
}

XAie_ReturnCode XAie_EventBroadcastUnblockDir(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    XAie_Events     switchEvent,
    u8              broadcastId,
    XAie_Events     directionsMask)
{
    (void)devInst;
    (void)loc;
    (void)module;
    (void)switchEvent;
    (void)broadcastId;
    (void)directionsMask;
    return XAIE_OK;
}

XAie_ReturnCode XAie_EventSelectStrmPort(
    void*              devInst,
    XAie_LocType       loc,
    u8                 eventPortId,
    XAie_StrmSwPortType slaveOrMaster,
    XAie_Direction      dir,
    u8                 streamPortId)
{
    (void)devInst;
    (void)loc;
    (void)eventPortId;
    (void)slaveOrMaster;
    (void)dir;
    (void)streamPortId;
    stub_controls::g_evtSelectCalls.fetch_add(1, std::memory_order_relaxed);
    return static_cast<XAie_ReturnCode>(stub_controls::g_evtSelectResult.load(std::memory_order_relaxed));
}

XAie_ReturnCode XAie_PerfCounterControlSet(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              counterId,
    XAie_Events     startEvent,
    XAie_Events     stopEvent)
{
    (void)devInst;
    (void)loc;
    (void)module;
    (void)counterId;
    (void)startEvent;
    (void)stopEvent;
    stub_controls::g_perfCtrlCalls.fetch_add(1, std::memory_order_relaxed);
    return static_cast<XAie_ReturnCode>(stub_controls::g_perfCtrlResult.load(std::memory_order_relaxed));
}

// Stub control setter functions
void XAie_SetPerfCounterResetResult(int result) {
    g_perfCounterResetResult.store(result, std::memory_order_relaxed);
}

void XAie_SetPerfCounterResetCtrlResult(int result) {
    g_perfCounterResetCtrlResult.store(result, std::memory_order_relaxed);
}

void XAie_SetEventSelectStrmPortResetResult(int result) {
    g_eventSelectStrmPortResetResult.store(result, std::memory_order_relaxed);
}

void XAie_SetEventPCResetResult(int result) {
    g_eventPCResetResult.store(result, std::memory_order_relaxed);
}
