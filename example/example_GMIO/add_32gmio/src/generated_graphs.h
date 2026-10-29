/**<!--
* Copyright (C) 2024 - 2026 Advanced Micro Devices, Inc.
* Auto-generated graph class
* -->*/

#ifndef __GENERATED_GRAPH_CLASS_H__
#define __GENERATED_GRAPH_CLASS_H__

#include "aeg_baremetal_api.h"
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include "aeg_baremetal_graph_sim_base.h"
#endif
#ifndef AbrGraph
#define AbrGraph BaremetalGraph
#endif

class GmioApiImpl;
class ExternalBufferApiImpl;
class RtpApiImpl;
class SharedBufferApiImpl;

namespace aeg {
struct gradf {};
}

template<typename GraphTag>
struct AEGGraph;

class aeg_gradf : public AbrGraph {
public:
    aeg::detail::InputGMIO input[12];
    aeg::detail::OutputGMIO output[12];
    struct k_t {
        aeg::detail::RTP in[2];
    } k[12];
    aeg_gradf(const std::string& graphName = "gradf")
        : AbrGraph(graphName) {
        input[0] = aeg::detail::InputGMIO("gradf.input[0]",
            this->getGraphImpl()
            , nullptr);
        input[1] = aeg::detail::InputGMIO("gradf.input[1]",
            this->getGraphImpl()
            , nullptr);
        input[2] = aeg::detail::InputGMIO("gradf.input[2]",
            this->getGraphImpl()
            , nullptr);
        input[3] = aeg::detail::InputGMIO("gradf.input[3]",
            this->getGraphImpl()
            , nullptr);
        input[4] = aeg::detail::InputGMIO("gradf.input[4]",
            this->getGraphImpl()
            , nullptr);
        input[5] = aeg::detail::InputGMIO("gradf.input[5]",
            this->getGraphImpl()
            , nullptr);
        input[6] = aeg::detail::InputGMIO("gradf.input[6]",
            this->getGraphImpl()
            , nullptr);
        input[7] = aeg::detail::InputGMIO("gradf.input[7]",
            this->getGraphImpl()
            , nullptr);
        input[8] = aeg::detail::InputGMIO("gradf.input[8]",
            this->getGraphImpl()
            , nullptr);
        input[9] = aeg::detail::InputGMIO("gradf.input[9]",
            this->getGraphImpl()
            , nullptr);
        input[10] = aeg::detail::InputGMIO("gradf.input[10]",
            this->getGraphImpl()
            , nullptr);
        input[11] = aeg::detail::InputGMIO("gradf.input[11]",
            this->getGraphImpl()
            , nullptr);
        output[0] = aeg::detail::OutputGMIO("gradf.output[0]",
            this->getGraphImpl()
            , nullptr);
        output[1] = aeg::detail::OutputGMIO("gradf.output[1]",
            this->getGraphImpl()
            , nullptr);
        output[2] = aeg::detail::OutputGMIO("gradf.output[2]",
            this->getGraphImpl()
            , nullptr);
        output[3] = aeg::detail::OutputGMIO("gradf.output[3]",
            this->getGraphImpl()
            , nullptr);
        output[4] = aeg::detail::OutputGMIO("gradf.output[4]",
            this->getGraphImpl()
            , nullptr);
        output[5] = aeg::detail::OutputGMIO("gradf.output[5]",
            this->getGraphImpl()
            , nullptr);
        output[6] = aeg::detail::OutputGMIO("gradf.output[6]",
            this->getGraphImpl()
            , nullptr);
        output[7] = aeg::detail::OutputGMIO("gradf.output[7]",
            this->getGraphImpl()
            , nullptr);
        output[8] = aeg::detail::OutputGMIO("gradf.output[8]",
            this->getGraphImpl()
            , nullptr);
        output[9] = aeg::detail::OutputGMIO("gradf.output[9]",
            this->getGraphImpl()
            , nullptr);
        output[10] = aeg::detail::OutputGMIO("gradf.output[10]",
            this->getGraphImpl()
            , nullptr);
        output[11] = aeg::detail::OutputGMIO("gradf.output[11]",
            this->getGraphImpl()
            , nullptr);
        k[0].in[1] = aeg::detail::RTP("gradf.k[0].in[1]",
            this->getGraphImpl()
            , nullptr);
        k[1].in[1] = aeg::detail::RTP("gradf.k[1].in[1]",
            this->getGraphImpl()
            , nullptr);
        k[2].in[1] = aeg::detail::RTP("gradf.k[2].in[1]",
            this->getGraphImpl()
            , nullptr);
        k[3].in[1] = aeg::detail::RTP("gradf.k[3].in[1]",
            this->getGraphImpl()
            , nullptr);
        k[4].in[1] = aeg::detail::RTP("gradf.k[4].in[1]",
            this->getGraphImpl()
            , nullptr);
        k[5].in[1] = aeg::detail::RTP("gradf.k[5].in[1]",
            this->getGraphImpl()
            , nullptr);
        k[6].in[1] = aeg::detail::RTP("gradf.k[6].in[1]",
            this->getGraphImpl()
            , nullptr);
        k[7].in[1] = aeg::detail::RTP("gradf.k[7].in[1]",
            this->getGraphImpl()
            , nullptr);
        k[8].in[1] = aeg::detail::RTP("gradf.k[8].in[1]",
            this->getGraphImpl()
            , nullptr);
        k[9].in[1] = aeg::detail::RTP("gradf.k[9].in[1]",
            this->getGraphImpl()
            , nullptr);
        k[10].in[1] = aeg::detail::RTP("gradf.k[10].in[1]",
            this->getGraphImpl()
            , nullptr);
        k[11].in[1] = aeg::detail::RTP("gradf.k[11].in[1]",
            this->getGraphImpl()
            , nullptr);
    }
};

template<>
struct AEGGraph<aeg::gradf> : aeg_gradf {
    using aeg_gradf::aeg_gradf;
};


#endif // __GENERATED_GRAPH_CLASS_H__
