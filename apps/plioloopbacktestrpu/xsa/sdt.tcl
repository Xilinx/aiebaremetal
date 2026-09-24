# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
set outdir [lindex $argv 1]
set xsa [lindex $argv 0]
exec rm -rf $outdir

sdtgen set_dt_param -xsa $xsa -dir $outdir

puts stderr [time {sdtgen generate_sdt}]

cd $outdir;
#exec cpp -nostdinc -I include -undef -x assembler-with-cpp system-top.dts > dtc_out.dts
exec cpp -nostdinc -I include -undef -x assembler-with-cpp system-top.dts | dtc -I dts -O dtb -b 0 -@ -o system.dtb - >@stdout 2>@stderr
exec dtc -I dtb -O dts -b 0 -@ -o regen_system.dts system.dtb >@stdout 2>@stderr
