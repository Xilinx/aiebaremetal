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
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unistd.h>
#include <utility>

#include "aeg_log.h"
#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__) || defined(__ZEPHYR__) || defined(__AIELINUX__))
#include "xscugic.h"
#endif

#define PS_SO "ps_i"

typedef int32_t PROFILE_ID;

namespace aeg {
enum class aeg_err : int32_t {
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
extern "C" {
extern const char binary_app_elf_start[];
extern const char binary_app_elf_end[];
}
enum class io_profiling_option : int32_t {
    io_total_stream_running_to_idle_cycles = 0x20,
    io_stream_start_to_bytes_transferred_cycles,
    io_stream_start_difference_cycles,
    io_stream_running_event_count
};

inline constexpr int32_t io_total_stream_running_to_idle_cycles =
    static_cast<int32_t>(io_profiling_option::io_total_stream_running_to_idle_cycles);
inline constexpr int32_t io_stream_start_to_bytes_transferred_cycles =
    static_cast<int32_t>(io_profiling_option::io_stream_start_to_bytes_transferred_cycles);
inline constexpr int32_t io_stream_start_difference_cycles =
    static_cast<int32_t>(io_profiling_option::io_stream_start_difference_cycles);
inline constexpr int32_t io_stream_running_event_count =
    static_cast<int32_t>(io_profiling_option::io_stream_running_event_count);

/**
 * Descriptor for one in-memory CDO (Configuration Data Object) blob.
 *
 * An array of these is handed to AIEGraph::init() so the CDOs are applied as
 * part of bring-up, before any runtime configuration touches the array. See
 * AIEGraph::init() for why the ordering matters.
 */
struct cdo_blob {
    const void *data; ///< Pointer to the CDO blob in memory.
    std::size_t len;  ///< Length of the CDO blob in bytes.
};

namespace detail {
using GraphImplPtr = void *;
} // namespace detail
} // namespace aeg

class AIEGraph;
using BaremetalGraph = AIEGraph;

struct AIEGraphImpl;
extern "C" {
void abr_init_isr();
void dump_uc_app();
void abr_error_isr(void *data);
}

#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__) || defined(__ZEPHYR__) || defined(__AIELINUX__))
extern "C" {
extern XScuGic xInterruptController;
}
#endif

class AIEGraph {
  public:
    explicit AIEGraph(std::string_view GraphName);
    ~AIEGraph();


    AIEGraph(const AIEGraph &) = delete;
    AIEGraph &operator=(const AIEGraph &) = delete;
    AIEGraph(AIEGraph &&) = delete;
    AIEGraph &operator=(AIEGraph &&) = delete;
    void *malloc(size_t len) const;
    aeg::aeg_err free(void *mem) const;
    aeg::aeg_err selftestexit() const;
    aeg::aeg_err clock_enable() const;
    aeg::aeg_err clock_disable() const;

    aeg::aeg_err error_handling_cb_init(void *cb, void *priv) const;

    /**
     * Bring the graph up, optionally applying a set of CDO blobs first.
     *
     * When @p blobs is non-empty each blob is applied (in array order) *before*
     * the runtime configures GMIOs, RTPs and buffers. That ordering is
     * mandatory, not cosmetic: a CDO set containing aie_cdo_reset.bin performs a
     * column reset and an NPI shim reset of the whole array, which clears the
     * shim DMA channel-enable bits that the GMIO setup programs -- and no CDO
     * restores them. Applying CDOs after the GMIO setup therefore leaves every
     * shim DMA channel disabled and hangs the first gmio wait in
     * XAie_DmaWaitForDone. Folding the load into init() makes that order
     * impossible to get wrong from the caller side.
     *
     * Calling init() with no arguments keeps the previous behaviour (no CDOs).
     *
     * @param blobs Array of CDO blobs to apply, or nullptr for none.
     * @param count Number of entries in @p blobs.
     * @return aeg_err::ok on success; the first failing blob's error otherwise.
     */
    aeg::aeg_err init(const aeg::cdo_blob *blobs = nullptr, std::size_t count = 0U);

    /**
     * Load a CDO (Configuration Data Object) blob resident in memory.
     *
     * Prefer passing the blobs to init() instead: calling this directly after
     * init() has run re-introduces the ordering hazard described there.
     *
     * @param addr Base memory address of the in-memory CDO blob.
     * @param len  Length of the CDO blob in bytes.
     * @return aeg_err::ok on success.
     */
    aeg::aeg_err load_cdo(uintptr_t addr, size_t len) const;
#ifdef __BAREMETAL_UTESTS__
    void reset_xaiemem();
    void reset_GraphApi();
    void reset_EventApi();
#endif
    aeg::aeg_err run(int32_t num = -1) const;
    aeg::aeg_err gm2aie_nb(const char *name, char *data, int32_t len) const;
    aeg::aeg_err aie2gm_nb(const char *name, char *data, int32_t len) const;
    aeg::aeg_err gm2aie(const char *name, char *data, int32_t len) const;
    aeg::aeg_err aie2gm(const char *name, char *data, int32_t len) const;
    aeg::aeg_err wait() const;
    aeg::aeg_err end() const;
    aeg::aeg_err gmio_wait(std::string_view gmio_name, bool syncmode = true) const;
    aeg::aeg_err update(std::string_view port_name, void *data, size_t len) const;
    aeg::aeg_err read(std::string_view port_name, char *data, int32_t len) const;

