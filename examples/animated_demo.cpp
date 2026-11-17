/*
 * animated_demo.cpp - Animated character demonstration
 * Example using the new library structure
 */

#include "lcd2usb/lcd2usb_lib.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>

using namespace LCD2USB;

volatile bool running = true;

void signalHandler(int signal) {
    std::cout << "\nShutting down..." << std::endl;
    running = false;
}

int main() {
    std::cout << "=== LCD2USB++ Animated Character Demo ===" << std::endl;
    
    // Set up signal handler for clean exit
    signal(SIGINT, signalHandler);
    
    // Initialize display
    LCDDisplay display;
    if (!display.initialize()) {
        std::cerr << "Failed to initialize LCD display" << std::endl;
        return -1;
    }
    
    std::cout << "LCD2USB device initialized!" << std::endl;
    std::cout << "Press Ctrl+C to stop the animation..." << std::endl;
    
    // Configure display
    display.setContrast(200);
    display.setBrightness(255);
    display.clear();
    
    // Animation counters
    int match_frame = 0;
    int spinner_frame = 0;
    int dancer_frame = 0;
    int update_counter = 0;
    
    while (running) {
        // Update animations at different speeds
        if (update_counter % 4 == 0) {
            // Update match guy (slower)
            auto match_guy = CharacterFactory::createMatchGuy(match_frame);
            display.uploadCustomCharacter(0, match_guy);
            match_frame = (match_frame + 1) % 4;
        }
        
        if (update_counter % 2 == 0) {
            // Update spinner (medium speed)
            auto spinner = CharacterFactory::createSpinner(spinner_frame);
            display.uploadCustomCharacter(1, spinner);
            spinner_frame = (spinner_frame + 1) % 4;
        }
        
        // Update dancer (fast)
        auto dancer = CharacterFactory::createDancer(dancer_frame);
        display.uploadCustomCharacter(2, dancer);
        dancer_frame = (dancer_frame + 1) % 2;
        
        // Display animated characters
        display.setCursor(0, 0);
        display.print("Anim-chars: ");
        display.print(static_cast<char>(0)); // Match guy
        display.print(static_cast<char>(1)); // Spinner
        display.print(static_cast<char>(2)); // Dancer
        
        display.setCursor(0, 1);
        display.print("Frame: " + std::to_string(update_counter));
        
        // Sleep and update counter
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        update_counter++;
        
        // Demo mode - run for limited time if no Ctrl+C
        if (update_counter > 200) {
            std::cout << "\nDemo completed! (200 frames)" << std::endl;
            break;
        }
    }
    
    // Final display
    display.clear();
    display.setCursor(0, 0);
    display.print("Animation Demo");
    display.setCursor(0, 1);
    display.print("Complete!");
    
    std::cout << "Animated character demo finished!" << std::endl;
    
    return 0;
}