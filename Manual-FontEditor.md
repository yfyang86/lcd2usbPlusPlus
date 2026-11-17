# LCD2USB++ Font Editor Manual

## Overview

The LCD2USB++ Font Editor is a professional font management system for creating, editing, and managing custom LCD character collections. It provides a binary file format (`.lcf` - LCD Character Font) for efficient storage and a complete API for programmatic font manipulation.

## Table of Contents

1. [Font File Format (.lcf)](#font-file-format-lcf)
2. [API Reference](#api-reference)
3. [Basic Usage](#basic-usage)
4. [Example Code Walkthrough](#example-code-walkthrough)
5. [Advanced Techniques](#advanced-techniques)
6. [Best Practices](#best-practices)
7. [Troubleshooting](#troubleshooting)

## Font File Format (.lcf)

The LCD Character Font (`.lcf`) format is a compact binary format designed for efficient storage and fast loading of custom character collections.

### File Structure

```
┌─────────────────┬──────────────┬─────────────────────────┐
│ Header          │ Metadata     │ Character Data          │
│ (Fixed Size)    │ (Variable)   │ (Variable)              │
└─────────────────┴──────────────┴─────────────────────────┘
```

### Header Format (32 bytes)

| Offset | Size | Type   | Field           | Description                    |
|--------|------|--------|-----------------|--------------------------------|
| 0x00   | 4    | char   | Magic           | "LCF\0" - File type identifier |
| 0x04   | 2    | uint16 | Version         | Format version (currently 1)   |
| 0x06   | 2    | uint16 | Character Count | Number of characters in font   |
| 0x08   | 4    | uint32 | Metadata Length | Size of metadata section       |
| 0x0C   | 20   | char   | Reserved        | Reserved for future use        |

### Metadata Section

The metadata section contains variable-length strings in the following order:

1. **Author Name** - Font creator identification
2. **Description** - Font purpose or description
3. **Creation Time** - ISO 8601 timestamp (YYYY-MM-DDTHH:mm:ss)

Each string is stored with a 2-byte length prefix followed by the UTF-8 encoded text.

### Character Data Section

For each character, the following data is stored:

| Field               | Size | Type    | Description                           |
|--------------------|------|---------|---------------------------------------|
| Character Index    | 1    | uint8   | LCD character slot (0-7 for custom)  |
| Bitmap Data        | 8    | uint8[] | 5x8 pixel bitmap (8 rows of 5 bits)  |
| Metadata Length    | 2    | uint16  | Size of character metadata           |
| Metadata           | var  | string  | Character description (UTF-8)        |

### Bitmap Encoding

Each character is represented as a 5×8 pixel bitmap:
- **8 rows** of pixel data
- **5 bits per row** (bits 0-4 used, bits 5-7 ignored)
- **1 = pixel on, 0 = pixel off**

Example heart character:
```
Row 0: 01010 → 0x0A
Row 1: 11111 → 0x1F  
Row 2: 11111 → 0x1F
Row 3: 01110 → 0x0E
Row 4: 00100 → 0x04
Row 5: 00000 → 0x00
Row 6: 00000 → 0x00
Row 7: 00000 → 0x00
```

## API Reference

### Core Classes

#### `LCD2USB::FontEdit::FontManager`

Main class for font collection management.

**Constructor:**
```cpp
FontManager();
FontManager(const std::string& author, const std::string& description);
```

**File Operations:**
```cpp
bool saveToFile(const std::string& filename);
bool loadFromFile(const std::string& filename);
```

**Character Management:**
```cpp
void addCharacter(uint8_t index, const CharacterBitmap& character);
bool removeCharacter(uint8_t index);
bool hasCharacter(uint8_t index) const;
CharacterBitmap getCharacter(uint8_t index) const;
```

**Metadata:**
```cpp
void setAuthor(const std::string& author);
void setDescription(const std::string& description);
std::string getAuthor() const;
std::string getDescription() const;
std::string getCreationTime() const;
size_t getCharacterCount() const;
```

#### `LCD2USB::FontEdit::CharacterBitmap`

Represents a single custom character with metadata.

**Constructor:**
```cpp
CharacterBitmap();
CharacterBitmap(const std::array<uint8_t, 8>& bitmap_data);
```

**Bitmap Manipulation:**
```cpp
void setFrame(int frame_index, const std::array<uint8_t, 8>& data);
std::array<uint8_t, 8> getFrame(int frame_index) const;
void clearFrame(int frame_index);
```

**Metadata:**
```cpp
void setDescription(const std::string& description);
std::string getDescription() const;
```

**Conversion:**
```cpp
LCD2USB::CustomChar toCustomChar() const;
static CharacterBitmap fromCustomChar(const LCD2USB::CustomChar& custom_char);
```

## Basic Usage

### Creating a Simple Font

```cpp
#include "lcd2usb/lcd2usb_lib.hpp"
using namespace LCD2USB;
using namespace LCD2USB::FontEdit;

int main() {
    // Create font manager
    FontManager font_manager("John Doe", "My Custom Icons");
    
    // Create heart character
    auto heart = CharacterFactory::createHeart();
    CharacterBitmap heart_bitmap = CharacterBitmap::fromCustomChar(heart);
    heart_bitmap.setDescription("Love symbol");
    
    // Add to font
    font_manager.addCharacter(0, heart_bitmap);
    
    // Save font
    font_manager.saveToFile("icons.lcf");
    
    return 0;
}
```

### Loading and Using a Font

```cpp
#include "lcd2usb/lcd2usb_lib.hpp"
using namespace LCD2USB;
using namespace LCD2USB::FontEdit;

int main() {
    // Initialize LCD
    LCDDisplay display;
    if (!display.initialize()) {
        return -1;
    }
    
    // Load font
    FontManager font_manager;
    if (!font_manager.loadFromFile("icons.lcf")) {
        std::cerr << "Failed to load font" << std::endl;
        return -1;
    }
    
    // Upload characters to LCD
    if (font_manager.hasCharacter(0)) {
        CharacterBitmap char_bitmap = font_manager.getCharacter(0);
        CustomChar custom_char = char_bitmap.toCustomChar();
        display.uploadCustomCharacter(0, custom_char);
    }
    
    // Display character
    display.clear();
    display.setCursor(0, 0);
    display.print('\0');  // Print custom character 0
    
    return 0;
}
```

## Example Code Walkthrough

Let's examine the complete font editor demo (`examples/font_editor_demo_simple.cpp`):

### Step 1: Initialization

```cpp
#include "lcd2usb/lcd2usb_lib.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace LCD2USB;
using namespace LCD2USB::FontEdit;

int main() {
    std::cout << "=== LCD2USB++ Font Editor Demo ===" << std::endl;
    
    // Initialize display
    LCDDisplay display;
    if (!display.initialize()) {
        std::cerr << "Failed to initialize LCD display" << std::endl;
        return -1;
    }
```

**Explanation:**
- Includes necessary headers for LCD2USB++ library
- Uses both main LCD2USB namespace and FontEdit sub-namespace
- Initializes the LCD display with error checking

### Step 2: Creating Character Library

```cpp
    // Create some pre-made characters for testing
    auto heart = CharacterFactory::createHeart();
    auto note = CharacterFactory::createNote();
    auto spinner = CharacterFactory::createSpinner(0);
    auto match_guy = CharacterFactory::createMatchGuy(0);
```

**Explanation:**
- Uses the `CharacterFactory` to create predefined characters
- `createSpinner(0)` and `createMatchGuy(0)` create frame 0 of animated sequences
- These are `CustomChar` objects that need conversion for font storage

### Step 3: Font Manager Setup

```cpp
    // Create a font manager with metadata
    FontManager font_manager("Demo Author", "Font Editor Demonstration");
    
    // Convert CustomChar objects to CharacterBitmap for font storage
    CharacterBitmap heart_bitmap = CharacterBitmap::fromCustomChar(heart);
    heart_bitmap.setDescription("Heart symbol for love");
    
    CharacterBitmap note_bitmap = CharacterBitmap::fromCustomChar(note);
    note_bitmap.setDescription("Musical note");
```

**Explanation:**
- Creates a `FontManager` with author and description metadata
- Converts `CustomChar` objects to `CharacterBitmap` for font storage
- Adds descriptive metadata to each character
- `CharacterBitmap` provides the persistence layer that `CustomChar` lacks

### Step 4: Font Population

```cpp
    // Add characters to font collection
    font_manager.addCharacter(0, heart_bitmap);
    font_manager.addCharacter(1, note_bitmap);
    
    std::cout << "Created font with " << font_manager.getCharacterCount() 
              << " characters" << std::endl;
    std::cout << "Author: " << font_manager.getAuthor() << std::endl;
    std::cout << "Description: " << font_manager.getDescription() << std::endl;
```

**Explanation:**
- Adds characters to specific slots (0 and 1)
- LCD2USB supports custom character slots 0-7
- Displays font metadata for verification
- `getCharacterCount()` returns the number of characters in the font

### Step 5: LCD Display Upload

```cpp
    // Upload characters to LCD for display
    display.uploadCustomCharacter(0, heart);
    display.uploadCustomCharacter(1, note);
    
    // Display the characters
    display.clear();
    display.setCursor(0, 0);
    display.print("Font Demo:");
    display.setCursor(0, 1);
    display.print('\0');  // Heart (custom char 0)
    display.print(' ');
    display.print('\1');  // Note (custom char 1)
```

**Explanation:**
- Uploads the original `CustomChar` objects to LCD hardware slots
- Custom characters are accessed using char codes 0-7
- `print('\0')` displays custom character from slot 0
- Creates a simple display showing both characters

### Step 6: File Persistence

```cpp
    std::cout << "Displaying characters on LCD for 3 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Save font to file
    std::string filename = "demo_font.lcf";
    if (font_manager.saveToFile(filename)) {
        std::cout << "Font saved to " << filename << std::endl;
    } else {
        std::cerr << "Failed to save font to " << filename << std::endl;
        return -1;
    }
```

**Explanation:**
- Pauses to show the display to user
- Saves the font collection to a binary `.lcf` file
- The file contains all characters plus metadata
- Error checking ensures save operation succeeded

### Step 7: Verification Load

```cpp
    // Demonstrate loading the saved font
    FontManager loaded_font;
    if (loaded_font.loadFromFile(filename)) {
        std::cout << "Successfully loaded font from " << filename << std::endl;
        std::cout << "Loaded font info:" << std::endl;
        std::cout << "  Author: " << loaded_font.getAuthor() << std::endl;
        std::cout << "  Description: " << loaded_font.getDescription() << std::endl;
        std::cout << "  Created: " << loaded_font.getCreationTime() << std::endl;
        std::cout << "  Characters: " << loaded_font.getCharacterCount() << std::endl;
    }
```

**Explanation:**
- Creates a new `FontManager` instance to test loading
- Loads the previously saved font file
- Displays all metadata to verify integrity
- `getCreationTime()` shows when the font was originally created

### Step 8: Character Verification

```cpp
    // Verify character data integrity
    if (loaded_font.hasCharacter(0)) {
        CharacterBitmap loaded_heart = loaded_font.getCharacter(0);
        std::cout << "  Heart character description: " 
                  << loaded_heart.getDescription() << std::endl;
    }
    
    if (loaded_font.hasCharacter(1)) {
        CharacterBitmap loaded_note = loaded_font.getCharacter(1);
        std::cout << "  Note character description: " 
                  << loaded_note.getDescription() << std::endl;
    }
```

**Explanation:**
- Checks if specific characters exist using `hasCharacter()`
- Retrieves individual characters with `getCharacter()`
- Displays character-specific metadata
- Verifies complete round-trip data integrity

## Advanced Techniques

### Animated Character Fonts

```cpp
FontManager animation_font("Animator", "Character animations");

// Create multi-frame spinner
for (int frame = 0; frame < 4; frame++) {
    auto spinner = CharacterFactory::createSpinner(frame);
    CharacterBitmap spinner_bitmap = CharacterBitmap::fromCustomChar(spinner);
    spinner_bitmap.setDescription("Spinner frame " + std::to_string(frame));
    animation_font.addCharacter(frame, spinner_bitmap);
}

animation_font.saveToFile("animations.lcf");
```

### Custom Character Creation

```cpp
// Create custom character using pixel manipulation
CustomChar custom;
custom.clear();

// Draw diagonal line
for (int i = 0; i < 5; i++) {
    custom.setPixel(i, i, true);
}

CharacterBitmap custom_bitmap = CharacterBitmap::fromCustomChar(custom);
custom_bitmap.setDescription("Diagonal line");
```

### Font Merging

```cpp
FontManager font1, font2, merged_font("Merger", "Combined fonts");

font1.loadFromFile("icons.lcf");
font2.loadFromFile("symbols.lcf");

// Copy characters from font1
for (uint8_t i = 0; i < 8; i++) {
    if (font1.hasCharacter(i)) {
        merged_font.addCharacter(i, font1.getCharacter(i));
    }
}

// Copy non-conflicting characters from font2
for (uint8_t i = 0; i < 8; i++) {
    if (font2.hasCharacter(i) && !merged_font.hasCharacter(i)) {
        merged_font.addCharacter(i, font2.getCharacter(i));
    }
}

merged_font.saveToFile("merged.lcf");
```

## Best Practices

### 1. Metadata Management

```cpp
// Always provide meaningful metadata
FontManager font("Developer Name", "Descriptive purpose");

CharacterBitmap char_bitmap;
char_bitmap.setDescription("Clear purpose description");
```

### 2. Error Handling

```cpp
// Always check file operations
if (!font_manager.saveToFile("font.lcf")) {
    std::cerr << "Save failed - check disk space and permissions" << std::endl;
    return -1;
}

if (!font_manager.loadFromFile("font.lcf")) {
    std::cerr << "Load failed - check file exists and is valid" << std::endl;
    return -1;
}
```

### 3. Character Slot Management

```cpp
// Document your character slot usage
const uint8_t HEART_SLOT = 0;
const uint8_t ARROW_UP_SLOT = 1;
const uint8_t ARROW_DOWN_SLOT = 2;
const uint8_t PROGRESS_BAR_SLOT = 3;

font_manager.addCharacter(HEART_SLOT, heart_bitmap);
font_manager.addCharacter(ARROW_UP_SLOT, arrow_bitmap);
```

### 4. Version Control

```cpp
// Include version in font description
FontManager font("Author", "Icon Set v2.1 - Updated arrows and symbols");
```

## Troubleshooting

### Common Issues

#### File Save/Load Failures

**Problem**: `saveToFile()` or `loadFromFile()` returns false

**Solutions**:
- Check file permissions and disk space
- Verify directory exists
- Ensure filename has `.lcf` extension
- Check for file lock by other processes

#### Character Display Issues

**Problem**: Custom characters don't display correctly

**Solutions**:
- Verify character uploaded to correct LCD slot (0-7)
- Check bitmap data isn't corrupted
- Ensure `CustomChar` conversion is correct
- Verify LCD initialization succeeded

#### Metadata Encoding Issues

**Problem**: Special characters in metadata appear corrupted

**Solutions**:
- Use UTF-8 compatible strings
- Avoid null bytes in descriptions
- Keep metadata reasonably short (< 256 chars)

### Debug Techniques

#### Font Inspection

```cpp
void debugFont(const FontManager& font) {
    std::cout << "Font Debug Info:" << std::endl;
    std::cout << "  Author: " << font.getAuthor() << std::endl;
    std::cout << "  Description: " << font.getDescription() << std::endl;
    std::cout << "  Created: " << font.getCreationTime() << std::endl;
    std::cout << "  Character count: " << font.getCharacterCount() << std::endl;
    
    for (uint8_t i = 0; i < 8; i++) {
        if (font.hasCharacter(i)) {
            CharacterBitmap char_bitmap = font.getCharacter(i);
            std::cout << "  Slot " << (int)i << ": " 
                      << char_bitmap.getDescription() << std::endl;
        }
    }
}
```

#### Bitmap Visualization

```cpp
void printCharacterBitmap(const CustomChar& character) {
    auto bytes = character.toBytes();
    std::cout << "Character bitmap:" << std::endl;
    for (int row = 0; row < 8; row++) {
        std::cout << "  ";
        for (int col = 0; col < 5; col++) {
            std::cout << ((bytes[row] & (1 << col)) ? "█" : "░");
        }
        std::cout << " (0x" << std::hex << (int)bytes[row] << ")" << std::endl;
    }
}
```

## File Format Specification

### Magic Number Verification

```cpp
bool isValidLCFFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    
    char magic[4];
    file.read(magic, 4);
    return (magic[0] == 'L' && magic[1] == 'C' && 
            magic[2] == 'F' && magic[3] == '\0');
}
```

### Manual File Creation

For advanced users who need to create `.lcf` files programmatically:

```cpp
// This is handled automatically by FontManager, but shown for reference
struct LCFHeader {
    char magic[4];      // "LCF\0"
    uint16_t version;   // Format version
    uint16_t char_count;// Number of characters
    uint32_t meta_len;  // Metadata section length
    char reserved[20];  // Reserved space
};
```

## Conclusion

The LCD2USB++ Font Editor provides a complete solution for custom character management with the `.lcf` binary format offering efficient storage and fast loading. The API supports both simple character creation and advanced font management workflows.

Key benefits:
- **Efficient binary format** for fast loading
- **Rich metadata support** for character documentation  
- **Complete round-trip integrity** (save/load preserves all data)
- **Integration with CustomChar system** for seamless LCD display
- **Professional API** with comprehensive error handling

For additional examples and advanced usage patterns, see the `examples/` directory and the main README.md file.