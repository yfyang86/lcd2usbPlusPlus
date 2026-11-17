/*
 * lcd2usb/glyph_source.hpp - Abstract glyph provider
 *
 * Any object that can turn a Unicode codepoint into a 1- or 2-cell HD44780
 * bitmap is a GlyphSource. The two concrete implementations today are:
 *
 *   - BitmapFont  — FreeType-backed, reads TTF at runtime.
 *   - GlyphPack   — loads a pre-rasterized .glyphs file, no font library
 *                   dependency at runtime.
 *
 * FontAdapter and any other glue code bind to this base so the runtime font
 * source is a policy choice, not a compile-time lock-in. For example, a
 * firmware build targeting a tiny embedded host can drop FreeType entirely
 * and ship only a .glyphs pack.
 */

#pragma once

#include "glyph_table.hpp"
#include <cstdint>
#include <string_view>

namespace LCD2USB {
namespace Font {

// Forward-declared here so GlyphSource can return it without a circular
// include. Actual definition is in bitmap_font.hpp.
struct Rendered {
    int            cell_width = 0;   // 0 = missing, 1 or 2 cells
    Glyph::Bitmap  left  {};
    Glyph::Bitmap  right {};
};

// The synthetic high-bit flag that marks "right half of a wide glyph"
// codepoints. See bitmap_font.hpp for the full rationale.
constexpr uint32_t RIGHT_HALF_FLAG = 0x80000000u;
inline constexpr uint32_t rightHalfOf(uint32_t cp) noexcept { return cp | RIGHT_HALF_FLAG; }
inline constexpr bool     isRightHalf(uint32_t cp) noexcept { return (cp & RIGHT_HALF_FLAG) != 0; }

class GlyphSource {
public:
    virtual ~GlyphSource() = default;

    // Rasterize / look up a single codepoint. Missing codepoints return a
    // Rendered with cell_width == 0.
    virtual Rendered render(uint32_t codepoint) = 0;

    // Handy default: just the cell count.
    virtual int cellWidth(uint32_t codepoint) {
        return render(codepoint).cell_width;
    }

    // Total cell count for a UTF-8 string. Missing glyphs count as 1
    // (the caller will substitute the replacement character).
    int measure(std::string_view utf8);
};

} // namespace Font
} // namespace LCD2USB
