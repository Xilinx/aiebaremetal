/******************************************************************************
* Copyright (C) 2019 - 2020 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
* @file xaie_load_ucelf.c
* @{
*
* This file contains the test application of data memory check.
*
* This application write random integers to first 1024 bytes of data memory of
* all valid tiles and reads the data back to check the basic functioning of
* data memory.
*
******************************************************************************/

/***************************** Include Files *********************************/
#include "xaie_test_all.h"
#include "xaiemem.h"
#include <iostream>
#include <cstring>
#include <cassert>

//#define __AIESIM__ 1
#ifdef __AIESIM__
#include <dlfcn.h>

int test_compare_lib_load(const char* lib_path, const char* name) {

    char line[1024];
    FILE *fp = fopen("/proc/self/maps", "r");
    if (fp == NULL) {
        perror("fopen");
        return 0;
    }
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, name)) {
            char *path_start = strchr(line, '/');
            if (path_start != NULL) {
                strtok(path_start, "\n");
                int len = strlen(path_start);
                if (len > 2 && strcmp(&path_start[len - 2], "so") == 0) {
                    printf("Found library at: %s\n", path_start);
                    
                    // check handle is already loaded
                    void *dlopen_addr = dlsym(RTLD_DEFAULT, "dlopen");

                    if (dlopen_addr != NULL) {
                        printf("dlopen is already loaded.\n");
                        //check path is correct
												int lib_path_len = strlen(lib_path);
												//lib_path is like ./Work/ps/c_rts/ps_22.so, compare with path start from /Work
                        assert(std::strcmp(&path_start[(len - lib_path_len) + 1], &lib_path[1]) == 0);
                        return 0;  // Exit since dlopen is already loaded
                    } else {
                        void* handle = dlopen(path_start, RTLD_LAZY);
                        if (!handle) {
                            fprintf(stderr, "%s\n", dlerror());
                        }
                    }
                }
                break;
            }
        }  
    }
    fclose(fp); 
    return 0;
}



#endif
