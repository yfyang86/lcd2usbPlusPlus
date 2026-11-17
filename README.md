# LCD2USB++ - Professional C++ Library

A modern, professional C++ library for controlling LCD2USB devices. This library provides a clean, object-oriented interface for USB LCD displays with comprehensive font management and character animation capabilities.

## 🚀 Features

- 🖥️ **Full LCD Control**: Text display, cursor positioning, screen management
- 🌍 **UTF-8 + multi-language text**: decode, render, and scroll text in any language that fits in ≤2 HD44780 cells per codepoint. See `NOTES-utf8.md` for the full design — the short version is: UTF-8 decoder → codepoint-to-glyph table → shadow-framebuffer diff → CGRAM LRU cache, with TTF or pre-baked `.glyphs` pack as the font source.
- 🧰 **`tools/` directory**: `tools/ufo_to_glyphs.py` converts a UFO pixel font into a compact 24-byte-per-glyph binary pack; `tools/glyph_inspect` previews any codepoint as ASCII art. Runs offline — no FreeType needed at runtime if you use packs.
- 🎨 **Custom Characters**: Create and upload 5x8 pixel custom characters  
- 🔄 **Character Animations**: Frame-based custom character animations
- 📚 **Font Management**: Professional font collection system with binary file I/O
- 🖼️ **Image Processing**: BMP to LCD converter with auto-threshold detection
- 🎬 **GIF Animation**: Animated GIF player with flexible grid sizing and FPS control
- 🚀 **Modern C++17**: Clean API with RAII and proper error handling
- 🔧 **CMake Build System**: Professional build configuration
- 📱 **Cross-Platform**: Works on macOS and Linux
- ✅ **Hardware Tested**: Verified with real LCD2USB devices

## 📦 Project Structure

```
lcd2usb++/
├── CMakeLists.txt              # Root build configuration
├── README.md                   # This file
├── Manual-FontEditor.md        # Font editor documentation
├── include/lcd2usb/            # Public library headers
│   ├── lcd2usb_lib.hpp        # Main library include
│   ├── lcd2usb.hpp            # Legacy compatibility header
│   ├── lcd_display.hpp        # Core LCD communication
│   ├── custom_char.hpp        # Custom character system
│   ├── font_editor.hpp        # Font management system
│   ├── multi_char_display.hpp # Grid display helper
│   └── scrolling_text.hpp     # Text scrolling system
├── src/                        # Library implementation
│   ├── lcd_display.cpp        # LCD communication code
│   ├── custom_char.cpp        # Custom character code
│   ├── font_editor.cpp        # Font management code
│   ├── multi_char_display.cpp # Grid display implementation
│   ├── scrolling_text.cpp     # Text scrolling code
│   └── CMakeLists.txt         # Library build config
├── examples/                   # Example applications
│   ├── basic_test.cpp         # Basic LCD functionality
│   ├── custom_char_demo.cpp   # Custom characters
│   ├── animated_demo.cpp      # Character animations
│   ├── font_editor_demo.cpp   # Advanced font management
│   ├── font_editor_demo_simple.cpp # Simple font demo
│   ├── storyteller.cpp        # Text file reader with scrolling
│   ├── bmp_to_lcd_demo.cpp    # BMP image viewer
│   ├── gif_to_lcd_demo.cpp    # Animated GIF player
│   ├── lcd-rstasks-kanban.cpp # RsTask kanban board viewer
│   ├── full_demo.cpp          # Complete feature showcase
│   ├── scrolling_demo.cpp     # Text scrolling demo
│   ├── system_monitor.cpp     # System stats display
│   └── CMakeLists.txt         # Examples build config
├── assets/                     # Demo files
│   ├── story.txt              # Sample text for storyteller
│   ├── demo1.bmp              # Sample BMP image
│   ├── Bad-Apple.gif          # Sample GIF animation
│   └── *.md                   # Additional text files
├── cmake/                      # CMake configuration
│   └── lcd2usbConfig.cmake.in
└── trunc/                      # Archive of old code
```

## 🛠️ Prerequisites

