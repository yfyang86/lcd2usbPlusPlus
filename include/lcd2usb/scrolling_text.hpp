/*
 * lcd2usb/scrolling_text.hpp - Scrolling text display utility
 * Part of LCD2USB++ library
 */

#pragma once

#include "lcd2usb.hpp"
#include <string>

namespace LCD2USB {

/**
 * Manages scrolling text with animated character padding
 */
class ScrollingText {
private:
    std::string original_text_;
    std::string scroll_text_;
    size_t position_;
    size_t scroll_speed_;

    void updateScrollText();

public:
    explicit ScrollingText(const std::string& text = "");
    
    // Text management
    void update(const std::string& new_text);
    const std::string& getOriginalText() const { return original_text_; }
    
    // Scrolling control
    std::string getCurrentView() const;
    void advance();
    void reset() { position_ = 0; }
    
    // Configuration
    void setScrollSpeed(size_t speed) { scroll_speed_ = speed; }
    size_t getScrollSpeed() const { return scroll_speed_; }
    
    // Information
    size_t getTextLength() const { return original_text_.length(); }
    size_t getBufferLength() const { return scroll_text_.length(); }
    size_t getCurrentPosition() const { return position_; }
    bool needsScrolling() const { return original_text_.length() > LCD::DISPLAY_WIDTH; }
};

/**
 * Application framework for LCD2USB applications
 */
class LCD2USBApp {
protected:
    std::unique_ptr<LCDDisplay> display_;

public:
    LCD2USBApp();
    virtual ~LCD2USBApp();

    // Core lifecycle
    virtual bool initialize();
    virtual void run() = 0;
    virtual void shutdown();

    // Utilities
    LCDDisplay* getDisplay() { return display_.get(); }
    const LCDDisplay* getDisplay() const { return display_.get(); }
};

} // namespace LCD2USB