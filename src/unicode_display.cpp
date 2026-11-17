/*
 * unicode_display.cpp - Shadow framebuffer, CGRAM LRU, run-coalesced diff
 *
 * See unicode_display.hpp for the big picture. The heart of this file is
 * flush(), which performs the following in order:
 *
 *   (a) Scan the next frame and collect the set of codepoints that need a
 *       CGRAM slot (Bitmap resolutions). Codepoints already loaded in a slot
 *       get their slot re-used; new ones displace the least-recently-used
 *       slot whose codepoint is NOT needed in the current frame.
 *       Codepoints that can't be fit (more than 7 distinct bitmap codepoints
 *       on screen at once) are downgraded to the replacement char.
 *
 *   (b) Upload any dirty CGRAM slots. Each slot upload is ~3 USB transfers
 *       regardless of how many cells on screen use it, so caching pays off
 *       every time a codepoint survives between frames.
 *
 *   (c) Diff the desired byte grid against the shadow. Coalesce changed
 *       cells into contiguous same-row runs and push each run with a single
 *       setCursor + printBytes pair. printBytes packs up to BUFFER_MAX_CMD
 *       bytes per USB transfer.
 */

#include "lcd2usb/unicode_display.hpp"
#include "lcd2usb/utf8.hpp"

#include <algorithm>
#include <unordered_map>

