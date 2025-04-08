# Copyright (C) 2025 Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"). You may
# not use this file except in compliance with the License. A copy of the
# License is located at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

source /proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/settings.sh
aarch64-none-elf-gcc graph.o aiemetadata.o -mcpu=cortex-a72 -Wl,-T -Wl,./lscript.ld -L./ -L./dependency/psv_cortexa72_0/lib/ -laiebaremetal_api -Wl,--start-group,-lxil,-lgcc,-lc,-lstdc++,--end-group -o main.elf
