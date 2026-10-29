// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "aiebaremetal.h"
#include "aeg_platform.h"
#if defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf;
#endif

#include <fstream>
#include <xaiengine.h>


AbrGraph gr("gradf");

int main(int argc, char ** argv)
{

	gr.getaiedevinst();
	gr.init();
	gr.run(1);
	gr.wait();
	gr.end();
  return 0;
}