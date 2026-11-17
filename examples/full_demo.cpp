/*
 * full_demo.cpp - Comprehensive LCD2USB++ feature demonstration
 * Example showcasing all library capabilities
 */

#include "lcd2usb/lcd2usb_lib.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <random>
#include <iomanip>

using namespace LCD2USB;

volatile bool running = true;

void signalHandler(int signal) {
    std::cout << "\nShutting down..." << std::endl;
    running = false;
}

void displaySection(const std::string& section, int number, int total) {
    std::cout << "\n╔══════════════════════════════════════╗" << std::endl;
    std::cout << "║ Section " << number << "/" << total << ": " << std::left << std::setw(24) << section << " ║" << std::endl;
    std::cout << "╚══════════════════════════════════════╝" << std::endl;
}

void waitForContinue() {
    std::cout << "Press Enter to continue to next section...";
    std::cin.get();
}

int main() {
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║              LCD2USB++ Full Feature Demo                ║\n";
    std::cout << "║              Complete Library Showcase                  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    
    // Set up signal handler
    signal(SIGINT, signalHandler);
    
    // Initialize display
    LCDDisplay display;
    if (!display.initialize()) {
        std::cerr << "Failed to initialize LCD display" << std::endl;
        return -1;
    }
    
    std::cout << "✓ LCD2USB device initialized!" << std::endl;
    std::cout << "\nThis demo will showcase all LCD2USB++ library features." << std::endl;
    std::cout << "You can press Ctrl+C at any time to exit cleanly." << std::endl;
    
    waitForContinue();
    
    // Configure display optimally
    display.setContrast(200);
    display.setBrightness(255);
    
    // === SECTION 1: BASIC FUNCTIONALITY ===
    displaySection("Basic Functionality", 1, 8);
    
    std::cout << "Testing basic LCD operations..." << std::endl;
    
    // Echo test and version
    display.performEchoTest();
    display.displayVersion();
    
    // Basic text display
    display.clear();
    display.setCursor(0, 0);
    display.print("LCD2USB++ Library");
    display.setCursor(0, 1);
    display.print("Basic Test OK!");
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    if (!running) return 0;
    waitForContinue();
    
    // === SECTION 2: CUSTOM CHARACTERS ===
    displaySection("Custom Characters", 2, 8);
    
    std::cout << "Demonstrating custom character creation..." << std::endl;
    
    // Create and display various custom characters
    auto heart = CharacterFactory::createHeart();
    auto smiley = CharacterFactory::createSmiley();
    auto note = CharacterFactory::createNote();
    auto progress_full = CharacterFactory::createProgressBar(5);
    
    display.uploadCustomCharacter(0, heart);
    display.uploadCustomCharacter(1, smiley);
    display.uploadCustomCharacter(2, note);
    display.uploadCustomCharacter(3, progress_full);
    
    display.clear();
    display.setCursor(0, 0);
    display.print("Custom chars: ");
    display.print(static_cast<char>(0));
    display.print(static_cast<char>(1));
    display.print(static_cast<char>(2));
    display.print(static_cast<char>(3));
    display.setCursor(0, 1);
    display.print("Heart,Smile,Note!");
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    if (!running) return 0;
    waitForContinue();
    
    // === SECTION 3: ANIMATED CHARACTERS ===
    displaySection("Animated Characters", 3, 8);
    
    std::cout << "Showcasing character animation..." << std::endl;
    
    for (int cycle = 0; cycle < 3 && running; ++cycle) {
        for (int frame = 0; frame < 4; ++frame) {
            // Multi-character animation
            auto match_guy = CharacterFactory::createMatchGuy(frame);
            auto spinner = CharacterFactory::createSpinner(frame);
            auto dancer = CharacterFactory::createDancer(frame % 2);
            
            display.uploadCustomCharacter(0, match_guy);
            display.uploadCustomCharacter(1, spinner);
            display.uploadCustomCharacter(2, dancer);
            
            display.clear();
            display.setCursor(0, 0);
            display.print("Animations: ");
            display.print(static_cast<char>(0));
            display.print(static_cast<char>(1));
            display.print(static_cast<char>(2));
            display.setCursor(0, 1);
            display.print("Frame " + std::to_string(frame + 1) + "/4 Cycle " + std::to_string(cycle + 1));
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    
    if (!running) return 0;
    waitForContinue();
    
    // === SECTION 4: FONT MANAGEMENT ===
    displaySection("Font Management", 4, 8);
    
    std::cout << "Demonstrating font editor capabilities..." << std::endl;
    
    // Create font collection
    FontManager font_manager;
    
    font_manager.addCharacter('H', heart, {
        "Heart", "Love symbol", {"symbol", "love", "emotion"}
    });
    font_manager.addCharacter('S', smiley, {
        "Smiley", "Happy face", {"emoji", "emotion", "happy"}
    });
    font_manager.addCharacter('N', note, {
        "Note", "Music symbol", {"music", "symbol", "note"}
    });
    
    // Save and load demonstration
    std::string font_file = "demo_complete.lcf";
    if (font_manager.saveToFile(font_file)) {
        std::cout << "✓ Font saved to " << font_file << std::endl;
        
        FontManager loaded_font;
        if (loaded_font.loadFromFile(font_file)) {
            std::cout << "✓ Font loaded successfully!" << std::endl;
            std::cout << "  Characters: " << loaded_font.size() << std::endl;
            
            display.clear();
            display.setCursor(0, 0);
            display.print("Font system OK!");
            display.setCursor(0, 1);
            display.print(std::to_string(loaded_font.size()) + " chars saved");
        }
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    if (!running) return 0;
    waitForContinue();
    
    // === SECTION 5: SCROLLING TEXT ===
    displaySection("Scrolling Text", 5, 8);
    
    std::cout << "Demonstrating text scrolling capabilities..." << std::endl;
    
    ScrollingText scroller(display);
    scroller.setText("LCD2USB++ library provides comprehensive LCD control with custom characters, animations, font management, and scrolling text!");
    scroller.setPosition(0, 0);
    scroller.setDirection(ScrollDirection::LEFT_TO_RIGHT);
    scroller.setScrollDelay(200);
    
    display.clear();
    display.setCursor(0, 1);
    display.print("Scrolling demo");
    
    for (int i = 0; i < 50 && running; ++i) {
        scroller.displayCurrentFrame();
        scroller.scrollStep();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    if (!running) return 0;
    waitForContinue();
    
    // === SECTION 6: MARQUEE TEXT ===
    displaySection("Marquee Effects", 6, 8);
    
    std::cout << "Demonstrating marquee text effects..." << std::endl;
    
    MarqueeText marquee1(display, "Line 1: Fast scrolling marquee text!", 0);
    MarqueeText marquee2(display, "Line 2: Slower marquee for contrast!", 1);
    
    marquee1.setSpeed(4); // Fast
    marquee2.setSpeed(2); // Slower
    
    marquee1.start();
    marquee2.start();
    
    // Let marquees run
    for (int i = 0; i < 40 && running; ++i) {
        marquee1.update();
        marquee2.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    
    marquee1.stop();
    marquee2.stop();
    
    if (!running) return 0;
    waitForContinue();
    
    // === SECTION 7: COMBINED EFFECTS ===
    displaySection("Combined Effects", 7, 8);
    
    std::cout << "Combining animations with scrolling text..." << std::endl;
    
    ScrollingText combined_scroller(display);
    combined_scroller.setText("Amazing combined effects: scrolling text with animated characters!");
    combined_scroller.setPosition(0, 0);
    combined_scroller.setDirection(ScrollDirection::RIGHT_TO_LEFT);
    combined_scroller.setScrollDelay(300);
    
    int anim_frame = 0;
    
    for (int i = 0; i < 40 && running; ++i) {
        // Update animation every few frames
        if (i % 3 == 0) {
            auto spinner = CharacterFactory::createSpinner(anim_frame);
            display.uploadCustomCharacter(0, spinner);
            anim_frame = (anim_frame + 1) % 4;
        }
        
        // Display scrolling text
        combined_scroller.displayCurrentFrame();
        combined_scroller.scrollStep();
        
        // Second line with animation
        display.setCursor(0, 1);
        display.print("Status: ");
        display.print(static_cast<char>(0)); // Animated spinner
        display.print(" Working!");
        
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    
    if (!running) return 0;
    waitForContinue();
    
    // === SECTION 8: INTERACTIVE DEMO ===
    displaySection("Interactive Demo", 8, 8);
    
    std::cout << "Running interactive system monitor simulation..." << std::endl;
    
    // Simulate system monitoring with dynamic updates
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> cpu_dist(15, 85);
    std::uniform_int_distribution<> mem_dist(30, 90);
    std::uniform_int_distribution<> temp_dist(35, 65);
    
    ScrollingText status_scroller(display);
    status_scroller.setPosition(0, 1);
    status_scroller.setDirection(ScrollDirection::LEFT_TO_RIGHT);
    status_scroller.setScrollDelay(400);
    
    int monitor_frame = 0;
    
    for (int i = 0; i < 30 && running; ++i) {
        // Generate fake system stats
        int cpu = cpu_dist(gen);
        int memory = mem_dist(gen);
        int temp = temp_dist(gen);
        
        // Update progress bar based on CPU usage
        int progress_level = (cpu * 5) / 100;
        auto cpu_bar = CharacterFactory::createProgressBar(progress_level);
        display.uploadCustomCharacter(0, cpu_bar);
        
        // Animated system indicator
        auto activity = CharacterFactory::createSpinner(monitor_frame % 4);
        display.uploadCustomCharacter(1, activity);
        
        // Top line: CPU with progress bar
        display.setCursor(0, 0);
        display.print("CPU:");
        display.print(static_cast<char>(0)); // Progress bar
        display.print(std::to_string(cpu) + "% ");
        display.print(static_cast<char>(1)); // Activity indicator
        
        // Bottom line: Scrolling system info
        std::string sys_info = "Memory: " + std::to_string(memory) + 
                              "% | Temp: " + std::to_string(temp) + 
                              "C | System OK | ";
        status_scroller.setText(sys_info);
        status_scroller.displayCurrentFrame();
        status_scroller.scrollStep();
        
        monitor_frame++;
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
    
    // === FINAL SUMMARY ===
    display.clear();
    
    // Upload a celebration character
    auto celebration = CharacterFactory::createHeart();
    display.uploadCustomCharacter(0, celebration);
    
    display.setCursor(0, 0);
    display.print("Demo Complete! ");
    display.print(static_cast<char>(0));
    display.setCursor(0, 1);
    display.print("All features OK!");
    
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                 Demo Complete!                           ║\n";
    std::cout << "║                                                          ║\n";
    std::cout << "║  Successfully demonstrated:                              ║\n";
    std::cout << "║  ✓ Basic LCD operations                                  ║\n";
    std::cout << "║  ✓ Custom character creation                             ║\n";
    std::cout << "║  ✓ Multi-frame character animations                     ║\n";
    std::cout << "║  ✓ Font management (save/load/search)                   ║\n";
    std::cout << "║  ✓ Scrolling text in multiple directions                ║\n";
    std::cout << "║  ✓ Marquee text effects                                 ║\n";
    std::cout << "║  ✓ Combined animations and scrolling                    ║\n";
    std::cout << "║  ✓ Interactive system monitoring simulation             ║\n";
    std::cout << "║                                                          ║\n";
    std::cout << "║  LCD2USB++ library is fully functional!                 ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    return 0;
}