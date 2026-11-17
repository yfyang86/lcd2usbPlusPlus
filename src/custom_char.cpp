/*
 * custom_char.cpp - Custom character implementation
 * Part of LCD2USB++ library
 */

#include "lcd2usb/custom_char.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace LCD2USB {

// CustomChar implementation
CustomChar::CustomChar() {
    clear();
}

CustomChar::CustomChar(const std::vector<std::string>& pattern) : CustomChar() {
    setFromPattern(pattern);
}

CustomChar::CustomChar(const std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT>& bytes) : CustomChar() {
    setFromBytes(bytes);
}

void CustomChar::clear() {
    for (auto& row : bitmap_) {
        row.fill(false);
    }
}

void CustomChar::setPixel(int x, int y, bool value) {
    if (x >= 0 && x < LCD::CUSTOM_CHAR_WIDTH && y >= 0 && y < LCD::CUSTOM_CHAR_HEIGHT) {
        bitmap_[y][x] = value;
    }
}

bool CustomChar::getPixel(int x, int y) const {
    if (x >= 0 && x < LCD::CUSTOM_CHAR_WIDTH && y >= 0 && y < LCD::CUSTOM_CHAR_HEIGHT) {
        return bitmap_[y][x];
    }
    return false;
}

void CustomChar::setFromPattern(const std::vector<std::string>& pattern) {
    clear();
    for (size_t y = 0; y < pattern.size() && y < LCD::CUSTOM_CHAR_HEIGHT; ++y) {
        for (size_t x = 0; x < pattern[y].length() && x < LCD::CUSTOM_CHAR_WIDTH; ++x) {
            bitmap_[y][x] = (pattern[y][x] != ' ' && pattern[y][x] != '.' && pattern[y][x] != '0');
        }
    }
}

void CustomChar::setFromBytes(const std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT>& bytes) {
    clear();
    for (int y = 0; y < LCD::CUSTOM_CHAR_HEIGHT; ++y) {
        for (int x = 0; x < LCD::CUSTOM_CHAR_WIDTH; ++x) {
            bitmap_[y][x] = (bytes[y] & (1 << (LCD::CUSTOM_CHAR_WIDTH - 1 - x))) != 0;
        }
    }
}

std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT> CustomChar::toBytes() const {
    std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT> bytes{};
    for (int y = 0; y < LCD::CUSTOM_CHAR_HEIGHT; ++y) {
        uint8_t byte = 0;
        for (int x = 0; x < LCD::CUSTOM_CHAR_WIDTH; ++x) {
            if (bitmap_[y][x]) {
                byte |= (1 << (LCD::CUSTOM_CHAR_WIDTH - 1 - x));
            }
        }
        bytes[y] = byte;
    }
    return bytes;
}

std::string CustomChar::toString() const {
    std::string result;
    for (int y = 0; y < LCD::CUSTOM_CHAR_HEIGHT; ++y) {
        for (int x = 0; x < LCD::CUSTOM_CHAR_WIDTH; ++x) {
            result += bitmap_[y][x] ? "#" : ".";
        }
        result += "\n";
    }
    return result;
}

std::string CustomChar::getHexBytes() const {
    auto bytes = toBytes();
    std::stringstream ss;
    for (int i = 0; i < LCD::CUSTOM_CHAR_HEIGHT; ++i) {
        if (i > 0) ss << ", ";
        ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(bytes[i]);
    }
    return ss.str();
}

// CharacterFactory implementation
CustomChar CharacterFactory::createMatchGuy(int frame) {
    switch (frame) {
        case 0:  // Standing
            return CustomChar({
                "  #  ",   // Head
                " ### ",   // Body
                "  #  ",   // Waist  
                " ### ",   // Arms spread
                "  #  ",   // Leg joint
                "  #  ",   // Upper leg
                " # # ",   // Lower legs
                "#   #"    // Feet apart
            });
        case 1:  // Crouching
            return CustomChar({
                "     ",
                "  #  ",   // Head
                " ### ",   // Body compressed
                "#####",   // Arms wide, crouched
                "  #  ",   // Compressed torso
                "  #  ",   // Leg joint
                " ### ",   // Legs together, bent
                "     "
            });
        case 2:  // Mid-jump
            return CustomChar({
                "#   #",   // Arms raised high
                " # # ",   // Upper arms
                " ### ",   // Shoulders/body
                "  #  ",   // Torso
                "  #  ",   // Waist
                "  #  ",   // Hip
                " # # ",   // Legs spread mid-air
                "     "    // Clear ground
            });
        case 3:  // Peak jump
        default:
            return CustomChar({
                "     ",
                "  #  ",   // Head at top
                " ### ",   // Body
                " ### ",   // Arms spread
                "  #  ",   // Torso
                "  #  ",   // Waist
                "  #  ",   // Single leg line
                " ### "    // Legs tucked
            });
    }
}

