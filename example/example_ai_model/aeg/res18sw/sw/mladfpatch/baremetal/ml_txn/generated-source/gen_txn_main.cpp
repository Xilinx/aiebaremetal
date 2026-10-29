// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "gen_txn.h"
#include <fstream>
#include <iostream>


void WriteTxn(std::string TxnBinFileName, bool dump)
{
    uint8_t *Ptr = GenerateSerializedTransaction();
    if (dump)
        PrintSerializedTransaction(Ptr);
    WriteSerializedTransactionToBin(Ptr, TxnBinFileName);

}

void ReadTxn(std::string TxnBinFileName, bool dump)
{
    uint8_t *ReadPtr = ReadSerializedTransactionFromBin(TxnBinFileName);

    PrintSerializedTransaction(ReadPtr);

}

int main(int argc, char** argv)
{
    bool writeTxn = false;
    bool readTxn = false;
    bool dump = false;
    std::string fileName;

    for (int i = 1; i <argc; i++)
    {
      if (std::string(argv[i]) == "--help") {
        std::cout << "\n genTxnMain.out --write-txn <txn-file-name> \n";
        std::cout << "\n genTxnMain.out --read-txn  <txn-file-name> \n";
        return 0;
      } else if (std::string(argv[i]) == "--write-txn") {
        writeTxn = true;
        if (i == argc -1) {
          std::cout << "--write-txn file name is not specified " << std::endl;
          return EXIT_FAILURE;
        }
        fileName = std::string(argv[++i]);
      } else if (std::string(argv[i]) == "--read-txn") {
        readTxn = true;
        if (i == argc -1) {
          std::cout << "--read-txn file name is not specified " << std::endl;
          return EXIT_FAILURE;
        }
        fileName = std::string(argv[++i]);
      } else if (std::string(argv[i]) == "--dump") {
        dump = true;
      } else {
        std::cout << argv[i] << " unknown option." << std::endl;
        return EXIT_FAILURE;
      }
    }

    if (writeTxn)
        WriteTxn(fileName, dump);
    else if (readTxn)
        ReadTxn(fileName, dump);

}
    