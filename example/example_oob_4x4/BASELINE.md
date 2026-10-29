<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# example_oob_4x4 — Gen-5 (AIE2PS) Baseline

This is the **reference baseline** for the `example_oob_4x4` INT32 GEMM workload on
the 4×4 systolic array. All optimization work is measured against these numbers.
Do not edit the numbers below — capture new runs in separate reports and compare.

Raw report: [`profiling_report.log`](profiling_report.log)

## Provenance

| | |
|---|---|
| Workload | INT32 blocked GEMM, `aie::mmul<4,4,4>` compute path |
| Target | vek385 — `--part=xc2ve3858`, `--aiearch aie2ps` (AIE gen 5 / AIE2PS) |
| Build | `bash build.sh 5 -bootgen` (exit 0), boot image `build/vek385.BIN` |
| Toolchain | Vitis 2026.2 (`9999.0_0716_2341`) |
| Run date | 2026-07-17 |
| Config | panel 256, `NUM_PANELS=1`, 8 matrices of 256×256, `CEILING_REPEAT=1` (streaming-bound) |
| Array | 4×4 = **16 tiles** of 144 on device |
| Profiled section | printf-free kernels; all host reporting emitted **after** the timed loop |

## Headline numbers

| Metric | Value | Notes |
|---|---|---|
| Total GEMM time | **11.121 ms** | 8 matrices, 8 panel-multiply steps |
| Wall-clock GFLOPS | **24.137** | 8 × 2·M³ / total (real PS timer, clock-free) |
| Per panel-multiply | **49.393 GFLOPS** | 2·panel³ / avg panel time |
| GEMM excl. verify | 7.427 ms → **36.145 GFLOPS** | verification store removed |
| Vec slot utilization | **13.6 %** | 7.35× headroom |
| Compute fraction | 84.2 % of active cycles | rest is lock stall |
| Lock stall | **15.8 %** of active cycles | waiting on buffer locks |
| Stream stall | 0.0 % | not input-starved at the core |
| HW util vs 16 active tiles | **0.97 %** | INT32 |
| HW util vs full device (144) | **0.11 %** | INT32 |

## Where the time goes

Three independent ceilings are all far from saturated. In rough priority order:

### 1. Only 16 of 144 tiles are used — device-level ceiling
The array is 4×4. Device INT32 peak is 46 TOPS (144 tiles); this run uses 16 tiles
(5.11 TOPS array peak) and hits 49.4 GFLOPS — **0.11 % of the device**. Array size is
the single biggest lever for *device* utilization; everything below improves
*per-tile* efficiency, which multiplies whatever array size we land on.

### 2. Per-tile vector utilization is 13.6 % — compute ceiling
The core is "active" 84.2 % of cycles but only issues a vector instruction on 13.6 %
of them (`Ceiling FLOP/cycle 408 vs Array FLOP/active-cycle 55.5`). Even on the
`aie::mmul<4,4,4>` path, most active cycles are scalar/address-gen/loads and B
re-staging between `mmul` calls rather than MAC issue. **7.35× headroom** here.
Levers: larger `mmul` shapes, keep accumulators resident across K, cut the per-block
B repack, more independent accumulators to fill the pipeline.

### 3. Lock stall is 15.8 % — buffer/sync ceiling
Nearly a sixth of active cycles are spent waiting on buffer locks (window
acquire/release granularity, ping-pong depth). Stream stall is 0 %, so the core is
not starved for data on the wire — it's serialized on locks. Levers: coarser window
granularity, deeper ping-pong, fewer acquire/release round-trips.

### 4. Input GMIO is B-broadcast bound — feed ceiling
`in[0]` moves 320 kB in 557 k cycles (0.588 B/cycle) while the core is active for
604 k cycles — input streaming time ≈ compute time. Of the 320 kB, **256 kB is the
full B panel**, streamed to every input row (B is broadcast). Output is fast
(8 B/cycle). Levers: share/cascade B across rows instead of per-row broadcast, or
keep B resident.

### 5. Host post-processing is 50.9 % of wall time — system ceiling
Half the measured GEMM wall time is PS-side, not AIE:
- **all_C store — 33.2 %**: pure verification bookkeeping. Not part of the workload;
  removing it gives 36.145 GFLOPS. Measure real GEMM with this excluded (already
  reported as "GEMM excl. verify").
- **cAcc k-accum — 17.7 %**: PS reads device `mem_out` and accumulates partials each
  k-panel. Inherent to PS-side k-accumulation in the blocked design. Lever: accumulate
  on-device instead of round-tripping partials through the PS.

## Optimization backlog (measure each against this baseline)

1. Scale the array past 4×4 toward the 144-tile device (device-util lever).
2. Raise per-tile vec utilization from 13.6 % (mmul shape / accumulator residency / B staging).
3. Cut lock stalls from 15.8 % (window granularity, ping-pong depth).
4. Reduce B-broadcast input traffic (share/cascade/keep-resident B).
5. Move k-accumulation on-device; keep verification store out of the timed region.

## Reproduce

```bash
cd example/example_oob_4x4
bash build.sh 5 -bootgen      # gen 5 (AIE2PS), boot image at build/vek385.BIN
# boot vek385.elf / build/vek385.BIN on target; report prints to stdout
```
