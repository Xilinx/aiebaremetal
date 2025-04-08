# Test Kernel PLIO

This test folder contains the scripts and source codes to compile a test image (for simulation or on hardware) using PLIO.

## Compile for Simulation

### Steps

```bash
$ source ./build.sh
$ source ./runsim.sh
```

1. Run `source ./build.sh`: build the AIE application from the graph and kernel definitions.
2. Run `source ./runsim.sh`: run the AIE simulator and generate outputs. It can take several minutes in active polling.

### Note

* Do not directly run `aieldpath.sh` and `Makefile`, or switch the order when running the scripts. Otherwise, error due to conflict of environment variable will occur.
* The simulation of PLIO only involves AIE but not PL kernels. The input/output is directly read from/write to the file location specified in AIE graph (`graph.h`).
* The simulation output will be `testkernelplio/aiesimulator_output/aietopl_output.txt`. It includes timestamps. A user can manually compare it with `testkernelplio/data/golden.txt`.
* The `./build.sh` accepts `AIE_GEN` as an argument.

> `AIE_GEN` is the most important user-specified argument during the compilation.
> This argument is used to switch between different device.
> It will impact the part (`DEVICE_FILE`) and the location of the base platform.


### Compile AIE Baremetal Runtime Library

As mentioned in the root `README.md`, the user can specify a folder of AIE graph and kernels to compile into the AIE baremetal runtime library (`libaiebaremetal.so`).
This library will be used in the simulation step.
For this PLIO test, the command to build the library (`libaiebaremetal.so`) is:

```bash
# Call this command under the repo root.
$ make AIE_KERNEL_LOC=$PWD/testkernelplio/ AIE_GEN=1 -j32
```

> By default, this command will be called and `libaiebaremetal.so` will be build by `testkernelplio/build.sh`.
> One can disable this feature by setting the second argument of `testkernelplio/build.sh` to `0`.

```bash
# 2 means AIE_BEN=2, and 0 disables force-rebuild of libaiebaremetal.so
$ source ./build.sh 2 0
```

In this case, the user can use the first argument to specify the AIE generation, and the second argument to force re-build the AIE bare-metal library.

## Compile for Hardware

To compile a `BOOT.BIN` file that can be directly loaded on a machine, the user can use the following command.

```bash
# Call this command under the repo root. vck190
./buildall.sh 1
```

The user can specify the AIE generation: 1 as vck190 and 2 as vek280.
The generated file will be under `build/` such as `build/Vck190PLIOBOOT.BIN` or `build/Vek280PLIOBOOT.BIN`.

The command performs the following two steps:

Step | Action                        | Main Input File                                                                                                           | Main Output File                                                                                                            | Compile Script
:--: | ----------------------------- | ------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------
 1   | Compile AIE Baremetal Library | AIE-RT repo, graph definition, and baremetal library                                                                      | `libadf.a` and `libaiebaremetala72.a` files, see below                                                                      | `Makefile`
1-1  | Compile AIE Graph and Kernels | `testkernelplio/src/graph.h`                                                                                              | `testkernelplio/libadf.a` <br> `testkernelplio/Work/ps/c_rts/aie_control_config` <br> `testkernelplio/Work/ps/cdo/aie_cdo*` | `testkernelplio/build.sh`
1-2  | Compile AIE Linux Library     | `thridparty/aielib/aie-rt/dirver/` <br> `testkernelplio/Work/ps/c_rts/aie_control_config.json`                            | `thridparty/aielib/aie-rt/dirver/src/libxaieengine.so`                                                                      | `thridparty/aielib/aie-rt/dirver/src/Makefile.Linux`
1-3  | Compile AIE Baremetal Library | `thridparty/aielib/aie-rt/dirver/` <br> `testkernelplio/Work/ps/c_rts/aie_control_config.json`                            | `build/libxaieenginea72.a`                                                                                                  | `thridparty/aielib/aie-rt/dirver/src/Makefile` 
1-4  | Combine AIE Graph & Library   | `src/` and `build/libxaieenginea72.a`                                                                                     | `build/libaiebaremetala72.a`                                                                                                | `Makefile.HW`
 2   | Compile `BOOT.BIN`            | -                                                                                                                         | `build/Vck190PLIOBOOT.BIN`                                                                                                  | `testkernelplio/builda72.sh`
