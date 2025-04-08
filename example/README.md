# AI Engine (AIE) Bare-metal

This repo supports AIE baremetal runtime, including the library and tests.

## Clone the Repo

The repos contains submodules and should be clone recursively:

```bash
$ git clone <aiebaremetal repo> --recursive
```

If the repo is cloned without `--recursive` command, the user can use

```bash
$ git submodule update --init --recursive
```

to download the submodules.

## Repo Example Directory

```
├── Makefile
├── README.md
├── example_external_buffer
├── example_GMIO
├── example_kernelplio
├── example_lwip_echo
├── example_multi_partition
├── example_oob
├── example_softmax
├── testadf
├── testaie              *
├── testapurpu
├── testkernel           *
├── testkernel_rtp
├── testkernel_rtp_sync
├── testuc
└── testut          
```

Test folders with `*` are described below:

- (missing) `KTEST/`: The aie kernel test for baremetal runtime.
- `testaie/`: Basic test to verify simulation setup is correct and `aie-rt` driver is working fine.
- `testkernel/`: AIE kernels and application which will use baremetal runtime.
- `testut/`: Tests to verify baremetal runtime APIs, multiple kernel scenarios, synchronization, etc.

## Test/Example Compilation

### Enter the example folder

```bash
cd aiebaremetal
cd example
```

### Compile with Default Options

```bash
$ make -j32 # Compile the entire repo
```

will build all libraries (including submodules) and applications, including:

- aie baremetal rumtime example--testkernel (simulator): source code under `testkernel/`, described in detail below.
- aie uc test firmware for aie2ps: source code under `testuc/ucfirmware/`, target is `testuc/ucfirmware/output/`.
- aie test app (RPU for vck190)
- aie test app (APU for vek280)
- aiedriver static library (RPU for vck190)
- aiedriver static library (APU for vek280)

The command also compiles one of the example kernels described below, as well as files under `testuc/` and `testaie/`.

### Specify Locations of AIE Graph and Kernels

When compiling the library of AIE baremental runtime (`libaiebaremetal.so` for the simulator or `libaiebaremetala72.a` for hardware), the AIE graph and kernels are also read and compiled together.
Thus, we will want to specify the subfolders that contains the desired graph and kernels in the `make` command mentioned in the last step.
Here are two examples:

```bash
$ make AIE_KERNEL_LOC=$PWD/testaie/kernel AIE_GEN=2 -j32 # Build testaie/
$ make AIE_KERNEL_LOC=$PWD/testkernel/ AIE_GEN=2 -j32 # Build testkernel/
```

> If not specified, the default value of `AIE_KERNEL_LOC` is `$PWD/testkernel/`, while `$PWD` is where the `Makefile` is.

> The source code of the AIE graph (`graph.h` and `graph.cpp`) and kernels (e.g., `square.h` and `square.cpp`) is under `testkernel/src/`.

### Compile only the Bare-metal Runtime Library

Since the whole build process can take some time, there are commands to re-build only the bare-metal runtime library.
For example, after enabling `gdb_point` for debug, we can rebuild the baremetal library with

```bash
$ make ./build/libaiebaremetal.so AIE_KERNEL_LOC=$PWD/testaie/kernel AIE_GEN=2 -j32
```

### Compile for Hardware Test

Pass the `hw` flag to `make` to generate image to load on the real hardware.

To compile an image to run on **vck190**, one can specify `AIE_GEN=1` or simply pass `AIE_GEN` flag to `make`:

```bash
# vck190
$ make hw AIE_GEN=1 # OR ``make hw AIE_GEN''
$ ls ./build/Vck190BOOT.BIN # Check the existance of the build target
```

To compile an image to run on **vek280**, one can specify `AIE_GEN=2`:

```bash
# vek280
$ make hw AIE_GEN=2
$ ls ./build/Vek280BOOT.BIN # Check the existance of the build target
```

<!-- > The user may need to re-compile the entire project for `AIE_GEN=2` to build hardware target (image) for vek280.
> Build command: `make clean; make AIE_GEN=2 -j32`. -->

Again, the user may want to specify the location for the desired AIE graph and kernels:

```bash
# Compile AIE kernel and graph under testkernel/
$ make hw AIE_KERNEL_LOC=$PWD/testkernel/ AIE_GEN=1
$ ls ./build/Vck190BOOT.BIN # Check the existance of the build target
```
### Using compile.sh to compile customize app

#### Settup the environment

```bash
source ./script/settings.sh 1
```

### Prepare your code and compile

For example the code is under /example/ folder

```
example          
	├── graph.cpp         
	├── graph.h
	├── kernel.cc
```

compile to generate the elf for vck190 APU

```bash
cd /example/
compile.sh --runtime_source_file ./graph.cpp --aie_version 1
```
compile to generate the elf for vck190 RPU

```bash
cd /example/
compile.sh --runtime_source_file ./graph.cpp --aie_version 1 -rpu
```

compile to generate the BOOT.BIN for vck190 APU

```bash
cd /example/
compile.sh --runtime_source_file ./graph.cpp --aie_version 1 -bootgen
```
compile to generate the BOOT.BIN for vck190 RPU

```bash
cd /example/
compile.sh --runtime_source_file ./graph.cpp --aie_version 1 -bootgen -rpu
```

#### Target Test board for the said BOOT.BIN and elf

* VCK190 APU & RPU:

*B-revB01,S80-prod*
*B-revB02,S80-prod*

* VEK280 APU:

*ES1(no prod label box)*

* VEK280 RPU:

*B-revB03,S-prod*


## Run a Test

The repo also provides a local test so that users can test APIs without using a simulator.

```bash
$ make ./build/test # Compile the local test
$ ./build/test # Run the test
```

## Performance

OOB Baremetal in Vek280

```
Output took 462214 clock cycles.
Output took 2.31 ms.
```

