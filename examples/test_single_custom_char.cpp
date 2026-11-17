/**
 * Test program to display a single custom character
 */

#include <iostream>
#include <thread>
#include <chrono>
#include "lcd2usb/lcd_display.hpp"
#include "lcd2usb/custom_char.hpp"

using namespace LCD2USB;

int main() {
    std::cout << "=== Testing Single Custom Character ===" << std::endl;
    
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
        
        // Create a simple smiley face pattern using CustomChar
        CustomChar smiley({
            "     ",
            " # # ",
            " # # ",
            "     ",
            "#   #",
            " ### ",
            "     ",
            "     "
        });
        
        // Clear display
        display.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Show startup screen
        display.setCursor(0, 0);
        display.print("Custom Char Test");
        display.setCursor(0, 1);
        display.print("Starting...");
        display.setCursor(0, 2);
        display.print("================");
        display.setCursor(0, 3);
        display.print("Please wait...");
        std::cout << "✓ Startup screen displayed" << std::endl;
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // Clear and display regular text to confirm LCD is working
        display.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        display.setCursor(0, 0);
        display.print("Regular text OK");
        display.setCursor(0, 1);
        display.print("All 4 rows work");
        display.setCursor(0, 2);
        display.print("Testing custom");
        display.setCursor(0, 3);
        display.print("chars next...");
        std::cout << "✓ Regular text displayed on all 4 rows" << std::endl;
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // Now upload and display custom character
        std::cout << "Uploading smiley to slot 0..." << std::endl;
        display.uploadCustomCharacter(0, smiley);
        std::cout << "✓ Custom character uploaded" << std::endl;
        
        // Need to wait and set cursor after CGRAM upload
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Clear and display mixed custom character and text
        display.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Row 0: Mix custom char with text
        display.setCursor(0, 0);
        display.print("0 ");
        display.print(static_cast<char>(0));  // Custom character
        display.print(" ABCD");
        
        // Row 1: Mix custom char with text
        display.setCursor(0, 1);
        display.print("1 ");
        display.print(static_cast<char>(0));  // Custom character
        display.print(" EFGH");
        
        // Row 2: Mix custom char with text
        display.setCursor(0, 2);
        display.print("2 ");
        display.print(static_cast<char>(0));  // Custom character
        display.print(" IJKL");
        
        // Row 3: Mix custom char with text
        display.setCursor(0, 3);
        display.print("3 ");
        display.print(static_cast<char>(0));  // Custom character
        display.print(" MNOP");
        
        std::cout << "Displaying custom character 0 mixed with text on all rows..." << std::endl;
        std::cout << "✓ Custom character sent to LCD" << std::endl;
        
        std::cout << "\nCheck your LCD - you should see:" << std::endl;
        std::cout << "Row 0: '0 [smiley] ABCD'" << std::endl;
        std::cout << "Row 1: '1 [smiley] EFGH'" << std::endl;
        std::cout << "Row 2: '2 [smiley] IJKL'" << std::endl;
        std::cout << "Row 3: '3 [smiley] MNOP'" << std::endl;
        std::cout << "If you see rows but NO smiley, custom char upload failed" << std::endl;
        std::cout << "If you see NOTHING, there's a communication issue" << std::endl;
        std::cout << "\nWaiting 5 seconds..." << std::endl;
        
        // Wait 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // Show completion screen
        std::cout << "\nDisplaying COMPLETE screen..." << std::endl;
        display.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        display.setCursor(0, 0);
        display.print("   TEST");
        display.setCursor(0, 1);
        display.print("  COMPLETE");
        display.setCursor(0, 2);
        display.print("=============");
        display.setCursor(0, 3);
        display.print(" Exiting now...");
        
        std::cout << "✓ Test complete! LCD should show 'TEST COMPLETE'" << std::endl;
        std::cout << "Exiting in 3 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        display.clear();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
