/*
 * custom_char_demo.cpp - Custom character demonstration
 * Example using the new library structure
 */

#include "lcd2usb/lcd2usb_lib.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace LCD2USB;

int main() {
    std::cout << "=== LCD2USB++ Custom Character Demo ===" << std::endl;
    
    // Initialize display
    LCDDisplay display;
    if (!display.initialize()) {
        std::cerr << "Failed to initialize LCD display" << std::endl;
        return -1;
    }
    
    std::cout << "LCD2USB device initialized!" << std::endl;
    
    // Configure display
    display.setContrast(200);
    display.setBrightness(255);
    display.clear();
    
    // Demo 1: Heart character
    std::cout << "\nDemo 1: Creating heart character..." << std::endl;
    auto heart = CharacterFactory::createHeart();
    display.uploadCustomCharacter(0, heart);
    
    display.setCursor(0, 0);
    display.print("I ");
    display.print(static_cast<char>(0)); // Heart character
    display.print(" LCD2USB++!");
    display.setCursor(0, 1);
    display.print("Custom chars!");
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Demo 2: Progress bars
    std::cout << "\nDemo 2: Animated progress bars..." << std::endl;
    for (int level = 0; level <= 5; ++level) {
        auto progress = CharacterFactory::createProgressBar(level);
        display.uploadCustomCharacter(1, progress);
        
        display.clear();
        display.setCursor(0, 0);
        display.print("Progress: ");
        display.print(static_cast<char>(1)); // Progress bar
        display.setCursor(0, 1);
        display.print(std::to_string(level * 20) + "% Complete");
        
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
    
    // Demo 3: Directional arrows
    std::cout << "\nDemo 3: Arrow directions..." << std::endl;
    std::vector<std::string> directions = {"Up", "Right", "Down", "Left"};
    
    for (int dir = 0; dir < 4; ++dir) {
        auto arrow = CharacterFactory::createArrow(dir);
        display.uploadCustomCharacter(2, arrow);
        
        display.clear();
        display.setCursor(0, 0);
        display.print("Direction: ");
        display.print(static_cast<char>(2)); // Arrow
        display.setCursor(0, 1);
        display.print(directions[dir]);
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // Demo 4: Mixed custom characters
    std::cout << "\nDemo 4: Multiple custom characters..." << std::endl;
    
    // Upload different characters to different slots
    display.uploadCustomCharacter(0, CharacterFactory::createHeart());
    display.uploadCustomCharacter(1, CharacterFactory::createNote());
    display.uploadCustomCharacter(2, CharacterFactory::createSmiley());
    display.uploadCustomCharacter(3, CharacterFactory::createProgressBar(4));
    
    display.clear();
    display.setCursor(0, 0);
    display.print(std::string(1, 0) + std::string(1, 1) + " LCD2USB++ " + std::string(1, 2) + std::string(1, 3));
    display.setCursor(0, 1);
    display.print("Amazing chars!");
    
    std::this_thread::sleep_for(std::chrono::seconds(4));
    
    // Demo 5: Create custom pattern
    std::cout << "\nDemo 5: Custom pattern..." << std::endl;
    
    CustomChar custom_pattern({
        "#####",  // Top border
        "#   #",  // Sides
        "# # #",  // Pattern
        "#   #",  // Sides  
        "# # #",  // Pattern
        "#   #",  // Sides
        "#   #",  // Sides
        "#####"   // Bottom border
    });
    
    display.uploadCustomCharacter(4, custom_pattern);
    
    display.clear();
    display.setCursor(0, 0);
    display.print("Custom pattern:");
    display.setCursor(0, 1);
    display.print("Made it! ");
    display.print(static_cast<char>(4));
    
    std::cout << "\nCustom character demo completed!" << std::endl;
    std::cout << "Pattern details:" << std::endl;
    std::cout << custom_pattern.toString() << std::endl;
    std::cout << "Hex bytes: " << custom_pattern.getHexBytes() << std::endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    return 0;
}