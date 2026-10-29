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
 * @file aeg_cdo.h
 *
 * CDO (Configuration Data Object) loading support for the AEG bare-metal
 * runtime. A CDO blob is a self-describing sequence of AIE configuration
 * commands; loading it applies that configuration to the device.
 *
 * The public entry point is `AIEGraph::load_cdo(addr, len)` (declared in
 * `aeg_baremetal_api.h`), which forwards to the module-level
 * `aeg::cdo::load()` implemented in `aeg_cdo.cc`.
 */

#ifndef AEG_CDO_H
#define AEG_CDO_H

#include "aeg_baremetal_api.h" // for aeg_err

#include <xaiengine.h> // for XAie_DevInst
#include "aeg_cdo_parser.h"
#include "aeg_log.h" // for AEG_LOG
#include "xil_io.h" // for Xil_In32, Xil_Out32
#include <cstddef>
#include <cstdint>
#include <unistd.h> // for usleep

/**
 * Per-register-write CDO tracing. Opt-in, off by default.
 *
 * Every CDO register write passes through several nested helpers, each of which
 * used to log: a single MASKWRITE64 costs four lines, and applying a normal
 * five-blob CDO set emits roughly 6.9k lines (~415 KB, about 36 seconds of UART
 * at 115200 baud) -- long enough to look like a hang. The per-write trace is
 * therefore gated behind AEG_CDO_VERBOSE; build with -DAEG_CDO_VERBOSE to get
 * it back. Blob-level progress (AEG_LOG) and all errors (AEG_ERROR) are
 * unaffected and still print.
 */
#ifndef AEG_CDO_LOG
#if defined(AEG_CDO_VERBOSE)
#define AEG_CDO_LOG AEG_LOG
#else
#define AEG_CDO_LOG ::aeg_log_inline::aeg_log_write_noop
#endif
#endif /* !AEG_CDO_LOG */

