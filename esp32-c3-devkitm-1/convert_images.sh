#!/bin/bash

# Script to convert PNG images to C-style char arrays for 128x64 monochrome OLED.
# Output format is horizontal scanline (1024 bytes for 128x64).
# Bitmap arrays are declared as "const unsigned char array_name[] PROGMEM".
# Width and height constants are also generated in each file.
#
# Dependencies: ImageMagick (for `convert`) and `xxd`.
# Ensure these are installed on your system (e.g., sudo apt install imagemagick xxd).

# Workspace and directory paths (adjust if your structure is different)
WORKSPACE_ROOT="/home/sam/SPOON_GIT/esp32/esp32-c3-devkitm-1"
SRC_DIR="${WORKSPACE_ROOT}/images/memes"
DEST_DIR="${WORKSPACE_ROOT}/images/bmp"

# Ensure destination directory exists
mkdir -p "$DEST_DIR"

# Check if source directory exists
if [ ! -d "$SRC_DIR" ]; then
    echo "Error: Source directory '$SRC_DIR' not found."
    exit 1
fi

echo "Starting image conversion (with PROGMEM and width/height constants)..."
echo "Source PNGs: $SRC_DIR"
echo "Output TXTs: $DEST_DIR"
echo "Output format: Horizontal scanline, 1 bit per pixel, MSB first."
echo "---------------------------------------------------------------------"

# Loop through all PNG files in the source directory
find "$SRC_DIR" -maxdepth 1 -type f \( -iname "*.png" \) -print0 | while IFS= read -r -d $'\0' png_file; do
    filename=$(basename -- "$png_file")
    base_name="${filename%.*}" # Strip .png extension
    dest_file_path="$DEST_DIR/${base_name}.txt"
    temp_xxd_output="$DEST_DIR/${base_name}_xxd.tmp"

    c_array_name=$(echo "$base_name" | tr '[:upper:]' '[:lower:]' | sed -e 's/[^a-z0-9_]/_/g' -e 's/__\+/_/g')
    if [[ "$c_array_name" =~ ^[0-9] ]]; then
        c_array_name="_${c_array_name}"
    fi
    if [ -z "$c_array_name" ]; then # Should not happen with actual filenames
        c_array_name="image"
    fi
    c_array_name="${c_array_name}_bmp"

    echo "Processing '$filename':"
    echo "  Output file: '$dest_file_path'"
    echo "  C array name: '$c_array_name'"

    # ImageMagick command piped to xxd, output to a temporary file
    convert "$png_file" \
        -alpha remove -background white -trim +repage \
        -resize 128x64\> \
        -gravity center \
        -background black \
        -extent 128x64 \
        -dither FloydSteinberg -monochrome \
        -depth 1 mono:- | xxd -i -n "$c_array_name" > "$temp_xxd_output"

    if [ $? -eq 0 ]; then
        # Start creating the final .txt file
        echo "// Format: Horizontal scanline, 128x64, 1bpp, MSB first" > "$dest_file_path"
        echo "// Bitmap data for $filename" >> "$dest_file_path"
        
        # Modify the array definition line from xxd output to add const and PROGMEM
        # and append it to dest_file_path
        # xxd output: unsigned char foobar_bmp[] = {
        # Desired: const unsigned char foobar_bmp[] PROGMEM = {
        sed "s/^unsigned char ${c_array_name}\[\]/const unsigned char ${c_array_name}[] PROGMEM/" "$temp_xxd_output" >> "$dest_file_path"
        
        # Append width and height constants
        echo "const unsigned int ${c_array_name}_width = 128;" >> "$dest_file_path"
        echo "const unsigned int ${c_array_name}_height = 64;" >> "$dest_file_path"
        
        rm "$temp_xxd_output"
        echo "  Successfully converted '$filename' to '$dest_file_path' with PROGMEM and constants."
    else
        echo "  ERROR converting '$filename'."
        if [ -f "$temp_xxd_output" ]; then
            rm "$temp_xxd_output"
        fi
    fi
    echo "---------------------------------------------------------------------"
done

echo "Image conversion process complete."
echo
echo "IMPORTANT REMINDER:"
echo "The generated .txt files contain bitmap data in a HORIZONTAL SCANLINE format."
echo "The C++ code needs a custom function to draw these (Adafruit_GFX default is vertical)."
echo "Bitmap arrays are now declared with 'const unsigned char array_name[] PROGMEM'."
echo "Width and height constants (e.g., ${c_array_name}_width) are also in each .txt file."

