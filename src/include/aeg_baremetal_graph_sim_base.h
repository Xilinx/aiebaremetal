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
#ifndef __AEG_BAREMETAL_GRAPH_SIM_BASE_H__
#define __AEG_BAREMETAL_GRAPH_SIM_BASE_H__

#include "aeg_baremetal_api.h"

class AIEGraphSim;

#include "aeg_baremetal_api.h"
#include <cstdint>

class BaremetalGraphSimBase {
  public:
    virtual aeg_err selftestexit() = 0;
    virtual aeg_err clock_enable() = 0;
    virtual aeg_err clock_disable() = 0;
    virtual aeg_err error_handling_cb_init(void *cb, void *priv) = 0;
    virtual aeg_err init() = 0;
    virtual aeg_err run(int num) = 0;
    virtual aeg_err gm2aie_nb(char *name, char *data, int len) = 0;
    virtual aeg_err aie2gm_nb(char *name, char *data, int len) = 0;
    virtual aeg_err gm2aie(char *name, char *data, int len) = 0;
    virtual aeg_err aie2gm(char *name, char *data, int len) = 0;
    virtual aeg_err wait() = 0;

    // External Buffer APIs
    virtual aeg_err setAddress(char *externalbuffer_name, void *ptr) = 0;
    virtual aeg_err wait(char *externalbuffer_name, std::string port_name) = 0;
    virtual aeg_err gm2aie_nb(char *externalbuffer_name, std::string port_name) = 0;
    virtual aeg_err aie2gm_nb(char *externalbuffer_name, std::string port_name) = 0;
    virtual aeg_err setupPingPongBuffers(char *externalbuffer_name, void *const ptr1, void *const ptr2) = 0;
    // End external buffer APIs

    virtual aeg_err end() = 0;
    virtual aeg_err gmio_wait(std::string gmio_name, bool syncmode = true) = 0;
    virtual void *malloc(uint32_t len) = 0;
    virtual aeg_err free(void *addr) = 0;

    virtual aeg_err update(const std::string &port_name, void *data, size_t len) = 0;
    virtual aeg_err read(const std::string &port_name, char *data, int len) = 0;
    virtual char *dumpcorelog(int row, int col) = 0;
    virtual void *getaiedevinst() = 0;
    virtual void *BacktrackErrors() = 0;
    virtual void gdbpoint() = 0;
    virtual int start_profiling(const std::string &port_name, int option, uint32_t value = 0) = 0;
    virtual int start_profiling(const std::string &port_name1, const std::string &port_name2, int option,
                                uint32_t value = 0) = 0;
    virtual long long read_profiling(int h) = 0;
    virtual aeg_err stop_profiling(int h) = 0;

    virtual AIEGraphSim *getGraphSimImpl() = 0;
    virtual void *getGraphHandle() = 0;
};

#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
namespace aeg {
namespace detail {

inline PROFILE_ID BasePort::start_profiling(int option, uint32_t value) {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (g) {
        return g->start_profiling(portName, option, value);
    }
    return -1;
}

inline long long BasePort::read_profiling(PROFILE_ID h) {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (g) {
        return g->read_profiling(h);
    }
    return -1;
}

inline void BasePort::stop_profiling(PROFILE_ID h) {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (g) {
        g->stop_profiling(h);
    }
}

inline void InputGMIO::gm2aie_nb(char *data, int len) {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (g) {
        g->gm2aie_nb(const_cast<char *>(portName.c_str()), data, len);
    }
}

inline void InputGMIO::gm2aie(char *data, int len) { gm2aie_nb(data, len); }

inline void OutputGMIO::aie2gm_nb(char *data, int len) {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (g) {
        g->aie2gm_nb(const_cast<char *>(portName.c_str()), data, len);
    }
}

inline void OutputGMIO::aie2gm(char *data, int len) { aie2gm_nb(data, len); }

inline void OutputGMIO::gmio_wait(bool syncmode) {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (g) {
        g->gmio_wait(portName, syncmode);
    }
}

inline void RTP::update(void *data, size_t len) {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (g) {
        g->update(portName, data, len);
    }
}

inline void RTP::read(char *data, int len) {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (g) {
        g->read(portName, data, len);
    }
}

inline void SharedBuffer::update(void *data, size_t len) {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (g) {
        g->update(portName, data, len);
    }
}

inline void ExternalBufferPort::wait() {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        return;
    }
    size_t lastDot = portName.rfind('.');
    if (lastDot == std::string::npos) {
        return;
    }
    std::string bufferName = portName.substr(0, lastDot);
    g->wait(const_cast<char *>(bufferName.c_str()), portName);
}

inline void ExternalBufferPort::gm2aie_nb() {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        return;
    }
    size_t lastDot = portName.rfind('.');
    if (lastDot == std::string::npos) {
        return;
    }
    std::string bufferName = portName.substr(0, lastDot);
    g->gm2aie_nb(const_cast<char *>(bufferName.c_str()), portName);
}

inline void ExternalBufferPort::aie2gm_nb() {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (!g) {
        return;
    }
    size_t lastDot = portName.rfind('.');
    if (lastDot == std::string::npos) {
        return;
    }
    std::string bufferName = portName.substr(0, lastDot);
    g->aie2gm_nb(const_cast<char *>(bufferName.c_str()), portName);
}

inline void ExternalBuffer::setAddress(void *ptr) {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (g) {
        g->setAddress(const_cast<char *>(portName.c_str()), ptr);
    }
}

inline void ExternalBuffer::wait(const std::string &port_name) {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (g) {
        g->wait(const_cast<char *>(portName.c_str()), port_name);
    }
}

inline void ExternalBuffer::gm2aie_nb(const std::string &port_name) {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (g) {
        g->gm2aie_nb(const_cast<char *>(portName.c_str()), port_name);
    }
}

inline void ExternalBuffer::aie2gm_nb(const std::string &port_name) {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (g) {
        g->aie2gm_nb(const_cast<char *>(portName.c_str()), port_name);
    }
}

inline void ExternalBuffer::setupPingPongBuffers(void *const ptr1, void *const ptr2) {
    auto *g = static_cast<BaremetalGraphSimBase *>(graphImpl);
    if (g) {
        g->setupPingPongBuffers(const_cast<char *>(portName.c_str()), ptr1, ptr2);
    }
}

inline void ExternalBuffer::gm2aie_nb() { gm2aie_nb(portName + ".out[0]"); }

inline void ExternalBuffer::aie2gm_nb() { aie2gm_nb(portName + ".in[0]"); }

} // namespace detail
} // namespace aeg
#endif // defined(__AIESIM__) || defined(__ADF_FRONTEND__)

#endif // __AEG_BAREMETAL_GRAPH_SIM_BASE_H__
