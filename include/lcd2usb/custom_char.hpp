/*
 * lcd2usb/custom_char.hpp - Custom character creation and animation
 * Part of LCD2USB++ library
 */

#pragma once

#include "lcd2usb.hpp"
#include <array>
#include <vector>
#include <string>
#include <cstdint>

namespace LCD2USB {

/**
 * Represents a single 5x8 LCD custom character
 */
class CustomChar {
private:
    std::array<std::array<bool, LCD::CUSTOM_CHAR_WIDTH>, LCD::CUSTOM_CHAR_HEIGHT> bitmap_;

public:
    CustomChar();
    explicit CustomChar(const std::vector<std::string>& pattern);
    explicit CustomChar(const std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT>& bytes);

    // Bitmap manipulation
    void clear();
    void setPixel(int x, int y, bool value);
    bool getPixel(int x, int y) const;
    
    // Pattern loading
    void setFromPattern(const std::vector<std::string>& pattern);
    void setFromBytes(const std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT>& bytes);
    
    // Data export
    std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT> toBytes() const;
    std::string toString() const;
    std::string getHexBytes() const;
};

/**
 * Factory class for creating animated custom characters
 */
class CharacterFactory {
public:
    // Animated jumping match guy (4 frames)
    static CustomChar createMatchGuy(int frame);
    
    // Loading spinner animation (4 frames)
    static CustomChar createSpinner(int frame);
    
    // Dancing figure animation (2 frames)
    static CustomChar createDancer(int frame);
    
    // Progress bar elements
    static CustomChar createProgressBar(int level); // 0-5 levels
    
    // Arrows and indicators
    static CustomChar createArrow(int direction); // 0=up, 1=right, 2=down, 3=left
    
    // Simple shapes
    static CustomChar createHeart();
    static CustomChar createSmiley();
    static CustomChar createNote();
};

} // namespace LCD2USB