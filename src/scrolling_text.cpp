/*
 * scrolling_text.cpp - Scrolling text utilities implementation
 * Part of LCD2USB++ library
 */

#include "lcd2usb/scrolling_text.hpp"
#include <algorithm>
#include <thread>

namespace LCD2USB {

// ScrollingText implementation
ScrollingText::ScrollingText(LCDDisplay& display) 
    : display_(display), text_(""), 
      start_col_(0), start_row_(0), 
      visible_width_(LCD::LCD_WIDTH), visible_height_(1),
      scroll_offset_(0), scroll_delay_(200), 
      direction_(ScrollDirection::LEFT_TO_RIGHT), 
      auto_scroll_(false), scroll_thread_(), stop_scroll_(false) {
}

ScrollingText::~ScrollingText() {
    stopScrolling();
}

void ScrollingText::setText(const std::string& text) {
    text_ = text;
    scroll_offset_ = 0;
}

void ScrollingText::setPosition(int col, int row) {
    start_col_ = col;
    start_row_ = row;
}

void ScrollingText::setViewport(int width, int height) {
    visible_width_ = std::max(1, std::min(width, LCD::LCD_WIDTH));
    visible_height_ = std::max(1, std::min(height, LCD::LCD_HEIGHT));
}

void ScrollingText::setScrollDelay(int milliseconds) {
    scroll_delay_ = std::max(50, milliseconds); // Minimum 50ms
}

void ScrollingText::setDirection(ScrollDirection dir) {
    direction_ = dir;
}

void ScrollingText::displayCurrentFrame() {
    if (text_.empty()) {
        return;
    }
    
    switch (direction_) {
        case ScrollDirection::LEFT_TO_RIGHT:
            displayHorizontalScroll();
            break;
        case ScrollDirection::RIGHT_TO_LEFT:
            displayHorizontalScroll();
            break;
        case ScrollDirection::UP_TO_DOWN:
            displayVerticalScroll();
            break;
        case ScrollDirection::DOWN_TO_UP:
            displayVerticalScroll();
            break;
    }
}

void ScrollingText::scrollStep() {
    switch (direction_) {
        case ScrollDirection::LEFT_TO_RIGHT:
            scroll_offset_--;
            if (scroll_offset_ < -(int)text_.length()) {
                scroll_offset_ = visible_width_;
            }
            break;
        case ScrollDirection::RIGHT_TO_LEFT:
            scroll_offset_++;
            if (scroll_offset_ > visible_width_) {
                scroll_offset_ = -(int)text_.length();
            }
            break;
        case ScrollDirection::UP_TO_DOWN:
        case ScrollDirection::DOWN_TO_UP:
            // For vertical scrolling, we'd need multi-line text support
            // For now, just do horizontal movement
            scroll_offset_++;
            if (scroll_offset_ > (int)text_.length()) {
                scroll_offset_ = 0;
            }
            break;
    }
    
    displayCurrentFrame();
}

void ScrollingText::startScrolling() {
    if (auto_scroll_) {
        return; // Already scrolling
    }
    
    auto_scroll_ = true;
    stop_scroll_ = false;
    
    scroll_thread_ = std::thread([this]() {
        while (!stop_scroll_) {
            scrollStep();
            std::this_thread::sleep_for(std::chrono::milliseconds(scroll_delay_));
        }
    });
}

void ScrollingText::stopScrolling() {
    if (auto_scroll_) {
        stop_scroll_ = true;
        if (scroll_thread_.joinable()) {
            scroll_thread_.join();
        }
        auto_scroll_ = false;
    }
}

bool ScrollingText::isScrolling() const {
    return auto_scroll_;
}

void ScrollingText::reset() {
    scroll_offset_ = 0;
    displayCurrentFrame();
}

void ScrollingText::displayHorizontalScroll() {
    std::string display_text;
    
    if (direction_ == ScrollDirection::LEFT_TO_RIGHT) {
        // Text moves from right to left across screen
        int text_start = scroll_offset_;
        
        for (int i = 0; i < visible_width_; ++i) {
            int text_pos = text_start + i;
            if (text_pos >= 0 && text_pos < (int)text_.length()) {
                display_text += text_[text_pos];
            } else {
                display_text += ' ';
            }
        }
    } else {
        // RIGHT_TO_LEFT: Text moves from left to right across screen
        int text_start = scroll_offset_ - visible_width_;
        
        for (int i = 0; i < visible_width_; ++i) {
            int text_pos = text_start + i;
            if (text_pos >= 0 && text_pos < (int)text_.length()) {
                display_text += text_[text_pos];
            } else {
                display_text += ' ';
            }
        }
    }
    
    // Pad to full width
    while ((int)display_text.length() < visible_width_) {
        display_text += ' ';
    }
    
    display_.setCursor(start_col_, start_row_);
    display_.print(display_text);
}

void ScrollingText::displayVerticalScroll() {
    // For vertical scrolling, we'd need to split text into lines
    // For now, just display as single line with horizontal offset
    std::string display_text;
    
    int start = scroll_offset_ % (int)(text_.length() + visible_width_);
    for (int i = 0; i < visible_width_; ++i) {
        int pos = (start + i) % (int)(text_.length() + visible_width_);
        if (pos < (int)text_.length()) {
            display_text += text_[pos];
        } else {
            display_text += ' ';
        }
    }
    
    display_.setCursor(start_col_, start_row_);
    display_.print(display_text);
}

// MarqueeText implementation
MarqueeText::MarqueeText(LCDDisplay& display, const std::string& text, int row)
    : scrolling_text_(display), text_width_(LCD::LCD_WIDTH), speed_(1), enabled_(false) {
    
    scrolling_text_.setText(text);
    scrolling_text_.setPosition(0, row);
    scrolling_text_.setViewport(text_width_, 1);
    scrolling_text_.setDirection(ScrollDirection::LEFT_TO_RIGHT);
}

void MarqueeText::setText(const std::string& text) {
    scrolling_text_.setText(text);
}

void MarqueeText::setRow(int row) {
    scrolling_text_.setPosition(0, row);
}

void MarqueeText::setWidth(int width) {
    text_width_ = std::max(1, std::min(width, LCD::LCD_WIDTH));
    scrolling_text_.setViewport(text_width_, 1);
}

void MarqueeText::setSpeed(int speed) {
    speed_ = std::max(1, speed);
    // Convert speed to delay (higher speed = lower delay)
    int delay = 1000 / speed_; // Base delay
    delay = std::max(50, std::min(delay, 2000)); // Clamp between 50ms and 2s
    scrolling_text_.setScrollDelay(delay);
}

void MarqueeText::start() {
    if (!enabled_) {
        enabled_ = true;
        scrolling_text_.startScrolling();
    }
}

void MarqueeText::stop() {
    if (enabled_) {
        enabled_ = false;
        scrolling_text_.stopScrolling();
    }
}

void MarqueeText::update() {
    if (enabled_) {
        scrolling_text_.displayCurrentFrame();
    }
}

bool MarqueeText::isActive() const {
    return enabled_ && scrolling_text_.isScrolling();
}

// TextBanner implementation
TextBanner::TextBanner(LCDDisplay& display) : display_(display), current_line_(0) {
    messages_.reserve(4); // Typical LCD height
}

void TextBanner::addMessage(const std::string& message, int duration_ms) {
    messages_.push_back({message, duration_ms});
}

void TextBanner::clear() {
    messages_.clear();
    current_line_ = 0;
}

void TextBanner::start() {
    if (messages_.empty()) {
        return;
    }
    
    current_line_ = 0;
    
    for (size_t i = 0; i < messages_.size(); ++i) {
        if ((int)i >= LCD::LCD_HEIGHT) {
            break; // Don't exceed display height
        }
        
        const auto& message = messages_[i];
        
        // Clear line first
        display_.setCursor(0, i);
        std::string clear_line(LCD::LCD_WIDTH, ' ');
        display_.print(clear_line);
        
        // Display message
        display_.setCursor(0, i);
        std::string display_msg = message.text;
        if ((int)display_msg.length() > LCD::LCD_WIDTH) {
            display_msg = display_msg.substr(0, LCD::LCD_WIDTH);
        }
        display_.print(display_msg);
        
        // Wait for specified duration
        if (message.duration_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(message.duration_ms));
        }
    }
}

void TextBanner::displayLine(int line, const std::string& message) {
    if (line < 0 || line >= LCD::LCD_HEIGHT) {
        return;
    }
    
    // Clear line
    display_.setCursor(0, line);
    std::string clear_line(LCD::LCD_WIDTH, ' ');
    display_.print(clear_line);
    
    // Display message
    display_.setCursor(0, line);
    std::string display_msg = message;
    if ((int)display_msg.length() > LCD::LCD_WIDTH) {
        display_msg = display_msg.substr(0, LCD::LCD_WIDTH);
    }
    display_.print(display_msg);
}

size_t TextBanner::getMessageCount() const {
    return messages_.size();
}

} // namespace LCD2USB