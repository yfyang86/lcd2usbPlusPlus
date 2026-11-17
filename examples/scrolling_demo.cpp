/*
 * scrolling_demo.cpp - Scrolling text demonstration
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
    std::cout << "=== LCD2USB++ Scrolling Text Demo ===" << std::endl;
    
    // Set up signal handler
    signal(SIGINT, signalHandler);
    
    // Initialize display
    LCDDisplay display;
    if (!display.initialize()) {
        std::cerr << "Failed to initialize LCD display" << std::endl;
        return -1;
    }
    
    std::cout << "LCD2USB device initialized!" << std::endl;
    std::cout << "Press Ctrl+C to stop scrolling..." << std::endl;
    
    // Configure display
    display.setContrast(200);
    display.setBrightness(255);
    display.clear();
    
    // Demo 1: Basic scrolling text
    std::cout << "\nDemo 1: Basic horizontal scrolling..." << std::endl;
    
    ScrollingText scroller1(display);
    scroller1.setText("Welcome to LCD2USB++ library with scrolling text support!");
    scroller1.setPosition(0, 0);
    scroller1.setViewport(16, 1);
    scroller1.setScrollDelay(300);
    scroller1.setDirection(ScrollDirection::LEFT_TO_RIGHT);
    
    // Manual scrolling for demo
    for (int i = 0; i < 50 && running; ++i) {
        scroller1.displayCurrentFrame();
        scroller1.scrollStep();
        
        display.setCursor(0, 1);
        display.print("Manual scroll " + std::to_string(i + 1));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    
    if (!running) return 0;
    
    // Demo 2: Marquee text
    std::cout << "\nDemo 2: Marquee text with different speeds..." << std::endl;
    
    MarqueeText marquee1(display, "LCD2USB++ is amazing for text displays!", 0);
    MarqueeText marquee2(display, "Multiple scrolling lines work perfectly!", 1);
    
    marquee1.setSpeed(2); // Medium speed
    marquee2.setSpeed(4); // Fast speed
    
    marquee1.start();
    marquee2.start();
    
    // Let them scroll for a while
    for (int i = 0; i < 100 && running; ++i) {
        marquee1.update();
        marquee2.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    marquee1.stop();
    marquee2.stop();
    
    if (!running) return 0;
    
    // Demo 3: Text banner with timed messages
    std::cout << "\nDemo 3: Text banner with timed messages..." << std::endl;
    
    TextBanner banner(display);
    banner.addMessage("Message 1: Hello!", 2000);
    banner.addMessage("Message 2: World!", 1500);
    
    banner.start(); // This will display messages sequentially
    
    if (!running) return 0;
    
    // Demo 4: Scrolling with animated characters
    std::cout << "\nDemo 4: Scrolling with animated characters..." << std::endl;
    
    ScrollingText scroller2(display);
    scroller2.setText("Animated scrolling with custom characters!");
    scroller2.setPosition(0, 0);
    scroller2.setDirection(ScrollDirection::RIGHT_TO_LEFT);
    scroller2.setScrollDelay(250);
    
    int animation_frame = 0;
    
    for (int i = 0; i < 60 && running; ++i) {
        // Update animation
        if (i % 3 == 0) {
            auto spinner = CharacterFactory::createSpinner(animation_frame);
            display.uploadCustomCharacter(0, spinner);
            animation_frame = (animation_frame + 1) % 4;
        }
        
        // Display scrolling text
        scroller2.displayCurrentFrame();
        scroller2.scrollStep();
        
        // Add animated character on second line
        display.setCursor(0, 1);
        display.print("Loading... ");
        display.print(static_cast<char>(0)); // Animated spinner
        display.print(" Please wait");
        
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    
    if (!running) return 0;
    
    // Demo 5: Multi-direction scrolling showcase
    std::cout << "\nDemo 5: Multi-direction scrolling showcase..." << std::endl;
    
    ScrollingText scroller3(display);
    scroller3.setText("Direction test: Left to Right scrolling");
    scroller3.setPosition(0, 0);
    scroller3.setDirection(ScrollDirection::LEFT_TO_RIGHT);
    
    ScrollingText scroller4(display);
    scroller4.setText("Direction test: Right to Left scrolling");
    scroller4.setPosition(0, 1);
    scroller4.setDirection(ScrollDirection::RIGHT_TO_LEFT);
    
    // Scroll in opposite directions
    for (int i = 0; i < 40 && running; ++i) {
        scroller3.displayCurrentFrame();
        scroller3.scrollStep();
        
        scroller4.displayCurrentFrame();
        scroller4.scrollStep();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }
    
    // Final display
    display.clear();
    display.setCursor(0, 0);
    display.print("Scrolling Demo");
    display.setCursor(0, 1);
    display.print("Complete!");
    
    std::cout << "\n=== Scrolling Demo Complete ===" << std::endl;
    std::cout << "Demonstrated:" << std::endl;
    std::cout << "  • Basic scrolling text" << std::endl;
    std::cout << "  • Marquee text at different speeds" << std::endl;
    std::cout << "  • Timed text banners" << std::endl;
    std::cout << "  • Animated characters with scrolling" << std::endl;
    std::cout << "  • Multi-direction scrolling" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    return 0;
}