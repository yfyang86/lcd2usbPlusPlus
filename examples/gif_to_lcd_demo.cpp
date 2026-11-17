/*
 * gif_to_lcd_demo.cpp - Animated GIF to LCD custom characters converter
 * Reads an animated GIF file, converts each frame to 2x2 LCD character grid,
 * and displays the animation on the LCD device
 */

#include "lcd2usb/lcd2usb_lib.hpp"
#include "lcd2usb/multi_char_display.hpp"
#include <gif_lib.h>
#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>
#include <memory>
#include <iomanip>
#include <sstream>
#include <csignal>
#include <atomic>

using namespace LCD2USB;

// Global cleanup flag
std::atomic<bool> g_cleanup_requested(false);

// Signal handler for graceful shutdown
void signalHandler(int signum) {
    g_cleanup_requested.store(true);
}

// Simple image class for grayscale images
class GrayscaleImage {
private:
    std::vector<uint8_t> pixels_;
    int width_;
    int height_;

public:
    GrayscaleImage(int width, int height) : width_(width), height_(height) {
        pixels_.resize(width * height, 0);
    }

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

    void setPixel(int x, int y, uint8_t value) {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) {
            pixels_[y * width_ + x] = value;
        }
    }

    uint8_t getPixel(int x, int y) const {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) {
            return pixels_[y * width_ + x];
        }
        return 0;
    }

    // Convert RGB to grayscale using standard luminance formula
    static uint8_t rgbToGray(uint8_t r, uint8_t g, uint8_t b) {
        return static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
    }
};

// GIF frame with timing information
struct GIFFrame {
    std::unique_ptr<GrayscaleImage> image;
    int delayMs;  // Delay in milliseconds
};

// GIF file reader using giflib
class GIFReader {
public:
    static std::vector<GIFFrame> loadGIF(const std::string& filename) {
        std::vector<GIFFrame> frames;
        int errorCode;
        
        // Open GIF file
        GifFileType* gif = DGifOpenFileName(filename.c_str(), &errorCode);
        if (!gif) {
            std::cerr << "Error: Cannot open GIF file " << filename << std::endl;
            std::cerr << "GIF Error: " << GifErrorString(errorCode) << std::endl;
            return frames;
        }

        // Read GIF data
        if (DGifSlurp(gif) == GIF_ERROR) {
            std::cerr << "Error: Failed to read GIF data" << std::endl;
            std::cerr << "GIF Error: " << GifErrorString(gif->Error) << std::endl;
            DGifCloseFile(gif, &errorCode);
            return frames;
        }

        std::cout << "GIF Info: " << gif->SWidth << "x" << gif->SHeight 
                  << " pixels, " << gif->ImageCount << " frames" << std::endl;

        // Get global color map
        ColorMapObject* globalColorMap = gif->SColorMap;

        // Process each frame
        for (int frameIdx = 0; frameIdx < gif->ImageCount; frameIdx++) {
            SavedImage* frame = &gif->SavedImages[frameIdx];
            
            // Create image for this frame
            auto image = std::make_unique<GrayscaleImage>(gif->SWidth, gif->SHeight);
            
            // Get the color map (prefer local, fall back to global)
            ColorMapObject* colorMap = frame->ImageDesc.ColorMap 
                                        ? frame->ImageDesc.ColorMap 
                                        : globalColorMap;
            
            if (!colorMap) {
                std::cerr << "Warning: No color map for frame " << frameIdx << std::endl;
                continue;
            }

            // Get transparent color index if specified
            int transparentIndex = -1;
            int delayMs = 100; // Default 100ms delay
            
            // Check for Graphics Control Extension
            for (int i = 0; i < frame->ExtensionBlockCount; i++) {
                ExtensionBlock* ext = &frame->ExtensionBlocks[i];
                if (ext->Function == GRAPHICS_EXT_FUNC_CODE && ext->ByteCount >= 4) {
                    uint8_t flags = ext->Bytes[0];
                    delayMs = (ext->Bytes[2] | (ext->Bytes[3] << 8)) * 10; // Centiseconds to milliseconds
                    if (flags & 0x01) { // Transparent color flag
                        transparentIndex = ext->Bytes[3];
                    }
                }
            }

            // Decode the frame
            GifImageDesc* imageDesc = &frame->ImageDesc;
            GifByteType* raster = frame->RasterBits;
            
            int left = imageDesc->Left;
            int top = imageDesc->Top;
            int width = imageDesc->Width;
            int height = imageDesc->Height;

            // Copy pixels from frame to image
            for (int y = 0; y < height && (top + y) < gif->SHeight; y++) {
                for (int x = 0; x < width && (left + x) < gif->SWidth; x++) {
                    int rasterIndex = y * width + x;
                    int colorIndex = raster[rasterIndex];
                    
                    // Skip transparent pixels
                    if (colorIndex == transparentIndex) {
                        continue;
                    }
                    
                    if (colorIndex < colorMap->ColorCount) {
                        GifColorType* color = &colorMap->Colors[colorIndex];
                        uint8_t gray = GrayscaleImage::rgbToGray(color->Red, color->Green, color->Blue);
                        image->setPixel(left + x, top + y, gray);
                    }
                }
            }

            // Store the frame
            GIFFrame gifFrame;
            gifFrame.image = std::move(image);
            gifFrame.delayMs = std::max(delayMs, 10); // Minimum 10ms delay
            frames.push_back(std::move(gifFrame));
        }

        // Close the GIF file
        DGifCloseFile(gif, &errorCode);
        
        std::cout << "Successfully loaded " << frames.size() << " frames" << std::endl;
        return frames;
    }
};

