# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Source the environment variables
export ARCH_DIR=`pwd`/../../
export KERNEL_DIR=$ARCH_DIR/kernels/

(
  source $ARCH_DIR/env.sh
  source $ARCH_DIR/compile_kernel.sh compile aie-ml
)

source /proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/settings.sh
#aarch64-none-elf-gcc -mcpu=cortex-a72 -Wl,-T -Wl,../lscript.ld -I../../../thirdparty/aielib/aie-rt/driver/internal/ -I../include/ -L../lib/ -L../../../build/ -o ./main.elf ./aie.cc $KERNEL_DIR/output/aieml/*.o -Wl,--start-group,-lxaienginea72,-lxil,-lgcc,-lc,-lstdc++,--end-group
aarch64-none-elf-gcc -mcpu=cortex-a72 -Wl,-T -Wl,../lscript.ld -I../../../thirdparty/aielib/aie-rt/driver/internal/ -I../include/ -L../lib/ -L../../../build/ -o ./main.elf ./aie.cc $KERNEL_DIR/output/aieml/*.o -Wl,--start-group,-laiebaremetala72,-lxil,-lgcc,-lc,-lstdc++,--end-group
cp ../../kernels/NeighborTileLoopback/build/D ../../../testaie/data/aiekernel
cp ../../kernels/NeighborTileLoopback/build/D.map ../../../testaie/data/aiekernel.map
cp ./main.elf /proj/xsjsswstaff/huaj/vek280/APU/

#armr5-none-eabi-g++ -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -Wl,-T -Wl,./lscript.ld -L/scratch/staff/abhattar/practice/vitis_2023.1_rpu_demo/vitis_design_wrapper/psv_cortexr5_0/standalone_psv_cortexr5_0/bsp/psv_cortexr5_0/lib/ -o ./main.elf ./aie.cc -Wl,--start-group,-lxil,-lgcc,-lc,-lstdc++,--end-group
