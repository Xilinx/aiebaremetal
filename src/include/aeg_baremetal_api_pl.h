/**<!--
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. A copy of the
 * License is located at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 * -->
 */

/**
 * @file aeg_baremetal_api_pl.h
 *
 * Unified host-side control API for PL (programmable logic) kernels in
 * the AEG bare-metal runtime. Provides a typed handle around the standard
 * Vitis-HLS s_axilite control protocol so host code does not have to
 * issue raw `Xil_Out32`/`Xil_In32` pokes against the HLS-generated
 * register map.
 *
 * The PL-kernel handle is intentionally independent of `AIEGraph`: a PL
 * kernel can be exercised in isolation by host code that does not
 * instantiate any AIE graph.
 *
 * Linux equivalent (informational, not a dependency): xrt::kernel /
 * xrt::run from XRT (https://github.com/Xilinx/XRT).
 */

#ifndef AEG_BAREMETAL_API_PL_H
#define AEG_BAREMETAL_API_PL_H

#include "aeg_baremetal_api.h" // for aeg_err

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace aeg {

class pl_kernel_impl;
// Default timeout for wait()/stop(), in microseconds. Polling uses 200us
// steps, matching the aie-rt baremetal backend (see aeg_cdo.h reg_maskpoll).
static constexpr uint64_t kWaitTimeoutUs = 5000000ULL;
/**
 * @class pl_kernel
 *
 * Host-side handle for a Vitis-HLS PL kernel that uses the standard
 * s_axilite control protocol (`ap_ctrl_hs`).
 *
 * Standard register map (HLS s_axilite, see UG902):
 *   - 0x00  control: bit0 AP_START, bit1 AP_DONE, bit2 AP_IDLE, bit3 AP_READY
 *   - 0x04  GIE  (global interrupt enable)
 *   - 0x08  IER  (IP interrupt enable)
 *   - 0x0C  ISR  (IP interrupt status)
 *   - 0x10+ kernel arguments (offsets per HLS-generated driver header)
 *
 * Argument offsets are kernel-specific. For the kernels in
 * `example/example_kernelplio` (`mm2s(int* mem, int size)` and
 * `s2mm(int* mem, int size)`) the layout is:
 *   - 0x10  mem (low 32 bits)
 *   - 0x14  mem (high 32 bits)
 *   - 0x1C  size
 *
 * Typical usage:
 *
 *     aeg::pl_kernel mm2s(XPAR_XMM2S_0_S_AXI_CONTROL_BASEADDR);
 *     mm2s.set_arg(0x10, input_ptr);   // 64-bit pointer: low/high split
 *     mm2s.set_arg(0x1C, length);      // 32-bit scalar
 *     mm2s.start();
 *     // ... AIE graph runs in parallel ...
 *     mm2s.wait();
 *
 * Under simulation (`__AIESIM__` or `__ADF_FRONTEND__`) all methods are
 * no-ops because the AIE simulator does not model PL kernels; this
 * matches the existing pattern in `example/example_kernelplio`.
 *
 * The class is shallow-copyable (PIMPL via `std::shared_ptr`); copies
 * share the same underlying register-file binding.
 */
class pl_kernel {
public:
    /**
     * Construct a kernel handle bound to the given s_axilite base address.
     *
     * @param base_addr  S_AXI_CONTROL base address. On hardware this is
     *                   `XPAR_X<KERNEL>_0_S_AXI_CONTROL_BASEADDR` from
     *                   `xparameters.h`, or `XPAR_X<KERNEL>_0_BASEADDR`
     *                   on platforms (e.g. VEK385) where Vitis omits the
     *                   `_S_AXI_CONTROL` suffix.
     */
    explicit pl_kernel(uintptr_t base_addr);

    ~pl_kernel();

    /**
     * Write a kernel argument at the given s_axilite register offset.
     *
     *  - 4-byte values: written as a single 32-bit register.
     *  - 8-byte values (e.g. `T*` to DDR): split into low/high pair at
     *    `offset` and `offset + 4` (HLS m_axi pointer convention).
     *
     * @param offset Register offset of the argument (per HLS driver
     *               header, or by inspection of the kernel signature).
     * @param value  Argument value.
     * @return aeg_err::ok on success; aeg_err::user_error if the offset
     *         is not 4-byte aligned.
     */
    template <typename T>
    aeg_err set_arg(uint32_t offset, T value);

    /**
     * Assert AP_START in the control register.
     *
     * Asynchronous: returns as soon as the start bit is written. Use
     * wait() or done() to observe completion.
     *
     * Does NOT gate on AP_IDLE before writing (an earlier revision did;
     * it hung on real hardware — see the AP_IDLE comment in
     * aeg_pl_kernel.cc for why). After AP_START is written, polls
     * ap_idle briefly to confirm the FSM left idle; returns
     * aeg_err::backend_not_ready if it never transitions.
     *
     * If a fast restart on the same instance ever drops an AP_START write
     * in practice, that will need a different fix once diagnosed with real
     * register data.
     */
    aeg_err start() const;

    /**
     * Block until the kernel asserts AP_DONE.
     *
     * Deliberately checks AP_DONE only, not AP_IDLE: AP_IDLE is a level
     * that stays high from one invocation's completion until the FSM
     * registers the *next* AP_START, so on a restart (calling start()+
     * wait() again on the same instance) a poll that accepts AP_IDLE can
     * observe the *previous* invocation's still-latched idle state and
     * return before the new invocation has done any work. AP_DONE pulses
     * fresh per invocation and auto-clears on read (UG1399), so it alone
     * is race-free across repeated start()/wait() cycles.
     *
     * @param timeout_us Maximum time to wait for AP_DONE, in microseconds.
     *        Values below 200us are rounded up to one poll iteration.
     * @return aeg_err::ok on success; aeg_err::resource_unavailable if the
     *         timeout is reached.
     */
    aeg_err wait(uint64_t timeout_us = kWaitTimeoutUs) const;

    /**
     * Synonym for wait() under the single-shot AP_CTRL_HS protocol.
     *
     * Reserved for forward compatibility with HLS auto-restart kernels,
     * where stop() will clear AP_AUTO_RESTART and drain the in-flight
     * iteration before returning.
     */
    aeg_err stop() const;

    /**
     * Non-blocking query: true if the kernel has completed (AP_DONE
     * asserted) since the last start(). See wait() for why AP_IDLE is
     * deliberately excluded from this check.
     *
     * Note: reading the control register may clear AP_DONE (UG1399), so
     * this is not a side-effect-free query on hardware.
     *
     * Always returns true under simulation.
     */
    bool done() const;

private:
    // Non-template raw setter that the template forwards to.
    aeg_err set_arg_raw(uint32_t offset, const void *data, std::size_t bytes) const;

    std::shared_ptr<pl_kernel_impl> handle_;
};

// --- Template implementation --------------------------------------------

template <typename T>
inline aeg_err pl_kernel::set_arg(uint32_t offset, T value) {
    static_assert(std::is_trivially_copyable<T>::value,
                  "pl_kernel::set_arg requires a trivially-copyable value");
    static_assert(sizeof(T) == sizeof(std::uint32_t) || sizeof(T) == sizeof(std::uint64_t),
                  "pl_kernel::set_arg supports only 4- or 8-byte values; "
                  "widen smaller scalars to uint32_t before passing");
    return set_arg_raw(offset, &value, sizeof(T));
}

} // namespace aeg

#endif // AEG_BAREMETAL_API_PL_H
