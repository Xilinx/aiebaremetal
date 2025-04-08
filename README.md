# AI Engine (AIE) Bare-metal

This repo supports AIE baremetal runtime, including the library and tests.

## Clone the Repo

The repos contains submodules and should be clone recursively:

```bash
$ git clone https://path/to/aiebaremetal.git --recursive
```

If the repo is cloned without `--recursive` command, the user can use

```bash
$ git submodule update --init --recursive
```

to download the submodules.

## Repo Directory

```
├── tutorial
├── include
├── Makefile
├── license.txt
├── script
├── README.md
├── src                  (the baremetal source code)
├── example              (the baremetal test code)
└── thirdparty           (contains submodules)
```

## Compile the runtime library

### Pepare the AIE driver header file

#### Option1  Get the aie driver header file from the standalone bsp

#### Option2 Compile to get the header file from aie-rt source

```bash
cd thirdparty/aielib/
git clone https://github.com/path/to/aie-rt
cd ./driver/src
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