- **CMake 3.16** or newer
- **C++17** compatible compiler (GCC 7+, Clang 5+, MSVC 2019+)
- **libusb** development libraries
- **giflib** (for GIF animation support)
- **ncurses** (for GIF player terminal UI)
- **LCD2USB hardware** for testing

### Installing Dependencies

**macOS:**
```bash
brew install cmake libusb-compat giflib ncurses
```

**Ubuntu/Debian:**
```bash
sudo apt install cmake build-essential libusb-dev libusb-1.0-0-dev libgif-dev libncurses-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install cmake gcc-c++ libusb-devel libusb1-devel giflib-devel ncurses-devel
```

## 🔨 Building

```bash
# Clone or download the project
cd lcd2usb++

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build library and examples
make -j4

# Built files will be in:
# - src/liblcd2usb.a     (static library)
# - examples/            (example executables)
```

## 📚 Library Usage

### Basic LCD Operations

```cpp
#include <lcd2usb/lcd2usb_lib.hpp>
using namespace LCD2USB;

int main() {
    // Initialize LCD display
    LCDDisplay display;
    
    // Basic text output
    display.clear();
    display.setCursor(0, 0);
    display.print("Hello, LCD2USB++!");
    display.setCursor(0, 1);
    display.print("Modern C++ API");
    
    return 0;
}
```

### Custom Characters

```cpp
#include <lcd2usb/lcd2usb_lib.hpp>
using namespace LCD2USB;

int main() {
    LCDDisplay display;
    
    // Create custom character using factory
    auto heart = CharacterFactory::createHeart();
    auto spinner = CharacterFactory::createSpinner(0);
    
    // Upload to LCD custom character memory
    display.uploadCustomCharacter(0, heart);
    display.uploadCustomCharacter(1, spinner);
    
    // Display custom characters
    display.clear();
    display.setCursor(0, 0);
    display.print("Custom chars: ");
    display.print(static_cast<char>(0)); // Heart
    display.print(static_cast<char>(1)); // Spinner
    
    return 0;
}
```

### Font Management

```cpp
#include <lcd2usb/lcd2usb_lib.hpp>
using namespace LCD2USB;
using namespace LCD2USB::FontEdit;

int main() {
    LCDDisplay display;
    FontManager font_manager("My Collection");
    
    // Create and add characters
    auto heart = CharacterFactory::createHeart();
    CharacterBitmap heart_bitmap("Heart");
    heart_bitmap.setDescription("Love symbol");
    heart_bitmap.setFrame(0, heart.toBytes());
    
    font_manager.addCharacter('H', heart_bitmap);
    
    // Save to binary file
    font_manager.saveToFile("my_font.lcf");
    
    // Load from file
    FontManager loaded_font;
    loaded_font.loadFromFile("my_font.lcf");
    
    return 0;
}
```

## 🎯 Examples Walkthrough

The library includes four comprehensive examples demonstrating different aspects of the LCD2USB++ functionality:

### 1. Basic Test (`examples/basic_test.cpp`)

**Purpose**: Demonstrates fundamental LCD operations and USB communication.

**Features**:
- USB device detection and initialization
- Basic text display and cursor positioning
- Communication verification with echo test
- Error handling and graceful shutdown

**Run**:
```bash
cd build/examples
./basic_test
```

**Expected Output**:
```
=== LCD2USB++ Basic Test ===
Found LCD2USB device on bus 008 device 017
LCD2USB device opened and communication verified
LCD2USB device found and initialized!
Echo test successful!
Firmware version 1.9
Basic text displayed. Waiting 3 seconds...
Basic test completed successfully!
```

### 2. Custom Character Demo (`examples/custom_char_demo.cpp`)

**Purpose**: Showcases the custom character creation system.

**Features**:
- Heart, progress bar, and arrow character creation
- Pattern-based character definition
- Hex byte display and validation
- Multiple character upload and display

**Run**:
```bash
./custom_char_demo
```

**What You'll See**:
- Heart character (❤️) displayed on LCD
- Animated progress bars showing completion
- Directional arrows for navigation
- Pattern breakdown with hex values

### 3. Animated Demo (`examples/animated_demo.cpp`)

**Purpose**: Demonstrates frame-based character animation.

