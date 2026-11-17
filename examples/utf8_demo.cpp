/*
 * utf8_demo.cpp - Shows UnicodeDisplay handling UTF-8 text, a diff-friendly
 * clock update, and CGRAM reuse across frames.
 *
 * Build target name: utf8_demo
 *
 * The printed counters at the end show the bandwidth win: even though each
 * frame of the clock has the full 20x4 of content logically, flush() pushes
 * only the bytes that actually changed.
 */

#include "lcd2usb/lcd2usb_lib.hpp"
#include <chrono>
#include <iostream>
#include <thread>

using namespace LCD2USB;
using namespace std::chrono_literals;

int main() {
    LCDDisplay lcd;
    if (!lcd.initialize()) {
        std::cerr << "Failed to open LCD2USB device\n";
        return 1;
    }
    lcd.setContrast(200);
    lcd.setBrightness(255);
    lcd.clear();

    UnicodeDisplay ud(lcd, 20, 4);

    // ── Part 1: mixed-script line, static ──────────────────────────────────
    // Latin-1 accents transliterate (no CGRAM used). °, ≥, € allocate slots.
    ud.println(0, "Café ≥ 38°C");       // é→e, ≥ bitmap, ° bitmap
    ud.println(1, "Price: 5€ / kg");    // € bitmap
    ud.println(2, "Arrows: ← ↑ → ↓");   // 4 arrow bitmaps
    ud.println(3, "Piña colada ±0.5");  // ñ→n, ± bitmap
    int cells_0 = ud.flush();
    std::cout << "Initial frame: " << cells_0 << " cells written, "
              << ud.cgramOccupancy() << "/" << LCD::USABLE_SLOT_COUNT
              << " CGRAM slots used.\n";

    std::this_thread::sleep_for(2s);

    // ── Part 2: animated clock — the diffing win ───────────────────────────
    // The bottom row becomes a live clock. Everything else stays exactly the
    // same, so each flush should push ~the digits that changed and no more.
    ud.println(3, "Clock:   00:00:00  ");   // reset to a known state
    (void)ud.flush();

    int total_cells_sent = 0;
    for (int tick = 0; tick < 5; ++tick) {
        const int hh = 12;
        const int mm = 34;
        const int ss = 56 + tick;
        char buf[32];
        std::snprintf(buf, sizeof(buf), "Clock:   %02d:%02d:%02d  ", hh, mm, ss);
        ud.println(3, buf);
        total_cells_sent += ud.flush();
        std::this_thread::sleep_for(1s);
    }
    std::cout << "Clock animation (5 ticks): " << total_cells_sent
              << " cells written total (vs " << (5 * 20) << " if no diffing)\n";

    // ── Part 3: runtime-registered custom glyph ────────────────────────────
    // Register a lambda-defined bitmap for U+266B (♫) and watch it render.
    ud.table().addBitmap(0x266B, Glyph::Bitmap{
        0b00000,
        0b00111,
        0b00101,
        0b00101,
        0b01101,
        0b11101,
        0b11000,
        0b00000,
    });
    ud.clear();
    ud.println(0, "Runtime glyph:");
    ud.println(1, "  ♫ hello ♫");
    ud.flush();
    std::this_thread::sleep_for(2s);

    std::cout << "Done. Final CGRAM occupancy: " << ud.cgramOccupancy()
              << "/" << LCD::USABLE_SLOT_COUNT << "\n";
    return 0;
}
