cmake_minimum_required(VERSION 3.16)
project(LCD2USB++ VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find libusb-compat
if(APPLE)
    # For macOS with Homebrew
    set(LIBUSB_ROOT "/opt/homebrew/Cellar/libusb-compat/0.1.8")
    set(LIBUSB_INCLUDE_DIR "${LIBUSB_ROOT}/include")
    set(LIBUSB_LIBRARY "${LIBUSB_ROOT}/lib/libusb.dylib")
else()
    # For Linux systems
    pkg_check_modules(LIBUSB REQUIRED libusb)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
if(APPLE)
    include_directories(${LIBUSB_INCLUDE_DIR})
else()
    include_directories(${LIBUSB_INCLUDE_DIRS})
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")

# Add subdirectories
add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(tools)

# Install configuration
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

install(TARGETS lcd2usb
    EXPORT lcd2usbTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Export targets for find_package
install(EXPORT lcd2usbTargets
    FILE lcd2usbTargets.cmake
    DESTINATION lib/cmake/lcd2usb
)

# Create config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "lcd2usbConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lcd2usbConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/lcd2usbConfig.cmake"
    INSTALL_DESTINATION lib/cmake/lcd2usb
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/lcd2usbConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/lcd2usbConfigVersion.cmake"
    DESTINATION lib/cmake/lcd2usb
)