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
    abr::SharedBuffer input;
    abr::SharedBuffer output;
    struct {
        abr::ExternalBuffer buffer;
        abr::ExternalBufferPort out[1];

        void setAddress(void *ptr) { buffer.setAddress(ptr); }
        void setupPingPongBuffers(void *const ptr1, void *const ptr2) { buffer.setupPingPongBuffers(ptr1, ptr2); }
        void gm2aie_nb() { buffer.gm2aie_nb(); }
        void aie2gm_nb() { buffer.aie2gm_nb(); }
    } ddrin;
    struct {
        abr::ExternalBuffer buffer;
        abr::ExternalBufferPort in[1];

        void setAddress(void *ptr) { buffer.setAddress(ptr); }
        void setupPingPongBuffers(void *const ptr1, void *const ptr2) { buffer.setupPingPongBuffers(ptr1, ptr2); }
        void gm2aie_nb() { buffer.gm2aie_nb(); }
        void aie2gm_nb() { buffer.aie2gm_nb(); }
    } ddrout;
    struct {
        abr::ExternalBuffer buffer;
        abr::ExternalBufferPort out[1];

        void setAddress(void *ptr) { buffer.setAddress(ptr); }
        void setupPingPongBuffers(void *const ptr1, void *const ptr2) { buffer.setupPingPongBuffers(ptr1, ptr2); }
        void gm2aie_nb() { buffer.gm2aie_nb(); }
        void aie2gm_nb() { buffer.aie2gm_nb(); }
    } weights_1;
    struct {
        abr::ExternalBuffer buffer;
        abr::ExternalBufferPort out[1];

        void setAddress(void *ptr) { buffer.setAddress(ptr); }
        void setupPingPongBuffers(void *const ptr1, void *const ptr2) { buffer.setupPingPongBuffers(ptr1, ptr2); }
        void gm2aie_nb() { buffer.gm2aie_nb(); }
        void aie2gm_nb() { buffer.aie2gm_nb(); }
    } weights_2;
    aeg_EG(const std::string &graphName = "EG")
        : AbrGraph(graphName), input("EG.input", this->getGraphImpl(), nullptr),
          output("EG.output", this->getGraphImpl(), nullptr) {
        ddrin.buffer = abr::ExternalBuffer("EG.ddrin", this->getGraphImpl(), nullptr);
        ddrin.out[0] = abr::ExternalBufferPort("EG.ddrin.out[0]", this->getGraphImpl(), nullptr);
        ddrout.buffer = abr::ExternalBuffer("EG.ddrout", this->getGraphImpl(), nullptr);
        ddrout.in[0] = abr::ExternalBufferPort("EG.ddrout.in[0]", this->getGraphImpl(), nullptr);
        weights_1.buffer = abr::ExternalBuffer("EG.weights_1", this->getGraphImpl(), nullptr);
        weights_1.out[0] = abr::ExternalBufferPort("EG.weights_1.out[0]", this->getGraphImpl(), nullptr);
        weights_2.buffer = abr::ExternalBuffer("EG.weights_2", this->getGraphImpl(), nullptr);
        weights_2.out[0] = abr::ExternalBufferPort("EG.weights_2.out[0]", this->getGraphImpl(), nullptr);
    }
};

#endif // __GENERATED_GRAPH_CLASS_H__
