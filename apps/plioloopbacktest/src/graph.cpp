// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "graph.h"

testcase::two_node_graph mygraph;

int main(int argc, char ** argv) { 

  mygraph.init();
  mygraph.run();
  mygraph.end();
  return 0;
}

