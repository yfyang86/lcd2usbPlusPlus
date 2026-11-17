/*
 * lcd2usb/lcd2usb_lib.hpp - Main include file for LCD2USB++ library
 * Part of LCD2USB++ library
 */

#pragma once

// Core functionality
#include "lcd2usb.hpp"
#include "lcd_display.hpp"

// Custom characters and animations
#include "custom_char.hpp"

// Font management
#include "font_editor.hpp"

// UTF-8 text pipeline
#include "utf8.hpp"
#include "glyph_table.hpp"
#include "unicode_display.hpp"
#include "glyph_source.hpp"
#include "glyph_pack.hpp"
#include "bitmap_font.hpp"

// Utilities
// #include "scrolling_text.hpp"  // Temporarily disabled

/**
 * LCD2USB++ Library
 * 
 * A modern C++17 library for interfacing with LCD2USB devices.
 * 
 * Features:
 * - USB communication with LCD2USB hardware
 * - Custom character creation and animation
 * - Font file management (.lcf format)
 * - Scrolling text with animated padding
 * - Application framework for LCD applications
 * 
 * Example usage:
 * 
 * #include <lcd2usb/lcd2usb_lib.hpp>
 * using namespace LCD2USB;
 * 
 * int main() {
 *     LCDDisplay display;
 *     if (display.initialize()) {
 *         display.clear();
 *         display.writeToLine(1, "Hello, World!");
 *     }
 *     return 0;
 * }
 */

namespace LCD2USB {
    // Library version
    constexpr int VERSION_MAJOR = 1;
    constexpr int VERSION_MINOR = 0;
    constexpr int VERSION_PATCH = 0;
    constexpr const char* VERSION_STRING = "1.0.0";
}