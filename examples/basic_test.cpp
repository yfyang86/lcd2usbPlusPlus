/*
 * basic_test.cpp - Basic LCD2USB++ functionality test
 * Example using the new library structure
 */

#include "lcd2usb/lcd2usb_lib.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace LCD2USB;

int main() {
    std::cout << "=== LCD2USB++ Basic Test ===" << std::endl;
    
    // Initialize display
    LCDDisplay display;
    if (!display.initialize()) {
        std::cerr << "Failed to initialize LCD display" << std::endl;
        return -1;
    }
    
    std::cout << "LCD2USB device found and initialized!" << std::endl;
    
    // Test communication
    display.performEchoTest();
    display.displayVersion();
    
    // Configure display
    display.setContrast(200);
    display.setBrightness(255);
    display.clear();
    
    // Basic text display
    display.setCursor(0, 0);
    display.print("Hello LCD2USB++!");
    display.setCursor(0, 1);
    display.print("Library works!");
    
    std::cout << "Basic text displayed. Waiting 3 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Test cursor positioning
    display.clear();
    for (int i = 0; i < 16; ++i) {
        display.setCursor(i, 0);
        display.print("*");
        display.setCursor(15 - i, 1);
        display.print("*");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    // Final message
    display.clear();
    display.setCursor(0, 0);
    display.print("Test Complete!");
    display.setCursor(0, 1);
    display.print("LCD2USB++ Ready");
    
    std::cout << "Basic test completed successfully!" << std::endl;
    
    return 0;
}