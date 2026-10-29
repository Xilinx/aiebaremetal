/**
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. A copy of the
 * License is located at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */
#ifndef AEG_VITIS_AIE_COMPAT_XAIENGINE_H
#define AEG_VITIS_AIE_COMPAT_XAIENGINE_H

#if defined(__has_include) && __has_include(<aie_codegen.h>)
#  include <aie_codegen.h>
#  include <aie_codegen_inc/xaie_routing.h>
#else
#  include_next <xaiengine.h>
#endif
#endif
