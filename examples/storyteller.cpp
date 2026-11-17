/*
 * storyteller.cpp - Text file reader with scrolling display and progress bar
 * 
 * Features:
 * - Load text files (txt, md, any text format)
 * - Configurable screen dimensions (default: 16 width x 2 rows)
 * - Adjustable scroll speed (default: 2 seconds)
 * - Progress bars using custom characters
 * - Vertical scrolling with chunk management
 * - Support for 1-4 row displays
 * 
 * Usage: ./storyteller [--file path] [--width chars] [--rows lines] [--speed seconds]
 */

#include "lcd2usb/lcd2usb_lib.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <algorithm>
#include <thread>
#include <chrono>
#include <signal.h>
#include <getopt.h>

using namespace LCD2USB;

volatile bool running = true;

void signalHandler(int) {
    std::cout << "\nShutting down storyteller..." << std::endl;
    running = false;
}

class ProgressBar {
private:
    LCDDisplay* display;
    CustomChar empty_bar;
    CustomChar quarter_bar;
    CustomChar half_bar;
    CustomChar three_quarter_bar;
    CustomChar full_bar;
    
public:
    ProgressBar(LCDDisplay* lcd) : display(lcd) {
        // Create progress bar custom characters using pixel manipulation
        // Empty bar (character 0)
        empty_bar.clear();
        for (int y = 0; y < 8; y++) {
            empty_bar.setPixel(0, y, true);  // Left border
            empty_bar.setPixel(4, y, true);  // Right border
        }
        
        // Quarter bar (character 1)
        quarter_bar.clear();
        for (int y = 0; y < 8; y++) {
            quarter_bar.setPixel(0, y, true);  // Left border
            quarter_bar.setPixel(1, y, true);  // Quarter fill
            quarter_bar.setPixel(4, y, true);  // Right border
        }
        
        // Half bar (character 2)  
        half_bar.clear();
        for (int y = 0; y < 8; y++) {
            half_bar.setPixel(0, y, true);  // Left border
            half_bar.setPixel(1, y, true);  // Half fill
            half_bar.setPixel(2, y, true);  // Half fill
            half_bar.setPixel(4, y, true);  // Right border
        }
        
        // Three quarter bar (character 3)
        three_quarter_bar.clear();
        for (int y = 0; y < 8; y++) {
            three_quarter_bar.setPixel(0, y, true);  // Left border
            three_quarter_bar.setPixel(1, y, true);  // Three quarter fill
            three_quarter_bar.setPixel(2, y, true);  // Three quarter fill
            three_quarter_bar.setPixel(3, y, true);  // Three quarter fill
            three_quarter_bar.setPixel(4, y, true);  // Right border
        }
        
        // Full bar (character 4)
        full_bar.clear();
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 5; x++) {
                full_bar.setPixel(x, y, true);  // Fully filled
            }
        }
        
        // Load custom characters
        display->uploadCustomCharacter(0, empty_bar);
        display->uploadCustomCharacter(1, quarter_bar);
        display->uploadCustomCharacter(2, half_bar);
        display->uploadCustomCharacter(3, three_quarter_bar);
        display->uploadCustomCharacter(4, full_bar);
    }
    
    void showProgress(int col, int line, double progress) {
        // progress is 0.0 to 1.0
        progress = std::max(0.0, std::min(1.0, progress));
        
        // Calculate which character to show based on progress
        char progress_char;
        if (progress < 0.2) progress_char = 0;        // empty
        else if (progress < 0.4) progress_char = 1;   // quarter
        else if (progress < 0.6) progress_char = 2;   // half
        else if (progress < 0.8) progress_char = 3;   // three quarter
        else progress_char = 4;                       // full
        
        display->setCursor(col, line);  // Right margin
        display->print(progress_char);  // Print custom character
    }
};

class TextStoryTeller {
private:
    LCDDisplay* display;
    ProgressBar progress_bar;
    std::string filename;
    int screen_width;
    int screen_rows;
    int scroll_speed;
    bool show_scrollbar;
    
