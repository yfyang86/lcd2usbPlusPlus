/*
 * lcd2usb/font_editor.hpp - Font file management and character collections
 * Part of LCD2USB++ library
 */

#pragma once

#include "custom_char.hpp"
#include <string>
#include <map>
#include <vector>
#include <ctime>

namespace LCD2USB {

namespace FontEdit {

/**
 * Represents an animated character with multiple frames
 */
class CharacterBitmap {
private:
    std::string name_;
    std::string description_;
    std::vector<std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT>> frames_;

public:
    explicit CharacterBitmap(const std::string& name = "");
    
    // Metadata
    void setName(const std::string& name) { name_ = name; }
    void setDescription(const std::string& desc) { description_ = desc; }
    const std::string& getName() const { return name_; }
    const std::string& getDescription() const { return description_; }
    
    // Frame management
    void setFrame(size_t frameIndex, const std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT>& data);
    std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT> getFrame(size_t frameIndex) const;
    size_t getFrameCount() const { return frames_.size(); }
    bool isAnimated() const { return frames_.size() > 1; }
    
    // Display
    void printCharacter(size_t frameIndex = 0) const;
    void printAllFrames() const;
};

/**
 * Manages collections of custom characters and saves/loads font files
 */
class FontManager {
private:
    std::string author_;
    std::time_t creation_time_;
    std::map<uint8_t, CharacterBitmap> characters_;

public:
    explicit FontManager(const std::string& author = "Unknown");
    
    // Character management
    void addCharacter(uint8_t index, const CharacterBitmap& character);
    bool hasCharacter(uint8_t index) const;
    const CharacterBitmap& getCharacter(uint8_t index) const;
    void removeCharacter(uint8_t index);
    void clear();
    
    // Metadata
    void setAuthor(const std::string& author) { author_ = author; }
    const std::string& getAuthor() const { return author_; }
    std::time_t getCreationTime() const { return creation_time_; }
    size_t getCharacterCount() const { return characters_.size(); }
    
    // File operations
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);
    
    // Display
    void printInfo() const;
    void printAllCharacters() const;
    
    // Iteration
    const std::map<uint8_t, CharacterBitmap>& getCharacters() const { return characters_; }
};

// File format constants
constexpr char MAGIC_SIGNATURE[4] = {'L', 'C', 'F', '1'};
constexpr uint8_t FORMAT_VERSION = 1;
constexpr size_t HEADER_SIZE = 32;

} // namespace FontEdit

} // namespace LCD2USB