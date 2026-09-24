/**<!--
 * Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
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
 * -->
 */
#ifndef MAIN_RTS_IO_H
#define MAIN_RTS_IO_H
#include "aeg_log.h"
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#define ess_Write32 ess_local_Write32
#define ess_Read32 ess_local_Read32

/* FIXME
     for the weak declare, the function name and the type should be exactly same with the
     real target app/lib which implement the function.
     in this example the ess_* function will get implement in a APP compiled by aie compiler
     and the type is defined in "aietools/include/adf/window/types.h", we hardcode the uint
     and uin64 with the same of the adf header declared, and need to get changed into reference
     from the adf type.h

     this workground is used to fix the weak declare not work issue, which caused by that we
     use uint64_t as parameter type but, the real App used the "unsigned long long", although
     the size is all is 8bytes, but the type still is different, then when libxaiedriver call
     the ess_* function from simulator backend, the App api not get triggered, and always be
     the weak one get triggered

     the weak delcare is used as workground  to compile libxaiederiver as simulator backend then
     used inside the simulator to write the axi-mm
*/
/*
     as the lib use the main app api, need to enable -rdynamic when using ld/linker, to ask lnker
     to check all symbol, without this the weak functon will keep get called , instead of the
     strong function in main app
*/
extern void *handle;
typedef void (*essw_t)(unsigned long long Addr, u_int32_t Data);
typedef u_int32_t (*essr_t)(unsigned long long Addr);
void *ess_Write32_Ptr = NULL;
void *ess_Read32_Ptr = NULL;

#define PS_SO "ps_i"
static void load_lib() {
    return;
}
static void ess_local_Write32(unsigned long long Addr, u_int32_t Data) {
    load_lib();
    if (ess_Write32_Ptr) {
        ((essw_t)ess_Write32_Ptr)(Addr, Data);
    } else {
    }
}
static u_int32_t ess_local_Read32(unsigned long long Addr) {
    load_lib();
    u_int32_t value = 0;
    if (ess_Read32_Ptr) {
        value = ((essr_t)ess_Read32_Ptr)(Addr);
    } else {
    }
    return value;
}

__attribute__((weak)) void ess_WriteCmd(unsigned char Command, unsigned char ColId, unsigned char RowId,
                                        unsigned int CmdWd0, unsigned int CmdWd1, unsigned char *CmdStr) {
    AEG_LOG("weak ess_WriteCmd\n");
}
#endif