2-1  | Compile PL Kernels            | `testkernelplio/pl_kernels/{mm2s.cpp,s2mm.cpp}`                                                                           | `testkernelplio/pl_kernels/{mm2s.xo,s2mm.xo}`                                                                               | `testkernelplio/pl_kernels/build.sh`
2-2  | Link the Hardware Design      | `testkernelplio/pl_kernels/{mm2s.xo,s2mm.xo}` <br> `testkernelplio/libadf.a`                                              | `testkernelplio/pl_kernels/new.xsa` <br> `testkernelplio/.../vitis_design_wrapper.pdi`                                      | `testkernelplio/pl_kernels/build.sh`
2-3  | Generate Platform Drivers     | `testkernelplio/pl_kernels/new.xsa`                                                                                       | `testkernelplio/pfm_baremetal/`                                                                                             | `testkernelplio/builda72.sh`
2-4  | Build Application             | `build/libaiebaremetala72.a` <br> `testkernelplio/pfm_baremetal/` <br> `testkernelplio/src/graph.cpp`                     | `testkernelplio/vck190.elf`                                                                                                 | `testkernelplio/builda72.sh`
2-5  | Generate `BOOT.BIN`           | `testkernelplio/.../vitis_design_wrapper.pdi` <br> `testkernelplio/Work/ps/cdo/aie_cdo*` <br> `testkernelplio/vck190.elf` | `build/Vck190PLIOBOOT.BIN`                                                                                                  | `testkernelplio/builda72.sh`
> The exact PDI location is `testkernelplio/pl_kernels/_x/link/vivado/vpl/prj/prj.runs/impl_1/vitis_design_wrapper.pdi`.

The below sessions described the detailed steps taken by the above command.

### Step 1: Compile AIE Baremetal Runtime Library

The source code of the AIE baremetal runtime library form this repo.
Therefore, most of the comiplation steps are shared across each tests.
The only difference is the definition of the AIE graph and kernels, located in `testkernelplio/src/` in this case, is required to compile the final `.a` libraries.

1. **Compile AIE Graph and Kernels** (AIE): the user-defined AIE graph and kernels are under `testkernelplio/src` and will be compiled by AIE compiler into `libadf.a` (hardware graph and kernels) and `aie_control_config.json` (as AIE control info).
2. **Compile AIE Linux Library**: use the third-party AIE run-time library to generate the Linux-based baremetal control library.
3. **Compile AIE Baremetal Library**: reuse the C++ header files generated in the previous step and compile the baremetal library, again leverage AIE run-time library.
4. **Combine AIE Graph & Library**: compile this repo and generate the bare-metal library for the final host (PS) program compilation.

Outputs:
* `../build/libaiebaremetala72.a`: contains the AIE graph definition and is used when we compile the host control program.
* `testkernelplio/Work/ps/cdo/aie_cdo*`: contains AIE control method that will be integrated into final boot images.
* `testkernelplio/libadf.a`: we will use in generating new hardware design.

### Step 2: Build Flow of `BOOT.BIN`

After preparing the library for AIE, the script then prepare the hardware platform and the host application.
It includes five steps:

1. **PL Kernels**: Compile programmable logic (PL) kernels via high-level synthesis. In this example, the PL kernels are `mm2s` and `s2mm`, meaning memory to stream and stream to memory as PLIOs. The source C++ code will be compiled into Xilinx object files (`.xo`).
2. **Link**: Generate new hardware design. Vitis is used to load (link) the library object files such as `.xo` (PL kernels) and `libadf.a` (AIE graph and kernels) to the base platform (device dependent) and generate a new hardware design `.xsa` file.
3. **Platform Generation**: Generate bare-metal driver include files. The default hardware design does not have a bare-metal control domain. To get this information, we will need to use a built-in script `generate-platform.sh` to add this domain for PS application to include.
4. **PS** (processing system): Compile the `.elf` file for the host application with a cross-compiler. Here, we need to load `libaiebaremetala72.a` from step 1 and include `xparameters.h` from the previous platform generation.
5. **Package**: Wrap the design into a bootable image. `bootgen` reads `.bif` file to find the location of `.pdi` file from the Link stage, CDOs from step 1 (AIE stage) and `.elf` file from PS stage.

