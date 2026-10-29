<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# ResNet18 — performance results

Designs:
- inst10: `aie/batch1_inst10/` (NUM_INST=10, BATCH_GRAPH), 1 batch/instance.
- single: `aie/` (NUM_INST=1), 1 instance.

For inst10 the same AIE graph/Work is shared by both host runtimes; only the
host ELF differs.

> **Measurement methodology (updated 2026-06-24).** Both runtimes now convert
> cycles→µs with a single pinned rate (`RESNET_TS_HZ = 100 MHz`) and also report
> raw cycle counts, which are rate-independent. This removed a ~1%
> apples-to-oranges skew: the old µs math did *integer* `COUNTS_PER_SECOND/1000000`,
> so baremetal's BSP value `99,999,908` truncated to **99** while Zephyr's
> hardcoded `100,000,000` gave **100**. The `b` summary now also reports per-round
> **mean/std/min/max** (the noise band) and a **phase split** (`gr.run()` vs the
> serial `executeRuntimeControl_TXN()`). Raw cycles are the source of truth.

# inst10 (unified timer, `-Os`, measured 2026-06-24)

## Zephyr (A72 @ EL3) — `./build_zephyr.sh -inst10`, benchmark `b` (100 × 10 = 1000)
| metric | value |
|---|---|
| wall-clock total | 1,946,829 us (194,682,859 cyc) |
| per round mean | 19,468.3 us (1,946,826 cyc) |
| per round std | 5.1 us (0.03% of mean) |
| per round min/max | 19,455.8 / 19,480.5 us |
| per image (avg) | 1,946.8 us |
| throughput | 513.7 img/s |
| compute (INT8) | 1869.71 GOPS |
| phase gr.run() | 0.0 us/round (0%) |
| phase ctrl_TXN() | 19,234.8 us/round (100%) |
| verification (inst 0) | idx 282 (tiger cat) |

## Baremetal (A72, lwIP) — `./buildbatch110.sh -bootgen` (host `-Os`), benchmark `b` (100 × 10 = 1000)
| metric | value |
|---|---|
| wall-clock total | 1,944,601 us (194,460,114 cyc) |
| per round mean | 19,445.8 us (1,944,577 cyc) |
| per round std | 4.4 us (0.02% of mean) |
| per round min/max | 19,437.5 / 19,458.2 us |
| per image (avg) | 1,944.6 us |
| throughput | 514.2 img/s |
| compute (INT8) | 1871.85 GOPS |
| phase gr.run() | 0.0 us/round (0%) |
| phase ctrl_TXN() | 19,420.4 us/round (100%) |
| verification (inst 0) | idx 282 (tiger cat) |

## Zephyr vs baremetal (inst10, unified timer)

| metric | Zephyr | Baremetal | delta |
|---|---|---|---|
| per round mean | 19,468.3 us | 19,445.8 us | **baremetal 22.5 us (0.12%) faster** |
| raw cycles/round | 1,946,826 | 1,944,577 | baremetal 2,249 cyc fewer |
| per round std | 5.1 us | 4.4 us | — |
| per image (avg) | 1,946.8 us | 1,944.6 us | baremetal 2.2 us faster |
| throughput | 513.7 img/s | 514.2 img/s | baremetal +0.5 img/s |

**This reverses the earlier "Zephyr 0.75% faster" result, which was entirely the
timer-truncation artifact.** With the unified rate, baremetal is marginally but
*genuinely* faster: the 22.5 us/round gap is ~45× the standard error of the mean
(std/√100 ≈ 0.5 us), i.e. real — and it matches the expectation that baremetal
carries less overhead.

The difference is tiny (0.12%) because the round is **100% serial A72
runtime-control** (`ctrl_TXN`; `gr.run()` is ~0). The breakdown is, however,
counterintuitive:

| phase | Zephyr | Baremetal |
|---|---|---|
| ctrl_TXN() | 19,234.8 us | 19,420.4 us |
| other host work (round − phase) | ~233 us | ~25 us |

Zephyr's control execution is actually ~185 us *lower*, but Zephyr pays ~208 us
more in "other" per-round host work — almost all of it the **rodata→buffer copy
of the input** (`ifm_buf[0][i] = ifm_data[i]`, ~150 KB) that only the Zephyr path
does each round (baremetal feeds `ifm_data` directly). These nearly cancel,
leaving baremetal ahead by a hair. In real (network) use both runtimes copy the
received image into a buffer per round, so that copy isn't Zephyr-specific
overhead; for the local `b` benchmark it is. Hoisting it out of the per-round
path (the benchmark reuses the same image) would make Zephyr's `b` ~200 us/round
faster — a benchmark-only optimization.

