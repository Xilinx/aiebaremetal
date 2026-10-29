<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# example_oob_4x4 — Gen-5 (AIE2PS) INT8 Baseline, 4096×4096 (single matrix)

This is the **INT8 4096×4096 reference** for the `example_oob_4x4` GEMM workload on
the 4×4 systolic array. It is the large-matrix counterpart of
[`BASELINE_int8_1024.md`](BASELINE_int8_1024.md) and
[`BASELINE_int8.md`](BASELINE_int8.md) (INT8, 256×256), and
[`BASELINE.md`](BASELINE.md) (INT32, 256×256).

Unlike the earlier int8 baselines, this run is a **single** full GEMM
(`NUM_MATRICES=1`, no benchmark batch) — one 4096×4096 · 4096×4096 multiply
distributed across the 16 tiles, matching the single-`gr.run` model of the original
`example_oob`. The matrix is built as a **blocked GEMM** of 16×16 = 256 output panels
with host-side k-accumulation over 16 k-panels (the on-core tile still works on a
256×256 panel — panel size is unchanged, only the number of panels grows). Inputs A
and B are `int8`; the matrix engine accumulates in `int32` and C streams out as
`int32`. Do not edit the numbers below — capture new runs in separate reports and
compare.

Raw report: [`profiling_report_int8_4096.log`](profiling_report_int8_4096.log)

## Provenance

| | |
|---|---|
| Workload | INT8 blocked GEMM, `aie::mmul<4,16,8,int8,int8>` (int8×int8→int32) compute path |
| Target | vek385 — `--part=xc2ve3858`, `--aiearch aie2ps` (AIE gen 5 / AIE2PS) |
| Build | `bash build.sh 5 -bootgen` (exit 0), boot image `build/vek385.BIN` |
| Toolchain | Vitis 2026.2 (`9999.0_0727_1936`) |
| Board | **crimini5** (VEK385), programmed via `src/tool/test/runhw_vek385.py` |
| Run date | 2026-07-28 |
| Config | panel 256, **`NUM_PANELS=16` → 4096×4096**, **1 matrix**, blocked 16×16×16 (host k-accum) |
| Array | 4×4 = **16 tiles** of 144 on device |
| Launch model | **single `gr.run(4096)`**; one A-stripe + B-panel streamed per iteration (no per-panel re-arm, no manual lock reset) |
| Verification | Spot check PASSED (int8×int8→int32, C = 4096·A since B≡1) |

## Headline numbers

| Metric | Value | Notes |
|---|---|---|
| Problem size | **1 × 4096×4096** | 4096 panel-multiplies, 256 output panels |
| Input prep (1-time) | 266.160 ms | packs 32 MB into `dev_A`/`dev_B`; not in GEMM time |
| Total GEMM time | **3825.289 ms** | 4096 panel-multiplies |
| Wall-clock throughput | **35.929 GOPS** | 2·M³ / total (real PS timer, clock-free) |
| Per panel-multiply | **51.966 GOPS** (**0.052 TOPS**) | 2·panel³ / avg panel time |
| GEMM excl. verify | 3712.222 ms → **37.023 GOPS** | verification store removed |
| Vec slot utilization | **0.5 %** | 207× headroom |
| Compute fraction | 64.5 % of active cycles | rest is lock stall |
| Lock stall | **35.5 %** of active cycles | waiting on buffer locks |
| Stream stall | 0.0 % | not input-starved at the core |
| Device INT8 peak | **184 TOPS** | 144 tiles |
| Array INT8 peak | **20.44 TOPS** | 16/144 active tiles |
| HW util vs 16 active tiles | **0.254 %** | INT8 |
| HW util vs full device (144) | **0.028 %** | INT8 |

## Key finding: per-panel physics is size-independent

The 16-tile array works on a fixed 256×256 on-core panel regardless of matrix size,
so scaling M just runs **more panel-multiplies** (256→1024→4096 ⇒ 8→64→4096 panels
here). The per-panel behavior is identical across all three int8 runs:
**~51.97 GOPS/panel**, **0.5 %** vec utilization, **0 %** stream stall, `gmio_wait`
at **89.9 %** of panel wall, and lock stall at **35.5 %**. The workload remains
**feed/sync bound, not compute bound** at every size.

What scaling to 4096 buys — and costs:

- **Wall-clock throughput edges up to 35.9 GOPS** (from 32.9 at 1024-single),
  because the fixed one-time costs (`gr.init()`, the single `gr.run()` launch) amortize
  over 4096 panels instead of 64. It is approaching the ~52 GOPS/panel ceiling but
  cannot exceed it while the array stays 4×4 and feed-bound.
- **k-accumulation is now the dominant host cost (27.9 %)**: with 16 k-panels per
  output panel, `cAcc += mem_out` runs 4096 times (1066 ms total). This is PS-side
  work that only exists at NUM_PANELS>1 and grows with the k-dimension.
- **The verification store shrank to 3.0 %** of wall (from 11 % at 1024): `all_C`
  stores 256 output panels once each, while the timed GEMM grew 60×, so verification
  is now negligible against the compute wall.
- **Device utilization is flat at 0.028 %** — a bigger matrix does *not* use more
  tiles, so the 144-tile device ceiling is unmoved by matrix size.

## Where the time goes

Same ceilings as the smaller int8 runs, essentially unchanged per panel:

