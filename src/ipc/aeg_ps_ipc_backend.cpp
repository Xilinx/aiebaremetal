// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
//
#include "aeg_ps_ipc_backend.h"
#include "ps_ipc_protocol.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <time.h>
#include <vector>

static int g_fd = -1;

static bool ipc_verbose() {
    static int v = -1;
    if (v < 0) v = (std::getenv("AEG_PS_IPC_VERBOSE") != nullptr) ? 1 : 0;
    return v == 1;
}

static FILE *g_log_fp = nullptr;
static uint64_t g_log_seq = 0;

static void log_open() {
    if (g_log_fp)
        return;
    const char *p = std::getenv("AEG_IPC_LOG");
    if (!p || !p[0])
        return;
    g_log_fp = fopen(p, "w");
    if (g_log_fp) {
        fprintf(g_log_fp, "seq,side,cmd,arg1,arg2,status,value,note\n");
        fflush(g_log_fp);
        if (ipc_verbose()) fprintf(stderr, "[aeg_ps_ipc_backend] IPC log -> %s\n", p);
    }
}

static void log_close() {
    if (g_log_fp) {
        fclose(g_log_fp);
        g_log_fp = nullptr;
    }
}

static const char *status_str(uint8_t s) {
    switch (s) {
    case AEG_PS_IPC_STATUS_OK:
        return "OK";
    case AEG_PS_IPC_STATUS_ERR_PROTO:
        return "ERR_PROTO";
    case AEG_PS_IPC_STATUS_ERR_IO:
        return "ERR_IO";
    default:
        return "ERR_CONN";
    }
}

static void log_tx(const char *cmd, uint64_t arg1, uint64_t arg2, const aeg_ps_ipc_response_t *resp, const char *note = "") {
    if (!g_log_fp)
        return;
    const char *st = resp ? status_str(resp->status) : "ERR_CONN";
    uint64_t val = resp ? resp->value : 0;
    fprintf(g_log_fp, "%llu,client,%s,0x%llx,%llu,%s,0x%llx,%s\n", (unsigned long long)g_log_seq++, cmd, (unsigned long long)arg1,
            (unsigned long long)arg2, st, (unsigned long long)val, note);
    fflush(g_log_fp);
}

extern "C" int aeg_ipc_backend_get_fd() { return g_fd; }

