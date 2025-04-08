/**
* Copyright (C) 2024 Advanced Micro Devices, Inc.
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
#ifndef MAIN_RTS_IO_H
#define MAIN_RTS_IO_H
//#include <stdint.h>
//#define __USE_GNU
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>
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
///proj/xbuilds/9999.0_INT_1220_2214/installs/lin64/Vitis/2024.1/aietools/include/adf/window/types.h
//typedef u_int32_t uint;
//typedef unsigned long long uint64;
/*
void ess_Write32(unsigned long long Addr, u_int32_t Data);
u_int32_t ess_Read32(unsigned long long Addr);
void ess_WriteCmd(unsigned char Command, unsigned char ColId, unsigned char RowId, unsigned int CmdWd0, unsigned int CmdWd1, unsigned char *CmdStr);
//*/
///*
extern void* handle;
typedef void (*essw_t)(unsigned long long Addr, u_int32_t Data);
typedef u_int32_t (*essr_t)(unsigned long long Addr);
void* ess_Write32_Ptr = NULL;
void* ess_Read32_Ptr = NULL;

//const char* pslib = "/scratch/staff/huaj/baremetal/aiebaremetal/testkernel/Work/ps/c_rts/systemC/generated-objects/ps_i3.so";
/*
	 the ess* function implemented by ps_i3.so library by aiecompiler, for some reason, the aiedriver can not find 
	 these, even the ps_i3.so already loaded, the workground is to dlopen the library again, the lib is a generated file
	 the name may change future, need to keep maintain it.
*/
#define PS_SO "ps_i"
static void load_lib() {
	// if (!ess_Write32_Ptr || !ess_Read32_Ptr) {
	// 	printf("load_lib----\n");
	// 	FILE *fp;
	// 	char line[1024];
	// 	void *handle = NULL;
	// 	// the maps solution need call adf::graph.init() to make ps_ix.so get
	// 	// loaded, if in the graph code not call graph.init, need to manually
	// 	// give the lib path and load it.
	// 	if (0) {
	// 		fp = fopen("/proc/self/maps", "r");
	// 		if (fp == NULL) {
	// 			perror("fopen");
	// 			return;
	// 		}
	// 		while (fgets(line, sizeof(line), fp)) {
	// 			//printf("line is %s\n", line);
	// 			if (strstr(line, name)) {
	// 				char *path_start = strchr(line, '/');
	// 				if (path_start != NULL) {
	// 					strtok(path_start, "\n");
	// 					int len = strlen(path_start);
	// 					if (len > 2 && strcmp(&path_start[len - 2], "so") == 0) {
	// 						printf("Found library at: %s\n", path_start);
	// 						handle = dlopen(path_start, RTLD_LAZY);
	// 						if (!handle) {
	// 							fprintf(stderr, "%s\n", dlerror());
	// 						}
	// 					}
	// 					break;
	// 				}
	// 			}
	// 		}
	// 		fclose(fp);
	// 	} else {
	// 		  //fix me , this is a worground ,should read  ./Work/config/scsim_config.json
	// 		  // to get the ps lib name
	// 			char curdir[256];
	// 			char *path = getenv("AIE_COMPILE_WORK");
	// 			if (path != NULL) {
	// 				const char* workpsdir = "/ps/c_rts/systemC/generated-objects/";
	// 				char lib[256];
	// 				for (int i = 0; i < 32; i++) {
	// 					snprintf(lib, sizeof(lib), "%s/%s/ps_i%d.so", path, workpsdir, i);
	// 					handle = dlopen(lib, RTLD_LAZY);
	// 					printf("opened: %s\n", handle);
	// 					if (handle == NULL) {
	// 						    printf("Error: %s\n",  dlerror());
	// 					} else {
	// 						break;
	// 					}
	// 				}
	// 			}
	// 	}
		/*
		if (handle) {
			ess_Write32_Ptr = (essw_t)dlsym(handle, "ess_Write32");
			ess_Read32_Ptr = (essr_t)dlsym(handle, "ess_Read32");
		} else {
				//if not defined the handle which should point to aiesimulator library, load symbol from current app sapce
				ess_Write32_Ptr = (essw_t)dlsym(NULL, "ess_Write32");
				ess_Read32_Ptr = (essr_t)dlsym(NULL, "ess_Read32");
		}
		*/
	// }
	return;
	
}
static void ess_local_Write32(unsigned long long Addr, u_int32_t Data){
	load_lib();
	//printf("ess_local_Write32\n");
	if (ess_Write32_Ptr) {
		((essw_t)ess_Write32_Ptr)(Addr, Data);
		//printf(" write ptr 0x%p data %x\n", Addr, Data);
	}else {
		//printf("not find write ptr failed\n");
	}


}
static u_int32_t ess_local_Read32(unsigned long long Addr) {
	load_lib();
	u_int32_t value = 0;
	if (ess_Read32_Ptr) {
		value =  ((essr_t)ess_Read32_Ptr)(Addr);
		//printf("value %x in Addr %x\n",value, Addr);
	} else {
		//printf("2read failed, not find read ptr, opend failed\n");
	}
	//printf("local ess_Read32_\n");
	return value;
}

__attribute__((weak)) void ess_WriteCmd(unsigned char Command, unsigned char ColId, unsigned char RowId, unsigned int CmdWd0, unsigned int CmdWd1, unsigned char *CmdStr) {
	printf("weak ess_WriteCmd\n");
}
//*/
#endif
