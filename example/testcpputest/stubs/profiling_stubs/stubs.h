#ifndef STUB_PROFILING_STUBS_H
#define STUB_PROFILING_STUBS_H

#include <vector>
#include <memory>
#include <cstdint>
#include <string>
#include "xaiengine/xaiegbl.h"

#ifdef __cplusplus
extern "C" {
#endif

XAie_ReturnCode XAie_EventBroadcastReset(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              broadcastId);

XAie_ReturnCode XAie_EventBroadcastUnblockDir(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    XAie_Events     switchEvent,
    u8              broadcastId,
    XAie_Events     directionsMask);
XAie_ReturnCode XAie_EventSelectStrmPort(
    void*              devInst,
    XAie_LocType       loc,
    u8                 eventPortId,
    XAie_StrmSwPortType slaveOrMaster,
    XAie_Direction      dir,
    u8                 streamPortId);
    XAie_ReturnCode XAie_PerfCounterControlSet(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              counterId,
    XAie_Events     startEvent,
    XAie_Events     stopEvent);

XAie_ReturnCode XAie_PerfCounterGet(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              counterId,
    u32*            value);

XAie_ReturnCode XAie_EventGenerate(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    XAie_Events     event);

XAie_ReturnCode XAie_PerfCounterEventValueSet(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              counterId,
    u32             value);

XAie_ReturnCode XAie_EventBroadcastBlockMapDir(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    XAie_Events     switchEvent,
    u8              broadcastId,
    u32             directionsMask);

XAie_ReturnCode XAie_EventBroadcast(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              broadcastId,
    XAie_Events     event);
XAie_ReturnCode XAie_PerfCounterReset(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              counterId);
XAie_ReturnCode XAie_PerfCounterResetControlReset(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              counterId);
XAie_ReturnCode XAie_EventSelectStrmPortReset(
    void*        devInst,
    XAie_LocType loc,
    u8           portId);
XAie_ReturnCode XAie_EventPCReset(
    void*        devInst,
    XAie_LocType loc,
    u8           pcId);

// Stub control functions
void XAie_SetPerfCounterResetResult(int result);
void XAie_SetPerfCounterResetCtrlResult(int result);
void XAie_SetEventSelectStrmPortResetResult(int result);
void XAie_SetEventPCResetResult(int result);

#ifdef __cplusplus
}
#endif

#endif // STUB_PROFILING_STUBS_H