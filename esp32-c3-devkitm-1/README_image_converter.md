# ESP32 OLED Image to Bitmap Converter

This toolset converts images to monochrome bitmap arrays suitable for ESP32 OLED displays (128x64 pixels).

## Features

- ✅ Resize images to fit 2:1 aspect ratio (128x64)
- ✅ Convert to monochrome (1-bit) with adjustable threshold
- ✅ Generate C bitmap arrays with PROGMEM storage
- ✅ Batch process multiple images
- ✅ Preview generated images before use
- ✅ Optimized for SSD1306 OLED displays

## Files

- `image_to_bitmap.py` - Main conversion script
- `batch_process_memes.py` - Batch processor for all images in `images/memes/`

## Prerequisites

```bash
# Make sure Python 3 and Pillow are installed
python3 -c "import PIL; print(f'Pillow version: {PIL.__version__}')"
```

If Pillow is not installed:
```bash
pip3 install Pillow
```

## Usage

### Single Image Conversion

```bash
# Basic usage
python3 image_to_bitmap.py input_image.png

# With custom variable name
python3 image_to_bitmap.py images/memes/watcat.png watcat

# With custom threshold (0-255, default: 128)
python3 image_to_bitmap.py my_image.jpg --threshold 100

# Custom dimensions (default: 128x64)
python3 image_to_bitmap.py image.png --width 64 --height 32
```

### Batch Processing

```bash
# Process all images in images/memes/ directory
python3 batch_process_memes.py
```

## Output Files

For each processed image, the script generates:

1. **`name_preview.png`** - Preview of the processed monochrome image
2. **`name_bitmap.txt`** - C code with bitmap array ready for ESP32

## Example Output

```c
const unsigned char watcat_bitmap[] PROGMEM = {
    0xFF, 0xFF, 0xFF, 0xF8, 0x00, 0x0C, 0x00, 0x00, 
    // ... more data ...
};
const int watcat_width = 128;
const int watcat_height = 64;
```

## Integration with ESP32 Code

1. Copy the generated bitmap arrays into your ESP32 code
2. Use with Adafruit_SSD1306 library:

```cpp
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Include your generated bitmap arrays here
const unsigned char watcat_bitmap[] PROGMEM = { /* ... data ... */ };
const int watcat_width = 128;
const int watcat_height = 64;

void displayMeme(const unsigned char *bitmap, int width, int height) {
    display.clearDisplay();
    display.drawBitmap(0, 0, bitmap, width, height, SSD1306_WHITE);
    display.display();
}

// Usage
displayMeme(watcat_bitmap, watcat_width, watcat_height);
```

## Image Processing Details

1. **Resize**: Images are resized to fit within 128x64 while maintaining aspect ratio
2. **Crop/Pad**: If needed, images are cropped to center or padded with white background
3. **Monochrome**: Converted to 1-bit using threshold (default 128)
4. **Bitmap**: Packed into bytes for efficient OLED display

## Tips

- **Threshold**: Lower values (0-127) make images darker, higher (128-255) make them lighter
- **High Contrast**: Images with high contrast work best for monochrome conversion
- **Simple Graphics**: Line art, logos, and simple illustrations convert better than photos
- **File Formats**: Supports PNG, JPG, JPEG, BMP, GIF, TIFF

## Troubleshooting

### Image appears too dark/light
```bash
# Try adjusting the threshold
python3 image_to_bitmap.py image.png --threshold 100  # Lighter
python3 image_to_bitmap.py image.png --threshold 200  # Darker
```

### Image is cropped unexpectedly
- Check the original aspect ratio
- Images much wider or taller than 2:1 will be cropped
- Consider editing the source image first

### No detail visible
- Try a different threshold value
- Ensure source image has good contrast
- Consider preprocessors the image (increase contrast, sharpen)

## File Structure

```
esp32-c3-devkitm-1/
├── image_to_bitmap.py          # Main conversion script
├── batch_process_memes.py      # Batch processor
├── README_image_converter.md   # This file
├── images/
│   └── memes/                  # Source images
│       ├── watcat.png
│       └── Cursed_cat.png
├── *_preview.png              # Generated preview images
├── *_bitmap.txt               # Generated C bitmap code
└── c3-oled/
    └── src/
        └── main.cpp           # ESP32 code
```
