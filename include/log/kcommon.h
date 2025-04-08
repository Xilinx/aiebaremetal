/**
* Copyright (C) 2025 Advanced Micro Devices, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#ifndef _K_LOG_COMMON_
#define _K_LOG_COMMON_
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
//#include "../kconfig.h"
#define LOG_MAX_LEN (4*1024)
typedef uint32_t (*Read32BitDataFunc)(int row, int col, uint64_t address,uint64_t inst);
//void wlog(const char *format, ...);
//void rlog(int r, int c, uint64_t inst, Read32BitDataFunc func);
static uint32_t logmarked = 0;
static uint32_t log_offset = 0;
//static size_t log_offset = 0;
#define MAICMARK 0XDEADFEEB

#ifdef _AIE_LOG_LOCAL_TEST_
static char glog[LOG_MAX_LEN];
#undef AIEML_CORE_DM_CORE_VIEW_LOG_ADDR
#undef AIEML_CORE_LOG_DM_REALADDR
#define AIEML_CORE_DM_CORE_VIEW_LOG_ADDR glog
//#define AIEML_HOST_VIEW_DM_ADDR(r,c) glog
#define AIEML_CORE_LOG_DM_REALADDR glog

#else
#define AIEML_CORE_LOG_DM_REALADDR_AIE1  (0x8000 - LOG_MAX_LEN)
#define AIEML_CORE_LOG_DM_REALADDR_AIE2  (0x10000 - LOG_MAX_LEN)

#if __AIE_ARCH__==10
#define AIEML_CORE_DM_CORE_VIEW_BASE 0x20000
#define AIE_CORE_DM_SIZE (32*1024)
#define AIEML_CORE_LOG_DM_REALADDR   AIEML_CORE_LOG_DM_REALADDR_AIE1
#else
#define AIEML_CORE_DM_CORE_VIEW_BASE 0x40000
#define AIE_CORE_DM_SIZE (64*1024)
#define AIEML_CORE_LOG_DM_REALADDR   AIEML_CORE_LOG_DM_REALADDR_AIE2
#endif

#define AIE_CORE_DM_CORE_VIEW_WEST_BASE (AIEML_CORE_DM_CORE_VIEW_BASE + (1*AIE_CORE_DM_SIZE))
#define AIE_CORE_DM_CORE_VIEW_EAST_BASE (AIEML_CORE_DM_CORE_VIEW_BASE + (3*AIE_CORE_DM_SIZE))

#define AIEML_CORE_DM_CORE_VIEW_LOAL_BASE (AIEML_CORE_DM_CORE_VIEW_BASE + (3*AIE_CORE_DM_SIZE))
#define AIEML_CORE_DM_CORE_VIEW_LOG_ADDR (AIEML_CORE_DM_CORE_VIEW_LOAL_BASE+AIEML_CORE_LOG_DM_REALADDR)
#define AIE_CORE_DM_CORE_VIEW_LOG_ADDR_WEST (AIE_CORE_DM_CORE_VIEW_WEST_BASE+AIEML_CORE_LOG_DM_REALADDR)
#define AIE_CORE_DM_CORE_VIEW_LOG_ADDR_EAST (AIE_CORE_DM_CORE_VIEW_EAST_BASE+AIEML_CORE_LOG_DM_REALADDR)
#endif

static uint64_t alignAddress(uint64_t addr) {
	uint64_t alignedAddr = (addr + 3) & ~3; 
	return alignedAddr;
}

static void wlog_reset() {
	log_offset = 0;
	logmarked = 0;
}

#endif
