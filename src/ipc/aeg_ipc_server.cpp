// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
//
// AEG_IPC_GRAPH_NAMES=graph1,graph2


#include "ps_ipc_protocol.h"

#include <systemc.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <vector>

extern void PSWrite32(uint64_t Addr, unsigned int Data);
extern unsigned int PSRead32(uint64_t Addr);
extern void PSWriteGM(uint64_t addr, const void *data, uint64_t size);
extern void PSReadGM(uint64_t addr, void *data, uint64_t size);
extern void PSNpiWrite32(uint64_t addr, unsigned int data);
extern unsigned int PSNpiRead32(uint64_t addr);
extern void PSStartPLIO();

extern void setWrite32Ptr(void (*)(uint64_t, unsigned int));
extern void setRead32Ptr(unsigned int (*)(uint64_t));
extern void setWriteGMPtr(void (*)(uint64_t, const void *, uint64_t));
extern void setReadGMPtr(void (*)(uint64_t, void *, uint64_t));
extern void setNpiWrite32Ptr(void (*)(uint64_t, unsigned int));
extern void setNpiRead32Ptr(unsigned int (*)(uint64_t));
extern void setPLIOStartPtr(void (*)());

extern "C" {
void aeg_ipc_devinst_init();
int  aeg_ipc_run_graph_init(const char *graph_name, const char *work_dir);
uint64_t aeg_ipc_alloc_mem(uint32_t size);
void aeg_ipc_free_mem(uint64_t dev_addr);
}

#include <aie_codegen.h>
extern XAie_DevInst DevInst;

static FILE *g_srv_log_fp = nullptr;
static uint64_t g_srv_log_seq = 0;

static bool ipc_verbose() {
    static int v = -1;
    if (v < 0) v = (std::getenv("AEG_PS_IPC_VERBOSE") != nullptr) ? 1 : 0;
    return v == 1;
}

static void srv_trace_open() {
    if (g_srv_log_fp)
        return;
    const char *p = std::getenv("AEG_IPC_LOG_SERVER");
    if (!p || !p[0])
        p = std::getenv("AEG_REGTRACE_SERVER");
    if (!p || !p[0])
        return;
    g_srv_log_fp = fopen(p, "w");
    if (g_srv_log_fp) {
        fprintf(g_srv_log_fp, "seq,side,cmd,arg1,arg2,status,value,note\n");
        fflush(g_srv_log_fp);
        if (ipc_verbose()) fprintf(stderr, "[AEG_PS_IPC] server IPC log -> %s\n", p);
    }
}

static void srv_log(const char *cmd, uint64_t arg1, uint64_t arg2, uint8_t status, uint64_t value, const char *note = "") {
    if (!g_srv_log_fp)
        return;
    const char *st;
    switch (status) {
    case AEG_PS_IPC_STATUS_OK:
        st = "OK";
        break;
    case AEG_PS_IPC_STATUS_ERR_PROTO:
        st = "ERR_PROTO";
        break;
    case AEG_PS_IPC_STATUS_ERR_IO:
        st = "ERR_IO";
        break;
    default:
        st = "ERR_UNK";
        break;
    }
    fprintf(g_srv_log_fp, "%llu,server,%s,0x%llx,%llu,%s,0x%llx,%s\n", (unsigned long long)g_srv_log_seq++, cmd, (unsigned long long)arg1,
            (unsigned long long)arg2, st, (unsigned long long)value, note);
    fflush(g_srv_log_fp);
}

static void srv_trace(const char *op, uint64_t addr, uint32_t data) {
    char note[32];
    snprintf(note, sizeof(note), "data=0x%08x", data);
    srv_log(op, addr, data, AEG_PS_IPC_STATUS_OK, data, note);
}


