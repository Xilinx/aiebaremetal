// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "aeg_log.h"

// Protocol definitions:
//  - The header is 12 bytes long: 4 bytes for a magic number and 8 bytes for the file size.
#define HEADER_SIZE 12
#define MAGIC_NUMBER 0x46494C45 // 'FILE' in ASCII

typedef int16_t (*DataCallback)(void *data, int len);

// Structure used by the server to maintain file transfer state.
typedef struct file_transfer_state {
    uint8_t header_buf[HEADER_SIZE]; // Buffer to accumulate the header.
    uint16_t header_received;        // Number of header bytes received so far.
    uint64_t file_size;              // Total expected file size (in bytes).
    uint64_t bytes_received;         // Total number of file data bytes received.
} file_transfer_state_t;

#ifdef __cplusplus
extern "C" {
#endif

static uint32_t crc32_bitwise(const unsigned char *buf, size_t len) {
    uint32_t crc = 0;
    crc = ~crc; // Invert bits of the initial crc value.
    while (len--) {
        crc ^= *buf++; // XOR byte into least significant byte of crc.
        // Process 8 bits of the current byte.
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc = crc >> 1;
        }
    }
    crc = ~crc;
    AEG_LOG("data crc is 0x%x\n", crc);
    return crc; // Invert bits back before returning.
}

#ifdef __cplusplus
}
#endif

#endif // FILE_TRANSFER_H
