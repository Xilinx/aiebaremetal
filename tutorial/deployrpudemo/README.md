<!--- Copyright (C) 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# Testing Bare-metal RPU Application with BSP

This tutorial demonstrates deploying an AIE bare-metal application on the **ARM Cortex-R5 (RPU)** of the VEK280 board.
All scripts use the `armr5-none-eabi` toolchain and RPU-specific compiler flags (`-mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -DARMR5`).

## Build Dependency Files

### Build AIE app JSON files and CDO

```bash
source /proj/xbuilds/HEAD_daily_latest/installs/lin64/HEAD/Vitis/settings64.sh
pushd dependency/aie
make compile
popd
```

### Build the BSP locally (for testing) using scripts

This is an SDT-based BSP generation flow. In the `dependency` folder, [sdt.tcl](./dependency/sdt.tcl) uses `sdtgen` with a fixed XSA to create the `sdt` directory.
The [gensdtbsp.sh](./dependency/gensdtbsp.sh) script then uses that generated `sdt` directory to build the bare-metal **RPU** BSP (`psv_cortexr5_0`).

```bash
pushd dependency
source ./gensdtbsp.sh
popd
```

### Build the metadata `.cpp` and RPU `boot.bin` with CDO

The RPU boot image uses `vek280_rpu_boot_image.bif`, which includes `rpu.pdi` and the AIE CDOs with an `r5-0` core entry.

```bash
pushd dependency
source ./genmetacpp.sh
source ./genbootbin.sh
popd
```

## Build the bare-metal RPU application and link to BSP

```bash
source ./buildmeta.sh
source ./buildmain.sh
source ./buildsdtbspelf.sh
```

## Copy ELF and boot image to your home directory

```bash
cp ./main.elf ~/
cp ./dependency/boot.BIN ~/
```

## Test the app on the board farm(Support PROD board only)

*Assumes board farm VEK280-9, targeting RPU R5-0 core (typically target 3 in `xsdb`).*

**On systest host console**

```bash
<host username>:~$ /proj/systest/bin/systest vek280-9
[vek280-9] Systest# power 0 power 1
[vek280-9] Systest# xsdb
xsdb% conn
xsdb% tar 11
xsdb% device program /home/<your username>/boot.BIN
```

**On second client console**

```bash
ssh bolete12
bolete12:~% /opt/systest/common/bin/systest-client
[vek280-9] Systest# connect com0
```

**Back on systest host console**

```bash
xsdb% tar 3
xsdb% rst -proc
xsdb% dow -force /home/<your username>/main.elf
xsdb% con
```

---

# [Early Access] Compile Application using New API

The new AEG API is class-based rather than text-based.

Instead of calling functions like `gr.gm2aie_nb("gradf.in", ...)`, you use a typed graph class
`AEGGraph<aeg::gradf>` with member ports (`gr.in`, `gr.out`), so `gr.in.gm2aie_nb(...)` and
`gr.out.aie2gm_nb(...)` are used instead.

The graph class and port members are generated from the ADF graph metadata and can be found in
`generated_graphs.h` after metadata generation.

## Build AIE routing/kernel and generate `generated_graphs.h`

```bash
source /proj/xbuilds/HEAD_daily_latest/installs/lin64/HEAD/Vitis/settings64.sh
pushd dependency/aie
make compile
popd
```

This runs `aiecompiler` to produce JSON metadata, runs `genmetacpp.sh` to generate `generated_graphs.h`
and `baremetal_metadata.cpp`, copies `generated_graphs.h` into `dependency/aie/` and into `src/include/`,
then runs `aiecompiler` again for the full design.

Then generate the metadata `.cpp` and `boot.bin`:

```bash
pushd dependency
source ./genmetacpp.sh
source ./gensdtbsp.sh
source ./genbootbin.sh
popd

# Optional: view generated_graphs.h
vim generated_graphs.h
```

## Build the New API host

```bash
source ./buildmeta.sh
source ./buildmain.sh -new_api
source ./buildlocalbspelf.sh -new_api
```

## Run on board

Copy the New API ELF and the same pre-built `testkernel.BIN`, then load it on the RPU:

```bash
cp ./main_newapi.elf ~/
cp ./dependency/boot.BIN ~/
```

Program the device with `testkernel.BIN`, then on the client console:

```bash
xsdb% tar 3
xsdb% rst -proc
xsdb% dow -force /home/<your username>/main_newapi.elf
xsdb% con
```
