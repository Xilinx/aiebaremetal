<!--- Copyright 2022-2023 Advanced Micro Devices, Inc. All Rights Reserved. --->

# ResNet18v1 Implementation using 10 instances of 4x7 Overlay


## About this Design

 - This design builds on ResNet18v1 design and extends it to 10 instances using (4x7+1)x10 = 290 cores
 - The 10-instance design is default and placement for this is fixed
 - It is possible to implement only a single instance using 4x7 array and supply co-ordinates to change placement for the single-instance design


## Environment

Please set the environment variables as recommended here: https://git......./AIELibs/mllib/tree/dev/L2/test
Recommended combination of build and OS version:
 - *BUILD: HEAD_daily_latest OS: CentOS 8+*  

## Running the Design

### Compile

 - Compile 10 instance design

   \>\> make compile  

 - Compile single instance design

   \>\> make compile NUM_INST=1

### Makefile variables to control placement of single instance
   Following makefile variables can be used to control placement of single instance design
   
   COL_OFFSET, ROW_OFFSET,MEM_ROW, SHIM_COL, SHIM_CH, SM_COL, SM_ROW


### Simulate the design after compiling
\>\> make sim

### Check outputs after simulation
\>\> diff data/ofm32.txt data/ofm32_ref.txt 
