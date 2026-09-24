<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# Testing Bare-metal application with BSP
## Build dependency files

### Build AIE app JSON files and CDO

```bash
source /proj/xbuilds/HEAD_daily_latest/installs/lin64/HEAD/Vitis/settings64.sh
pushd dependency/aie
make compile
popd
```

### Build the BSP locally (for testing) using scripts

This is an SDT-based BSP generation flow. In the `dependency` folder, [sdt.tcl](./dependency/sdt.tcl) uses `sdtgen` with a fixed XSA to create the `sdt` directory.
The [gensdtbsp.sh](./dependency/gensdtbsp.sh) script then uses that generated `sdt` directory to build the bare-metal BSP.

```bash
pushd dependency
source ./gensdtbsp.sh
popd
```

### Build the metadata `.cpp` and `boot.bin` with CDO

```bash
pushd dependency
source ./genmetacpp.sh
source ./genbootbin.sh
popd
```

## Build the bare-metal application and link to BSP

```bash
source ./buildmeta.sh
source ./buildmain.sh
source ./buildsdtbspelf.sh
```

## Copy ELF and boot image to home directory

```bash
cp ./main.elf ~/
cp ./dependency/testkernel.BIN ~/
```

## Test the app on the board farm

*Assumes board farm VEK280-4, targeting APU A72 core (typically target 6 in `xsdb`).*

**On systest host console**

```bash
<host username>:~$ /proj/systest/bin/systest vek280-4
[vek280-4] Systest# power 0 power 1
[vek280-4] Systest# xsdb
xsdb% conn
xsdb% tar 11
xsdb% device program /home/<your username>/testkernel.BIN
```

**On second client console**

```bash
ssh muscaria18
muscaria18:~% /opt/systest/common/bin/systest-client
[vek280-4] Systest# connect com0
```

**Back on systest host console**

```bash
xsdb% tar 6
xsdb% rst -proc
xsdb% dow -force /home/<your username>/main.elf
xsdb% con
```
 
# 2. [Early Access] Compile Application Using New API

*Skip to section 3.2 if you already ran section 1.*

The new AEG API is class-based rather than text-based.

Instead of calling functions like `gr.gm2aie_nb("gradf.in", ...)`, you use a typed graph class `AEGGraph<aeg::gradf>` with member ports: `gr.in`, `gr.out`, so that `gr.in.gm2aie_nb(...)` and `gr.out.aie2gm_nb(...)` are used instead.

The graph class and port members are generated from the ADF graph metadata and can be found in `generated_graphs.h` after running metadata generation.

### 3.1 Build AIE Routing/Kernel and generate `generated_graphs.h`

```bash
source /proj/xbuilds/HEAD_daily_latest/installs/lin64/HEAD/Vitis/settings64.sh
pushd dependency/aie
make compile
popd
```

This runs `aiecompiler` to produce JSON metadata, runs `genmetacpp.sh` to generate `generated_graphs.h` and `baremetal_metadata.cpp`, copies `generated_graphs.h` into `dependency/aie/` and into `src/include/`, then runs `aiecompiler` again for the full design.

Then generate the meta cpp and boot.bin as in the rest of the tutorial (see sections above):

```bash
pushd dependency
source ./genmetacpp.sh
source ./genbootbin.sh
popd

# Optional: view generated_graphs.h
vim generated_graphs.h
```

### 3.2 Build host_new_api.cpp

Build the aiebaremetal runtime library (if not already built), then the meta object and the **New API** host, and link the ELF:

```bash
source ./buildmeta.sh
source ./buildmain.sh -new_api
source ./buildsdtbspelf.sh -new_api
```

### 3.3 Run on board

Copy the New API ELF and the same pre-built `testkernel.BIN` as in section 2.3, then load and run `main_newapi.elf` on the APU (same steps as for `main.elf`):

```bash
cp ./main_newapi.elf ~/
cp ./dependency/testkernel.BIN ~/
```

Program the device with `testkernel.BIN`, then load and run `main_newapi.elf` on the APU (same steps as section 1):

```
xsdb% dow -force /home/<your username>/main_newapi.elf
xsdb% con
```


# 3. Testing Bare-Metal Application with Locally Built `aiebaremetal` Library

The BSP flow is the recommended way to build the bare-metal application.
However, if you want to build the bare-metal application with a locally built `aiebaremetal` library, you can follow the steps below.

# 1. Compile the bare-metal runtime library

## 1.1 Prerequisites

### A. Environment setup
   A.1 Vitis environment setup

    source /proj/xbuilds/HEAD_daily_latest/installs/lin64/HEAD/Vitis/settings64.sh

   A.2 Check environment library/header files. The folder includes embedded and AI Engine libraries and headers.

    ls ./dependency/psv_cortexa72_0/lib
    ls ./dependency/psv_cortexa72_0/include


### B. AIE routing and kernel compile
a. AIE routing and kernel compile

```bash
pushd ./dependency/aie
make compile
```

c. Generate the AIE metadata `.cpp`

```bash
source ../genmetacpp.sh
```

d. Generate the bare-metal `boot.bin` that includes the AIE CDO

```bash
source ../genbootbin.sh
```

```
# return to deploydemo folder
popd
source ./build_baremetal.sh
```

This compile generates `libaiebaremetal_api.a`, which should be integrated into `libxil.a` after
official release

# 2 Build app to use `libaiebaremetal_api.a` to accelerate the algorithm

## 2.2 Build the app

  a. Build the metadata object

 ```
 source ./buildmeta.sh
 ```

  b. Build the main app source file

  ```
  vim ./host.cpp
  # add new logic
  source ./buildmain.sh
  ```

  c. Build the app ELF

  ```
  source ./buildlocalbspelf.sh
  ```

  d. Copy ELF and pre-built test `boot.bin` into your home folder

  ```
  cp ./main.elf ~/
  cp ./dependency/testkernel.BIN ~/
  ```

## 2.3 Test the app on the board farm


_Assume board farm test box `vek280-4` and APU target 6._

**On systest host console**

```
<host username>:~$ /proj/systest/bin/systest vek280-4
[vek280-4] Systest# power 0 power 1
[vek280-4] Systest# xsdb
xsdb% conn
xsdb% tar 11
xsdb% device program /home/<your username>/testkernel.BIN
```

**On second client console**

```
ssh muscaria18
muscaria18:~% /opt/systest/common/bin/systest-client
[vek280-4] Systest# connect com0
On systest host console:

xsdb% tar 6
xsdb% rst -proc
xsdb% dow -force /home/<your username>/main.elf
xsdb% con
```
