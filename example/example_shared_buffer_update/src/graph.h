// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "kernels.h"
class MyGraph : public adf::graph {
    adf::kernel k;
    adf::shared_buffer<int32> ifm;

  public:
    adf::output_gmio out;
    adf::input_gmio in;
    adf::shared_buffer<int32> wts;
    MyGraph() {
        in = adf::input_gmio::create("gmio1", 64, 1);
        out = adf::output_gmio::create("gmio2", 64, 1);
        ifm = adf::shared_buffer<int32>::create({NUM_SAMPLES}, 1, 1);
        adf::write_access(ifm.in[0]) =
            adf::tiling({.buffer_dimension = {NUM_SAMPLES}, .tiling_dimension = {NUM_SAMPLES}, .offset = {0}});
        adf::read_access(ifm.out[0]) =
            adf::tiling({.buffer_dimension = {NUM_SAMPLES}, .tiling_dimension = {NUM_SAMPLES}, .offset = {0}});
        // wts is read-only shared buffer (0 input ports)
        wts = adf::shared_buffer<int32>::create({NUM_SAMPLES}, 0, 1);
        adf::read_access(wts.out[0]) =
            adf::tiling({.buffer_dimension = {NUM_SAMPLES}, .tiling_dimension = {NUM_SAMPLES}, .offset = {0}});
        // compile time loading
        // adf::initial_value(wts) = {1, 2, 3, 4, 5, 6, 7, 8};
        k = adf::kernel::create(multiply);
        adf::source(k) = "src/kernels.cpp";
        adf::runtime<adf::ratio>(k) = 0.8;
        adf::dimensions(k.in[0]) = {NUM_SAMPLES};
        adf::dimensions(k.in[1]) = {NUM_SAMPLES};
        adf::dimensions(k.out[0]) = {NUM_SAMPLES};
        // adf::repetition_count(k) = 2;
        adf::connect(in.out[0], ifm.in[0]);
        adf::connect(ifm.out[0], k.in[0]);
        adf::connect(wts.out[0], k.in[1]);
        adf::connect(k.out[0], out.in[0]);
    }
};