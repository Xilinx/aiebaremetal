# Copyright (C) 2024 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
bash -c 'export RUN_SCRIPT_DIR=$(dirname "$(realpath "$0")") && \
	export AIE_COMPILE_WORK="$RUN_SCRIPT_DIR/Work/" \
	&& export SCRIPT_DIR=$(dirname "$(readlink -f "$BASH_SOURCE")") \
	&& echo "$AIE_COMPILE_WORK" \
	&& cd $BAREMETAL_DIR/ \
	&& source ./script/aieldpath.sh \
	&& source ./script/envaie2pst50.sh \
	&& export LD_PRELOAD=$SCRIPT_DIR/../thirdparty/aielib/aie-rt/driver/src/libxaiengine.so \
	&& echo $RUN_SCRIPT_DIR \
	&& cd $RUN_SCRIPT_DIR && \
	aiesimulator --profile --disable-unused-mem-tiles=false --enable-uc-uart-to-stdout=true --pkg-dir=./Work'
#bash -c "cd ../;source ./aieldpath.sh; cd ./testkernel/; source envaie2pst50.sh;aiesimulator --profile --disable-unused-mem-tiles=false --pkg-dir=./Work"
#&& export LD_PRELOAD=$SCRIPT_DIR/../thirdparty/aielib/aie-rt/driver/src/libxaiengine.so \
