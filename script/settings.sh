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

COMPILE_TYPE=0
if [ $# -gt 0 ]; then
	COMPILE_TYPE=$1
fi
echo ${COMPILE_TYPE}

# Get the directory of the script
SCRIPT_DIR=$(dirname "$(readlink -f "$BASH_SOURCE")")
 
echo "The script is located in: $SCRIPT_DIR"

export PATH=$PATH:$SCRIPT_DIR

if [ ${COMPILE_TYPE} -eq  0 ]; then
	pushd .
	if test -d "$TMPDIR"; then
  	  :
	elif test -d "$TMP"; then
  	  TMPDIR=$TMP
	elif test -d "$TEMP"; then
  	  TMPDIR=$TEMP
	elif test -d /var/tmp; then
  	  TMPDIR=/var/tmp
	else
  	  TMPDIR=/tmp
	fi

	foldername=baremetal_runtime_$(date +%Y%m%d%H%M%S)
	cd $TMPDIR
	mkdir $foldername
	cd $foldername
	git clone --recursive https://github.com/Xilinx/aiebaremetal.git

	export BAREMETAL_DIR=$(pwd)/aiebaremetal
	export PATH=$BAREMETAL_DIR/script/:$PATH
	popd

else
	export BAREMETAL_DIR=$SCRIPT_DIR/../
fi

export LM_LICENSE_FILE=1757@xsjlicsrvip
export RDI_INTERNAL_ALLOW_PARTIAL_DATA=yes
#source /proj/xbuilds/2024.1_INT_qualified_latest/installs/lin64/Vitis/2024.1/settings64.sh
# For external buffer latest compiler is needed
#source /proj/xbuilds/HEAD_daily_latest/installs/lin64/Vitis/HEAD/settings64.sh
source envaie2pst50.sh
#create the platform header and library
source bspsetup.sh