// Image processing utilities (same as BMP version)
class ImageProcessor {
public:
    // Resize image to target dimensions using bilinear interpolation
    static std::unique_ptr<GrayscaleImage> resize(const GrayscaleImage& src, int targetWidth, int targetHeight) {
        auto dst = std::make_unique<GrayscaleImage>(targetWidth, targetHeight);
        
        float xRatio = static_cast<float>(src.getWidth() - 1) / targetWidth;
        float yRatio = static_cast<float>(src.getHeight() - 1) / targetHeight;

        for (int y = 0; y < targetHeight; y++) {
            for (int x = 0; x < targetWidth; x++) {
                float srcX = x * xRatio;
                float srcY = y * yRatio;

                int x1 = static_cast<int>(srcX);
                int y1 = static_cast<int>(srcY);
                int x2 = std::min(x1 + 1, src.getWidth() - 1);
                int y2 = std::min(y1 + 1, src.getHeight() - 1);

                float xWeight = srcX - x1;
                float yWeight = srcY - y1;

                // Bilinear interpolation
                float p11 = src.getPixel(x1, y1);
                float p12 = src.getPixel(x1, y2);
                float p21 = src.getPixel(x2, y1);
                float p22 = src.getPixel(x2, y2);

                float value = p11 * (1 - xWeight) * (1 - yWeight) +
                            p21 * xWeight * (1 - yWeight) +
                            p12 * (1 - xWeight) * yWeight +
                            p22 * xWeight * yWeight;

                dst->setPixel(x, y, static_cast<uint8_t>(value));
            }
        }

        return dst;
    }

    // Resize to fit within target dimensions while maintaining aspect ratio
    static std::unique_ptr<GrayscaleImage> resizeToFit(const GrayscaleImage& src, int maxWidth, int maxHeight, bool scaleToFill = false) {
        if (scaleToFill) {
            // Scale to fill the entire area (may crop or distort)
            return resize(src, maxWidth, maxHeight);
        }
        
        // Maintain aspect ratio (may leave empty space)
        float srcAspect = static_cast<float>(src.getWidth()) / src.getHeight();
        float targetAspect = static_cast<float>(maxWidth) / maxHeight;

        int newWidth, newHeight;
        if (srcAspect > targetAspect) {
            // Width is the limiting factor
            newWidth = maxWidth;
            newHeight = static_cast<int>(maxWidth / srcAspect);
        } else {
            // Height is the limiting factor
            newHeight = maxHeight;
            newWidth = static_cast<int>(maxHeight * srcAspect);
        }

        return resize(src, newWidth, newHeight);
    }

