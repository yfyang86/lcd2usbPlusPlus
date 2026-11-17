# Animated GIF to LCD Converter

This example demonstrates how to convert animated GIF files into LCD custom characters and play them as animations on the LCD2USB device.

## Features

- **Animated GIF Support**: Reads multi-frame animated GIF files using giflib
- **Automatic Resizing**: Maintains aspect ratio while fitting to 2x2 LCD character grid (16×10 pixels)
- **Frame Timing**: Respects original GIF frame delays
- **Speed Control**: Adjust playback speed with multiplier (0.5x to 2x or more)
- **Loop Control**: Play once or loop continuously
- **Smart Thresholding**: Auto-detects optimal threshold using Otsu's method
- **Font File Export**: Save first frame to `.lcf` font file

## Prerequisites

### Install giflib

```bash
brew install giflib
```

After installation, rebuild the project:

```bash
cd build
cmake ..
make gif_to_lcd_demo
```

## Usage

```bash
# Play animated GIF on LCD (loops by default)
./gif_to_lcd_demo --input-gif animation.gif

# Play at 2x speed
./gif_to_lcd_demo -i animation.gif --speed 2.0

# Play once (no loop)
./gif_to_lcd_demo -i animation.gif --no-loop

# Limit to first 100 frames at half speed
./gif_to_lcd_demo -i animation.gif -m 100 -s 0.5

# Use manual threshold
./gif_to_lcd_demo -i animation.gif -t 128

# Save first frame to font file
./gif_to_lcd_demo -i animation.gif -o first_frame.lcf

# Process without displaying (saves to file only)
./gif_to_lcd_demo -i animation.gif -o output.lcf --no-display
```

## Command-Line Options

| Option | Short | Description |
|--------|-------|-------------|
| `--input-gif FILE` | `-i` | Input GIF file to convert (required) |
| `--output FILE` | `-o` | Output .lcf font file for first frame (optional) |
| `--threshold VALUE` | `-t` | Threshold value 0-255 (default: auto-detect) |
| `--max-frames NUM` | `-m` | Maximum frames to process (default: all) |
| `--speed MULT` | `-s` | Speed multiplier (default: 1.0) |
| `--no-loop` | | Play animation only once |
| `--no-display` | | Don't display on LCD device |
| `--help` | `-h` | Show help message |

## How It Works

### 1. GIF File Loading (using giflib)
The program reads animated GIF files with support for:
- **Multiple frames**: Extracts all frames from the GIF
- **Color palettes**: Handles both global and local color maps
- **Transparency**: Respects transparent color indices
- **Frame timing**: Reads Graphics Control Extension for accurate delays
- **Variable sizes**: Handles different frame positions and sizes within the canvas

### 2. Frame Processing
Each frame is processed individually:
- **RGB to Grayscale**: Converts color frames using luminance formula
- **Bilinear Interpolation**: Smooth resizing to 10×16 pixels
- **Aspect Ratio**: Maintains original proportions
- **Threshold Detection**: Otsu's method on first frame (applied to all frames)

### 3. Character Generation
For each frame:
- Splits the 10×16 pixel image into a 2×2 character grid
- Each character is 5×8 pixels
- Generates 4 custom characters per frame
- Binary conversion using threshold

### 4. Animation Playback
- Uploads 4 custom characters for each frame to LCD slots 0-3
- Displays them in a 2×2 grid on rows 0-1
- Waits for frame delay (adjusted by speed multiplier)
- Loops or plays once based on settings
- Shows progress every 100 frames

## Display Layout

```
LCD Display (2 rows × 16 columns):
Row 0:  [Char 0][Char 1] ...
Row 1:  [Char 2][Char 3] ...

Where each character is 5×8 pixels:
Total visible area: 10×16 pixels
```

## Performance Considerations

### Frame Processing Time
- Processing happens upfront (all frames before playback)
- Progress indicator shows every 10 frames during processing
- Large GIFs with many frames may take time to load

### Memory Usage
- Each frame stores a 10×16 pixel grayscale image
- Plus 4 custom character definitions (32 bytes)
- For a 1000-frame GIF: ~190KB (very manageable)

### Playback Performance
- Frame updates are near-instantaneous
- USB communication is fast enough for smooth animation
- Typical frame rates: 10-30 FPS depending on GIF delays

## Optimization Tips

### For Large GIFs
Use `--max-frames` to limit processing:
```bash
# Process only first 200 frames
./gif_to_lcd_demo -i large.gif -m 200
```

### For Slow Animations
Increase playback speed:
```bash
# Play at 2x speed
./gif_to_lcd_demo -i slow.gif -s 2.0
```