**Features**:
- Multi-frame character animation
- Jumping stick figure animation
- Rotating spinner effects
- Signal handling for graceful exit (Ctrl+C)

**Run**:
```bash
./animated_demo
# Press Ctrl+C to stop cleanly
```

**Animation Sequence**:
- Jumping character with 4-frame animation
- Smooth transitions between frames
- Continuous loop until interrupted

### 4. Font Editor Demo (`examples/font_editor_demo_simple.cpp`)

**Purpose**: Showcases the professional font management system.

**Features**:
- Font collection creation and management
- Binary file save/load operations (.lcf format)
- Character metadata (names, descriptions, timestamps)
- Integration with display system

**Run**:
```bash
./font_editor_demo
```

**Demonstration Flow**:
1. Creates a font collection with heart and note characters
2. Displays characters on LCD
3. Saves collection to `demo_font.lcf`
4. Loads the saved font and verifies integrity
5. Shows metadata information (author, creation time, character count)

**Generated Files**:
- `demo_font.lcf`: Binary font file containing character data

### 5. Story Teller (`examples/storyteller.cpp`)

**Purpose**: Advanced text file reader with scrolling display and progress visualization.

**Features**:
- Load any text file (txt, md, etc.) in 256-byte chunks for memory efficiency
- Configurable screen width support (2-40 characters)
- Adjustable scroll speed (1+ seconds per scroll)
- Visual progress bars using custom characters:
  - Top line: Current chunk progress
  - Bottom line: Total file progress  
- Vertical scrolling with word wrap and padding
- Command-line argument parsing
- Signal handling for clean shutdown

**Command Line Options**:
```bash
./storyteller [options]
Options:
  --file <path>        Text file to display (default: assets/story.txt)
  --width <n>          Screen width in characters, 2-20 (default: 16)
  --rows <n>           Number of rows, 1-4 (default: 2)
  --speed <seconds>    Scroll speed in seconds (default: 2)
  --help              Show this help message
```

**Usage Examples**:
```bash
# Use defaults (story.txt, 16 chars wide, 2 rows, 2 sec scroll)
./storyteller

# Custom file and faster scrolling
./storyteller --file my_story.txt --speed 1

# Wide 4-row display with slow scrolling
./storyteller --file novel.md --width 20 --rows 4 --speed 3

# 2x2 display
./storyteller --width 16 --rows 2

# Show help
./storyteller --help
```

**Run from project root**:
```bash
cd /path/to/LCD2USB-master/testapp
build/examples/storyteller --file assets/story.txt --width 16 --rows 2 --speed 2
```

**Expected Output**:
```
=== LCD2USB++ Story Teller ===
Configuration:
  File: assets/story.txt
  Screen width: 16 characters
  Screen rows: 2 rows
  Scroll speed: 2 seconds

Found LCD2USB device on bus 008 device 017
LCD initialized successfully
Loading file: assets/story.txt (2936 bytes)
Loaded 12 chunks
Starting story display (Press Ctrl+C to stop)...
```

**LCD Display Features**:
- Line 1: Scrolling text content with word wrapping
- Line 2: Continues scrolling text seamlessly
- Right margin (char 16): Progress bars showing:
  - Current chunk progress (top line)
  - Total file progress (bottom line)
- Progress bars use 5 custom characters (empty, quarter, half, three-quarter, full)

**Technical Details**:
- **Chunk Processing**: Files loaded in 256-byte segments to handle large texts
- **Memory Management**: Only keeps current chunk and small line queue in memory
- **Word Wrapping**: Smart text reflow that respects word boundaries
- **Progress Tracking**: Dual-level progress (chunk and total file)
- **Custom Characters**: 5-level progress bar visualization (░▊▌▊█)
- **Signal Handling**: Responds to Ctrl+C for clean shutdown

---

### 6. BMP Image Viewer (`examples/bmp_to_lcd_demo.cpp`)

**Purpose**: Display static BMP images on the LCD using custom character grids. Converts BMP images to 5×8 pixel custom character patterns with automatic threshold detection.

**Features**:
- Flexible grid sizing (--width, --height)
- Bilinear interpolation for smooth image resizing
- Otsu's method for automatic threshold detection
- Manual threshold control (--threshold 0-255)
- Image scaling factor (--scale)
- Works with 24-bit and 32-bit BMP files
- Graceful Ctrl+C handling with cleanup message
- Startup CGRAM clearing to remove residual patterns

