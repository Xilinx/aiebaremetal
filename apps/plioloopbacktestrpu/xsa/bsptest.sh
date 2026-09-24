# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#source ../../../script/settings.sh 1
echo 111
rm -rf ./outdir
rm -rf ./integration_flow
rm -rf ./test_bsp
source /proj/xbuilds/2025.2_daily_latest/installs/lin64/2025.2/Vitis/settings64.sh
#sdtgen sdt.tcl /scratch/staff/huaj/aiebr/RPU/Vitis_Embedded_Platform_Source/Xilinx_Official_Platforms/xilinx_vck190_base/hw/build/my_project/vitis_design_wrapper.xsa outdir
#sdtgen sdt.tcl /scratch/staff/huaj/allgmio/20252/aie1build-nolibadf/allgmioboot/platform/hw.xsa  outdir
sdtgen sdt.tcl /proj/xsjsswstaff/huaj/aegapi/xsa/vck190/rpu/vpl_gen_fixed.xsa  outdir
#sdtgen sdt.tcl /proj/xsjsswstaff/huaj/aegapi/xsa/vck190/rpu/rpubase.xsa outdir
mkdir integration_flow
pushd integration_flow
   
ln -s /proj/xbuilds/2025.2_daily_latest/installs/lin64/2025.2/Vitis/data/embeddedsw ./
empyro repo -st ./embeddedsw 
empyro create_bsp -w test_bsp -s ../outdir/system-top.dts -p psv_cortexr5_0 -t hello_world
#empyro create_bsp -w test_bsp -s ../outdir/system-top.dts -p psv_cortexa72_0 -t hello_world
empyro config_bsp -d test_bsp -al aiebaremetal
#empyro build_bsp -d test_bsp
popd