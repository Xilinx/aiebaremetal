// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <fstream>
#include <iomanip>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <xaiengine.h>
#include "aiebaremetal.h"

#if defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf;
#endif

#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
BaremetalGraphSim gr("gradf");
#else
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xiltimer.h"
#if (__AIE_ARCH__ != ARCH_AIE2PS)
#endif
BaremetalGraph gr("gradf");

// NOTE: no AIE clock constant. All cycle-counter metrics (Layer 2/3) are reported
// in frequency-free units (cycles, bytes/cycle, FLOP/cycle, cycle-fractions) so a
// guessed core clock can never corrupt them. The only absolute throughput numbers
// are the Layer 1 wall-clock GFLOPS, measured with the real PS timer (XTime).
#endif
#include "kernels/config.h"
#if defined(__ADF_FRONTEND__)
void adf_run() {
    std::cout << " adf_run alloc " << NUM_HW_ROWS * MAT_A_CHUNK_SIZE / 1024 << " kB" << std::endl;
    // INT8 A/B inputs, INT32 C output.
    std::vector<GEMM_AB_T *> inputArray(NUM_HW_ROWS, NULL), inputArrayB(NUM_HW_ROWS, NULL);
    std::vector<int32_t *> outputArray(NUM_HW_ROWS, NULL);
    GEMM_AB_T *mem_in = (GEMM_AB_T *)GMIO::malloc(NUM_HW_ROWS * MAT_A_CHUNK_BYTES_IN);
    GEMM_AB_T *mem_in_b = (GEMM_AB_T *)GMIO::malloc(NUM_ELMNTS * sizeof(GEMM_AB_T));

    int32_t *mem_out = (int32_t *)GMIO::malloc(NUM_HW_ROWS * MAT_A_CHUNK_SIZE);
    std::cout << "after alloc size = " << NUM_HW_ROWS * MAT_A_CHUNK_SIZE / 1024 << "kb" << std::endl;
    for (int i = 0; i < NUM_HW_ROWS; i++) {
        std::cout << " 1 i = " << i << std::endl;
        inputArray[i] = mem_in + i * MAT_A_CHUNK;
        inputArrayB[i] = mem_in_b + i * MAT_A_CHUNK;
        outputArray[i] = mem_out + i * MAT_A_CHUNK;
        for (int j = 0; j < MAT_A_CHUNK; j++) {
            inputArray[i][j] = i + 1;
            inputArrayB[i][j] = i + 2;
            outputArray[i][j] = i + 1;
        }
    }

    std::cout << "before run" << std::endl;
    gradf.run(1);
    std::cout << "after run" << std::endl;
    int rown = 1;
    for (int i = 0; i < rown; i++) {
        gradf.in[i].gm2aie_nb(inputArray[i], MAT_A_CHUNK_BYTES_IN);
        gradf.in[i].gm2aie_nb(inputArrayB[i], NUM_ELMNTS * sizeof(GEMM_AB_T));
    }

    for (int i = 0; i < rown; i++) {
        gradf.out[i].aie2gm_nb(outputArray[i], MAT_A_CHUNK_SIZE);
        gradf.out[i].wait(); // assuming data from gm1 are processed by the graph and output to gm2
    }

    std::cout << "before end" << std::endl;
    gradf.end();
    for (auto x : inputArray) {
        GMIO::free(x);
    }
    for (auto x : inputArrayB) {
        GMIO::free(x);
    }
    for (auto x : outputArray) {
        GMIO::free(x);
    }
}
#endif
int main(int argc, char **argv) {
#if defined(__ADF_FRONTEND__)
    std::cout << "gradf.a  =" << gradf.a << " before init" << std::endl;
    gradf.init();
    std::cout << "after init" << gradf.a << std::endl;
#else
    // FIXME, should handle the sync in runtime api
#endif
    const int rown = NUM_HW_ROWS;
    const int NP = NUM_PANELS;
    const int P = PANEL_SIZE;
    const int M = MATRIX_SIZE;

    // A single full GEMM, distributed across the 4x4 tile array (no benchmark batch).
    static constexpr int NUM_MATRICES = 1;

    // INT8 GEMM: A/B are int8; C streams out as int8 (int32 accum saturated to
    // int8 per tile). Host all_C/cAcc keep int32 storage but hold int8-range values.
    std::vector<GEMM_AB_T> Afull((size_t)M * M);
    std::vector<GEMM_AB_T> Bfull((size_t)M * M, 1);

    // all_C[matrix_idx] stores every output panel's accumulated result for spot verification.
    // Layout: [(m_panel*NP+n_panel) * NUM_HW_ROWS*MAT_A_CHUNK + chain_r*MAT_A_CHUNK + elem]
    std::vector<std::vector<int32_t>> all_C(NUM_MATRICES,
        std::vector<int32_t>(NP * NP * NUM_HW_ROWS * MAT_A_CHUNK, 0));

    // --- EXPERIMENT: pre-pack ALL inputs into device (LPDDR) buffers ONCE, then
    // DMA directly from panel offsets in the hot loop (no per-step CPU repack). ---
    // dev_A holds every (matrix, m_panel, k_panel, chain-row) A chunk in the exact
    // dense layout the kernel consumes. dev_B holds every (k_panel, n_panel) B panel
    // pre-transposed; B is identical across matrices so it is packed once.
    const size_t devA_elems = (size_t)NUM_MATRICES * NP * NP * NUM_HW_ROWS * MAT_A_CHUNK;
    const size_t devB_elems = (size_t)NP * NP * NUM_ELMNTS;
    GEMM_AB_T *dev_A   = (GEMM_AB_T *)gr.malloc(devA_elems * sizeof(GEMM_AB_T));
    GEMM_AB_T *dev_B   = (GEMM_AB_T *)gr.malloc(devB_elems * sizeof(GEMM_AB_T));
    // C now streams out as int8 (each tile saturates its full-K int32 result to
    // int8). MAT_A_CHUNK_SIZE (int32-sized) over-allocates harmlessly.
    GEMM_C_T *mem_out = (GEMM_C_T *)gr.malloc(NUM_HW_ROWS * MAT_A_CHUNK_SIZE);
    std::vector<GEMM_C_T *> outC(NUM_HW_ROWS);
    for (int r = 0; r < NUM_HW_ROWS; r++)
        outC[r] = mem_out + r * MAT_A_CHUNK;

    // Offsets (in elements) into the packed device buffers.
    auto devA_off = [&](int mi, int mp, int kp, int r) -> size_t {
        return ((((size_t)mi * NP + mp) * NP + kp) * NUM_HW_ROWS + r) * MAT_A_CHUNK;
    };
    auto devB_off = [&](int kp, int np_) -> size_t {
        return ((size_t)kp * NP + np_) * NUM_ELMNTS;
    };

    std::vector<int32_t> cAcc(NUM_HW_ROWS * MAT_A_CHUNK);

    // --- Layer 1: capture init start time ---
#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
    XTime t_init_start;
    XTime_GetTime(&t_init_start);
#endif

    gr.init();

    // --- Profiling setup (hardware only): runs once after gr.init() ---
#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
    XTime t_init_end;
    XTime_GetTime(&t_init_end);
    double init_ms = 1000.0 * static_cast<double>(t_init_end - t_init_start) / COUNTS_PER_SECOND;

    // Layer 3: configure AIE core perf counters on the first compute tile (col=0, ADF row 0).
    // Read AieTileRowStart from the device instance so this works across all AIE generations:
    //   AIE gen 1   → AieTileRowStart = 1 (shim=0)
    //   AIE-ML/gen5 → AieTileRowStart = 2 (shim=0, mem-tile=1)
    // Counter 0: active cycles per invocation (ACTIVE_CORE → DISABLED_CORE).
    // Counter 1: vector instruction count (self-counting INSTR_VECTOR_CORE events).
    // Counter 2: stream stall cycles — cycles the core waits for window/cascade data
    //            (STREAM_STALL_CORE start, ACTIVE_CORE stop).
    // Counter 3: lock stall cycles — cycles the core waits to acquire a buffer lock
    //            (LOCK_STALL_CORE start, ACTIVE_CORE stop).
    XAie_DevInst *devInst = static_cast<XAie_DevInst *>(gr.getaiedevinst());
    XAie_LocType coreTile = XAie_TileLoc(0U, devInst->AieTileRowStart);
    XAie_PerfCounterControlSet(devInst, coreTile, XAIE_CORE_MOD, 0U,
                               XAIE_EVENT_ACTIVE_CORE, XAIE_EVENT_DISABLED_CORE);
    XAie_PerfCounterControlSet(devInst, coreTile, XAIE_CORE_MOD, 1U,
                               XAIE_EVENT_INSTR_VECTOR_CORE, XAIE_EVENT_INSTR_VECTOR_CORE);
    XAie_PerfCounterControlSet(devInst, coreTile, XAIE_CORE_MOD, 2U,
                               XAIE_EVENT_STREAM_STALL_CORE, XAIE_EVENT_ACTIVE_CORE);
    XAie_PerfCounterControlSet(devInst, coreTile, XAIE_CORE_MOD, 3U,
                               XAIE_EVENT_LOCK_STALL_CORE, XAIE_EVENT_ACTIVE_CORE);

    // Storage for the probe iteration (first matrix, m_panel=n_panel=k_panel=0).
    int64_t  probe_in_cycles      = 0;
    int64_t  probe_out_cycles     = 0;
    uint32_t probe_active_cycles  = 0U;
    uint32_t probe_vec_instrs     = 0U;
    uint32_t probe_stream_stall   = 0U;
    uint32_t probe_lock_stall     = 0U;

    // Per panel-multiply wall-clock statistics (one A-panel x B-panel on the array).
    double total_panel_mul_ms = 0.0;
    double min_panel_mul_ms   = 1e18;
    double max_panel_mul_ms   = 0.0;
    // Wall time of the *probe* panel-multiply specifically. The perf counters above
    // are captured on the first panel-multiply only, so all "% of wall time" figures
    // in Layer 3 must compare against THIS step's wall time, not the average across
    // all 64 steps (the first step is slower due to graph warm-up).
    double probe_panel_mul_ms = 0.0;
    // Per-invocation phase breakdown (probe step only): where the per-gr.run()
    // wall time actually goes — graph launch vs input-DMA issue vs output-DMA
    // issue vs blocking gmio_wait. Locates the fixed overhead floor.
    double probe_phase_run_ms  = 0.0;
    double probe_phase_in_ms   = 0.0;
    double probe_phase_out_ms  = 0.0;
    double probe_phase_wait_ms = 0.0;
    // Host post-processing time, summed over ALL panel steps. Split so we can see
    // how much is real GEMM work (k-accumulation reading the device output buffer)
    // vs pure verification bookkeeping (all_C store).
    double host_acc_ms   = 0.0;  // cAcc = / += mem_out  (reads device memory from PS)
    double host_store_ms = 0.0;  // all_C store          (verification only)
    // Profiling instrumentation overhead that lands inside the timed region:
    // arming the GMIO/core perf counters before the launch, and reading /
    // stopping / resetting them on the probe iteration. This is measurement
    // scaffolding, not GEMM work, so it is excluded from the reported timing.
    double prof_overhead_ms = 0.0;
#endif

    // --- One-time input packing into device buffers (EXCLUDED from GEMM timing). ---
#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
    XTime t_prep_start;
    XTime_GetTime(&t_prep_start);
#endif
    for (int matrix_idx = 0; matrix_idx < NUM_MATRICES; matrix_idx++) {
        // Each matrix has a distinct A pattern so results differ across iterations.
        // B = all-ones is constant; expected output: C[row][*] = M * Aval(row).
        auto Aval = [matrix_idx](int row) -> GEMM_AB_T {
            return (GEMM_AB_T)((row + matrix_idx) % 7 + 1);
        };
        for (int r = 0; r < M; r++)
            for (int c = 0; c < M; c++)
                Afull[(size_t)r * M + c] = Aval(r);

        for (int m_panel = 0; m_panel < NP; m_panel++)
            for (int k_panel = 0; k_panel < NP; k_panel++)
                for (int r = 0; r < NUM_HW_ROWS; r++) {
                    GEMM_AB_T *dst = dev_A + devA_off(matrix_idx, m_panel, k_panel, r);
                    for (int lr = 0; lr < NUM_ROWS_PER_HW_ROW; lr++) {
                        int grow = m_panel * P + r * NUM_ROWS_PER_HW_ROW + lr;
                        for (int c = 0; c < P; c++) {
                            int gcol = k_panel * P + c;
                            dst[lr * NUM_COLS + c] = Afull[(size_t)grow * M + gcol];
                        }
                    }
                }
    }
    // B is identical across matrices: pack each (k_panel, n_panel) panel transposed once.
    for (int k_panel = 0; k_panel < NP; k_panel++)
        for (int n_panel = 0; n_panel < NP; n_panel++) {
            GEMM_AB_T *dst = dev_B + devB_off(k_panel, n_panel);
            for (int col = 0; col < P; col++)
                for (int l = 0; l < P; l++)
                    dst[col * P + l] =
                        Bfull[(size_t)(k_panel * P + l) * M + (n_panel * P + col)];
        }

#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
    XTime t_prep_end;
    XTime_GetTime(&t_prep_end);
    double prep_ms = 1000.0 * static_cast<double>(t_prep_end - t_prep_start) / COUNTS_PER_SECOND;

    XTime t_gemm_start;
    XTime_GetTime(&t_gemm_start);
#endif

    bool test_passed = true;
    int panels_checked = 0;

    // Total graph iterations = one per panel-multiply. We launch the graph ONCE
    // for the whole workload and then stream a fresh A-stripe + B-panel per
    // iteration through GMIO. The ADF runtime advances the ping-pong window locks
    // across iterations by itself, so there is no per-panel re-arm (gr.run) and
    // no manual lock reset. In simulator mode StartPlios() fires on this one run.
    const int total_panel_mul_iters = NUM_MATRICES * NP * NP * NP;

#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
    // Arm GMIO stream profiling + core perf counters BEFORE the launch so the
    // first streamed iteration below is the profiling probe. This arming is
    // measurement scaffolding, so its cost is tracked and later subtracted out.
    XTime t_prof0; XTime_GetTime(&t_prof0);
    PROFILE_ID pid_in  = gr.start_profiling("gradf.in[0]",  io_total_stream_running_to_idle_cycles);
    PROFILE_ID pid_out = gr.start_profiling("gradf.out[0]", io_total_stream_running_to_idle_cycles);
    XAie_PerfCounterSet(devInst, coreTile, XAIE_CORE_MOD, 0U, 0U);
    XAie_PerfCounterSet(devInst, coreTile, XAIE_CORE_MOD, 1U, 0U);
    XAie_PerfCounterSet(devInst, coreTile, XAIE_CORE_MOD, 2U, 0U);
    XAie_PerfCounterSet(devInst, coreTile, XAIE_CORE_MOD, 3U, 0U);
    XTime t_prof1; XTime_GetTime(&t_prof1);
    prof_overhead_ms += 1000.0 * double(t_prof1 - t_prof0) / COUNTS_PER_SECOND;
    XTime t_run_launch0; XTime_GetTime(&t_run_launch0);
#endif
    gr.run(total_panel_mul_iters);
#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
    XTime t_run_launch1; XTime_GetTime(&t_run_launch1);
    double run_launch_ms = 1000.0 * double(t_run_launch1 - t_run_launch0) / COUNTS_PER_SECOND;
#endif

    // Blocked GEMM: C[m_panel,n_panel] += A[m_panel,k_panel] * B[k_panel,n_panel]
    // All inputs are already resident in dev_A / dev_B; the hot loop only issues DMA
    // and drains one output panel per graph iteration.
    for (int matrix_idx = 0; matrix_idx < NUM_MATRICES; matrix_idx++) {
        for (int m_panel = 0; m_panel < NP; m_panel++) {
            for (int n_panel = 0; n_panel < NP; n_panel++) {
                for (int k_panel = 0; k_panel < NP; k_panel++) {
#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
                    // Profiling counters were armed once before the launch; the very
                    // first streamed iteration is the probe.
                    const bool is_probe_iter = (matrix_idx == 0 && m_panel == 0 &&
                                                n_panel == 0 && k_panel == 0);
                    XTime t_panel_mul_start, t_after_in, t_after_out;
                    XTime_GetTime(&t_panel_mul_start);
#endif

                    std::vector<std::string> out_strs;
                    for (int r = 0; r < rown; r++) {
                        std::ostringstream in_ostr;
                        in_ostr << "gradf.in[" << r << "]";
                        gr.gm2aie_nb((char *)in_ostr.str().c_str(),
                                     reinterpret_cast<char *>(dev_A + devA_off(matrix_idx, m_panel, k_panel, r)),
                                     MAT_A_CHUNK_BYTES_IN);
                        gr.gm2aie_nb((char *)in_ostr.str().c_str(),
                                     reinterpret_cast<char *>(dev_B + devB_off(k_panel, n_panel)),
                                     NUM_ELMNTS * sizeof(GEMM_AB_T));
                    }
#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
                    if (is_probe_iter) XTime_GetTime(&t_after_in);
#endif
                    for (int r = 0; r < rown; r++) {
                        std::ostringstream out_ostr;
                        out_ostr << "gradf.out[" << r << "]";
                        out_strs.push_back(out_ostr.str());
                        gr.aie2gm_nb((char *)out_ostr.str().c_str(), reinterpret_cast<char *>(outC[r]), MAT_A_CHUNK_BYTES_OUT);
                    }
#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
                    if (is_probe_iter) XTime_GetTime(&t_after_out);
#endif
                    for (int r = 0; r < rown; r++)
                        gr.gmio_wait((char *)out_strs[r].c_str());

#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
                    XTime t_panel_mul_end;
                    XTime_GetTime(&t_panel_mul_end);
                    double panel_mul_ms = 1000.0 *
                        static_cast<double>(t_panel_mul_end - t_panel_mul_start) / COUNTS_PER_SECOND;
                    total_panel_mul_ms += panel_mul_ms;
                    if (panel_mul_ms < min_panel_mul_ms) min_panel_mul_ms = panel_mul_ms;
                    if (panel_mul_ms > max_panel_mul_ms) max_panel_mul_ms = panel_mul_ms;

                    if (is_probe_iter) {
                        probe_panel_mul_ms = panel_mul_ms;
                        probe_phase_run_ms  = 0.0;  // graph launch is one-time (reported separately), not per-panel
                        probe_phase_in_ms   = 1000.0 * double(t_after_in     - t_panel_mul_start) / COUNTS_PER_SECOND;
                        probe_phase_out_ms  = 1000.0 * double(t_after_out    - t_after_in)         / COUNTS_PER_SECOND;
                        probe_phase_wait_ms = 1000.0 * double(t_panel_mul_end - t_after_out)       / COUNTS_PER_SECOND;
                        // Reading/stopping/resetting the perf counters over the debug
                        // path is slow; track it so it is excluded from GEMM time.
                        XTime t_prof2; XTime_GetTime(&t_prof2);
                        probe_in_cycles  = gr.read_profiling(pid_in);
                        probe_out_cycles = gr.read_profiling(pid_out);
                        gr.stop_profiling(pid_in);
                        gr.stop_profiling(pid_out);
                        XAie_PerfCounterGet(devInst, coreTile, XAIE_CORE_MOD, 0U, &probe_active_cycles);
                        XAie_PerfCounterGet(devInst, coreTile, XAIE_CORE_MOD, 1U, &probe_vec_instrs);
                        XAie_PerfCounterGet(devInst, coreTile, XAIE_CORE_MOD, 2U, &probe_stream_stall);
                        XAie_PerfCounterGet(devInst, coreTile, XAIE_CORE_MOD, 3U, &probe_lock_stall);
                        XAie_PerfCounterReset(devInst, coreTile, XAIE_CORE_MOD, 0U);
                        XAie_PerfCounterReset(devInst, coreTile, XAIE_CORE_MOD, 1U);
                        XAie_PerfCounterReset(devInst, coreTile, XAIE_CORE_MOD, 2U);
                        XAie_PerfCounterReset(devInst, coreTile, XAIE_CORE_MOD, 3U);
                        XAie_PerfCounterResetControlReset(devInst, coreTile, XAIE_CORE_MOD, 0U);
                        XAie_PerfCounterResetControlReset(devInst, coreTile, XAIE_CORE_MOD, 1U);
                        XAie_PerfCounterResetControlReset(devInst, coreTile, XAIE_CORE_MOD, 2U);
                        XAie_PerfCounterResetControlReset(devInst, coreTile, XAIE_CORE_MOD, 3U);
                        XTime t_prof3; XTime_GetTime(&t_prof3);
                        prof_overhead_ms += 1000.0 * double(t_prof3 - t_prof2) / COUNTS_PER_SECOND;
                    }
#endif

#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
                    XTime t_acc0; XTime_GetTime(&t_acc0);
#endif
                    if (k_panel == 0)
                        for (int idx = 0; idx < NUM_HW_ROWS * MAT_A_CHUNK; idx++)
                            cAcc[idx] = mem_out[idx];
                    else
                        for (int idx = 0; idx < NUM_HW_ROWS * MAT_A_CHUNK; idx++)
                            cAcc[idx] += mem_out[idx];
#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
                    XTime t_acc1; XTime_GetTime(&t_acc1);
                    host_acc_ms += 1000.0 * double(t_acc1 - t_acc0) / COUNTS_PER_SECOND;
#endif
                }

                // Store accumulated result for post-run spot verification.
                int panel_base = (m_panel * NP + n_panel) * NUM_HW_ROWS * MAT_A_CHUNK;
#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
                XTime t_store0; XTime_GetTime(&t_store0);
#endif
                for (int idx = 0; idx < NUM_HW_ROWS * MAT_A_CHUNK; idx++)
                    all_C[matrix_idx][panel_base + idx] = cAcc[idx];
#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
                XTime t_store1; XTime_GetTime(&t_store1);
                host_store_ms += 1000.0 * double(t_store1 - t_store0) / COUNTS_PER_SECOND;
#endif

                panels_checked++;
            }
        }
    }

#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
    XTime t_gemm_end;
    XTime_GetTime(&t_gemm_end);
#endif

    gr.end();

#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
    // Raw wall of the timed region, then the reported GEMM time with the
    // non-workload scaffolding removed: the golden-check store (all_C,
    // verification-only bookkeeping) and the profiling instrumentation
    // (perf-counter arm/read/reset). Neither is GEMM work, so both are excluded
    // from "Total GEMM time" and every throughput number derived from it.
    double gemm_wall_ms   = 1000.0 * static_cast<double>(t_gemm_end - t_gemm_start) / COUNTS_PER_SECOND;
    double gemm_ms        = gemm_wall_ms - host_store_ms - prof_overhead_ms;
    double avg_panel_mul_ms      = total_panel_mul_ms / static_cast<double>(total_panel_mul_iters);

    // CEILING_REPEAT>1: each tile does this many compute passes per N-block on
    // local buffers, so the actual MAC work (and thus the FLOPs the cycle
    // counters reflect) is scaled by this factor. =1 for normal runs.
    const double compute_repeat = static_cast<double>(CEILING_REPEAT);
    double total_flops    = compute_repeat * static_cast<double>(NUM_MATRICES) *
                            2.0 * static_cast<double>(MATRIX_SIZE) *
                                  static_cast<double>(MATRIX_SIZE) *
                                  static_cast<double>(MATRIX_SIZE);
    double panel_mul_flops = compute_repeat * 2.0 * static_cast<double>(PANEL_SIZE) *
                                   static_cast<double>(PANEL_SIZE) *
                                   static_cast<double>(PANEL_SIZE);
    double gflops_wall        = total_flops / (gemm_ms * 1e-3) / 1e9;
    double gflops_panel_mul   = panel_mul_flops / (avg_panel_mul_ms * 1e-3) / 1e9;

    // --- Layer 2/3 metrics are reported FREQUENCY-FREE (cycles, bytes/cycle,
    // FLOP/cycle, cycle-fractions). We deliberately do NOT convert AIE cycles to
    // seconds, because the true AIE core clock is not known here and a guessed
    // constant would silently corrupt every derived ms/GB/s/GFLOPS number.
    // The only absolute throughput figures are the Layer 1 wall-clock GFLOPS,
    // which come from the real PS timer (XTime) and need no AIE clock. ---
    // INT8 in (A stripe + B panel), INT8 out (saturated C result).
    double in_bytes  = static_cast<double>(MAT_A_CHUNK_BYTES_IN + NUM_ELMNTS * static_cast<int>(sizeof(GEMM_AB_T)));
    double out_bytes = static_cast<double>(MAT_A_CHUNK_BYTES_OUT);
    // Bytes moved per stream-active cycle (multiply by the real AIE clock for GB/s).
    double in_bytes_per_cyc  = (probe_in_cycles  > 0) ? (in_bytes  / static_cast<double>(probe_in_cycles))  : 0.0;
    double out_bytes_per_cyc = (probe_out_cycles > 0) ? (out_bytes / static_cast<double>(probe_out_cycles)) : 0.0;

    bool   out_cycles_suspect = (probe_out_cycles > 0 && probe_out_cycles <= 4096);

    double vec_util_frac = (probe_active_cycles > 0U) ?
                           (static_cast<double>(probe_vec_instrs) /
                            static_cast<double>(probe_active_cycles)) : 0.0;
    double vec_util      = 100.0 * vec_util_frac;

    uint32_t probe_other_stall = 0U;
    if (probe_active_cycles > probe_stream_stall + probe_lock_stall)
        probe_other_stall = probe_active_cycles - probe_stream_stall - probe_lock_stall;
    double stream_pct      = (probe_active_cycles > 0U) ?
                              (100.0 * static_cast<double>(probe_stream_stall) /
                               static_cast<double>(probe_active_cycles)) : 0.0;
    double lock_pct        = (probe_active_cycles > 0U) ?
                              (100.0 * static_cast<double>(probe_lock_stall) /
                               static_cast<double>(probe_active_cycles)) : 0.0;
    double other_pct       = (probe_active_cycles > 0U) ?
                              (100.0 * static_cast<double>(probe_other_stall) /
                               static_cast<double>(probe_active_cycles)) : 0.0;

    // INFORMATIONAL ONLY (not used in any metric): the core was active for
    // probe_active_cycles within a probe-step wall of probe_panel_mul_ms, and active
    // cycles cannot exceed wall time, so the core clock is at least this value.
    // Derived from measurement, not a configured constant.
    double implied_core_ghz = (probe_panel_mul_ms > 0.0) ?
                              (static_cast<double>(probe_active_cycles) /
                               (probe_panel_mul_ms * 1e-3) / 1e9) : 0.0;

    // Frequency-free compute efficiency: array FLOPs accomplished per AIE core cycle.
    // panel_mul_flops is the whole-array work; all 16 tiles run concurrently for ~the
    // same cycle count, so these are array-level FLOP/cycle figures.
    double flops_per_active_cyc   = (probe_active_cycles > 0U) ?
                                    (panel_mul_flops / static_cast<double>(probe_active_cycles)) : 0.0;
    double flops_per_compute_cyc  = (probe_other_stall > 0U) ?
                                    (panel_mul_flops / static_cast<double>(probe_other_stall)) : 0.0;
    double flops_per_cyc_full_vec = (vec_util_frac > 0.0) ?
                                    (flops_per_active_cyc / vec_util_frac) : 0.0;
    double vec_headroom_x         = (vec_util_frac > 0.0) ? (1.0 / vec_util_frac) : 0.0;

    static constexpr double DEVICE_INT8_TOPS   = 184.0;
    static constexpr int    DEVICE_TOTAL_TILES = 144;
    static constexpr int    ARRAY_TILES        = NUM_HW_ROWS * NUM_HW_COLS;
    // INT8 matrix-engine peak (no INT8/4 downscale — this run IS int8).
    double device_int8_tops       = DEVICE_INT8_TOPS;
    double device_int8_gops       = device_int8_tops * 1000.0;   // GOPS
    double peak_int8_tops         = device_int8_tops *
                                    (static_cast<double>(ARRAY_TILES) /
                                     static_cast<double>(DEVICE_TOTAL_TILES));
    double peak_int8_gops         = peak_int8_tops * 1000.0;
    // Measured throughput in TOPS (2*panel^3 ops / avg panel-multiply time).
    double measured_tops          = gflops_panel_mul / 1000.0;
    // Wall-based utilisation is frequency-free: measured GOPS (real seconds) over
    // peak GOPS (spec). The AIE clock cancels and never enters this ratio.
    double hw_util_array_pct      = (peak_int8_gops > 0.0) ?
                                     (gflops_panel_mul / peak_int8_gops * 100.0) : 0.0;
    double hw_util_device_pct     = (device_int8_gops > 0.0) ?
                                     (gflops_panel_mul / device_int8_gops * 100.0) : 0.0;

    std::cout << std::fixed << std::setprecision(3);

    std::cout << "\n=== OOB 4x4 Profiling Report ===\n";
    std::cout << "  Matrices:           " << NUM_MATRICES << " x "
              << MATRIX_SIZE << " x " << MATRIX_SIZE
              << "  (panel " << PANEL_SIZE << ", " << NP << " panels/dim)\n";
    std::cout << "  Array:              " << NUM_HW_ROWS << " x " << NUM_HW_COLS
              << " = " << ARRAY_TILES << " tiles  (" << ARRAY_TILES
              << "/" << DEVICE_TOTAL_TILES << " of device)\n";
    std::cout << "  Completed:          " << NUM_MATRICES << " matrices, "
              << total_panel_mul_iters << " panel-multiply steps, "
              << panels_checked << " output panels\n";
    if (CEILING_REPEAT > 1)
        std::cout << "  CEILING PROBE:      compute repeated " << CEILING_REPEAT
                  << "x/N-block on local buffers (FLOP accounting scaled x"
                  << CEILING_REPEAT << ")\n";

    std::cout << "\n--- Layer 1: PS Wall-Clock ---\n";
    std::cout << "  gr.init() time:     " << init_ms   << " ms\n";
    std::cout << "  Input prep (1-time): " << prep_ms  << " ms  (pack "
              << (devA_elems + devB_elems) * sizeof(GEMM_AB_T) / 1024 << " kB into dev_A/dev_B; not in GEMM time)\n";
    std::cout << "  Total GEMM time:    " << gemm_ms   << " ms  (" << NUM_MATRICES
              << " matrices, " << total_panel_mul_iters << " panel-multiply steps; golden-check store excluded)\n";
    std::cout << "  Wall-clock GFLOPS:  " << gflops_wall << " GFLOPS  ("
              << NUM_MATRICES << " x 2*M^3 / total_ms)\n";
    std::cout << "  Per panel-multiply: " << gflops_panel_mul
              << " GFLOPS  (2*panel^3 / avg panel-multiply time)\n";
    std::cout << "  panel-multiply avg / min / max: " << avg_panel_mul_ms << " / "
              << min_panel_mul_ms << " / " << max_panel_mul_ms << " ms\n";

    // Per-invocation phase breakdown: where one gr.run()'s wall time goes. This
    // locates the fixed overhead floor that compute optimizations can't shrink.
    double probe_phase_sum = probe_phase_run_ms + probe_phase_in_ms +
                             probe_phase_out_ms + probe_phase_wait_ms;
    auto phase_pct = [&](double p) { return probe_panel_mul_ms > 0.0 ? 100.0 * p / probe_panel_mul_ms : 0.0; };
    std::cout << "\n--- Layer 1b: Per-Invocation Phase Breakdown (probe step) ---\n";
    std::cout << "  graph launch (1x):  " << run_launch_ms << " ms  (one-time gr.run(" << total_panel_mul_iters
              << "), excluded from per-panel wall below)\n";
    std::cout << "  input DMA issue:    " << probe_phase_in_ms   << " ms  (" << phase_pct(probe_phase_in_ms)   << "%)\n";
    std::cout << "  output DMA issue:   " << probe_phase_out_ms  << " ms  (" << phase_pct(probe_phase_out_ms)  << "%)\n";
    std::cout << "  gmio_wait (drain):  " << probe_phase_wait_ms << " ms  (" << phase_pct(probe_phase_wait_ms) << "%)\n";
    std::cout << "  sum / probe wall:   " << probe_phase_sum << " / " << probe_panel_mul_ms << " ms\n";

    // Layer 1c: host post-processing, summed over all steps. Separates real GEMM
    // k-accumulation from verification-only bookkeeping, and shows the GEMM time
    // with the verification store removed (verification is not part of the workload).
    auto gemm_pct = [&](double p) { return gemm_ms > 0.0 ? 100.0 * p / gemm_ms : 0.0; };
    std::cout << "\n--- Layer 1c: Host Post-Processing (sum over all steps) ---\n";
    std::cout << "  cAcc = / += mem_out:   " << host_acc_ms   << " ms  (" << gemm_pct(host_acc_ms)
              << "% of GEMM)  [k-accum; reads device mem_out from PS; IN GEMM time]\n";
    std::cout << "  all_C store:           " << host_store_ms << " ms  [verification only; EXCLUDED from GEMM time]\n";
    std::cout << "  profiling overhead:    " << prof_overhead_ms << " ms  [perf-counter arm/read/reset; EXCLUDED from GEMM time]\n";
    std::cout << "  GEMM time raw wall:    " << gemm_wall_ms  << " ms  (timed region incl. store + profiling, for reference)\n";

    std::cout << "\n--- Layer 2: GMIO Stream (first panel-multiply, gradf.in[0] / gradf.out[0]) ---\n";
    std::cout << "  in[0]  active cycles:  " << probe_in_cycles << "\n";
    std::cout << "  out[0] active cycles:  " << probe_out_cycles;
    if (out_cycles_suspect)
        std::cout << "  [NOTE: may reflect only the first DMA burst, not the full transfer]";
    std::cout << "\n";
    std::cout << std::setprecision(4);
    std::cout << "  in  throughput:        " << in_bytes_per_cyc  << " bytes/cycle  ("
              << std::setprecision(3) << in_bytes/1024.0  << " kB over " << probe_in_cycles  << " cyc)\n";
    std::cout << std::setprecision(4);
    std::cout << "  out throughput:        " << out_bytes_per_cyc << " bytes/cycle  ("
              << std::setprecision(3) << out_bytes/1024.0 << " kB over " << probe_out_cycles << " cyc)";
    if (out_cycles_suspect) std::cout << "  [suspect]";
    std::cout << "\n  (multiply bytes/cycle by the real AIE clock for GB/s)\n";

    std::cout << "\n--- Layer 3: AIE Core Tile (col=0, row=" << static_cast<int>(devInst->AieTileRowStart)
              << ") Cycle Budget (first panel-multiply) ---\n";
    std::cout << std::setprecision(1);
    std::cout << "  Active cycles:         " << probe_active_cycles << "\n";
    std::cout << "  Stream stall cycles:   " << probe_stream_stall
              << "  (" << stream_pct      << "% of active)  [waiting for window/cascade data]\n";
    std::cout << "  Lock stall cycles:     " << probe_lock_stall
              << "  (" << lock_pct        << "% of active)  [waiting for buffer lock]\n";
    std::cout << "  Other/compute cycles:  " << probe_other_stall
              << "  (" << other_pct       << "% of active)\n";
    std::cout << "  Vector instructions:   " << probe_vec_instrs    << "\n";
    std::cout << "  Vec utilization:       " << vec_util << " %  (vec instrs / active cycles)\n";
    std::cout << std::setprecision(3);
    std::cout << "  [info] probe-step wall: " << probe_panel_mul_ms
              << " ms -> implied core clock >= " << std::setprecision(2) << implied_core_ghz
              << " GHz (measured, not used in any metric below)\n";

    std::cout << "\n--- Kernel Efficiency (frequency-free; probed tile, first panel-multiply) ---\n";
    std::cout << std::setprecision(3);
    std::cout << "  Array FLOP / active-cycle:   " << flops_per_active_cyc
              << "  (2*panel^3 / active cycles)\n";
    std::cout << "  Array FLOP / compute-cycle:  " << flops_per_compute_cyc
              << "  (excludes lock/stream stalls)\n";
    std::cout << "  Ceiling FLOP / cycle:        " << flops_per_cyc_full_vec
              << "  (active value / vec util; kernel ceiling at 100% vec)\n";
    std::cout << std::setprecision(1);
    std::cout << "  Vec slot utilization:        " << vec_util
              << " %  (SIMD instrs per active cycle)\n";
    std::cout << "  Compute fraction:            " << other_pct
              << " % of active cycles  (not lock/stream stalled)\n";
    std::cout << std::setprecision(2);
    std::cout << "  Vectorization headroom:      " << vec_headroom_x
              << "x  (1 / vec utilization)\n";
    std::cout << "  NOTE: absolute GFLOPS for these needs the real AIE clock (multiply\n"
              << "        FLOP/cycle by it). Layer 1 wall-clock GFLOPS is already clock-free.\n";

    std::cout << "\n--- Hardware Utilization (INT8) ---\n";
    std::cout << std::setprecision(2);
    std::cout << "  Device INT8 peak:      " << device_int8_tops    << " TOPS  ("
              << DEVICE_TOTAL_TILES << " tiles)\n";
    std::cout << "  Array INT8 peak:       " << peak_int8_tops      << " TOPS  ("
              << ARRAY_TILES << "/" << DEVICE_TOTAL_TILES << " active tiles)\n";
    std::cout << "  Measured (panel-mul):  " << std::setprecision(3) << gflops_panel_mul << " GOPS  ("
              << measured_tops << " TOPS)\n";
    std::cout << std::setprecision(4);
    std::cout << "  vs active tiles  (" << ARRAY_TILES << "):   " << hw_util_array_pct
              << " %\n";
    std::cout << "  vs full device  (" << DEVICE_TOTAL_TILES << "):   " << hw_util_device_pct
              << " %\n";

    // Spot-verify a spread of (matrix, panel, chain) entries from stored all_C results.
    // Uses the same sorted-row-comparison as the per-panel check would, tolerating any
    // column permutation in the kernel's output layout.
    struct SpotSample { int matrix_idx, m_panel, n_panel, chain; };
    // With NUM_PANELS panels per dim there are NP*NP output panels; sample a
    // spread of (matrix, panel, chain) that is valid for any NP >= 1 and the
    // single matrix (matrix_idx 0).
    const SpotSample spots[] = {
        {0, 0, 0, 0},                   // first panel, first chain
        {0, 0, 0, NUM_HW_ROWS - 1},     // first panel, last chain
        {0, (NP - 1), 0, 2},            // last m-panel
        {0, 0, (NP - 1), 1},            // last n-panel
        {0, (NP - 1), (NP - 1), NUM_HW_ROWS - 1}, // corner panel, last chain
    };
    std::cout << "\n--- Spot Verification (" << NUM_MATRICES << " matrices stored) ---\n";
    for (const auto& s : spots) {
        auto Aval_s = [matrix_idx = s.matrix_idx](int row) -> int32_t {
            return (int32_t)((row + matrix_idx) % 7 + 1);
        };
        int panel_base = (s.m_panel * NP + s.n_panel) * NUM_HW_ROWS * MAT_A_CHUNK;
        std::vector<int32_t> got(
            all_C[s.matrix_idx].begin() + panel_base + s.chain * MAT_A_CHUNK,
            all_C[s.matrix_idx].begin() + panel_base + (s.chain + 1) * MAT_A_CHUNK);
        std::vector<int32_t> exp;
        exp.reserve(MAT_A_CHUNK);
        for (int lr = 0; lr < NUM_ROWS_PER_HW_ROW; lr++) {
            int grow = s.m_panel * P + s.chain * NUM_ROWS_PER_HW_ROW + lr;
            int32_t rowsum = (int32_t)M * Aval_s(grow);
            // C streams out as int8: each tile saturates its full-K int32 result
            // to int8 (matches aiehlc simplematmul2). Clamp the golden the same way.
            // (With B==1 and K>=256 every rowsum >= 256, so these all saturate to 127.)
            if (rowsum > 127) rowsum = 127;
            else if (rowsum < -128) rowsum = -128;
            for (int c = 0; c < P; c++)
                exp.push_back(rowsum);
        }
        std::sort(got.begin(), got.end());
        std::sort(exp.begin(), exp.end());
        if (got == exp) {
            std::cout << "  OK   matrix=" << s.matrix_idx << " C[m=" << s.m_panel
                      << ",n=" << s.n_panel << "] chain=" << s.chain << "\n";
        } else {
            test_passed = false;
            std::cout << "  FAIL matrix=" << s.matrix_idx << " C[m=" << s.m_panel
                      << ",n=" << s.n_panel << "] chain=" << s.chain << "\n";
            int shown = 0;
            for (int i = 0; i < (int)got.size() && shown < 4; i++) {
                if (got[i] != exp[i]) {
                    std::cout << "    sorted[" << i << "] got=" << got[i]
                              << " exp=" << exp[i] << "\n";
                    shown++;
                }
            }
        }
    }
    std::cout << "=================================\n";
#else
    std::cout << "OOB 4x4 blocked GEMM: " << NUM_MATRICES << " x " << M << "x" << M
              << ", panel " << P << ", " << NP << " panels/dim\n";
    std::cout << "Completed: " << panels_checked << " / " << (NUM_MATRICES * NP * NP)
              << " output panels\n";
#endif
    if (test_passed) {
        std::cout << "\nOOB Test Passed\n";
        return 0;
    }
    std::cout << "\nOOB Test FAILED\n";
    return 1;
}
