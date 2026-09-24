/**<!--
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 * Auto-generated graph class
 * -->*/

#ifndef __GENERATED_GRAPH_CLASS_H__
#define __GENERATED_GRAPH_CLASS_H__

#include "aeg_baremetal_api.h"
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include "aeg_baremetal_graph_sim_base.h"
#endif

class GmioApiImpl;
class ExternalBufferApiImpl;
class RtpApiImpl;
class SharedBufferApiImpl;

class aeg_EG : public AbrGraph {
  public:
    struct {
        abr::ExternalBuffer buffer;
        abr::ExternalBufferPort out[1];

        void setAddress(void *ptr) { buffer.setAddress(ptr); }
        void setupPingPongBuffers(void *const ptr1, void *const ptr2) { buffer.setupPingPongBuffers(ptr1, ptr2); }
        void gm2aie_nb() { buffer.gm2aie_nb(); }
        void aie2gm_nb() { buffer.aie2gm_nb(); }
    } in1;
    struct {
        abr::ExternalBuffer buffer;
        abr::ExternalBufferPort in[1];

        void setAddress(void *ptr) { buffer.setAddress(ptr); }
        void setupPingPongBuffers(void *const ptr1, void *const ptr2) { buffer.setupPingPongBuffers(ptr1, ptr2); }
        void gm2aie_nb() { buffer.gm2aie_nb(); }
        void aie2gm_nb() { buffer.aie2gm_nb(); }
    } out1;
    struct {
        abr::ExternalBuffer buffer;
        abr::ExternalBufferPort in[1];
        abr::ExternalBufferPort out[1];

        void setAddress(void *ptr) { buffer.setAddress(ptr); }
        void setupPingPongBuffers(void *const ptr1, void *const ptr2) { buffer.setupPingPongBuffers(ptr1, ptr2); }
        void gm2aie_nb() { buffer.gm2aie_nb(); }
        void aie2gm_nb() { buffer.aie2gm_nb(); }
    } ext;
    aeg_EG(const std::string &graphName = "EG") : AbrGraph(graphName) {
        in1.buffer = abr::ExternalBuffer("EG.in1", this->getGraphImpl(), nullptr);
        in1.out[0] = abr::ExternalBufferPort("EG.in1.out[0]", this->getGraphImpl(), nullptr);
        out1.buffer = abr::ExternalBuffer("EG.out1", this->getGraphImpl(), nullptr);
        out1.in[0] = abr::ExternalBufferPort("EG.out1.in[0]", this->getGraphImpl(), nullptr);
        ext.buffer = abr::ExternalBuffer("EG.ext", this->getGraphImpl(), nullptr);
        ext.in[0] = abr::ExternalBufferPort("EG.ext.in[0]", this->getGraphImpl(), nullptr);
        ext.out[0] = abr::ExternalBufferPort("EG.ext.out[0]", this->getGraphImpl(), nullptr);
    }
};

#endif // __GENERATED_GRAPH_CLASS_H__
