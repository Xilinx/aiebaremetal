<!--- Copyright (C) 2023 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# AI Engine Graph API - Baremetal

This repo supports AI Engine Graph API for baremetal, abbreviation is AEG, including the library and tests.

## Clone the Repo

The repos contains submodules and should be clone recursively:

```bash
$ git clone <aiebaremetal_repo_url> --recursive
```

If the repo is cloned without `--recursive` command, the user can use

```bash
$ git submodule update --init --recursive
```

to download the submodules.

## Repo Directory

```
├── example
├── include
├── Makefile
├── docs
├── license.txt
├── script
├── README.md
├── src                  (the baremetal source code)
├── test                 (the baremetal test code)
└── thirdparty           (contains submodules)
```

## Compile the runtime library

### Pepare the AIE driver header file

#### Option1  Get the aie driver header file from the standalone bsp

#### Option2 Compile to get the header file from aie-rt source

```bash
cd thirdparty/aielib/aie-rt/driver/src
mkdir ../../../include/xaiengine
make include
```

the header file will be in 

thirdparty/aielib/include

### Compile the runtimne library.

```bash
make release -f ./Makefile \
        CC=aarch64-none-elf-gcc \
        AR=aarch64-none-elf-ar \
        CFLAGS="-O2 -Wall -Wextra -fPIC -DNO_JSON -Dversal -DARMA72_EL3 -fno-tree-loop-distribute-patterns \
                -I./thirdparty/aielib/include/"

```

## Documentation

### Generate Documentation
	make Makefile doc-generate

HTML docs are populated in docs/tmp/html/

PDF doc will be available at docs/tmp/latex/refman.pdf

### Clean Documentation
	make Makefile doc-clean

Deletes tmp folder

## Various ways to compile a Baremetal application using AI Engine Graph API

AI Engine graph API will be compiled into a library which can be linked by an application. On a high level there are two ways in which this can be achieved. They are

1. Use the pre-built BSP(using Vitis GUI, empyro scripts or Vitis Python CLI) containing AI Engine graph API library, link the application to it.

2. Use AI Engine graph API source code along with application and build everything together.

On a detailed note, various compilation scenarios are supported in AI Engine graph API. They are explained with example below

1. Using compile.sh - Majority of the tests inside example folder use the script build.sh, which calls compile.sh to build the application. For eg. testkernel uses build.sh. compile.sh uses pre-built bsp(inside thirdparty), AI Engine graph API source code to build runtime library and aiecompiler to generate baremetal_metadata.cpp. These components are combined to finally generate BOOT.BIN.

2. example_kernelplio uses buildall.sh to build PL kernels, and then link AI engine design with PL and generate the XSA. This XSA is used to generate the BSP and application is linked to this BSP.

3. deploydemo does not depend on AI Engine graph API source code. It expects that BSP containing AI Engine graph API library is available. It takes the application source code, calls aiecompiler to generate the baremetal_metadata.cpp, compiles application code and baremetal_metadat.cpp and links to bsp to generate the final BOOT.BIN.

4. Like deploydemo, inside apps folder there is test called plioloopbacktest which depends on BSP. But this demonstrates how we can use a Makefile to generate bsp using embeddedsw and then link the application to it.

This gives us various ways to build AI Engine Graph API library and then link the application to it.

---
