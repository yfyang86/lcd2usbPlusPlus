/*
 * lcd2usb/glyph_pack.hpp - Load pre-rasterized .glyphs files
 *
 * A .glyphs file is a compact binary pack produced by
 * `tools/ufo_to_glyphs.py` (or any other tool that knows the format).
 * It contains 1- or 2-cell HD44780 bitmaps for a set of Unicode codepoints
 * and is the recommended way to ship fonts that have been rasterized at
 * design time — no FreeType dependency needed at runtime.
 *
 * Binary layout (little-endian throughout):
 *
 *   offset  size  field
 *   ──────  ────  ─────────────────────────────────────────────────────────
 *   0       4     magic         = "LCDG"  (ASCII, four bytes)
 *   4       1     version       = 1
 *   5       1     cell_width    = 5       (pixels per HD44780 cell)
 *   6       1     cell_height   = 8
 *   7       1     slot_bytes    = 24      (size of each glyph entry below)
 *   8       4     glyph_count   = N       (u32 LE)
 *   12      4     reserved
 *   16      N*24  glyph entries, SORTED by codepoint ascending
 *
 * Each glyph entry (24 bytes):
 *
 *   offset  size  field
 *   ──────  ────  ─────────────────────────────────────────────────────────
 *   0       4     codepoint     (u32 LE — the Unicode scalar value)
 *   4       1     cell_count    (1 = single cell, 2 = two cells wide)
 *   5       3     padding (zero)
 *   8       8     left bitmap   (row 0..7, low 5 bits = pixels, bit 4 = leftmost)
 *   16      8     right bitmap  (same; all-zero for single-cell glyphs)
 *
 * Entries are sorted by codepoint so lookups can use binary search in the
 * mmap'd buffer with no decoding, no allocation, and no hash map.
 */

#pragma once

#include "glyph_source.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace LCD2USB {
namespace Font {

/**
 * Runtime loader for a .glyphs pack.
 *
 * Usage:
 *
 *   GlyphPack pack;
 *   if (!pack.loadFile("assets/boutique7x7.glyphs")) { ... }
 *
 *   BitmapFont or GlyphPack — either one is a GlyphSource, so:
 *   FontAdapter adapter(pack, unicode_display);
 *
 * Memory model: the entire file is read into a private buffer on load. At
 * ~250KB for a full CJK font this is nothing to worry about; swapping in a
 * smaller, ASCII-only pack is trivial and lets you deploy on smaller hosts.
 */
class GlyphPack : public GlyphSource {
public:
    GlyphPack() = default;
    ~GlyphPack() override = default;

    GlyphPack(const GlyphPack&) = delete;
    GlyphPack& operator=(const GlyphPack&) = delete;

    /**
     * Read and validate a .glyphs file. Returns false on I/O failure, bad
     * magic, unsupported version, or structural inconsistency. On failure
     * any previously-loaded data is discarded and glyphCount()==0.
     */
    bool loadFile(const std::string& path);

    /**
     * Lookup path — binary search over the glyph table. Missing codepoints
     * return a Rendered with cell_width == 0, same convention as BitmapFont.
     */
    Rendered render(uint32_t codepoint) override;
    int      cellWidth(uint32_t codepoint) override;

    bool        isLoaded()   const { return !data_.empty(); }
    std::size_t glyphCount() const { return glyph_count_; }

    // Constants matching the binary format — exposed because test code and
    // writer-side tools need to agree on them.
    static constexpr char     MAGIC[4]      = {'L', 'C', 'D', 'G'};
    static constexpr uint8_t  VERSION       = 1;
    static constexpr uint8_t  CELL_WIDTH    = 5;
    static constexpr uint8_t  CELL_HEIGHT   = 8;
    static constexpr uint8_t  SLOT_BYTES    = 24;
    static constexpr std::size_t HEADER_SIZE = 16;

private:
    std::vector<uint8_t> data_;           // the whole file, kept around
    const uint8_t* entries_ = nullptr;    // data_.data() + HEADER_SIZE
    std::size_t    glyph_count_ = 0;
};

} // namespace Font
} // namespace LCD2USB