**Command Line Options**:
```bash
./bmp_to_lcd_demo [options] <bmp_file>
Options:
  --width <n>         Grid width (default: 2)
  --height <n>        Grid height (default: 2)
  --scale <n>         Image scale factor (default: 1)
  --threshold <0-255> Manual threshold (default: auto-detect)
```

**Usage Examples**:
```bash
# Display image as 2x4 grid (8 characters, recommended)
./bmp_to_lcd_demo --width 2 --height 4 image.bmp

# Display as 2x2 grid with auto-threshold
./bmp_to_lcd_demo --width 2 --height 2 image.bmp

# Display with manual threshold (0-255)
./bmp_to_lcd_demo --width 2 --height 3 --threshold 128 image.bmp

# Display with 2x scaling
./bmp_to_lcd_demo --width 2 --height 4 --scale 2 image.bmp
```

**Implementation Highlights**:
- **Image Processing**: Bilinear interpolation for high-quality resizing
- **Threshold Detection**: Otsu's automatic threshold for optimal black/white conversion
- **Custom Characters**: Each 5×8 pixel character = 40 pixels
- **CGRAM Management**: MultiCharDisplay uploads patterns directly to slots 1-7 before display
- **Signal Handling**: Clean exit with "Image viewer stopped" message
- **Display Strategy**: Atomic upload and display prevents blank character artifacts

**⚠️ Grid Size Recommendations**:
- **Best for static images**: 2×3 (6 chars), 2×2 (4 chars), 1×7 (7 chars)
- **Works with warning**: 2×4 (8 chars), 3×3 (9 chars), 4×4 (16 chars) - uses slot reuse technique
- See "HD44780 Hardware Limitations" section below for technical details

---

### 7. Animated GIF Player (`examples/gif_to_lcd_demo.cpp`)

**Purpose**: Play animated GIF files on the LCD with real-time status display in terminal using ncurses.

**Features**:
- Animated GIF playback with configurable FPS
- Flexible grid sizing (--width, --height)
- Real-time terminal status: frame counter, FPS, progress bar
- Auto-threshold detection or manual control
- Frame limiting and loop control
- Graceful Ctrl+C handling
- ncurses-based status display (clean terminal UI)

**Command Line Options**:
```bash
./gif_to_lcd_demo [options] <gif_file>
Options:
  --width <n>         Grid width (default: 2)
  --height <n>        Grid height (default: 2)
  --fps <n>           Frames per second (default: 10)
  --scale <n>         Image scale factor (default: 1)
  --threshold <0-255> Manual threshold (default: auto-detect)
  -m <n>              Maximum frames to display
  --loop              Loop animation continuously
```

**Usage Examples**:
```bash
# Play GIF as 2x4 grid at 15 FPS (recommended for smooth animation)
./gif_to_lcd_demo --width 2 --height 4 --fps 15 animation.gif

# Play with frame limit and loop
./gif_to_lcd_demo --width 2 --height 3 --fps 10 -m 100 --loop animation.gif

# Play with manual threshold
./gif_to_lcd_demo --width 2 --height 4 --threshold 128 --fps 15 animation.gif

# Bad Apple!! demo (famous animation)
./gif_to_lcd_demo --width 2 --height 4 --fps 15 badapple.gif
```

**Terminal Status Display** (ncurses):
```
┌─ GIF Animation Status ─────────────────────────┐
│ Frame: 147/220 (66.8%)                         │
│ FPS: 15                                         │
│ Progress: ████████████░░░░░░░░░░░░░░░░░░░░░░░  │
│                                                 │
│ Press Ctrl+C to stop                           │
└─────────────────────────────────────────────────┘
```

**Implementation Highlights**:
- **GIF Decoding**: Uses `giflib` for frame extraction
- **Terminal UI**: ncurses for clean status display without flickering
- **Frame Rendering**: Frame-by-frame character grid conversion
- **Timing Control**: Configurable FPS with precise frame timing
- **Signal Handling**: Clean exit with "Animation stopped" message on LCD

