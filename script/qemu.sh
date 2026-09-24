# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
source /proj/xbuilds/2025.2_daily_latest/installs/lin64/2025.2/Vitis/settings64.sh
source /proj/petalinux/2025.2/petalinux_daily_latest_for_vitis/tool/petalinux-v2025.2-final/settings.sh

qemu-system-aarch64 \
-M arm-generic-fdt \
-serial null -serial null -serial mon:stdio \
-device loader,file=vck190.elf,cpu-num=0 \
-device loader,addr=0xFD1A0300,data=0x8000000e,data-len=4 \
-hw-dtb /mnt/1tb/scratch/git/qemu-devicetrees/LATEST/SINGLE_ARCH/board-versal-ps-vck190.dtb \
-m 512M \
-display none
