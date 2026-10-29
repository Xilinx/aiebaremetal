# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
import numpy as np
import sys
from PIL import Image

# Function 1: Convert an image to 224x224 int8 and save as BGR-formatted txt file
def image_to_int8_txt(image_path, output_txt_path):
    # Step 1: Load and resize the image
    image = Image.open(image_path).convert("RGB")  # Ensure RGB format
    image = image.resize((224, 224))  # Resize to ResNet18 standard size
    image_uint8 = np.array(image, dtype=np.uint8)  # Convert to uint8 array (RGB)

    # Step 2: Convert uint8 to float32 and scale to [0, 1]
    image_float32 = image_uint8.astype(np.float32) / 255.0

    # Step 3: Normalize with ResNet18 ImageNet parameters
    mean = np.array([0.485, 0.456, 0.406], dtype=np.float32).reshape(1, 1, 3)
    std = np.array([0.229, 0.224, 0.225], dtype=np.float32).reshape(1, 1, 3)
    image_normalized = (image_float32 - mean) / std  # Apply normalization

    # Step 4: Quantize float32 to int8
    data_min = image_normalized.min()
    data_max = image_normalized.max()
    scale = (data_max - data_min) / 255  # Scale to fit int8 range (-128 to 127)
    #this is experience value, we do not knwo what zero_point the mlib resnet
    #after multiple test, we found zero_point = 0 is the correct one
    zero_point = 0#np.round(-data_min / scale).astype(np.int8)  # Adjust zero point
    image_int8 = np.round(image_normalized / scale + zero_point).astype(np.int8)

    # Step 5: Convert to BGR and save to txt file
    output_txt_raw = output_txt_path + ".raw"
    with open(output_txt_path, 'w') as f, open(output_txt_raw, 'wb') as fraw:
        for i in range(224):
            for j in range(224):
                b = image_int8[i, j, 2] & 0xFF  # Blue channel (convert to uint8 representation)
                g = image_int8[i, j, 1] & 0xFF  # Green channel
                r = image_int8[i, j, 0] & 0xFF  # Red channel
                hex_line = f"{b:02x}{g:02x}{r:02x}"  # Format as BGR (e.g., eeccdd)
                f.write(hex_line + '\n')
                #write raw into file
                raw = int((b << 16) | (g << 8) | r)
                binary_value = raw.to_bytes(4, byteorder='big')
                fraw.write(binary_value)

    
    return image_int8, scale, zero_point  # Return int8 data and quantization params

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
    # Test code
    #image_path = "/home/huaj/pepper.png"  # Replace with your image path
    image_path = sys.argv[1]
    txt_path = "ifm32.txt"
    t2_path = "int8.txt"
    restored_image_path = "restored_rgb_image.jpg"
    direct_image_path = "direct_rgb_image.jpg"

    # Convert image to int8 and save to txt
    image_int8, scale, zero_point = image_to_int8_txt(image_path, txt_path)
    print("Quantization parameters: scale =", scale, "zero_point =", zero_point)
    print("Quantized int8 sample value:", image_int8[0, 0, :])

    # Restore RGB image with dequantization
    restored_image = txt_to_rgb_image2(t2_path, restored_image_path, scale, zero_point)
    print("Restored RGB uint8 sample value:", restored_image[0, 0, :])

    # Generate RGB image directly (no dequantization)
    direct_image = txt_to_rgb_direct(t2_path, direct_image_path)
    print("Direct RGB uint8 sample value:", direct_image[0, 0, :])

    # Display results
    #Image.open(image_path).show(title="Original Image")
    #Image.open(restored_image_path).show(title="Restored Image (Dequantized)")
    #Image.open(direct_image_path).show(title="Direct Image (No Dequantization)")
