// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <string>
#include <iostream>

void WriteSerializedTransactionToBin(uint8_t* ptr, std::string TxnBinFileName);
uint8_t* ReadSerializedTransactionFromBin(std::string TxnBinFileName);
void PrintSerializedTransaction(uint8_t *Ptr);
uint8_t* GenerateSerializedTransaction();
void executeRuntimeControl();

extern "C"
{
 #include <xaiengine.h>
}
extern XAie_DevInst DevInst;

