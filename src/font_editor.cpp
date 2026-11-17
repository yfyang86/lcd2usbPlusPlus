/*
 * font_editor.cpp - Font editor implementation
 * Part of LCD2USB++ library
 */

#include "lcd2usb/font_editor.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <cstring>

namespace LCD2USB {
namespace FontEdit {

// CharacterBitmap implementation
CharacterBitmap::CharacterBitmap(const std::string& name) 
    : name_(name), description_("") {
}

void CharacterBitmap::setFrame(size_t frameIndex, const std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT>& data) {
    if (frameIndex >= frames_.size()) {
        frames_.resize(frameIndex + 1);
    }
    frames_[frameIndex] = data;
}

std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT> CharacterBitmap::getFrame(size_t frameIndex) const {
    if (frameIndex < frames_.size()) {
        return frames_[frameIndex];
    }
    return {}; // Return empty frame if out of bounds
}

void CharacterBitmap::printCharacter(size_t frameIndex) const {
    if (frameIndex >= frames_.size()) {
        std::cout << "Frame " << frameIndex << " not available\n";
        return;
    }
    
    std::cout << "Character: " << name_ << " (Frame " << frameIndex << ")\n";
    if (!description_.empty()) {
        std::cout << "Description: " << description_ << "\n";
    }
    
    const auto& frame = frames_[frameIndex];
    
    std::cout << "┌──────────┐\n";
    for (int y = 0; y < LCD::CUSTOM_CHAR_HEIGHT; ++y) {
        std::cout << "│";
        for (int x = 0; x < LCD::CUSTOM_CHAR_WIDTH; ++x) {
            bool pixel = (frame[y] & (1 << (LCD::CUSTOM_CHAR_WIDTH - 1 - x))) != 0;
            std::cout << (pixel ? "██" : "  ");
        }
        std::cout << "│\n";
    }
    std::cout << "└──────────┘\n";
    
    // Show hex bytes
    std::cout << "Hex: ";
    for (int i = 0; i < LCD::CUSTOM_CHAR_HEIGHT; ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << "0x" << std::hex << std::setfill('0') << std::setw(2) 
                  << static_cast<int>(frame[i]);
    }
    std::cout << std::dec << "\n\n";
}

void CharacterBitmap::printAllFrames() const {
    std::cout << "=== " << name_ << " ===\n";
    if (!description_.empty()) {
        std::cout << "Description: " << description_ << "\n";
    }
    std::cout << "Frames: " << frames_.size() << "\n\n";
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        printCharacter(i);
    }
}

// FontManager implementation
FontManager::FontManager(const std::string& author) : author_(author) {
    creation_time_ = std::time(nullptr);
}

void FontManager::addCharacter(uint8_t index, const CharacterBitmap& character) {
    characters_[index] = character;
}

bool FontManager::hasCharacter(uint8_t index) const {
    return characters_.find(index) != characters_.end();
}

const CharacterBitmap& FontManager::getCharacter(uint8_t index) const {
    auto it = characters_.find(index);
    if (it != characters_.end()) {
        return it->second;
    }
    throw std::runtime_error("Character at index " + std::to_string(index) + " not found");
}

void FontManager::removeCharacter(uint8_t index) {
    characters_.erase(index);
}

void FontManager::clear() {
    characters_.clear();
}

bool FontManager::saveToFile(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        // Write magic signature
        file.write(MAGIC_SIGNATURE, 4);
        
        // Write format version
        file.write(reinterpret_cast<const char*>(&FORMAT_VERSION), sizeof(FORMAT_VERSION));
        
        // Write author (fixed 16 bytes)
        char author_buffer[16] = {0};
        strncpy(author_buffer, author_.c_str(), 15);
        file.write(author_buffer, 16);
        
        // Write creation time
        file.write(reinterpret_cast<const char*>(&creation_time_), sizeof(creation_time_));
        
        // Write character count
        uint8_t char_count = static_cast<uint8_t>(characters_.size());
        file.write(reinterpret_cast<const char*>(&char_count), sizeof(char_count));
        
        // Padding to reach header size
        size_t padding_size = HEADER_SIZE - (4 + 1 + 16 + sizeof(creation_time_) + 1);
        std::vector<char> padding(padding_size, 0);
        file.write(padding.data(), padding_size);
        
        // Write characters
        for (const auto& pair : characters_) {
            uint8_t index = pair.first;
            const auto& character = pair.second;
            
            // Write index
            file.write(reinterpret_cast<const char*>(&index), sizeof(index));
            
            // Write name (16 bytes)
            char name_buffer[16] = {0};
            strncpy(name_buffer, character.getName().c_str(), 15);
            file.write(name_buffer, 16);
            
            // Write description length and description
            uint8_t desc_len = static_cast<uint8_t>(std::min(character.getDescription().length(), size_t(255)));
            file.write(reinterpret_cast<const char*>(&desc_len), sizeof(desc_len));
            file.write(character.getDescription().c_str(), desc_len);
            
            // Write frame count
            uint8_t frame_count = static_cast<uint8_t>(character.getFrameCount());
            file.write(reinterpret_cast<const char*>(&frame_count), sizeof(frame_count));
            
            // Write each frame
            for (size_t f = 0; f < character.getFrameCount(); ++f) {
                auto frame_data = character.getFrame(f);
                file.write(reinterpret_cast<const char*>(frame_data.data()), LCD::CUSTOM_CHAR_HEIGHT);
            }
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

bool FontManager::loadFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        // Read and verify magic signature
        char magic[4];
        file.read(magic, 4);
        if (std::string(magic, 4) != std::string(MAGIC_SIGNATURE, 4)) {
            return false;
        }
        
        // Read format version
        uint8_t version;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        if (version != FORMAT_VERSION) {
            return false;
        }
        
        // Read author
        char author_buffer[16];
        file.read(author_buffer, 16);
        author_ = std::string(author_buffer, 16);
        // Remove null characters
        author_.erase(std::find(author_.begin(), author_.end(), '\0'), author_.end());
        
        // Read creation time
        file.read(reinterpret_cast<char*>(&creation_time_), sizeof(creation_time_));
        
        // Read character count
        uint8_t char_count;
        file.read(reinterpret_cast<char*>(&char_count), sizeof(char_count));
        
        // Skip padding
        size_t padding_size = HEADER_SIZE - (4 + 1 + 16 + sizeof(creation_time_) + 1);
        file.seekg(padding_size, std::ios::cur);
        
        // Clear existing characters
        characters_.clear();
        
        // Read characters
        for (uint8_t i = 0; i < char_count; ++i) {
            // Read index
            uint8_t index;
            file.read(reinterpret_cast<char*>(&index), sizeof(index));
            
            // Read name
            char name_buffer[16];
            file.read(name_buffer, 16);
            std::string name(name_buffer, 16);
            name.erase(std::find(name.begin(), name.end(), '\0'), name.end());
            
            CharacterBitmap character(name);
            
            // Read description
            uint8_t desc_len;
            file.read(reinterpret_cast<char*>(&desc_len), sizeof(desc_len));
            if (desc_len > 0) {
                std::string description(desc_len, '\0');
                file.read(&description[0], desc_len);
                character.setDescription(description);
            }
            
            // Read frame count
            uint8_t frame_count;
            file.read(reinterpret_cast<char*>(&frame_count), sizeof(frame_count));
            
            // Read frames
            for (uint8_t f = 0; f < frame_count; ++f) {
                std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT> frame_data;
                file.read(reinterpret_cast<char*>(frame_data.data()), LCD::CUSTOM_CHAR_HEIGHT);
                character.setFrame(f, frame_data);
            }
            
            characters_[index] = character;
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

void FontManager::printInfo() const {
    std::cout << "=== Font Manager Info ===\n";
    std::cout << "Author: " << author_ << "\n";
    
    // Format creation time
    char time_str[100];
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", 
                  std::localtime(&creation_time_));
    std::cout << "Created: " << time_str << "\n";
    std::cout << "Characters: " << characters_.size() << "\n\n";
    
    for (const auto& pair : characters_) {
        std::cout << "Index " << static_cast<int>(pair.first) << ": " 
                  << pair.second.getName() << " (" 
                  << pair.second.getFrameCount() << " frames)\n";
    }
}

void FontManager::printAllCharacters() const {
    printInfo();
    std::cout << "\n=== Character Details ===\n";
    
    for (const auto& pair : characters_) {
        std::cout << "\nIndex " << static_cast<int>(pair.first) << ":\n";
        pair.second.printAllFrames();
    }
}

} // namespace FontEdit
} // namespace LCD2USB
