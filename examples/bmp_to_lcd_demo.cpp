/*
 * bmp_to_lcd_demo.cpp - BMP file to LCD custom characters converter
 * Reads a BMP file, converts it to configurable LCD character grid,
 * and displays it on the LCD device
 */

#include "lcd2usb/lcd2usb_lib.hpp"
#include "lcd2usb/multi_char_display.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>

using namespace LCD2USB;

// Global cleanup flag
std::atomic<bool> g_cleanup_requested(false);

// Signal handler for graceful shutdown
void signalHandler(int signum) {
    g_cleanup_requested.store(true);
}

// BMP file structures
#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t signature;      // 'BM' signature
    uint32_t fileSize;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t dataOffset;     // Offset to pixel data
};

struct BMPInfoHeader {
    uint32_t headerSize;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitsPerPixel;
    uint32_t compression;
    uint32_t imageSize;
    int32_t xPixelsPerMeter;
    int32_t yPixelsPerMeter;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
};
#pragma pack(pop)

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

// BMP file reader
class BMPReader {
public:
    static GrayscaleImage* loadBMP(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return nullptr;
        }

        // Read file header
        BMPFileHeader fileHeader;
        file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));

        if (fileHeader.signature != 0x4D42) { // 'BM' in little-endian
            std::cerr << "Error: Not a valid BMP file (wrong signature)" << std::endl;
            return nullptr;
        }

        // Read info header
        BMPInfoHeader infoHeader;
        file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

        // Validate BMP format
        if (infoHeader.compression != 0) {
            std::cerr << "Error: Compressed BMP files are not supported" << std::endl;
            return nullptr;
        }

        if (infoHeader.bitsPerPixel != 24 && infoHeader.bitsPerPixel != 32 && infoHeader.bitsPerPixel != 8) {
            std::cerr << "Error: Only 8-bit grayscale, 24-bit, and 32-bit BMP files are supported" << std::endl;
            return nullptr;
        }

        std::cout << "BMP Info: " << infoHeader.width << "x" << std::abs(infoHeader.height) 
                  << " pixels, " << infoHeader.bitsPerPixel << " bits per pixel" << std::endl;

        // Seek to pixel data
        file.seekg(fileHeader.dataOffset, std::ios::beg);

        int width = infoHeader.width;
        int height = std::abs(infoHeader.height);
        bool topDown = infoHeader.height < 0;

        GrayscaleImage* image = new GrayscaleImage(width, height);

        // Calculate row padding (rows are aligned to 4 bytes)
        int bytesPerPixel = infoHeader.bitsPerPixel / 8;
        int rowSize = ((width * bytesPerPixel + 3) / 4) * 4;
        std::vector<uint8_t> rowBuffer(rowSize);

        // Read pixel data
        for (int y = 0; y < height; y++) {
            file.read(reinterpret_cast<char*>(rowBuffer.data()), rowSize);

            int destY = topDown ? y : (height - 1 - y); // BMP is bottom-up by default

            for (int x = 0; x < width; x++) {
                uint8_t gray;
                if (infoHeader.bitsPerPixel == 24 || infoHeader.bitsPerPixel == 32) {
                    // BGR(A) format
                    uint8_t b = rowBuffer[x * bytesPerPixel];
                    uint8_t g = rowBuffer[x * bytesPerPixel + 1];
                    uint8_t r = rowBuffer[x * bytesPerPixel + 2];
                    gray = GrayscaleImage::rgbToGray(r, g, b);
                } else {
                    // Already grayscale
                    gray = rowBuffer[x];
                }
                image->setPixel(x, destY, gray);
            }
        }

        return image;
    }
};

