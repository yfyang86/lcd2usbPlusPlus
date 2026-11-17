/*
 * lcd2usb/glyph_table.hpp - Codepoint-to-glyph registry
 *
 * HD44780 displays have ~96 ROM glyphs (plain ASCII) plus a region of extended
 * symbols that varies between ROM code A00 (US/Japanese, katakana at 0xA0..0xDF)
 * and A02 (European, Latin-1 at 0x80..0xFF). On top of that we have 7 usable
 * CGRAM slots for bitmap glyphs.
 *
 * The GlyphTable maps each incoming Unicode codepoint to one of:
 *
 *   - Rom      → a single ROM byte to send verbatim (covers ASCII plus any
 *                transliterations you register, e.g. U+00E9 é → 'e').
 *   - Bitmap   → 8 bytes of pixel data that must be uploaded to CGRAM before
 *                use (for characters the ROM simply doesn't have: °, €, ←, …).
 *   - Missing  → no mapping — caller substitutes the replacement glyph.
 *
 * Why this split matters for bandwidth: every Bitmap codepoint that actually
 * appears on screen costs a CGRAM slot + ~3 USB transfers to upload. Rom
 * passthroughs cost zero slots and fold into the data-stream diff. So a good
 * default table has *many* transliterations and *few* bitmaps.
 */

#pragma once

#include "lcd2usb.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace LCD2USB {
namespace Glyph {

using Bitmap = std::array<uint8_t, LCD::CUSTOM_CHAR_HEIGHT>;

enum class Kind : uint8_t {
    Missing = 0,
    Rom     = 1,
    Bitmap  = 2,
};

struct Resolution {
    Kind    kind     = Kind::Missing;
    uint8_t rom_byte = 0;       // valid when kind == Rom
    Bitmap  bitmap{};           // valid when kind == Bitmap
};

/**
 * A registry of codepoint → glyph mappings. Cheap to copy-construct — uses
 * unordered_map under the hood so fill it once and reuse.
 */
class GlyphTable {
public:
    GlyphTable() = default;

    // Insert / override a ROM passthrough or transliteration.
    // Example: addRom(0x00E9 /*é*/, 'e');   addRom(0x2013 /*–*/, '-');
    void addRom(uint32_t codepoint, uint8_t rom_byte);

    // Insert / override a CGRAM bitmap (8 rows, low 5 bits each).
    void addBitmap(uint32_t codepoint, const Bitmap& bitmap);

    // Convenience: register a whole batch at once.
    void addRomBatch(const std::initializer_list<std::pair<uint32_t, uint8_t>>& entries);

    // Resolve a single codepoint. Never throws.
    Resolution resolve(uint32_t codepoint) const noexcept;

    // Number of entries (mostly useful for tests / diagnostics).
    std::size_t romCount()    const noexcept { return rom_.size(); }
    std::size_t bitmapCount() const noexcept { return bitmap_.size(); }

private:
    std::unordered_map<uint32_t, uint8_t> rom_;
    std::unordered_map<uint32_t, Bitmap>  bitmap_;
};

/**
 * Build the default table targeting the A00 (US/Japanese) ROM variant, which
 * is the commonest one shipped with LCD2USB hardware. Includes:
 *
 *   - Full ASCII passthrough (implicit — any codepoint < 0x80 is ROM-handled
 *     directly by UnicodeDisplay; no map entries needed).
 *   - Latin-1 accented letters transliterated to their unaccented equivalents
 *     (é → e, ñ → n, ü → u, …) so Western European text "just works" without
 *     burning CGRAM slots.
 *   - Typographic punctuation flattened to ASCII (smart quotes → " / ',
 *     en/em dash → -, ellipsis → ... *unless* you prefer the 1-cell bitmap,
 *     see below).
 *   - A small set of bitmap glyphs for symbols with no ASCII equivalent that
 *     people actually want on an LCD: °, ←, →, ↑, ↓, ≤, ≥, ±, ×, ÷, •, …,
 *     €, £, ¥, ©, § — 16 bitmaps, most of which share at most a few cells so
 *     the LRU cache handles normal prose without thrashing.
 */
GlyphTable makeDefaultTable();

} // namespace Glyph
} // namespace LCD2USB
