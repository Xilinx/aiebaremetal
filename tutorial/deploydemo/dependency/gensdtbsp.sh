# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

source /proj/xbuilds/HEAD_daily_latest/installs/lin64/HEAD/Vitis/settings64.sh
sdtgen sdt.tcl /proj/xbuilds/2026.1_daily_latest/internal_platforms/xilinx_vek280_base_202610_1/hw/hw.xsa outdir

mkdir integration_flow
pushd integration_flow
   
ln -s /proj/xbuilds/2026.1_daily_latest/installs/lin64/2026.1/Vitis/data/embeddedsw ./
empyro repo -st ./embeddedsw 
empyro create_bsp -w test_bsp -s ../outdir/system-top.dts -p psv_cortexa72_0 -t hello_world
empyro config_bsp -d test_bsp -al aiebaremetal
empyro build_bsp -d test_bsp
popd
