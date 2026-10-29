// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
//
#ifndef PS_IPC_PROTOCOL_H
#define PS_IPC_PROTOCOL_H

#include <stdint.h>

typedef uint8_t aeg_ps_ipc_cmd_t;

enum {
    AEG_PS_IPC_CMD_PING = 0x01u,

    AEG_PS_IPC_CMD_WRITE32 = 0x10u,
    AEG_PS_IPC_CMD_READ32 = 0x11u,

    AEG_PS_IPC_CMD_NPI_WRITE32 = 0x12u,
    AEG_PS_IPC_CMD_NPI_READ32 = 0x13u,

    AEG_PS_IPC_CMD_WRITE_GM = 0x20u,
    AEG_PS_IPC_CMD_READ_GM = 0x21u,

    AEG_PS_IPC_CMD_ALLOC_GM = 0x22u,

    AEG_PS_IPC_CMD_FREE_GM = 0x23u,

    AEG_PS_IPC_CMD_START_PLIO = 0x30u,

    AEG_PS_IPC_CMD_GRAPH_INIT = 0x40u,

    AEG_PS_IPC_CMD_EXIT = 0xFFu,
};

typedef uint8_t aeg_ps_ipc_status_t;

enum {
    AEG_PS_IPC_STATUS_OK = 0x00u,
    AEG_PS_IPC_STATUS_ERR_PROTO = 0x01u,
    AEG_PS_IPC_STATUS_ERR_IO = 0x02u,
};

#pragma pack(push, 1)

typedef struct {
    aeg_ps_ipc_cmd_t cmd;
    uint8_t _reserved[3];
    uint64_t arg1;
    uint32_t arg2;
} aeg_ps_ipc_request_t;

typedef struct {
    aeg_ps_ipc_status_t status;
    uint8_t _reserved[7];
    uint64_t value;
} aeg_ps_ipc_response_t;

#pragma pack(pop)

#define AEG_PS_IPC_SOCKET_PATH_MAX 256
#define AEG_PS_IPC_ENV_SOCKET "AEG_PS_IPC_SOCKET"
#define AEG_PS_IPC_ENV_ENABLE "AEG_PS_IPC_SERVER"

#ifdef __cplusplus
#include <cerrno>
#include <cstdio>
#include <unistd.h>

namespace aeg_ps_ipc {

inline int send_all(int fd, const void *buf, size_t n) {
    const char *p = static_cast<const char *>(buf);
    size_t remaining = n;
    while (remaining > 0) {
        ssize_t sent = ::write(fd, p, remaining);
        if (sent < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (sent == 0)
            return -1;
        p += sent;
        remaining -= static_cast<size_t>(sent);
    }
    return 0;
}

inline int recv_all(int fd, void *buf, size_t n) {
    char *p = static_cast<char *>(buf);
    size_t remaining = n;
    while (remaining > 0) {
        ssize_t got = ::read(fd, p, remaining);
        if (got < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (got == 0)
            return -1;
        p += got;
        remaining -= static_cast<size_t>(got);
    }
    return 0;
}

inline int send_request(int fd, aeg_ps_ipc_cmd_t cmd, uint64_t arg1, uint32_t arg2, const void *payload, size_t payload_size) {
    aeg_ps_ipc_request_t req;
    req.cmd = cmd;
    req._reserved[0] = 0;
    req._reserved[1] = 0;
    req._reserved[2] = 0;
    req.arg1 = arg1;
    req.arg2 = arg2;
    if (send_all(fd, &req, sizeof(req)) != 0)
        return -1;
    if (payload_size > 0 && payload != nullptr) {
        if (send_all(fd, payload, payload_size) != 0)
            return -1;
    }
    return 0;
}

inline int recv_response(int fd, aeg_ps_ipc_response_t *resp) { return recv_all(fd, resp, sizeof(*resp)); }

} // namespace aeg_ps_ipc
#endif // __cplusplus

#endif // PS_IPC_PROTOCOL_H