#!/usr/bin/env python3
"""
Batch process meme images for ESP32 OLED display

This script processes all images in the memes directory and generates
bitmap arrays for each one.
"""

import os
import sys
from pathlib import Path

# Add the current directory to path so we can import our converter
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

try:
    from image_to_bitmap import process_image
except ImportError:
    print("❌ Error: Could not import image_to_bitmap module")
    print("Make sure image_to_bitmap.py is in the same directory")
    sys.exit(1)

def batch_process_memes():
    """
    Process all images in the images/memes directory
    """
    memes_dir = Path("images/memes")
    
    if not memes_dir.exists():
        print(f"❌ Error: Directory {memes_dir} does not exist")
        return
    
    # Supported image extensions
    image_extensions = {'.png', '.jpg', '.jpeg', '.bmp', '.gif', '.tiff'}
    
    # Find all image files
    image_files = []
    for ext in image_extensions:
        image_files.extend(memes_dir.glob(f"*{ext}"))
        image_files.extend(memes_dir.glob(f"*{ext.upper()}"))
    
    if not image_files:
        print(f"❌ No image files found in {memes_dir}")
        return
    
    print(f"Found {len(image_files)} image(s) to process:")
    for img in image_files:
        print(f"  - {img.name}")
    
    print("\n" + "="*60)
    
    # Process each image
    for img_path in image_files:
        # Generate variable name from filename
        var_name = img_path.stem.lower().replace(" ", "_").replace("-", "_")
        var_name = ''.join(c for c in var_name if c.isalnum() or c == '_')
        
        try:
            print(f"\n🔄 Processing: {img_path.name}")
            process_image(str(img_path), var_name)
            print(f"✅ Completed: {var_name}")
            
        except Exception as e:
            print(f"❌ Failed to process {img_path.name}: {e}")
    
    print("\n" + "="*60)
    print("🎉 Batch processing complete!")
    print("\nGenerated files:")
    print("  - *_preview.png - Preview of processed images")  
    print("  - *_bitmap.txt - C bitmap code for ESP32")

if __name__ == "__main__":
    batch_process_memes()
