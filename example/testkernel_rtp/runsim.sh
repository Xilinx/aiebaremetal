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

bash -c 'export SCRIPT_DIR=$(dirname "$(realpath "$0")") && export AIE_COMPILE_WORK="$SCRIPT_DIR/Work/" && echo "$AIE_COMPILE_WORK" && cd ../ && source ./aieldpath.sh && cd ./testkernel_rtp/ && source envaie2pst50.sh && aiesimulator --profile --disable-unused-mem-tiles=false --enable-uc-uart-to-stdout=true --pkg-dir=./Work'
#bash -c "cd ../;source ./aieldpath.sh; cd ./testkernel/; source envaie2pst50.sh;aiesimulator --profile --disable-unused-mem-tiles=false --pkg-dir=./Work"