    // Internal Debug APIs
    char *dumpcorelog(int32_t row, int32_t col) const;
    void *getaiedevinst() const;

    // get the aie error report after doing the backtrack.
    void *BacktrackErrors() const;

    // External Buffer APIs
    aeg::aeg_err setAddress(const char *externalbuffer_name, void *ptr) const;
    aeg::aeg_err wait(const char *externalbuffer_name, std::string_view port_name) const;
    aeg::aeg_err gm2aie_nb(const char *externalbuffer_name, std::string_view port_name) const;
    aeg::aeg_err aie2gm_nb(const char *externalbuffer_name, std::string_view port_name) const;
    aeg::aeg_err setupPingPongBuffers(const char *externalbuffer_name, void *const ptr1, void *const ptr2, bool sync_cache = true) const;

    // Profiling APIs
    PROFILE_ID start_profiling(std::string_view port_name, int32_t option, uint32_t value = 0) const;
    PROFILE_ID start_profiling(std::string_view port_name1, std::string_view port_name2, int32_t option,
                               uint32_t value = 0) const;
    int64_t read_profiling(PROFILE_ID h) const;
    aeg::aeg_err stop_profiling(PROFILE_ID h) const;

    aeg::detail::GraphImplPtr getGraphImpl() const;

  private:
    std::shared_ptr<AIEGraphImpl> graph;
    bool isInitialized;

    bool check_initialized_impl() const;
};

namespace gmioapiimpl {
class GmioApiImpl;
}
namespace externalbufferapiimpl {
class ExternalBufferApiImpl;
}
namespace rtpapiimpl {
class RtpApiImpl;
}
namespace sharedbufferapiimpl {
class SharedBufferApiImpl;
}
namespace aeg {
namespace detail {

using gmioapiimpl::GmioApiImpl;
using externalbufferapiimpl::ExternalBufferApiImpl;
using rtpapiimpl::RtpApiImpl;
using sharedbufferapiimpl::SharedBufferApiImpl;

// Port objects keep the name as a non-owning view, so the caller's character
// data must outlive the port object. String literals (what the generator emits
// in generated_graphs.h) and anything else with static storage duration satisfy
// this; a std::string temporary, a local std::string, or a local char buffer
// does not, and leaves portName dangling.
//
// The requirement applies only to names a port object retains: the constructors
// below and bindPort()/bind(). The AIEGraph methods that take a name --
// update(), read(), gmio_wait(), wait(), gm2aie_nb(), aie2gm_nb() -- only borrow
// it for the duration of the call, and AIEGraph copies its own graph name, so
// temporaries are safe in those cases.
class BasePort {
  public:
    std::string_view portName;
    GraphImplPtr graphImpl;
    BasePort() = delete;
    BasePort(std::string_view name, GraphImplPtr impl) : portName(name), graphImpl(impl) {}

    std::string_view getName() const { return portName; }

    PROFILE_ID start_profiling(int32_t option, uint32_t value = 0) const;
    int64_t read_profiling(PROFILE_ID h) const;
    aeg_err stop_profiling(PROFILE_ID h) const;

  protected:
    ~BasePort() = default;
    BasePort(const BasePort&) = default;
    BasePort& operator=(const BasePort&) = default;
    BasePort(BasePort&&) = default;
    BasePort& operator=(BasePort&&) = default;

    void bindPort(std::string_view name, GraphImplPtr impl)
    {
        portName = name;
        graphImpl = impl;
    }
};

class BaseGMIO : public BasePort {
  protected:
    BaseGMIO(std::string_view name, GraphImplPtr impl) : BasePort(name, impl) {}
    BaseGMIO(const BaseGMIO &) = delete;
    BaseGMIO &operator=(const BaseGMIO &) = delete;
    BaseGMIO(BaseGMIO &&) = delete;
    BaseGMIO &operator=(BaseGMIO &&) = delete;

  public:
    virtual ~BaseGMIO() = default;
};

class InputGMIO : public BaseGMIO {
  private:
    std::shared_ptr<GmioApiImpl> apiImpl;

  public:
    InputGMIO() : BaseGMIO("", nullptr), apiImpl(nullptr) {}
    InputGMIO(std::string_view name, GraphImplPtr impl, std::shared_ptr<GmioApiImpl> api)
        : BaseGMIO(name, impl), apiImpl(std::move(api)) {}

