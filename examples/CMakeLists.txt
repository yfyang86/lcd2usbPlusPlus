# LCD2USB++ Examples
# This CMakeLists.txt builds the example applications

# Function to create an example executable
function(add_lcd_example name source_file)
    add_executable(${name} ${source_file})
    
    # Link to our library
    target_link_libraries(${name} PRIVATE LCD2USB::lcd2usb)
    
    # Link to libusb
    if(APPLE)
        target_link_libraries(${name} PRIVATE ${LIBUSB_LIBRARY})
    else()
        target_link_libraries(${name} PRIVATE ${LIBUSB_LIBRARIES})
    endif()
    
    # Set output directory
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )
    
    # Install the example
    install(TARGETS ${name}
        RUNTIME DESTINATION bin/examples
    )
endfunction()

# Basic LCD test example
add_lcd_example(basic_test basic_test.cpp)

# Custom character demo
add_lcd_example(custom_char_demo custom_char_demo.cpp)

# Animated character demo
add_lcd_example(animated_demo animated_demo.cpp)

# Font editor demo (simple version with correct API)
add_lcd_example(font_editor_demo font_editor_demo_simple.cpp)

# Story teller demo - text file reader with scrolling and progress bars
add_lcd_example(storyteller storyteller.cpp)

# BMP to LCD converter - converts BMP images to LCD custom characters
add_lcd_example(bmp_to_lcd_demo bmp_to_lcd_demo.cpp)

# Test 4-row LCD support
add_lcd_example(test_4rows test_4rows.cpp)

# GIF to LCD converter - converts animated GIFs to LCD custom characters
# Requires giflib: brew install giflib
if(APPLE)
    # On macOS, giflib is typically installed via Homebrew
    find_path(GIFLIB_INCLUDE_DIR gif_lib.h
        PATHS /opt/homebrew/include /usr/local/include
    )
    find_library(GIFLIB_LIBRARY gif
        PATHS /opt/homebrew/lib /usr/local/lib
    )
else()
    # On Linux, use pkg-config
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(GIFLIB giflib)
    endif()
endif()

if(GIFLIB_INCLUDE_DIR AND GIFLIB_LIBRARY)
    # Find ncurses for status display
    find_package(Curses REQUIRED)
    
    add_executable(gif_to_lcd_demo gif_to_lcd_demo.cpp)
    target_link_libraries(gif_to_lcd_demo PRIVATE LCD2USB::lcd2usb)
    target_link_libraries(gif_to_lcd_demo PRIVATE ${GIFLIB_LIBRARY})
    target_link_libraries(gif_to_lcd_demo PRIVATE ${CURSES_LIBRARIES})
    target_include_directories(gif_to_lcd_demo PRIVATE ${GIFLIB_INCLUDE_DIR})
    target_include_directories(gif_to_lcd_demo PRIVATE ${CURSES_INCLUDE_DIRS})
    if(APPLE)
        target_link_libraries(gif_to_lcd_demo PRIVATE ${LIBUSB_LIBRARY})
    else()
        target_link_libraries(gif_to_lcd_demo PRIVATE ${LIBUSB_LIBRARIES})
    endif()
    set_target_properties(gif_to_lcd_demo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )
    install(TARGETS gif_to_lcd_demo
        RUNTIME DESTINATION bin/examples
    )
    message(STATUS "Building gif_to_lcd_demo with giflib at ${GIFLIB_LIBRARY}")
elseif(GIFLIB_FOUND)
    # Linux pkg-config path
    find_package(Curses REQUIRED)
    
    add_executable(gif_to_lcd_demo gif_to_lcd_demo.cpp)
    target_link_libraries(gif_to_lcd_demo PRIVATE LCD2USB::lcd2usb)
    target_link_libraries(gif_to_lcd_demo PRIVATE ${GIFLIB_LIBRARIES})
    target_link_libraries(gif_to_lcd_demo PRIVATE ${CURSES_LIBRARIES})
    target_include_directories(gif_to_lcd_demo PRIVATE ${GIFLIB_INCLUDE_DIRS})
    target_include_directories(gif_to_lcd_demo PRIVATE ${CURSES_INCLUDE_DIRS})
    target_link_libraries(gif_to_lcd_demo PRIVATE ${LIBUSB_LIBRARIES})
    set_target_properties(gif_to_lcd_demo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )
    install(TARGETS gif_to_lcd_demo
        RUNTIME DESTINATION bin/examples
    )
    message(STATUS "Building gif_to_lcd_demo with giflib support")
else()
    message(WARNING "giflib not found. gif_to_lcd_demo will not be built. Install with: brew install giflib")
endif()

# Scrolling text demo (temporarily disabled - requires implementation)
# add_lcd_example(scrolling_demo scrolling_demo.cpp)

# Complete feature showcase (temporarily disabled - requires ScrollingText, FontManager)
# add_lcd_example(full_demo full_demo.cpp)

# System monitoring demo (temporarily disabled - requires ScrollingText)
# add_lcd_example(system_monitor system_monitor.cpp)

# Test 4x4 grid display
add_lcd_example(test_4x4_grid test_4x4_grid.cpp)

# Test single custom character
add_lcd_example(test_single_custom_char test_single_custom_char.cpp)

# UTF-8 / Unicode display demo — bandwidth-aware text, diffing, CGRAM LRU
add_lcd_example(utf8_demo utf8_demo.cpp)

# UTF-8 multi-language storyteller — uses a bitmap TTF (e.g. ChillBitmap) for
# CJK glyphs. Requires FreeType (already a lcd2usb library dep).
add_lcd_example(storyteller_utf8 storyteller_utf8.cpp)

# RsTask kanban board viewer — shells out to the `rstasks` CLI
add_lcd_example(lcd_rstasks_kanban lcd-rstasks-kanban.cpp)