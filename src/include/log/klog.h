// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef _K_LOG_
#define _K_LOG_
#include "kcommon.h"
#include <adf.h>
#include <xaiengine/xaie_helper.h>
#define DISABLE_LOG
static void wlog(const char *format, uint32_t value) {
    // Fix the log beyond the boundry issue
#ifdef DISABLE_LOG
    return;
#endif
    if (log_offset >= (LOG_MAX_LEN - 100)) {
        printf("log_offset = %d  wlog return\n", log_offset);
        return;
    }
    int32_t row = get_coreid() & 0xF;

#ifdef _AIE_LOG_LOCAL_TEST_
    uint32_t *address = AIEML_CORE_DM_CORE_VIEW_LOG_ADDR;
#else
    uint32_t address = AIEML_CORE_DM_CORE_VIEW_LOG_ADDR;
#if __AIE_ARCH__ == 10
    if (row & 1) { // row % 2 == 1
        address = AIE_CORE_DM_CORE_VIEW_LOG_ADDR_EAST;
    } else {
        address = AIE_CORE_DM_CORE_VIEW_LOG_ADDR_WEST;
    }
#endif
    printf("log base address = %x for row %d -logmarked = %d\n", address, row, logmarked);
#endif

    printf("a wlog address = 0x%x log_offset = 0x%0x row = %d\n", address, log_offset, row);
    uint32_t cur_log_offset = 0;
    if (0 == logmarked) {
        *((uint32_t *)address) = MAICMARK;
        logmarked = 1;
        log_offset = log_offset + 4;
        cur_log_offset = log_offset;
    }
    // printf("log1\n");
    uint8_t *ptr = (uint8_t *)((uint8_t *)address + log_offset);
    while (*format) {
        *(char *)(ptr) = *format;
        // printf("%x address is %c\n", (ptr + log_offset), *format);
        ptr += sizeof(char);
        log_offset += sizeof(char);
        format++;
    }
    printf("log_offset = %d logmarked = %d\n", log_offset, logmarked);
#define STR_MAX 16
    char str[STR_MAX]; // Assuming an int can be represented in 11 characters
    str[STR_MAX - 1] = 0;
    char *pstr = str;
    if (value == 0) {
        str[0] = (value % 10) + '0';
        str[1] = '\0';
    } else {
        for (int i = STR_MAX - 2; i >= 0 && value; i--) {
            int v = value % 10;
            value = value / 10;
            str[i] = v + '0';
            pstr = &str[i];
        }
    }
    uint32_t str_length = strlen(pstr);
    memcpy(ptr, pstr, str_length);
    ptr += str_length;
    log_offset += str_length;

    *(char *)(ptr++) = ';';
    log_offset += sizeof(char);
    if (*format == 0) { // use previous variable format to simu the chess_mem_fence
        int32_t bytesN = log_offset % 4;
        if (bytesN) {
            int i = 0;
            while (i++ < (4 - bytesN)) {
                *(ptr++) = ' ';
                log_offset++;
            }
        }
        *ptr = 0;
    }
}

#endif
