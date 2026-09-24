<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

## What each stage does and how to use it

### aie cdos

```
aiecompiler -v  --platform=${PLATFORM} --disable-multirate-analysis src/graph.cpp --include=./src
```

### logicarch: compile the AIE graph to an archive

Why: v++ system-link needs the compiled AIE design to wire PLIO streams and generate the NoC/AIE integration.

How it works: aiecompiler transforms src/graph.cpp (your ADF graph) into libadf.a plus an AIE work directory (_aie).

Run it:

make logicarch

Common adjustments:

Add includes for your AIE kernels: extend AIE_OPTS with additional --include or source() in graph.cpp.

libsdf.a is the aie logic arch

```
$(AIECOMPILER) $(AIECOMPILER_COMMON_OPTS) --platform $(PLATFORM) $(KERNEL_1_COMPILE_FLAGS) $+
```

### pl module: compile each PL kernel to .xo

Why: v++ needs per-kernel object (.xo) built for the target device/platform.

How it works: v++ -c compiles each HLS/RTL kernel into a linkable .xo. The pattern rule %.xo: %.cpp assumes the top function name equals the file stem; otherwise specialize rules and pass -k <top_name>.

Run it:

make pl

Common adjustments:

If you have per-kernel interface/clock directives, add --config <kernel>.xo_config.ini to each specialized rule.

Switch TARGET=hw_emu for emulation builds.


### new xsa: system-link to produce the XSA

Why: The XSA is the hardware handoff of the integrated design (PS/PL/AIE/NoC), used for BSPs and boot images.

How it works: v++ -l links all .xo + libadf.a against the base .xpfm; it consumes system.cfg which contains [connectivity] (stream_connect lines tying PL CUs to AIE PLIO names) and [clock] (freqHz per CU).

Run it:

make xsa

Be sure that:

system.cfg PLIO names match the names created in your AIE graph (e.g., ai_engine_0.pl_in).

Your platform exposes the needed clocks, DDR, and NoC endpoints.

buildbsp: generate a baremetal BSP from the XSA

Why: To build PS applications (baremetal or RTOS) that control and orchestrate AIE/PL.