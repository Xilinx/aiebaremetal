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

namespace aeg {
struct gradf {};
} // namespace aeg

template <typename GraphTag> struct AEGGraph;

class aeg_gradf : public AbrGraph {
  public:
    abr::InputGMIO in;
    abr::OutputGMIO out;
    aeg_gradf(const std::string &graphName = "gradf")
        : AbrGraph(graphName), in("gradf.in", this->getGraphImpl(), nullptr),
          out("gradf.out", this->getGraphImpl(), nullptr) {}
};

template <> struct AEGGraph<aeg::gradf> : aeg_gradf {
    using aeg_gradf::aeg_gradf;
};

#endif // __GENERATED_GRAPH_CLASS_H__