    void bind(std::string_view name, GraphImplPtr impl, std::shared_ptr<GmioApiImpl> api)
    {
        bindPort(name, impl);
        apiImpl = std::move(api);
    }

    aeg_err gm2aie_nb(char *data, int32_t len);
    aeg_err gm2aie(char *data, int32_t len);
};

class OutputGMIO : public BaseGMIO {
  private:
    std::shared_ptr<GmioApiImpl> apiImpl;

  public:
    OutputGMIO() : BaseGMIO("", nullptr), apiImpl(nullptr) {}
    OutputGMIO(std::string_view name, GraphImplPtr impl, std::shared_ptr<GmioApiImpl> api)
        : BaseGMIO(name, impl), apiImpl(std::move(api)) {}

    void bind(std::string_view name, GraphImplPtr impl, std::shared_ptr<GmioApiImpl> api)
    {
        bindPort(name, impl);
        apiImpl = std::move(api);
    }

    aeg_err aie2gm_nb(char *data, int32_t len);
    aeg_err aie2gm(char *data, int32_t len);
    aeg_err gmio_wait(bool syncmode = true);
};

class BasePLIO : public BasePort {
  protected:
    BasePLIO(std::string_view name, GraphImplPtr impl) : BasePort(name, impl) {}
    BasePLIO(const BasePLIO &) = delete;
    BasePLIO &operator=(const BasePLIO &) = delete;
    BasePLIO(BasePLIO &&) = delete;
    BasePLIO &operator=(BasePLIO &&) = delete;

  public:
    virtual ~BasePLIO() = default;
};

class InputPLIO : public BasePLIO {
  public:
    InputPLIO() : BasePLIO("", nullptr) {}
    InputPLIO(std::string_view name, GraphImplPtr impl) : BasePLIO(name, impl) {}

    void bind(std::string_view name, GraphImplPtr impl) { bindPort(name, impl); }
};

class OutputPLIO : public BasePLIO {
  public:
    OutputPLIO() : BasePLIO("", nullptr) {}
    OutputPLIO(std::string_view name, GraphImplPtr impl) : BasePLIO(name, impl) {}

    void bind(std::string_view name, GraphImplPtr impl) { bindPort(name, impl); }
};

class RTP : public BasePort {
  private:
    std::shared_ptr<RtpApiImpl> apiImpl;

  public:
    RTP() : BasePort("", nullptr), apiImpl(nullptr) {}
    RTP(std::string_view name, GraphImplPtr impl, std::shared_ptr<RtpApiImpl> api = nullptr)
        : BasePort(name, impl), apiImpl(std::move(api)) {}

    aeg_err update(void *data, size_t len);
    aeg_err read(char *data, int32_t len);
};

class SharedBuffer : public BasePort {
  private:
    std::shared_ptr<SharedBufferApiImpl> apiImpl;

  public:
    SharedBuffer() : BasePort("", nullptr), apiImpl(nullptr) {}
    SharedBuffer(std::string_view name, GraphImplPtr impl, std::shared_ptr<SharedBufferApiImpl> api = nullptr)
        : BasePort(name, impl), apiImpl(std::move(api)) {}

    aeg_err update(void *data, size_t len);
};

class ExternalBufferPort : public BasePort {
  private:
    std::shared_ptr<ExternalBufferApiImpl> apiImpl;

  public:
    ExternalBufferPort() : BasePort("", nullptr), apiImpl(nullptr) {}
    ExternalBufferPort(std::string_view name, GraphImplPtr impl, std::shared_ptr<ExternalBufferApiImpl> api)
        : BasePort(name, impl), apiImpl(std::move(api)) {}

    aeg_err wait();
    aeg_err gm2aie_nb();
    aeg_err aie2gm_nb();
};

class ExternalBuffer : public BasePort {
  private:
    std::string_view currentPortName;
    std::shared_ptr<ExternalBufferApiImpl> apiImpl;

  public:
    ExternalBuffer() : BasePort("", nullptr), currentPortName{}, apiImpl(nullptr) {}
    ExternalBuffer(std::string_view name, GraphImplPtr impl, std::shared_ptr<ExternalBufferApiImpl> api)
        : BasePort(name, impl), currentPortName{}, apiImpl(std::move(api)) {}

    aeg_err setAddress(void *ptr);
    aeg_err wait(std::string_view port_name);
    aeg_err gm2aie_nb(std::string_view port_name);
    aeg_err aie2gm_nb(std::string_view port_name);
    aeg_err setupPingPongBuffers(void *const ptr1, void *const ptr2 ,bool sync_cache = true);
    aeg_err gm2aie_nb();
    aeg_err aie2gm_nb();
};

} // namespace detail
} // namespace aeg
#endif // AIE_BAREMETAL_API_H
