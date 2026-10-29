#!/usr/bin/env python3
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
import sys
from PIL import Image

# Determine the appropriate resampling method for resizing
try:
    resample_method = Image.Resampling.LANCZOS
except AttributeError:
    resample_method = Image.ANTIALIAS

def image_to_hex_txt(input_image_path, output_txt_path):
    """
    Convert an image to 224x224 with 256 colors, then output each pixel's RGB value
    as a 6-digit hexadecimal string. Each line corresponds to one pixel (in HWC order).
    """
    # Open the image file
    img = Image.open(input_image_path)
    # Resize the image to 224x224 using the determined resampling method
    img = img.resize((224, 224), resample=resample_method)
    # Convert the image to 256 colors (P mode) using an adaptive palette
    img = img.convert('P', palette=Image.ADAPTIVE, colors=256)
    
    # Retrieve the palette (a flat list: [R0, G0, B0, R1, G1, B1, ..., R255, G255, B255])
    palette = img.getpalette()
    # Get the pixel data (each pixel is represented by a palette index)
    indices = list(img.getdata())
    
    with open(output_txt_path, 'w') as f:
        for idx in indices:
            # Get the corresponding RGB value from the palette using the index
            r, g, b = palette[3 * idx : 3 * idx + 3]
            # Format the RGB value as a 6-digit hexadecimal string (each channel uses 2 digits)
            hex_str = "{:02X}{:02X}{:02X}".format(r, g, b)
            f.write(hex_str + "\n")
    
    print("Conversion complete. Hex data saved to:", output_txt_path)

def hex_txt_to_image(input_txt_path, output_image_path):
    """
    Convert a hex text file back into a 224x224 image with 256 colors.
    Each line of the file must contain a 6-digit hex string representing one pixel's RGB value.
    """
    pixels = []
    with open(input_txt_path, 'r') as f:
        for line in f:
            hex_str = line.strip()
            if len(hex_str) != 6:
                raise ValueError("Each line must be a 6-digit hexadecimal string. Problematic line: " + hex_str)
            # Convert the hex string to an (R, G, B) tuple
            r = int(hex_str[0:2], 16)
            g = int(hex_str[2:4], 16)
            b = int(hex_str[4:6], 16)
            pixels.append((r, g, b))
    
    # Check that we have exactly 224 * 224 pixels
    if len(pixels) != 224 * 224:
        raise ValueError("Pixel count mismatch. Expected {} pixels, got {}.".format(224*224, len(pixels)))
    
    # Create a new RGB image of size 224x224
    img_rgb = Image.new("RGB", (224, 224))
    # Populate the image with the pixel data (in row-major order)
    img_rgb.putdata(pixels)
    
    # Convert the RGB image to 256 colors (P mode) using an adaptive palette
    img_p = img_rgb.convert("P", palette=Image.ADAPTIVE, colors=256)
    img_p.save(output_image_path, format="PNG")
    print("Image saved as:", output_image_path)

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python script.py [convert|revert] input_path output_path")
        sys.exit(1)
    
    mode = sys.argv[1].lower()
    input_path = sys.argv[2]
    output_path = sys.argv[3]
    
    if mode == "convert":
        image_to_hex_txt(input_path, output_path)
    elif mode == "revert":
        hex_txt_to_image(input_path, output_path)
    else:
        print("Unknown mode. Use 'convert' to convert an image to hex text, or 'revert' to convert hex text to image.")
        sys.exit(1)

