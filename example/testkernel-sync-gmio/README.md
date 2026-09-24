<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# Test gmio using square kernel.

This test case run square kernel, which squares the input provided and copies it to output gmio
memory.

# To Run

    pushd ../ && make clean && rm build -rf && popd && source ./build.sh 5 && source runsim.sh
