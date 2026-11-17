/*
 * glyph_inspect.cpp - Print glyphs from a .glyphs pack as ASCII art.
 *
 * A small maintenance tool. Useful after running ufo_to_glyphs.py to
 * sanity-check that the pack contains what you expect, and at what
 * positions. Works on any GlyphSource, so it doubles as a preview for
 * TTFs rasterized by BitmapFont.
 *
 * Usage:
 *   glyph_inspect assets/boutique7x7.glyphs 'A'              # single char
 *   glyph_inspect assets/boutique7x7.glyphs 0x4E2D           # hex codepoint
 *   glyph_inspect assets/boutique7x7.glyphs 'Hello 中国'      # a whole string
 *   glyph_inspect assets/chillbitmap7x.ttf  'A' --ttf [--px 9]
 */

#include "lcd2usb/glyph_pack.hpp"
#include "lcd2usb/bitmap_font.hpp"
#include "lcd2usb/utf8.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

using namespace LCD2USB;
using namespace LCD2USB::Font;

namespace {

void printGlyph(uint32_t cp, const Rendered& r) {
    std::printf("U+%04X  cell_width=%d\n", cp, r.cell_width);
    if (r.cell_width == 0) {
        std::printf("  (missing — not in this source)\n\n");
        return;
    }

    // One visual row, with a separator between the two 5-cell halves for
    // 2-cell glyphs so you can see the split the LCD will show.
    for (int y = 0; y < 8; ++y) {
        std::printf("  ");
        for (int x = 0; x < 5; ++x) putchar((r.left[y] >> (4 - x)) & 1 ? '#' : '.');
        if (r.cell_width == 2) {
            std::putchar('|');
            for (int x = 0; x < 5; ++x) putchar((r.right[y] >> (4 - x)) & 1 ? '#' : '.');
        }
        std::putchar('\n');
    }

    // Hex bytes — handy when copying into C source.
    std::printf("  left  = { ");
    for (int i = 0; i < 8; ++i) std::printf("0x%02X%s", r.left[i], i < 7 ? ", " : "");
    std::printf(" }\n");
    if (r.cell_width == 2) {
        std::printf("  right = { ");
        for (int i = 0; i < 8; ++i) std::printf("0x%02X%s", r.right[i], i < 7 ? ", " : "");
        std::printf(" }\n");
    }
    std::putchar('\n');
}

int parseCodepointOrStr(const char* arg, std::string& out_utf8) {
    // Hex numeric?  "0xNNNN" or "U+NNNN"
    if ((arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X')) ||
        (arg[0] == 'U' && arg[1] == '+')) {
        const char* p = arg + 2;
        uint32_t v = std::strtoul(p, nullptr, 16);
        // Encode as UTF-8.
        if (v < 0x80)       { out_utf8 = std::string(1, char(v)); }
        else if (v < 0x800) { out_utf8 = std::string({char(0xC0 | (v >> 6)), char(0x80 | (v & 0x3F))}); }
        else if (v < 0x10000) {
            out_utf8 = std::string({char(0xE0 | (v >> 12)), char(0x80 | ((v >> 6) & 0x3F)), char(0x80 | (v & 0x3F))});
        } else {
            out_utf8 = std::string({char(0xF0 | (v >> 18)), char(0x80 | ((v >> 12) & 0x3F)),
                                    char(0x80 | ((v >> 6) & 0x3F)), char(0x80 | (v & 0x3F))});
        }
        return 0;
    }
    out_utf8 = arg;
    return 0;
}

void usage() {
    std::fprintf(stderr,
        "usage: glyph_inspect <pack-or-ttf> <char | string | 0xHEX | U+HEX> [--ttf] [--px N]\n");
}

} // anonymous

int main(int argc, char** argv) {
    if (argc < 3) { usage(); return 1; }

    const char* path = argv[1];
    const char* what = argv[2];
    bool use_ttf = false;
    int  px      = 9;

    for (int i = 3; i < argc; ++i) {
        if (std::strcmp(argv[i], "--ttf") == 0) use_ttf = true;
        else if (std::strcmp(argv[i], "--px") == 0 && i + 1 < argc) { px = std::atoi(argv[++i]); }
        else { usage(); return 1; }
    }

    std::unique_ptr<GlyphSource> src;
    if (use_ttf) {
        auto f = std::make_unique<BitmapFont>();
        if (!f->loadTTF(path, px)) {
            std::fprintf(stderr, "failed to load TTF: %s\n", path);
            return 2;
        }
        std::printf("[ttf %s @ %dpx]\n\n", path, px);
        src = std::move(f);
    } else {
        auto p = std::make_unique<GlyphPack>();
        if (!p->loadFile(path)) {
            std::fprintf(stderr, "failed to load pack: %s\n", path);
            return 2;
        }
        std::printf("[pack %s: %zu glyphs]\n\n", path, p->glyphCount());
        src = std::move(p);
    }

    std::string utf8;
    parseCodepointOrStr(what, utf8);

    // Iterate the string and print every codepoint found.
    const char* q = utf8.data();
    const char* e = q + utf8.size();
    while (q < e) {
        const uint32_t cp = Utf8::decode_next(&q, e);
        if (cp == 0) break;
        printGlyph(cp, src->render(cp));
    }
    return 0;
}
