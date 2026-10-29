// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef COMMON_MODULE_H
#define COMMON_MODULE_H

// Per-tile length: softmax is computed independently over each TILE_LEN-element
// chunk (matches example_softmax/aie_only's semantics, so the two examples are
// directly comparable). NUM_TILES chunks are streamed through per host run in
// a single aeg::pl_kernel start/wait cycle, with softmax_pl.cpp looping over
// tiles internally.
#define TILE_LEN 32
#define NUM_TILES 8
#define TOTAL_LEN (TILE_LEN * NUM_TILES)

#endif