    std::vector<std::string> text_chunks;
    std::queue<std::string> line_queue;
    size_t current_chunk;
    size_t total_file_size;
    size_t processed_size;
    
public:
    TextStoryTeller(LCDDisplay* lcd, const std::string& file, int width, int rows, int speed,
                    bool scrollbar = true) 
        : display(lcd), progress_bar(lcd), filename(file), 
          screen_width(width), screen_rows(rows), scroll_speed(speed),
          show_scrollbar(scrollbar),
          current_chunk(0), total_file_size(0), processed_size(0) {}
    
    bool loadFile() {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return false;
        }
        
        // Get total file size
        total_file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::cout << "Loading file: " << filename << " (" << total_file_size << " bytes)" << std::endl;
        
        // Read file in 256-byte chunks
        const size_t chunk_size = 256;
        char buffer[chunk_size + 1];
        
        while (file.read(buffer, chunk_size) || file.gcount() > 0) {
            size_t bytes_read = file.gcount();
            buffer[bytes_read] = '\0';
            text_chunks.push_back(std::string(buffer, bytes_read));
        }
        
        file.close();
        
        std::cout << "Loaded " << text_chunks.size() << " chunks" << std::endl;
        return true;
    }
    
    void processChunkToLines(size_t chunk_index) {
        if (chunk_index >= text_chunks.size()) return;
        
        std::string chunk = text_chunks[chunk_index];
        std::istringstream stream(chunk);
        std::string word;
        std::string current_line;
        
        // Available width for text. When the scrollbar is shown the rightmost
        // column is reserved for it; when disabled, text owns the full width.
        size_t text_width = show_scrollbar
            ? static_cast<size_t>(screen_width - 1)
            : static_cast<size_t>(screen_width);
        
        while (stream >> word) {
            // Check if adding this word would exceed line width
            if (current_line.empty()) {
                current_line = word;
            } else if (current_line.length() + 1 + word.length() <= text_width) {
                current_line += " " + word;
            } else {
                // Line is full, queue it and start new line
                // Pad line to exact width for clean scrolling
                while (current_line.length() < text_width) {
                    current_line += " ";
                }
                line_queue.push(current_line);
                current_line = word;
            }
        }
        
        // Add remaining content
        if (!current_line.empty()) {
            while (current_line.length() < text_width) {
                current_line += " ";
            }
            line_queue.push(current_line);
        }
        
        processed_size += chunk.length();
    }
    
    void displayStory() {
        if (text_chunks.empty()) return;
        
        // Start with first chunk
        processChunkToLines(current_chunk);
        
        const int text_cols = show_scrollbar ? (screen_width - 1) : screen_width;
        std::vector<std::string> display_lines(screen_rows, std::string(text_cols, ' '));
        
        while (running && (current_chunk < text_chunks.size() || !line_queue.empty())) {
            // Load more lines if queue is getting low and we have more chunks
            if (line_queue.size() < 5 && current_chunk + 1 < text_chunks.size()) {
                current_chunk++;
                processChunkToLines(current_chunk);
            }
            
            // Scroll up: shift all lines up by one
            for (int i = 0; i < screen_rows - 1; i++) {
                display_lines[i] = display_lines[i + 1];
            }
            
            // Get new line for bottom row
            if (!line_queue.empty()) {
                display_lines[screen_rows - 1] = line_queue.front();
                line_queue.pop();
            } else {
                display_lines[screen_rows - 1] = std::string(text_cols, ' ');
            }
            
            // Update display
            display->clear();
            for (int row = 0; row < screen_rows; row++) {
                display->setCursor(0, row);
                display->print(display_lines[row]);
            }
            
            if (show_scrollbar) {
                // Update progress bars
                // Top line: Current chunk progress
                double chunk_progress = (double)current_chunk / text_chunks.size();
                progress_bar.showProgress(screen_width - 1, 0, chunk_progress);
                
                // Bottom line: Total file progress
                double total_progress = (double)processed_size / total_file_size;
                progress_bar.showProgress(screen_width - 1, screen_rows - 1, total_progress);
            }
            
            // Wait for scroll interval
            std::this_thread::sleep_for(std::chrono::seconds(scroll_speed));
        }
        
        // Show completion message
        if (!running) {
            display->clear();
            display->setCursor(0, 0);
            display->print("Story stopped");
        } else {
            display->clear();
            display->setCursor(0, 0);
            display->print("Story complete!");
            if (screen_rows > 1) {
                display->setCursor(0, 1);
                display->print("Press Ctrl+C");
            }
            
            if (show_scrollbar) {
                // Show 100% progress
                progress_bar.showProgress(screen_width - 1, 0, 1.0);
                if (screen_rows > 1) {
                    progress_bar.showProgress(screen_width - 1, screen_rows - 1, 1.0);
                }
            }
        }
    }
};

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --file <path>        Text file to display (default: assets/story.txt)" << std::endl;
    std::cout << "  --width <n>          Screen width in characters, 2-20 (default: 16)" << std::endl;
    std::cout << "  --rows <n>           Number of rows, 1-4 (default: 2)" << std::endl;
    std::cout << "  --speed <seconds>    Scroll speed in seconds (default: 2)" << std::endl;
    std::cout << "  --disable-scrollbar  Use the full display width for text; hide the progress bar" << std::endl;
    std::cout << "  --help               Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << " --file story.txt --width 20 --rows 4 --speed 1" << std::endl;
    std::cout << "  " << program_name << " --width 16 --rows 2 --disable-scrollbar" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== LCD2USB++ Story Teller ===" << std::endl;
    
    // Default parameters
    std::string filename = "assets/story.txt";
    int screen_width = 16;
    int screen_rows = 2;
    int scroll_speed = 2;
    bool show_scrollbar = true;
    
    // Parse command line arguments
    struct option long_options[] = {
        {"file",              required_argument, 0, 'f'},
        {"width",             required_argument, 0, 'w'},
        {"rows",              required_argument, 0, 'r'},
        {"speed",             required_argument, 0, 'p'},
        {"disable-scrollbar", no_argument,       0, 'S'},
        {"help",              no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    while ((opt = getopt_long(argc, argv, "f:w:r:p:Sh", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'f':
                filename = optarg;
                break;
            case 'w':
                screen_width = std::atoi(optarg);
                if (screen_width < 2 || screen_width > 20) {
                    std::cerr << "Error: Width must be between 2 and 20, resetting to 16" << std::endl;
                    screen_width = 16;
                }
                break;
            case 'r':
                screen_rows = std::atoi(optarg);
                if (screen_rows < 1 || screen_rows > 4) {
                    std::cerr << "Error: Rows must be between 1 and 4, resetting to 2" << std::endl;
                    screen_rows = 2;
                }
                break;
            case 'p':
                scroll_speed = std::atoi(optarg);
                if (scroll_speed < 1) {
                    std::cerr << "Error: Speed must be a positive integer" << std::endl;
                    return 1;
                }
                break;
            case 'S':
                show_scrollbar = false;
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            case '?':
                printUsage(argv[0]);
                return 1;
            default:
                break;
        }
    }
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  File: " << filename << std::endl;
    std::cout << "  Screen width: " << screen_width << " characters" << std::endl;
    std::cout << "  Screen rows: " << screen_rows << " rows" << std::endl;
    std::cout << "  Scroll speed: " << scroll_speed << " seconds" << std::endl;
    std::cout << "  Scrollbar: " << (show_scrollbar ? "on" : "off (full-width text)") << std::endl;
    std::cout << std::endl;
    
    // Set up signal handler for clean exit
    signal(SIGINT, signalHandler);
    
    // Initialize display
    LCDDisplay display;
    if (!display.initialize()) {
        std::cerr << "Failed to initialize LCD display" << std::endl;
        return 1;
    }
    
    std::cout << "LCD initialized successfully" << std::endl;
    
    // Create story teller
    TextStoryTeller storyteller(&display, filename, screen_width, screen_rows, scroll_speed,
                                show_scrollbar);
    
    // Load file
    if (!storyteller.loadFile()) {
        return 1;
    }
    
    std::cout << "Starting story display (Press Ctrl+C to stop)..." << std::endl;
    
    // Display story
    storyteller.displayStory();
    
    // Clean up
    display.clear();
    display.print("Goodbye!");
    
    std::cout << "Story teller finished." << std::endl;
    return 0;
}