    // Calculate automatic threshold using Otsu's method
    static uint8_t calculateOtsuThreshold(const GrayscaleImage& image) {
        // Build histogram
        std::vector<int> histogram(256, 0);
        int totalPixels = image.getWidth() * image.getHeight();
        
        for (int y = 0; y < image.getHeight(); y++) {
            for (int x = 0; x < image.getWidth(); x++) {
                histogram[image.getPixel(x, y)]++;
            }
        }

        // Calculate Otsu threshold
        float sum = 0;
        for (int i = 0; i < 256; i++) {
            sum += i * histogram[i];
        }

        float sumB = 0;
        int wB = 0;
        int wF = 0;
        float maxVariance = 0;
        uint8_t threshold = 0;

        for (int t = 0; t < 256; t++) {
            wB += histogram[t];
            if (wB == 0) continue;

            wF = totalPixels - wB;
            if (wF == 0) break;

            sumB += t * histogram[t];

            float mB = sumB / wB;
            float mF = (sum - sumB) / wF;

            float variance = wB * wF * (mB - mF) * (mB - mF);

            if (variance > maxVariance) {
                maxVariance = variance;
                threshold = t;
            }
        }

        return threshold;
    }

    // Quantize to 5 levels (0-4) based on grayscale value
    static uint8_t quantizeTo5Levels(uint8_t gray) {
        return (gray * 5) / 256;
    }

    // Convert 5-level quantized image to custom characters
    static std::vector<CustomChar> convertToCustomChars(const GrayscaleImage& image, int charWidth, int charHeight, uint8_t threshold) {
        std::vector<CustomChar> chars;
        
        // LCD custom character is 5 pixels wide, 8 pixels tall
        const int LCD_CHAR_WIDTH = 5;
        const int LCD_CHAR_HEIGHT = 8;

        for (int charY = 0; charY < charHeight; charY++) {
            for (int charX = 0; charX < charWidth; charX++) {
                CustomChar customChar;
                
                for (int py = 0; py < LCD_CHAR_HEIGHT; py++) {
                    for (int px = 0; px < LCD_CHAR_WIDTH; px++) {
                        int imgX = charX * LCD_CHAR_WIDTH + px;
                        int imgY = charY * LCD_CHAR_HEIGHT + py;
                        
                        bool pixelOn = false;
                        if (imgX < image.getWidth() && imgY < image.getHeight()) {
                            uint8_t gray = image.getPixel(imgX, imgY);
                            // Use threshold for binary conversion
                            pixelOn = (gray < threshold);
                        }
                        
                        customChar.setPixel(px, py, pixelOn);
                    }
                }
                
                chars.push_back(customChar);
            }
        }
        
        return chars;
    }
};

// Command-line argument parser
struct Config {
    std::string inputFile;
    std::string outputFile;
    int threshold = -1; // -1 means auto-detect
    bool showHelp = false;
    bool displayOnLCD = true;
    bool loop = true;
    int maxFrames = -1; // -1 means all frames
    float fps = 0.0f; // 0 means use original GIF timing
    bool scaleToFill = false; // Scale to fill entire grid
    int width = 2; // Character grid width (default 2)
    int height = 2; // Character grid height (default 2)
};

Config parseArguments(int argc, char* argv[]) {
    Config config;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            config.showHelp = true;
        } else if (arg == "--input-gif" || arg == "-i") {
            if (i + 1 < argc) {
                config.inputFile = argv[++i];
            }
        } else if (arg == "--output" || arg == "-o") {
            if (i + 1 < argc) {
                config.outputFile = argv[++i];
            }
        } else if (arg == "--threshold" || arg == "-t") {
            if (i + 1 < argc) {
                config.threshold = std::atoi(argv[++i]);
            }
        } else if (arg == "--no-display") {
            config.displayOnLCD = false;
        } else if (arg == "--no-loop") {
            config.loop = false;
        } else if (arg == "--max-frames" || arg == "-m") {
            if (i + 1 < argc) {
                config.maxFrames = std::atoi(argv[++i]);
            }
        } else if (arg == "--fps") {
            if (i + 1 < argc) {
                config.fps = std::atof(argv[++i]);
            }
        } else if (arg == "--scale") {
            config.scaleToFill = true;
        } else if (arg == "--width") {
            if (i + 1 < argc) {
                config.width = std::atoi(argv[++i]);
                if (config.width <= 0) {
                    config.width = 2; // Reset to default if not positive
                }
            }
        } else if (arg == "--height") {
            if (i + 1 < argc) {
                config.height = std::atoi(argv[++i]);
                if (config.height <= 0) {
                    config.height = 2; // Reset to default if not positive
                }
            }
        }
    }
    
    return config;
}

