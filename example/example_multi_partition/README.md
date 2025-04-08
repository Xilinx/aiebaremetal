# Compiling AIE engine graphs for independent partitions in Baremetal 

This test case is designed to demonstrate the flow for compiling AI Engine Graphs for AI Engine partitions in vck190. The AI Engine graphs are located in different partitions of the device, verified by aiesimulator independently, but integrated by v++ linker and packager into device.

There are two graphs in this application under folders test_partition1 and test_partition2.

## Steps to compile and merge the graphs

### Compile the first graph under test_partition1

```bash
cd test_parition1
source build.sh 1
source runsim.sh 1
cd ..
```
### Compile the second graph under test_partition2

```bash
cd test_parition2
source build.sh 1
source runsim.sh 1
cd ..
```
### Merge the generated json files

```bash
v++ -l --platform /proj/xbuilds/2024.2_weekly_latest/internal_platforms/xilinx_vck190_base_202420_1/xilinx_vck190_base_202420_1.xpfm test_partition1/libadf.a test_partition2/libadf.a --save-temps

v++ -p -s --platform /proj/xbuilds/2024.2_weekly_latest/internal_platforms/xilinx_vck190_base_202420_1/xilinx_vck190_base_202420_1.xpfm a.xsa test_partition1/libadf.a test_partition2/libadf.a



```

This will generate the merged json file under folder _x/package with name aie_control_config_mod.json.

### Compile the application with merged json

```bash
source build.sh 1
```
### Create a BIF file to generate BOOT.BIN

Create a BIF file to include all the CDOs from both graphs and the ELF. For eg. BIF file can look like below

all:
{
    image
    {
        { type=bootimage, file=./arch/platform/pdi/vck190/allgmiorpu.pdi }
    }
    image
    {
        name=aie_image, id=0x1c000000
        { type=cdo
          /* The following commented lines show the CDOs used to create a merged CDO 'aie.cdo.merged.bin'. For debugging purpose,
             uncomment these CDOs and comment the line that adds 'aie.cdo.merged.bin' */
          file = /scratch/staff/sangejha/src/baremetal-runtime/independent_graphs/with_fix/aiebaremetal/test/test_multi_partition/_x/package/libadf_n0/sw/aie.cdo.reset.bin
          file = /scratch/staff/sangejha/src/baremetal-runtime/independent_graphs/with_fix/aiebaremetal/test/test_multi_partition/_x/package/libadf_n1/sw/aie.cdo.reset.bin
          file = /scratch/staff/sangejha/src/baremetal-runtime/independent_graphs/with_fix/aiebaremetal/test/test_multi_partition/_x/package/libadf_n0/sw/aie.cdo.clock.gating.bin
          file = /scratch/staff/sangejha/src/baremetal-runtime/independent_graphs/with_fix/aiebaremetal/test/test_multi_partition/_x/package/libadf_n1/sw/aie.cdo.clock.gating.bin
          file = /scratch/staff/sangejha/src/baremetal-runtime/independent_graphs/with_fix/aiebaremetal/test/test_multi_partition/_x/package/libadf_n0/sw/aie.cdo.error.handling.bin
          file = /scratch/staff/sangejha/src/baremetal-runtime/independent_graphs/with_fix/aiebaremetal/test/test_multi_partition/_x/package/libadf_n1/sw/aie.cdo.error.handling.bin
          file = /scratch/staff/sangejha/src/baremetal-runtime/independent_graphs/with_fix/aiebaremetal/test/test_multi_partition/_x/package/libadf_n0/sw/aie.cdo.elfs.bin
          file = /scratch/staff/sangejha/src/baremetal-runtime/independent_graphs/with_fix/aiebaremetal/test/test_multi_partition/_x/package/libadf_n1/sw/aie.cdo.elfs.bin
          file = /scratch/staff/sangejha/src/baremetal-runtime/independent_graphs/with_fix/aiebaremetal/test/test_multi_partition/_x/package/libadf_n0/sw/aie.cdo.init.bin
          file = /scratch/staff/sangejha/src/baremetal-runtime/independent_graphs/with_fix/aiebaremetal/test/test_multi_partition/_x/package/libadf_n1/sw/aie.cdo.init.bin
          /*file=aie.merged.cdo.bin*/
        }
    }
    image
    {
        name=default_subsys, id=0x1c000000
        { core=a72-0, file=./vck190.elf }
    }
}

### Generate BOOT.BIN and test on VCK190 HW

```bash
/proj/xbuilds/2022.2_daily_latest/installs/lin64/Vitis/2022.2/bin/bootgen -arch versal -image ${BIF} -o ${BOOTBIN} -w
```
