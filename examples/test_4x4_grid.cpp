/**
 * Test program to display a 4x4 grid of custom characters on LCD
 */

#include <iostream>
#include <thread>
#include <chrono>
#include "lcd2usb/lcd_display.hpp"
#include "lcd2usb/custom_char.hpp"

using namespace LCD2USB;

int main() {
    std::cout << "=== Testing 4x4 Custom Character Grid ===" << std::endl;
    
    try {
        LCDDisplay display;
        if (!display.initialize()) {
            std::cerr << "Failed to initialize LCD display" << std::endl;
            return -1;
        }
        
        std::cout << "✓ LCD initialized" << std::endl;
        
        // Configure display
        display.setContrast(200);
        display.setBrightness(255);
        
        // Create 16 different patterns (one for each position) using CustomChar
        std::vector<CustomChar> patterns;
        
        // Pattern 0-3: Full blocks with different alignments
        patterns.push_back(CustomChar({"#####", "#####", "#####", "     ", "     ", "     ", "     ", "     "})); // Top
        patterns.push_back(CustomChar({"     ", "     ", "     ", "#####", "#####", "#####", "     ", "     "})); // Middle
        patterns.push_back(CustomChar({"     ", "     ", "     ", "     ", "     ", "#####", "#####", "#####"})); // Bottom
        patterns.push_back(CustomChar({"#####", "#####", "#####", "#####", "#####", "#####", "#####", "#####"})); // Full
        
        // Pattern 4-7: Different densities
        patterns.push_back(CustomChar({"# # #", " # # ", "# # #", " # # ", "# # #", " # # ", "# # #", " # # "})); // Checkerboard
        patterns.push_back(CustomChar({"#####", "     ", "#####", "     ", "#####", "     ", "#####", "     "})); // Horizontal lines
        patterns.push_back(CustomChar({"#   #", "#   #", "#   #", "#   #", "#   #", "#   #", "#   #", "#   #"})); // Vertical lines
        patterns.push_back(CustomChar({"# # #", "# # #", "# # #", "# # #", "# # #", "# # #", "# # #", "# # #"})); // Dense dots
        
        // Pattern 8-15: More variations (will use dynamic slot 7)
        patterns.push_back(CustomChar({"#####", "#   #", "#   #", "#   #", "#   #", "#   #", "#   #", "#####"}));  // Frame
        patterns.push_back(CustomChar({" ### ", "#   #", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}));  // Circle
        patterns.push_back(CustomChar({"  #  ", " ### ", "# # #", "  #  ", "  #  ", "  #  ", "  #  ", "  #  "})); // Arrow up
        patterns.push_back(CustomChar({"  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "# # #", " ### ", "  #  "})); // Arrow down
        patterns.push_back(CustomChar({"     ", "  #  ", " #   ", "#####", " #   ", "  #  ", "     ", "     "})); // Arrow left
        patterns.push_back(CustomChar({"     ", "  #  ", "   # ", "#####", "   # ", "  #  ", "     ", "     "})); // Arrow right
        patterns.push_back(CustomChar({" # # ", "# # #", " # # ", "# # #", " # # ", "# # #", " # # ", "# # #"})); // Dense checkerboard
        patterns.push_back(CustomChar({"###  ", "#  # ", "###  ", "  #  ", " ### ", "  #  ", "  #  ", "  #  "})); // Number-like
        
        // Clear display
        display.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Upload first 8 characters to slots 0-7
        std::cout << "Uploading first 8 custom characters..." << std::endl;
        for (int i = 0; i < 8; i++) {
            display.uploadCustomCharacter(i, patterns[i]);
        }
        std::cout << "✓ First 8 characters uploaded" << std::endl;
        
        // Display 4x4 grid
        std::cout << "Displaying 4x4 grid:" << std::endl;
        int charIndex = 0;
        for (int row = 0; row < 4; row++) {
            std::cout << "  Row " << row << ": ";
            display.setCursor(0, row);
            
            for (int col = 0; col < 4; col++) {
                if (charIndex < 8) {
                    // Use pre-uploaded characters 0-7
                    display.print(static_cast<char>(charIndex));
                    std::cout << "char" << charIndex << " ";
                } else {
                    // For characters 8-15, upload to slot 7 and display
                    display.uploadCustomCharacter(7, patterns[charIndex]);
                    // CRITICAL: Restore cursor position after upload resets it
                    display.setCursor(col, row);
                    display.print(static_cast<char>(7));
                    std::cout << "char" << charIndex << "(->slot7) ";
                }
                charIndex++;
            }
            std::cout << std::endl;
        }
        
        std::cout << "\n✓ 4x4 grid displayed!" << std::endl;
        std::cout << "Check your LCD - you should see a 4x4 grid of different patterns" << std::endl;
        std::cout << "Row 0: Top-aligned, Middle, Bottom, Full blocks" << std::endl;
        std::cout << "Row 1: Checkerboard, Horizontal lines, Vertical lines, Dense dots" << std::endl;
        std::cout << "Row 2: Frame, Circle, Arrow up, Arrow down" << std::endl;
        std::cout << "Row 3: Arrow left, Arrow right, Dense checkerboard, Number" << std::endl;
        std::cout << "\nPress Ctrl+C to exit..." << std::endl;
        
        // Keep running
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
