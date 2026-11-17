/*
 * glyph_source.cpp - shared GlyphSource helpers
 */

#include "lcd2usb/glyph_source.hpp"
#include "lcd2usb/utf8.hpp"

namespace LCD2USB {
namespace Font {

int GlyphSource::measure(std::string_view utf8) {
    int total = 0;
    const char* p   = utf8.data();
    const char* end = p + utf8.size();
    while (p < end) {
        const uint32_t cp = Utf8::decode_next(&p, end);
        if (cp == 0) break;
        const int w = cellWidth(cp);
        total += (w > 0) ? w : 1;      // missing glyph still takes 1 cell
    }
    return total;
}

} // namespace Font
} // namespace LCD2USB
