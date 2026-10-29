<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# example_oob_4x4 — Gen-5 (AIE2PS) INT8 Baseline

This is the **INT8 reference** for the `example_oob_4x4` GEMM workload on the 4×4
systolic array. It is the int8 counterpart of [`BASELINE.md`](BASELINE.md) (INT32).
Inputs A and B are `int8`; the matrix engine accumulates in `int32` and the C
result streams out as `int32`. Do not edit the numbers below — capture new runs in
separate reports and compare.

Raw report: [`profiling_report_int8.log`](profiling_report_int8.log)

## Provenance

| | |
|---|---|
| Workload | INT8 blocked GEMM, `aie::mmul<4,16,8,int8,int8>` (int8×int8→int32) compute path |
| Target | vek385 — `--part=xc2ve3858`, `--aiearch aie2ps` (AIE gen 5 / AIE2PS) |
| Build | `bash build.sh 5 -bootgen` (exit 0), boot image `build/vek385.BIN` |
| Toolchain | Vitis 2026.2 (`9999.0_0727_1936`) |
| Board | **crimini5** (VEK385), programmed via `src/tool/test/runhw_vek385.py` |
| Run date | 2026-07-28 |
| Config | panel 256, `NUM_PANELS=1`, 8 matrices of 256×256, `CEILING_REPEAT=1` (streaming-bound) |
| Array | 4×4 = **16 tiles** of 144 on device |
| Launch model | **single `gr.run(8)`**; one A-stripe + B-panel streamed per iteration (no per-panel re-arm, no manual lock reset) |
| Verification | Spot check PASSED (int8×int8→int32, C = 256·A since B≡1) |

## Headline numbers

| Metric | Value | Notes |
|---|---|---|
| Total GEMM time | **10.868 ms** | 8 matrices, 8 panel-multiply steps |
| Wall-clock throughput | **24.699 GOPS** | 8 × 2·M³ / total (real PS timer, clock-free) |
| Per panel-multiply | **51.740 GOPS** (**0.052 TOPS**) | 2·panel³ / avg panel time |
| GEMM excl. verify | 7.335 ms → **36.597 GOPS** | verification store removed |
| Vec slot utilization | **0.5 %** | 204× headroom |
| Compute fraction | 65.4 % of active cycles | rest is lock stall |
| Lock stall | **34.6 %** of active cycles | waiting on buffer locks |
| Stream stall | 0.0 % | not input-starved at the core |
| Device INT8 peak | **184 TOPS** | 144 tiles |
| Array INT8 peak | **20.44 TOPS** | 16/144 active tiles |
| HW util vs 16 active tiles | **0.253 %** | INT8 |
| HW util vs full device (144) | **0.028 %** | INT8 |

## Key finding: int8 does **not** speed this workload up

Throughput is essentially unchanged from the INT32 baseline (51.7 GOPS vs 49.4
GFLOPS per panel-multiply), because this configuration is **entirely feed/sync
bound, not compute bound**:

- **`gmio_wait` (drain) is 91 % of the per-panel wall** (0.601 ms of 0.660 ms).
  The PS spends the panel almost entirely blocked waiting for the array to finish
  streaming, exactly as in INT32.
- **Input GMIO is the wire ceiling**: `in[0]` moves 80 kB at **0.139 B/cycle**
  (589 k cycles) — a quarter of the INT32 byte volume (int8 is 4× denser), but the
  per-byte stream rate is the same, so wall time barely moves.
- **The matrix engine got *denser*, so it is even *less* busy.** int8 `<4,16,8>`
  does 4× the MACs per instruction, so vector-instruction count fell to 4224 and
  **vec utilization dropped to 0.5 %** (was 13.6 % in INT32). The compute ceiling
  is now ~200× away — compute is nowhere near the bottleneck.
- **Lock stall rose to 34.6 %** (was 15.8 %): with each mmul retiring 4× the work,
  the core spends proportionally more of its active time waiting on window/ping-pong
  locks between the (unchanged) B re-staging and result flushes.

Because the achieved ops/s stayed flat while the int8 *peak* is 4× the int32 peak
(184 vs 46 TOPS), device utilization actually reads **lower** in int8 (0.028 % vs
0.11 %). This is expected and not a regression: the same silicon is doing the same
real work per second; int8 only raises the ceiling it is measured against.

