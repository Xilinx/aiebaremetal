/**<!--
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
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
#ifndef AEG_BAREMETAL_GRAPH_SIM_BASE_H
#define AEG_BAREMETAL_GRAPH_SIM_BASE_H

#include "aeg_baremetal_api.h"

class AIEGraphSim;

#include "aeg_baremetal_api.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <array>

class BaremetalGraphSimBase {
  public:
    virtual ~BaremetalGraphSimBase() = default;
    virtual aeg::aeg_err selftestexit() = 0;
    virtual aeg::aeg_err clock_enable() = 0;
    virtual aeg::aeg_err clock_disable() = 0;
    virtual aeg::aeg_err error_handling_cb_init(void *cb, void *priv) = 0;
    virtual aeg::aeg_err init() = 0;
    virtual aeg::aeg_err load_cdo(uintptr_t addr, size_t len) = 0;

    /// @brief Apply @p count CDO blobs, then run init().
    ///
    /// CDOs must be applied before init() configures GMIOs: a set containing
    /// aie_cdo_reset.bin resets the array and clears the shim DMA channel
    /// enables that the GMIO setup programs, which no CDO restores. See
    /// AIEGraph::init(). This default implementation composes the two
    /// primitives above so existing implementers need no change.
    virtual aeg::aeg_err init(const aeg::cdo_blob *blobs, std::size_t count) {
        if ((blobs == nullptr) && (count != 0U)) {
            return aeg::aeg_err::null_argument;
        }
        for (std::size_t i = 0U; i < count; ++i) {
            if (blobs[i].data == nullptr) {
                return aeg::aeg_err::null_argument;
            }
            const aeg::aeg_err status = load_cdo(
                reinterpret_cast<uintptr_t>(blobs[i].data), blobs[i].len);
            if (status != aeg::aeg_err::ok) {
                return status;
            }
        }
        return init();
    }
    virtual aeg::aeg_err run(int32_t num) = 0;
    virtual aeg::aeg_err gm2aie_nb(char *name, char *data, int32_t len) = 0;
    virtual aeg::aeg_err aie2gm_nb(char *name, char *data, int32_t len) = 0;
    virtual aeg::aeg_err gm2aie(char *name, char *data, int32_t len) = 0;
    virtual aeg::aeg_err aie2gm(char *name, char *data, int32_t len) = 0;
    virtual aeg::aeg_err wait() = 0;

    // External Buffer APIs
    virtual aeg::aeg_err setAddress(char *externalbuffer_name, void *ptr) = 0;
    virtual aeg::aeg_err wait(char *externalbuffer_name, std::string_view port_name) = 0;
    virtual aeg::aeg_err gm2aie_nb(char *externalbuffer_name, std::string_view port_name) = 0;
    virtual aeg::aeg_err aie2gm_nb(char *externalbuffer_name, std::string_view port_name) = 0;
    virtual aeg::aeg_err setupPingPongBuffers(char *externalbuffer_name, void *const ptr1, void *const ptr2, bool sync_cache = true) = 0;
    // End external buffer APIs

    virtual aeg::aeg_err end() = 0;
    virtual aeg::aeg_err gmio_wait(std::string_view gmio_name, bool syncmode = true) = 0;
    virtual void *malloc(uint32_t len) = 0;
    virtual aeg::aeg_err free(void *addr) = 0;

    virtual aeg::aeg_err update(const std::string_view port_name, void *data, size_t len) = 0;
    virtual aeg::aeg_err read(const std::string_view port_name, char *data, int32_t len) = 0;
    virtual char *dumpcorelog(int32_t row, int32_t col) = 0;
    virtual void *getaiedevinst() = 0;
    virtual void *BacktrackErrors() = 0;
    virtual void gdbpoint() = 0;
    virtual int32_t start_profiling(const std::string_view port_name, int32_t option, uint32_t value = 0) = 0;
    virtual int32_t start_profiling(const std::string_view port_name1, const std::string_view port_name2, int32_t option,
                                uint32_t value = 0) = 0;
    virtual int64_t read_profiling(int32_t h) = 0;
    virtual aeg::aeg_err stop_profiling(int32_t h) = 0;

    virtual AIEGraphSim *getGraphSimImpl() = 0;
    virtual void *getGraphHandle() = 0;
};

#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
namespace aeg {
namespace detail {

inline PROFILE_ID BasePort::start_profiling(int32_t option, uint32_t value) const {
    if (portName.empty()) {
        AEG_ERROR("%s: invalid profiling port name", "start_profiling");
        return -1;
    }
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "start_profiling");
        return -1;
    }
    return g->start_profiling(portName, option, value);
}

inline int64_t BasePort::read_profiling(PROFILE_ID h) const {
    if (h < 0) {
        AEG_ERROR("%s: invalid profiling handle=%d", "read_profiling", h);
        return static_cast<int64_t>(-1);
    }
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "read_profiling");
        return static_cast<int64_t>(-1);
    }
    return g->read_profiling(h);
}

inline aeg_err BasePort::stop_profiling(PROFILE_ID h) const {
    if (h < 0) {
        AEG_ERROR("%s: invalid profiling handle=%d", "stop_profiling", h);
        return aeg_err::invalid_state;
    }
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "stop_profiling");
        return aeg_err::backend_not_ready;
    }
    return g->stop_profiling(h);
}

inline aeg_err InputGMIO::gm2aie_nb(char *data, int32_t len) {
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "gm2aie_nb");
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", "gm2aie_nb");
        return aeg_err::invalid_length;
    }
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "gm2aie_nb");
        return aeg_err::backend_not_ready;
    }
    return g->gm2aie_nb(const_cast<char *>(portName.data()), data, len);
}

inline aeg_err InputGMIO::gm2aie(char *data, int32_t len) {
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "gm2aie");
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", "gm2aie");
        return aeg_err::invalid_length;
    }
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "gm2aie");
        return aeg_err::backend_not_ready;
    }
    return g->gm2aie(const_cast<char *>(portName.data()), data, len);
}

inline aeg_err OutputGMIO::aie2gm_nb(char *data, int32_t len) {
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "aie2gm_nb");
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", "aie2gm_nb");
        return aeg_err::invalid_length;
    }
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "aie2gm_nb");
        return aeg_err::backend_not_ready;
    }
    return g->aie2gm_nb(const_cast<char *>(portName.data()), data, len);
}

inline aeg_err OutputGMIO::aie2gm(char *data, int32_t len) {
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "aie2gm");
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", "aie2gm");
        return aeg_err::invalid_length;
    }
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "aie2gm");
        return aeg_err::backend_not_ready;
    }
    return g->aie2gm(const_cast<char *>(portName.data()), data, len);
}

inline aeg_err OutputGMIO::gmio_wait(bool syncmode) {
    if (!syncmode) {
        AEG_ERROR("%s: invalid syncmode argument", "gmio_wait");
        return aeg_err::user_error;
    }
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "gmio_wait");
        return aeg_err::backend_not_ready;
    }
    return g->gmio_wait(portName, syncmode);
}

inline aeg_err RTP::update(void *data, size_t len) {
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "update");
        return aeg_err::null_argument;
    }
    if (len == 0U) {
        AEG_ERROR("%s: invalid length argument", "update");
        return aeg_err::invalid_length;
    }
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "update");
        return aeg_err::backend_not_ready;
    }
    return g->update(portName, data, len);
}

inline aeg_err RTP::read(char *data, int32_t len) {
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "read");
        return aeg_err::null_argument;
    }
    if (len <= 0) {
        AEG_ERROR("%s: invalid length argument", "read");
        return aeg_err::invalid_length;
    }
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "read");
        return aeg_err::backend_not_ready;
    }
    return g->read(portName, data, len);
}

inline aeg_err SharedBuffer::update(void *data, size_t len) {
    if (data == nullptr) {
        AEG_ERROR("%s: null argument", "update");
        return aeg_err::null_argument;
    }
    if (len == 0U) {
        AEG_ERROR("%s: invalid length argument", "update");
        return aeg_err::invalid_length;
    }
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "update");
        return aeg_err::backend_not_ready;
    }
    return g->update(portName, data, len);
}

inline aeg_err ExternalBufferPort::wait() {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "wait");
        return aeg_err::backend_not_ready;
    }
    size_t lastDot = portName.rfind('.');
    if (lastDot == std::string::npos) {
        AEG_ERROR("%s: invalid port name argument", "wait");
        return aeg_err::invalid_name;
    }
    char bufferName[256] = {};
    std::size_t bufLen = 0U;
    for (std::size_t i = 0U; (i < lastDot) && (bufLen < (sizeof(bufferName) - 1U)); ++i) {
        bufferName[bufLen] = portName[i];
        ++bufLen;
    }
    return g->wait(bufferName, portName);
}

inline aeg_err ExternalBufferPort::gm2aie_nb() {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "gm2aie_nb");
        return aeg_err::backend_not_ready;
    }
    size_t lastDot = portName.rfind('.');
    if (lastDot == std::string::npos) {
        AEG_ERROR("%s: invalid port name argument", "gm2aie_nb");
        return aeg_err::invalid_name;
    }
    char bufferName[256] = {};
    std::size_t bufLen = 0U;
    for (std::size_t i = 0U; (i < lastDot) && (bufLen < (sizeof(bufferName) - 1U)); ++i) {
        bufferName[bufLen] = portName[i];
        ++bufLen;
    }
    return g->gm2aie_nb(bufferName, portName);
}

inline aeg_err ExternalBufferPort::aie2gm_nb() {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "aie2gm_nb");
        return aeg_err::backend_not_ready;
    }
    size_t lastDot = portName.rfind('.');
    if (lastDot == std::string::npos) {
        AEG_ERROR("%s: invalid port name argument", "aie2gm_nb");
        return aeg_err::invalid_name;
    }
    char bufferName[256] = {};
    std::size_t bufLen = 0U;
    for (std::size_t i = 0U; (i < lastDot) && (bufLen < (sizeof(bufferName) - 1U)); ++i) {
        bufferName[bufLen] = portName[i];
        ++bufLen;
    }
    return g->aie2gm_nb(bufferName, portName);
}

inline aeg_err ExternalBuffer::setAddress(void *ptr) {
    if (ptr == nullptr) {
        AEG_ERROR("%s: null argument", "setAddress");
        return aeg_err::null_argument;
    }
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "setAddress");
        return aeg_err::backend_not_ready;
    }
    return g->setAddress(const_cast<char *>(portName.data()), ptr);
}

inline aeg_err ExternalBuffer::wait(std::string_view port_name) {
    if (port_name.empty()) {
        AEG_ERROR("%s: invalid port name argument", "wait");
        return aeg_err::invalid_name;
    }
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "wait");
        return aeg_err::backend_not_ready;
    }
    return g->wait(const_cast<char *>(portName.data()), port_name);
}

inline aeg_err ExternalBuffer::gm2aie_nb(std::string_view port_name) {
    if (port_name.empty()) {
        AEG_ERROR("%s: invalid port name argument", "gm2aie_nb");
        return aeg_err::invalid_name;
    }
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "gm2aie_nb");
        return aeg_err::backend_not_ready;
    }
    return g->gm2aie_nb(const_cast<char *>(portName.data()), port_name);
}

inline aeg_err ExternalBuffer::aie2gm_nb(std::string_view port_name) {
    if (port_name.empty()) {
        AEG_ERROR("%s: invalid port name argument", "aie2gm_nb");
        return aeg_err::invalid_name;
    }
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "aie2gm_nb");
        return aeg_err::backend_not_ready;
    }
    return g->aie2gm_nb(const_cast<char *>(portName.data()), port_name);
}

inline aeg_err ExternalBuffer::setupPingPongBuffers(void *const ptr1, void *const ptr2, bool sync_cache) {
    if ((ptr1 == nullptr) || (ptr2 == nullptr)) {
        AEG_ERROR("%s: null argument", "setupPingPongBuffers");
        return aeg_err::null_argument;
    }
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        AEG_ERROR("%s: backend not ready", "setupPingPongBuffers");
        return aeg_err::backend_not_ready;
    }
    return g->setupPingPongBuffers(const_cast<char *>(portName.data()), ptr1, ptr2, sync_cache);
}

inline aeg_err ExternalBuffer::gm2aie_nb() {
    if (portName.empty()) {
        AEG_ERROR("%s: invalid port name argument", "gm2aie_nb");
        return aeg_err::invalid_name;
    }
    std::array<char, 256> buf = {};
    std::size_t len = 0U;
    std::size_t i = 0U;
    while ((i < portName.size()) && (len < (buf.size() - 1U))) {
        buf[len] = portName[i];
        ++len;
        ++i;
    }
    const char suffix[] = ".out[0]";
    i = 0U;
    while ((suffix[i] != '\0') && (len < (buf.size() - 1U))) {
        buf[len] = suffix[i];
        ++len;
        ++i;
    }
    return gm2aie_nb(std::string_view(buf.data(), len));
}

inline aeg_err ExternalBuffer::aie2gm_nb() {
    if (portName.empty()) {
        AEG_ERROR("%s: invalid port name argument", "aie2gm_nb");
        return aeg_err::invalid_name;
    }
    std::array<char, 256> buf = {};
    std::size_t len = 0U;
    std::size_t i = 0U;
    while ((i < portName.size()) && (len < (buf.size() - 1U))) {
        buf[len] = portName[i];
        ++len;
        ++i;
    }
    const char suffix[] = ".in[0]";
    i = 0U;
    while ((suffix[i] != '\0') && (len < (buf.size() - 1U))) {
        buf[len] = suffix[i];
        ++len;
        ++i;
    }
    return aie2gm_nb(std::string_view(buf.data(), len));
}

} // namespace detail
} // namespace aeg
#endif // GRAPH_COMPILE_OR_SIM

#endif // AEG_BAREMETAL_GRAPH_SIM_BASE_H