CustomChar CharacterFactory::createSpinner(int frame) {
    switch (frame % 4) {
        case 0: // Vertical line
            return CustomChar({
                "  #  ",
                "  #  ", 
                "  #  ",
                "  #  ",
                "  #  ",
                "  #  ",
                "  #  ",
                "     "
            });
        case 1: // Diagonal backslash
            return CustomChar({
                "    #",
                "   # ",
                "  #  ",
                " #   ",
                "#    ",
                "     ",
                "     ",
                "     "
            });
        case 2: // Horizontal line
            return CustomChar({
                "     ",
                "     ",
                "     ",
                "#####",
                "     ",
                "     ",
                "     ",
                "     "
            });
        case 3: // Diagonal forward slash
        default:
            return CustomChar({
                "#    ",
                " #   ",
                "  #  ",
                "   # ",
                "    #",
                "     ",
                "     ",
                "     "
            });
    }
}

CustomChar CharacterFactory::createDancer(int frame) {
    if (frame % 2 == 0) {  // Left pose
        return CustomChar({
            " #   ",   // Head tilted
            "#### ",   // Arms left
            " ### ",   // Body
            "  #  ",   // Waist
            " ##  ",   // Hips left
            " #   ",   // Left leg
            "#    ",   // Left foot
            "   ##"    // Right leg/foot
        });
    } else {  // Right pose
        return CustomChar({
            "   # ",   // Head tilted right
            " ####",   // Arms right
            " ### ",   // Body
            "  #  ",   // Waist
            "  ## ",   // Hips right
            "   # ",   // Right leg
            "    #",   // Right foot
            "##   "    // Left leg/foot
        });
    }
}

CustomChar CharacterFactory::createProgressBar(int level) {
    level = std::max(0, std::min(5, level)); // Clamp to 0-5
    
    std::vector<std::string> pattern = {
        "#####",   // Top border
        "#   #",   // Side borders
        "#   #",
        "#   #",
        "#   #",
        "#   #",
        "#   #",
        "#####"    // Bottom border
    };
    
    // Fill progress from bottom up
    for (int y = 7 - level; y < 7; ++y) {
        if (y > 0 && y < 7) {  // Don't overwrite borders
            pattern[y] = "###";
        }
    }
    
    return CustomChar(pattern);
}

CustomChar CharacterFactory::createArrow(int direction) {
    switch (direction % 4) {
        case 0: // Up
            return CustomChar({
                "  #  ",
                " ### ",
                "#####",
                "  #  ",
                "  #  ",
                "  #  ",
                "  #  ",
                "     "
            });
        case 1: // Right
            return CustomChar({
                "     ",
                "   # ",
                "   ##",
                "#####",
                "#####",
                "   ##",
                "   # ",
                "     "
            });
        case 2: // Down
            return CustomChar({
                "     ",
                "  #  ",
                "  #  ",
                "  #  ",
                "  #  ",
                "#####",
                " ### ",
                "  #  "
            });
        case 3: // Left
        default:
            return CustomChar({
                "     ",
                " #   ",
                "##   ",
                "#####",
                "#####",
                "##   ",
                " #   ",
                "     "
            });
    }
}

CustomChar CharacterFactory::createHeart() {
    return CustomChar({
        "     ",
        " # # ",
        "#####",
        "#####",
        " ### ",
        "  #  ",
        "     ",
        "     "
    });
}

CustomChar CharacterFactory::createSmiley() {
    return CustomChar({
        "     ",
        " ### ",
        "# # #",
        "#   #",
        "# # #",
        "#   #",
        " ### ",
        "     "
    });
}

CustomChar CharacterFactory::createNote() {
    return CustomChar({
        "   ##",
        "   # ",
        "   # ",
        " ### ",
        "#####",
        "#####",
        " ### ",
        "     "
    });
}

} // namespace LCD2USB