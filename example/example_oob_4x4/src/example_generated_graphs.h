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

class aeg_gradf : public AbrGraph {
  public:
    abr::InputGMIO in[8];
    abr::OutputGMIO out[8];
    aeg_gradf(const std::string &graphName = "gradf") : AbrGraph(graphName) {
        in[0] = abr::InputGMIO("gradf.in[0]", this->getGraphImpl(), nullptr);
        in[1] = abr::InputGMIO("gradf.in[1]", this->getGraphImpl(), nullptr);
        in[2] = abr::InputGMIO("gradf.in[2]", this->getGraphImpl(), nullptr);
        in[3] = abr::InputGMIO("gradf.in[3]", this->getGraphImpl(), nullptr);
        in[4] = abr::InputGMIO("gradf.in[4]", this->getGraphImpl(), nullptr);
        in[5] = abr::InputGMIO("gradf.in[5]", this->getGraphImpl(), nullptr);
        in[6] = abr::InputGMIO("gradf.in[6]", this->getGraphImpl(), nullptr);
        in[7] = abr::InputGMIO("gradf.in[7]", this->getGraphImpl(), nullptr);
        out[0] = abr::OutputGMIO("gradf.out[0]", this->getGraphImpl(), nullptr);
        out[1] = abr::OutputGMIO("gradf.out[1]", this->getGraphImpl(), nullptr);
        out[2] = abr::OutputGMIO("gradf.out[2]", this->getGraphImpl(), nullptr);
        out[3] = abr::OutputGMIO("gradf.out[3]", this->getGraphImpl(), nullptr);
        out[4] = abr::OutputGMIO("gradf.out[4]", this->getGraphImpl(), nullptr);
        out[5] = abr::OutputGMIO("gradf.out[5]", this->getGraphImpl(), nullptr);
        out[6] = abr::OutputGMIO("gradf.out[6]", this->getGraphImpl(), nullptr);
        out[7] = abr::OutputGMIO("gradf.out[7]", this->getGraphImpl(), nullptr);
    }
};

#endif // __GENERATED_GRAPH_CLASS_H__
