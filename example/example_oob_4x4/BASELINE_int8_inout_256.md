<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# example_oob_4x4 — Gen-5 (AIE2PS) INT8-in / INT8-out Baseline, 256×256

This is the **INT8-in / INT8-out reference** for `example_oob_4x4`, created so the
workload can be compared head-to-head against aiehlc's `simplematmul2_prof.cc`
(also int8-in / int8-out, 256³, 4×4 mesh). It differs from
[`BASELINE_int8.md`](BASELINE_int8.md) (int8-in / **int32**-out) only in the output
precision: each tile now **saturates its full-K int32 accumulator to int8** before
the result is streamed out, exactly as aiehlc does.

Single matrix (`NUM_MATRICES=1`, `NUM_PANELS=1`). Verification passed.

Raw report: [`profiling_report_int8_inout_256.log`](profiling_report_int8_inout_256.log)

## Provenance

| | |
|---|---|
| Workload | INT8 GEMM, `aie::mmul<4,16,8,int8,int8>` → int32 accum → **saturate to int8** |
| Target | vek385 — `--part=xc2ve3858`, `--aiearch aie2ps` (AIE gen 5 / AIE2PS) |
| Build | `bash build.sh 5 -bootgen` (exit 0) |
| Board | **crimini5** (VEK385), via `src/tool/test/runhw_vek385.py` |
| Run date | 2026-07-29 |
| Config | panel 256, `NUM_PANELS=1`, **1 × 256×256**, int8-in / int8-out |
| Array | 4×4 = **16 tiles** of 144 on device |
| Verification | Spot check PASSED (int32 golden clamped to int8 to match saturation) |

## Headline numbers

| Metric | Value | Notes |
|---|---|---|
| **Total GEMM time** | **0.916 ms** | single 256³; golden-check store **and** profiling instrumentation excluded |
| **Wall-clock throughput** | **36.621 GOPS** | 2·M³ / total (real PS timer) |
| Per panel-multiply | **51.253 GOPS** (0.051 TOPS) | 2·panel³ / avg panel time |
| — of which k-accum (host) | 0.237 ms (25.8 %) | `cAcc += mem_out`, real GEMM work, counted |
| Excluded: verify store | 0.448 ms | `all_C` copy for spot check — not GEMM |
| Excluded: profiling | 0.211 ms | perf-counter arm/read/reset — measurement only |
| Raw timed wall (ref) | 1.576 ms | region incl. store + profiling |
| **Output DMA volume** | **16 kB** | int8 out (was 64 kB int32) — 4× smaller |
| Input DMA volume | 80 kB | int8 A stripe + B panel (unchanged) |
| `gmio_wait` (drain) | 91.8 % of panel wall | feed/drain-bound |
| Lock stall | 8.8 % of active cycles | cold single-panel (uncontended) |
| Stream stall | 0.0 % | not input-starved at the core |
| Vector instructions / tile | **4 224** | dense `aie::mmul` matrix-engine issue |
| Vec utilization | 0.6 % | 161× headroom |

## What changed vs the int8-in / int32-out baseline (code)

- `src/kernels/config.h`: `GEMM_C_T = int8_t` (was int32), `VEC_OUT = 32` (int8
  lanes). All C connections in `graph.h` share `WIN_SIZE_BYTES_C`, so the whole C
  path (inter-tile gather **and** final GMIO) becomes int8 uniformly — valid
  because the systolic C path only *gathers* per-tile finals, it never
  re-accumulates across tiles.
- `src/kernels/mmul_gemm.h`: extract the accumulator as **int32** and **clamp to
  [-128, 127]** in C before storing to the int8 `intrmdtResult`. NB:
  `aie::accum::to_vector<int8>()` does **not** do a plain saturating clamp (its
  shift/round-narrow returned 0 on hardware) — extracting int32 and clamping is the
  correct, aiehlc-matching approach.
- `src/graph.cpp`: `mem_out`/`outC` are int8; output DMA uses `MAT_A_CHUNK_BYTES_OUT`
  (int8); the spot-check golden is clamped to int8 to match device saturation.
- Note: with B≡1 and K=256 every output saturates to 127, so the check is correct
  but numerically degenerate — the same regime aiehlc runs in. The datapath and DMA
  sizing are what this validates.

## Timing scope: what "GEMM time" counts

The reported **0.916 ms / 36.621 GOPS** is the pure GEMM: graph launch + input-DMA
issue + `gmio_wait` array drain + host k-accumulation (`cAcc += mem_out`). Two
non-workload items measured inside the timed region are **excluded**:
- **Golden-check store** (`all_C`, 0.448 ms) — copies the result aside so the host
  spot-check can run after `gr.end()`. Pure verification bookkeeping.
- **Profiling instrumentation** (0.211 ms) — arming the GMIO/core perf counters and
  the slow debug-path counter read/reset on the probe iteration.

The raw timed-region wall including both is **1.576 ms** (kept for reference only).

## Key finding: int8 output does not move the wall

The workload is **91.8 % `gmio_wait`-bound on the input side** (80 kB in vs 16 kB
out), so shrinking the output stream 4× vs int32-out barely touches end-to-end time.
The `[suspect]` flag on the output-throughput line is the report noticing the output
stream is now a tiny burst.

---

# Apples-to-apples vs aiehlc `simplematmul2_prof`

