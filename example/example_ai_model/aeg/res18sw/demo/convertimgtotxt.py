# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
import numpy as np
import sys
from PIL import Image

# Function 1: Convert an image to 224x224 int8 and save as raw file
# Model expects NORMALIZED int8 values scaled by 32 (xscale=5)
# Reference resnet18.mat shows L1 input: dtype=int8, range=[-64,84], mean=-18
def image_to_int8_txt(image_path, output_txt_path):
    # Step 1: Load and resize the image
    image = Image.open(image_path).convert("RGB")  # Ensure RGB format
    image = image.resize((224, 224))  # Resize to ResNet18 standard size
    image_uint8 = np.array(image, dtype=np.uint8)  # Raw pixel values (0-255)

    # Step 2: Normalize with ImageNet mean/std
    image_float32 = image_uint8.astype(np.float32) / 255.0
    mean = np.array([0.485, 0.456, 0.406], dtype=np.float32).reshape(1, 1, 3)
    std = np.array([0.229, 0.224, 0.225], dtype=np.float32).reshape(1, 1, 3)
    image_normalized = (image_float32 - mean) / std  # Range roughly [-2.x, +2.x]

    # Step 3: Scale by 32 (xscale=5 means multiply by 2^5=32) and convert to int8
    # Reference shows input range [-64, 84], so clip to int8 range [-128, 127]
    scale = 32.0
    image_scaled = image_normalized * scale
    image_int8 = np.clip(np.round(image_scaled), -128, 127).astype(np.int8)

    # Step 4: Write as little-endian uint32 with padding
    # Memory layout: [R, G, B, 0x00] stored as little-endian
    output_txt_raw = output_txt_path + ".raw"
    with open(output_txt_path, 'w') as f, open(output_txt_raw, 'wb') as fraw:
        for i in range(224):
            for j in range(224):
                # Convert int8 to unsigned byte representation
                r = int(image_int8[i, j, 0]) & 0xFF
                g = int(image_int8[i, j, 1]) & 0xFF
                b = int(image_int8[i, j, 2]) & 0xFF
                hex_line = f"{b:02x}{g:02x}{r:02x}"  # Format as BGR hex for txt
                f.write(hex_line + '\n')
                # Write raw into file as little-endian uint32
                raw = int((b << 16) | (g << 8) | r)
                binary_value = raw.to_bytes(4, byteorder='little')
                fraw.write(binary_value)

    zero_point = 0
    return image_int8, scale, zero_point

# Function 2: Restore RGB image from int8 txt file with dequantization
def txt_to_rgb_image(txt_path, output_image_path, scale, zero_point, height=224, width=224):
    # Step 1: Read txt file and parse BGR hex values
    with open(txt_path, 'r') as f:
        lines = f.readlines()
    
    hex_values = [line.strip() for line in lines]
    expected_size = height * width
    assert len(hex_values) == expected_size, f"Line count ({len(hex_values)}) does not match image size ({expected_size})"

    # Parse hex values into int8 array
    data_int8 = np.zeros((height, width, 3), dtype=np.int8)
    for i in range(height):
        for j in range(width):
            hex_str = hex_values[i * width + j].zfill(6)  # Pad to 6 digits with zeros if needed
            b = int(hex_str[0:2], 16)  # Blue (first two digits)
            g = int(hex_str[2:4], 16)  # Green (middle two digits)
            r = int(hex_str[4:6], 16)  # Red (last two digits)
            data_int8[i, j, 0] = r if r <= 127 else r - 256  # Convert to int8 for Red
            data_int8[i, j, 1] = g if g <= 127 else g - 256  # Convert to int8 for Green
            data_int8[i, j, 2] = b if b <= 127 else b - 256  # Convert to int8 for Blue

    # Step 2: Dequantize int8 to float32
    image_float32 = (data_int8.astype(np.float32) - zero_point) * scale

    # Step 3: Denormalize with ResNet18 parameters
    mean = np.array([0.485, 0.456, 0.406], dtype=np.float32).reshape(1, 1, 3)
    std = np.array([0.229, 0.224, 0.225], dtype=np.float32).reshape(1, 1, 3)
    image_restored = image_float32 * std + mean  # Restore to [0, 1] range

    # Step 4: Convert float32 to uint8 (RGB)
    image_rgb = np.clip(image_restored * 255.0, 0, 255).astype(np.uint8)

    # Step 5: Save as RGB image
    output_image = Image.fromarray(image_rgb)
    output_image.save(output_image_path)
    return image_rgb

