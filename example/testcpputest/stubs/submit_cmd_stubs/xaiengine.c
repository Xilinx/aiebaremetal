#include "xaiengine.h"
#include <stdlib.h>
#include <string.h>

/*
 * XAIE engine low-level stubs used by cert_host.cc.
 *
 * This file provides basic hardware-style helpers but deliberately avoids any
 * complex behaviour. Memory allocation and completion control for unit tests
 * are implemented in stub_xaie_mem.c.
 */

/*
 * NOTE: XAie_MemAllocate / XAie_MemFree / XAie_MemGetVAddr /
 * XAie_MemGetDevAddr / XAie_MemSyncForCPU / XAie_MemSyncForDev /
 * XAie_DataMemBlockWrite / XAie_LoadUcMem / Stub_XAie_CreateMinimalDevInst
 * are implemented in stub_xaie_mem.c so that tests can control completion behaviour.
 * Do NOT reimplement them here to avoid multiple-definition link errors.
 */

/* -- */
/* Minimal DevInst/DevOps/UcMod wiring for cert_api tests                     */
/* -- */

/*
 * cert_host::cert_api relies on XAie_DevInst::DevOps and DevProp.DevMod[*].UcMod
 * to call into GetTTypefromLoc(), Wakeup(), and GetCoreStatus().
 *
 * For unit tests we provide a single, process-local XAie_DevInst whose
 * callbacks are all safe no-ops. This prevents run_cert() from touching
 * any real hardware state while still exercising the submit_* APIs.
 */

static u8 Stub_GetTTypefromLoc(XAie_DevInst *DevInst, XAie_LocType Loc)
{
    (void)DevInst;
    (void)Loc;
    /* Single tile type 0 is sufficient for tests. */
    return (u8)0;
}

static AieRC Stub_UcWakeup(XAie_DevInst *DevInst,
                           XAie_LocType Loc,
                           const XAie_UcMod *UcMod)
{
    (void)DevInst;
    (void)Loc;
    (void)UcMod;
    return XAIE_OK;
}

static AieRC Stub_UcGetCoreStatus(XAie_DevInst *DevInst,
                                  XAie_LocType Loc,
                                  u32 *CoreStatus,
                                  const XAie_UcMod *UcMod)
{
    (void)DevInst;
    (void)Loc;
    (void)UcMod;
    if (CoreStatus)
        *CoreStatus = 0u;
    return XAIE_OK;
}

/*
 * NOTE: Stub_XAie_CreateMinimalDevInst is implemented in stub_xaie_mem.c
 * to avoid multiple definitions.
 */
