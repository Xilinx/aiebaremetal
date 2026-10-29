// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef STUB_PROFILING_STUBS_H
#define STUB_PROFILING_STUBS_H

#include <vector>
#include <memory>
#include <cstdint>
#include <string>
#include "xaiengine/xaiegbl.h"

#ifdef __cplusplus
#define PROFILING_STUB_NOEXCEPT noexcept
extern "C" {
#else
#define PROFILING_STUB_NOEXCEPT
#endif

XAie_ReturnCode XAie_EventBroadcastReset(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              broadcastId) PROFILING_STUB_NOEXCEPT;

XAie_ReturnCode XAie_EventBroadcastUnblockDir(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    XAie_Events     switchEvent,
    u8              broadcastId,
    XAie_Events     directionsMask) PROFILING_STUB_NOEXCEPT;
XAie_ReturnCode XAie_EventSelectStrmPort(
    void*              devInst,
    XAie_LocType       loc,
    u8                 eventPortId,
    XAie_StrmSwPortType slaveOrMaster,
    XAie_Direction      dir,
    u8                 streamPortId) PROFILING_STUB_NOEXCEPT;
    XAie_ReturnCode XAie_PerfCounterControlSet(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              counterId,
    XAie_Events     startEvent,
    XAie_Events     stopEvent) PROFILING_STUB_NOEXCEPT;

XAie_ReturnCode XAie_PerfCounterGet(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              counterId,
    u32*            value) PROFILING_STUB_NOEXCEPT;

XAie_ReturnCode XAie_EventGenerate(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    XAie_Events     event) PROFILING_STUB_NOEXCEPT;

XAie_ReturnCode XAie_PerfCounterEventValueSet(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              counterId,
    u32             value) PROFILING_STUB_NOEXCEPT;

XAie_ReturnCode XAie_EventBroadcastBlockMapDir(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    XAie_Events     switchEvent,
    u8              broadcastId,
    u32             directionsMask) PROFILING_STUB_NOEXCEPT;

XAie_ReturnCode XAie_EventBroadcast(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              broadcastId,
    XAie_Events     event) PROFILING_STUB_NOEXCEPT;
XAie_ReturnCode XAie_PerfCounterReset(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              counterId) PROFILING_STUB_NOEXCEPT;
XAie_ReturnCode XAie_PerfCounterResetControlReset(
    void*           devInst,
    XAie_LocType    loc,
    XAie_ModuleType module,
    u8              counterId) PROFILING_STUB_NOEXCEPT;
XAie_ReturnCode XAie_EventSelectStrmPortReset(
    void*        devInst,
    XAie_LocType loc,
    u8           portId) PROFILING_STUB_NOEXCEPT;
XAie_ReturnCode XAie_EventPCReset(
    void*        devInst,
    XAie_LocType loc,
    u8           pcId) PROFILING_STUB_NOEXCEPT;

void XAie_SetPerfCounterResetResult(int result);
void XAie_SetPerfCounterResetCtrlResult(int result);
void XAie_SetEventSelectStrmPortResetResult(int result);
void XAie_SetEventPCResetResult(int result);

#ifdef __cplusplus
}
#endif

#undef PROFILING_STUB_NOEXCEPT

#endif
