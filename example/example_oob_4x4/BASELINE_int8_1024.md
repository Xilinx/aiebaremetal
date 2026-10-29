<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# example_oob_4x4 — Gen-5 (AIE2PS) INT8 Baseline, 1024×1024

This is the **INT8 1024×1024 reference** for the `example_oob_4x4` GEMM workload on
the 4×4 systolic array. It is the large-matrix counterpart of
[`BASELINE_int8.md`](BASELINE_int8.md) (INT8, 256×256) and
[`BASELINE.md`](BASELINE.md) (INT32, 256×256). Each matrix is now 1024×1024, built
as a **blocked GEMM** of 4×4 = 16 output panels with host-side k-accumulation over
4 k-panels (the on-core tile still works on a 256×256 panel — panel size is
unchanged, only the number of panels grows). Inputs A and B are `int8`; the matrix
engine accumulates in `int32` and C streams out as `int32`. Do not edit the numbers
below — capture new runs in separate reports and compare.

Raw report: [`profiling_report_int8_1024.log`](profiling_report_int8_1024.log)

## Provenance

| | |
|---|---|
| Workload | INT8 blocked GEMM, `aie::mmul<4,16,8,int8,int8>` (int8×int8→int32) compute path |
| Target | vek385 — `--part=xc2ve3858`, `--aiearch aie2ps` (AIE gen 5 / AIE2PS) |
| Build | `bash build.sh 5 -bootgen` (exit 0), boot image `build/vek385.BIN` |
| Toolchain | Vitis 2026.2 (`9999.0_0727_1936`) |
| Board | **crimini5** (VEK385), programmed via `src/tool/test/runhw_vek385.py` |
| Run date | 2026-07-28 |
| Config | panel 256, **`NUM_PANELS=4` → 1024×1024**, 8 matrices, blocked 4×4×4 (host k-accum) |
| Array | 4×4 = **16 tiles** of 144 on device |
| Launch model | **single `gr.run(512)`**; one A-stripe + B-panel streamed per iteration (no per-panel re-arm, no manual lock reset) |
| Verification | Spot check PASSED (int8×int8→int32, C = 1024·A since B≡1) |

## Headline numbers

| Metric | Value | Notes |
|---|---|---|
| Problem size | **8 × 1024×1024** | 512 panel-multiply steps, 128 output panels |
| Total GEMM time | **515.969 ms** | 8 matrices, 512 panel-multiplies |
| Wall-clock throughput | **33.296 GOPS** | 8 × 2·M³ / total (real PS timer, clock-free) |
| Per panel-multiply | **51.996 GOPS** (**0.052 TOPS**) | 2·panel³ / avg panel time |
| GEMM excl. verify | 459.364 ms → **37.399 GOPS** | verification store removed |
| Vec slot utilization | **0.5 %** | 208× headroom |
| Compute fraction | 64.3 % of active cycles | rest is lock stall |
| Lock stall | **35.7 %** of active cycles | waiting on buffer locks |
| Stream stall | 0.0 % | not input-starved at the core |
| Device INT8 peak | **184 TOPS** | 144 tiles |
| Array INT8 peak | **20.44 TOPS** | 16/144 active tiles |
| HW util vs 16 active tiles | **0.254 %** | INT8 |
| HW util vs full device (144) | **0.028 %** | INT8 |

## Key finding: scaling to 1024 does not change the per-panel physics

The 16-tile array works on a fixed 256×256 on-core panel regardless of matrix size,
so scaling M from 256 → 1024 just runs **64× more panel-multiplies** (8 → 512). The
per-panel behavior is identical to the 256 baseline: **51.996 GOPS/panel** vs 51.740
(within noise), **0.5 %** vec utilization, **0 %** stream stall, and `gmio_wait` at
**89.7 %** of panel wall. Nothing about the bottleneck moved — the workload is still
**feed/sync bound, not compute bound**.

Two things *did* improve with the larger problem:

- **Wall-clock throughput rose 24.7 → 33.3 GOPS.** With 512 panels instead of 8,
  fixed one-time costs (`gr.init()`, the single `gr.run()` launch) amortize over far
  more work, so the aggregate M³ rate climbs even though each panel is unchanged.
- **Host post-processing shrank as a fraction of wall (50.1 % → 35.9 %)**, for the
  same amortization reason — though k-accumulation is now the largest host cost
  (24.9 %) because 1024 requires summing 4 k-panels per output panel.

Everything else is a rerun of the same ceilings at 64× the count.

## Where the time goes

Same ceilings as the 256 int8 baseline, essentially unchanged per panel:

