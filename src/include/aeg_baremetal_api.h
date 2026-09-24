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
#ifndef AIE_BAREMETAL_API_H
#define AIE_BAREMETAL_API_H
#include "errno.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

#define PS_SO "ps_i"

#if defined(AEG_DEBUG) || defined(DEBUG) || defined(DEBUG_RUNTIME) || !defined(NDEBUG)
#define AEG_ERROR(...)                                                                                                 \
    do {                                                                                                               \
        std::fprintf(stderr, "[AEG_ERROR] ");                                                                          \
        std::fprintf(stderr, __VA_ARGS__);                                                                             \
        std::fprintf(stderr, "\n");                                                                                    \
    } while (0)
#else
#define AEG_ERROR(...)                                                                                                 \
    do {                                                                                                               \
    } while (0)
#endif

#ifndef AEG_LOG
#if defined(AEG_DEBUG)
#define AEG_LOG(...)                                                                                                   \
    do {                                                                                                               \
        std::printf(__VA_ARGS__);                                                                                      \
    } while (0)
#else
#define AEG_LOG(...)                                                                                                   \
    do {                                                                                                               \
    } while (0)
#endif
#endif

typedef int PROFILE_ID;

enum class aeg_err : int {
    ok = 0,
    user_error = EINVAL,
    internal_error = ENOTSUP,
    aie_driver_error = EIO,
    resource_unavailable = EAGAIN,
    invalid_state = 1000,
    null_argument = 1001,
    invalid_length = 1002,
    invalid_name = 1003,
    backend_not_ready = 1004,
    postcondition_failed = 1005,
};

class AIEGraph;
void abr_init_isr();
void dump_uc_app();
void abr_error_isr(void *data);
using BaremetalGraph = AIEGraph;

struct AIEGraphImpl;

namespace aeg {
namespace detail {
using GraphImplPtr = void *;
} // namespace detail
} // namespace aeg

class AIEGraph {
  public:
    AIEGraph(std::string GraphName);
    ~AIEGraph();
    void *malloc(size_t len);
    aeg_err free(void *mem);
    aeg_err selftestexit();
    aeg_err clock_enable();
    aeg_err clock_disable();

    aeg_err error_handling_cb_init(void *cb, void *priv);
    aeg_err init();
    aeg_err run(int num = -1);
    aeg_err gm2aie_nb(char *name, char *data, int len);
    aeg_err aie2gm_nb(char *name, char *data, int len);
    aeg_err gm2aie(char *name, char *data, int len);
    aeg_err aie2gm(char *name, char *data, int len);
    aeg_err wait();
    aeg_err end();
    aeg_err gmio_wait(std::string gmio_name, bool syncmode = true);
    aeg_err update(const std::string &port_name, void *data, size_t len);
    aeg_err read(const std::string &port_name, char *data, int len);

    // Internal Debug APIs
    char *dumpcorelog(int row, int col);
    void *getaiedevinst();

    // get the aie error report after doing the backtrack.
    void *BacktrackErrors();

    // External Buffer APIs
    aeg_err setAddress(char *externalbuffer_name, void *ptr);
    aeg_err wait(char *externalbuffer_name, std::string port_name);
    aeg_err gm2aie_nb(char *externalbuffer_name, std::string port_name);
    aeg_err aie2gm_nb(char *externalbuffer_name, std::string port_name);
    aeg_err setupPingPongBuffers(char *externalbuffer_name, void *const ptr1, void *const ptr2);

    // Profiling APIs
    PROFILE_ID start_profiling(const std::string &port_name, int option, uint32_t value = 0);
    PROFILE_ID start_profiling(const std::string &port_name1, const std::string &port_name2, int option,
                               uint32_t value = 0);
    long long read_profiling(PROFILE_ID h);
    aeg_err stop_profiling(PROFILE_ID h);

    aeg::detail::GraphImplPtr getGraphImpl() const;

  private:
    std::shared_ptr<AIEGraphImpl> graph;
    bool isInitialized;

    bool check_initialized_impl(const char *function_name);
};

#define CHECK_INITIALIZED() check_initialized_impl(__func__)

enum options_enum_init { io_profiling_option_enum_init = 0x20 };

enum io_profiling_option {
    io_total_stream_running_to_idle_cycles = io_profiling_option_enum_init,
    io_stream_start_to_bytes_transferred_cycles,
    io_stream_start_difference_cycles,
    io_stream_running_event_count
};

class GmioApiImpl;
class ExternalBufferApiImpl;
class RtpApiImpl;
class SharedBufferApiImpl;
namespace aeg {
namespace detail {

class BasePort {
  protected:
    std::string portName;
    GraphImplPtr graphImpl;
    BasePort(const std::string &name, GraphImplPtr impl) : portName(name), graphImpl(impl) {}