namespace aeg {
namespace cdo {

/**
 * Load an in-memory CDO blob into the device.
 *
 * Forwards to `aeg::cdo::parse_and_apply()`, which validates the blob header
 * and applies each configuration command through the XAie driver.
 *
 * @param data Pointer to the CDO blob in memory.
 * @param len  Length of the CDO blob in bytes.
 * @param dev  Device instance used to issue register writes.
 * @param cdo_base Original CDO base address to strip from each AIE address
 *         (0 on APU for absolute addressing; the original AIE base on RPU).
 * @return aeg_err::ok on success; an error code on a malformed blob or a
 *         failed driver write.
 */
aeg_err load(const void *data, std::size_t len, XAie_DevInst *dev,
             std::uint64_t cdo_base);

inline void cdo_reg_write32_raw(uintptr_t addr, uint32_t val)
{
	AEG_CDO_LOG("%s: address=0x%llx value=0x%x\n", "reg_write32",
	        static_cast<uint64_t>(addr), val);
	Xil_Out32(static_cast<UINTPTR>(addr), val);
}

/*****************************************************************************
 *
 * @brief	Performs 32bit register mask write. It reads data from the
 *		register and mask out the bits to write, and then or the value
 *
 * @param	addr: contains the address to perform the input operation
 * @param	mask: mask value
 * @param	val: value to write the register
 *
 ******************************************************************************/
inline void cdo_reg_maskwrite32_raw(uintptr_t addr, uint32_t mask,
		uint32_t val)
{
	uint32_t regval = 0U;

	AEG_CDO_LOG("%s: address=0x%llx mask=0x%x value=0x%x\n", "cdo_reg_maskwrite32_raw",
	        static_cast<uint64_t>(addr), mask, val);
	regval = Xil_In32(static_cast<UINTPTR>(addr));
	cdo_reg_write32_raw(addr, (regval & (~mask)) | (val & mask));
}

/*****************************************************************************
 *
 * @brief	Performs 32bit register read.
 *
 * @param	addr: contains the address to perform the input operation
 * @return	value read from the register.
 ******************************************************************************/
inline uint32_t cdo_reg_read32(uintptr_t addr)
{
	uint32_t val = Xil_In32(static_cast<UINTPTR>(addr));
	AEG_CDO_LOG("%s: address=0x%llx value=0x%x\n", "reg_read32",
	        static_cast<uint64_t>(addr), val);
	return val;
}

/*****************************************************************************
 *
 * @brief	Writes a single 32bit word, mirroring XAie_Write32. The absolute cdo_reg_off.
 *
 * @param	cdo_reg_off: register offset to write
 * @param	val: value to write
 * @return	XAIE_OK on success
 ******************************************************************************/
inline AieRC cdo_reg_write32(uint64_t cdo_reg_off,
		uint32_t val)
{
	uintptr_t addr = static_cast<uintptr_t>(cdo_reg_off);
	AEG_CDO_LOG("%s: reg=0x%llx address=0x%llx value=0x%x\n", "cdo_reg_write32",
	        static_cast<uint64_t>(cdo_reg_off), static_cast<uint64_t>(addr), val);
	cdo_reg_write32_raw(addr, val);
	return XAIE_OK;
}

/*****************************************************************************
 *
 * @brief	Performs a 32bit register mask write, mirroring XAie_MaskWrite32.
 *		The absolute target address is cdo_reg_off.
 *
 * @param	cdo_reg_off: register offset to write
 * @param	mask: mask value
 * @param	val: value to write to the register
 * @return	XAIE_OK on success
 ******************************************************************************/
inline AieRC cdo_reg_maskwrite32( uint64_t cdo_reg_off,
		uint32_t mask, uint32_t val)
{
	uintptr_t addr = static_cast<uintptr_t>(cdo_reg_off);
	AEG_CDO_LOG("%s: reg=0x%llx address=0x%llx mask=0x%x value=0x%x\n", "cdo_reg_maskwrite32",
	        static_cast<uint64_t>(cdo_reg_off), static_cast<uint64_t>(addr), mask, val);
	cdo_reg_maskwrite32_raw(addr, mask, val);
	return XAIE_OK;
}

/*****************************************************************************
 *
 * @brief	Writes a contiguous block of 32bit words to consecutive
 *		registers, mirroring XAie_BlockWrite32. The absolute target
 *		address is cdo_reg_off; successive words are placed
 *		at 4-byte increments.
 *
 * @param	cdo_reg_off: register offset of the first word
 * @param	src: pointer to the source words
 * @param	count: number of 32bit words to write
 * @return	XAIE_OK on success
 ******************************************************************************/
inline AieRC cdo_reg_blockwrite32(uint64_t cdo_reg_off,
		const uint32_t *src, uint32_t count)
{
	uintptr_t base = static_cast<uintptr_t>(cdo_reg_off);
	AEG_CDO_LOG("%s: reg=0x%llx address=0x%llx count=%u\n", "cdo_reg_blockwrite32",
	        static_cast<uint64_t>(cdo_reg_off), static_cast<uint64_t>(base), count);
	for (uint32_t i = 0U; i < count; i++) {
		cdo_reg_write32_raw(base + static_cast<uintptr_t>(i * 4U), src[i]);
	}
	return XAIE_OK;
}

/*****************************************************************************
 *
 * @brief	Polls a 32bit register until (read & mask) == value or the
 *		timeout elapses, mirroring XAie_MaskPoll. The polling cadence
 *		matches the aie-rt baremetal backend (200us minimum granularity).
 *
 * @param	cdo_reg_off: register offset to poll
 * @param	mask: bits of the register to compare
 * @param	value: expected value after masking
 * @param	timeout_us: maximum time to wait, in microseconds
 * @return	XAIE_OK if the condition was met, XAIE_ERR on timeout
 ******************************************************************************/
inline AieRC cdo_reg_maskpoll(uint64_t cdo_reg_off,
		uint32_t mask, uint32_t value, uint32_t timeout_us)
{
	const uint32_t min_timeout_us = 200U;
	uint32_t count = (timeout_us + min_timeout_us - 1U) / min_timeout_us;
	uintptr_t addr = static_cast<uintptr_t>(cdo_reg_off);

	AEG_CDO_LOG("%s: reg=0x%llx address=0x%llx mask=0x%x value=0x%x "
	        "timeout_us=%u\n", "cdo_reg_maskpoll", static_cast<uint64_t>(cdo_reg_off),
	        static_cast<uint64_t>(addr), mask, value, timeout_us);

	while (count > 0U) {
		if ((cdo_reg_read32(addr) & mask) == value) {
			return XAIE_OK;
		}
		static_cast<void>(usleep(static_cast<useconds_t>(min_timeout_us)));
		count--;
	}

	/* Final check after the timed-out loop. */
	if ((cdo_reg_read32(addr) & mask) == value) {
		return XAIE_OK;
	}
	return XAIE_ERR;
}

} // namespace cdo
} // namespace aeg

#endif // AEG_CDO_H