// Image processing utilities
class ImageProcessor {
public:
    // Resize image to target dimensions using bilinear interpolation
    static GrayscaleImage* resize(const GrayscaleImage& src, int targetWidth, int targetHeight) {
        GrayscaleImage* dst = new GrayscaleImage(targetWidth, targetHeight);
        
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

    // Resize to fit within target dimensions, optionally maintaining aspect ratio
    static GrayscaleImage* resizeToFit(const GrayscaleImage& src, int maxWidth, int maxHeight, bool fillToFit = false) {
        int newWidth, newHeight;
        
        if (fillToFit) {
            // Scale to fill - may distort aspect ratio
            newWidth = maxWidth;
            newHeight = maxHeight;
        } else {
            // Maintain aspect ratio
            float srcAspect = static_cast<float>(src.getWidth()) / src.getHeight();
            float targetAspect = static_cast<float>(maxWidth) / maxHeight;

            if (srcAspect > targetAspect) {
                // Width is the limiting factor
                newWidth = maxWidth;
                newHeight = static_cast<int>(maxWidth / srcAspect);
            } else {
                // Height is the limiting factor
                newHeight = maxHeight;
                newWidth = static_cast<int>(maxHeight * srcAspect);
            }
        }

        std::cout << "Resizing from " << src.getWidth() << "x" << src.getHeight()
                  << " to " << newWidth << "x" << newHeight << std::endl;

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
        // Map 0-255 to 0-4
        // 0-50 -> 0 (darkest)
        // 51-102 -> 1
        // 103-153 -> 2
        // 154-204 -> 3
        // 205-255 -> 4 (lightest)
        return (gray * 5) / 256;
    }

    // Convert image to custom characters using threshold
    static std::vector<CustomChar> convertToCustomChars(const GrayscaleImage& image, int charWidth, int charHeight, uint8_t threshold = 128) {
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
                            // Apply threshold: darker pixels (below threshold) are "on"
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
    int width = 2;  // Grid width in characters
    int height = 2; // Grid height in characters
    bool scaleToFill = false; // Scale mode
};

Config parseArguments(int argc, char* argv[]) {
    Config config;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            config.showHelp = true;
        } else if (arg == "--input-bmp" || arg == "-i") {
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
        } else if (arg == "--width") {
            if (i + 1 < argc) {
                config.width = std::atoi(argv[++i]);
                if (config.width < 1) config.width = 1;
            }
        } else if (arg == "--height") {
            if (i + 1 < argc) {
                config.height = std::atoi(argv[++i]);
                if (config.height < 1) config.height = 1;
            }
        } else if (arg == "--scale") {
            config.scaleToFill = true;
        } else if (arg == "--no-display") {
            config.displayOnLCD = false;
        }
    }
    
    return config;
}

void printHelp() {
    std::cout << "BMP to LCD Converter - Convert BMP images to LCD custom characters\n\n";
    std::cout << "Usage: bmp_to_lcd_demo [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --input-bmp, -i FILE    Input BMP file to convert\n";
    std::cout << "  --output, -o FILE       Output .lcf font file (optional)\n";
    std::cout << "  --width VALUE           Grid width in characters (default: 2)\n";
    std::cout << "  --height VALUE          Grid height in characters (default: 2)\n";
    std::cout << "  --scale                 Scale to fill entire grid (may distort aspect ratio)\n";
    std::cout << "  --threshold, -t VALUE   Threshold value 0-255 (default: auto-detect using Otsu's method)\n";
    std::cout << "  --no-display            Don't display on LCD device (only save to file)\n";
    std::cout << "  --help, -h              Show this help message\n\n";
    std::cout << "Grid sizes with ≤8 characters display instantly. Larger grids use progressive reveal.\n";
    std::cout << "Recommended: 2x4 (8 chars), 2x3 (6 chars), or 2x2 (4 chars) for best results.\n\n";
    std::cout << "Examples:\n";
    std::cout << "  bmp_to_lcd_demo --input-bmp image.bmp\n";
    std::cout << "  bmp_to_lcd_demo -i image.bmp --width 2 --height 4\n";
    std::cout << "  bmp_to_lcd_demo -i image.bmp --width 3 --height 3 --scale\n";
    std::cout << "  bmp_to_lcd_demo -i image.bmp -o custom_image.lcf -t 128\n";
}