## Where the time goes

Same three ceilings as INT32, re-ordered by how binding they now are:

### 1. Feed / drain ceiling (now dominant)
`gmio_wait` is 91 % of panel wall and input streams at 0.139 B/cycle. B is still
broadcast to every input row (256 kB→64 kB of the 80 kB is the B panel in int8).
Levers: share/cascade B across rows instead of per-row broadcast, keep B resident,
widen the input DMA, or overlap iterations (deeper GMIO queue) so the PS is not
serialized on one `gmio_wait` per panel.

### 2. Lock stall is 34.6 % — buffer/sync ceiling
A third of active cycles wait on buffer locks. int8 amplified this because compute
retires faster between the same window acquire/release round-trips. Levers: coarser
window granularity, deeper ping-pong, fewer acquire/release round-trips, and staging
B once per N-block rather than re-reading per column.

### 3. Per-tile vector utilization is 0.5 % — compute ceiling (far away)
The core issues a vector instruction on only 0.5 % of active cycles
(`Ceiling FLOP/cycle 7944 vs Array FLOP/active-cycle 38.8`). There is ~200×
compute headroom; raising it only helps once the feed/lock ceilings above are lifted.

### 4. Only 16 of 144 tiles are used — device-level ceiling
The array is 4×4. int8 device peak is 184 TOPS (144 tiles); this run uses 16 tiles
(20.44 TOPS array peak) and hits 0.052 TOPS — **0.028 % of the device**. Array size
is the biggest lever for *device* utilization.

### 5. Host post-processing is 50.1 % of wall time — system ceiling
Unchanged from INT32 (output C is still int32): `all_C` verification store is 32.5 %
(removed for the "excl. verify" number), PS-side k-accumulation is 17.6 %.

## What changed vs the INT32 baseline (code)

- `src/kernels/config.h`: `GEMM_AB_T=int8`, `GEMM_C_T=int32`; mmul shape `<4,16,8>`
  (`MM_K=16` so each per-row A load is a full 128-bit int8 sub-vector — `<4,8,8>`
  with 64-bit rows is rejected by `aie::concat`); split window byte sizes
  (`WIN_SIZE_BYTES_AB` int8 vs `WIN_SIZE_BYTES_C` int32) and vector widths
  (`VEC_IN=32` int8, `VEC_OUT=8` int32).
- `src/kernels/{one_input,two_inputs,one_output}.cc`, `mmul_gemm.h`, `kernels.h`:
  int8 A/B windows and buffers, int32 C windows/buffers/accumulator.
- `src/graph.h`: A/B connections use `WIN_SIZE_BYTES_AB`, C connections use
  `WIN_SIZE_BYTES_C`.
- `src/graph.cpp`: int8 `dev_A`/`dev_B`, int32 output; DMA byte sizes; INT8 TOPS
  reporting (device peak 184 TOPS, no INT8/4 downscale). **Launch model changed to a
  single `gr.run(total_iters)` with per-iteration GMIO streaming**, which removed the
  per-panel `gr.run(1)` and the hand-maintained inter-panel lock-reset block — the
  ADF runtime now advances the ping-pong locks across iterations. This also fixed an
  int8 correctness failure caused by the stale (INT32-derived) lock-reset table.

## Optimization backlog (measure each against this baseline)

1. Lift the feed ceiling: share/cascade/keep-resident B; overlap GMIO across iterations.
2. Cut lock stalls from 34.6 % (window granularity, ping-pong depth, B re-staging).
3. Scale the array past 4×4 toward the 144-tile device (device-util lever).
4. Only then chase vec utilization (0.5 %) — compute is not the bottleneck here.
5. Move k-accumulation on-device; keep verification store out of the timed region.

## Reproduce

```bash
cd example/example_oob_4x4
bash build.sh 5 -bootgen      # gen 5 (AIE2PS), boot image at build/vek385.BIN

# program + run on the crimini5 VEK385 board (streams the report to stdout):
export VEK385IP=crimini5 USERNAME=bkirinci
export VEK385_LOCAL_BIN=$PWD/build/vek385.BIN
export VEK385_LOCAL_ELF=$PWD/vek385.elf
export VEK385PDI=/home/$USERNAME/naiebaremetal/vek385.BIN
python3 ../../src/tool/test/runhw_vek385.py
```
