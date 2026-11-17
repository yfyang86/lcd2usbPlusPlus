/*
 * glyph_table.cpp - Default codepoint → glyph mappings
 */

#include "lcd2usb/glyph_table.hpp"

namespace LCD2USB {
namespace Glyph {

void GlyphTable::addRom(uint32_t codepoint, uint8_t rom_byte) {
    rom_[codepoint] = rom_byte;
    bitmap_.erase(codepoint);   // a later addRom overrides any prior bitmap
}

void GlyphTable::addBitmap(uint32_t codepoint, const Bitmap& b) {
    bitmap_[codepoint] = b;
    rom_.erase(codepoint);
}

void GlyphTable::addRomBatch(
    const std::initializer_list<std::pair<uint32_t, uint8_t>>& entries)
{
    for (const auto& e : entries) addRom(e.first, e.second);
}

Resolution GlyphTable::resolve(uint32_t cp) const noexcept {
    Resolution r;
    // ASCII is the wide-open fast path: any 0x20..0x7E maps straight to ROM.
    // We keep 0x7F out — most LCDs render it as a filled block / undefined.
    if (cp >= 0x20u && cp <= 0x7Eu) {
        r.kind = Kind::Rom;
        r.rom_byte = static_cast<uint8_t>(cp);
        return r;
    }

    if (auto it = rom_.find(cp); it != rom_.end()) {
        r.kind = Kind::Rom;
        r.rom_byte = it->second;
        return r;
    }
    if (auto it = bitmap_.find(cp); it != bitmap_.end()) {
        r.kind = Kind::Bitmap;
        r.bitmap = it->second;
        return r;
    }
    return r;  // Missing
}

// ─────────────────────────────────────────────────────────────────────────────
// Default table
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// Bitmap helper: each string is 5 chars; '#' = pixel on, anything else = off.
// Rows are top-to-bottom, so index 7 is the bottom row (descender space).
constexpr Bitmap pack(const char (&r0)[6], const char (&r1)[6],
                      const char (&r2)[6], const char (&r3)[6],
                      const char (&r4)[6], const char (&r5)[6],
                      const char (&r6)[6], const char (&r7)[6])
{
    auto row = [](const char* s) -> uint8_t {
        uint8_t v = 0;
        for (int i = 0; i < 5; ++i) {
            v = static_cast<uint8_t>(v << 1);
            if (s[i] != ' ' && s[i] != '.' && s[i] != '0') v |= 1u;
        }
        return v;
    };
    return Bitmap{ row(r0), row(r1), row(r2), row(r3),
                   row(r4), row(r5), row(r6), row(r7) };
}

} // anonymous

GlyphTable makeDefaultTable() {
    GlyphTable t;

    // ── Latin-1 accented letters → unaccented ASCII ─────────────────────────
    // This is the single biggest bandwidth win. Western European text becomes
    // free of CGRAM pressure because no slot has to be allocated.
    t.addRomBatch({
        // Uppercase
        {0x00C0,'A'},{0x00C1,'A'},{0x00C2,'A'},{0x00C3,'A'},{0x00C4,'A'},{0x00C5,'A'},
        {0x00C6,'A'},{0x00C7,'C'},
        {0x00C8,'E'},{0x00C9,'E'},{0x00CA,'E'},{0x00CB,'E'},
        {0x00CC,'I'},{0x00CD,'I'},{0x00CE,'I'},{0x00CF,'I'},
        {0x00D0,'D'},{0x00D1,'N'},
        {0x00D2,'O'},{0x00D3,'O'},{0x00D4,'O'},{0x00D5,'O'},{0x00D6,'O'},{0x00D8,'O'},
        {0x00D9,'U'},{0x00DA,'U'},{0x00DB,'U'},{0x00DC,'U'},
        {0x00DD,'Y'},{0x00DE,'T'},{0x00DF,'s'},                // ß → s (lower)
        // Lowercase
        {0x00E0,'a'},{0x00E1,'a'},{0x00E2,'a'},{0x00E3,'a'},{0x00E4,'a'},{0x00E5,'a'},
        {0x00E6,'a'},{0x00E7,'c'},
        {0x00E8,'e'},{0x00E9,'e'},{0x00EA,'e'},{0x00EB,'e'},
        {0x00EC,'i'},{0x00ED,'i'},{0x00EE,'i'},{0x00EF,'i'},
        {0x00F0,'d'},{0x00F1,'n'},
        {0x00F2,'o'},{0x00F3,'o'},{0x00F4,'o'},{0x00F5,'o'},{0x00F6,'o'},{0x00F8,'o'},
        {0x00F9,'u'},{0x00FA,'u'},{0x00FB,'u'},{0x00FC,'u'},
        {0x00FD,'y'},{0x00FE,'t'},{0x00FF,'y'},
    });

    // ── Typographic punctuation → plain ASCII ──────────────────────────────
    // These have no distinguishable 5x8 form anyway — save the CGRAM slots.
    t.addRomBatch({
        {0x00A0, ' '},   // NBSP
        {0x00AB, '"'},{0x00BB, '"'},     // « »
        {0x2018, '\''},{0x2019, '\''},   // ‘ ’
        {0x201C, '"' },{0x201D, '"' },   // “ ”
        {0x2013, '-' },{0x2014, '-' },   // – —
        {0x2022, '*' },                  // •  (bullet - also has a bitmap fallback below)
        {0x00B7, '.' },                  // ·  middle dot
        {0x00BF, '?' },                  // ¿
        {0x00A1, '!' },                  // ¡
        {0x00B4, '\''},                  // ´
        {0x00B8, ',' },                  // ¸
    });

    // ── Extended Latin diacritics people occasionally paste ────────────────
    t.addRomBatch({
        {0x0152,'O'},{0x0153,'o'},   // Œ œ
        {0x0160,'S'},{0x0161,'s'},   // Š š
        {0x0178,'Y'},{0x017D,'Z'},{0x017E,'z'},   // Ÿ Š Ž ž
    });

    // ── Bitmap glyphs — real pictographs the ROM lacks ─────────────────────
    // Degree sign °
    t.addBitmap(0x00B0, pack(
        " ##  ",
        "#  # ",
        "#  # ",
        " ##  ",
        "     ",
        "     ",
        "     ",
        "     "));

    // Plus-minus ±
    t.addBitmap(0x00B1, pack(
        "  #  ",
        "  #  ",
        "#####",
        "  #  ",
        "  #  ",
        "     ",
        "#####",
        "     "));

    // Multiplication ×
    t.addBitmap(0x00D7, pack(
        "     ",
        "     ",
        "#   #",
        " # # ",
        "  #  ",
        " # # ",
        "#   #",
        "     "));

    // Division ÷
    t.addBitmap(0x00F7, pack(
        "     ",
        "  #  ",
        "     ",
        "#####",
        "     ",
        "  #  ",
        "     ",
        "     "));

    // Less-or-equal ≤ / Greater-or-equal ≥
    t.addBitmap(0x2264, pack(
        "    #",
        "   # ",
        "  #  ",
        "   # ",
        "    #",
        "     ",
        "#####",
        "     "));
    t.addBitmap(0x2265, pack(
        "#    ",
        " #   ",
        "  #  ",
        " #   ",
        "#    ",
        "     ",
        "#####",
        "     "));

    // Arrows ← ↑ → ↓
    t.addBitmap(0x2190, pack(
        "     ",
        "  #  ",
        " #   ",
        "#####",
        " #   ",
        "  #  ",
        "     ",
        "     "));
    t.addBitmap(0x2191, pack(
        "  #  ",
        " ### ",
        "# # #",
        "  #  ",
        "  #  ",
        "  #  ",
        "  #  ",
        "     "));
    t.addBitmap(0x2192, pack(
        "     ",
        "  #  ",
        "   # ",
        "#####",
        "   # ",
        "  #  ",
        "     ",
        "     "));
    t.addBitmap(0x2193, pack(
        "  #  ",
        "  #  ",
        "  #  ",
        "  #  ",
        "# # #",
        " ### ",
        "  #  ",
        "     "));

    // Ellipsis … — the standalone 1-cell form
    t.addBitmap(0x2026, pack(
        "     ",
        "     ",
        "     ",
        "     ",
        "     ",
        "     ",
        "# # #",
        "     "));

    // Bullet • — prefer the bitmap if the slot is available; falls back to '*'
    // (addBitmap wins over the earlier Rom mapping).
    t.addBitmap(0x2022, pack(
        "     ",
        "     ",
        " ### ",
        " ### ",
        " ### ",
        "     ",
        "     ",
        "     "));

    // Currency: £, ¥, €
    t.addBitmap(0x00A3, pack(
        "  ## ",
        " #  #",
        " #   ",
        "###  ",
        " #   ",
        " #   ",
        "#### ",
        "     "));
    t.addBitmap(0x00A5, pack(
        "#   #",
        "#   #",
        " # # ",
        "#####",
        "  #  ",
        "#####",
        "  #  ",
        "     "));
    t.addBitmap(0x20AC, pack(
        "  ###",
        " #   ",
        "#### ",
        " #   ",
        "#### ",
        " #   ",
        "  ###",
        "     "));

    // § section sign, © copyright (compact)
    t.addBitmap(0x00A7, pack(
        " ### ",
        "#    ",
        " ##  ",
        "#  # ",
        "  ## ",
        "    #",
        " ### ",
        "     "));
    t.addBitmap(0x00A9, pack(
        " ### ",
        "#   #",
        "# ###",
        "# #  ",
        "# ###",
        "#   #",
        " ### ",
        "     "));

    return t;
}

} // namespace Glyph
} // namespace LCD2USB