struct ipc_work_item {
    aeg_ps_ipc_request_t req;
    aeg_ps_ipc_response_t resp;
    std::vector<uint8_t> gm_data;
    bool ready;
    std::mutex mtx;
    std::condition_variable cv;
    ipc_work_item() : ready(false) {
        memset(&req, 0, sizeof(req));
        memset(&resp, 0, sizeof(resp));
    }
};

static std::queue<std::shared_ptr<ipc_work_item>> g_work_queue;
static std::mutex g_queue_mutex;
static std::condition_variable g_queue_cv;
static std::atomic<bool> g_server_done{false};
static std::atomic<int> g_dbg_listen_fd{-1};
static sc_core::sc_event *g_work_event = nullptr;

static std::mutex g_ipc_exit_mutex;
static std::condition_variable g_ipc_exit_cv;
static bool g_ipc_exit_done = false;
static int g_ipc_exit_code = 0;
extern int graph_return_value;

// Post-run hold.  A passing run tears the SystemC loop down the moment the app
// disconnects, and the read-only debug socket dies with it -- so the array was
// only readable for the ~80s the app ran, and a scan afterwards found nothing.
// AEG_PS_IPC_HOLD_SEC keeps the loop draining debug reads for N seconds past
// the end of the run, with the array frozen in its final state.
static std::atomic<long long> g_exit_deadline_ms{-1};

static long long now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

static unsigned ipc_hold_sec() {
    static int hold = -1;
    if (hold < 0) {
        hold = 0;
        const char *p = std::getenv("AEG_PS_IPC_HOLD_SEC");
        if (p && p[0]) {
            const long v = strtol(p, nullptr, 10);
            if (v > 0)
                hold = static_cast<int>(v);
        }
    }
    return static_cast<unsigned>(hold);
}

// Arm the hold on the first exit request.  Returns false when no hold is
// configured, in which case the caller tears down immediately as before.
static bool arm_exit_hold() {
    const unsigned h = ipc_hold_sec();
    if (h == 0)
        return false;
    long long expected = -1;
    if (g_exit_deadline_ms.compare_exchange_strong(expected, now_ms() + static_cast<long long>(h) * 1000)) {
        fprintf(stderr, "[AEG_PS_IPC] run finished -- holding simulator open %us for live debug reads\n", h);
        fflush(stderr);
    }
    return true;
}

static bool exit_hold_active() {
    const long long d = g_exit_deadline_ms.load();
    return d >= 0 && now_ms() < d;
}

static std::shared_ptr<ipc_work_item> enqueue_work(const aeg_ps_ipc_request_t &req, const std::vector<uint8_t> &payload = {}) {
    auto item = std::make_shared<ipc_work_item>();
    item->req = req;
    if (!payload.empty())
        item->gm_data = payload;
    {
        std::lock_guard<std::mutex> lk(g_queue_mutex);
        g_work_queue.push(item);
    }
    if (g_work_event)
        g_work_event->notify(1, sc_core::SC_NS);
    g_queue_cv.notify_one();
    return item;
}

