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
 * @file aeg_pl_kernel.cc
 *
 * Runtime implementation of `aeg::pl_kernel` (declared in
 * `aeg_baremetal_api_pl.h`). Replaces the hand-written register-poke
 * boilerplate present in `example/example_kernelplio/src/graph_new_api.cpp`
 * (`start_pl()`, `wait_s2mm()`) with a typed handle.
 *
 * Under simulation the methods are no-ops, mirroring the existing pattern
 * of guarding raw `Xil_Out32`/`Xil_In32` calls behind
 * `#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))`.
 */

#include "aeg_baremetal_api_pl.h"
#include "aeg_log.h"

#include <cinttypes>
#include <cstdint>
#include <cstring>
#include <type_traits>

#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "xil_io.h"
#include <unistd.h>
#endif

namespace {

// HLS s_axilite control-register layout (UG902).
constexpr uint32_t kCtrlOffset   = 0x0U;
constexpr uint32_t kApStartBit   = 0x1U; // bit 0
constexpr uint32_t kApDoneBit    = 0x2U; // bit 1
constexpr uint32_t kApIdleBit    = 0x4U; // bit 2
// ap_idle (bit 2) is intentionally not used as a completion signal in
// wait()/done(), and must not be polled *before* writing AP_START (can hang
// at reset). ap_done alone is reliable for completion. After AP_START is
// written, start() polls ap_idle briefly (kApIdleStartRetries) as a
// best-effort sanity check that the FSM left idle (UG902/UG1399); the FSM
// may need a few cycles to transition, so a single immediate read is racy.

// 4-byte alignment mask for s_axilite register offsets.
constexpr uint32_t kRegAlignMask = 0x3U;

// Helpers for splitting a 64-bit value (e.g. a DDR pointer) across the
// low/high register pair that the HLS m_axi master-pointer ABI uses.
constexpr uint32_t kLow32Mask  = 0xFFFFFFFFU;
constexpr uint32_t kU32BitWidth = 32U;

// Post-start ap_idle polls before reporting backend_not_ready. The FSM
// needs a few cycles to leave idle after AP_START is written.
constexpr uint32_t kApIdleStartRetries = 64U;

#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
inline void write_u32(uintptr_t addr, uint32_t value) { Xil_Out32(addr, value); }
inline uint32_t read_u32(uintptr_t addr) { return Xil_In32(addr); }

// Poll cadence for wait()/stop(), matching aie-rt baremetal and aeg_cdo.h.
// Typed as useconds_t so usleep() is called without a numeric conversion.
constexpr useconds_t kPollIntervalUs = 200U;
#endif

} // namespace

namespace {

// Assembles the value arithmetically rather than writing through its object
// representation, so no cast to the destination type is needed. Source bytes
// are little-endian, matching the s_axilite register layout.
template <typename T>
void copy_from_bytes(T &value, const void *data) {
    static_assert(std::is_unsigned<T>::value, "copy_from_bytes supports unsigned integral types only");
    const auto *bytes = static_cast<const uint8_t *>(data);
    T result = 0U;
    std::size_t i = 0U;
    while (i < sizeof(T)) {
        const uint32_t shift = static_cast<uint32_t>(i) * 8U;
        result = static_cast<T>(result | (static_cast<T>(bytes[i]) << shift));
        ++i;
    }
    value = result;
}

} // namespace

namespace aeg {

class pl_kernel_impl {
public:
    explicit pl_kernel_impl(uintptr_t base_addr) : base_(base_addr) {}

    aeg_err set_arg_raw(uint32_t offset, const void *data, std::size_t bytes) const {
        AEG_LOG("pl_kernel set_arg_raw() called: base 0x%" PRIx64 " offset 0x%" PRIx32 " bytes %" PRIu64 "\n",
                static_cast<uint64_t>(base_), offset,
                static_cast<uint64_t>(bytes));
        if (data == nullptr) {
            AEG_ERROR("%s: null argument", "set_arg_raw");
            return aeg_err::null_argument;
        }
        if ((bytes != sizeof(uint32_t)) && (bytes != sizeof(uint64_t))) {
            AEG_ERROR("%s: invalid length argument", "set_arg_raw");
            return aeg_err::invalid_length;
        }
        // s_axilite registers are 32-bit and 4-byte aligned.
        if ((offset & kRegAlignMask) != 0) {
            AEG_ERROR("%s: misaligned register offset 0x%" PRIx32, "set_arg_raw", offset);
            return aeg_err::user_error;
        }

#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
        if (bytes == sizeof(uint32_t)) {
            uint32_t value = 0U;
            copy_from_bytes(value, data);
            AEG_LOG("pl_kernel set_arg_raw() write32: addr 0x%" PRIx64 " value 0x%08" PRIx32 "\n",
                    static_cast<uint64_t>(base_ + offset), value);
            write_u32(base_ + offset, value);
            // Read back and verify the argument register latched the value.
            const uint32_t readback = read_u32(base_ + offset);
            if (readback != value) {
                AEG_ERROR("%s: write32 verify failed at offset 0x%" PRIx32
                          " (wrote 0x%08" PRIx32 ", read 0x%08" PRIx32 ")",
                          "set_arg_raw", offset, value, readback);
                return aeg_err::postcondition_failed;
            }
        } else {
            // 64-bit value (typically a pointer) split into low/high pair,
            // matching the HLS m_axi master-pointer ABI used by the kernels
            // in example/example_kernelplio.
            uint64_t value = 0U;
            copy_from_bytes(value, data);
            AEG_LOG("pl_kernel set_arg_raw() write64: addr 0x%" PRIx64 " value 0x%016" PRIx64 "\n",
                    static_cast<uint64_t>(base_ + offset),
                    static_cast<uint64_t>(value));
            const uint32_t low  = static_cast<uint32_t>(value & kLow32Mask);
            const uint32_t high = static_cast<uint32_t>(value >> kU32BitWidth);
            write_u32(base_ + offset, low);
            write_u32(base_ + offset + sizeof(uint32_t), high);
            // Read back both halves and verify they latched the value.
            const uint32_t low_rb  = read_u32(base_ + offset);
            const uint32_t high_rb = read_u32(base_ + offset + sizeof(uint32_t));
            if ((low_rb != low) || (high_rb != high)) {
                AEG_ERROR("%s: write64 verify failed at offset 0x%" PRIx32
                          " (wrote 0x%08" PRIx32 "_%08" PRIx32
                          ", read 0x%08" PRIx32 "_%08" PRIx32 ")",
                          "set_arg_raw", offset, high, low, high_rb, low_rb);
                return aeg_err::postcondition_failed;
            }
        }
#else
        (void)offset;
#endif
        return aeg_err::ok;
    }

