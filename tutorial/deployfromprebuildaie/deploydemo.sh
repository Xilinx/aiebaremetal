# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
source /proj/xbuilds/HEAD_daily_latest/installs/lin64/HEAD/Vitis/settings64.sh

pushd dependency/aie/
make clean
make compile
popd

pushd dependency/
source ./genmetacpp.sh
source ./genbootbin.sh
popd

source ./build_baremetal.sh
source ./buildmeta.sh
source ./buildmain.sh
source ./buildelf.sh

