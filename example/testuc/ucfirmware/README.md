<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

1. Set up the vitis and vsp environment for build

```
source ./env.sh
```

2. build the UC firmware

```
make
```

3. check the uc firmware/elf by using readelf

```
readelf -S ./output/*.elf
```
