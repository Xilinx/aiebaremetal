#pragma once

/* Minimal stub for xaiengine.h used only in unit tests.
 * Provides trivial type declarations so that cert_host.h and
 * cert_api code can compile without the real XAIE SDK.
 *
 * This header intentionally avoids defining the real layout or
 * behavior of these types; they are only used to satisfy the compiler
 * and to allow basic in-memory mocking in unit tests.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* Basic scalar types used by the XAIE-style APIs. */
typedef uint8_t  u8;
typedef uint32_t u32;

typedef int AieRC;

/* Simple location type (row/col). */
typedef struct {
    u32 Row;
    u32 Col;
} XAie_LocType;

struct XAie_DevInst; /* forward */

/* Minimal microcode module description used by cert_host.cc. */
typedef struct XAie_UcMod {
    AieRC (*Wakeup)(struct XAie_DevInst *DevInst,
                    XAie_LocType Loc,
                    const struct XAie_UcMod *Self);
    AieRC (*GetCoreStatus)(struct XAie_DevInst *DevInst,
                           XAie_LocType Loc,
                           u32 *CoreStatus,
                           const struct XAie_UcMod *Self);
} XAie_UcMod;

/* Device properties used by cert_host.cc. */
typedef struct {
    const XAie_UcMod *UcMod;
} XAie_DevModEntry;

typedef struct {
    u32 ColShift;
    u32 NumCols;
    /* A very small table is enough for unit tests. */
    XAie_DevModEntry DevMod[4];
} XAie_DevProp;

/* Device ops used by cert_host.cc. */
typedef struct {
    u8 (*GetTTypefromLoc)(struct XAie_DevInst *DevInst, XAie_LocType Loc);
} XAie_DevOps;

/* Minimal device instance with just the fields cert_host.cc touches. */
typedef struct XAie_DevInst {
    int StartCol;
    int NumCols;
    XAie_DevProp DevProp;
    XAie_DevOps *DevOps;
} XAie_DevInst;

/* Simple memory descriptor used by XAie_Mem* helpers. */
typedef struct XAie_MemInst {
    void    *vaddr;
    uintptr_t devAddr;
    size_t   size;
} XAie_MemInst;

/* Status and memory flags. Values are not important for tests. */
#define XAIE_OK               0
#define XAIE_MEM_NONCACHEABLE 0u

/* Memory management helpers (stubbed in xaiengine.c). */
XAie_MemInst *XAie_MemAllocate(XAie_DevInst *DevInst, size_t size, u32 flags);
void          XAie_MemFree(XAie_MemInst *mem);
void         *XAie_MemGetVAddr(XAie_MemInst *mem);
uintptr_t     XAie_MemGetDevAddr(XAie_MemInst *mem);
AieRC         XAie_MemSyncForCPU(XAie_MemInst *mem);
AieRC         XAie_MemSyncForDev(XAie_MemInst *mem);

/* Data movement / microcode helpers used by cert_host.cc. */
AieRC XAie_DataMemBlockWrite(XAie_DevInst *DevInst,
                             XAie_LocType Loc,
                             u32 Addr,
                             const void *Data,
                             size_t Size);

AieRC XAie_LoadUcMem(XAie_DevInst *DevInst,
                     XAie_LocType Loc,
                     const void *ElfData);

#ifdef __cplusplus
}
#endif
