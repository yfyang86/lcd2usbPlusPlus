/*
 * bitmap_font.cpp - TTF rasterizer driving the UnicodeDisplay pipeline
 *
 * The heart of this file is render(): given a codepoint, produce 1 or 2
 * HD44780 5×8 cells. Everything else is plumbing.
 *
 * Layout algorithm, in words:
 *
 *   1. Ask FreeType for a monochrome 1-bit bitmap of the glyph at our
 *      configured pixel size.
 *   2. FreeType hands back `bitmap_left` (pen → leftmost pixel) and
 *      `bitmap_top` (pen → topmost pixel, measured UPWARDS from baseline).
 *   3. Translate to (cell_x, cell_y) coordinates:
 *         cell_x = bitmap_left            (left-align to the advance box)
 *         cell_y = baseline_y - bitmap_top
 *      so a glyph whose pen-top is at the baseline lands at row=baseline_y.
 *   4. Pick cell_width = 1 if (cell_x + glyph.width) ≤ 5, else 2.
 *   5. Paint each glyph pixel at (px, py) into
 *         target_col = cell_x + px          (in 10-px-wide virtual space)
 *         target_row = cell_y + py
 *      then split: columns 0..4 → left cell, columns 5..9 → right cell,
 *      clipping anything outside row 0..7.
 */

#include "lcd2usb/bitmap_font.hpp"
#include "lcd2usb/unicode_display.hpp"
#include "lcd2usb/utf8.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cstdio>

namespace LCD2USB {
namespace Font {

// ─────────────────────────────────────────────────────────────────────────────
// BitmapFont
// ─────────────────────────────────────────────────────────────────────────────

BitmapFont::BitmapFont() = default;

BitmapFont::~BitmapFont() { destroy(); }

void BitmapFont::destroy() {
    if (face_)    { FT_Done_Face(static_cast<FT_Face>(face_));          face_ = nullptr; }
    if (library_) { FT_Done_FreeType(static_cast<FT_Library>(library_)); library_ = nullptr; }
    cache_.clear();
}

bool BitmapFont::loadTTF(const std::string& path, int pixel_size, int baseline_y) {
    destroy();

    FT_Library lib = nullptr;
    if (FT_Init_FreeType(&lib) != 0) return false;

    FT_Face face = nullptr;
    if (FT_New_Face(lib, path.c_str(), 0, &face) != 0) {
        FT_Done_FreeType(lib);
        return false;
    }

    // Set pixel size (width=0 tells FreeType to use the same value for width
    // and height, which is what pixel fonts expect).
    if (FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(pixel_size)) != 0) {
        FT_Done_Face(face);
        FT_Done_FreeType(lib);
        return false;
    }

    library_    = lib;
    face_       = face;
    pixel_size_ = pixel_size;
    // Clamp baseline_y into the 0..7 cell range, but leave the caller free
    // to push descenders off if they really want an ascender-biased layout.
    baseline_y_ = std::clamp(baseline_y, 0, LCD::CUSTOM_CHAR_HEIGHT - 1);
    return true;
}

namespace {

// A 10×8 scratch buffer big enough for a 2-cell wide glyph. A `true` means
// pixel-on. We blit into this first, then split into left/right cells.
struct Scratch {
    static constexpr int W = 10;
    static constexpr int H = LCD::CUSTOM_CHAR_HEIGHT;       // 8
    bool px[H][W] = {};

    // Pack a 5-column vertical strip starting at column c0 into a Glyph::Bitmap.
    // HD44780 stores each row as a 5-bit value where bit 4 is the leftmost.
    Glyph::Bitmap pack5(int c0) const {
        Glyph::Bitmap out{};
        for (int y = 0; y < H; ++y) {
            uint8_t b = 0;
            for (int x = 0; x < LCD::CUSTOM_CHAR_WIDTH; ++x) {
                b = static_cast<uint8_t>(b << 1);
                if (c0 + x < W && px[y][c0 + x]) b |= 1u;
            }
            out[y] = b;
        }
        return out;
    }