**⚠️ Critical for Animations**:
- **Recommended grids**: 2×3 (6 chars), 2×2 (4 chars), 1×7 (7 chars) **ONLY**
- **Avoid**: Grids >7 characters will show **visual artifacts** in animations
- Each animation frame must use ≤7 unique character patterns (we skip slot 0 to avoid display issues)
- See "HD44780 Hardware Limitations" section below for why this matters

---

### 8. RsTask Kanban Board (`examples/lcd-rstasks-kanban.cpp`)

**Purpose**: Live kanban board for [RsTask](https://github.com/yfyang86/rsTask) workspaces (the `rstasks` crate / CLI) on the LCD — queue / work / review / done counts on the first row, active tasks below, each with its own custom-character icon. Requires the `rstasks` binary on `PATH`.

**Features**:
- Fetches the board by running `rstasks --db <db> board`; `--rstasks-path` accepts a workspace directory (holding `.rstask/`) or a direct `rstask.sqlite` file, and is repeatable — the first path that exists wins
- Row 0 is always the summary: `Q:n W:n R:n D:n` counts, plus a page marker and clock when they fit
- Every task gets a deterministic 5×8 "identicon" custom character (FNV-1a of the task id, mirrored for symmetry) with a lane footer baked into the glyph: queue = bottom corners, work = up arrow, review = diamond
- Default mode paginates active tasks (work → review → queue, done counted only); the page flips on every data refresh
- `--details` horse-racing mode — applied only to rows that overflow the screen; a row whose content fits stays static:
  - 2–3 row displays stream all active tasks upward through the detail rows, one task per step
  - 4-row displays dedicate one row per lane (WORK | REVIEW | QUEUE) and marquee each lane's tasks horizontally, icons riding along
- Board data refreshes every `--duration` minutes; the animation runs on its own ~4 fps clock between refreshes
- `--dry-run` console preview (icons shown as `#`), `--once` renders a single frame, clean SIGINT/SIGTERM shutdown

**Command Line Options**:
```bash
./lcd_rstasks_kanban [options]
Options:
  --rstasks-path PATH  RsTask workspace dir (with .rstask/) or path to
                       rstask.sqlite. Repeatable; first match wins (default: .)
  --rows N             LCD rows (default 4)
  --width N            LCD columns (default 20)
  --duration MIN       minutes between board refreshes (default 10)
  --details            horse-racing detail view (see above)
  --once               render one frame and exit
  --dry-run            print frames to stdout, do not touch the LCD
  --help               show help
```

**Usage Examples**:
```bash
# Live board from a workspace on a 4x20 LCD
./lcd_rstasks_kanban --rstasks-path ~/projects/my-workspace

# Horse-racing details, refresh every 10 minutes
./lcd_rstasks_kanban --rstasks-path ~/projects/my-workspace --details --duration 10

# Preview the layout in a terminal (no hardware needed)
./lcd_rstasks_kanban --rstasks-path . --details --dry-run --once
```

**Expected Output** (4×20 with `--details`; `#` stands for the task's identicon glyph):

```
+--------------------+
|Q:4 W:1 R:1 D:1     |
|W # 184e Write kanb|
|R # 89c2 Audit UTF-|
|Q # 1d29 Fix LCD ro|
+--------------------+
```

**Technical Details**:
- **CLI integration**: parses the `== status (n) ==` sections and task table of `rstasks board`; workspace directories resolve to `.rstask/rstask.sqlite`
- **Identicons**: `CustomChar` 5×8 patterns generated from an FNV-1a hash of the task id; uploaded to CGRAM slots 1–7 (slot 0 is skipped — see HD44780 notes) with change-only uploads; when a frame needs more than 7 distinct icons, the extras degrade to a space
- **Width-exact lines**: every row is padded/truncated to exactly `--width` bytes, so the HD44780 cursor never wraps into the next row
- **Two-clock design**: board data refreshes on the `--duration` (minutes) schedule while the horse-racing animation ticks every 250 ms between refreshes

---

## ⚠️ HD44780 Hardware Limitations

### The 8-CGRAM Slot Constraint

The HD44780 LCD controller has a fundamental hardware limitation that affects custom character display:

**Key Facts**:
- Only **8 CGRAM slots** available (indices 0-7)
- Each slot stores one 5×8 pixel custom character pattern (8 bytes)
- Display positions store **slot numbers**, not actual pixel patterns
- All positions referencing the same slot show the **same pattern in real-time**
- **Character code 0 may not display properly** on some LCD controllers, so we use slots 1-7 (effectively 7 usable slots)

### Why This Matters

When you display a custom character:
1. You upload the 8-byte pattern to a CGRAM slot (0-7)
2. You tell the LCD to display that slot number at a position
3. The LCD **references** the slot - it doesn't copy the pattern
4. If you change slot 7, **all positions showing slot 7 update instantly**

**Example Problem**:
```cpp
// Upload pattern A to slot 7, display at (0,0)
display.uploadCustomCharacter(7, patternA);
display.setCursor(0, 0);
display.print((char)7);  // Shows pattern A at (0,0)

// Upload pattern B to slot 7, display at (1,0)
display.uploadCustomCharacter(7, patternB);
display.setCursor(1, 0);
display.print((char)7);  // Shows pattern B at (1,0)
                         // But (0,0) also changes to pattern B! ❌
```

### Impact on Grid Displays

**For Static Images** (bmp_to_lcd_demo):
- ✅ **≤7 characters**: Each gets a unique slot (1-7), perfect display
- ⚠️ **>7 characters**: Uses "slot 7 reuse" - uploads chars 0-5 to slots 1-6, then dynamically uploads chars 6+ to slot 7 one at a time. Works because static images don't change after display.

**For Animations** (gif_to_lcd_demo):
- ✅ **≤7 characters**: Smooth animation, no artifacts
- ❌ **>7 characters**: Severe artifacts - updating slot 7 for char[6] changes previously displayed char[7], char[8], etc.

**Note**: We skip CGRAM slot 0 because character code 0 may not display properly on some HD44780 controllers, effectively giving us 7 reliably usable slots (1-7) instead of 8.

### Recommended Configurations

**Best Choices** (≤7 characters):
- **2×3 grid** = 6 characters ✅ (recommended for clean display)
- **2×2 grid** = 4 characters ✅
- **1×7 grid** = 7 characters ✅ (uses all available slots)

**Works for Static Images Only** (>7 characters):
- **2×4 grid** = 8 characters ⚠️ (shows warning, uses slot reuse)
- **3×3 grid** = 9 characters ⚠️ (shows warning, uses slot reuse)
- **4×4 grid** = 16 characters ⚠️ (shows warning, uses slot reuse)

**Avoid for Animations** (>7 characters):
- Any grid >7 characters will show visual artifacts

### Technical Workarounds

The `MultiCharDisplay` helper class implements two strategies:

**Strategy 1: Direct Display** (≤7 characters)
```cpp
// Upload all characters to unique slots 1-7 (skip slot 0)
for (int i = 0; i < numChars; i++) {
    display.uploadCustomCharacter(i + 1, chars[i]);
}
// Display grid using character codes 1-7
for (int i = 0; i < numChars; i++) {
    display.print((char)(i + 1));
}
```

**Strategy 2: Slot 7 Reuse** (>7 characters)
```cpp
// Upload first 6 characters to slots 1-6
for (int i = 0; i < 6; i++) {
    display.uploadCustomCharacter(i + 1, chars[i]);
}
// For characters 6+, dynamically reuse slot 7
for (int i = 6; i < numChars; i++) {
    display.uploadCustomCharacter(7, chars[i]);
    display.setCursor(col, row);
    display.print((char)7);  // Display immediately before next upload
}
```

**Why Skip Slot 0?**
Character code 0 (null character) may not display properly on some HD44780 controllers or may be treated specially. By using slots 1-7, we avoid potential display issues with character code 0.

**Why Strategy 2 Works for Static Images**:
- Each character is displayed immediately after upload
- Static images don't need to update existing positions
- Once displayed, the pattern persists even if slot 7 changes

**Why Strategy 2 Fails for Animations**:
- Animation frames need to update all positions
- Updating slot 7 for position (3,3) also changes positions (2,3), (1,3), etc.
- Causes flickering and incorrect patterns

### Summary

**The 7-character limit is a hardware constraint, not a software bug.** While the HD44780 controller has 8 CGRAM slots (0-7), character code 0 may not display properly on some controllers. By using only slots 1-7, we ensure reliable display across all HD44780-compatible LCDs.

For best results:
- Use 2×3, 2×2, or 1×7 grids for animations (≤7 characters)
- Larger grids work for static images but show warnings
- Plan your character layouts to stay within 7 unique patterns when possible

## 🔧 API Reference

### Core Classes

#### `LCD2USB::LCDDisplay`
Main class for LCD communication and control.

**Key Methods**:
- `clear()` - Clear display
- `setCursor(int col, int row)` - Set cursor position
- `print(const std::string& text)` - Display text
- `uploadCustomCharacter(int slot, const CustomChar& char)` - Upload custom character

#### `LCD2USB::CustomChar`
Class for creating custom 5x8 pixel characters.

**Key Methods**:
- `setPixel(int x, int y, bool value)` - Set individual pixel
- `toBytes()` - Convert to byte array for upload
- `clear()` - Clear all pixels

#### `LCD2USB::CharacterFactory`
Factory class with predefined character templates.

**Available Characters**:
- `createHeart()` - Heart symbol
- `createNote()` - Musical note
- `createSpinner(int frame)` - Loading spinner (4 frames)
- `createMatchGuy(int frame)` - Jumping character (4 frames)
- `createDancer(int frame)` - Dancing figure (2 frames)

#### `LCD2USB::FontEdit::FontManager`
Professional font collection management.

**Key Methods**:
- `addCharacter(uint8_t index, const CharacterBitmap& char)` - Add character
- `saveToFile(const std::string& filename)` - Save to .lcf file
- `loadFromFile(const std::string& filename)` - Load from .lcf file
- `getCharacterCount()` - Get number of characters

## 🔗 Integration

### Using in Your CMake Project

Add to your `CMakeLists.txt`:

```cmake
# Find the library
find_package(LCD2USB REQUIRED)

# Link to your target
target_link_libraries(your_target PRIVATE LCD2USB::lcd2usb)
```

### Manual Compilation

```bash
# Compile your application
g++ -std=c++17 your_app.cpp -I/path/to/lcd2usb/include -L/path/to/lcd2usb/build/src -llcd2usb -lusb
```

## 🐛 Troubleshooting

### Device Not Found
```
Error: LCD2USB device not found
```
**Solutions**:
- Check USB connection
- Try different USB port
- Run with sudo (Linux): `sudo ./your_app`
- Check device permissions

### Build Issues
```
libusb not found
```
**Solutions**:
- Install libusb development package
- Check CMake output for specific missing dependencies
- Verify compiler C++17 support

### Permission Denied (Linux)
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER
# Log out and back in

# Or create udev rule
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="c630", MODE="0666"' | sudo tee /etc/udev/rules.d/99-lcd2usb.rules
sudo udevadm control --reload-rules
```

### GIF/BMP Demo Issues

**"Warning: Grid has >7 characters..."**
- This is expected behavior for grids larger than 7 characters
- See "HD44780 Hardware Limitations" section for details
- Use 2×3, 2×2, or 1×7 grids for best results (≤7 characters)

**"giflib not found" during build**
```bash
# macOS
brew install giflib

# Ubuntu/Debian
sudo apt install libgif-dev

# Fedora/RHEL
sudo dnf install giflib-devel
```

**GIF player shows artifacts in animations**
- Grid size exceeds 7 characters (HD44780 CGRAM limitation + avoiding character code 0)
- Solution: Use `--width 2 --height 3` (6 chars) or smaller for smooth animations
- See "HD44780 Hardware Limitations" section for technical explanation

**BMP file not supported**
- Only 24-bit and 32-bit BMP files are supported
- Convert your image: `convert input.png -type truecolor output.bmp`

## 📖 Font File Format

The library uses a custom binary format (`.lcf`) for font files:

**Structure**:
- Header: Magic signature, version, author, timestamp
- Character data: Index, name, description, frame data
- Metadata: Creation info, character count

**File Extension**: `.lcf` (LCD Character Font)

## 🤝 Contributing

Contributions are welcome! Areas for enhancement:

1. **ScrollingText System**: Implement text scrolling and marquee effects
2. **Advanced Animations**: More character animation presets
3. **Font Import/Export**: Support for other font formats
4. **Documentation**: API documentation with Doxygen
5. **Platform Support**: Windows support enhancement

## 📄 License

Licensed under either of

- Apache License, Version 2.0 ([LICENSE-Apache-2.0](LICENSE-Apache-2.0))
- MIT License ([LICENSE-MIT](LICENSE-MIT))

at your option (i.e. `MIT OR Apache-2.0`).

### Licensing rationale

Although this library drives LCD2USB devices, it contains no code from the
original LCD2USB project's host tools (whose firmware and utilities are GPL):
the USB identifiers and command codes in `include/lcd2usb/lcd2usb.hpp` are
functional hardware-interface definitions, and every implementation file here
is an original C++ design. Interoperating with a device over its documented
USB protocol does not make the host program a derived work of the device's
firmware, so the code is offered under permissive terms instead. Thanks to
Till Harbaum for the LCD2USB hardware design and protocol this library speaks.

### Third-party components

These ship with or are linked by the project and keep their own licenses:

- `assets/ChillBitmap7x.ttf` — SIL Open Font License 1.1
- `assets/boutique7x7.glyphs`, `fonts/uni668E_.glif` — derived glyph data,
  under the terms of their respective source fonts
- [giflib](https://sourceforge.net/projects/giflib/) — MIT-style license (GIF example)
- [FreeType](https://freetype.org/) — FTL / GPL-2.0 with font exception (UTF-8 pipeline)
- [ncurses](https://invisible-island.net/ncurses/) — MIT-style license (GIF example UI)
- [libusb / libusb-compat](https://libusb.info/) — LGPL-2.1, dynamically linked

## 🎯 Hardware Compatibility

**Tested With**:
- LCD2USB v1.9 firmware
- 16x2 and 20x4 character displays
- HD44780-compatible LCD controllers

**Requirements**:
- USB 2.0 or higher
- LCD2USB adapter board
- Compatible character LCD display

## 🔍 Version History

- **v1.2.0**: RsTask kanban board example
  - Live kanban board from RsTask workspaces via the `rstasks` CLI
  - Per-task CGRAM identicon icons with lane-aware footer patterns
  - Horse-racing marquee details mode (rows that fit stay static)
  - Configurable display geometry and minute-scale refresh scheduling

- **v1.2.0 (legal)**: dual-licensed under MIT OR Apache-2.0
  - LICENSE-MIT and LICENSE-Apache-2.0 added; rationale and third-party
    notices (font OFL, giflib, FreeType, ncurses, libusb LGPL) in README

- **v1.1.0**: Image and animation support
  - BMP image viewer with flexible grids
  - Animated GIF player with ncurses UI
  - MultiCharDisplay helper for grid management
  - Otsu's method auto-threshold detection
  - Bilinear interpolation for image scaling
  - Comprehensive HD44780 limitation documentation
  - Signal handling for graceful cleanup
  - Simplified initialization prevents display artifacts
  - 7 working example applications

- **v1.0.0**: Initial professional library release
  - Core LCD communication
  - Custom character system
  - Font management with binary I/O
  - 5 working example applications (including storyteller)
  - CMake build system
  - Real hardware testing complete

## 📚 Additional Documentation

- **[Manual-FontEditor.md](Manual-FontEditor.md)**: Comprehensive font editor guide
  - `.lcf` binary format specification
  - Complete API reference with examples
  - Step-by-step code walkthrough
  - Advanced techniques and best practices
  - Troubleshooting guide and debug tools

---

**The LCD2USB++ library provides a modern, professional interface for LCD2USB devices with comprehensive character management, image processing, and animation capabilities. Features include BMP/GIF support, flexible grid displays, and automatic image processing with threshold detection. Perfect for embedded projects, information displays, interactive applications, and creative visual displays. Note: Be aware of the HD44780 8-CGRAM slot limitation when designing character-based animations (see "HD44780 Hardware Limitations" section).**