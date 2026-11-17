/*
 * test_4rows.cpp - Test 4-row LCD display support
 */

#include "lcd2usb/lcd2usb_lib.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace LCD2USB;

int main() {
    std::cout << "=== Testing 4-Row LCD Support ===" << std::endl;
    
    LCDDisplay display;
    if (!display.initialize()) {
        std::cerr << "Failed to initialize LCD display" << std::endl;
        return -1;
    }
    
    std::cout << "✓ LCD initialized" << std::endl;
    
    display.setContrast(200);
    display.setBrightness(255);
    display.clear();
    
    // Test writing to all 4 rows
    std::cout << "Testing all 4 rows..." << std::endl;
    
    display.setCursor(0, 0);
    display.print("Row 0: ABCDEFGHIJ");
    
    display.setCursor(0, 1);
    display.print("Row 1: 1234567890");
    
    display.setCursor(0, 2);
    display.print("Row 2: !@#$%^&*()");
    
    display.setCursor(0, 3);
    display.print("Row 3: <=>?[]{}+-");
    
    std::cout << "✓ All 4 rows written" << std::endl;
    std::cout << "Check your LCD display - you should see text on all 4 rows" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Test writeToLine for all rows
    std::cout << "\nTesting writeToLine function..." << std::endl;
    display.clear();
    
    display.writeToLine(0, "Line 0 Test");
    display.writeToLine(1, "Line 1 Test");
    display.writeToLine(2, "Line 2 Test");
    display.writeToLine(3, "Line 3 Test");
    
    std::cout << "✓ writeToLine tested on all 4 rows" << std::endl;
    std::cout << "Test complete!" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    display.clear();
    return 0;
}
