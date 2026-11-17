/*
 * lcd_display.cpp - LCD display communication implementation
 * Part of LCD2USB++ library
 */

#include "lcd2usb/lcd_display.hpp"
#include "lcd2usb/custom_char.hpp"
#include <iostream>
#include <usb.h>

namespace LCD2USB {

LCDDisplay::LCDDisplay() 
    : handle_(nullptr), buffer_current_type_(-1), buffer_current_fill_(0) {
}

LCDDisplay::~LCDDisplay() { 
    if (handle_) {
        std::cout << "Closing USB connection..." << std::endl;
        usb_close(handle_);
        handle_ = nullptr;
    }
}

bool LCDDisplay::initialize() {
    usb_init();
    usb_find_busses();
    usb_find_devices();

    for (auto bus = usb_get_busses(); bus; bus = bus->next) {
        for (auto dev = bus->devices; dev; dev = dev->next) {
            if (dev->descriptor.idVendor == USB::LCD2USB_VID &&
                dev->descriptor.idProduct == USB::LCD2USB_PID) {
                
                std::cout << "Found LCD2USB device on bus " << bus->dirname 
                         << " device " << dev->filename << std::endl;

                handle_ = usb_open(dev);
                if (!handle_) {
                    std::cerr << "Error: Cannot open USB device: " << usb_strerror() << std::endl;
                    return false;
                }
                
                // Test communication immediately after opening
                unsigned short test_val = 0x1234;
                unsigned short ret_val;
                
                int test_result = usb_control_msg(handle_,
                                                USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                                LCD::ECHO, test_val, 0, (char*)&ret_val, sizeof(ret_val), 1000);
                
                if (test_result < 0 || test_val != ret_val) {
                    std::cerr << "Device communication test failed - device may be in use or malfunctioning" << std::endl;
                    usb_close(handle_);
                    handle_ = nullptr;
                    return false;
                }
                
                std::cout << "LCD2USB device opened and communication verified" << std::endl;
                return true;
            }
        }
    }

    std::cerr << "Error: Could not find LCD2USB device" << std::endl;
    return false;
}

void LCDDisplay::performEchoTest() {
    if (!handle_) return;
    
    int errors = 0;
    const int ECHO_NUM = 10;
    
    for (int i = 0; i < ECHO_NUM; i++) {
        unsigned short val = rand() & 0xffff;
        unsigned short ret;
        
        int nBytes = usb_control_msg(handle_,
                                   USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                                   LCD::ECHO, val, 0, (char*)&ret, sizeof(ret), 1000);
        
        if (nBytes < 0) {
            std::cerr << "USB echo request failed: " << usb_strerror() << " (code: " << nBytes << ")" << std::endl;
            return;
        }
        
        if (val != ret) errors++;
    }
    
    if (errors) {
        std::cerr << "ERROR: " << errors << " out of " << ECHO_NUM << " echo transfers failed!" << std::endl;
    } else {
        std::cout << "Echo test successful!" << std::endl;
    }
}

void LCDDisplay::displayVersion() {
    if (!handle_) return;
    
    unsigned char buffer[2];
    int nBytes = usb_control_msg(handle_,
                               USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                               LCD::GET_FWVER, 0, 0, (char*)buffer, sizeof(buffer), 1000);
    
    if (nBytes >= 0) {
        int ver = buffer[0] + 256 * buffer[1];
        std::cout << "Firmware version " << (ver & 0xff) << "." << (ver >> 8) << std::endl;
    }
}

void LCDDisplay::clear() {
    if (!handle_) return;
    
    lcdCommand(LCD::BOTH, 0x01);  // Clear display
    lcdCommand(LCD::BOTH, 0x03);  // Return home
    flushBuffer();
}

void LCDDisplay::setContrast(int value) {
    if (!handle_) return;
    
    int result = usb_control_msg(handle_, USB_TYPE_VENDOR, LCD::SET_CONTRAST, value, 0, 
                               nullptr, 0, 1000);
    if (result < 0) {
        std::cerr << "Failed to set contrast: " << usb_strerror() << " (code: " << result << ")" << std::endl;
    }
}

void LCDDisplay::setBrightness(int value) {
    if (!handle_) return;
    
    int result = usb_control_msg(handle_, USB_TYPE_VENDOR, LCD::SET_BRIGHTNESS, value, 0,
                               nullptr, 0, 1000);
    if (result < 0) {
        std::cerr << "Failed to set brightness: " << usb_strerror() << " (code: " << result << ")" << std::endl;
    }
}

void LCDDisplay::writeToLine(int line, const std::string& text) {
    if (!handle_) return;
    
    int ctrl = LCD::CTRL_0;
    
    // Position cursor to start of line using the canonical row-address table.
    // Falls back to row 0 for any out-of-range line.
    const unsigned char line_addr =
        (line >= 0 && line < static_cast<int>(LCD::ROW_ADDR.size()))
            ? LCD::ROW_ADDR[line]
            : LCD::ROW_ADDR[0];
    lcdCommand(ctrl, line_addr);
    
    // Write text data
    for (char c : text) {
        enqueue(LCD::DATA | ctrl, static_cast<unsigned char>(c));
    }
    
    flushBuffer();
}

void LCDDisplay::setCursor(int col, int row) {
    if (!handle_) return;
    
    int ctrl = LCD::CTRL_0;
    
    // HD44780 DDRAM row addresses live in a small lookup (see lcd2usb.hpp)
    // because they are *not* sequential on 20x4 displays.
    const unsigned char base =
        (row >= 0 && row < static_cast<int>(LCD::ROW_ADDR.size()))
            ? LCD::ROW_ADDR[row]
            : LCD::ROW_ADDR[0];
    const unsigned char address = base + static_cast<unsigned char>(col);
    
    lcdCommand(ctrl, address);
    flushBuffer();
}

void LCDDisplay::print(const std::string& text) {
    if (!handle_) return;
    
    int ctrl = LCD::CTRL_0;
    
    // Write text data at current cursor position
    for (char c : text) {
        enqueue(LCD::DATA | ctrl, static_cast<unsigned char>(c));
    }
    
    flushBuffer();
}

void LCDDisplay::printBytes(const uint8_t* data, size_t len) {
    if (!handle_ || data == nullptr) return;

    int ctrl = LCD::CTRL_0;
    // Queue every byte verbatim — 0x00 is a legal CGRAM slot reference and
    // must not be treated as a string terminator.
    for (size_t i = 0; i < len; ++i) {
        enqueue(LCD::DATA | ctrl, data[i]);
    }
    flushBuffer();
}

void LCDDisplay::print(char c) {
    if (!handle_) return;
    
    int ctrl = LCD::CTRL_0;
    enqueue(LCD::DATA | ctrl, static_cast<unsigned char>(c));
    flushBuffer();
}

void LCDDisplay::setupMatchGuy(int slot, int frame) {
    if (!handle_ || slot >= LCD::MAX_CUSTOM_CHARS) return;
    
    auto character = CharacterFactory::createMatchGuy(frame);
    uploadCustomCharacter(slot, character.toBytes());
}

void LCDDisplay::setupSpinner(int slot, int frame) {
    if (!handle_ || slot >= LCD::MAX_CUSTOM_CHARS) return;
    
    auto character = CharacterFactory::createSpinner(frame);
    uploadCustomCharacter(slot, character.toBytes());
}

void LCDDisplay::enqueue(int type, unsigned char value) {
    // Flush buffer if switching to different command type
    if (buffer_current_type_ >= 0 && buffer_current_type_ != type) {
        flushBuffer();
    }
    
    // Set buffer type if empty
    if (buffer_current_type_ == -1) {
        buffer_current_type_ = type;
    }
    
    buffer_[buffer_current_fill_++] = value;
    
    if (buffer_current_fill_ == LCD::BUFFER_MAX_CMD) {
        flushBuffer();
    }
}

void LCDDisplay::flushBuffer() {
    if (!handle_ || buffer_current_fill_ == 0) return;
    
    // Build request byte (original protocol)
    int request = buffer_current_type_ | (buffer_current_fill_ - 1);
    
    // Fill value and index with buffer contents (original protocol)
    int value = buffer_[0] | (buffer_[1] << 8);
    int index = buffer_[2] | (buffer_[3] << 8);
    
    // Send using value/index parameters, not data buffer
    int result = usb_control_msg(handle_, USB_TYPE_VENDOR, request,
                               value, index, nullptr, 0, 1000);
    if (result < 0) {
        std::cerr << "USB flush failed: " << usb_strerror() << " (code: " << result << ")" << std::endl;
    }
    
    buffer_current_fill_ = 0;
    buffer_current_type_ = -1;
}

void LCDDisplay::lcdCommand(int ctrl, unsigned char cmd) {
    enqueue(LCD::CMD | ctrl, cmd);
}

void LCDDisplay::uploadCustomCharacter(int slot, const std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT>& pattern) {
    if (slot >= LCD::MAX_CUSTOM_CHARS) return;
    
    int ctrl = LCD::CTRL_0;
    
    // Set CGRAM address for this character slot
    unsigned char cgram_addr = LCD::CGRAM_BASE_ADDR + (slot * 8);
    lcdCommand(ctrl, cgram_addr);
    
    // Upload the 8 bytes of character data
    for (int i = 0; i < LCD::CUSTOM_CHAR_HEIGHT; ++i) {
        enqueue(LCD::DATA | ctrl, pattern[i]);
    }
    
    flushBuffer();
    
    // CRITICAL: Must return to DDRAM mode after CGRAM write
    // The LCD will stay in CGRAM mode until we send a DDRAM address command
    // Set DDRAM address 0 to switch modes (cursor position will be set by caller)
    lcdCommand(ctrl, 0x80);
    flushBuffer();
}

void LCDDisplay::uploadCustomCharacter(int slot, const CustomChar& character) {
    uploadCustomCharacter(slot, character.toBytes());
}

} // namespace LCD2USB