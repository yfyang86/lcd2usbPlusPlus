# BMP to LCD Custom Character Converter

This example demonstrates how to convert BMP image files into LCD custom characters and display them on the LCD2USB device.

## Features

- **Pure C++ BMP Reader**: Reads BMP files without external image libraries
- **Automatic Resizing**: Maintains aspect ratio while fitting to 2x2 LCD character grid (16×10 pixels)
- **Smart Thresholding**: Auto-detects optimal threshold using Otsu's method
- **Font File Export**: Save converted characters to `.lcf` font files for reuse
- **Direct LCD Display**: Upload and display the image on the LCD device

## Image Specifications

The LCD display uses a **2×2 grid of custom characters**, where each character is 5×8 pixels:
- **Total display area**: 10 pixels wide × 16 pixels tall
- **Input format**: 24-bit or 32-bit uncompressed BMP files
- **Automatic conversion**: RGB → Grayscale → Binary threshold

## Usage

```bash
# Display BMP on LCD with auto-threshold
./bmp_to_lcd_demo --input-bmp image.bmp

# Save to font file without displaying
./bmp_to_lcd_demo -i image.bmp -o output.lcf --no-display

# Use manual threshold value (0-255)
./bmp_to_lcd_demo -i image.bmp -t 128

# Short form
./bmp_to_lcd_demo -i image.bmp -o output.lcf -t 100
```

## Command-Line Options

| Option | Short | Description |
|--------|-------|-------------|
| `--input-bmp FILE` | `-i` | Input BMP file to convert (required) |
| `--output FILE` | `-o` | Output .lcf font file (optional) |
| `--threshold VALUE` | `-t` | Threshold value 0-255 (default: auto-detect) |
| `--no-display` | | Don't display on LCD device |
| `--help` | `-h` | Show help message |

## How It Works

### 1. BMP File Loading
The program reads uncompressed BMP files (8-bit grayscale, 24-bit, or 32-bit RGB):
- Parses BMP file and info headers
- Handles bottom-up and top-down image formats
- Converts RGB to grayscale using standard luminance formula: `Y = 0.299R + 0.587G + 0.114B`

### 2. Image Resizing
Images are resized to fit the 10×16 pixel display area:
- Uses **bilinear interpolation** for smooth resizing
- Maintains original aspect ratio (may not fill entire display)
- Centers the image in the available space

### 3. Threshold Detection
The program uses **Otsu's method** for automatic threshold detection:
- Analyzes the grayscale histogram
- Finds the optimal threshold that maximizes inter-class variance
- Separates foreground and background pixels

You can override auto-detection with the `--threshold` option for fine-tuning.

### 4. Quantization & Character Generation
- Converts grayscale values to 5 levels (0-4)
- Splits the image into a 2×2 grid of LCD characters
- Each character is 5×8 pixels
- Generates 4 custom characters total

### 5. LCD Upload & Display
- Uploads the 4 custom characters to LCD slots 0-3
- Displays them in a 2×2 grid on the LCD screen
- Characters positions:
  ```
  Row 0:  [0][1] ...
  Row 1:  [2][3] ...
  ```

## Creating Test Images

### Recommended Image Guidelines
- **Size**: Any size (will be resized, but 10×16 or 20×32 work well)
- **Format**: 24-bit uncompressed BMP
- **Content**: High contrast images work best
- **Simple shapes**: Icons, logos, symbols are ideal

### Creating a BMP in Python
```python
#!/usr/bin/env python3
from PIL import Image
import numpy as np

# Create a 32x32 white image
img = Image.new('RGB', (32, 32), 'white')
pixels = img.load()

# Draw a simple pattern (e.g., a circle)
center = 16
for y in range(32):
    for x in range(32):
        dist = ((x-center)**2 + (y-center)**2)**0.5
        if dist < 10:
            pixels[x, y] = (0, 0, 0)  # Black circle

# Save as BMP
img.save('circle.bmp')
```

### Creating a BMP in GIMP
1. Create new image (any size, recommend 32×32)
2. Draw your design
3. File → Export As
4. Choose `.bmp` extension
5. Uncheck "Run-Length Encoded" (use uncompressed)
6. Select 24-bit or 32-bit color depth

### Creating a BMP on Command Line (ImageMagick)
```bash
# Convert any image to 24-bit BMP
convert input.png -type TrueColor output.bmp

# Create a simple test pattern
convert -size 32x32 xc:white \
        -fill black -draw "circle 16,16 16,8" \
        test.bmp
```

## Technical Details

### LCD Custom Character Format
Each LCD character is defined by 8 bytes (one per row):
```
Byte 0: xxxxx000  (top row, 5 pixels)
Byte 1: xxxxx000
Byte 2: xxxxx000
Byte 3: xxxxx000
Byte 4: xxxxx000
Byte 5: xxxxx000
Byte 6: xxxxx000
Byte 7: xxxxx000  (bottom row)
```

Each `x` is a bit (1 = pixel on, 0 = pixel off).

### Font File Format (.lcf)
The program can save converted characters to LCD2USB font files:
- Binary format with magic signature "LCF1"
- Contains metadata: author, creation time
- Stores all 4 custom characters
- Can be loaded by other LCD2USB applications

## Example Use Cases

### 1. Display a Logo
```bash
./bmp_to_lcd_demo -i company_logo.bmp
```

### 2. Create Reusable Icon Set
```bash
./bmp_to_lcd_demo -i icon.bmp -o icons.lcf --no-display
```

### 3. Fine-tune Threshold
```bash
# Try different thresholds to find the best look
./bmp_to_lcd_demo -i photo.bmp -t 80
./bmp_to_lcd_demo -i photo.bmp -t 120
./bmp_to_lcd_demo -i photo.bmp -t 160
```

## Limitations

- Maximum display size is 10×16 pixels (2×2 character grid)
- Only uncompressed BMP format supported
- Binary output (pixels are either on or off, no grayscale)
- For larger displays, consider using multiple character grids

## See Also

- `custom_char_demo.cpp` - Basic custom character usage
- `font_editor_demo.cpp` - Font file management
- `animated_demo.cpp` - Animated custom characters
