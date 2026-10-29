<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# Auto Radar 2D-FFT on Baremetal Runtime

## Overview

This application reads radar data and produces range and velocity measurements
using a 3-kernel pipeline running on 4 parallel AI Engine instances:

1. **RangeFFT** -- 2048-point FFT producing range data
2. **cint16_to_cint32** -- type conversion kernel
3. **VelocityFFT** -- 64-point FFT producing Doppler/velocity data

Two Mem Tiles perform intermediate transposes of the 4D output matrix
`[G][TX*RX][L]`, with a DDR bounce between them since the full matrix
(~12.5M samples) cannot fit on-chip.

See `ARCHITECTURE.md` for the full data flow diagram and design details.

## Design Parameters

| Parameter | Value |
|---|---|
| Range FFT point size | 2048 |
| Velocity FFT point size | 64 (= Groups) |
| TX channels | 24 |
| RX channels | 32 total (8 per instance, 4 instances) |
| Groups (G) | 64 |
| Radar cycle | 50 ms |
| Aggregate throughput | ~2.0 GSPS |

## Host API Versions

This example supports two host APIs:

- **Legacy (string-based)**: `src/graph.cpp` -- uses `AbrGraph`, direct
  `XAie_Write32` shim DMA config, manual `XAie_SetupConfig/CfgInitialize`.
- **New typed AEG API**: `src/graph_new_api.cpp` -- uses `AEGGraph<aeg::gradf>`,
  all shim DMA config handled by the runtime, no direct register access.

## Build Instructions

### Prerequisites

```bash
# One-time: download the Vitis DSP library
bash download_dsplib.sh
```

### Build with Legacy API (baseline)

```bash
source build.sh 2 -bootgen
```

### Build with New Typed AEG API

```bash
source build.sh 2 -new_api -bootgen
```

### Build Options

| Flag | Description |
|---|---|
| `2` | AIE generation (1, 2, or 5; default: 2) |
| `-new_api` | Use `graph_new_api.cpp` with typed AEG API |
| `-bootgen` | Generate `BOOT.BIN` for VEK280 |
| `-rpu` | Build for RPU instead of APU |
| `-perf` | Enable `[PERF]` phase timers (see Performance Testing) |

The build script handles all steps automatically:
1. Environment setup via `settings.sh`
2. Data extraction (`input.data`, `golden.data`)
3. AIE compilation (metadata generation + full compile)
4. CDO patching for conjugate symmetry buffer trimming
5. Baremetal library build + host ELF link
6. `BOOT.BIN` generation (when `-bootgen` is specified)

### Build Artifacts

| File | Description |
|---|---|
| `Work/` | AIE compiler output directory |
| `vek280.elf` | Baremetal host ELF |
| `build/Vek280BOOT.BIN` | Bootable image for VEK280 (with `-bootgen`) |

## Running on VEK280

Copy `BOOT.BIN` to the SD card, boot the board, and observe UART output.
The application runs the golden data check automatically and reports PASS/FAIL.

## Performance Testing

Both host versions carry phase timers that are compiled out unless you build
with `-perf`. A normal build contains no instrumentation.

### 1. Silence the runtime log before building

```bash
export EXTRA_CPPFLAGS=-DAEG_LOG=::aeg_log_inline::aeg_log_write_noop
```

Do this first, in the same shell you build from. Since commit `21d5166` the
runtime's `AEG_LOG` calls are compiled in for any build that does not define
`NDEBUG`, which includes every build here. That output leaves the board over a
115200 baud UART from inside the regions the timers bracket, and at roughly 11k
characters per second a few hundred log lines cost hundreds of milliseconds.
Measured with logging left on, a run reports `init` at 232 ms and `dma_config`
at 552 ms; silenced, the same phases are 0.23 ms and 0.12 ms. The numbers are
meaningless without this.

The export works because the runtime is compiled with `example/Makefile`'s
`EXTRA_CPPFLAGS ?=`, and `?=` defers to the environment. `aeg_log.h` wraps its
own definition in `#ifndef AEG_LOG`, so pre-defining the macro suppresses the
chatter while leaving `AEG_ERROR` intact and failures still reported. Note that
`compile.sh` passes `EXTRA_CPPFLAGS` on the `make` command line for `-linux`
and `-aeg_debug` builds, and a command-line assignment beats the environment,
so the export is ignored in those two cases. Run `unset EXTRA_CPPFLAGS` when you
want the normal chatty build back.

### 2. Build and run each version with timers

```bash
source build.sh 2 -perf -bootgen             # legacy API baseline
# boot the board, capture UART output to legacy.log

source build.sh 2 -new_api -perf -bootgen    # new typed AEG API
# boot the board, capture UART output to new_api.log
```

If you forget the export, `perf_report.py` counts the leftover `[AEG_LOG]` lines
and warns that the numbers are not usable rather than reporting them silently.

### 3. Generate the report

```bash
python3 script/perf_report.py legacy.log new_api.log
python3 script/perf_report.py legacy.log new_api.log --csv perf.csv -o perf_report.md
```

The report gives per-phase timings with a before-vs-after delta, plus
throughput in GB/s and MSPS. A single log also works if you only need one side.

### Measured phases

| Phase | Covers |
|---|---|
| `init` | `gr.init()` |
| `dma_config` | `shim_config1-4()` (legacy) vs `setupPingPongBuffers()` (new) |
| `malloc` | I/O buffer allocation, plus DDR bounce buffers in the new API |
| `preprocess` | Host-side chirp rotation and buffer replication |
| `gm2aie` | Issuing the four input GMIO transfers |
| `run_output_wait` | `gr.run()` through the final output wait |
| `golden_check` | Output comparison loop |
| `total` | Whole application, `init` through graph teardown |

Notes on interpretation:

- The legacy host's `sleep(100)` is skipped in `-perf` builds, since a fixed
  100 s delay would swamp `run_output_wait`. The golden check still passes.
- Throughput is an end-to-end figure over `run_output_wait`, counting input and
  output bytes. It is not a steady-state streaming rate, so it reads lower than
  the shim-counter numbers in the upstream IPSP design docs.
- The new API additionally calls `gr.wait()` inside `run_output_wait`; the
  legacy host has no equivalent call.

## References

- **Confluence**: https://confluence.amd.com/pages/viewpage.action?spaceKey=~sangejha&title=Auto+Radar+2d+FFT+application+on+Baremetal+runtime
- **Upstream design**: https://gitenterprise.xilinx.com/IPSP/auto_radar_2dfft/tree/main/2048x64_4inst
