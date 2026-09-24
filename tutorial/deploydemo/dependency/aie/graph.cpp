// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <fstream>
#include "graph.h"
myGraph gradf;
int main(int argc, char ** argv)
{
	gradf.init();
	gradf.run(1);
	gradf.wait();
	gradf.end();
	return 0;
}