void printHelp() {
    std::cout << "Animated GIF to LCD Converter - Convert animated GIFs to LCD custom characters\n\n";
    std::cout << "Usage: gif_to_lcd_demo [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --input-gif, -i FILE    Input GIF file to convert\n";
    std::cout << "  --output, -o FILE       Output .lcf font file for first frame (optional)\n";
    std::cout << "  --threshold, -t VALUE   Threshold value 0-255 (default: auto-detect using Otsu's method)\n";
    std::cout << "  --max-frames, -m NUM    Maximum number of frames to process (default: all)\n";
    std::cout << "  --fps FPS               Target frames per second (default: use GIF timing)\n";
    std::cout << "  --width WIDTH           Character grid width (default: 2, min: 1)\n";
    std::cout << "  --height HEIGHT         Character grid height (default: 2, min: 1)\n";
    std::cout << "  --scale                 Scale image to fill entire grid (may distort aspect ratio)\n";
    std::cout << "  --no-loop               Play animation only once\n";
    std::cout << "  --no-display            Don't display on LCD device (only save first frame to file)\n";
    std::cout << "  --help, -h              Show this help message\n\n";
    std::cout << "The LCD displays images using a grid of custom characters.\n";
    std::cout << "Each character is 5x8 pixels. Default is 2x2 grid (10x16 pixels).\n";
    std::cout << "Use --width and --height to create larger displays (e.g., 4x2 = 20x16 pixels).\n";
    std::cout << "By default, frames maintain aspect ratio. Use --scale to fill the entire grid.\n\n";
    std::cout << "Examples:\n";
    std::cout << "  gif_to_lcd_demo --input-gif animation.gif\n";
    std::cout << "  gif_to_lcd_demo -i animation.gif --fps 30 --width 4 --height 2\n";
    std::cout << "  gif_to_lcd_demo -i animation.gif -t 128 --no-loop --scale\n";
    std::cout << "  gif_to_lcd_demo -i animation.gif --width 3 --height 3 --fps 15\n";
}