    // True if any pixel in columns [c0, c0+5) is set.
    bool nonEmpty(int c0) const {
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < LCD::CUSTOM_CHAR_WIDTH; ++x)
                if (c0 + x < W && px[y][c0 + x]) return true;
        return false;
    }
};

} // anonymous

int BitmapFont::cellWidth(uint32_t cp) {
    return render(cp).cell_width;
}

Rendered BitmapFont::render(uint32_t codepoint) {
    if (auto it = cache_.find(codepoint); it != cache_.end()) return it->second;

    Rendered r;                       // default: cell_width=0, empty bitmaps
    if (!face_) { cache_[codepoint] = r; return r; }

    FT_Face face = static_cast<FT_Face>(face_);

    // FT_Load_Char silently falls back to glyph index 0 (.notdef / tofu) for
    // codepoints the font doesn't cover. We want to know the difference, so
    // probe the cmap first.
    if (FT_Get_Char_Index(face, codepoint) == 0) {
        cache_[codepoint] = r;         // cell_width stays 0 → caller can fall back
        return r;
    }

    // Load + render as 1-bit monochrome. FT_LOAD_TARGET_MONO forces the
    // renderer to use the MONO rasterizer (no antialiasing), which is what
    // we want for a pixel font.
    const FT_Int32 flags = FT_LOAD_RENDER | FT_LOAD_MONOCHROME | FT_LOAD_TARGET_MONO;
    if (FT_Load_Char(face, codepoint, flags) != 0) {
        cache_[codepoint] = r;
        return r;
    }
    FT_GlyphSlot slot = face->glyph;
    FT_Bitmap* bm     = &slot->bitmap;

    // Empty glyph (zero-width space, etc.) — treat as a blank 1-cell.
    if (bm->rows == 0 || bm->width == 0) {
        r.cell_width = 1;
        cache_[codepoint] = r;
        return r;
    }

    Scratch s;

    // Top-left of the glyph in cell coordinates. See file header for the math.
    const int cell_x = slot->bitmap_left;
    const int cell_y = baseline_y_ - slot->bitmap_top;

    // Paint each on-pixel of the FT bitmap into the scratch buffer. FreeType
    // stores MONO bitmaps MSB-first, one row per `pitch` bytes (pitch can be
    // negative for flipped layout, but for glyphs loaded with FT_LOAD_RENDER
    // it's positive).
    for (unsigned int py = 0; py < bm->rows; ++py) {
        const unsigned char* row = bm->buffer + py * bm->pitch;
        for (unsigned int px = 0; px < bm->width; ++px) {
            const unsigned char byte = row[px >> 3];
            const bool on = (byte >> (7 - (px & 7))) & 1u;
            if (!on) continue;
            const int dx = cell_x + static_cast<int>(px);
            const int dy = cell_y + static_cast<int>(py);
            if (dx < 0 || dx >= Scratch::W) continue;    // clip horizontally
            if (dy < 0 || dy >= Scratch::H) continue;    // clip vertically
            s.px[dy][dx] = true;
        }
    }

    // Decide cell width. Start with the glyph's horizontal extent; if any
    // pixel landed in columns 5..9, we need two cells.
    const bool has_right = s.nonEmpty(LCD::CUSTOM_CHAR_WIDTH);
    r.cell_width = has_right ? 2 : 1;
    r.left  = s.pack5(0);
    r.right = has_right ? s.pack5(LCD::CUSTOM_CHAR_WIDTH) : Glyph::Bitmap{};

    cache_[codepoint] = r;
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
// FontAdapter
// ─────────────────────────────────────────────────────────────────────────────

FontAdapter::FontAdapter(GlyphSource& font, UnicodeDisplay& display)
    : font_(font), display_(display) {}

int FontAdapter::codepointWidth(uint32_t cp) {
    // ASCII in ROM mode always takes exactly 1 cell regardless of what the
    // font might report (the ROM fast path in UnicodeDisplay is single-cell).
    if (!latin_from_font_ && cp >= 0x20u && cp <= 0x7Eu) return 1;
    const int w = font_.cellWidth(cp);
    return (w > 0) ? w : 1;      // missing glyph → placeholder occupies 1 cell
}

void FontAdapter::installGlyph(uint32_t cp) {
    if (installed_.count(cp)) return;

    // ASCII-via-ROM: no bitmap install at all — UnicodeDisplay's 0x20..0x7E
    // fast path will emit the ROM byte directly. This is the default and
    // saves one CGRAM slot per unique ASCII character visible.
    if (!latin_from_font_ && cp >= 0x20u && cp <= 0x7Eu) {
        installed_[cp] = 1;
        return;
    }

    const auto rendered = font_.render(cp);
    if (rendered.cell_width == 0) {
        // Font has no glyph; don't install anything. UnicodeDisplay will
        // fall through to its own table (transliterations or replacement).
        installed_[cp] = 1;
        return;
    }

    display_.table().addBitmap(cp, rendered.left);
    if (rendered.cell_width == 2) {
        display_.table().addBitmap(rightHalfOf(cp), rendered.right);
    }
    installed_[cp] = rendered.cell_width;
}

void FontAdapter::preloadGlyphs(std::string_view utf8) {
    const char* p   = utf8.data();
    const char* end = p + utf8.size();
    while (p < end) {
        const uint32_t cp = Utf8::decode_next(&p, end);
        if (cp == 0) break;
        if (cp == '\n' || cp == '\r' || cp == '\t') continue;
        installGlyph(cp);
    }
}

int FontAdapter::measureCells(std::string_view utf8) {
    int total = 0;
    const char* p   = utf8.data();
    const char* end = p + utf8.size();
    while (p < end) {
        const uint32_t cp = Utf8::decode_next(&p, end);
        if (cp == 0) break;
        if (cp == '\n') continue;                       // newline is a movement, not a cell
        total += codepointWidth(cp);
    }
    return total;
}

void FontAdapter::print(std::string_view utf8) {
    // First pass: decode + ensure glyphs are installed. We buffer the
    // codepoints (including right-half sentinels) then push them to the
    // display in one call so it only advances the cursor once per cell.
    std::vector<uint32_t> cps;
    cps.reserve(utf8.size());          // upper bound: 1 byte ≤ 1 codepoint

    const int row_width = display_.width();
    int col = display_.cursorCol();
    int row = display_.cursorRow();

    const char* p   = utf8.data();
    const char* end = p + utf8.size();
    while (p < end) {
        const uint32_t cp = Utf8::decode_next(&p, end);
        if (cp == 0) break;

        if (cp == '\n') {
            cps.push_back('\n');
            col = 0;
            if (row + 1 < display_.height()) ++row;
            continue;
        }
        if (cp < 0x20)  continue;                      // strip other controls

        installGlyph(cp);
        const int w = codepointWidth(cp);

        // Soft-wrap: if a wide glyph won't fit in the remaining columns of
        // the current row, force a newline BEFORE it so both halves land on
        // the same row. Without this, the right half would spill onto the
        // next row and render as a disconnected mess.
        if (col + w > row_width) {
            if (row + 1 >= display_.height()) break;    // no more room
            cps.push_back('\n');
            col = 0;
            ++row;
        }

        cps.push_back(cp);
        if (w == 2) cps.push_back(rightHalfOf(cp));
        col += w;
    }

    display_.printCodepoints(cps.data(), cps.size());
}

void FontAdapter::printAt(int col, int row, std::string_view utf8) {
    display_.setCursor(col, row);
    print(utf8);
}

void FontAdapter::println(int row, std::string_view utf8) {
    if (row < 0 || row >= display_.height()) return;
    // Clear the row in the framebuffer. printCodepoints with spaces is fine.
    std::vector<uint32_t> spaces(display_.width(), static_cast<uint32_t>(' '));
    display_.setCursor(0, row);
    display_.printCodepoints(spaces.data(), spaces.size());
    printAt(0, row, utf8);
}

} // namespace Font
} // namespace LCD2USB
