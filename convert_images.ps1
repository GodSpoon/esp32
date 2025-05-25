# PowerShell script to convert PNG images to Arduino bitmap arrays
# This script uses Python with PIL to convert images

# First, create the Python conversion script
$pythonScript = @"
import sys
from PIL import Image
import os

def convert_to_bitmap(image_path, output_name):
    try:
        # Open and convert image
        img = Image.open(image_path)
        
        # Resize to fit OLED (128x64) while maintaining aspect ratio
        img.thumbnail((128, 64), Image.Resampling.LANCZOS)
        
        # Convert to 1-bit (black and white)
        img = img.convert('1')
        
        # Get actual size after resize
        width, height = img.size
        
        # Convert to bitmap array
        bitmap_data = []
        
        # Process image data for SSD1306 format (vertical bytes)
        for page in range((height + 7) // 8):  # 8 pages for 64 pixels height
            for col in range(width):
                byte_val = 0
                for bit in range(8):
                    y = page * 8 + bit
                    if y < height:
                        pixel = img.getpixel((col, y))
                        if pixel == 0:  # Black pixel (0 in 1-bit mode)
                            byte_val |= (1 << bit)
                bitmap_data.append(byte_val)
        
        # Generate Arduino C++ code
        array_name = f"{output_name}_bitmap"
        cpp_code = f"""
// {output_name} - {width}x{height} pixels
const unsigned char {array_name}[] PROGMEM = {{
"""
        
        # Add bitmap data
        for i, byte in enumerate(bitmap_data):
            if i % 16 == 0:
                cpp_code += "\n  "
            cpp_code += f"0x{byte:02X}"
            if i < len(bitmap_data) - 1:
                cpp_code += ", "
        
        cpp_code += f"""
}};

const int {output_name}_width = {width};
const int {output_name}_height = {height};
"""
        
        # Write to file
        output_file = f"{output_name}_bitmap.h"
        with open(output_file, 'w') as f:
            f.write(cpp_code)
        
        print(f"Converted {image_path} -> {output_file}")
        print(f"Size: {width}x{height}")
        print(f"Bitmap size: {len(bitmap_data)} bytes")
        
    except Exception as e:
        print(f"Error converting {image_path}: {e}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python convert_image.py <image_path> <output_name>")
        sys.exit(1)
    
    image_path = sys.argv[1]
    output_name = sys.argv[2]
    convert_to_bitmap(image_path, output_name)
"@

# Save Python script
$pythonScript | Out-File -FilePath "convert_image.py" -Encoding UTF8

# Check if Python is available
try {
    python --version
    Write-Host "Python found, converting images..." -ForegroundColor Green
} catch {
    Write-Host "Python not found. Please install Python first." -ForegroundColor Red
    Write-Host "Download from: https://www.python.org/downloads/" -ForegroundColor Yellow
    exit 1
}

# Install required Python packages
Write-Host "Installing required Python packages..." -ForegroundColor Yellow
python -m pip install Pillow

# Convert the images
$image1 = "C:\Users\stupi\SPOON_GIT\esp32\watcat.png"
$image2 = "C:\Users\stupi\SPOON_GIT\esp32\Cursed_cat.png"

if (Test-Path $image1) {
    Write-Host "Converting watcat.png..." -ForegroundColor Green
    python convert_image.py $image1 "watcat"
} else {
    Write-Host "Warning: $image1 not found!" -ForegroundColor Red
}

if (Test-Path $image2) {
    Write-Host "Converting Cursed_cat.png..." -ForegroundColor Green
    python convert_image.py $image2 "cursed_cat"
} else {
    Write-Host "Warning: $image2 not found!" -ForegroundColor Red
}

Write-Host "Conversion complete! Check for watcat_bitmap.h and cursed_cat_bitmap.h files." -ForegroundColor Green
Write-Host "Copy the contents of these files into your Arduino code." -ForegroundColor Yellow