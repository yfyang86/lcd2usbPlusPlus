# LCD2USB++ tools — small command-line utilities that don't talk to the LCD
# but help during development.
#
# NOTE on linking: the tools never call into LCDDisplay (they only use the
# UTF-8 / glyph-pack pieces of the library). However, those pieces live in
# the same static archive as LCDDisplay, and when the linker pulls
# liblcd2usb.a's object files it also drags in the unresolved libusb
# symbols referenced by lcd_display.cpp. Same workaround the examples use:
# link libusb on the tool too. The linker discards unused functions.

add_executable(glyph_inspect glyph_inspect.cpp)
target_link_libraries(glyph_inspect PRIVATE LCD2USB::lcd2usb)

# libusb is exposed via two different cmake variables depending on the
# platform — on macOS we go through find_library (LIBUSB_LIBRARY, singular),
# on Linux through pkg-config (LIBUSB_LIBRARIES, plural). Mirror what the
# examples do.
if(APPLE)
    target_link_libraries(glyph_inspect PRIVATE ${LIBUSB_LIBRARY})
else()
    target_link_libraries(glyph_inspect PRIVATE ${LIBUSB_LIBRARIES})
endif()

set_target_properties(glyph_inspect PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tools
)
install(TARGETS glyph_inspect RUNTIME DESTINATION bin)

# ufo_to_glyphs.py is a Python tool — just install it alongside the binary.
install(PROGRAMS ufo_to_glyphs.py DESTINATION bin)