    aeg_err start() const {
        AEG_LOG("pl_kernel start() called: base 0x%" PRIx64 "\n",
                static_cast<uint64_t>(base_));
#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
        // Clear AP_START before setting it so every start() produces a real
        // falling-then-rising edge, even if a previous invocation left it
        // asserted. Do not gate this on AP_IDLE (see kApDoneBit comment
        // above) — spinning on AP_IDLE before writing AP_START can hang.
        write_u32(base_ + kCtrlOffset, 0U);
        write_u32(base_ + kCtrlOffset, kApStartBit);
        // Per UG902/UG1399, ap_idle goes Low once the design is operating.
        // Retry briefly — an immediate read can race the FSM transition.
        for (uint32_t retry = 0; retry < kApIdleStartRetries; ++retry) {
            const uint32_t ctrl = read_u32(base_ + kCtrlOffset);
            if ((ctrl & kApIdleBit) == 0) {
                return aeg_err::ok;
            }
        }
        const uint32_t ctrl = read_u32(base_ + kCtrlOffset);
        AEG_ERROR("%s: ap_idle still High after start (ctrl 0x%08" PRIx32 ")", "start", ctrl);
        return aeg_err::backend_not_ready;
#else
        return aeg_err::ok;
#endif
    }

    aeg_err wait(uint64_t timeout_us = kWaitTimeoutUs) const {
        AEG_LOG("pl_kernel wait() called: base 0x%" PRIx64
                " polling for AP_DONE (timeout_us=%" PRIu64 ")\n",
                static_cast<uint64_t>(base_), timeout_us);
#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
        // Poll the control register until AP_DONE is asserted. See the
        // kApDoneBit comment above for why AP_IDLE must not be OR'd in here.
        // Bounded by timeout_us; poll every kPollIntervalUs (aie-rt cadence).
        uint64_t count = (timeout_us + kPollIntervalUs - 1U) / kPollIntervalUs;
        while (count > 0U) {
            const uint32_t ctrl = read_u32(base_ + kCtrlOffset);
            if ((ctrl & kApDoneBit) != 0) {
                AEG_LOG("pl_kernel wait() done: base 0x%" PRIx64 " ctrl 0x%08" PRIx32 "\n",
                        static_cast<uint64_t>(base_), ctrl);
                return aeg_err::ok;
            }
            static_cast<void>(usleep(kPollIntervalUs));
            count--;
        }
        const uint32_t ctrl = read_u32(base_ + kCtrlOffset);
        if ((ctrl & kApDoneBit) != 0) {
            return aeg_err::ok;
        }
        AEG_ERROR("%s: timed out waiting for AP_DONE after %" PRIu64 " us (base 0x%" PRIx64 ")",
                  "wait", timeout_us, static_cast<uint64_t>(base_));
        return aeg_err::resource_unavailable;
#else
        (void)timeout_us;
        return aeg_err::ok;
#endif
    }

    // Single-shot AP_CTRL_HS: there is no runtime stop primitive in the
    // standard HLS control protocol, so stop() degenerates to wait().
    aeg_err stop() const {
        AEG_LOG("pl_kernel stop() called: base 0x%" PRIx64 " (delegating to wait)\n",
                static_cast<uint64_t>(base_));
        return wait();
    }

    bool done() const {
#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
        const uint32_t ctrl = read_u32(base_ + kCtrlOffset);
        const bool is_done = (ctrl & kApDoneBit) != 0;
        AEG_LOG("pl_kernel done() called: base 0x%" PRIx64 " ctrl 0x%08" PRIx32 " done %d\n",
                static_cast<uint64_t>(base_), ctrl, is_done ? 1 : 0);
        return is_done;
#else
        AEG_LOG("pl_kernel done() called (sim): base 0x%" PRIx64 " done 1\n",
                static_cast<uint64_t>(base_));
        return true;
#endif
    }

private:
    uintptr_t base_;
};

// --- pl_kernel public-class delegators ---------------------------------

pl_kernel::pl_kernel(uintptr_t base_addr) : handle_(std::make_shared<pl_kernel_impl>(base_addr)) {}

pl_kernel::~pl_kernel() = default;

aeg_err pl_kernel::set_arg_raw(uint32_t offset, const void *data, std::size_t bytes) const {
    return handle_->set_arg_raw(offset, data, bytes);
}

aeg_err pl_kernel::start() const { return handle_->start(); }
aeg_err pl_kernel::wait(uint64_t timeout_us) const { return handle_->wait(timeout_us); }
aeg_err pl_kernel::stop() const { return handle_->stop(); }
bool pl_kernel::done() const { return handle_->done(); }

} // namespace aeg
