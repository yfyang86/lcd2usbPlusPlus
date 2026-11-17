# LCD2USB++ Library Source Files
# This CMakeLists.txt defines the library sources

# Library source files
set(LCD2USB_LIB_SOURCES
    lcd_display.cpp
    custom_char.cpp
    font_editor.cpp
    multi_char_display.cpp
    utf8.cpp
    glyph_table.cpp
    unicode_display.cpp
    glyph_source.cpp
    glyph_pack.cpp
    bitmap_font.cpp
    # scrolling_text.cpp  # Interface mismatch - needs redesign
)

# Find FreeType — used by bitmap_font.cpp for TTF rasterization.
# Both Linux (libfreetype6-dev) and macOS (Homebrew `freetype`) ship
# standard CMake find-module support.
find_package(Freetype REQUIRED)

# Add library target
add_library(lcd2usb ${LCD2USB_LIB_SOURCES})

# Add alias for better namespacing
add_library(LCD2USB::lcd2usb ALIAS lcd2usb)

# Set target properties
set_target_properties(lcd2usb PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    EXPORT_NAME lcd2usb
    OUTPUT_NAME lcd2usb
)

# Include directories for the library
target_include_directories(lcd2usb PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link libraries
target_link_libraries(lcd2usb PRIVATE ${LIBUSB_LIBRARIES})
target_link_libraries(lcd2usb PUBLIC Freetype::Freetype)

# Compiler features
target_compile_features(lcd2usb PUBLIC cxx_std_17)

# Install the library
install(TARGETS lcd2usb
    EXPORT LCD2USB-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)