# 功能2：从int8 txt文件反量化并恢复RGB图像
def txt_to_rgb_image2(txt_path, output_image_path, scale, zero_point, height=224, width=224):
    # 步骤1：读取txt文件并解析BGR十六进制
    with open(txt_path, 'r') as f:
        lines = f.readlines()
    
    hex_values = [line.strip() for line in lines]
    expected_size = height * width
    assert len(hex_values) == expected_size, f"数据行数 ({len(hex_values)}) 不匹配图像尺寸 ({expected_size})"

    # 解析为int8
    data_int8 = np.zeros((height, width, 3), dtype=np.int8)
    for i in range(height):
        for j in range(width):
            hex_str = hex_values[i * width + j].zfill(6)  # 不足6位填充0
            b = int(hex_str[0:2], 16)  # Blue
            g = int(hex_str[2:4], 16)  # Green
            r = int(hex_str[4:6], 16)  # Red
            # 转换为int8 (-128到127)
            data_int8[i, j, 0] = r if r <= 127 else r - 256
            data_int8[i, j, 1] = g if g <= 127 else g - 256
            data_int8[i, j, 2] = b if b <= 127 else b - 256

    # 步骤2：反量化 int8 -> float32
    image_float32 = (data_int8.astype(np.float32) - zero_point) * scale

    # 步骤3：反标准化（ResNet18参数）
    mean = np.array([0.485, 0.456, 0.406], dtype=np.float32).reshape(1, 1, 3)
    std = np.array([0.229, 0.224, 0.225], dtype=np.float32).reshape(1, 1, 3)
    image_restored = image_float32 * std + mean

    # 步骤4：float32 -> uint8（RGB）
    image_rgb = np.clip(image_restored * 255.0, 0, 255).astype(np.uint8)

    # 步骤5：保存为RGB图像
    output_image = Image.fromarray(image_rgb)
    output_image.save(output_image_path)

    return image_rgb

# Function 3: Generate RGB image directly from int8 txt file (no dequantization)
def txt_to_rgb_direct(txt_path, output_image_path, height=224, width=224):
    # Step 1: Read txt file and parse BGR hex values
    with open(txt_path, 'r') as f:
        lines = f.readlines()
    
    hex_values = [line.strip() for line in lines]
    expected_size = height * width
    assert len(hex_values) == expected_size, f"Line count ({len(hex_values)}) does not match image size ({expected_size})"

    # Step 2: Parse hex values directly into uint8 array (no dequantization)
    image_rgb = np.zeros((height, width, 3), dtype=np.uint8)
    for i in range(height):
        for j in range(width):
            hex_str = hex_values[i * width + j].zfill(6)  # Pad to 6 digits with zeros if needed
            b = int(hex_str[0:2], 16)  # Blue (first two digits)
            g = int(hex_str[2:4], 16)  # Green (middle two digits)
            r = int(hex_str[4:6], 16)  # Red (last two digits)
            image_rgb[i, j, 0] = r  # Assign Red
            image_rgb[i, j, 1] = g  # Assign Green
            image_rgb[i, j, 2] = b  # Assign Blue

    # Step 3: Save as RGB image
    output_image = Image.fromarray(image_rgb)
    output_image.save(output_image_path)
    return image_rgb

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python script.py input_path ")
        sys.exit(1)
    
    image_path = sys.argv[1]
    txt_path = "ifm32.txt"

    # Convert image to int8 and save to txt
    image_int8, scale, zero_point = image_to_int8_txt(image_path, txt_path)
    print("Quantization parameters: scale =", scale, "zero_point =", zero_point)
    print("Quantized int8 sample value:", image_int8[0, 0, :])