### 1. Feed / drain ceiling (dominant)
`gmio_wait` is **89.9 %** of the per-panel wall (0.602 ms of 0.669 ms), and input
streams at **0.139 B/cycle** (80 kB over 590,363 cyc) — the same per-byte wire rate
as the 256/1024 runs. B is still broadcast to every input row. Levers: share/cascade
B across rows, keep B resident, widen the input DMA, or overlap iterations (deeper
GMIO queue) so the PS is not serialized on one `gmio_wait` per panel.

### 2. Lock stall is 35.5 % — buffer/sync ceiling
Just over a third of active cycles wait on buffer locks (311,025 of 876,252),
matching the smaller int8 runs. int8 compute retires 4× faster between the same
window acquire/release round-trips, so the core spends proportionally more time
lock-stalled. Levers: coarser window granularity, deeper ping-pong, fewer
acquire/release round-trips, stage B once per N-block.

### 3. Per-tile vector utilization is 0.5 % — compute ceiling (far away)
The core issues a vector instruction on only 0.5 % of active cycles
(`Ceiling FLOP/cycle 7944` vs `Array FLOP/active-cycle 38.3`). ~207× compute
headroom; raising it only helps once the feed/lock ceilings above are lifted.

### 4. Only 16 of 144 tiles are used — device-level ceiling
The array is 4×4. int8 device peak is 184 TOPS (144 tiles); this run uses 16 tiles
(20.44 TOPS array peak) and hits 0.052 TOPS — **0.028 % of the device**. Array size
is the biggest lever for *device* utilization, and it is size-independent (a bigger
matrix does not use more tiles).

### 5. Host post-processing is 30.8 % of wall time — system ceiling
PS-side k-accumulation (`cAcc += mem_out`) is **27.9 %** (it sums 16 k-panels per
output panel — this is the largest host cost at 4096), and the `all_C` verification
store is only **3.0 %** (removed for the "excl. verify" number).

## What changed vs the 1024 INT8 baseline (code)

- `src/kernels/config.h`: **`NUM_PANELS` 4 → 16**, giving `MATRIX_SIZE = 16·256 =
  4096`. On-core panel (`PANEL_SIZE=256`), mmul shape `<4,16,8>`, window byte sizes,
  and all kernels/graph connections are identical — only the host loop count
  (m/n/k panel loops → 4096 iterations) and the packed `dev_A`/`dev_B`/`all_C` buffer
  sizes scale up.
- `src/graph.cpp`: **`NUM_MATRICES` 8 → 1** — this run is a single full GEMM rather
  than a benchmark batch; the spot-checker now samples matrix 0 across a spread of
  panels/chains (including the corner panel `m=15,n=15`). No kernel, `graph.h`, or
  launch-model changes: the single `gr.run(total_iters)` streaming model carries over;
  the ADF runtime advances the ping-pong locks across all 4096 iterations.
- Memory: fits comfortably — APU heap is 1 GB and DDR is 2 GB, vs ~100 MB host
  (`all_C` 67 MB + `Afull`/`Bfull` 33 MB) and 32 MB device (`dev_A`+`dev_B`).

## INT8 size scaling at a glance (same 16 tiles)

| Metric | 256 (NP=1, ×8) | 1024 (NP=4, ×1) | 4096 (NP=16, ×1) |
|---|---|---|---|
| Panel-multiply steps | 8 | 64 | 4096 |
| Total GEMM time | 10.868 ms | 65.362 ms | 3825.289 ms |
| Wall-clock throughput | 24.699 GOPS | 32.855 GOPS | **35.929 GOPS** |
| Per panel-multiply | 51.740 GOPS | ~52 GOPS | 51.966 GOPS |
| gmio_wait share of panel | 91 % | 89.7 % | 89.9 % |
| Lock stall | 34.6 % | 35.7 % | 35.5 % |
| Vec utilization | 0.5 % | 0.5 % | 0.5 % |
| k-accum share of GEMM | n/a (NP=1) | 24.9 % | 27.9 % |
| verify-store share | 32.5 % | 11.0 % | 3.0 % |
| HW util (144-tile device) | 0.028 % | 0.028 % | 0.028 % |

## Optimization backlog (measure each against this baseline)

1. Lift the feed ceiling: share/cascade/keep-resident B; overlap GMIO across iterations.
2. Cut lock stalls from 35.5 % (window granularity, ping-pong depth, B re-staging).
3. Scale the array past 4×4 toward the 144-tile device (device-util lever, size-independent).
4. Move k-accumulation on-device — at 4096 it is the single biggest host cost (27.9 %).
5. Only then chase vec utilization (0.5 %) — compute is not the bottleneck here.

## Reproduce

```bash
cd example/example_oob_4x4
# NUM_PANELS defaults to 16 in src/kernels/config.h -> 4096x4096 matrices
# NUM_MATRICES = 1 in src/graph.cpp -> single full GEMM
bash build.sh 5 -bootgen      # gen 5 (AIE2PS), boot image at build/vek385.BIN

# program + run on the crimini5 VEK385 board (streams the report to stdout):
export VEK385IP=crimini5 USERNAME=bkirinci
export VEK385_LOCAL_BIN=$PWD/build/vek385.BIN
export VEK385_LOCAL_ELF=$PWD/vek385.elf
export VEK385PDI=/home/$USERNAME/naiebaremetal/vek385.BIN
export SYSTEST_TIMEOUT=600   # 4096 panel-multiplies takes ~4 s of GEMM + host post-proc
python3 ../../src/tool/test/runhw_vek385.py
```
