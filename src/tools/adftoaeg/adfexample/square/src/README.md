<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# How to convert & compile adf code into RPU

## download and set up aiebaremetal runtime path

```
source /path/to/aiebaremetal/script/settings.sh 1
```

## set up adftoaeg converter path

```
source /path/to/adftoaeg/setup.sh
```

## 3 convert&compile the adf code into rpu with one line command

```
convert.sh ./graph.adf.nsc.cpp -compile--aie_version 2 -rpu
```

# Trouble shooting

### 1. some aiebaremetal runtime lib api not found

this should related first did a APU compile then RPU, but the history data not get clean up

```
cd /path/to/aiebaremetal/
cd example
make clean
```