### For Fast Animations
Decrease playback speed:
```bash
# Play at half speed
./gif_to_lcd_demo -i fast.gif -s 0.5
```

## Creating/Converting GIFs

### Recommended GIF Specifications
- **Size**: Any size (will be resized, but 10×16 or 20×32 work well)
- **Colors**: Any (converted to grayscale)
- **Frame Rate**: 10-30 FPS for smooth playback
- **Content**: High contrast works best (icons, silhouettes, simple animations)

### Convert Video to GIF (ffmpeg)
```bash
# Convert video to GIF, resize to 16x10, 15 FPS
ffmpeg -i input.mp4 -vf "fps=15,scale=16:10:flags=lanczos" \
       -loop 0 output.gif

# Extract first 5 seconds at 20 FPS
ffmpeg -i input.mp4 -t 5 -vf "fps=20,scale=16:10:flags=lanczos" \
       -loop 0 output.gif
```

### Optimize Existing GIF (gifsicle)
```bash
# Install gifsicle
brew install gifsicle

# Reduce colors and optimize
gifsicle -O3 --colors 16 input.gif -o optimized.gif

# Resize GIF
gifsicle --resize 16x10 input.gif -o resized.gif

# Extract frames 0-99
gifsicle input.gif '#0-99' -o subset.gif
```

### Create GIF from Images (ImageMagick)
```bash
# Install ImageMagick
brew install imagemagick

# Create GIF from image sequence
convert -delay 10 -loop 0 frame*.png output.gif

# Create simple animation
convert -size 32x32 -delay 10 -loop 0 \
        xc:white -fill black -draw "circle 16,16 16,8" \
        xc:white -fill black -draw "circle 16,16 20,16" \
        animation.gif
```

## Example: Bad Apple Animation

The famous "Bad Apple!!" animation is a great test case:

```bash
# Play the full animation (loops)
./gif_to_lcd_demo -i /Users/yifanyang/Git/LCD2USB/testapp/assets/Bad-Apple.gif

# Play first 500 frames at 1.5x speed
./gif_to_lcd_demo -i /Users/yifanyang/Git/LCD2USB/testapp/assets/Bad-Apple.gif \
                   -m 500 -s 1.5

# Play once without looping
./gif_to_lcd_demo -i /Users/yifanyang/Git/LCD2USB/testapp/assets/Bad-Apple.gif \
                   --no-loop
```

## Technical Details

### GIF Format Support
- **Compression**: LZW compressed GIF format (standard)
- **Color modes**: Palette-based (up to 256 colors per frame)
- **Interlacing**: Supported (de-interlaced automatically)
- **Disposal methods**: Respects frame disposal settings
- **Extensions**: Graphics Control Extension for timing and transparency

### Library: giflib
- **Version**: 5.x (installed via Homebrew)
- **License**: MIT License
- **API**: Standard C API (`DGifOpenFileName`, `DGifSlurp`)
- **Documentation**: https://giflib.sourceforge.net/

### Frame Timing Accuracy
- GIF specifies delays in centiseconds (1/100 second)
- Converted to milliseconds for precise timing
- Speed multiplier applied: `adjustedDelay = originalDelay / speedMultiplier`
- Minimum delay enforced: 10ms (prevents too-fast animations)

## Troubleshooting

### "giflib not found" during build
```bash
# Install giflib
brew install giflib

# Rebuild
cd build
rm CMakeCache.txt
cmake ..
make gif_to_lcd_demo
```

### "Cannot open GIF file"
- Check file path is correct
- Ensure file is a valid GIF (test with `file` command)
- Check read permissions

### Animation too fast/slow
- Use `--speed` to adjust: `-s 2.0` for 2x speed, `-s 0.5` for half speed
- Some GIFs have incorrect frame delays; manual adjustment may help

### Memory issues with large GIFs
- Use `--max-frames` to process fewer frames
- Consider pre-processing the GIF with gifsicle to reduce size

### Display looks wrong
- Try adjusting threshold: `-t 100`, `-t 128`, `-t 150`, etc.
- Check if GIF has transparency issues
- Verify GIF displays correctly in other viewers

## See Also

- `bmp_to_lcd_demo.cpp` - Static BMP image conversion
- `custom_char_demo.cpp` - Basic custom character usage
- `animated_demo.cpp` - Built-in animated characters
- `font_editor_demo.cpp` - Font file management

## Performance Stats

Typical performance on M1 Mac:
- **Loading**: ~50-100 frames/second
- **Processing**: ~200-300 frames/second  
- **Playback**: Limited by GIF frame delays (typically 10-30 FPS)
- **1000-frame GIF**: ~10-15 seconds total processing time