int main(int argc, char* argv[]) {
    std::cout << "=== BMP to LCD Custom Character Converter ===" << std::endl;
    
    Config config = parseArguments(argc, argv);
    
    if (config.showHelp) {
        printHelp();
        return 0;
    }
    
    if (config.inputFile.empty()) {
        std::cerr << "Error: No input file specified. Use --input-bmp or -i to specify a BMP file.\n";
        std::cerr << "Use --help for more information.\n";
        return 1;
    }
    
    // Load BMP file
    std::cout << "\nLoading BMP file: " << config.inputFile << std::endl;
    GrayscaleImage* originalImage = BMPReader::loadBMP(config.inputFile);
    if (!originalImage) {
        return 1;
    }
    
    // Calculate target dimensions based on character grid size
    const int TARGET_WIDTH = config.width * 5;   // width chars * 5 pixels
    const int TARGET_HEIGHT = config.height * 8;  // height chars * 8 pixels
    
    std::cout << "\nCharacter grid: " << config.width << "x" << config.height 
              << " (" << TARGET_WIDTH << "x" << TARGET_HEIGHT << " pixels)" << std::endl;
    
    // Warn about CGRAM limitations
    int totalChars = config.width * config.height;
    if (totalChars > 7) {
        std::cout << "\n⚠️  Note: Grid uses " << totalChars << " characters (>7 usable CGRAM slots)" << std::endl;
        std::cout << "Display will use progressive reveal strategy for static images." << std::endl;
        std::cout << "(Character code 0 is skipped for compatibility)\n" << std::endl;
    }
    
    std::cout << "Resizing image to fit " << TARGET_WIDTH << "x" << TARGET_HEIGHT << " pixels..." << std::endl;
    if (config.scaleToFill) {
        std::cout << "Scale mode: Fill entire grid (may distort)" << std::endl;
    } else {
        std::cout << "Scale mode: Maintain aspect ratio" << std::endl;
    }
    
    GrayscaleImage* resizedImage = ImageProcessor::resizeToFit(*originalImage, TARGET_WIDTH, TARGET_HEIGHT, config.scaleToFill);
    delete originalImage;
    
    // Apply threshold if specified, otherwise auto-detect
    if (config.threshold == -1) {
        config.threshold = ImageProcessor::calculateOtsuThreshold(*resizedImage);
        std::cout << "Auto-detected threshold: " << config.threshold << " (using Otsu's method)" << std::endl;
    } else {
        std::cout << "Using specified threshold: " << config.threshold << std::endl;
    }
    
    // Convert to custom characters
    std::cout << "\nConverting to LCD custom characters (" << config.width << "x" << config.height << " grid)..." << std::endl;
    std::vector<CustomChar> customChars = ImageProcessor::convertToCustomChars(*resizedImage, config.width, config.height, config.threshold);
    delete resizedImage;
    
    std::cout << "Generated " << customChars.size() << " custom characters" << std::endl;
    
    // Save to font file if output specified
    if (!config.outputFile.empty()) {
        std::cout << "\nSaving to font file: " << config.outputFile << std::endl;
        
        FontEdit::FontManager fontManager("BMP Converter");
        
        // Add each character to the font (limited by font format to 256 characters)
        size_t maxChars = std::min(customChars.size(), size_t(256));
        for (size_t i = 0; i < maxChars; i++) {
            FontEdit::CharacterBitmap charBitmap("BMP_Char_" + std::to_string(i));
            charBitmap.setDescription("Converted from " + config.inputFile);
            charBitmap.setFrame(0, customChars[i].toBytes());
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
            std::cerr << "Image was processed but not displayed." << std::endl;
            return 1;
        }
        
        std::cout << "✓ LCD initialized" << std::endl;
        
        // Configure display
        display.setContrast(200);
        display.setBrightness(255);
        display.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Display the image using MultiCharDisplay
        // This uploads patterns to CGRAM and displays them
        std::cout << "Displaying image on LCD..." << std::endl;
        MultiCharDisplay multiDisplay(display);
        multiDisplay.displayGrid(customChars, config.width, config.height);
        
        std::cout << "✓ Image displayed on LCD!" << std::endl;
        std::cout << "\nPress Ctrl+C to exit..." << std::endl;
        
        // Keep the display on, check for exit signal
        while (!g_cleanup_requested.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Cleanup sequence
        std::cout << "\nCleaning up..." << std::endl;
        display.clear();
        display.setCursor(0, 0);
        display.print("Image viewer");
        display.setCursor(0, 1);
        display.print("stopped.");
        std::cout << "✓ Display cleaned" << std::endl;
    }
    
    std::cout << "\n=== Conversion Complete ===" << std::endl;
    return 0;
}
