<!--- Copyright (C) 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# Debugging baremetal AIE-ML runtime-control hangs

A practical playbook for diagnosing hangs in the on-board transaction
interpreter (`XAie_SubmitSerializedTransaction` in `sw/transactionsubmit.h`),
written from the experience of root-causing the multi-instance ResNet-18
deadlock at `NUM_INST >= 2`.

If you see the board print `process_data ---2` and then go silent, this is
the doc for you.

---

## 1. When to use this doc

Symptoms that fit:
- Board prints `executeRuntimeControl` and then halts silently.
- UART logging stops mid-transaction with no error.
- Single-instance build (`source ./build.sh -bootgen`) works, multi-instance
  build (`source ./buildbatch110.sh -bootgen`) hangs.
- Last UART line looks like `MP: 0xXXXXXXXXXXX, 0xYYYYYY, 0x0` followed by
  nothing.

If your hang is in the host-side network stack, IFM parser, or before
`executeRuntimeControl`, this doc isn't the right tool — start by adding
`printf`s in `sw/resnet18_top.cpp` / `sw/process_data.cpp`.

---

## 2. Step 1 — Add diagnostic instrumentation to `transactionsubmit.h`

The stock interpreter spins forever on `MaskPoll` (the "wait for HW DMA to
finish this BD" op). To diagnose, you need three things:

1. **MaskPoll bail-out** — count the poll iterations and dump state instead
   of spinning forever.
2. **Per-op logging** — print `BlockWrite` (the op that configures DMA BDs)
   so you can see which tiles/channels were configured before the hang.
3. **Tile + DMA status dump helpers** — sweep the array and read
   shim/mem-tile DMA registers on demand.

A reference diagnostic patch (~103 lines added to `transactionsubmit.h`)
is preserved at `logs/transactionsubmit.diagnostic.h.bak` in the
debugging workspace; ask the author or apply equivalent instrumentation.
Key compile-time flags it introduces:

| Flag | Default | Effect |
|---|---|---|
| `MP_BAIL_OUT` | 1 (diag) / 0 (prod) | After `MP_TIMEOUT` polls, print state and skip the MP. =0 keeps the original spin-forever behavior. |
| `MP_TIMEOUT` | 100000 | Number of register-read iterations before bail-out (~tens of ms wall). |
| Always-on per-op `BW:` print | — | One line per BlockWrite: `BW: op=N c=X r=Y off=0xZZZ sz=S w0=0xVVVV` |
| `aie_tile_check()` | callable | Sweeps all 38 columns × rows 2-5, prints non-zero core-status registers. |
| `aie_dump_shim_dma(col)` | callable | Reads `0x1d200/208/220/228` (S2MM ch0/1, MM2S ch0/1 status). |
| `aie_dump_memtile_dma(col)` | callable | Reads 6×S2MM + 6×MM2S status + queue_ctl per col. |

**Caveat:** the BW logging at 115200 baud adds ~270 KB of UART traffic per
inference (each BW line is ~50 chars × ~40k ops). Wall-clock measurements
become useless. Turn it off for any timing comparison.

---

## 3. Step 2 — Decode the hang address

When `MP_BAIL_OUT=1` triggers, you'll see:

```
!!! MP TIMEOUT #1: op 16/39940  addr=0x2001e01d228 mask=0x78003c expect=0x0 got=0x80012
```

Decode `addr` using the AIE-ML tile address layout:

```
0x20000000000               base of the AIE NoC window
            + (col << 25)   target column        (bits 30:25)
            + (row << 20)   target row           (bits 24:20)
            + register_off  per-tile register    (bits 19:0)
```

So `0x2001e01d228` = `col 0x0F` (= **15**), `row 0` (= **shim**),
`register 0x1d228` (= **MM2S channel 1 status**).

Other common offsets on the shim tile (row 0):
| Offset | Register |
|---|---|
| `0x1d200` | S2MM ch0 status |
| `0x1d208` | S2MM ch1 status |
| `0x1d220` | MM2S ch0 status |
| `0x1d228` | MM2S ch1 status |
| `0x1d214` | S2MM ch0 task queue |
| `0x1d21c` | S2MM ch1 task queue |
| `0x1d234` | MM2S ch0 task queue |
| `0x1d23c` | MM2S ch1 task queue |

On the mem-tile (row 1) the DMA register block sits at offset `0xA0000`:
| Offset | Register |
|---|---|
| `0xA0660 + ch*4` | S2MM ch0..5 status |
| `0xA0678 + ch*4` | MM2S ch0..5 status |
| `0xA0604 + ch*8` | S2MM ch0..5 queue_ctl |
| `0xA0634 + ch*8` | MM2S ch0..5 queue_ctl |

The `mask=0x78003c` the transaction MaskPolls against, on the DMA channel
status register, decodes as:

| Bit | Meaning |
|---|---|
| 0x000004 | IDLE (channel has no work) |
| 0x000008 | START_PENDING (start bit was set but HW hasn't picked it up) |
| 0x000010 | TASK_QUEUE_OVERFLOW |
| 0x000020 | (reserved) |
| 0x080000-0x300000 | TASK_QUEUE_SIZE bits (depth used / available) |
| 0x400000 | TASK_QUEUE_FULL |

`expect=0x0` means "all these bits must be 0" — i.e., the channel must be
fully idle with empty queue. The transaction is waiting for the BD work
to drain. `got=0x80012` for example means TASK_QUEUE_SIZE != 0
(work still queued) and START_PENDING is set.

---

## 4. Step 3 — Find the op# in the generated runtime control

The transaction binary `ml_txn.bin` is generated from
`Work/ps/c_rts/aie_runtime_control.cpp` by running it (compiled as
`genTxnMain.out`) which records every BD-config / enqueue / wait call
into the binary stream. The op-index from the MaskPoll bail-out maps
deterministically back to a source line in that .cpp.

The aiecompiler emits the .cpp as a wrapper
`executeRuntimeControlInternal()` that calls sub-functions
`executeRuntimeControlInternal0`, `Internal1`, etc. (partitioned for
parallel codegen). Each call generates a contiguous range of ops.

Recipe to map op-index → source line:

```bash
# Open the generated wrapper
$EDITOR aie/batch1_inst10/Work/ps/c_rts/aie_runtime_control.cpp

# The .cpp has __LOG__ statements for almost every BD-config and enqueue.
# Count them in order: each `enqueueTask`, `configure*BD`, `wait*DMAChannelDone`
# is one txn op (roughly). For exact mapping, instrument the patcher
# (`sw/mladfpatch/patchddraddr.py`) to print "op N: <source-line>" as it
# rewrites — or just bisect manually using the BW: log lines, which DO
# include op#.
```

The fastest way in practice: every `BW: op=X c=Y r=Z` line lets you
correlate op-X with "configure BD at col Y, row Z". Walk backwards from
the hang to find the most recent BW that targets the same (col, row) as
the failing MP — that's the BD being polled.

---

## 5. Step 4 — The N=1 vs N>=2 generated-code diff trick

If a multi-instance build hangs and the single-instance build of the
same source works, **diff the two generated runtime controls**:

```bash
# Build single-inst
source ./build.sh -bootgen
cp aie/Work/ps/c_rts/aie_runtime_control.cpp /tmp/runtime.N1.cpp

# Build multi-inst (different aie/ dir, different macro)
rm -rf aie/batch1_inst10/Work
source ./buildbatch110.sh -bootgen
cp aie/batch1_inst10/Work/ps/c_rts/aie_runtime_control.cpp /tmp/runtime.N2.cpp

diff /tmp/runtime.N1.cpp /tmp/runtime.N2.cpp | less
```

Look for:
- Calls to sub-functions in different order between N=1 and N=2
- Lines that are interleaved-per-layer in N=1 but batched-per-instance in N=2
- Receiver-config and source-kick that appear adjacent in N=1 but far apart in N=2

Bookkeeping artifacts of this comparison for the ResNet-18 case live in
`example/example_ai_model/aeg/res18sw/logs/control_diff/` (in the
debugging workspace) — see `DIFF_SUMMARY.md` there for an annotated
walkthrough with line references.

---

## 6. Step 5 — Iterate fast via `patchddraddr.py`

A full AIE recompile takes 10-30 minutes per variant. To test ordering
hypotheses faster, edit `sw/mladfpatch/patchddraddr.py` — it runs as a
post-pass on the freshly-generated `aie_runtime_control.cpp` and
rewrites it before `genTxnMain.out` serializes the txn binary. So:

- Edit the patcher → re-run `source ./buildbatch110.sh -bootgen`
- aiecompile is skipped (cached `Work/` dir reused)
- ~30 seconds vs ~30 minutes
- Same on-board test cycle

The patcher already has a `PER_LAYER_INTERLEAVE = True` toggle (line ~170)
for the multi-instance fix; flipping to `False` falls back to the original
broken order — useful for A/B testing whether your suspect change actually
matters.

---

## 7. Case study: the multi-instance ordering deadlock

Concrete walkthrough of how the methodology above led to the
`NUM_INST >= 2` fix:

1. **Symptom:** `buildbatch110.sh` build hangs on `MP: 0x2001e01d228,
   0x78003c, 0x0`. (Section 2 instrumentation added; saw bail-out at op 16.)
2. **Decoded:** col 15, shim row, MM2S ch1 status; transaction is waiting
   for a shim source DMA to drain.
3. **Backwards-walked the BW log:** found `BW: op=4 c=15 r=0 off=0x1d000`
   which configured the BD at col 15, row 0 (shim) — that's the source
   of the broadcast.
4. **Looked at the multicast targets:** the broadcast goes to mem-tile
   receivers on multiple columns (inst-0 + inst-1 + ...). Their BD-config
   BWs appear in the log MUCH later — op 8000+. Hypothesis: shim source
   was kicked before receivers were armed, multicast back-pressure
   hangs the source.
5. **Confirmed via N=1 vs N=2 diff (Section 5):** N=1 interleaves source
   kick and receiver setup per layer. N=2 partitions them into separate
   sub-functions and serializes them — all sources first, then all
   receivers. That matches the hypothesis.
6. **Patched (Section 6):** `patchddraddr.py` `PER_LAYER_INTERLEAVE`
   pass rewrites the wrapper into a 6-phase per-layer interleave —
   source KICK → receivers → compute → source WAIT. Took 5 iterations
   of A/B testing the patcher (~30 seconds each) to get the ordering
   right; would have been ~25 hours of aiecompile time without the
   shortcut.
7. **Verified:** zero MaskPoll bail-outs, both instances classify
   correctly. See `logs/control_diff/DIFF_SUMMARY.md` for the
   iteration history.

The whole diagnosis-to-validated-fix cycle was ~2 days of debug time;
without instrumentation + the N=1 diff trick + the patcher iteration
loop it would have been weeks.

---

## 8. Gotchas

- **Don't trust the demo timing while instrumentation is on.** The 115200
  baud UART is ~3 orders of magnitude slower than actual AIE compute;
  every per-op log line costs ~700 µs. The 18.6 s / 82.5 s
  "per-inference" numbers you'll see in the diagnostic build's UART are
  almost entirely UART time. For real timing, disable
  `DETAILED_BLOCKWRITE_PROFILE` and `MP_BAIL_OUT`.
- **`MP_BAIL_OUT=1` doesn't fix anything — it just keeps the log scrolling.**
  After the first MP times out, every subsequent MP that depends on the
  bailed-out one will also time out. Expect cascading failure. The
  classification output is meaningless in bail-out mode.
- **Lock-fence-mode is a red herring for multi-instance hangs.** Setting
  `--lock-fence-mode=0` in the AIE compile relaxes BD locking; this can
  *appear* to fix multi-instance hangs (transaction completes, no MPs
  time out), but the broadcast multicast still races and the
  classification will be silently wrong. The real fix is op-ordering,
  not lock relaxation.
- **The shim DMA task queue depth is 4.** Enqueuing more than 4 source
  BDs in a row, without a `waitDMAChannelDone` between, will silently
  overflow. The queue-full status bit gets set but the failure mode
  looks identical to a multicast back-pressure hang.
- **BD IDs are reused across layers.** On the shim source channels of
  this ResNet design only BD IDs 0 and 1 are used (alternating). So
  layer L's source BD MUST drain before layer L+1's BD-config writes,
  or you'll corrupt an in-flight transfer. This is why the patcher's
  per-layer `shared.WAIT[L]` step matters.

---

## 9. Reference material

- `sw/transactionsubmit.h` — the on-board transaction interpreter; where
  the diagnostic instrumentation patch goes.
- `sw/mladfpatch/patchddraddr.py` — the runtime-control post-pass.
  Comments around `PER_LAYER_INTERLEAVE` explain the multi-instance
  fix in detail.
- `aie/batch1_inst10/Work/ps/c_rts/aie_runtime_control.cpp` — the
  generated wrapper. Read it before patching it.
- Vitis 2024.x AIE-ML hardware specification — definitive bit-level
  decoding of the DMA status registers. The MaskPoll mask values
  (e.g. `0x78003c`) come from here.
- AI Engine Run-Time Library (`aie-rt`) source — for the BD config and
  enqueue helpers, when you need to know what `configureBD` and
  `enqueueTask` actually emit at the register level.
