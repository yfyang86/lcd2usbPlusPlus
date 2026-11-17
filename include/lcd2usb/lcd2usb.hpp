/*
 * lcd2usb/lcd2usb.hpp - Core LCD2USB interface and constants
 * Part of LCD2USB++ library
 */

#pragma once

#include <cstdint>
#include <array>

namespace LCD2USB {

// USB device constants
namespace USB {
    constexpr uint16_t LCD2USB_VID = 0x0403;
    constexpr uint16_t LCD2USB_PID = 0xc630;
}

// LCD control constants
namespace LCD {
    // Controller targets (a single LCD2USB can drive two HD44780 controllers).
    constexpr int CTRL_0 = (1 << 3);
    constexpr int CTRL_1 = (1 << 4);
    constexpr int BOTH   = (CTRL_0 | CTRL_1);

    // Request type bits packed into the USB bRequest byte.
    constexpr int ECHO = (0 << 5);
    constexpr int CMD  = (1 << 5);
    constexpr int DATA = (2 << 5);
    constexpr int SET  = (3 << 5);
    constexpr int GET  = (4 << 5);

    constexpr int SET_CONTRAST     = (SET | (0 << 3));
    constexpr int SET_BRIGHTNESS   = (SET | (1 << 3));
    constexpr int SET_RESERVED0    = (SET | (2 << 3));
    constexpr int SET_RESERVED1    = (SET | (3 << 3));

    constexpr int GET_FWVER        = (GET | (0 << 3));
    constexpr int GET_KEYS         = (GET | (1 << 3));
    constexpr int GET_CTRL         = (GET | (2 << 3));
    constexpr int GET_RESERVED1    = (GET | (3 << 3));

    // ─── Bandwidth-critical constants ───────────────────────────────────────
    // BUFFER_MAX_CMD is the number of payload bytes that fit in ONE USB
    // control transfer (two in wValue, two in wIndex). Every flush = one USB
    // round-trip, so packing bytes up to this limit is how we reduce traffic.
    constexpr int BUFFER_MAX_CMD = 4;

    constexpr int DISPLAY_WIDTH  = 16;   // legacy default; see UnicodeDisplay for per-device geometry
    constexpr int ECHO_NUM       = 100;

    // HD44780 CGRAM layout: 8 slots × 8 rows × 5 columns.
    constexpr int CUSTOM_CHAR_WIDTH  = 5;
    constexpr int CUSTOM_CHAR_HEIGHT = 8;
    constexpr int MAX_CUSTOM_CHARS   = 8;
    constexpr int CGRAM_BASE_ADDR    = 0x40;

    // HD44780 DDRAM address commands. For a 20x4 display the row layout is
    // non-sequential in memory — this table hides that.
    // Index into ROW_ADDR with a row number (0..3); add the column to get the
    // byte for the "Set DDRAM address" command.
    constexpr std::array<uint8_t, 4> ROW_ADDR = {
        0x80,   // row 0 : DDRAM 0x00
        0xC0,   // row 1 : DDRAM 0x40
        0x94,   // row 2 : DDRAM 0x14
        0xD4,   // row 3 : DDRAM 0x54
    };

    // CGRAM slot 0 is unreliable on many HD44780 variants (character code 0
    // sometimes renders as blank or corrupt). Consumers that care about
    // reliability allocate from slot 1 upwards, giving 7 usable custom glyphs.
    constexpr int FIRST_USABLE_SLOT = 1;
    constexpr int USABLE_SLOT_COUNT = MAX_CUSTOM_CHARS - FIRST_USABLE_SLOT;  // 7
}

} // namespace LCD2USB
