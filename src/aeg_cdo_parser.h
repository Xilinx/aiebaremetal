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
 * @file aeg_cdo_parser.h
 *
 * Self-contained parser for in-memory CDO (Configuration Data Object) blobs.
 *
 * A CDO blob is a self-describing sequence of AIE configuration commands
 * (32-bit words, CDOv2 format). `parse_and_apply()` validates the blob header,
 * decodes each command and applies it to the device through the XAie driver
 * (`XAie_Write32` / `XAie_MaskWrite32` / `XAie_BlockWrite32`).
 *
 * The decoding logic is adapted from the device loader `XCdo_LoadCdo`
 * (thirdparty/aie-pdi-transform/lib/cdo_cmd.c). All format constants are
 * inlined in the implementation so this translation unit has no dependency on
 * the aie-pdi-transform headers and links into both the hardware static
 * library and the simulator shared object.
 */

#ifndef AEG_CDO_PARSER_H
#define AEG_CDO_PARSER_H

#include "aeg_baremetal_api.h" // for aeg_err

#include <xaiengine.h> // for XAie_DevInst

#include <cstddef>
#include <cstdint>

namespace aeg {
namespace cdo {

/**
 * Parse an in-memory CDO blob and apply its configuration commands.
 *
 * @param data Pointer to the CDO blob (an array of 32-bit little-endian words).
 * @param len  Length of the blob in bytes.
 * @param dev  Device instance used to issue the register writes. CDO commands
 *             carry absolute AIE addresses; the register offset passed to the
 *             XAie driver is computed as `CdoAddr & ~cdo_base`.
 * @param cdo_base Original CDO base address to strip from each AIE address
 *             (0 on APU for absolute addressing; the original AIE base on RPU).
 * @return aeg_err::ok on success; aeg_err::user_error on a malformed blob
 *         (bad length, bad magic, bad checksum, out-of-range command, or an
 *         unsupported opcode); aeg_err::aie_driver_error if a driver write
 *         fails.
 */
aeg_err parse_and_apply(const void *data, std::size_t len, XAie_DevInst *dev,
                        std::uint64_t cdo_base);

} // namespace cdo
} // namespace aeg

#endif // AEG_CDO_PARSER_H
