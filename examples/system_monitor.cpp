/*
 * system_monitor.cpp - System monitoring example using LCD2USB++ library
 * Refactored from lcd2usb++.cpp to use the new library structure
 */

#include "lcd2usb/lcd2usb_lib.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>
#include <sstream>
#include <iomanip>

// macOS system headers
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>
#include <mach/kern_return.h>

using namespace LCD2USB;

volatile bool running = true;

void signalHandler(int signal) {
    std::cout << "\nShutting down..." << std::endl;
    running = false;
}

// System information gathering functions
std::string getSystemInfo() {
    // Get basic system info
    return "macOS System | LCD2USB++ Active";
}

std::string getMemoryInfo() {
    vm_size_t page_size;
    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t host_size = sizeof(vm_statistics64_data_t) / sizeof(natural_t);
    
    host_page_size(mach_host_self(), &page_size);
    host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vm_stat, &host_size);
    
    // Calculate memory usage percentage (simplified)
    uint64_t total_mem = (vm_stat.free_count + vm_stat.active_count + 
                         vm_stat.inactive_count + vm_stat.wire_count) * page_size;
    uint64_t used_mem = (vm_stat.active_count + vm_stat.wire_count) * page_size;
    
    double mem_percentage = (double)used_mem / total_mem * 100.0;
    
    std::stringstream ss;
    ss << "Mem: " << std::fixed << std::setprecision(1) << mem_percentage << "% | Cores: Available";
    return ss.str();
}

int main() {
    std::cout << "=== LCD2USB++ System Monitor ===" << std::endl;
    std::cout << "Enhanced C++17 System Monitoring" << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;
    
    // Set up signal handler for clean exit
    signal(SIGINT, signalHandler);
    
    // Initialize display
    LCDDisplay display;
    if (!display.initialize()) {
        std::cerr << "Failed to initialize LCD display" << std::endl;
        return -1;
    }
    
    std::cout << "LCD2USB device initialized successfully!" << std::endl;
    
    // Run communication tests
    display.performEchoTest();
    display.displayVersion();
    
    // Configure display
    display.setContrast(200);
    display.setBrightness(255);
    display.clear();
    
    // Create scrolling text objects
    ScrollingText system_scroll(display);
    ScrollingText memory_scroll(display);
    
    system_scroll.setPosition(0, 0);
    system_scroll.setViewport(16, 1);
    system_scroll.setDirection(ScrollDirection::LEFT_TO_RIGHT);
    system_scroll.setScrollDelay(200);
    
    memory_scroll.setPosition(0, 1);
    memory_scroll.setViewport(16, 1);
    memory_scroll.setDirection(ScrollDirection::LEFT_TO_RIGHT);
    memory_scroll.setScrollDelay(250);
    
    // Setup animated characters
    int match_frame = 0;
    int spinner_frame = 0;
    int update_counter = 0;
    
    std::cout << "Starting system monitoring with animated characters..." << std::endl;
    
    while (running) {
        // Update system information periodically
        if (update_counter % 20 == 0) {
            std::string sys_info = getSystemInfo();
            std::string mem_info = getMemoryInfo();
            
            // Add animated separators
            sys_info += "   " + std::string(1, char(0)) + "   ";
            mem_info += "   " + std::string(1, char(1)) + "   ";
            
            system_scroll.setText(sys_info);
            memory_scroll.setText(mem_info);
            
            std::cout << "Updated: " << sys_info.substr(0, 30) << "..." << std::endl;
        }
        
        // Update animations
        if (update_counter % 4 == 0) {
            // Update match guy animation (slower)
            auto match_guy = CharacterFactory::createMatchGuy(match_frame);
            display.uploadCustomCharacter(0, match_guy);
            match_frame = (match_frame + 1) % 4;
        }
        
        if (update_counter % 2 == 0) {
            // Update spinner animation (faster)
            auto spinner = CharacterFactory::createSpinner(spinner_frame);
            display.uploadCustomCharacter(1, spinner);
            spinner_frame = (spinner_frame + 1) % 4;
        }
        
        // Display current frames
        system_scroll.displayCurrentFrame();
        memory_scroll.displayCurrentFrame();
        
        // Advance scrolling
        system_scroll.scrollStep();
        memory_scroll.scrollStep();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        update_counter++;
        
        // Demo mode - run for limited time
        if (update_counter > 300) { // About 1 minute demo
            std::cout << "\nSystem monitoring demo completed!" << std::endl;
            break;
        }
    }
    
    // Final display
    display.clear();
    display.setCursor(0, 0);
    display.print("System Monitor");
    display.setCursor(0, 1);
    display.print("Demo Complete!");
    
    std::cout << "System monitoring finished!" << std::endl;
    
    return 0;
}