#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Source the environment variables
#source /proj/xbuilds/2024.1_INT_qualified_latest/installs/lin64/Vitis/2024.1/settings64.sh
source /proj/xbuilds/2024.2_INT_qualified_latest/installs/lin64/Vitis/2024.2/settings64.sh
use_rpu=0

# Loop through all the command-line arguments
for arg in "$@"; do
  if [ "$arg" = "userpu" ]; then
    use_rpu=1
    break
  fi
done

AIECOMPILERPATH=$(which aiecompiler)
AIETOOLPATH=$(dirname "$(dirname "$AIECOMPILERPATH")")
AIETOOLPATHINCLUDE=$AIETOOLPATH/include/
AIETOOLPATHLIB=$AIETOOLPATH/lib/aarchnone64.o/


RESNETSRCPATH=$(dirname "$(dirname $(pwd))")
AIECONTROLCPPPATH=$RESNETSRCPATH/Work/ps/c_rts/

COM_INC=../../../../../mllib/L1/include/common/
COM_INC2=../../../../../mllib/L2/include/common/
COM_GRAPH_INC=../../../../../mllib/L2/include/

#aie control runtime lib, temporary use the xrt one, this is used to suppor aiecontrolruntime.cpp
#AIECONTROLRUNTIMELIB=$AIETOOLPATH/lib/aarch64.o/libadf_rt_ctrl_api_xrt.a

BAREMETAL_DIR=$(cd "$(pwd)/../../../../../../../" && pwd)
echo $BAREMETAL_DIR
BAREMETAL_TEST_DIR=$BAREMETAL_DIR/example/
#/proj/xbuilds/2024.1_INT_0522_2023/installs/lin64/Vitis/2024.1/aietools/include/drivers/aiengine/xaiengine.h
#/proj/xbuilds/2024.1_INT_0522_2023/installs/lin64/Vitis/2024.1/aietools/include/drivers/aiengine/xaiefal
#comment this sys path as we still need libxil.a, and that should rely on baremetal build
AIENGINE_PATH=$AIETOOLPATHINCLUDE/drivers/aiengine/
ARCH_DIR=$BAREMETAL_DIR/thirdparty/arch/ps/
ARCH_72_DIR=$ARCH_DIR/psv_cortexa72_0/
sysrootinclude="$ARCH_72_DIR/include"
sysrootlib="$ARCH_72_DIR/lib"
ELF=vek280.elf
runtime_source_file=../../resnet18_top.cpp
#build host
echo $AIETOOLPATHINCLUDE
echo $AIECONTROLCPPPATH
#prepare libxil.a and aiengine header and lib
pushd $BAREMETAL_TEST_DIR 
# make clean
if [ $use_rpu -eq 0 ]; then
	make hw_external_apu AIE_GEN=2  AIE_COMPILE_WORK=$RESNETSRCPATH/Work 
else
	make hw_external_rpu AIE_GEN=2  AIE_COMPILE_WORK=$RESNETSRCPATH/Work
  # Place your alternative code here
fi
popd
## comment this copy the vekbif and arch
#cp -rf $BAREMETAL_DIR/thirdparty/arch/ ./
#cp $BAREMETAL_DIR/script/vek280_boot_image.bif ./
#compile
#aarch64-none-elf-gcc -D__AIE_BAREMETAL__  -D__AIE_ARCH__=20 -mcpu=cortex-a72 -Wl,-T -Wl,$BAREMETAL_DIR/thirdparty/arch/ps/psv_cortexa72_0/lscript.ld -I$AIECONTROLCPPPATH -I$AIETOOLPATHINCLUDE -I$COM_INC -I$COM_INC2 -I$COM_GRAPH_INC -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/driver/internal/ -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/fal/build/src/include/ -I$AIENGINE_PATH -I/$BAREMETAL_DIR/include -I$sysrootinclude -L$sysrootlib -L$AIETOOLPATHLIB -L$BAREMETAL_TEST_DIR/build/ -o ./${ELF} $runtime_source_file -Wl,--start-group,-laiebaremetala72,-lxaienginea72,-lxil,-lgcc,-lc,-lstdc++,--end-group
#/proj/xbuilds/2022.2_daily_latest/installs/lin64/Vitis/2022.2/bin/bootgen -arch versal -image vek280_boot_image.bif -o boot.bin -w