Both run the **same problem** — 256×256×256, int8-in / int8-out, saturate-at-end, on
the **same 4×4 = 16-tile** partition of the same VEK385 device — so per-tile work is
identical: **(256/4)·(256/4)·256 = 1,048,576 MACs/tile** (2,097,152 FLOP/tile).

aiehlc source: `../aiehlc_aiesim/example/tileprogram/ccode/simplematmul2_prof.cc`.
aiehlc numbers below are from its latest board run (autoperf `56.log`).

## The timing-methodology gap (why raw ms ≠ ms)

The two harnesses do **not** measure the same wall by default:

| | example_oob_4x4 (ours) | aiehlc simplematmul2_prof |
|---|---|---|
| Launch model | **single `gr.run`**, kernels loaded once at `gr.init()` (amortized, outside timer) | **per-call `matmul<<<mesh>>>`**, reloads the ELF onto all 16 tiles every launch |
| Timed window | `t_gemm_start → t_gemm_end` (DMA-issue + `gmio_wait` drain + host k-accum; **verify store & profiling excluded**) | `t0 → t1`: async launch → poll DDR until result present |
| PS wall timer | XTime **live** (0.916 ms GEMM-only) | XTime **frozen this boot → 0 ms**; falls back to A78 PMU cycles |
| Reported wall GOPS | 36.621 | 0.000 (timer frozen) — unusable |

Because aiehlc's XTime read 0, its only usable timing is the **A78 PMU cycle** budget,
and that budget is **dominated by kernel load**, not the GEMM:

```
aiehlc single-256³ launch = 18,856,784 A78 cycles
  kload    16,782,126  (89.0%)   <- per-launch XAie_LoadElfMem onto 16 tiles
  wait_io   1,114,109  ( 5.9%)   <- the actual array output-DMA drain (the "GEMM")
  lockinit    558,341  ( 3.0%)
  bdcfg       354,594  ( 1.9%)
  coreen+startio 47,614 (0.2%)
  poll (DDR->result)  89,529 cycles
```

So aiehlc's single-shot cost is a **framework kernel-reload cost** (89%); the streaming
GEMM itself (`wait_io`) is a 5.9% tail. In our ADF flow that reload happens **once** at
`gr.init()` (0.056 ms, outside the timed GEMM) and never again — the analog of aiehlc's
`wait_io` drain is our `gmio_wait` (0.601 ms). A raw ms-to-ms is therefore **not**
available from aiehlc's frozen-timer run; the honest comparison is (a) the launch model
above and (b) the **frequency-free core metrics** below.

## Frequency-free comparison (directly comparable — same counters, same work)

Both harnesses read the same AIE core perf counters on a probe compute tile, and vec
counts are whole-run. Per-tile MACs are identical (1,048,576), so these compare cleanly:

| Metric (probe tile, per-tile) | ours (int8-out) | aiehlc `simplematmul2_prof` | takeaway |
|---|---|---|---|
| Microkernel | `aie::mmul<4,16,8>` (matrix engine) | `aie::mul`+`reduce_add` dot-product | — |
| **Vector instrs** | **4 224** | 98 560 | ours issues **23× fewer** |
| **MACs / vector instr** | **248.2** | 10.6 | ours **23× denser** vectorization |
| Compute fraction | 91.2 % | 44.9 % | see caveat |
| Lock stall | 8.8 % | 55.1 % | see caveat |
| Stream stall | 0.0 % | 0.0 % | both never input-starved at the core |
| Output DMA volume | 16 kB (int8) | 16 kB (int8) | identical |

**Headline:** for the *identical* per-tile GEMM, our matrix-engine kernel retires the
work in **23× fewer vector instructions** (248 vs 10.6 MAC/instr) than aiehlc's
mul+reduce dot-product. That is the real kernel-efficiency difference between the two.

**Caveat on the compute/lock split:** aiehlc's own report notes its active/lock cycles
are a *sampled sub-window* (ratios valid, absolute cycles not), and our 8.8 % lock is a
**cold, uncontended single-panel** (our steady-state int8 batch runs ~35 % lock). So
read compute-vs-lock as "both are lock/feed-bound, neither is stream-starved," not as a
precise 91 vs 45. The vec-instruction/MACs-per-instr row has no such caveat — it is
whole-run and exact.

## To get a true ms-to-ms number

Run aiehlc's `simplematmul2_prof` on **crimini5** (where XTime is live, as our runs
show), so its `t0→t1` wall and `wall GFLOPS` populate. Then compare either:
- **whole single-shot** (ours 0.916 ms GEMM-only, or 1.576 ms incl. verify+profiling,
  vs aiehlc launch incl. its 89 % kernel-reload — not really equivalent), or
- **array drain only** (ours `gmio_wait` 0.601 ms vs aiehlc `wait_io`, both the
  PS-blocked-on-array-output-drain) — this is the fair one.

## Reproduce

```bash
cd example/example_oob_4x4
# config.h: NUM_PANELS=1 (256x256), GEMM_C_T=int8 (int8 out); graph.cpp: NUM_MATRICES=1
bash build.sh 5 -bootgen
export VEK385IP=crimini5 USERNAME=bkirinci
export VEK385_LOCAL_BIN=$PWD/build/vek385.BIN VEK385_LOCAL_ELF=$PWD/vek385.elf
export VEK385PDI=/home/$USERNAME/naiebaremetal/vek385.BIN
python3 ../../src/tool/test/runhw_vek385.py
```
