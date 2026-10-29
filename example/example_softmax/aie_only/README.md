<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# AIE-Only Softmax

Softmax computed entirely on a single AIE core, for direct comparison against
the PL-kernel implementation in `../pl_only/`. Both examples use the exact
same math (`exp(x) / sum(exp(x))` per 32-element tile, no max-subtraction)
over the same input data, so their outputs — and, informally, their
performance — are directly comparable.

## Build

### Simulation (aiesimulator)

```bash
$ source ../../../script/settings.sh 1
$ make compile AIE_GEN=2
$ make sim
```

`AIE_GEN`: 1 = vck190 (`xcvc1902`), 2 = vek280 (`xcvc2802`), 5 = vek385
(`xcve3858`).

### Hardware (BOOT.BIN)

```bash
# From this directory:
$ source ./build.sh <AIE_GEN> -bootgen "" -custom_pdi
```

`-custom_pdi` links against the real production platform
(`xilinx_vek280_base_202520_1.xpfm` for `AIE_GEN=2`) instead of the raw
silicon part, so the resulting PDI's IDCODE matches **production (S-prod)
silicon**. This runs the full Vivado synth+impl+bitgen pipeline
(~20+ minutes) instead of a plain `aiecompiler`/`v++ --part=...` build.

* **Use `-custom_pdi` for S-prod boards** (e.g. `vek280-11`). Without it, the
  PDI is generic/unlocked and fails to program with
  `PLM Error Major 0x326 Minor 0x14` (IDCODE mismatch).
* **Omit `-custom_pdi` for non-S-prod boards** (e.g. `vek280-5`):
  `source ./build.sh <AIE_GEN> -bootgen`. This skips the Vivado platform
  build and links straight against `--part=$DEVICE_FILE`, so it's much
  faster, but the resulting PDI will not boot on S-prod silicon.

Decode a board's silicon revision via `xsdb`
(`mrd 0xF11A0000 1`; revision-0 IDCODE = ES1, revision-1+ = production) if
you're unsure which build to use for a given board.

## Expected output

On success, prints a per-tile error breakdown followed by:

```
Max abs error vs CPU reference: <~1e-9, FP roundoff>
Test Passed
```

## Correctness check

`src/graph.cpp` computes a CPU-side reference (`exp(x)/sum(exp(x))` per
tile) after `gr.wait()`/`gr.end()` return, compares it against the AIE
output, and returns a real exit code — kept strictly post-timing so it
can't affect the `aie_only` vs `pl_only` performance comparison. See
`../pl_only/README.md` for the same pattern on the PL side.
