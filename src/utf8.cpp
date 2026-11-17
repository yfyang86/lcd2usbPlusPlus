/*
 * utf8.cpp - UTF-8 decoder implementation
 */

#include "lcd2usb/utf8.hpp"

namespace LCD2USB {
namespace Utf8 {

namespace {
// Is this byte a valid UTF-8 continuation byte (10xxxxxx)?
inline bool is_cont(unsigned char b) noexcept {
    return (b & 0xC0u) == 0x80u;
}
} // anonymous

uint32_t decode_next(const char** pp, const char* end) noexcept {
    const char* p = *pp;
    if (p >= end) return 0;

    const unsigned char b0 = static_cast<unsigned char>(*p);

    // ASCII fast path — the common case on an LCD: letters, digits, spaces.
    if (b0 < 0x80u) {
        *pp = p + 1;
        return b0;
    }

    // Continuation byte without a preceding lead byte → invalid. Resync.
    if (b0 < 0xC2u) {                   // also catches overlong 2-byte starts
        *pp = p + 1;
        return REPLACEMENT;
    }

    // 2-byte sequence: 110xxxxx 10xxxxxx  (U+0080 .. U+07FF)
    if (b0 < 0xE0u) {
        if (p + 1 >= end) { *pp = p + 1; return REPLACEMENT; }
        const unsigned char b1 = static_cast<unsigned char>(p[1]);
        if (!is_cont(b1)) { *pp = p + 1; return REPLACEMENT; }
        *pp = p + 2;
        return (uint32_t(b0 & 0x1Fu) << 6) | uint32_t(b1 & 0x3Fu);
    }

    // 3-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx  (U+0800 .. U+FFFF)
    if (b0 < 0xF0u) {
        if (p + 2 >= end) { *pp = p + 1; return REPLACEMENT; }
        const unsigned char b1 = static_cast<unsigned char>(p[1]);
        const unsigned char b2 = static_cast<unsigned char>(p[2]);
        if (!is_cont(b1) || !is_cont(b2)) { *pp = p + 1; return REPLACEMENT; }

        // Reject overlong encodings (anything < 0x800 packed in 3 bytes).
        if (b0 == 0xE0u && b1 < 0xA0u)     { *pp = p + 1; return REPLACEMENT; }
        // Reject UTF-16 surrogates U+D800..U+DFFF (3-byte range ED A0..ED BF).
        if (b0 == 0xEDu && b1 >= 0xA0u)    { *pp = p + 1; return REPLACEMENT; }

        *pp = p + 3;
        return (uint32_t(b0 & 0x0Fu) << 12)
             | (uint32_t(b1 & 0x3Fu) << 6)
             |  uint32_t(b2 & 0x3Fu);
    }

    // 4-byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (U+10000 .. U+10FFFF)
    if (b0 < 0xF5u) {
        if (p + 3 >= end) { *pp = p + 1; return REPLACEMENT; }
        const unsigned char b1 = static_cast<unsigned char>(p[1]);
        const unsigned char b2 = static_cast<unsigned char>(p[2]);
        const unsigned char b3 = static_cast<unsigned char>(p[3]);
        if (!is_cont(b1) || !is_cont(b2) || !is_cont(b3)) {
            *pp = p + 1; return REPLACEMENT;
        }
        // Overlong / out-of-range guards.
        if (b0 == 0xF0u && b1 < 0x90u)     { *pp = p + 1; return REPLACEMENT; }
        if (b0 == 0xF4u && b1 >= 0x90u)    { *pp = p + 1; return REPLACEMENT; }

        *pp = p + 4;
        return (uint32_t(b0 & 0x07u) << 18)
             | (uint32_t(b1 & 0x3Fu) << 12)
             | (uint32_t(b2 & 0x3Fu) << 6)
             |  uint32_t(b3 & 0x3Fu);
    }

    // 0xF5..0xFF are never valid as lead bytes in UTF-8.
    *pp = p + 1;
    return REPLACEMENT;
}

std::size_t count_codepoints(std::string_view s) noexcept {
    std::size_t n = 0;
    const char* p = s.data();
    const char* end = p + s.size();
    while (p < end) {
        decode_next(&p, end);
        ++n;
    }
    return n;
}

} // namespace Utf8
} // namespace LCD2USB
