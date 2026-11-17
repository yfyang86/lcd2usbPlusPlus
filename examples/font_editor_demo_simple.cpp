/*
 * font_editor_demo_simple.cpp - Simple font editor demonstration
 * Example using the new library structure with correct API
 */

#include "lcd2usb/lcd2usb_lib.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace LCD2USB;
using namespace LCD2USB::FontEdit;

int main() {
    std::cout << "=== LCD2USB++ Font Editor Demo ===" << std::endl;
    
    // Initialize display
    LCDDisplay display;
    
    std::cout << "LCD2USB device initialized!" << std::endl;
    
    // Create FontManager instance
    FontManager font_manager("LCD2USB++ Demo");
    
    // Create some custom characters using the CustomChar API
    auto heart = CharacterFactory::createHeart();
    auto note = CharacterFactory::createNote(); 
    auto spinner = CharacterFactory::createSpinner(0);
    auto match_guy = CharacterFactory::createMatchGuy(0);
    
    // Convert CustomChar to CharacterBitmap for FontManager
    CharacterBitmap heart_bitmap("Heart");
    heart_bitmap.setDescription("Love heart symbol");
    auto heart_array = heart.toBytes();
    heart_bitmap.setFrame(0, heart_array);
    
    CharacterBitmap note_bitmap("MusicNote");
    note_bitmap.setDescription("Musical note symbol");
    auto note_array = note.toBytes();
    note_bitmap.setFrame(0, note_array);
    
    // Add characters to font manager
    font_manager.addCharacter('H', heart_bitmap);
    font_manager.addCharacter('N', note_bitmap);
    
    std::cout << "Added " << font_manager.getCharacterCount() << " characters to font collection" << std::endl;
    
    // Display characters on LCD
    display.clear();
    display.setCursor(0, 0);
    display.print("Font Demo:");
    
    // Upload and display characters
    display.uploadCustomCharacter(0, heart);
    display.uploadCustomCharacter(1, note);
    
    display.setCursor(0, 1);
    display.print(static_cast<char>(0)); // Heart
    display.print(static_cast<char>(1)); // Note
    display.print(" FontMgr OK!");
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Save font to file
    std::string font_filename = "demo_font.lcf";
    if (font_manager.saveToFile(font_filename)) {
        std::cout << "✓ Font saved to " << font_filename << std::endl;
        
        // Try loading it back
        FontManager loaded_font;
        if (loaded_font.loadFromFile(font_filename)) {
            std::cout << "✓ Font loaded successfully!" << std::endl;
            std::cout << "  Loaded " << loaded_font.getCharacterCount() << " characters" << std::endl;
            
            // Display on LCD
            display.clear();
            display.setCursor(0, 0);
            display.print("Loaded font OK!");
            display.setCursor(0, 1);
            display.print(std::to_string(loaded_font.getCharacterCount()) + " characters");
            
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            // Print font information
            loaded_font.printInfo();
        } else {
            std::cout << "✗ Failed to load font file" << std::endl;
        }
    } else {
        std::cout << "✗ Failed to save font file" << std::endl;
    }
    
    // Final summary
    display.clear();
    display.setCursor(0, 0);
    display.print("Font Editor");
    display.setCursor(0, 1);
    display.print("Demo Complete!");
    
    std::cout << "\n=== Demo Complete ===" << std::endl;
    std::cout << "FontManager features demonstrated:" << std::endl;
    std::cout << "✓ Character creation and management" << std::endl;
    std::cout << "✓ Font file save/load (.lcf format)" << std::endl;
    std::cout << "✓ Display integration with LCD" << std::endl;
    std::cout << "✓ Metadata handling (author, creation time)" << std::endl;
    
    return 0;
}