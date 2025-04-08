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

#source ./build.sh 2 -bootgen -rpu --workfolder ./Work/
AIE_GEN=1
RPUFLAG=
BOOTGEN=
if [ $# -gt 0 ]; then
	AIE_GEN=$1
fi
if [ $# -gt 1 ]; then
	BOOTGEN=$2
	#-bootgen
fi
if [ $# -gt 2 ]; then
	RPUFLAG=$3
	#-rpu
fi


echo ${AIE_GEN}
#set this is is local env
source ../../script/settings.sh 1
compile.sh --runtime_source_file ./src/graph.cpp --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN}
#compile.sh --runtime_source_file ./src/graph.cpp --aie_version 1 -bootgen --workfolder ./Work/