  public:
    virtual ~BasePort() = default;
    const std::string &getName() const { return portName; }

    PROFILE_ID start_profiling(int option, uint32_t value = 0);
    long long read_profiling(PROFILE_ID h);
    void stop_profiling(PROFILE_ID h);
};

class BaseGMIO : public BasePort {
  protected:
    BaseGMIO(const std::string &name, GraphImplPtr impl) : BasePort(name, impl) {}

  public:
    virtual ~BaseGMIO() = default;
};

class InputGMIO : public BaseGMIO {
  private:
    std::shared_ptr<GmioApiImpl> apiImpl;

  public:
    InputGMIO() : BaseGMIO("", nullptr), apiImpl(nullptr) {}
    InputGMIO(const std::string &name, GraphImplPtr impl, std::shared_ptr<GmioApiImpl> api)
        : BaseGMIO(name, impl), apiImpl(api) {}

    void gm2aie_nb(char *data, int len);
    void gm2aie(char *data, int len);
};

class OutputGMIO : public BaseGMIO {
  private:
    std::shared_ptr<GmioApiImpl> apiImpl;

  public:
    OutputGMIO() : BaseGMIO("", nullptr), apiImpl(nullptr) {}
    OutputGMIO(const std::string &name, GraphImplPtr impl, std::shared_ptr<GmioApiImpl> api)
        : BaseGMIO(name, impl), apiImpl(api) {}

    void aie2gm_nb(char *data, int len);
    void aie2gm(char *data, int len);
    void gmio_wait(bool syncmode = true);
};

class BasePLIO : public BasePort {
  protected:
    BasePLIO(const std::string &name, GraphImplPtr impl) : BasePort(name, impl) {}

  public:
    virtual ~BasePLIO() = default;
};

class InputPLIO : public BasePLIO {
  public:
    InputPLIO() : BasePLIO("", nullptr) {}
    InputPLIO(const std::string &name, GraphImplPtr impl) : BasePLIO(name, impl) {}
};

class OutputPLIO : public BasePLIO {
  public:
    OutputPLIO() : BasePLIO("", nullptr) {}
    OutputPLIO(const std::string &name, GraphImplPtr impl) : BasePLIO(name, impl) {}
};

class RTP : public BasePort {
  private:
    std::shared_ptr<RtpApiImpl> apiImpl;

  public:
    RTP() : BasePort("", nullptr), apiImpl(nullptr) {}
    RTP(const std::string &name, GraphImplPtr impl, std::shared_ptr<RtpApiImpl> api = nullptr)
        : BasePort(name, impl), apiImpl(api) {}

    void update(void *data, size_t len);
    void read(char *data, int len);
};

class SharedBuffer : public BasePort {
  private:
    std::shared_ptr<SharedBufferApiImpl> apiImpl;

  public:
    SharedBuffer() : BasePort("", nullptr), apiImpl(nullptr) {}
    SharedBuffer(const std::string &name, GraphImplPtr impl, std::shared_ptr<SharedBufferApiImpl> api = nullptr)
        : BasePort(name, impl), apiImpl(api) {}

    void update(void *data, size_t len);
};

class ExternalBufferPort : public BasePort {
  private:
    std::shared_ptr<ExternalBufferApiImpl> apiImpl;

  public:
    ExternalBufferPort() : BasePort("", nullptr), apiImpl(nullptr) {}
    ExternalBufferPort(const std::string &name, GraphImplPtr impl, std::shared_ptr<ExternalBufferApiImpl> api)
        : BasePort(name, impl), apiImpl(api) {}

    void wait();
    void gm2aie_nb();
    void aie2gm_nb();
};

class ExternalBuffer : public BasePort {
  private:
    std::string currentPortName;
    std::shared_ptr<ExternalBufferApiImpl> apiImpl;

  public:
    ExternalBuffer() : BasePort("", nullptr), currentPortName(""), apiImpl(nullptr) {}
    ExternalBuffer(const std::string &name, GraphImplPtr impl, std::shared_ptr<ExternalBufferApiImpl> api)
        : BasePort(name, impl), currentPortName(""), apiImpl(api) {}

    void setAddress(void *ptr);
    void wait(const std::string &port_name);
    void gm2aie_nb(const std::string &port_name);
    void aie2gm_nb(const std::string &port_name);
    void setupPingPongBuffers(void *const ptr1, void *const ptr2);
    void gm2aie_nb();
    void aie2gm_nb();
};

} // namespace detail
} // namespace aeg

#endif // AIE_BAREMETAL_API_H