static void dispatch_work_item(std::shared_ptr<ipc_work_item> &item) {
    const aeg_ps_ipc_request_t &req = item->req;
    aeg_ps_ipc_response_t &resp = item->resp;
    memset(&resp, 0, sizeof(resp));
    resp.status = AEG_PS_IPC_STATUS_OK;

    switch (req.cmd) {

    case AEG_PS_IPC_CMD_WRITE32:
        srv_trace("W32", req.arg1, req.arg2);
        PSWrite32(static_cast<uint64_t>(req.arg1), static_cast<unsigned int>(req.arg2));
        break;

    case AEG_PS_IPC_CMD_READ32:
        resp.value = PSRead32(static_cast<uint64_t>(req.arg1));
        srv_trace("R32", req.arg1, static_cast<uint32_t>(resp.value));
        break;

    case AEG_PS_IPC_CMD_NPI_WRITE32: {
        char note[32];
        snprintf(note, sizeof(note), "data=0x%08x", req.arg2);
        PSNpiWrite32(static_cast<uint64_t>(req.arg1), static_cast<unsigned int>(req.arg2));
        srv_log("NPI_WRITE32", req.arg1, req.arg2, AEG_PS_IPC_STATUS_OK, 0, note);
        break;
    }

    case AEG_PS_IPC_CMD_NPI_READ32:
        resp.value = PSNpiRead32(static_cast<uint64_t>(req.arg1));
        srv_log("NPI_READ32", req.arg1, 0, AEG_PS_IPC_STATUS_OK, resp.value);
        break;

    case AEG_PS_IPC_CMD_WRITE_GM: {
        char note[32];
        snprintf(note, sizeof(note), "bytes=%zu", item->gm_data.size());
        if (!item->gm_data.empty())
            PSWriteGM(static_cast<uint64_t>(req.arg1), item->gm_data.data(), static_cast<uint64_t>(item->gm_data.size()));
        srv_log("WRITE_GM", req.arg1, item->gm_data.size(), AEG_PS_IPC_STATUS_OK, 0, note);
        break;
    }

    case AEG_PS_IPC_CMD_READ_GM: {
        uint32_t read_size = req.arg2;
        if (read_size > 0) {
            item->gm_data.resize(read_size);
            PSReadGM(static_cast<uint64_t>(req.arg1), item->gm_data.data(), static_cast<uint64_t>(read_size));
        }
        char note[32];
        snprintf(note, sizeof(note), "bytes=%u", read_size);
        srv_log("READ_GM", req.arg1, read_size, AEG_PS_IPC_STATUS_OK, 0, note);
        break;
    }

    case AEG_PS_IPC_CMD_ALLOC_GM: {
        uint32_t size = req.arg2;
        if (size == 0) {
            resp.status = AEG_PS_IPC_STATUS_ERR_IO;
            srv_log("ALLOC_GM", 0, size, AEG_PS_IPC_STATUS_ERR_IO, 0, "zero_size");
            break;
        }
        uint64_t dev_addr = aeg_ipc_alloc_mem(size);
        if (!dev_addr) {
            resp.status = AEG_PS_IPC_STATUS_ERR_IO;
            srv_log("ALLOC_GM", 0, size, AEG_PS_IPC_STATUS_ERR_IO, 0, "alloc_failed");
            break;
        }
        resp.value = dev_addr;
        {
            char note[32];
            snprintf(note, sizeof(note), "size=%u", size);
            srv_log("ALLOC_GM", 0, size, AEG_PS_IPC_STATUS_OK, dev_addr, note);
        }
        break;
    }

    case AEG_PS_IPC_CMD_FREE_GM: {
        uint64_t dev_addr = req.arg1;
        aeg_ipc_free_mem(dev_addr);
        srv_log("FREE_GM", dev_addr, 0, AEG_PS_IPC_STATUS_OK, 0);
        break;
    }

    case AEG_PS_IPC_CMD_START_PLIO:
        PSStartPLIO();
        srv_log("START_PLIO", 0, 0, AEG_PS_IPC_STATUS_OK, 0);
        break;

    case AEG_PS_IPC_CMD_GRAPH_INIT: {
        const char *work_dir = std::getenv("AIE_WORK_DIR");
        if (!work_dir || !work_dir[0]) work_dir = "./Work";
        std::string graph_name(reinterpret_cast<const char *>(item->gm_data.data()),
                               item->gm_data.size());
        if (DevInst.DevOps && DevInst.DevOps->SetPartIsolationAfterRst)
            DevInst.DevOps->SetPartIsolationAfterRst(&DevInst, 0);
        bool ok = (aeg_ipc_run_graph_init(graph_name.c_str(), work_dir) == 0);
        if (!ok) resp.status = AEG_PS_IPC_STATUS_ERR_IO;
        if (ipc_verbose()) {
            fprintf(stderr, "[AEG_PS_IPC] GRAPH_INIT '%s' %s\n",
                    graph_name.c_str(), ok ? "ok" : "failed");
            fflush(stderr);
        }
        srv_log("GRAPH_INIT", 0, 0, resp.status, 0, graph_name.c_str());
        break;
    }

    case AEG_PS_IPC_CMD_EXIT:
        graph_return_value = static_cast<int>(req.arg1);
        srv_log("EXIT", req.arg1, 0, AEG_PS_IPC_STATUS_OK, 0);
        break;

    default: {
        resp.status = AEG_PS_IPC_STATUS_ERR_PROTO;
        char note[32];
        snprintf(note, sizeof(note), "unknown_cmd=0x%02x", req.cmd);
        srv_log("UNKNOWN", req.arg1, req.arg2, AEG_PS_IPC_STATUS_ERR_PROTO, 0, note);
        break;
    }
    }

    {
        std::lock_guard<std::mutex> lk(item->mtx);
        item->ready = true;
    }
    item->cv.notify_one();
}

