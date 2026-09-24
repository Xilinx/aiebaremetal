# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

source /proj/xbuilds/2026.1_daily_latest/installs/lin64/2026.1/Vitis/settings64.sh
#sdtgen sdt.tcl /proj/xsjsswstaff/`whoami`/aegapi/xsa/vek280/rpu/sank.xsa  outdir
sdtgen sdt.tcl /wrk/xsjhdnobkup1/jhwang/Vitis_Embedded_Platform_Source/Xilinx_Official_Platforms/vek280_base/hw/build/hw.xsa outdir
mkdir integration_flow
pushd integration_flow
   
ln -s /proj/xbuilds/2026.1_daily_latest/installs/lin64/2026.1/Vitis/data/embeddedsw ./
empyro repo -st ./embeddedsw 
empyro create_bsp -w test_bsp -s ../outdir/system-top.dts -p psv_cortexr5_0 -t hello_world
empyro config_bsp -d test_bsp -al aiebaremetal
empyro build_bsp -d test_bsp
popd
