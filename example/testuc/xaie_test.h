// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xaiengine.h>
#include "../../include/hw_config.h"
#include "ucfirmware/ucheader.h"
extern "C" {
int test_aie_load_uc_elf(XAie_DevInst *DevInst);
}
