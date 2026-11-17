#include "lcd2usb/multi_char_display.hpp"
#include <algorithm>
#include <cstring>

namespace LCD2USB {

void MultiCharDisplay::displayGrid(const std::vector<CustomChar>& chars, 
                                   int width, int height) {
    // Convert CustomChar objects to raw patterns
    std::vector<std::array<uint8_t, 8>> patterns;
    patterns.reserve(chars.size());
    
    for (const auto& ch : chars) {
        patterns.push_back(ch.toBytes());
    }
    
    displayGrid(patterns, width, height);
}

void MultiCharDisplay::displayGrid(const std::vector<std::array<uint8_t, 8>>& patterns,
                                   int width, int height) {
    int totalChars = patterns.size();
    
    // Strategy 1: If 8 or fewer characters, upload all and display directly (fast)
    // Note: We use slots 1-7 (skipping slot 0) because character code 0 may not display properly on some LCDs
    if (totalChars <= 7) {
        // Upload to slots 1-7 (skip slot 0 to avoid character code 0 display issues)
        for (int i = 0; i < totalChars; i++) {
            display_.uploadCustomCharacter(i + 1, patterns[i]);
        }
        
        int charIndex = 0;
        for (int row = 0; row < height && charIndex < totalChars; row++) {
            display_.setCursor(0, row);
            for (int col = 0; col < width && charIndex < totalChars; col++) {
                // Use slot numbers 1-7 (charIndex + 1)
                display_.print(static_cast<char>(charIndex + 1));
                charIndex++;
            }
        }
        return;
    }
    
    // Strategy 2: More than 7 characters - use slots 1-6 for first 6 chars, slot 7 for dynamic updates
    // This works for static images but will show artifacts in animations
    // We avoid slot 0 because character code 0 may not display properly
    
    // Upload first 6 characters to slots 1-6
    int initialUpload = std::min(totalChars, 6);
    for (int i = 0; i < initialUpload; i++) {
        display_.uploadCustomCharacter(i + 1, patterns[i]);
    }
    
    // Display grid position by position
    int charIndex = 0;
    for (int row = 0; row < height && charIndex < totalChars; row++) {
        display_.setCursor(0, row);
        
        for (int col = 0; col < width && charIndex < totalChars; col++) {
            if (charIndex < 6) {
                // First 6 characters can be displayed directly using slots 1-6
                display_.print(static_cast<char>(charIndex + 1));
            } else {
                // Characters 6+ need dynamic upload to slot 7
                display_.uploadCustomCharacter(7, patterns[charIndex]);
                display_.setCursor(col, row);  // Restore cursor after upload
                display_.print(static_cast<char>(7));
            }
            charIndex++;
        }
    }
}

} // namespace LCD2USB