Here we briefly go through the commands in each scripts to understand the action.
Note that these commands are called automatically.

#### Compile PL Kernels

```bash
# testkernelplio/pl_kernels/build.sh
v++ --compile --platform $PLATFORM --save-temps -g -k s2mm s2mm.cpp -o s2mm.xo
v++ --compile --platform $PLATFORM --save-temps -g -k mm2s mm2s.cpp -o mm2s.xo
```

* Input file: `s2mm.cpp` and `mm2s.cpp`.
* Output file: `s2mm.xo` and `mm2s.xo`.
* `--platform`: This step is device-dependent and we need to load the base platform.

Reference: [Compiling C/C++ PL Kernels (UG1393)](https://docs.amd.com/r/en-US/ug1393-vitis-application-acceleration/Compiling-C/C-PL-Kernels)

#### Link the System

The LINK stage takes the kernel libraries, creates instances, and wrap them into a hardware design.

```bash
# testkernelplio/pl_kernels/build.sh
v++ --link --platform $PLATFORM s2mm.xo mm2s.xo ../libadf.a --target hw --save-temps -g --config system.cfg -o new.xsa
```

* Input file: `s2mm.xo`, `mm2s.xo`, and `libadf.a`.
* Output file: `new.xsa` and intermediate files under `_x/`.
* `--target` is set to `hw` to enable the synthesis. This option should remain the same through the build flow for each command if specified.
* `--save-temps` option saves the intermediate results under `_x` folder, which contains a PDI file (`_x/link/vivado/vpl/prj/prj.runs/impl_1/vitis_design_wrapper.pdi`) used later.
* `--link` option: Set Vitis to linking mode.
* `system.cfg` specifies the connection between the AIE and the PL kernels. Note the port name should align with the name defined in `testkernelplio/src/graph.h`

In `system.cfg`, we instantiated the PL kernels and connect them to AIE ports.

```bash
# testkernelplio/pl_kernels/system.cfg
[connectivity]
nk=mm2s:1:mm2s # <kernel_name>:<instance count>:<name1>,<name2>,...<name#>
nk=s2mm:1:s2mm
stream_connect=mm2s.s:ai_engine_0.plioin1 # <name>.<streaming_output_port>:<name>.<streaming_input_port>[:<fifo_depth>]
stream_connect=ai_engine_0.plioout1:s2mm.s
```

Reference:
* [Linking the System (UG1393)](https://docs.amd.com/r/en-US/ug1393-vitis-application-acceleration/Linking-the-System)
* [Connectivity Options (UG1393)](https://docs.amd.com/r/en-US/ug1393-vitis-application-acceleration/connectivity-Options)

### Generate Platform

The platform generation creates a folder containing all the required Board Support Packages (BSP), which will be included/loaded in PS programming.

```bash
# testkernelplio/script/builda72.sh
generate-platform.sh -name $PFM_NAME -hw ../pl_kernels/new.xsa -domain psv_cortexa72_0:standalone -domain ai_engine:aie_runtime
```

* Input file: `new.xsa`
* Output file: a folder named `$PFM_NAME`
* `generate-platform.sh` is a script come with Vitis and can be found under `$XILINX_VITIS/bin/`. The domain name is not arbitrary but has options defined in the script. The standalone domain will generate BSP for on-chip A72 processor.

Reference: [Building a Bare-Metal System (UG1393)](https://docs.amd.com/r/en-US/ug1393-vitis-application-acceleration/Building-a-Bare-Metal-System)

### Compile the PS Program

We use a cross-compiler alone with the BSP to build the host (PS) program.

```bash
# testkernelplio/script/builda72.sh
export SW_DOMAIN=./$PFM_NAME/export/$PFM_NAME/sw/$PFM_NAME/standalone_domain
aarch64-none-elf-gcc                                \
  -mcpu=cortex-a72                                  \
  -Wl,-T -Wl,$ARCH_72_DIR/lscript.ld                \
  -I../../thirdparty/aielib/aie-rt/driver/internal/ \
  -I$XILINX_VITIS/aietools/include/                 \
  -I$SW_DOMAIN/bspinclude/include                   \
  -L$ARCH_72_DIR/lib/                               \
  -L../../build/                                    \
  -o ./${ELF} ../src/graph.cpp                      \
  -Wl,--start-group,-laiebaremetala72,-lxil,-lgcc,-lc,-lstdc++,--end-group
```

* Input file: `testkernelplio/src/graph.cpp`.
* Output file: `${ELF}`, e.g., `vck190.elf`.
* `-mcpu` specifies the ARM core type. A72 is a 64-bit processor.
* `lscript.ld` defines the stack and heap size of the bare-metal program.
* `$SW_DOMAIN/bspinclude/include` includes the base address definition of PL kernels.
* `../../build/` includes `libaiebaremetala72.a` for bare-metal AIE graph APIs.

Note that in our example, we do not use the original runtime, but use the bare-metal runtime implemented in this repo.
Therefore, the graph declaration and the include path will be different from the reference.
The usage is similar since the bare-metal runtime overloads most of the functions.

```cpp
// testkernelplio/src/graph.cpp
#include "xparameters.h" // located in $SW_DOMAIN
BaremetalGraph gr("gradf2"); // in libaiebaremetala72.a, including the graph name "gradf2".
...
#define MM2S_BASE   XPAR_XMM2S_0_S_AXI_CONTROL_BASEADDR
```

The base address of the PL kernel `mm2s` is defined within `xparameters.h` (take vck190 as an example):

```c
// $SW_DOMAIN/xparameters.h
/* Canonical definitions for peripheral VITISREGION_MM2S */
#define XPAR_XMM2S_0_DEVICE_ID XPAR_VITISREGION_MM2S_DEVICE_ID
#define XPAR_XMM2S_0_S_AXI_CONTROL_BASEADDR 0xA4050000
#define XPAR_XMM2S_0_S_AXI_CONTROL_HIGHADDR 0xA405FFFF
```

Reference: [Host Programming for Bare-Metal (UG1076)](https://docs.amd.com/r/en-US/ug1076-ai-engine-environment/Host-Programming-for-Bare-Metal)

### Package the Image

System packaging can be done in either `--package` command in Vitis, or with `bootgen`.

Since the pacakge command is more straightforward, we will introduce this first before we discuss the bootgen method used in this example.
The package stage combines the platform with the compiled `.elf` file, and generates the bootable image `sd_card/BOOT.BIN`.
`v++ --package` internally calls `bootgen`.

```Makefile
# example Makefile, not in this repo
PKG_OPT = --save-temps
PKG_OPT += --package.out_dir ./sd_card
PKG_OPT += --package.defer_aie_run
PKG_OPT += --package.boot_mode sd
PKG_OPT += --package.ps_elf ./vck190.elf,a72-0

PKG_PFM = ./$(PFM_NAME)/export/$(PFM_NAME)/$(PFM_NAME).xpfm
PKG_OUT = aie_graph.xclbin

package:
  v++ --package --target hw --platform $(PKG_PFM) libadf.a new.xsa $(PKG_OPT) -o $(PKG_OUT)
```

In this repo, we call `bootgen` directly.

```bash
# testkernelplio/script/builda72.sh
/proj/xbuilds/2022.2_daily_latest/installs/lin64/Vitis/2022.2/bin/bootgen -arch versal -image vck190_boot_image.bif -o Vck190PLIOBOOT.BIN -w
```

`bootgen` takes `.bif` file, which defines the location of PDI, CDO, and `.elf` files.

Reference: [Packaging the System (UG1393)](https://docs.amd.com/r/en-US/ug1393-vitis-application-acceleration/Packaging-the-System)

---

### Concept Clarification

In this example, we compile `graph.cpp` twice using AIE compiler and ARM cross-compiler.
The first one generates `libadf.a` and the later one generates `main.elf`.
This is because the AIE compiler only take care of the graph connection defined in `graph.h`, but the ARM cross-compiler also requires the graph definition to compile the host program.
One can seperate the two condition by making `graph.cpp` empty (contains only simulation-related codes) and creating another `main.cpp` to `#include "graph.cpp"` for the host program.