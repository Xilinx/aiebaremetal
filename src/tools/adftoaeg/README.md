<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# ADF to Baremetal Runtime Conversion Tool 

## Usage Tutorial

1. first, clone the repo and navigate to this directory

```bash
# skip if you cloned the repo already
git clone --recursive <aiebaremetal_repo_url>

cd aiebaremetal/src/tools/adftoaeg
```


2. source the settings script

```bash
source ../../../script/settings.sh 1
```

3. Run the convert.sh script to do the conversion

Try the following example:

```bash
# usage format: ./convert.sh  </path/to/target> -out <newfile>
convert.sh ./tests/test_general/graph.cpp -out ./graph_new.cpp

# view adf file
vim ./tests/test_general/graph.cpp

# view generated aeg file
vim ./graph_new.cpp
```

Additionally, if the code you are converting requires extra include paths, you can add them here with the `-I` option.

```bash
./convert.sh </path/to/target> -I/path/to/include/dir1 -I/path/to/include/dir2 -out <newfile>
```

## Build Instructions

1. Rebuild adftoaeg (optional, uses prebuilt version by default)

```bash
mkdir build
cd build
cmake ../
make
cd ..
```
