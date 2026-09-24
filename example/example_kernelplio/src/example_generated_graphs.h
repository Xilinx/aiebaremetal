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

class aeg_gradf2 : public AbrGraph {
  public:
    abr::InputPLIO pin;
    abr::OutputPLIO pout;
    aeg_gradf2(const std::string &graphName = "gradf2")
        : AbrGraph(graphName), pin("gradf2.pin", this->getGraphImpl()), pout("gradf2.pout", this->getGraphImpl()) {}
};

#endif // __GENERATED_GRAPH_CLASS_H__
