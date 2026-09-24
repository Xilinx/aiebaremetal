# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
mkdir dsp_lib 
cd ./dsp_lib 
git init 
git remote add -f origin https://github.com/Xilinx/Vitis_Libraries.git && 
git config core.sparseCheckout true && 
echo "dsp/" >> .git/info/sparse-checkout  && 
git pull origin 2023.2 