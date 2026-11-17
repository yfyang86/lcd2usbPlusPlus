#ifndef MULTI_CHAR_DISPLAY_HPP
#define MULTI_CHAR_DISPLAY_HPP

#include "lcd_display.hpp"
#include "custom_char.hpp"
#include <vector>
#include <array>

namespace LCD2USB {

/**
 * Helper class for displaying grids of custom characters on multi-row LCDs
 * Handles the complexity of:
 * - Limited 8 CGRAM slots (but only 7 reliably usable)
 * - Cursor positioning across multiple rows
 * - Dynamic character slot management for grids > 7 characters
 * 
 * Strategy:
 * - ≤7 chars total: Upload all to unique slots 1-7, display directly (instant, no flicker)
 * - >7 chars: Upload chars 0-5 to slots 1-6, dynamically upload chars 6+ to slot 7
 *   Works well for static images, but will show artifacts in animations
 * 
 * ⚠️  Hardware Limitation: HD44780 LCD has 8 CGRAM slots (0-7), but character code 0
 *    may not display properly on some controllers. We use slots 1-7 for reliability.
 *    Grids >7 characters will experience display artifacts. 
 *    Recommended: 2x3, 2x2, or 1x7 grids.
 */
class MultiCharDisplay {
public:
    MultiCharDisplay(LCDDisplay& display) : display_(display) {}
    
    /**
     * Display a grid of custom characters
     * 
     * For grids with ≤7 characters:
     * - Uploads all characters to unique CGRAM slots (1-7, skipping 0)
     * - Displays the entire grid at once (instant, no flicker)
     * 
     * For grids with >7 characters:
     * - Uploads chars 0-5 to slots 1-6
     * - Dynamically uploads chars 6+ to slot 7 one at a time
     * - Works for static images but shows artifacts in animations
     * 
     * Note: We skip CGRAM slot 0 because character code 0 may not display
     * properly on some HD44780 controllers.
     * 
     * @param chars Vector of custom characters to display (row-major order)
     * @param width Grid width (characters per row)
     * @param height Grid height (number of rows)
     */
    void displayGrid(const std::vector<CustomChar>& chars, int width, int height);
    
    /**
     * Display a grid using raw byte patterns instead of CustomChar objects
     * Useful for image processing pipelines
     */
    void displayGrid(const std::vector<std::array<uint8_t, 8>>& patterns, 
                    int width, int height);

private:
    LCDDisplay& display_;
};

} // namespace LCD2USB

#endif // MULTI_CHAR_DISPLAY_HPP
