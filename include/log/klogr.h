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

#ifndef _KLOG_R_
#define _KLOG_R_
#include "kcommon.h"
#include "unistd.h"
static uint64_t _get_log_address(uint64_t inst) {
#ifdef _AIE_LOG_LOCAL_TEST_
	return AIEML_CORE_LOG_DM_REALADDR;
#else
  XAie_DevInst *g_Inst = (XAie_DevInst*)(inst);
	if (g_Inst->DevProp.DevGen == XAIE_DEV_GEN_AIE) {
		return AIEML_CORE_LOG_DM_REALADDR_AIE1;
	} else {
		return AIEML_CORE_LOG_DM_REALADDR_AIE2;
	}
#endif
}
static char* _rlog(int r, int c, uint64_t inst, Read32BitDataFunc func) {
	  uint64_t address = _get_log_address(inst);//AIEML_HOST_VIEW_DM_ADDR(r,c);
    uint32_t data;
#if !defined(__AIESIM__)
		usleep(1);
#endif
    printf("r log address = %x\n", address);
#define STR_LOG_LEN (1024*2)
    static char str_log[STR_LOG_LEN];
    char str[256] = {0};
    str_log[0] = '\0';
    size_t index = 0;
		data = func(r,c, address, inst);
#if defined(__AIESIM__)
		if (data != MAICMARK) {
			int count = 2000;
			while(count-- > 0) {
				data = func(r,c, address, inst);
				usleep(1000);
			}
		}
#endif
		printf("rlog data = %x\n", data);
		if (data != MAICMARK) {
			return str_log;
		}
		printf("data2 = %x\n", func(r,c, address + 4, inst));
		printf("data3 = %x\n", func(r,c, address + 8, inst));
	  address += sizeof(uint32_t);
    while ((data = func(r,c, address,inst))) {
	    //printf("read address is %x\n", address);
				if (index >= 256) {
          str[255] = '\0';
					printf("%s\n", str);
					if (strlen(str_log) < STR_LOG_LEN - 256) {
						strcat(str_log, str);
					}
					break;
				}
        for (size_t i = 0; i < 4; i++) {
            char c = (data >> (i * 8)) & 0xFF;
            if (c == ';') {
                str[index>=256 ? 255 : index] = '\0';
                printf("%s\n", str);
		if (strlen(str_log) < STR_LOG_LEN - 256) {
			strcat(str_log, str);
			strcat(str_log, ";");
		}
                index = 0;
		str[0] = 0;
            } else {
                str[index++] = c;
            }
        }
        address += sizeof(uint32_t);
    }
    return str_log;
}
#ifndef _AIE_LOG_LOCAL_TEST_
static uint32_t aier(int r, int c, uint64_t daddr, uint64_t inst)
{
	XAie_DevInst *g_Inst = (XAie_DevInst*)(inst);

	int row = r;
  	int col = c;
	uint32_t taddr = XAie_GetTileAddr(g_Inst, row, col)+daddr;
	uint32_t value = 0;
	
	XAie_Read32(g_Inst,taddr, &value);
	return value;
}
static char * rlog(int r, int c, uint64_t inst) {
     return _rlog(r, c, inst, aier);
}
#else
static uint32_t read32BitData(int row, int col, uint64_t address, uint64_t insnt) { 
	return *(uint32_t *)address;
}
static char * rlog(int r, int c, uint64_t inst) {
     return _rlog(r, c, inst, read32BitData);
}
#endif
#endif
