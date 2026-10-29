<!-- Copyright (C) 2026 Advanced Micro Devices, Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# ResNet18 on Zephyr (VEK280)

Zephyr build glue for the ResNet18 demo. The host application sources are
**reused in place** from the baremetal tree (`../sw`); only the Zephyr-specific
files live here, so there is a single source of truth.

## Layout

```
res18sw/
  aie/        AIE design variants (Work/ CDOs, shared)
  sw/         baremetal host app  <-- REUSED, not copied
  zephyr/     <-- this directory (Zephyr build only)
    CMakeLists.txt          reuses ../sw/resnet18_top.cpp + include dirs
    prj.conf                app Kconfig (C++/STL, POSIX, networking)
    boards/versal_apu.conf  VEK280 APU board overlay (48-bit VA/PA, arenas)
    dts/bindings/           xlnx,aiengine binding
    build_zephyr.sh         graph build -> west build -> bootgen .BIN
```

The AIE generation / geometry comes from `script/aiengine_vek280.overlay` and
`-DAIE_GEN=2`, identical to `example/testkernel_zephyr`.

## Build

```bash
cd example/example_ai_model/aeg/res18sw/zephyr
./build_zephyr.sh            # single-instance (NUM_INST=1, default)
./build_zephyr.sh -inst10    # 10 instances (aie/batch1_inst10, BATCH_GRAPH)
```

Output image: `build/Vek280_zephyr.BIN` (xsdb `device program`).

`-inst10` (equivalently `-num_inst 10`) drives the multi-instance design:
`build_zephyr.sh` runs `../sw/buildbatch110.sh` for the graph/Work, and the app
is compiled with `-DNUM_INST=10 -DBATCH_GRAPH`. Default (no flag) is the
single-instance design. The AIE graph/`Work` and graph metadata are shared with
the baremetal build of the matching design — see `../sw/build.sh` (single) and
`../sw/buildbatch110.sh` (inst10).

Useful flags: `-no_west_update` (skip `west update`/lopper after first run),
`-no_graph_build` (skip the AIE/mladfpatch rebuild), `-zephyr_dir <path>`
(reuse an existing Zephyr workspace instead of `setup_zephyr.sh`).

> **Switching designs:** `-no_graph_build` is only safe when the last graph
> build matched the design you are flashing. After switching between
> single-instance and `-inst10`, run one full graph build for the new design;
> otherwise the stale shared graph metadata triggers a null-buffer data abort
> (`FAR=0x0`) at runtime.

Measured single-instance and inst10 numbers (Zephyr vs baremetal) are in
[`PERF_RESULTS.md`](PERF_RESULTS.md).

Console UART RX bring-up (SBSA PL011 init + RX prime + GICv3 `ARE_S` fix):
[`UART_RX_DEBUG.md`](UART_RX_DEBUG.md).
