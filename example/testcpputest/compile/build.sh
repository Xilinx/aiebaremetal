#!/bin/bash
#source ./build.sh 2 -bootgen -rpu
set -e
CURRDIR=$PWD

AIE_GEN=1
RPUFLAG=
BOOTGEN=
HW_QEMU=
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
if [ $# -gt 3 ]; then
	HW_QEMU=$4
	HW_QEMU=--hw_qemu
	#--hw_qemu
fi

echo ${AIE_GEN}
#set this is is local env
source $CURRDIR/../../../script/settings.sh 1
$CURRDIR/../../../script/compile.sh --runtime_source_file ./src/graph.cpp --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN} ${HW_QEMU}
compile_status=$?
if [ $compile_status -ne 0 ]; then
    echo "Error: compile.sh failed with status $compile_status" >&2
    exit $compile_status
fi
echo "Build completed successfully with AIE_GEN=${AIE_GEN}, BOOTGEN=${BOOTGEN}, RPUFLAG=${RPUFLAG}"
