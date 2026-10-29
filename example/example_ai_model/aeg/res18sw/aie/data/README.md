<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# Data

## ifm32.txt

this is the input image, it get converted from a 224 * 224 RBB image, the original image is a int32 or int8,
to apply the resnet18 model, we need first convert the int32/int8 into float32, and do normalized

the reason we do this is because the model trained is go through float32 that is because model need to find
the grandient and do differentiation.

so when inference we need to do same convert, from int into float


but that not finish, because we also did a quantize, to convert the float32 into int8 for acceleration, hence
we need to apply another quantize logic to covert from float 32 into int8


the logic is go through zero_point and scale to map the float32 into int8

finnally we covert a image from int --> float32 --> int8, that is ifm32.txt, 


another information is that ,the ifm32.txt, each line represent 4 byte, RGB(0) is 0, then RGB, that means only
have 3 bytes

## wgts.txt

this is the 8 bits weight , but the bias is 16 bits, based on the kernel analysis

## int8.txt

this is a tigger cat data for test purpose, after run ./convertimgtotxt.py , this file will be convert into a image


## ./convertimgtotxt.py

this file demostrate how to convert an image into the model understandable data format, the api is  "image_to_int8_txt"
by using "python3 ./convertimgtotxt.py <image>" we can convert the image into ifm32.txt, then after convert, go ../../sw
do source ./build.sh -bootgen to generate new app for test
