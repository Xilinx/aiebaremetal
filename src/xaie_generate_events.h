// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef XAIE_GENERATE_EVENTS_H_
#define XAIE_GENERATE_EVENTS_H_

#include <cstdint>
#include <xaiengine.h>

#include "aeg_log.h"

#ifdef __cplusplus
extern "C" {
#endif

void generate_events_on_col(XAie_DevInst *DevInst, int32_t col);
int32_t test_aie_generate_intr(XAie_DevInst *DevInst);


#define AegError AEG_ERROR
#define AegLog AEG_LOG

void AegWaitForContinue(void);

#ifdef __cplusplus
}
#endif

#endif /* XAIE_GENERATE_EVENTS_H_ */
