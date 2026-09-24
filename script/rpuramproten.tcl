# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# init_regs.tcl

# Write registers

# targets -set -nocase -filter {name =~ "Versal Gen 2*"}

mwr -force 0xeb5e001c 0x0
mwr -force 0xEB580000 1
mwr -force 0xbbf20000 0xeafffffe

# Set variable and use it
set addr 0xbbf20000
mwr -force 0xEB588008 $addr

# Masked writes
mask_write 0xEB588000 0x10 0x10
mask_write 0xEB5E0310 0x10101 0x1
mask_write 0xEB5E0310 0x1 0x0