static void run_debug_client_thread(int client_fd) {
    using namespace aeg_ps_ipc;
    while (true) {
        aeg_ps_ipc_request_t req;
        if (recv_all(client_fd, &req, sizeof(req)) != 0)
            break;
        aeg_ps_ipc_response_t resp;
        memset(&resp, 0, sizeof(resp));
        if (req.cmd == AEG_PS_IPC_CMD_READ32 || req.cmd == AEG_PS_IPC_CMD_NPI_READ32) {
            auto item = enqueue_work(req);
            bool ready;
            {
                std::unique_lock<std::mutex> lk(item->mtx);
                while (!(ready = item->ready) && !g_server_done)
                    item->cv.wait_for(lk, std::chrono::milliseconds(250));
                ready = item->ready;
            }
            if (ready)
                resp = item->resp;
            else
                resp.status = AEG_PS_IPC_STATUS_ERR_IO;
        } else {
            resp.status = AEG_PS_IPC_STATUS_ERR_PROTO;
        }
        if (send_all(client_fd, &resp, sizeof(resp)) != 0)
            break;
    }
    ::close(client_fd);
}

static void run_debug_socket_thread(std::string dbg_path) {
    using namespace aeg_ps_ipc;

    unlink(dbg_path.c_str());
    int listen_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("[AEG_PS_IPC] debug socket()");
        return;
    }
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, dbg_path.c_str(), sizeof(addr.sun_path) - 1);
    if (::bind(listen_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
        perror("[AEG_PS_IPC] debug bind()");
        ::close(listen_fd);
        return;
    }
    if (::listen(listen_fd, 16) < 0) {
        perror("[AEG_PS_IPC] debug listen()");
        ::close(listen_fd);
        return;
    }
    if (ipc_verbose()) {
        fprintf(stderr, "[AEG_PS_IPC] debug socket listening on %s\n", dbg_path.c_str());
        fflush(stderr);
    }
    g_dbg_listen_fd = listen_fd;
    while (!g_server_done) {
        int client_fd = ::accept(listen_fd, nullptr, nullptr);
        if (client_fd < 0) break;
        std::thread(run_debug_client_thread, client_fd).detach();
    }
    g_dbg_listen_fd = -1;
    ::close(listen_fd);
    unlink(dbg_path.c_str());
}

