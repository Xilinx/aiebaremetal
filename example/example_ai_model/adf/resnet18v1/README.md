<!--- Copyright 2022-2023 Advanced Micro Devices, Inc. All Rights Reserved. --->

# ResNet18v1 Implementation on 4x7 Overlay

Please set the environment variables as recommended here: https://git......./AIELibs/mllib/tree/dev/L2/test
Recommended combination of build and OS version:
1. *BUILD: HEAD_INT_daily_latest OS: CentOS 8+*


## Overlay Architecture Basics
FlexML implements configurable overlay architectures as a rectangular grid of MxN AIE2 cores with M (number of rows) WLP stream (weights + Layer Parameters, broadcast horizontally) and N (number of columns) IFM/OFM streams (broadcast vertically) as shown in the diagram below. Overlay size of 4x7 is used for ResNet18v1 implementation. IFM and WLP tensors are transferred from external DDR to mem-tile shared buffers (or L2 memory). Data from L2 memory then sent to individual AIE2 cores in small chunks called sub-volumes (SV) of 2-8kB (typical). Each core then processes (conv2d, maxpool etc) SVs and produces OFM SV for sending back to L2 in another shared buffer (see the figure below). This process continues for num_iter times consuming the entire IFM tensor. This way it completes one full layer processing. In the next layer, OFM buffer becomes the source and IFM buffer becomes the destination and the same process continues layer after layer. It's necessary to reconfigure the mem-tile DMAs for IFM, OFM and WLP for the next layer as SV dimensions change. Note, core DMAs don't need reconfiguration (by external controller) due to the following:
1. _Core-tile s2mm DMAs for IFM will be configured by kernel code and WLP will be in FoT mode (finish on TLAST)_
1. _OFM mm2s DMA will be reconfigured by kernel code by accessing local register space_

<!-- <p align="center">
<img src="https://git......./AIELibs/mllib/blob/dev/internal/demo/resnet18v1/ResNet18v1.PNG" width="890" height="500">
</p>

<p align="center">
<img src="https://git......./AIELibs/mllib/blob/dev/internal/demo/resnet18v1/4x7overlay.PNG" width="890" height="500">
</p> -->

## Model
ResNet18 is the smallest version of Residual Networks with 18 convolution, fully-connected and SoftMax layers used for image classification. This network is trained on ImageNet dataset with 1000 classes. Pretrained ONNX network used for this design implementation can be downloaded from model zoo:
https://github.com/onnx/models/blob/main/vision/classification/resnet/model/resnet18-v1-7.onnx


## Design Status

Work ongoing to bring up the residual blocks.

- Layers L1 to L18 are coded up, please see the table below for the sub-volume dimensions and other details. Layer 17 is a fused layer with conv2d+eltwise-add+GAP (global avgpool), while layer 18 is the fully connected (FC) implemented using conv2d. 
- Layers 1-18 are implemented on 4x7 (4-row, 7-col) overlay and SoftMax on another 1x1 overlay. 
- Layers L1-L18+SoftMax is now a single design, core configuration is sent over AXI-MM interface by PL controller to keep overall program memory less than 16 kB. 



<!-- <p align="center">
<img src="https://git......./AIELibs/mllib/blob/dev/internal/demo/resnet18v1/ResNet18v1svs.PNG" width="1000" height="642">
</p> -->


### Compile
\>\> make compile  

#### Extra BW for WTS transfer from DDR to L2
Use of 2 DMA channels in shim DMA for WTS transfer from DDR to L2 is now enabled by default. This speeds up WTS transfer from DDR to L2. This can be reverted to a single DMA channel (used earlier) by compiling the design as specified below

\>\> make compile NUM_DMA_CH=1  

### Simulate the design after compiling
\>\> make sim

### Check outputs after simulation
\>\> diff data/ofm32.txt data/ofm32_ref.txt 

