/**<!--
 * Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
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
#ifndef MAIN_RTS_IO_H
#define MAIN_RTS_IO_H

#include <stdint.h>
typedef unsigned int uint;

void ess_Write32(uint64_t Addr, uint Data);
uint ess_Read32(uint64_t Addr);

void ess_Write128(uint64_t Addr, uint32_t* Data);
void ess_Read128(uint64_t Addr, uint32_t* Data);

void ess_WriteCmd(unsigned char Command, unsigned char ColId, unsigned char RowId, unsigned int CmdWd0, unsigned int CmdWd1, unsigned char *CmdStr);

void ess_NpiWrite32(uint64_t Addr, uint Data);
uint ess_NpiRead32(uint64_t Addr);

void ess_WriteGM(uint64_t addr, const void* data, uint64_t size);
void ess_ReadGM(uint64_t addr, void* data, uint64_t size);

void start_plios();

#endif