static void run_ps_ipc_server_thread(std::string socket_path) {
    using namespace aeg_ps_ipc;

    unlink(socket_path.c_str());

    int listen_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("[AEG_PS_IPC] socket()");
        return;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (::bind(listen_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
        perror("[AEG_PS_IPC] bind()");
        ::close(listen_fd);
        return;
    }
    if (::listen(listen_fd, 1) < 0) {
        perror("[AEG_PS_IPC] listen()");
        ::close(listen_fd);
        return;
    }

    {
        char ready_path[AEG_PS_IPC_SOCKET_PATH_MAX];
        snprintf(ready_path, sizeof(ready_path), "/tmp/aeg_ps_ipc_%d.ready", static_cast<int>(getpid()));
        FILE *rf = fopen(ready_path, "w");
        if (rf) {
            fprintf(rf, "%s\n", socket_path.c_str());
            fclose(rf);
        }
    }

    if (ipc_verbose()) {
        fprintf(stderr, "[AEG_PS_IPC] server listening on %s\n", socket_path.c_str());
        fflush(stderr);
    }

    int client_fd = ::accept(listen_fd, nullptr, nullptr);
    if (client_fd < 0) {
        perror("[AEG_PS_IPC] accept()");
        ::close(listen_fd);
        return;
    }
    if (ipc_verbose()) {
        fprintf(stderr, "[AEG_PS_IPC] client connected\n");
        fflush(stderr);
    }

    bool running = true;
    while (running) {
        aeg_ps_ipc_request_t req;
        if (recv_all(client_fd, &req, sizeof(req)) != 0) {
            if (ipc_verbose()) fprintf(stderr, "[AEG_PS_IPC] connection closed or error\n");
            // ipc_app normally just exits rather than sending EXIT -- its graph
            // destructor does not reliably run -- so this, not the EXIT case, is
            // the path a passing run actually takes. Arm the hold here too or
            // the common case would never get a debug window.
            if (!arm_exit_hold()) {
                std::lock_guard<std::mutex> lk(g_ipc_exit_mutex);
                g_ipc_exit_done = true;
            }
            g_ipc_exit_cv.notify_all();
            if (g_work_event)
                g_work_event->notify(1, sc_core::SC_NS);
            break;
        }

        if (req.cmd == AEG_PS_IPC_CMD_PING) {
            aeg_ps_ipc_response_t resp;
            memset(&resp, 0, sizeof(resp));
            resp.status = AEG_PS_IPC_STATUS_OK;
            send_all(client_fd, &resp, sizeof(resp));
            continue;
        }

        std::vector<uint8_t> payload;
        bool recv_err = false;
        if ((req.cmd == AEG_PS_IPC_CMD_WRITE_GM || req.cmd == AEG_PS_IPC_CMD_GRAPH_INIT) && req.arg2 > 0) {
            payload.resize(req.arg2);
            if (recv_all(client_fd, payload.data(), payload.size()) != 0)
                recv_err = true;
        }

        if (recv_err) {
            aeg_ps_ipc_response_t err;
            memset(&err, 0, sizeof(err));
            err.status = AEG_PS_IPC_STATUS_ERR_IO;
            send_all(client_fd, &err, sizeof(err));
            continue;
        }

        auto item = enqueue_work(req, payload);

        {
            std::unique_lock<std::mutex> lk(item->mtx);
            item->cv.wait(lk, [&item] { return item->ready; });
        }

        if (req.cmd == AEG_PS_IPC_CMD_EXIT)
            running = false;

        send_all(client_fd, &item->resp, sizeof(item->resp));

        if (req.cmd == AEG_PS_IPC_CMD_READ_GM && !item->gm_data.empty())
            send_all(client_fd, item->gm_data.data(), item->gm_data.size());
    }

    if (ipc_verbose()) {
        fprintf(stderr, "[AEG_PS_IPC] server thread done\n");
        fflush(stderr);
    }

    // Hold the debug listener open past the end of the run. Closing it here is
    // what made a finished run unreadable -- the socket file was already gone by
    // the time anyone scanned.
    while (exit_hold_active())
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

    g_server_done = true;
    {
        int dbg_fd = g_dbg_listen_fd.exchange(-1);
        if (dbg_fd >= 0)
            ::shutdown(dbg_fd, SHUT_RDWR);
    }

    ::close(client_fd);
    ::close(listen_fd);
    unlink(socket_path.c_str());

    char ready_path[AEG_PS_IPC_SOCKET_PATH_MAX];
    snprintf(ready_path, sizeof(ready_path), "/tmp/aeg_ps_ipc_%d.ready", static_cast<int>(getpid()));
    unlink(ready_path);
}

static std::vector<std::string> get_graph_names() {
    std::vector<std::string> names;

    const char *multi = std::getenv("AEG_IPC_GRAPH_NAMES");
    if (multi && multi[0]) {
        std::string s(multi);
        size_t pos = 0;
        while (pos < s.size()) {
            size_t comma = s.find(',', pos);
            if (comma == std::string::npos)
                comma = s.size();
            std::string name = s.substr(pos, comma - pos);
            if (!name.empty())
                names.push_back(name);
            pos = comma + 1;
        }
    }

    if (names.empty()) {
        const char *single = std::getenv("AEG_IPC_GRAPH_NAME");
        if (single && single[0])
            names.emplace_back(single);
    }

    if (names.empty() && ipc_verbose())
        fprintf(stderr, "[AEG_PS_IPC] WARNING: AEG_IPC_GRAPH_NAMES not set; "
                        "skipping server-side graph init\n");
    return names;
}

extern "C" int ps_main() {
    srv_trace_open();

    aeg_ipc_devinst_init();

    std::string socket_path;
    const char *env_path = std::getenv(AEG_PS_IPC_ENV_SOCKET);
    if (env_path && env_path[0] != '\0') {
        socket_path = env_path;
    } else {
        char buf[AEG_PS_IPC_SOCKET_PATH_MAX];
        snprintf(buf, sizeof(buf), "/tmp/aeg_ps_ipc_%d.sock", static_cast<int>(getpid()));
        socket_path = buf;
    }

    g_work_event = new sc_core::sc_event("ipc_work_event");

    std::thread ipc_thread(run_ps_ipc_server_thread, socket_path);
    ipc_thread.detach();

    std::string dbg_path = socket_path + ".dbg";
    std::thread dbg_thread(run_debug_socket_thread, dbg_path);
    dbg_thread.detach();

    static constexpr int kYieldEveryN = 1;
    static constexpr int kPollYieldNs = 100;
    int poll_count = 0;
    while (!g_ipc_exit_done) {
        sc_core::wait(kPollYieldNs, sc_core::SC_NS, *g_work_event);

        while (true) {
            std::shared_ptr<ipc_work_item> item;
            {
                std::lock_guard<std::mutex> lk(g_queue_mutex);
                if (!g_work_queue.empty()) {
                    item = g_work_queue.front();
                    g_work_queue.pop();
                }
            }
            if (!item)
                break;

            dispatch_work_item(item);

            if (item->req.cmd == AEG_PS_IPC_CMD_EXIT) {
                if (arm_exit_hold())
                    break;  // stay in the outer loop and keep serving debug reads
                g_ipc_exit_done = true;
                g_ipc_exit_code = graph_return_value;
                g_ipc_exit_cv.notify_all();
                break;
            }

            if (item->req.cmd == AEG_PS_IPC_CMD_READ32 || item->req.cmd == AEG_PS_IPC_CMD_NPI_READ32) {
                ++poll_count;
            } else {
                poll_count = 0;
            }
            if (poll_count > 0 && (poll_count % kYieldEveryN) == 0)
                sc_core::wait(kPollYieldNs, sc_core::SC_NS);
        }

        if (g_exit_deadline_ms.load() >= 0) {
            if (!exit_hold_active()) {
                fprintf(stderr, "[AEG_PS_IPC] hold elapsed -- shutting down\n");
                fflush(stderr);
                g_ipc_exit_done = true;
                g_ipc_exit_code = graph_return_value;
                g_ipc_exit_cv.notify_all();
            } else {
                // The outer wait() only advances 100ns of simulated time, so
                // without this the hold would spin a core flat for its whole
                // duration. Debug reads notify g_work_event, so a 2ms nap costs
                // them nothing measurable.
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        }
    }

    return g_ipc_exit_code;
}
