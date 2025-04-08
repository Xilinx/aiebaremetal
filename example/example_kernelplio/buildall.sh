#!/bin/bash
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

SCRIPT_DIR=$(dirname "$(realpath "$0")")
AIE_GEN=1
FORCEBUILDLIB=1
if [ $# -gt 0 ]; then
	AIE_GEN=$1
fi
pushd ../
make hw AIE_KERNEL_LOC=$SCRIPT_DIR AIE_GEN=$AIE_GEN -j32
popd
pushd ./script
source ./builda72.sh $AIE_GEN
popd