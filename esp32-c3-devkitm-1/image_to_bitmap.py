#!/usr/bin/env python3
"""
Image to ESP32 OLED Bitmap Converter

This script resizes images to fit within a 2:1 aspect ratio, crops them to 128x64,
converts them to monochrome, and generates C bitmap arrays for ESP32 OLED displays.

Usage:
    python3 image_to_bitmap.py input_image.png [output_name]

Example:
    python3 image_to_bitmap.py images/memes/watcat.png watcat
"""

import sys
import os
from PIL import Image, ImageOps
import argparse

def resize_to_fit_aspect_ratio(image, target_width, target_height):
    """
    Resize image to fit within target dimensions while maintaining aspect ratio.
    """
    target_ratio = target_width / target_height
    img_ratio = image.width / image.height
    
    if img_ratio > target_ratio:
        # Image is wider than target ratio, fit by width
        new_width = target_width
        new_height = int(target_width / img_ratio)
    else:
        # Image is taller than target ratio, fit by height
        new_height = target_height
        new_width = int(target_height * img_ratio)
    
    return image.resize((new_width, new_height), Image.Resampling.LANCZOS)

def crop_to_center(image, target_width, target_height):
    """
    Crop image to target dimensions from the center.
    """
    left = (image.width - target_width) // 2
    top = (image.height - target_height) // 2
    right = left + target_width
    bottom = top + target_height
    
    return image.crop((left, top, right, bottom))

def convert_to_monochrome(image, threshold=128):
    """
    Convert image to monochrome (1-bit) using a threshold.
    """
    # Convert to grayscale first
    gray_image = ImageOps.grayscale(image)
    
    # Apply threshold to create monochrome
    mono_image = gray_image.point(lambda x: 255 if x > threshold else 0, mode='1')
    
    return mono_image

def image_to_bitmap_array(image, variable_name):
    """
    Convert a monochrome PIL Image to a C bitmap array.
    """
    width, height = image.size
    
    # Convert image to bytes
    pixels = list(image.getdata())
    
    # Group pixels into bytes (8 pixels per byte)
    bitmap_bytes = []
    for y in range(height):
        for x in range(0, width, 8):
            byte_value = 0
            for bit in range(8):
                if x + bit < width:
                    pixel_x = x + bit
                    pixel_y = y
                    # Get pixel value (0 or 255)
                    pixel_value = pixels[pixel_y * width + pixel_x]
                    # If pixel is white (255), set the bit
                    if pixel_value > 0:
                        byte_value |= (1 << (7 - bit))
            bitmap_bytes.append(byte_value)
    
    # Generate C array code
    array_size = len(bitmap_bytes)
    bytes_per_line = 16
    
    code_lines = [
        f"const unsigned char {variable_name}_bitmap[] PROGMEM = {{",
    ]
    
    for i in range(0, array_size, bytes_per_line):
        line_bytes = bitmap_bytes[i:i + bytes_per_line]
        hex_values = [f"0x{b:02X}" for b in line_bytes]
        
        if i + bytes_per_line < array_size:
            code_lines.append("    " + ", ".join(hex_values) + ",")
        else:
            code_lines.append("    " + ", ".join(hex_values))
    
    code_lines.extend([
        "};",
        f"const int {variable_name}_width = {width};",
        f"const int {variable_name}_height = {height};"
    ])
    
    return "\n".join(code_lines)

def process_image(input_path, output_name=None, target_width=128, target_height=64, threshold=128):
    """
    Process an image: resize, crop, convert to monochrome, and generate bitmap code.
    """
    if not os.path.exists(input_path):
        raise FileNotFoundError(f"Input file not found: {input_path}")
    
    # Generate output name if not provided
    if output_name is None:
        base_name = os.path.splitext(os.path.basename(input_path))[0]
        output_name = base_name.lower().replace(" ", "_").replace("-", "_")
    
    print(f"Processing: {input_path}")
    print(f"Output variable name: {output_name}")
    
    # Load image
    try:
        image = Image.open(input_path)
        print(f"Original size: {image.width}x{image.height}")
    except Exception as e:
        raise Exception(f"Failed to open image: {e}")
    
    # Convert to RGB if necessary (to handle RGBA, P mode, etc.)
    if image.mode != 'RGB':
        image = image.convert('RGB')
    
    # Step 1: Resize to fit within 2:1 aspect ratio
    resized_image = resize_to_fit_aspect_ratio(image, target_width, target_height)
    print(f"After resize to fit: {resized_image.width}x{resized_image.height}")
    
    # Step 2: If image is smaller than target, pad it; if larger, crop it
    if resized_image.width < target_width or resized_image.height < target_height:
        # Pad with white background
        padded_image = Image.new('RGB', (target_width, target_height), 'white')
        paste_x = (target_width - resized_image.width) // 2
        paste_y = (target_height - resized_image.height) // 2
        padded_image.paste(resized_image, (paste_x, paste_y))
        final_image = padded_image
    else:
        # Crop to center
        final_image = crop_to_center(resized_image, target_width, target_height)
    
    print(f"Final size: {final_image.width}x{final_image.height}")
    
    # Step 3: Convert to monochrome
    mono_image = convert_to_monochrome(final_image, threshold)
    
    # Step 4: Generate bitmap code
    bitmap_code = image_to_bitmap_array(mono_image, output_name)
    
    # Save preview image
    preview_path = f"{output_name}_preview.png"
    mono_image.save(preview_path)
    print(f"Preview saved: {preview_path}")
    
    # Save bitmap code to file
    code_path = f"{output_name}_bitmap.txt"
    with open(code_path, 'w') as f:
        f.write(bitmap_code)
    print(f"Bitmap code saved: {code_path}")
    
    # Also print to console
    print("\n" + "="*60)
    print("C BITMAP CODE:")
    print("="*60)
    print(bitmap_code)
    print("="*60)
    
    return bitmap_code

def main():
    parser = argparse.ArgumentParser(
        description='Convert images to ESP32 OLED bitmap arrays',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 image_to_bitmap.py images/memes/watcat.png
  python3 image_to_bitmap.py images/memes/Cursed_cat.png cursed_cat
  python3 image_to_bitmap.py my_image.jpg --threshold 100
        """
    )
    
    parser.add_argument('input', help='Input image file path')
    parser.add_argument('output_name', nargs='?', help='Output variable name (optional)')
    parser.add_argument('--width', type=int, default=128, help='Target width (default: 128)')
    parser.add_argument('--height', type=int, default=64, help='Target height (default: 64)')
    parser.add_argument('--threshold', type=int, default=128, 
                       help='Monochrome threshold 0-255 (default: 128)')
    
    args = parser.parse_args()
    
    try:
        process_image(
            args.input, 
            args.output_name, 
            args.width, 
            args.height, 
            args.threshold
        )
        print(f"\n✅ Successfully processed {args.input}")
        
    except Exception as e:
        print(f"❌ Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
