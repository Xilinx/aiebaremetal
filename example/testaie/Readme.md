<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# Build and Test

1. git clone --recursive <aiebaremetal_repo_url>

2. cd aiebaremetal

3. make AIE_KERNEL_LOC=$PWD/testaie/kernel AIE_GEN=5 -j32

4. cd ./testaie/kernel

5. source ./runsim.sh

6. the code are in following folder.

   a. testuc
   
   b. testaie