### 1. Feed / drain ceiling (dominant)
`gmio_wait` is **89.7 %** of the per-panel wall (0.602 ms of 0.670 ms), and input
streams at **0.139 B/cycle** (80 kB over 589,856 cyc) — the same per-byte wire rate
as the 256 run. B is still broadcast to every input row. Levers: share/cascade B
across rows, keep B resident, widen the input DMA, or overlap iterations (deeper
GMIO queue) so the PS is not serialized on one `gmio_wait` per panel.

### 2. Lock stall is 35.7 % — buffer/sync ceiling
Just over a third of active cycles wait on buffer locks (313,214 of 878,472),
matching the 256 int8 run (34.6 %). Compute retires 4× faster in int8 between the
same window acquire/release round-trips, so the core spends proportionally more time
lock-stalled. Levers: coarser window granularity, deeper ping-pong, fewer
acquire/release round-trips, stage B once per N-block.

### 3. Per-tile vector utilization is 0.5 % — compute ceiling (far away)
The core issues a vector instruction on only 0.5 % of active cycles
(`Ceiling FLOP/cycle 7944` vs `Array FLOP/active-cycle 38.2`). ~208× compute
headroom; raising it only helps once the feed/lock ceilings above are lifted.

### 4. Only 16 of 144 tiles are used — device-level ceiling
The array is 4×4. int8 device peak is 184 TOPS (144 tiles); this run uses 16 tiles
(20.44 TOPS array peak) and hits 0.052 TOPS — **0.028 % of the device**. Array size
is the biggest lever for *device* utilization, and it is size-independent (a bigger
matrix does not use more tiles).

### 5. Host post-processing is 35.9 % of wall time — system ceiling
PS-side k-accumulation (`cAcc += mem_out`) is now **24.9 %** (it sums 4 k-panels per
output panel — this is new work that only exists at NUM_PANELS>1), and the `all_C`
verification store is **11.0 %** (removed for the "excl. verify" number).

## What changed vs the 256 INT8 baseline (code)

- `src/kernels/config.h`: **`NUM_PANELS` 1 → 4**, giving `MATRIX_SIZE = 4·256 =
  1024`. Nothing else changed. The on-core panel (`PANEL_SIZE=256`), mmul shape
  `<4,16,8>`, window byte sizes, and all kernels/graph connections are identical —
  only the host loop count (m/n/k panel loops → 512 iterations) and the packed
  `dev_A`/`dev_B`/`all_C` buffer sizes scale up.
- No kernel, `graph.h`, or launch-model changes: the single `gr.run(total_iters)`
  streaming model from the 256 int8 baseline carries over unmodified; the ADF runtime
  advances the ping-pong locks across all 512 iterations.

## 256 → 1024 at a glance (INT8, same 16 tiles)

| Metric | 256 (NUM_PANELS=1) | 1024 (NUM_PANELS=4) |
|---|---|---|
| Panel-multiply steps | 8 | 512 |
| Total GEMM time | 10.868 ms | 515.969 ms |
| Wall-clock throughput | 24.699 GOPS | **33.296 GOPS** |
| Per panel-multiply | 51.740 GOPS | 51.996 GOPS |
| gmio_wait share of panel | 91 % | 89.7 % |
| Lock stall | 34.6 % | 35.7 % |
| Vec utilization | 0.5 % | 0.5 % |
| Host post-proc share | 50.1 % | 35.9 % |
| HW util (144-tile device) | 0.028 % | 0.028 % |

## Optimization backlog (measure each against this baseline)

1. Lift the feed ceiling: share/cascade/keep-resident B; overlap GMIO across iterations.
2. Cut lock stalls from 35.7 % (window granularity, ping-pong depth, B re-staging).
3. Scale the array past 4×4 toward the 144-tile device (device-util lever, size-independent).
4. Move k-accumulation on-device — at 1024 it is the biggest single host cost (24.9 %).
5. Only then chase vec utilization (0.5 %) — compute is not the bottleneck here.

## Reproduce

```bash
cd example/example_oob_4x4
# NUM_PANELS defaults to 4 in src/kernels/config.h -> 1024x1024 matrices
bash build.sh 5 -bootgen      # gen 5 (AIE2PS), boot image at build/vek385.BIN

# program + run on the crimini5 VEK385 board (streams the report to stdout):
export VEK385IP=crimini5 USERNAME=bkirinci
export VEK385_LOCAL_BIN=$PWD/build/vek385.BIN
export VEK385_LOCAL_ELF=$PWD/vek385.elf
export VEK385PDI=/home/$USERNAME/naiebaremetal/vek385.BIN
export SYSTEST_TIMEOUT=420   # 512 panel-multiplies takes longer than the 256 run
python3 ../../src/tool/test/runhw_vek385.py
```
