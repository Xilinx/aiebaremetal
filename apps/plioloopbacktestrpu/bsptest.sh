# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#source ../../../script/settings.sh 1
rm -rf ./outdir
rm -rf ./integration_flow
source /proj/xbuilds/2025.2_daily_latest/installs/lin64/2025.2/Vitis/settings64.sh
#sdtgen sdt.tcl /proj/xsjsswstaff/huaj/aegapi/xsa/vck190/rpu/vpl_gen_fixed.xsa  outdir
sdtgen sdt.tcl /proj/xsjsswstaff/huaj/aegapi/xsa/vck190/rpu/rpubase.xsa outdir
mkdir integration_flow
pushd integration_flow
   
ln -s /proj/xbuilds/2025.2_daily_latest/installs/lin64/2025.2/Vitis/data/embeddedsw ./
empyro repo -st ./embeddedsw 
empyro create_bsp -w test_bsp -s ../outdir/system-top.dts -p psv_cortexr5_0 -t hello_world
empyro config_bsp -d test_bsp -al aiebaremetal
#empyro build_bsp -d test_bsp
popdq