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
using namespace abr;
#define GRAPH_COMPILE_OR_SIM (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
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

// TODO: remove this hardcode after aiecompiler add rpu base address into json
#define RPU_AIE_BASE 0x40000000
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
    BaremetalGraphSim(std::string GraphName, uint8_t rpu = 0) : isInitialized(false) {
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
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            return bgraph->selftestexit();
        } else {
            return aeg_err::internal_error;
        }
        return aeg_err::ok;
    }

    aeg_err clock_enable() {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            return bgraph->clock_enable();
        } else {
            return aeg_err::internal_error;
        }
    }

    aeg_err clock_disable() {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            return bgraph->clock_disable();
        } else {
            return aeg_err::internal_error;
        }
    }

    aeg_err error_handling_cb_init(void *cb, void *priv) {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            return bgraph->error_handling_cb_init(cb, priv);
        } else {
            return aeg_err::internal_error;
        }
        return aeg_err::ok;
    }

    aeg_err init() {
        if (bgraph) {
            auto ret = bgraph->init();
            if (ret != aeg_err::ok) {
                return ret;
            }
            isInitialized = true;
            return aeg_err::ok;
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::internal_error;
        }
    }

    aeg_err run(int num = -1) {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            return bgraph->run(num);
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::internal_error;
        }
    }

    aeg_err gm2aie_nb(char *name, char *data, int len) {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            return bgraph->gm2aie_nb(name, data, len);
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::internal_error;
        }
    }

    aeg_err aie2gm_nb(char *name, char *data, int len) {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            return bgraph->aie2gm_nb(name, data, len);
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::internal_error;
        }
    }

    aeg_err gm2aie(char *name, char *data, int len) {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            return bgraph->gm2aie(name, data, len);
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::internal_error;
        }
    }

    aeg_err aie2gm(char *name, char *data, int len) {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            return bgraph->aie2gm(name, data, len);
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::internal_error;
        }
    }

    aeg_err gmio_wait(std::string gmio_name, bool syncmode = true) {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        aeg_err ret = aeg_err::ok;
        if (bgraph) {
            ret = bgraph->gmio_wait(gmio_name, syncmode);
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::internal_error;
        }
        return ret;
    }

    // External Buffer APIs
    aeg_err setAddress(char *externalbuffer_name, void *ptr) {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            return bgraph->setAddress(externalbuffer_name, ptr);
        } else {
            std::cout << "External Buffer setaddress failed ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::internal_error;
        }
        return aeg_err::ok;
    }

    aeg_err wait(char *externalbuffer_name, std::string port_name) {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            bgraph->wait(externalbuffer_name, port_name);
        } else {
            std::cout << "External Buffer wait failed ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::user_error;
        }
        return aeg_err::ok;
    }

    aeg_err gm2aie_nb(char *externalbuffer_name, std::string port_name) {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            bgraph->gm2aie_nb(externalbuffer_name, port_name);
        } else {
            std::cout << "External Buffer gm2aie_nb failed ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::user_error;
        }
        return aeg_err::ok;
    }

    aeg_err aie2gm_nb(char *externalbuffer_name, std::string port_name) {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            bgraph->aie2gm_nb(externalbuffer_name, port_name);
        } else {
            std::cout << "External Buffer aie2gm_nb failed ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::user_error;
        }
        return aeg_err::ok;
    }

    aeg_err setupPingPongBuffers(char *externalbuffer_name, void *const ptr1, void *const ptr2) {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            bgraph->setupPingPongBuffers(externalbuffer_name, ptr1, ptr2);
        } else {
            std::cout << "External Buffer setupPingPongBuffers failed ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::user_error;
        }
        return aeg_err::ok;
    }

    // End external buffer APIs
    aeg_err wait() {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            return bgraph->wait();
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::internal_error;
        }
    }

    aeg_err end() {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            return bgraph->end();
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::user_error;
        }
        return aeg_err::ok;
    }

    void *malloc(uint32_t len) {
        //if (!CHECK_INITIALIZED())
        //    return nullptr;

        if (bgraph) {
            return bgraph->malloc(len);
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return nullptr;
        }
    }

    aeg_err free(void *addr) {
        //if (!CHECK_INITIALIZED())
        //    return aeg_err::user_error;

        if (bgraph) {
            return bgraph->free(addr);
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::internal_error;
        }
        return aeg_err::ok;
    }

    aeg_err update(const std::string &port_name, void *data, size_t len) {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            return bgraph->update(port_name, data, len);
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::user_error;
        }
    }

    aeg_err read(const std::string &port_name, char *data, int len) {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            return bgraph->read(port_name, data, len);
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return aeg_err::user_error;
        }
    }

    char *dumpcorelog(int row, int col) {
        if (!CHECK_INITIALIZED())
            return nullptr;

        if (bgraph) {
            return bgraph->dumpcorelog(row, col);
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return NULL;
        }
    }

    void *getaiedevinst() {
        if (!CHECK_INITIALIZED())
            return nullptr;

        if (bgraph) {
            return bgraph->getaiedevinst();
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return NULL;
        }
    }

    void *BacktrackErrors() {
        if (!CHECK_INITIALIZED())
            return nullptr;

        if (bgraph) {
            return bgraph->BacktrackErrors();
        } else {
            std::cout << "baremetal graph error backtracking failed, please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return NULL;
        }
    }

    void gdbpoint() {
        if (!CHECK_INITIALIZED())
            return;

        if (bgraph) {
            return bgraph->gdbpoint();
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return;
        }
    }

    int start_profiling(const std::string &port_name, int option, uint32_t value = 0) {
        if (!CHECK_INITIALIZED())
            return -1;

        if (bgraph) {
            return bgraph->start_profiling(port_name, option, value);
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return -1;
        }
    }

    int start_profiling(const std::string &port_name1, const std::string &port_name2, int option, uint32_t value = 0) {
        if (!CHECK_INITIALIZED())
            return -1;

        if (bgraph) {
            return bgraph->start_profiling(port_name1, port_name2, option, value);
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return -1;
        }
    }

    long long read_profiling(int h) {
        if (!CHECK_INITIALIZED())
            return -1;

        if (bgraph) {
            return bgraph->read_profiling(h);
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
            return -1;
        }
    }

    aeg_err stop_profiling(int h) {
        if (!CHECK_INITIALIZED())
            return aeg_err::user_error;

        if (bgraph) {
            return bgraph->stop_profiling(h);
        } else {
            std::cout << "baremetal graph load failed ,please check the ";
            std::cout << AIE_BAREMETAL_LIB << " env" << std::endl;
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
    void *handle = NULL;
    BaremetalGraphSimBase *bgraph;
    bool isInitialized;

    bool check_initialized_impl(const char *function_name) {
        if (!isInitialized) {
            AEG_ERROR("BaremetalGraphSim::%s: invalid state (graph not initialized; call init() first)", function_name);
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
        AEG_LOG("%d global<- lazy-->  %d %s\n", (int)RTLD_GLOBAL, (int)RTLD_LAZY, ablib);
        handle = dlopen(ablib, RTLD_LAZY);

        if (handle != NULL) {

            AEG_LOG("%sload success !\n", ablib);
            createBaremetalGraphInstance_t createInstance =
                (createBaremetalGraphInstance_t)dlsym(handle, CREATEGRAPHAPI);
            if (!createInstance) {
                AEG_LOG("Cannot load symbol: \n");
                dlclose(handle);
                return nullptr;
            }
            BaremetalGraphSimBase *inst = createInstance(gname.c_str(), rpu);
            return inst;
        }
        const char *error = dlerror();
        if (error) {
            AEG_LOG("Error loading library: %s\n", error);
        } else {
            AEG_LOG("Unknown error loading library.\n");
        }
        AEG_LOG("load failed !\n");
        return nullptr;
    }
};

namespace abr {
inline int abr_port_start_profiling(void *graph_impl, const char *name, int option, unsigned value) {
    return graph_impl ? static_cast<BaremetalGraphSimBase *>(graph_impl)->start_profiling(name, option, value) : -1;
}
inline long long abr_port_read_profiling(void *graph_impl, int h) {
    return graph_impl ? static_cast<BaremetalGraphSimBase *>(graph_impl)->read_profiling(h) : -1;
}
inline void abr_port_stop_profiling(void *graph_impl, int h) {
    if (graph_impl)
        static_cast<BaremetalGraphSimBase *>(graph_impl)->stop_profiling(h);
}
inline void abr_gmio_gm2aie_nb(void *graph_impl, const char *name, char *data, int len) {
    if (graph_impl)
        static_cast<BaremetalGraphSimBase *>(graph_impl)->gm2aie_nb(const_cast<char *>(name), data, len);
}
inline void abr_gmio_aie2gm_nb(void *graph_impl, const char *name, char *data, int len) {
    if (graph_impl)
        static_cast<BaremetalGraphSimBase *>(graph_impl)->aie2gm_nb(const_cast<char *>(name), data, len);
}
inline void abr_gmio_gmio_wait(void *graph_impl, const char *name, int syncmode) {
    if (graph_impl)
        static_cast<BaremetalGraphSimBase *>(graph_impl)->gmio_wait(name, syncmode != 0);
}
} // namespace abr

#include "aeg_class_api.h"
#endif

#ifdef AEG_INHERITANCE_BASED_API
#include "generated_graphs.h"
#endif

#endif // AIE_BAREMETAL_LIB_H
