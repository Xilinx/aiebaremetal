// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include <cstring>
#include <iostream>
#include <stdio.h>

#include <cert_elf_loader.h>

void usage(const char *progname) {
    printf("usage: %s --elf <filename>\n", progname);
    exit(-EINVAL);
}

int main(int argc, char **argv) {
    const char *filename;
    cert_elf_loader cel;
    int ret;

    if (argc == 1) {
        usage(argv[0]);
    }
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--elf")) {
            i++;
            if (i >= argc) {
                usage(argv[0]);
                return -EINVAL;
            }
            filename = argv[i];
            continue;
        } else {
            usage(argv[0]);
            return 0;
        }
    }

    ret = cel.begin(filename);
    if (ret) {
        printf("cert elf loader failed: %d\n", ret);
        return ret;
    }

    ret = cel.parse();
    if (ret) {
        printf("cert elf parsing failed: %d\n", ret);
        return ret;
    }

    return 0;
}
