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
#ifndef AIE_BAREMETAL_LIB_H
#define AIE_BAREMETAL_LIB_H
#include "common_layer/aeg_api_message.h"
#include <memory>
#include <unistd.h>
#include <xaiengine.h>
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#define GRAPH_COMPILE_OR_SIM 1
#else
#define GRAPH_COMPILE_OR_SIM 0
#endif
#if defined(__AIEBAREMETAL__) || !GRAPH_COMPILE_OR_SIM
#define AEG_HARDWARE 1
#endif
#if GRAPH_COMPILE_OR_SIM
#define AbrGraph BaremetalGraphSim
#define AIEGRAPH AIEGraphSim
#else
#define AbrGraph BaremetalGraph
#define AIEGRAPH AIEGraph
#endif

#include "aeg_baremetal_api.h"
#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "aeg_class_api.h"
#endif

using namespace abr;
using namespace aeg;
extern "C" void aie_driver_test(const XAie_DevInst *);

// TODO: remove this hardcode after aiecompiler add rpu base address into json
#define RPU_AIE_BASE 0x40000000ULL
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include <dlfcn.h>
#include <stdio.h>
#define CREATEGRAPHAPI "createBaremetalGraphInstance"
#define AIE_BAREMETAL_LIB "AIE_BAREMETAL_LIB"

#include "aeg_baremetal_graph_sim_base.h"
#include "aeg_class_api.h"
typedef BaremetalGraphSimBase *(*createBaremetalGraphInstance_t)(const char *name, const uint8_t rpu);
class BaremetalGraphSim {
  public:
    BaremetalGraphSim(std::string GraphName, uint8_t rpu = 0) : isInitialized(false), StoredGraphName(GraphName), StoredRpu(rpu) {
#if !defined(__ADF_FRONTEND__)
        bgraph = createBGraphInstance(GraphName, rpu);
#endif
        AEG_LOG("BaremetalGraphSim created!!!\n");
    }

    ~BaremetalGraphSim() {
#if !defined(__ADF_FRONTEND__)
        if (bgraph) {
            delete bgraph;
        }
        if (handle) {
            dlclose(handle);
        }
#endif
    }

    aeg_err selftestexit() {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        if (bgraph) {
            return bgraph->selftestexit();
        } else {
            return aeg_err::internal_error;
        }
        return aeg_err::ok;
    }

    aeg_err clock_enable() {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        if (bgraph) {
            return bgraph->clock_enable();
        } else {
            return aeg_err::internal_error;
        }
    }

    aeg_err clock_disable() {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        if (bgraph) {
            return bgraph->clock_disable();
        } else {
            return aeg_err::internal_error;
        }
    }

    aeg_err error_handling_cb_init(void *cb, void *priv) {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        if (bgraph) {
            return bgraph->error_handling_cb_init(cb, priv);
        } else {
            return aeg_err::internal_error;
        }
        return aeg_err::ok;
    }

    /// Bring the graph up, optionally applying @p count CDO blobs first.
    /// The CDOs must be applied before the GMIO setup inside init() -- see
    /// AIEGraph::init() for why. Calling init() with no arguments is unchanged.
    aeg_err init(const aeg::cdo_blob *blobs = nullptr, std::size_t count = 0U) {
        if (!bgraph) {
            bgraph = createBGraphInstance(StoredGraphName, StoredRpu);
            fflush(stderr);
        }
        if (bgraph) {
            auto ret = bgraph->init(blobs, count);
            if (ret != aeg_err::ok) {
                return ret;
            }
            isInitialized = true;
            return aeg_err::ok;
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::internal_error;
        }
    }

    aeg_err load_cdo(uintptr_t addr, size_t len) const {
        if (!bgraph) {
            return aeg_err::user_error;
        }
        return bgraph->load_cdo(addr, len);
    }

      aeg_err run(int32_t num = -1) const {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        if (bgraph) {
            return bgraph->run(num);
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::internal_error;
        }
    }

    aeg_err gm2aie_nb(char *name, char *data, int32_t len) {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }

