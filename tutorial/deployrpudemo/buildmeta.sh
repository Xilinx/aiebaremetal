# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

#pushd ../
#./generate_json_c.sh --work_folder ./deploydemo/Work/
#mv ./baremetal_metadata.cpp ./deploydemo/
#popd
source /proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/settings.sh
armr5-none-eabi-g++ -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -DARMR5 -g -c -std=c++17 -o aiemetadata.o ./dependency/baremetal_metadata.cpp
