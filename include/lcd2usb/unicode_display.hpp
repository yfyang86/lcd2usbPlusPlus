/*
 * lcd2usb/unicode_display.hpp - UTF-8-aware, bandwidth-friendly display wrapper
 *
 * Wraps an LCDDisplay with three layers that together solve the "UTF-8 on a
 * tiny USB LCD" problem:
 *
 *   1. UTF-8 decode → codepoints (via Utf8::decode_next).
 *   2. Codepoint → cell-byte via a GlyphTable. If the codepoint needs a
 *      bitmap, we allocate a CGRAM slot for it from an LRU-managed pool of
 *      7 slots (1..7, slot 0 is deliberately avoided).
 *   3. Shadow framebuffer: write operations mutate an in-memory frame; flush()
 *      diffs that frame against what is currently on the LCD and pushes only
 *      the changed cells, coalesced into contiguous same-row runs so each
 *      USB transfer carries up to BUFFER_MAX_CMD (=4) payload bytes.
 *
 * Typical usage:
 *
 *   LCDDisplay lcd;
 *   lcd.initialize();
 *
 *   UnicodeDisplay ud(lcd);                 // 20×4 by default
 *   ud.print("Café ≥ 38°C");                // no USB traffic yet
 *   ud.setCursor(0, 1);
 *   ud.print("Price: 5€");
 *   ud.flush();                             // ONE batch of USB transfers
 *
 * Every subsequent flush() only sends the diff from the previous flush, so
 * an animated clock that changes two digits a second costs ~2 transfers, not
 * 80.
 */

#pragma once

#include "lcd2usb.hpp"
#include "lcd_display.hpp"
#include "glyph_table.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace LCD2USB {

class UnicodeDisplay {
public:
    // Sentinel codepoint used internally to mark "cell is blank" in both the
    // desired and shadow framebuffers. U+0020 (space) is the obvious choice.
    static constexpr uint32_t BLANK = 0x20;

    /**
     * @param lcd      Underlying LCDDisplay (not owned; must outlive *this).
     * @param width    Columns (default 20 — matches the 20x4 hardware the
     *                 existing row-address table targets).
     * @param height   Rows (default 4).
     * @param table    Glyph table. Defaults to the built-in Latin-1 +
     *                 typographic + symbol table.
     */
    explicit UnicodeDisplay(LCDDisplay& lcd,
                            int width  = 20,
                            int height = 4,
                            Glyph::GlyphTable table = Glyph::makeDefaultTable());

    UnicodeDisplay(const UnicodeDisplay&) = delete;
    UnicodeDisplay& operator=(const UnicodeDisplay&) = delete;

    // ─── Framebuffer mutations (no USB traffic on their own) ───────────────
    void clear();                                       // logical clear → flush sends only the diff
    void setCursor(int col, int row);                   // out-of-range is clamped
    void print(std::string_view utf8);                  // writes at current cursor, wraps on row end
    void printAt(int col, int row, std::string_view utf8);
    void println(int row, std::string_view utf8);       // overwrites the entire row with padding

    // Low-level raw-codepoint write. Use this when the caller has already
    // decoded UTF-8 and wants to place synthetic codepoints (e.g. the
    // "right-half of a wide glyph" sentinels used by BitmapFont adapters).
    // Advances the cursor by exactly `n` cells — one per codepoint — and
    // does NOT consult any width hook, so the caller is responsible for
    // emitting a continuation codepoint for each wide glyph.
    void printCodepoints(const uint32_t* cps, std::size_t n);

    // Current cursor position (so adapters can measure their own progress).
    int cursorCol() const { return cursor_col_; }
    int cursorRow() const { return cursor_row_; }

    // ─── Commit ─────────────────────────────────────────────────────────────
    // Uploads any CGRAM slot whose assigned bitmap has changed, then pushes
    // the cell-level diff. Returns the number of DDRAM cells written — useful
    // for benchmarking.
    int flush();

    // Forces the next flush to rewrite every cell. Use when you don't trust
    // the LCD's state (e.g. after an external device reset).
    void invalidate();

    // ─── Config ────────────────────────────────────────────────────────────
    // Character to substitute when a codepoint has no mapping AND all CGRAM
    // slots are already locked by other bitmap codepoints in the same frame.
    // Default is ASCII '?'.
    void setReplacement(uint8_t rom_byte) { replacement_ = rom_byte; }

    // Expose the underlying table so users can register custom glyphs at runtime.
    Glyph::GlyphTable&       table()       { return table_; }
    const Glyph::GlyphTable& table() const { return table_; }

    int width()  const { return width_; }
    int height() const { return height_; }

    // Diagnostics — how many codepoints currently live in CGRAM.
    int cgramOccupancy() const;

private:
    // A single CGRAM slot's state across frames.
    struct Slot {
        bool            occupied = false;    // does this slot hold anything?
        uint32_t        codepoint = 0;       // which codepoint it holds
        Glyph::Bitmap   bitmap{};            // ...and its pixel data
        bool            dirty = false;       // needs re-uploading to the LCD
        uint64_t        last_used = 0;       // LRU counter
        bool            locked_this_frame = false; // in use by the frame being flushed
    };

    // ─── Resolution & CGRAM allocation ─────────────────────────────────────
    // Returns the byte to place in DDRAM for this cell, or -1 to indicate
    // "no glyph available even after replacement" (shouldn't happen in
    // practice; kept as a defensive signal).
    int resolveCell(uint32_t codepoint);
    int allocateSlot(uint32_t codepoint, const Glyph::Bitmap& bitmap);
    void lockSlot(int slot);

    // Helpers.
    int  cellIndex(int col, int row) const { return row * width_ + col; }
    bool inBounds(int col, int row)  const { return col >= 0 && col < width_ && row >= 0 && row < height_; }

    LCDDisplay&         lcd_;
    int                 width_;
    int                 height_;
    Glyph::GlyphTable   table_;
    uint8_t             replacement_ = '?';

    // desired[i]  = codepoint we want at cell i  (what print() writes into)
    // shown_cp_[i]= codepoint currently on screen at cell i  (shadow)
    // shown_byte_ = byte currently on screen at cell i        (needed because
    //               the same codepoint can map to different bytes across
    //               frames when CGRAM reassignments happen — we compare bytes
    //               for the diff, not codepoints).
    std::vector<uint32_t> desired_;
    std::vector<uint32_t> shown_cp_;
    std::vector<uint8_t>  shown_byte_;

    int cursor_col_ = 0;
    int cursor_row_ = 0;

    std::array<Slot, LCD::MAX_CUSTOM_CHARS> slots_{};
    uint64_t tick_ = 0;
};

} // namespace LCD2USB
