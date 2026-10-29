// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
//


#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int aeg_ipc_backend_connect(const char *socket_path);

void aeg_ipc_backend_disconnect(int exit_code);

int aeg_ipc_backend_get_fd();

uint64_t aeg_ipc_alloc_gm(uint32_t size);
void aeg_ipc_free_gm(uint64_t aie_addr);
void aeg_ipc_write_gm(uint64_t aie_addr, const void *data, size_t size);
void aeg_ipc_read_gm(uint64_t aie_addr, void *buf, size_t size);

void *aeg_ipc_alloc_gm_host(uint32_t size);
void aeg_ipc_free_gm_host(void *host);
uint64_t aeg_ipc_dev_addr(const void *host);

#ifdef __cplusplus
}
#endif