# single instance (unified timer, `-Os`, measured 2026-06-24)

## Zephyr (A72 @ EL3) — `./build_zephyr.sh`, benchmark `b` (100 × 1 = 100)
| metric | value |
|---|---|
| wall-clock total | 285,404 us (28,540,361 cyc) |
| per round mean | 2,854.0 us (285,398 cyc) |
| per round std | 4.2 us (0.15% of mean) |
| per round min/max | 2,850.6 / 2,893.8 us |
| per image (avg) | 2,854.0 us |
| throughput | 350.4 img/s |
| compute (INT8) | 1275.39 GOPS |
| phase gr.run() | 0.0 us/round (0%) |
| phase ctrl_TXN() | 2,695.4 us/round (100%) |
| verification (inst 0) | idx 282 (tiger cat) |

## Baremetal (A72, lwIP) — `./build.sh -bootgen` (host `-Os`), benchmark `b` (100 × 1 = 100)
| metric | value |
|---|---|
| wall-clock total | 271,994 us (27,199,379 cyc) |
| per round mean | 2,719.7 us (271,969 cyc) |
| per round std | 2.6 us (0.10% of mean) |
| per round min/max | 2,716.5 / 2,742.9 us |
| per image (avg) | 2,719.9 us |
| throughput | 367.7 img/s |
| compute (INT8) | 1338.27 GOPS |
| phase gr.run() | 0.0 us/round (0%) |
| phase ctrl_TXN() | 2,711.5 us/round (100%) |
| verification (inst 0) | idx 282 (tiger cat) |

## Zephyr vs baremetal (single, unified timer)

| metric | Zephyr | Baremetal | delta |
|---|---|---|---|
| per round mean | 2,854.0 us | 2,719.7 us | **baremetal 134.3 us (4.9%) faster** |
| raw cycles/round | 285,398 | 271,969 | baremetal 13,429 cyc fewer |
| per round std | 4.2 us | 2.6 us | — |
| per image (avg) | 2,854.0 us | 2,719.9 us | baremetal 134.1 us faster |
| throughput | 350.4 img/s | 367.7 img/s | baremetal +17.3 img/s |
| compute (INT8) | 1275.39 GOPS | 1338.27 GOPS | +62.9 GOPS |

The corrected gap (4.9%) is **wider** than the stale pre-fix "3.9%" — as expected,
since the old math divided baremetal by 99 instead of 100, flattering Zephyr. The
134 us gap is ~320× the standard error of the mean (std/√100 ≈ 0.3–0.4 us), i.e.
unambiguously real.

The driver is the same per-round **rodata→buffer copy** that only the Zephyr path
does (`ifm_buf[0][i] = ifm_data[i]`): Zephyr's `ctrl_TXN` is ~16 us *lower* but it
pays ~159 us in "other" host work (round − phase) vs baremetal's ~8 us. That copy
is a roughly fixed ~150 us/round cost, so it hurts most when the round is short:
it is **4.9%** of the 2.85 ms single round but only **0.12%** of the 19.5 ms
inst10 round — which is exactly why baremetal's lead shrinks from 4.9% (N=1) to
0.12% (N=10). It is a local-`b` artifact; in real network use both runtimes copy
the received image per round.

> Note on the phase split: `ctrl_TXN()` reports ~100% at both N=1 and N=10 because
> the AIE compute kick + wait-for-done happens *inside* `executeRuntimeControl_TXN()`,
> not in `gr.run()` (which only sets the iteration count). So the phase split is a
> host-control vs. host-copy view, **not** a pure host-vs-AIE split. The AIE-compute
> vs. host decomposition comes from the cross-N fit below, not from this split.

# single vs inst10 (unified timer, benchmark)

| metric | single | inst10 | inst10 gain |
|---|---|---|---|
| per image (Zephyr) | 2,854.0 us | 1,946.8 us | 1.47x faster |
| per image (baremetal) | 2,719.9 us | 1,944.6 us | 1.40x faster |
| throughput (baremetal) | 367.7 img/s | 514.2 img/s | 1.40x |
| compute (baremetal) | 1338.27 GOPS | 1871.85 GOPS | 1.40x |

inst10 raises per-image throughput ~1.47x (Zephyr; ~1.40x baremetal), not 10x,
because the round is bound by serial A72 runtime-control, not AIE compute.
Fitting round(N) = A + N*h to the unified baremetal data (single 2,719.7 us,
inst10 19,445.8 us) gives A ~860 us (parallel AIE compute), h ~1,860 us (serial
per-instance host work): at N=10 the round is ~96% host orchestration. Per-image
asymptote is h (~538 img/s); N=10 already hits 514 img/s, so more instances buy
almost nothing without moving runtime-control off the A72.