        if (bgraph) {
            return bgraph->gm2aie_nb(name, data, len);
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::internal_error;
        }
    }

    aeg_err aie2gm_nb(char *name, char *data, int32_t len) {
        if (!check_initialized_impl()) {
            return aeg_err::user_error;
          }
        if (bgraph) {
            return bgraph->aie2gm_nb(name, data, len);
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::internal_error;
        }
    }

    aeg_err gm2aie(char *name, char *data, int32_t len) {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        if (bgraph) {
            return bgraph->gm2aie(name, data, len);
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::internal_error;
        }
    }

    aeg_err aie2gm(char *name, char *data, int32_t len) {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        if (bgraph) {
            return bgraph->aie2gm(name, data, len);
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::internal_error;
        }
    }

    aeg_err gmio_wait(std::string gmio_name, bool syncmode = true) {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        aeg_err ret = aeg_err::ok;
        if (bgraph) {
            ret = bgraph->gmio_wait(gmio_name, syncmode);
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::internal_error;
        }
        return ret;
    }

    // External Buffer APIs
    aeg_err setAddress(char *externalbuffer_name, void *ptr) {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        if (bgraph) {
            return bgraph->setAddress(externalbuffer_name, ptr);
        } else {
              AEG_ERROR("External Buffer setaddress failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::internal_error;
        }
        return aeg_err::ok;
    }

    aeg_err wait(char *externalbuffer_name, std::string port_name) {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        if (bgraph) {
            bgraph->wait(externalbuffer_name, port_name);
        } else {
              AEG_ERROR("External Buffer wait failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::user_error;
        }
        return aeg_err::ok;
    }

    aeg_err gm2aie_nb(char *externalbuffer_name, std::string port_name) {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        if (bgraph) {
            bgraph->gm2aie_nb(externalbuffer_name, port_name);
        } else {
              AEG_ERROR("External Buffer gm2aie_nb failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::user_error;
        }
        return aeg_err::ok;
    }

    aeg_err aie2gm_nb(char *externalbuffer_name, std::string port_name) {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        if (bgraph) {
            bgraph->aie2gm_nb(externalbuffer_name, port_name);
        } else {
              AEG_ERROR("External Buffer aie2gm_nb failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::user_error;
        }
        return aeg_err::ok;
    }

    aeg_err setupPingPongBuffers(char *externalbuffer_name, void *const ptr1, void *const ptr2,bool sync_cache = true) {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        if (bgraph) {
            bgraph->setupPingPongBuffers(externalbuffer_name, ptr1, ptr2, sync_cache);
        } else {
              AEG_ERROR("External Buffer setupPingPongBuffers failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::user_error;
        }
        return aeg_err::ok;
    }

    // End external buffer APIs
    aeg_err wait() {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        if (bgraph) {
            return bgraph->wait();
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::internal_error;
        }
    }

    aeg_err end() {
        if (!check_initialized_impl()) {
            return aeg_err::user_error;
          }
        if (bgraph) {
            return bgraph->end();
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::user_error;
        }
        return aeg_err::ok;
    }

    void *malloc(uint32_t len) {
        if (bgraph) {
            return bgraph->malloc(len);
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return nullptr;
        }
    }

    aeg_err free(void *addr) {

        if (bgraph) {
            return bgraph->free(addr);
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::internal_error;
        }
        return aeg_err::ok;
    }

    aeg_err update(const std::string &port_name, void *data, size_t len) {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        if (bgraph) {
            return bgraph->update(port_name, data, len);
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::user_error;
        }
    }

    aeg_err read(const std::string &port_name, char *data, int32_t len) {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        if (bgraph) {
            return bgraph->read(port_name, data, len);
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
              AEG_ERROR("%s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::user_error;
        }
    }

    char *dumpcorelog(int32_t row, int32_t col) {
        if (!check_initialized_impl()){
            return nullptr;
          }
        if (bgraph) {
            return bgraph->dumpcorelog(row, col);
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return NULL;
        }
    }

    void *getaiedevinst() {
        if (!check_initialized_impl()){
            return nullptr;
          }
        if (bgraph) {
            return bgraph->getaiedevinst();
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
              return nullptr;
        }
    }

    void *BacktrackErrors() {
        if (!check_initialized_impl()){
            return nullptr;
          }
        if (bgraph) {
            return bgraph->BacktrackErrors();
        } else {
              AEG_ERROR("baremetal graph error backtracking failed, please check the %s env\n", AIE_BAREMETAL_LIB);
              return nullptr;
        }
    }

    void gdbpoint() {
        if (!check_initialized_impl()){
            return;
          }
        if (bgraph) {
            return bgraph->gdbpoint();
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return;
        }
    }

      PROFILE_ID start_profiling(const std::string &port_name, int32_t option, uint32_t value = 0) {
        if (!check_initialized_impl()){
            return -1;
          }
        if (bgraph) {
            return bgraph->start_profiling(port_name, option, value);
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return -1;
        }
    }

      PROFILE_ID start_profiling(const std::string &port_name1, const std::string &port_name2, int32_t option,
                                 uint32_t value = 0) {
        if (!check_initialized_impl()){
            return -1;
          }

        if (bgraph) {
            return bgraph->start_profiling(port_name1, port_name2, option, value);
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return -1;
        }
    }

      int64_t read_profiling(int32_t h) {
        if (!check_initialized_impl()){
              return static_cast<int64_t>(-1);
          }

        if (bgraph) {
            return bgraph->read_profiling(h);
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
              return static_cast<int64_t>(-1);
        }
    }

      aeg_err stop_profiling(int32_t h) {
        if (!check_initialized_impl()){
            return aeg_err::user_error;
          }
        if (bgraph) {
            return bgraph->stop_profiling(h);
        } else {
              AEG_ERROR("baremetal graph load failed ,please check the %s env\n", AIE_BAREMETAL_LIB);
            return aeg_err::internal_error;
        }
        return aeg_err::ok;
    }

    AIEGraphSim *getGraphSimImpl() {
        if (bgraph) {
            return bgraph->getGraphSimImpl();
        }
        return nullptr;
    }

    void *getGraphHandle() const { return static_cast<void *>(bgraph); }

    aeg::detail::GraphImplPtr getGraphImpl() const { return static_cast<aeg::detail::GraphImplPtr>(getGraphHandle()); }

  protected:
    void *handle = nullptr;
    BaremetalGraphSimBase *bgraph;
    bool isInitialized;
    std::string StoredGraphName;
    uint8_t StoredRpu = 0;

    bool check_initialized_impl() const {
        if (!isInitialized) {
            AEG_ERROR("BaremetalGraphSim: invalid state (graph not initialized; call init() first)");
            return false;
        }
        return true;
    }

    BaremetalGraphSimBase *createBGraphInstance(std::string gname, uint8_t rpu) {
        const char *ablib = getenv(AIE_BAREMETAL_LIB);
        if (!ablib) {
            AEG_LOG("AIE_BAREMETAL_LIB environment variable not set\n");
            return nullptr;
        }
        AEG_LOG("%d global<- lazy-->  %d %s\n", static_cast<int32_t>(RTLD_GLOBAL), static_cast<int32_t>(RTLD_LAZY), ablib);
        handle = dlopen(ablib, RTLD_LAZY);

        if (handle != nullptr) {

            AEG_LOG("%sload success !\n", ablib);
            createBaremetalGraphInstance_t createInstance =
                (createBaremetalGraphInstance_t)dlsym(handle, CREATEGRAPHAPI);
            if (!createInstance) {
                AEG_LOG("Cannot load symbol: \n");
                dlclose(handle);
                return nullptr;
            }
            BaremetalGraphSimBase *inst = createInstance(gname.c_str(), static_cast<uint8_t>(rpu));
            return inst;
        }
        const char *error = dlerror();
          if (error != nullptr) {
            AEG_LOG("Error loading library: %s\n", error);
        } else {
            AEG_LOG("Unknown error loading library.\n");
        }
        AEG_LOG("load failed !\n");
        return nullptr;
    }
};

namespace abr {
inline PROFILE_ID abr_port_start_profiling(void *graph_impl, const char *name, int32_t option,
                                             uint32_t value) {
    return graph_impl ? static_cast<BaremetalGraphSimBase *>(graph_impl)->start_profiling(name, option, value) : -1;
}
inline int64_t abr_port_read_profiling(void *graph_impl, int32_t h) {
      return graph_impl ? static_cast<BaremetalGraphSimBase *>(graph_impl)->read_profiling(h)
                        : static_cast<int64_t>(-1);
}
inline void abr_port_stop_profiling(void *graph_impl, int32_t h) {
      if (graph_impl) {
        static_cast<BaremetalGraphSimBase *>(graph_impl)->stop_profiling(h);
}
  }
inline void abr_gmio_gm2aie_nb(void *graph_impl, const char *name, char *data, int32_t len) {
      if (graph_impl) {
        static_cast<BaremetalGraphSimBase *>(graph_impl)->gm2aie_nb(const_cast<char *>(name), data, len);
}
  }
inline void abr_gmio_aie2gm_nb(void *graph_impl, const char *name, char *data, int32_t len) {
      if (graph_impl) {
        static_cast<BaremetalGraphSimBase *>(graph_impl)->aie2gm_nb(const_cast<char *>(name), data, len);
}
  }
inline void abr_gmio_gmio_wait(void *graph_impl, const char *name, int32_t syncmode) {
      if (graph_impl) {
        static_cast<BaremetalGraphSimBase *>(graph_impl)->gmio_wait(name, syncmode != 0);
}
  }
} // namespace abr

#include "aeg_class_api.h"
#endif

#ifdef AEG_INHERITANCE_BASED_API
#include "generated_graphs.h"
#endif

#endif // AIE_BAREMETAL_LIB_H
