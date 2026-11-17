/*
 * lcd2usb/bitmap_font.hpp - TTF → HD44780-cell glyph rasterizer
 *
 * Wraps FreeType to rasterize arbitrary TrueType fonts (particularly pixel
 * fonts like ChillBitmap) into HD44780-compatible 5×8 bitmap cells.
 *
 * Design decisions driven by the 5×8 hardware:
 *
 *   1. Latin glyphs render at roughly 5 pixels wide → ONE cell.
 *   2. CJK glyphs render at roughly 8 pixels wide → TWO adjacent cells.
 *      The left half occupies the first cell; the right half occupies the
 *      second cell with its bitmap shifted left by 5 columns.
 *   3. Glyphs wider than 2 cells (11+ pixels) are currently clipped to two
 *      cells. For a 7×7 pixel font this doesn't happen in practice.
 *
 * To make the "two-cell glyph" concept fit the single-cell GlyphTable /
 * UnicodeDisplay pipeline cleanly, the right half of a wide glyph is
 * registered in the GlyphTable under a synthetic codepoint:
 *
 *       right_half_codepoint(cp) = cp | 0x80000000
 *
 * 0x80000000 is above U+10FFFF (the maximum real Unicode codepoint) so it
 * can't collide with anything the UTF-8 decoder produces. The FontAdapter
 * writes [cp, right_half(cp)] into two adjacent cells of UnicodeDisplay and
 * the rest of the pipeline (CGRAM allocation, diffing, run coalescing) works
 * unchanged.
 *
 * Typical usage:
 *
 *   BitmapFont font;
 *   font.loadTTF("fonts/ChillBitmap7x.ttf", 9);   // pick a good pixel size
 *
 *   UnicodeDisplay ud(lcd, 20, 4);
 *   FontAdapter adapter(font, ud);
 *   adapter.printAt(0, 0, "Hello 中国！");       // width-aware
 *   ud.flush();
 */

#pragma once

#include "glyph_source.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace LCD2USB {

// Forward declare — implementation hides FreeType behind an opaque pointer so
// users of this header don't need <ft2build.h> on their include path.
class UnicodeDisplay;

namespace Font {

// `Rendered`, `RIGHT_HALF_FLAG`, `rightHalfOf`, `isRightHalf`, and
// `GlyphSource` all come from glyph_source.hpp and are shared between
// BitmapFont (this file) and GlyphPack (glyph_pack.hpp).

/**
 * TTF rasterizer. Loads a TrueType font, rasterizes individual codepoints
 * into HD44780-compatible 5×8 cells. Internally caches renders keyed by
 * codepoint.
 *
 * Safe to copy-assign the output Rendered; the BitmapFont itself holds a
 * FreeType handle and is non-copyable.
 */
class BitmapFont : public GlyphSource {
public:
    BitmapFont();
    ~BitmapFont() override;

    BitmapFont(const BitmapFont&) = delete;
    BitmapFont& operator=(const BitmapFont&) = delete;

    /**
     * @param path        Path to the TTF file.
     * @param pixel_size  FreeType "pixel size" (roughly glyph em-box in px).
     *                    9 is the sweet spot for ChillBitmap7x — Latin fits
     *                    in 5×6, CJK in ~8×7.
     * @param baseline_y  Which row of the 5×8 cell is the text baseline.
     *                    Default 6 leaves one row below for descenders.
     * @return true on success, false if FreeType couldn't open the file.
     */
    bool loadTTF(const std::string& path, int pixel_size = 9, int baseline_y = 6);

    bool isLoaded() const { return face_ != nullptr; }

    /**
     * Rasterize a codepoint into 1 or 2 cells. Returns cell_width=0 when the
     * font has no glyph for this codepoint OR the font isn't loaded.
     * Results are cached: calling render() on the same codepoint twice hits
     * a hash-map lookup.
     */
    Rendered render(uint32_t codepoint) override;

    /**
     * Quick width probe. Equivalent to `render(cp).cell_width` but skips
     * returning the bitmaps. Useful for layout (word-wrapping) code that
     * needs to count cells without actually asking for pixels.
     */
    int cellWidth(uint32_t codepoint) override;

    // Note: measure() is inherited from GlyphSource — the default
    // implementation is the same byte-for-byte as the old one.

    int pixelSize() const { return pixel_size_; }
    int baselineY() const { return baseline_y_; }

private:
    // FreeType state is held via opaque pointers so this header doesn't
    // drag in ft2build.h / FT_FREETYPE_H.
    void* library_ = nullptr;   // FT_Library
    void* face_    = nullptr;   // FT_Face
    int   pixel_size_ = 0;
    int   baseline_y_ = 6;

    std::unordered_map<uint32_t, Rendered> cache_;

    void destroy();
};

/**
 * Glue between a BitmapFont and a UnicodeDisplay.
 *
 *   - Lazily registers bitmaps in the display's GlyphTable the first time a
 *     codepoint is seen. Wide glyphs get both halves registered (cp and
 *     cp|RIGHT_HALF_FLAG).
 *   - Offers `print` / `printAt` / `println` that mirror UnicodeDisplay's
 *     API but are wide-glyph-aware: a CJK codepoint advances the cursor by
 *     TWO cells and writes TWO framebuffer entries.
 *
 * The FontAdapter does NOT own the font or the display. Either may be used
 * directly alongside the adapter; the adapter just layers wide-glyph
 * bookkeeping on top.
 */
class FontAdapter {
public:
    FontAdapter(GlyphSource& font, UnicodeDisplay& display);

    // Same semantics as the corresponding UnicodeDisplay methods, but
    // width-aware: each CJK codepoint consumes two cells.
    void print(std::string_view utf8);
    void printAt(int col, int row, std::string_view utf8);
    void println(int row, std::string_view utf8);   // clears the row first

    // How many cells the given UTF-8 text will consume — the unit of width
    // for all layout decisions in this adapter (word wrap, padding, etc.).
    int measureCells(std::string_view utf8);

    // Pre-register every codepoint in a string. Called automatically by
    // print/printAt/println, but exposed for callers who want to load the
    // glyph cache ahead of a tight-timing loop.
    void preloadGlyphs(std::string_view utf8);

    GlyphSource& font() { return font_; }
    UnicodeDisplay& display() { return display_; }

    /**
     * Whether ASCII codepoints (U+0020..U+007E) are rendered from the font.
     * Default: false — ASCII goes through UnicodeDisplay's ROM fast path,
     * which costs zero CGRAM slots. Set to true for a pure pixel-font look
     * where every cell including Latin letters comes from the font.
     *
     * Affects future print() calls; already-installed glyphs are not
     * uninstalled (you'd need a fresh GlyphTable for that).
     */
    void setLatinFromFont(bool on) { latin_from_font_ = on; }
    bool latinFromFont() const { return latin_from_font_; }

private:
    GlyphSource&    font_;
    UnicodeDisplay& display_;
    bool            latin_from_font_ = false;

    // Track which codepoints we've already installed into the display's
    // glyph table, so repeated print() calls on the same text don't do
    // redundant hash-map inserts.
    std::unordered_map<uint32_t, int /*cell_width*/> installed_;

    void installGlyph(uint32_t cp);
    int  codepointWidth(uint32_t cp);   // respects latin_from_font_
};

} // namespace Font
} // namespace LCD2USB
