<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# What is this example

   This example use 8 * 11 core tile with legacy ADF to implement the conv2d

# Compile

```
source ./buildwithpatch.sh
```

# Known issue

using mulitple output merged into a single DMA bd, need to manually spit and need some patch to fix aiecompiler 
out of order issue