namespace LCD2USB {

UnicodeDisplay::UnicodeDisplay(LCDDisplay& lcd, int width, int height, Glyph::GlyphTable table)
    : lcd_(lcd),
      width_(width),
      height_(height),
      table_(std::move(table))
{
    const std::size_t n = static_cast<std::size_t>(width_) * height_;
    desired_.assign(n, BLANK);
    shown_cp_.assign(n, BLANK);
    shown_byte_.assign(n, ' ');
    // We assume the LCDDisplay::clear() that happens before the first flush
    // leaves the LCD full of spaces. The existing codebase already clears in
    // initialize()/clear(), so this matches reality. invalidate() is offered
    // for users who need to force a full re-send.
}

void UnicodeDisplay::clear() {
    std::fill(desired_.begin(), desired_.end(), BLANK);
    cursor_col_ = 0;
    cursor_row_ = 0;
}

void UnicodeDisplay::setCursor(int col, int row) {
    cursor_col_ = std::clamp(col, 0, width_ - 1);
    cursor_row_ = std::clamp(row, 0, height_ - 1);
}

void UnicodeDisplay::print(std::string_view utf8) {
    const char* p   = utf8.data();
    const char* end = p + utf8.size();
    while (p < end) {
        const uint32_t cp = Utf8::decode_next(&p, end);
        if (cp == 0) break;

        // Newline moves to next line's column 0. Tabs and other controls are
        // treated as a single blank cell — good enough for an LCD.
        if (cp == '\n') {
            cursor_col_ = 0;
            if (cursor_row_ + 1 < height_) ++cursor_row_;
            continue;
        }
        if (cursor_col_ >= width_) {
            // Soft wrap to the next row. If we're on the last row, stop —
            // we don't want surprise scrolling on a 4-line display.
            if (cursor_row_ + 1 >= height_) break;
            cursor_col_ = 0;
            ++cursor_row_;
        }
        desired_[cellIndex(cursor_col_, cursor_row_)] = cp;
        ++cursor_col_;
    }
}

void UnicodeDisplay::printAt(int col, int row, std::string_view utf8) {
    setCursor(col, row);
    print(utf8);
}

void UnicodeDisplay::printCodepoints(const uint32_t* cps, std::size_t n) {
    // Raw codepoint emission. Each codepoint occupies exactly one cell — no
    // width introspection happens here. The caller (e.g. FontAdapter) is the
    // one that knows wide glyphs and is expected to emit a continuation
    // codepoint immediately after a wide left-half.
    for (std::size_t i = 0; i < n; ++i) {
        const uint32_t cp = cps[i];
        if (cp == '\n') {
            cursor_col_ = 0;
            if (cursor_row_ + 1 < height_) ++cursor_row_;
            continue;
        }
        if (cursor_col_ >= width_) {
            if (cursor_row_ + 1 >= height_) break;
            cursor_col_ = 0;
            ++cursor_row_;
        }
        desired_[cellIndex(cursor_col_, cursor_row_)] = cp;
        ++cursor_col_;
    }
}

void UnicodeDisplay::println(int row, std::string_view utf8) {
    if (row < 0 || row >= height_) return;
    // Clear the row first (in the framebuffer only — actual bytes flow at flush).
    for (int c = 0; c < width_; ++c) desired_[cellIndex(c, row)] = BLANK;
    setCursor(0, row);
    print(utf8);
}

void UnicodeDisplay::invalidate() {
    // Pretend the LCD holds an impossible sentinel in every cell so the next
    // flush rewrites everything. Also force all CGRAM slots dirty.
    std::fill(shown_cp_.begin(),   shown_cp_.end(),   0xFFFFFFFFu);
    std::fill(shown_byte_.begin(), shown_byte_.end(), 0xFF);
    for (auto& s : slots_) s.dirty = s.occupied;
}

int UnicodeDisplay::cgramOccupancy() const {
    int n = 0;
    for (const auto& s : slots_) if (s.occupied) ++n;
    return n;
}

// ─────────────────────────────────────────────────────────────────────────────
// Slot management
// ─────────────────────────────────────────────────────────────────────────────

void UnicodeDisplay::lockSlot(int slot) {
    slots_[slot].locked_this_frame = true;
    slots_[slot].last_used = ++tick_;
}

int UnicodeDisplay::allocateSlot(uint32_t codepoint, const Glyph::Bitmap& bitmap) {
    // 1. Already loaded?
    for (int i = LCD::FIRST_USABLE_SLOT; i < LCD::MAX_CUSTOM_CHARS; ++i) {
        if (slots_[i].occupied && slots_[i].codepoint == codepoint) {
            slots_[i].locked_this_frame = true;
            slots_[i].last_used = ++tick_;
            // Re-upload if the bitmap was changed via table_ at runtime.
            if (slots_[i].bitmap != bitmap) {
                slots_[i].bitmap = bitmap;
                slots_[i].dirty = true;
            }
            return i;
        }
    }
    // 2. Any free slot?
    for (int i = LCD::FIRST_USABLE_SLOT; i < LCD::MAX_CUSTOM_CHARS; ++i) {
        if (!slots_[i].occupied) {
            slots_[i].occupied = true;
            slots_[i].codepoint = codepoint;
            slots_[i].bitmap = bitmap;
            slots_[i].dirty = true;
            slots_[i].locked_this_frame = true;
            slots_[i].last_used = ++tick_;
            return i;
        }
    }
    // 3. Evict LRU among *unlocked* slots (locked = needed in this frame).
    int victim = -1;
    uint64_t oldest = UINT64_MAX;
    for (int i = LCD::FIRST_USABLE_SLOT; i < LCD::MAX_CUSTOM_CHARS; ++i) {
        if (!slots_[i].locked_this_frame && slots_[i].last_used < oldest) {
            oldest = slots_[i].last_used;
            victim = i;
        }
    }
    if (victim < 0) {
        // Every slot is in use by *this frame* already. Caller must fall back
        // to the replacement char.
        return -1;
    }
    slots_[victim].codepoint = codepoint;
    slots_[victim].bitmap = bitmap;
    slots_[victim].dirty = true;
    slots_[victim].locked_this_frame = true;
    slots_[victim].last_used = ++tick_;
    // Mark every on-screen cell that used to reference this slot for
    // rewriting. Without this, cells whose codepoint is still on screen but
    // whose slot was just stolen would show the WRONG bitmap until they
    // happen to be rewritten for other reasons.
    for (std::size_t k = 0; k < shown_byte_.size(); ++k) {
        if (shown_byte_[k] == static_cast<uint8_t>(victim)) {
            shown_byte_[k] = 0xFF;     // force a diff
            shown_cp_[k]   = 0xFFFFFFFFu;
        }
    }
    return victim;
}

// ─────────────────────────────────────────────────────────────────────────────
// Flush: the whole dance
// ─────────────────────────────────────────────────────────────────────────────

int UnicodeDisplay::flush() {
    // Reset per-frame locks.
    for (auto& s : slots_) s.locked_this_frame = false;

    const std::size_t n = desired_.size();

    // ── Pass 1: pin codepoints that are already resident in CGRAM ──────────
    // This pass intentionally *only* looks at bitmap codepoints whose glyph
    // is already loaded. That way, pass 2's allocator has the correct set of
    // "still needed" slots and won't evict a glyph we're about to use.
    for (std::size_t i = 0; i < n; ++i) {
        const uint32_t cp = desired_[i];
        if (cp < 0x80u) continue;  // ASCII fast path — no table / no slots.
        const auto r = table_.resolve(cp);
        if (r.kind != Glyph::Kind::Bitmap) continue;
        for (int s = LCD::FIRST_USABLE_SLOT; s < LCD::MAX_CUSTOM_CHARS; ++s) {
            if (slots_[s].occupied && slots_[s].codepoint == cp) {
                lockSlot(s);
                // If the table's bitmap for this codepoint was changed at
                // runtime, record that so pass 2 will re-upload it.
                if (slots_[s].bitmap != r.bitmap) {
                    slots_[s].bitmap = r.bitmap;
                    slots_[s].dirty  = true;
                }
                break;
            }
        }
    }

    // ── Pass 2: build the target byte grid, allocating new slots as needed ──
    std::vector<uint8_t> target(n, ' ');
    for (std::size_t i = 0; i < n; ++i) {
        target[i] = static_cast<uint8_t>(resolveCell(desired_[i]));
    }

    // ── Pass 3: upload any dirty CGRAM slots ───────────────────────────────
    // Each uploadCustomCharacter returns the LCD to DDRAM mode internally,
    // so it's safe to interleave with DDRAM writes — but we batch uploads
    // first to keep the DDRAM cursor positioning predictable.
    int cells_written = 0;
    for (int s = LCD::FIRST_USABLE_SLOT; s < LCD::MAX_CUSTOM_CHARS; ++s) {
        if (slots_[s].occupied && slots_[s].dirty) {
            lcd_.uploadCustomCharacter(s, slots_[s].bitmap);
            slots_[s].dirty = false;
        }
    }

    // ── Pass 4: diff against the shadow and push coalesced runs ────────────
    // A "run" is a maximal sequence of consecutive same-row cells whose byte
    // differs from shown_byte_. Each run costs 1 setCursor plus ceil(len/4)
    // data transfers (BUFFER_MAX_CMD = 4 bytes per control msg).
    for (int row = 0; row < height_; ++row) {
        int col = 0;
        while (col < width_) {
            const int idx = cellIndex(col, row);
            if (target[idx] == shown_byte_[idx]) { ++col; continue; }

            // Extend run.
            const int start = col;
            std::vector<uint8_t> payload;
            payload.reserve(width_);
            while (col < width_) {
                const int k = cellIndex(col, row);
                if (target[k] == shown_byte_[k]) break;
                payload.push_back(target[k]);
                ++col;
            }

            lcd_.setCursor(start, row);
            lcd_.printBytes(payload.data(), payload.size());

            // Commit to shadow.
            for (int c = start; c < col; ++c) {
                const int k = cellIndex(c, row);
                shown_byte_[k] = target[k];
                shown_cp_[k]   = desired_[k];
            }
            cells_written += static_cast<int>(payload.size());
        }
    }

    return cells_written;
}

int UnicodeDisplay::resolveCell(uint32_t cp) {
    // Fast path: plain ASCII.
    if (cp >= 0x20u && cp <= 0x7Eu) return static_cast<int>(cp);
    // Treat other low-ASCII (NUL, control codes) as spaces to keep the grid
    // sane — an LCD has no concept of "control character".
    if (cp < 0x20u) return ' ';

    const auto r = table_.resolve(cp);
    switch (r.kind) {
        case Glyph::Kind::Rom:
            return r.rom_byte;

        case Glyph::Kind::Bitmap: {
            const int slot = allocateSlot(cp, r.bitmap);
            if (slot >= 0) return slot;
            // Out of slots for this frame — fall back to replacement char.
            return replacement_;
        }

        case Glyph::Kind::Missing:
        default:
            return replacement_;
    }
}

} // namespace LCD2USB
