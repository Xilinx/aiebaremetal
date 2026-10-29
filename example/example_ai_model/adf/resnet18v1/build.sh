# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#source /proj/xbuilds/2024.1_INT_qualified_latest/installs/lin64/Vitis/2024.1/settings64.sh
source /proj/xbuilds/2024.2_INT_qualified_latest/installs/lin64/Vitis/2024.2/settings64.sh
#source /proj/xbuilds/HEAD_daily_latest/installs/lin64/Vitis/HEAD/settings64.sh
AIECOMPILERPATH=$(which aiecompiler)
AIETOOLPATH=$(dirname "$(dirname "$AIECOMPILERPATH")")
export XILINX_VITIS_AIETOOLS=$AIETOOLPATH
python3 ./dataconvert.py 
make compile
pushd ./baremetal/ml_txn/
source ./build.sh
cd ../
source ./buildappbootbin.sh
popd