int main(int argc, char* argv[]) {
    std::cout << "=== Animated GIF to LCD Custom Character Converter ===" << std::endl;
    
    Config config = parseArguments(argc, argv);
    
    if (config.showHelp) {
        printHelp();
        return 0;
    }
    
    if (config.inputFile.empty()) {
        std::cerr << "Error: No input file specified. Use --input-gif or -i to specify a GIF file.\n";
        std::cerr << "Use --help for more information.\n";
        return 1;
    }
    
    // Load GIF file
    std::cout << "\nLoading GIF file: " << config.inputFile << std::endl;
    std::vector<GIFFrame> frames = GIFReader::loadGIF(config.inputFile);
    
    if (frames.empty()) {
        std::cerr << "Error: No frames loaded from GIF file" << std::endl;
        return 1;
    }
    
    // Limit frames if requested
    if (config.maxFrames > 0 && frames.size() > static_cast<size_t>(config.maxFrames)) {
        std::cout << "Limiting to first " << config.maxFrames << " frames" << std::endl;
        frames.resize(config.maxFrames);
    }
    
    // Calculate target dimensions based on character grid size
    const int TARGET_WIDTH = config.width * 5;   // width chars * 5 pixels
    const int TARGET_HEIGHT = config.height * 8;  // height chars * 8 pixels
    
    // Process all frames
    std::cout << "\nProcessing " << frames.size() << " frames..." << std::endl;
    std::cout << "Character grid: " << config.width << "x" << config.height 
              << " (" << TARGET_WIDTH << "x" << TARGET_HEIGHT << " pixels)" << std::endl;
    
    // Warn about CGRAM limitations
    int totalChars = config.width * config.height;
    if (totalChars > 7) {
        std::cout << "\n⚠️  WARNING: HD44780 LCD Hardware Limitation ⚠️" << std::endl;
        std::cout << "Your grid uses " << totalChars << " characters per frame, but the LCD only has 7 usable CGRAM slots." << std::endl;
        std::cout << "(Character code 0 is skipped for compatibility)" << std::endl;
        std::cout << "This will cause severe flickering and incorrect display during animation." << std::endl;
        std::cout << "\nRecommended grids (7 or fewer characters):" << std::endl;
        std::cout << "  - 2x3 (6 chars) - Best balance for smooth animations" << std::endl;
        std::cout << "  - 2x2 (4 chars) - Very fast, less detail" << std::endl;
        std::cout << "  - 1x7 (7 chars) - Maximum resolution in single row" << std::endl;
        std::cout << "\nContinuing anyway, but expect visual artifacts...\n" << std::endl;
    }
    
    if (config.scaleToFill) {
        std::cout << "Scale mode: Fill entire grid (may distort)" << std::endl;
    } else {
        std::cout << "Scale mode: Maintain aspect ratio" << std::endl;
    }
    
    std::vector<std::vector<CustomChar>> processedFrames;
    uint8_t threshold = 0;
    
    for (size_t i = 0; i < frames.size(); i++) {
        if (i % 10 == 0) {
            std::cout << "Processing frame " << (i + 1) << "/" << frames.size() << "..." << std::endl;
        }
        
        // Resize frame
        auto resizedImage = ImageProcessor::resizeToFit(*frames[i].image, TARGET_WIDTH, TARGET_HEIGHT, config.scaleToFill);
        
        // Calculate threshold from first frame or use specified value
        if (i == 0) {
            if (config.threshold == -1) {
                threshold = ImageProcessor::calculateOtsuThreshold(*resizedImage);
                std::cout << "Auto-detected threshold: " << static_cast<int>(threshold) << " (using Otsu's method)" << std::endl;
            } else {
                threshold = static_cast<uint8_t>(config.threshold);
                std::cout << "Using specified threshold: " << static_cast<int>(threshold) << std::endl;
            }
        }
        
        // Convert to custom characters
        std::vector<CustomChar> customChars = ImageProcessor::convertToCustomChars(*resizedImage, config.width, config.height, threshold);
        processedFrames.push_back(customChars);
    }
    
    std::cout << "✓ All frames processed (" << config.width * config.height << " characters per frame)" << std::endl;
    
    // Save first frame to font file if output specified
    if (!config.outputFile.empty() && !processedFrames.empty()) {
        std::cout << "\nSaving first frame to font file: " << config.outputFile << std::endl;
        
        FontEdit::FontManager fontManager("GIF Converter");
        
        // Save all characters from first frame (limited by font format to 256 characters)
        size_t maxChars = std::min(processedFrames[0].size(), size_t(256));
        for (size_t i = 0; i < maxChars; i++) {
            FontEdit::CharacterBitmap charBitmap("GIF_Frame0_Char_" + std::to_string(i));
            charBitmap.setDescription("Converted from " + config.inputFile);
            charBitmap.setFrame(0, processedFrames[0][i].toBytes());
            fontManager.addCharacter(static_cast<uint8_t>(i), charBitmap);
        }
        
        if (fontManager.saveToFile(config.outputFile)) {
            std::cout << "✓ Font file saved successfully" << std::endl;
        } else {
            std::cerr << "✗ Failed to save font file" << std::endl;
        }
    }
    
    // Display on LCD if requested
    if (config.displayOnLCD) {
        // Register signal handlers for graceful cleanup
        std::signal(SIGINT, signalHandler);   // Ctrl+C
        std::signal(SIGTERM, signalHandler);  // kill command
        
        std::cout << "\nInitializing LCD display..." << std::endl;
        LCDDisplay display;
        
        if (!display.initialize()) {
            std::cerr << "Warning: Failed to initialize LCD display" << std::endl;
            std::cerr << "Animation was processed but not displayed." << std::endl;
            return 1;
        }
        
        std::cout << "✓ LCD initialized" << std::endl;
        
        // Configure display
        display.setContrast(200);
        display.setBrightness(255);
        display.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Calculate frame delay
        int frameDelayMs;
        if (config.fps > 0) {
            frameDelayMs = static_cast<int>(1000.0f / config.fps);
            std::cout << "Using fixed FPS: " << config.fps << " (" << frameDelayMs << "ms per frame)" << std::endl;
        } else {
            std::cout << "Using original GIF frame timing" << std::endl;
        }
        
        // Initialize ncurses for status display
        initscr();
        cbreak();
        noecho();
        curs_set(0);
        nodelay(stdscr, TRUE);
        
        // Display animation info
        clear();
        mvprintw(0, 0, "=== Animated GIF Player ===");
        mvprintw(1, 0, "File: %s", config.inputFile.c_str());
        mvprintw(2, 0, "Total frames: %zu", frames.size());
        mvprintw(3, 0, "Grid size: %dx%d characters (%dx%d pixels)", 
                 config.width, config.height, config.width * 5, config.height * 8);
        mvprintw(4, 0, "Loop: %s", config.loop ? "Yes" : "No");
        if (config.fps > 0) {
            mvprintw(5, 0, "FPS: %.1f", config.fps);
        } else {
            mvprintw(5, 0, "FPS: Variable (from GIF)");
        }
        mvprintw(7, 0, "Press 'q' to quit");
        mvprintw(8, 0, "----------------------------------------");
        refresh();
        
        // Animation loop
        size_t totalFrameCount = 0;
        int loopCount = 0;
        auto animationStart = std::chrono::steady_clock::now();
        bool quit = false;
        
        do {
            loopCount++;
            
            for (size_t frameIdx = 0; frameIdx < frames.size() && !quit; frameIdx++) {
                // Check for quit signals
                if (g_cleanup_requested.load()) {
                    quit = true;
                    break;
                }
                
                int ch = getch();
                if (ch == 'q' || ch == 'Q') {
                    quit = true;
                    break;
                }
                
                // Clear display to avoid artifacts from previous frame
                display.clear();
                
                // Use MultiCharDisplay helper to handle complex multi-row grids
                // This avoids CGRAM slot conflicts by processing row-by-row
                MultiCharDisplay multiDisplay(display);
                multiDisplay.displayGrid(processedFrames[frameIdx], config.width, config.height);
                
                // Update status display
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - animationStart).count();
                float actualFps = totalFrameCount > 0 ? (totalFrameCount * 1000.0f / elapsed) : 0.0f;
                
                mvprintw(10, 0, "Loop: %d", loopCount);
                mvprintw(11, 0, "Current frame: %5zu / %zu", frameIdx + 1, frames.size());
                mvprintw(12, 0, "Total displayed: %zu", totalFrameCount);
                mvprintw(13, 0, "Elapsed: %.2fs", elapsed / 1000.0);
                mvprintw(14, 0, "Actual FPS: %.2f", actualFps);
                
                // Progress bar
                int barWidth = 50;
                int progress = static_cast<int>((frameIdx + 1) * barWidth / frames.size());
                mvprintw(16, 0, "Progress: [");
                for (int i = 0; i < barWidth; i++) {
                    if (i < progress) {
                        addch('=');
                    } else if (i == progress) {
                        addch('>');
                    } else {
                        addch(' ');
                    }
                }
                printw("] %3d%%", (int)((frameIdx + 1) * 100 / frames.size()));
                
                refresh();
                
                // Wait for frame delay
                int delay = config.fps > 0 ? frameDelayMs : frames[frameIdx].delayMs;
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                
                totalFrameCount++;
            }
        } while (config.loop && !quit);
        
        // Cleanup sequence
        std::cout << "\nCleaning up..." << std::endl;
        
        // Clean up ncurses first (so we can print to stdout)
        endwin();
        
        // Clear LCD display
        display.clear();
        display.setCursor(0, 0);
        display.print("Animation");
        display.setCursor(0, 1);
        display.print("stopped.");
        
        std::cout << "✓ Display cleaned" << std::endl;
        std::cout << "✓ Animation complete!" << std::endl;
        std::cout << "Total frames displayed: " << totalFrameCount << std::endl;
        std::cout << "Loops completed: " << loopCount << std::endl;
    }
    
    std::cout << "\n=== Conversion Complete ===" << std::endl;
    return 0;
}
