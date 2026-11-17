/*
 * font_editor_demo.cpp - Font editor system demonstration
 * Example using the new library structure
 */

#include "lcd2usb/lcd2usb_lib.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>

using namespace LCD2USB;

int main() {
    std::cout << "=== LCD2USB++ Font Editor Demo ===" << std::endl;
    
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
    
    // Create font manager
    FontManager font_manager;
    
    // Demo 1: Add custom characters to font
    std::cout << "\nDemo 1: Creating font collection..." << std::endl;
    
    // Add jumping match guy
    auto match_guy = CharacterFactory::createMatchGuy(0);
    font_manager.addCharacter('J', match_guy, {
        "JumpingGuy", "Jumping stick figure", {"animation", "character", "fun"}
    });
    
    // Add heart
    auto heart = CharacterFactory::createHeart();
    font_manager.addCharacter('H', heart, {
        "Heart", "Love heart symbol", {"symbol", "love", "emotion"}
    });
    
    // Add note
    auto note = CharacterFactory::createNote();
    font_manager.addCharacter('N', note, {
        "MusicNote", "Musical note symbol", {"music", "symbol", "note"}
    });
    
    // Add spinner
    auto spinner = CharacterFactory::createSpinner(0);
    font_manager.addCharacter('S', spinner, {
        "Spinner", "Loading spinner", {"animation", "loading", "wait"}
    });
    
    std::cout << "Added " << font_manager.size() << " characters to font collection" << std::endl;
    
    // Display characters on LCD
    display.uploadCustomCharacter(0, match_guy);
    display.uploadCustomCharacter(1, heart);
    display.uploadCustomCharacter(2, note);
    display.uploadCustomCharacter(3, spinner);
    
    display.setCursor(0, 0);
    display.print("Font Demo: ");
    display.print(static_cast<char>(0));
    display.print(static_cast<char>(1));
    display.print(static_cast<char>(2));
    display.print(static_cast<char>(3));
    display.setCursor(0, 1);
    display.print("4 characters!");
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Demo 2: Save font to file
    std::cout << "\nDemo 2: Saving font to file..." << std::endl;
    
    std::string font_filename = "demo_font.lcf";
    if (font_manager.saveToFile(font_filename)) {
        std::cout << "✓ Font saved to: " << font_filename << std::endl;
        
        // Verify file exists
        std::ifstream test_file(font_filename);
        if (test_file.good()) {
            test_file.close();
            std::cout << "✓ Font file verified" << std::endl;
        }
        
        display.clear();
        display.setCursor(0, 0);
        display.print("Font saved!");
        display.setCursor(0, 1);
        display.print(font_filename);
        
    } else {
        std::cout << "✗ Failed to save font" << std::endl;
        
        display.clear();
        display.setCursor(0, 0);
        display.print("Save failed!");
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Demo 3: Load font from file
    std::cout << "\nDemo 3: Loading font from file..." << std::endl;
    
    FontManager loaded_font;
    if (loaded_font.loadFromFile(font_filename)) {
        std::cout << "✓ Font loaded successfully!" << std::endl;
        std::cout << "  Loaded " << loaded_font.size() << " characters" << std::endl;
        
        // List loaded characters
        auto chars = loaded_font.getAvailableCharacters();
        std::cout << "  Characters: ";
        for (char c : chars) {
            std::cout << "'" << c << "' ";
        }
        std::cout << std::endl;
        
        display.clear();
        display.setCursor(0, 0);
        display.print("Font loaded!");
        display.setCursor(0, 1);
        display.print(std::to_string(loaded_font.size()) + " characters");
        
    } else {
        std::cout << "✗ Failed to load font" << std::endl;
        
        display.clear();
        display.setCursor(0, 0);
        display.print("Load failed!");
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Demo 4: Search by tags
    std::cout << "\nDemo 4: Searching characters by tags..." << std::endl;
    
    auto animation_chars = font_manager.searchByTag("animation");
    std::cout << "Characters tagged 'animation': ";
    for (char c : animation_chars) {
        std::cout << "'" << c << "' ";
    }
    std::cout << "(" << animation_chars.size() << " found)" << std::endl;
    
    auto symbol_chars = font_manager.searchByTag("symbol");
    std::cout << "Characters tagged 'symbol': ";
    for (char c : symbol_chars) {
        std::cout << "'" << c << "' ";
    }
    std::cout << "(" << symbol_chars.size() << " found)" << std::endl;
    
    display.clear();
    display.setCursor(0, 0);
    display.print("Tag search done");
    display.setCursor(0, 1);
    display.print("Check console!");
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Demo 5: Export as C++ code
    std::cout << "\nDemo 5: Exporting font as C++ code..." << std::endl;
    
    std::string code_output = font_manager.exportAsCode();
    std::string code_filename = "font_export.cpp";
    
    std::ofstream code_file(code_filename);
    if (code_file.is_open()) {
        code_file << code_output;
        code_file.close();
        
        std::cout << "✓ Font exported to: " << code_filename << std::endl;
        std::cout << "  " << code_output.size() << " bytes written" << std::endl;
        
        display.clear();
        display.setCursor(0, 0);
        display.print("Code exported!");
        display.setCursor(0, 1);
        display.print(code_filename);
        
    } else {
        std::cout << "✗ Failed to export code" << std::endl;
        
        display.clear();
        display.setCursor(0, 0);
        display.print("Export failed!");
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Final summary
    display.clear();
    display.setCursor(0, 0);
    display.print("Font Demo Done!");
    display.setCursor(0, 1);
    display.print("Check files!");
    
    std::cout << "\n=== Font Editor Demo Complete ===" << std::endl;
    std::cout << "Files created:" << std::endl;
    std::cout << "  • " << font_filename << " - Binary font file" << std::endl;
    std::cout << "  • " << code_filename << " - C++ code export" << std::endl;
    
    return 0;
}