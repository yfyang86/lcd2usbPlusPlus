/*
 * lcd2usb/utf8.hpp - Minimal UTF-8 decoder for LCD2USB++
 *
 * We don't need a full ICU-grade decoder. Display text is tiny, arrives in
 * small batches, and the only thing that matters is that we turn a byte
 * sequence into a stream of codepoints without crashing on malformed input.
 *
 * Design goals:
 *   - Zero allocation, no exceptions.
 *   - Reject overlong encodings and UTF-16 surrogates.
 *   - Replace any bad sequence with U+FFFD and resync at the next valid
 *     start byte, so one corrupt byte never consumes a whole string.
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

namespace LCD2USB {
namespace Utf8 {

// The canonical replacement character. Used both as the decoder's substitute
// for invalid sequences, and (upstream) as the default "unknown glyph" marker.
constexpr uint32_t REPLACEMENT = 0xFFFDu;

/**
 * Decode one codepoint from [p, end).
 *
 * On success, advances *p past the consumed bytes and returns the codepoint.
 * On failure (truncated / overlong / surrogate / invalid start byte), advances
 * *p by exactly one byte and returns REPLACEMENT — the caller continues the
 * loop and we resync at the next start byte. At end-of-input returns 0 and
 * does not advance.
 */
uint32_t decode_next(const char** p, const char* end) noexcept;

/**
 * Stream adapter — iterate codepoints of a string_view in a range-for loop:
 *
 *   for (uint32_t cp : Utf8::Iter(text)) { ... }
 */
class Iter {
public:
    explicit Iter(std::string_view s) noexcept : begin_(s.data()), end_(s.data() + s.size()) {}

    class Cursor {
    public:
        Cursor(const char* p, const char* end) noexcept : p_(p), end_(end), cp_(0) { advance(); }

        uint32_t operator*() const noexcept { return cp_; }
        Cursor& operator++() noexcept { advance(); return *this; }
        bool operator!=(const Cursor& other) const noexcept {
            // A range ends when we run out of bytes AND the current codepoint
            // has been consumed (cp_ == 0 sentinel).
            return !(p_ == other.p_ && cp_ == 0);
        }
    private:
        void advance() noexcept {
            if (p_ >= end_) { cp_ = 0; return; }
            cp_ = decode_next(&p_, end_);
        }
        const char* p_;
        const char* end_;
        uint32_t cp_;
    };

    Cursor begin() const noexcept { return Cursor(begin_, end_); }
    Cursor end()   const noexcept { return Cursor(end_, end_); }

private:
    const char* begin_;
    const char* end_;
};

/**
 * Count codepoints in a UTF-8 string. Useful for sanity checks. Malformed
 * bytes count as one codepoint each (REPLACEMENT).
 */
std::size_t count_codepoints(std::string_view s) noexcept;

} // namespace Utf8
} // namespace LCD2USB
