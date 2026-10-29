// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef AEG_METADATA_MAIN_H
#define AEG_METADATA_MAIN_H

#if defined(__AEG_METADATA_ONLY__) && !defined(__AIENGINE__)
int main(int, char **) { return 0; }
#endif

#endif /* AEG_METADATA_MAIN_H */
