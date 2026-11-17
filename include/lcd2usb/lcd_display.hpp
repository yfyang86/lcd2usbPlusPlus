/*
 * lcd2usb/lcd_display.hpp - LCD display communication interface
 * Part of LCD2USB++ library
 */

#pragma once

#include "lcd2usb.hpp"
#include <string>
#include <array>
#include <memory>

// Forward declaration for USB handle
struct usb_dev_handle;

namespace LCD2USB {

// Forward declarations
class CustomChar;

/**
 * Main LCD display communication class
 * Handles USB communication with LCD2USB device
 */
class LCDDisplay {
private:
    usb_dev_handle* handle_;
    int buffer_current_type_;
    int buffer_current_fill_;
    std::array<unsigned char, LCD::BUFFER_MAX_CMD> buffer_;

    // Private helper methods
    void enqueue(int type, unsigned char value);
    void flushBuffer();
    void lcdCommand(int ctrl, unsigned char cmd);

public:
    LCDDisplay();
    ~LCDDisplay();

    // Non-copyable
    LCDDisplay(const LCDDisplay&) = delete;
    LCDDisplay& operator=(const LCDDisplay&) = delete;

    // Core functionality
    bool initialize();
    void performEchoTest();
    void displayVersion();
    
    // Display control
    void clear();
    void setContrast(int value);
    void setBrightness(int value);
    
    // Text output
    void writeToLine(int line, const std::string& text);
    void setCursor(int col, int row);
    void print(const std::string& text);
    void print(char c);
    // Raw byte run — writes len bytes verbatim at the current cursor, batched
    // into the minimum number of USB transfers. Use this when the payload may
    // contain 0x00 (CGRAM slot 0) or is not a NUL-terminated string.
    void printBytes(const uint8_t* data, size_t len);
    
    // Custom characters
    void uploadCustomCharacter(int slot, const std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT>& pattern);
    void uploadCustomCharacter(int slot, const CustomChar& character);
    void setupMatchGuy(int slot, int frame);
    void setupSpinner(int slot, int frame);
    
    // Status
    bool isConnected() const { return handle_ != nullptr; }
};

} // namespace LCD2USB