extern "C" int aeg_ipc_backend_connect(const char *socket_path) {
    if (g_fd >= 0) {
        if (ipc_verbose()) fprintf(stderr, "[aeg_ps_ipc_backend] already connected\n");
        return 0;
    }

    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("[aeg_ps_ipc_backend] socket()");
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (::connect(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
        perror("[aeg_ps_ipc_backend] connect()");
        ::close(fd);
        return -1;
    }

    g_fd = fd;
    if (ipc_verbose()) fprintf(stderr, "[aeg_ps_ipc_backend] connected to %s\n", socket_path);
    log_open();
    return 0;
}

extern "C" void aeg_ipc_backend_disconnect(int exit_code) {
    if (g_fd < 0)
        return;

    aeg_ps_ipc::send_request(g_fd, AEG_PS_IPC_CMD_EXIT, static_cast<uint64_t>(exit_code), 0, nullptr, 0);
    aeg_ps_ipc_response_t resp;
    memset(&resp, 0, sizeof(resp));
    aeg_ps_ipc::recv_response(g_fd, &resp); // wait for server ACK
    log_tx("EXIT", static_cast<uint64_t>(exit_code), 0, &resp);

    ::shutdown(g_fd, SHUT_WR);
    char drain[64];
    while (::read(g_fd, drain, sizeof(drain)) > 0) {
    }

    ::close(g_fd);
    g_fd = -1;
    log_close();
    if (ipc_verbose()) fprintf(stderr, "[aeg_ps_ipc_backend] disconnected (exit_code=%d)\n", exit_code);
}

static inline bool do_transaction(aeg_ps_ipc_cmd_t cmd, uint64_t arg1, uint32_t arg2, const void *payload, size_t payload_size,
                                  aeg_ps_ipc_response_t *resp_out, const char *log_cmd, const char *log_note = "") {
    if (g_fd < 0) {
        if (ipc_verbose()) fprintf(stderr, "[aeg_ps_ipc_backend] ERROR: not connected (cmd=0x%02x)\n", static_cast<unsigned>(cmd));
        if (resp_out) {
            memset(resp_out, 0, sizeof(*resp_out));
            resp_out->status = 0xFF; // signal connection error
        }
        if (g_log_fp) {
            fprintf(g_log_fp, "%llu,client,%s,0x%llx,%u,ERR_CONN,0x0,%s\n", (unsigned long long)g_log_seq++, log_cmd,
                    (unsigned long long)arg1, arg2, log_note);
            fflush(g_log_fp);
        }
        return false;
    }

    if (aeg_ps_ipc::send_request(g_fd, cmd, arg1, arg2, payload, payload_size) != 0) {
        if (ipc_verbose()) fprintf(stderr, "[aeg_ps_ipc_backend] send error (cmd=0x%02x)\n", static_cast<unsigned>(cmd));
        if (resp_out) {
            memset(resp_out, 0, sizeof(*resp_out));
            resp_out->status = 0xFF;
        }
        log_tx(log_cmd, arg1, arg2, nullptr, log_note);
        return false;
    }

    aeg_ps_ipc_response_t resp;
    memset(&resp, 0, sizeof(resp));
    bool ok = (aeg_ps_ipc::recv_response(g_fd, &resp) == 0);
    if (!ok) {
        if (ipc_verbose()) fprintf(stderr, "[aeg_ps_ipc_backend] recv error (cmd=0x%02x)\n", static_cast<unsigned>(cmd));
        resp.status = 0xFF; // signal connection error
    }

    log_tx(log_cmd, arg1, arg2, ok ? &resp : nullptr, log_note);

    if (resp_out)
        *resp_out = resp;
    return ok && (resp.status == AEG_PS_IPC_STATUS_OK);
}

extern "C" uint64_t aeg_ipc_alloc_gm(uint32_t size) {
    aeg_ps_ipc_response_t resp;
    if (!do_transaction(AEG_PS_IPC_CMD_ALLOC_GM, 0, size, nullptr, 0, &resp, "ALLOC_GM")) {
        return 0;
    }
    return resp.value;
}

extern "C" void aeg_ipc_free_gm(uint64_t aie_addr) {
    char note[32];
    snprintf(note, sizeof(note), "addr=0x%llx", (unsigned long long)aie_addr);
    do_transaction(AEG_PS_IPC_CMD_FREE_GM, aie_addr, 0, nullptr, 0, nullptr, "FREE_GM", note);
}

extern "C" void aeg_ipc_write_gm(uint64_t aie_addr, const void *data, size_t size) {
    char note[32];
    snprintf(note, sizeof(note), "bytes=%zu", size);
    do_transaction(AEG_PS_IPC_CMD_WRITE_GM, aie_addr, static_cast<uint32_t>(size), data, size, nullptr, "WRITE_GM", note);
}

extern "C" void aeg_ipc_read_gm(uint64_t aie_addr, void *buf, size_t size) {
    if (g_fd < 0)
        return;

    char note[32];
    snprintf(note, sizeof(note), "bytes=%zu", size);

    aeg_ps_ipc::send_request(g_fd, AEG_PS_IPC_CMD_READ_GM, aie_addr, static_cast<uint32_t>(size), nullptr, 0);
    aeg_ps_ipc_response_t resp;
    memset(&resp, 0, sizeof(resp));
    bool ok = (aeg_ps_ipc::recv_response(g_fd, &resp) == 0);
    log_tx("READ_GM", aie_addr, size, ok ? &resp : nullptr, note);
    if (!ok || resp.status != AEG_PS_IPC_STATUS_OK)
        return;
    if (aeg_ps_ipc::recv_all(g_fd, buf, size) != 0) {
        if (ipc_verbose()) fprintf(stderr, "[aeg_ps_ipc_backend] recv payload error (READ_GM)\n");
    }
}

namespace {
struct GmHostAlloc {
    uint64_t dev_addr = 0U;
    size_t   size     = 0U;
};
std::map<uintptr_t, GmHostAlloc> g_gm_host_allocs;
}

extern "C" void *aeg_ipc_alloc_gm_host(uint32_t size) {
    uint64_t dev_addr = aeg_ipc_alloc_gm(size);
    if (!dev_addr)
        return nullptr;
    void *host = malloc(size);
    if (!host) {
        aeg_ipc_free_gm(dev_addr);
        return nullptr;
    }
    g_gm_host_allocs[reinterpret_cast<uintptr_t>(host)] = GmHostAlloc{dev_addr, size};
    return host;
}

extern "C" void aeg_ipc_free_gm_host(void *host) {
    auto it = g_gm_host_allocs.find(reinterpret_cast<uintptr_t>(host));
    if (it != g_gm_host_allocs.end()) {
        aeg_ipc_free_gm(it->second.dev_addr);
        g_gm_host_allocs.erase(it);
    }
    free(host);
}

extern "C" uint64_t aeg_ipc_dev_addr(const void *host) {
    const uintptr_t addr = reinterpret_cast<uintptr_t>(host);
    auto it = g_gm_host_allocs.upper_bound(addr);
    if (it == g_gm_host_allocs.begin())
        return 0U;
    --it;
    const uintptr_t base = it->first;
    if (addr < base || addr >= base + it->second.size)
        return 0U;
    return it->second.dev_addr + static_cast<uint64_t>(addr - base);
}

extern "C" void ess_Write32(uint64_t Addr, unsigned int Data) {
    char note[32];
    snprintf(note, sizeof(note), "data=0x%08x", Data);
    do_transaction(AEG_PS_IPC_CMD_WRITE32, Addr, static_cast<uint32_t>(Data), nullptr, 0, nullptr, "WRITE32", note);
}

extern "C" unsigned int ess_Read32(uint64_t Addr) {
    aeg_ps_ipc_response_t resp;
    do_transaction(AEG_PS_IPC_CMD_READ32, Addr, 0, nullptr, 0, &resp, "READ32");
    return static_cast<unsigned int>(resp.value);
}

extern "C" void ess_NpiWrite32(uint64_t Addr, unsigned int Data) {
    char note[32];
    snprintf(note, sizeof(note), "data=0x%08x", Data);
    do_transaction(AEG_PS_IPC_CMD_NPI_WRITE32, Addr, static_cast<uint32_t>(Data), nullptr, 0, nullptr, "NPI_WRITE32", note);
}

extern "C" unsigned int ess_NpiRead32(uint64_t Addr) {
    aeg_ps_ipc_response_t resp;
    do_transaction(AEG_PS_IPC_CMD_NPI_READ32, Addr, 0, nullptr, 0, &resp, "NPI_READ32");
    return static_cast<unsigned int>(resp.value);
}

extern "C" void ess_WriteGM(uint64_t addr, const void *data, uint64_t size) { aeg_ipc_write_gm(addr, data, static_cast<size_t>(size)); }

extern "C" void ess_ReadGM(uint64_t addr, void *data, uint64_t size) { aeg_ipc_read_gm(addr, data, static_cast<size_t>(size)); }

extern "C" void ess_WriteCmd(unsigned char /*Command*/, unsigned char /*ColId*/, unsigned char /*RowId*/, unsigned int /*CmdWd0*/,
                             unsigned int /*CmdWd1*/, unsigned char * /*CmdStr*/) {}

extern "C" void start_plios() { do_transaction(AEG_PS_IPC_CMD_START_PLIO, 0, 0, nullptr, 0, nullptr, "START_PLIO"); }

int32_t g_start_plio = 0;
namespace aiesim {
void StartPlios() { start_plios(); }
}

extern "C" int aeg_ipc_graph_init_ipc(const char *graph_name, uint32_t name_len) {
    if (!graph_name || name_len == 0) return -1;
    return do_transaction(AEG_PS_IPC_CMD_GRAPH_INIT, 0, name_len,
                          graph_name, name_len, nullptr, "GRAPH_INIT") ? 0 : -1;
}

#include <ctime>
extern "C" void XTime_GetTime(unsigned long long *t) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    *t = (unsigned long long)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}
