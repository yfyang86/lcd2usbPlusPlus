/*
 * glyph_pack.cpp - Runtime loader for .glyphs binary packs
 *
 * Design notes:
 *
 *   - Endianness: the file is specified little-endian. We do NOT assume the
 *     host is LE; the two u32 reads below are explicit byte-assemblies.
 *     This means the same .glyphs file works on x86_64, aarch64, and also
 *     on the rare BE host without byte-swapping.
 *
 *   - Lookup: fixed-size sorted entries → std::lower_bound. Zero allocation
 *     per render() call.
 *
 *   - No internal cache: the entries are already dense in memory, so hash-
 *     caching on top would just burn bytes. BitmapFont caches because
 *     FreeType rasterization is expensive; here render() is two memcpy's.
 */

#include "lcd2usb/glyph_pack.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>

namespace LCD2USB {
namespace Font {

namespace {

constexpr std::size_t CP_OFF     = 0;
constexpr std::size_t CW_OFF     = 4;
constexpr std::size_t LEFT_OFF   = 8;
constexpr std::size_t RIGHT_OFF  = 16;

inline uint32_t readU32LE(const uint8_t* p) noexcept {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

inline uint32_t entryCodepoint(const uint8_t* e) noexcept {
    return readU32LE(e + CP_OFF);
}

} // anonymous

bool GlyphPack::loadFile(const std::string& path) {
    data_.clear();
    entries_ = nullptr;
    glyph_count_ = 0;

    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) return false;
    const std::streamsize size = f.tellg();
    if (size < static_cast<std::streamsize>(HEADER_SIZE)) return false;
    f.seekg(0, std::ios::beg);

    data_.resize(static_cast<std::size_t>(size));
    if (!f.read(reinterpret_cast<char*>(data_.data()), size)) {
        data_.clear();
        return false;
    }

    const uint8_t* p = data_.data();
    if (std::memcmp(p, MAGIC, 4) != 0) { data_.clear(); return false; }
    if (p[4] != VERSION)               { data_.clear(); return false; }
    if (p[5] != CELL_WIDTH)            { data_.clear(); return false; }
    if (p[6] != CELL_HEIGHT)           { data_.clear(); return false; }
    if (p[7] != SLOT_BYTES)            { data_.clear(); return false; }

    const uint32_t n = readU32LE(p + 8);
    const std::size_t need = HEADER_SIZE + static_cast<std::size_t>(n) * SLOT_BYTES;
    if (static_cast<std::size_t>(size) < need) { data_.clear(); return false; }

    // Verify the ascending-codepoint invariant. Cheap (O(N)), and catches
    // a whole class of pack-writer bugs that would otherwise silently break
    // binary search in weird ways.
    const uint8_t* base = data_.data() + HEADER_SIZE;
    uint32_t prev = 0;
    for (uint32_t i = 0; i < n; ++i) {
        const uint32_t cp = entryCodepoint(base + static_cast<std::size_t>(i) * SLOT_BYTES);
        if (i > 0 && cp <= prev) { data_.clear(); return false; }
        prev = cp;
    }

    entries_ = base;
    glyph_count_ = n;
    return true;
}

int GlyphPack::cellWidth(uint32_t codepoint) {
    return render(codepoint).cell_width;
}

Rendered GlyphPack::render(uint32_t codepoint) {
    Rendered r;
    if (glyph_count_ == 0) return r;

    // Binary search over the sorted codepoint column. We compare by reading
    // the u32 at each slot's start.
    std::size_t lo = 0, hi = glyph_count_;
    while (lo < hi) {
        const std::size_t mid = lo + (hi - lo) / 2;
        const uint32_t mid_cp = entryCodepoint(entries_ + mid * SLOT_BYTES);
        if (mid_cp == codepoint) {
            const uint8_t* e = entries_ + mid * SLOT_BYTES;
            r.cell_width = e[CW_OFF];
            std::memcpy(r.left.data(),  e + LEFT_OFF,  Glyph::Bitmap{}.size());
            std::memcpy(r.right.data(), e + RIGHT_OFF, Glyph::Bitmap{}.size());
            return r;
        }
        if (mid_cp < codepoint) lo = mid + 1;
        else                    hi = mid;
    }
    return r;      // not found → cell_width stays 0
}

} // namespace Font
} // namespace LCD2USB
