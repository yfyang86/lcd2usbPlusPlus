/*
 * storyteller_utf8.cpp - UTF-8 / multi-language text reader with bitmap font
 *
 * This is a UTF-8-aware rewrite of storyteller.cpp. The differences worth
 * understanding:
 *
 *   1. Text is processed as codepoints, not bytes. A Chinese glyph in
 *      ChillBitmap takes TWO LCD cells, and word-wrap / padding all count
 *      cells rather than bytes or codepoints.
 *
 *   2. Rendering goes through UnicodeDisplay + FontAdapter, so each frame
 *      is a logical framebuffer update followed by one ud.flush() — the
 *      diffing layer pushes only cells that changed. On a scrolling story,
 *      that's one row's worth of changed cells per tick, not a full repaint.
 *
 *   3. The progress bar is implemented as five procedural bitmap glyphs
 *      registered in the glyph table at Private-Use Area codepoints
 *      U+F000..U+F004. From UnicodeDisplay's point of view they're just
 *      bitmap codepoints — it caches them in CGRAM like any other.
 *
 *   4. ASCII still uses HD44780 ROM by default (FontAdapter::setLatinFromFont
 *      is false). With a 7-slot CGRAM budget and many Latin words having
 *      7+ unique letters, bitmap-font ASCII would constantly thrash the
 *      cache. Pass --bitmap-latin to opt in anyway.
 *
 * Usage:
 *   storyteller_utf8 [--file path] [--width N] [--rows N] [--speed N]
 *                    [--font path.ttf] [--font-size px] [--bitmap-latin]
 */

#include "lcd2usb/lcd2usb_lib.hpp"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace LCD2USB;

// ─────────────────────────────────────────────────────────────────────────────
// Globals — Ctrl-C handling lives here so the main loop can break cleanly.
// ─────────────────────────────────────────────────────────────────────────────
namespace {
volatile sig_atomic_t g_running = 1;
void onSigint(int) { g_running = 0; }

// Private Use Area codepoints we commandeer for the progress-bar frames.
// Anything in U+F000..U+F8FF is guaranteed never to conflict with real text.
constexpr uint32_t PROGRESS_GLYPH_BASE = 0xF000;   // 6 levels: F000..F005
constexpr int      PROGRESS_LEVELS     = 6;        // 0 (empty) .. 5 (full)
} // anonymous

// ─────────────────────────────────────────────────────────────────────────────
// UTF-8 helpers — we need a few small utilities on top of Utf8::decode_next
// for line-break decisions. Keeping them local so the library stays minimal.
// ─────────────────────────────────────────────────────────────────────────────
namespace ut {

// Is a codepoint a "soft break point" — somewhere we're happy to wrap?
// Includes ASCII whitespace AND CJK ideographs (since CJK conventionally
// allows break between any two characters).
inline bool isBreak(uint32_t cp) {
    if (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r') return true;
    // CJK unified ideographs + extensions we care about:
    if (cp >= 0x3040 && cp <= 0x30FF) return true;   // Hiragana/Katakana
    if (cp >= 0x4E00 && cp <= 0x9FFF) return true;   // CJK Unified
    if (cp >= 0x3400 && cp <= 0x4DBF) return true;   // CJK Ext A
    if (cp >= 0x20000 && cp <= 0x2A6DF) return true; // CJK Ext B
    return false;
}

// Encode a single codepoint back to UTF-8 bytes (for building display strings).
inline void appendUtf8(std::string& out, uint32_t cp) {
    if (cp < 0x80)         { out.push_back(static_cast<char>(cp)); }
    else if (cp < 0x800)   { out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                             out.push_back(static_cast<char>(0x80 | (cp & 0x3F))); }
    else if (cp < 0x10000) { out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                             out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                             out.push_back(static_cast<char>(0x80 | (cp & 0x3F))); }
    else                   { out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                             out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                             out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                             out.push_back(static_cast<char>(0x80 | (cp & 0x3F))); }
}

} // namespace ut

// ─────────────────────────────────────────────────────────────────────────────
// Progress bar — five procedural 5x8 bitmaps registered at PUA codepoints.
// ─────────────────────────────────────────────────────────────────────────────
namespace {

Glyph::Bitmap progressFrame(int level /* 0..5 */) {
    level = std::clamp(level, 0, PROGRESS_LEVELS - 1);
    // A 5x8 cell with a 1-col frame on each side and vertical fill between.
    // Level 0 = just the borders. Level 5 = fully solid (used as end cap).
    Glyph::Bitmap bm{};
    for (int y = 0; y < 8; ++y) {
        uint8_t v = 0b10001;   // left + right border
        switch (level) {
            case 0: break;
            case 1: v |= 0b01000; break;                  // 1 fill column (from left)
            case 2: v |= 0b01100; break;                  // 2 fill cols
            case 3: v |= 0b01110; break;                  // 3 fill cols
            case 4: v |= 0b01111; break;                  // fills out to border
            case 5: v  = 0b11111; break;                  // solid (100%)
        }
        bm[y] = v;
    }
    return bm;
}

void registerProgressGlyphs(UnicodeDisplay& ud) {
    for (int i = 0; i < PROGRESS_LEVELS; ++i) {
        ud.table().addBitmap(PROGRESS_GLYPH_BASE + i, progressFrame(i));
    }
}

// Emit a single progress codepoint at (col, row). Caller flushes.
void drawProgress(UnicodeDisplay& ud, int col, int row, double progress) {
    progress = std::clamp(progress, 0.0, 1.0);
    const int level = static_cast<int>(progress * (PROGRESS_LEVELS - 1) + 0.5);
    ud.setCursor(col, row);
    const uint32_t cp = PROGRESS_GLYPH_BASE + level;
    ud.printCodepoints(&cp, 1);
}

} // anonymous

// ─────────────────────────────────────────────────────────────────────────────
// UTF-8 word wrapper — splits text into display lines that each fit in the
// given cell budget. Cell budget is not codepoint count: CJK counts as 2.
// ─────────────────────────────────────────────────────────────────────────────

class Wrapper {
public:
    Wrapper(Font::FontAdapter& adapter, int cells_per_line)
        : adapter_(adapter), cells_(cells_per_line) {}

    // Feed a UTF-8 chunk; appended lines are ready to display.
    void feed(std::string_view utf8) {
        const char* p   = utf8.data();
        const char* end = p + utf8.size();
        while (p < end) {
            uint32_t cp = Utf8::decode_next(&p, end);
            if (cp == 0) break;
            consume(cp);
        }
    }

    // Finish any in-progress line. Call once after the last feed().
    void finish() {
        flushWord();
        if (!line_.empty() || line_width_ > 0) {
            pushLine();
        }
    }

    // Pop the oldest completed line, or "" if none.
    std::string takeLine() {
        if (lines_.empty()) return {};
        std::string s = std::move(lines_.front());
        lines_.pop_front();
        return s;
    }

    bool hasLine() const { return !lines_.empty(); }
    std::size_t pendingLines() const { return lines_.size(); }

private:
    Font::FontAdapter& adapter_;
    int                cells_;          // cells per line

    // "Line in progress" state (already committed to the current line).
    std::string line_;
    int         line_width_ = 0;

    // "Word in progress" — buffered until we see a break point, then flushed
    // to line_. This is what lets us wrap on word boundaries for ASCII.
    std::string word_;
    int         word_width_ = 0;

    std::deque<std::string> lines_;

    void consume(uint32_t cp) {
        if (cp == '\n') {
            flushWord();
            pushLine();
            return;
        }
        if (cp < 0x20) return;           // skip other controls

        const int w = adapter_.measureCells(std::string(1, static_cast<char>(cp < 0x80 ? cp : '?')));
        // That measured an ASCII char for non-ASCII — redo it properly:
        std::string tmp;
        ut::appendUtf8(tmp, cp);
        const int cp_w = adapter_.measureCells(tmp);

        (void)w;   // we only keep cp_w

        if (cp == ' ' || cp == '\t') {
            flushWord();
            // A space fits unless it'd push the line over width — then wrap.
            if (line_width_ + cp_w > cells_) {
                pushLine();
            }
            if (line_width_ > 0 || cp != ' ') {           // skip leading space on new line
                line_ += ' ';
                line_width_ += cp_w;
            }
            return;
        }

        // For CJK codepoints (each one is its own break opportunity), flush
        // any buffered word then immediately commit this char. That matches
        // the typographic rule that CJK doesn't need word boundaries.
        if (ut::isBreak(cp)) {
            flushWord();
            if (line_width_ + cp_w > cells_) pushLine();
            ut::appendUtf8(line_, cp);
            line_width_ += cp_w;
            return;
        }

        // Ordinary Latin letter / digit / punct — accumulate into word_.
        // If the word alone exceeds the line budget, break mid-word.
        if (word_width_ + cp_w > cells_) {
            // Word is too long to fit on any line; emit what we have and
            // start the word fresh with this codepoint.
            flushWord();
            pushLine();
        }
        ut::appendUtf8(word_, cp);
        word_width_ += cp_w;
    }

    void flushWord() {
        if (word_width_ == 0) return;
        if (line_width_ + word_width_ > cells_) {
            // Word doesn't fit on current line — push line, start fresh.
            pushLine();
        }
        line_ += word_;
        line_width_ += word_width_;
        word_.clear();
        word_width_ = 0;
    }

    void pushLine() {
        // Trim trailing space — looks neater on the LCD.
        while (!line_.empty() && line_.back() == ' ') {
            line_.pop_back();
            --line_width_;
        }
        lines_.emplace_back(std::move(line_));
        line_.clear();
        line_width_ = 0;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// The storyteller itself.
// ─────────────────────────────────────────────────────────────────────────────

struct Config {
    std::string file       = "assets/story_utf8.txt";
    std::string font_path  = "assets/boutique7x7.glyphs";   // pack default
    int         screen_w   = 20;
    int         screen_h   = 4;
    int         scroll_sec = 2;
    int         font_size  = 9;     // only applied when font_path ends in .ttf
    bool        bitmap_latin = false;
    bool        no_scrollbar = false;
};

// Polymorphic font-source loading. `.glyphs` → GlyphPack (fast, no FT dep).
// Anything else (.ttf, .otf) → BitmapFont via FreeType.
static bool isGlyphsPack(const std::string& p) {
    return p.size() >= 7 && p.compare(p.size() - 7, 7, ".glyphs") == 0;
}

class Storyteller {
public:
    Storyteller(LCDDisplay& lcd, const Config& cfg)
        : cfg_(cfg),
          ud_(lcd, cfg.screen_w, cfg.screen_h) {}

    bool initialize() {
        if (isGlyphsPack(cfg_.font_path)) {
            auto p = std::make_unique<Font::GlyphPack>();
            if (!p->loadFile(cfg_.font_path)) {
                std::cerr << "Failed to load .glyphs pack: " << cfg_.font_path << "\n";
                return false;
            }
            std::cout << "[font] pack: " << cfg_.font_path
                      << " (" << p->glyphCount() << " glyphs)\n";
            font_ = std::move(p);
        } else {
            auto f = std::make_unique<Font::BitmapFont>();
            if (!f->loadTTF(cfg_.font_path, cfg_.font_size)) {
                std::cerr << "Failed to load TTF: " << cfg_.font_path << "\n";
                return false;
            }
            std::cout << "[font] TTF: " << cfg_.font_path
                      << " @ " << cfg_.font_size << "px\n";
            font_ = std::move(f);
        }

        adapter_ = std::make_unique<Font::FontAdapter>(*font_, ud_);
        adapter_->setLatinFromFont(cfg_.bitmap_latin);
        if (!cfg_.no_scrollbar) {
            registerProgressGlyphs(ud_);
        }
        return true;
    }

    bool loadFile() {
        std::ifstream file(cfg_.file, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Cannot open: " << cfg_.file << "\n";
            return false;
        }
        total_bytes_ = static_cast<std::size_t>(file.tellg());
        file.seekg(0, std::ios::beg);
        text_.assign(std::istreambuf_iterator<char>(file), {});
        std::cout << "Loaded " << total_bytes_ << " bytes from " << cfg_.file << "\n";
        return true;
    }

    void run() {
        // When the scrollbar is enabled the rightmost column is reserved for
        // it and the wrapper only gets `screen_w - 1` cells of text space.
        // When disabled, text owns the full width.
        const int text_w = cfg_.no_scrollbar ? cfg_.screen_w : cfg_.screen_w - 1;

        Wrapper wrapper(*adapter_, text_w);
        wrapper.feed(text_);
        wrapper.finish();
        const std::size_t total_lines = wrapper.pendingLines();
        if (total_lines == 0) {
            std::cerr << "No displayable text in file.\n";
            return;
        }

        std::vector<std::string> rows(cfg_.screen_h);
        std::size_t lines_shown = 0;

        while (g_running && wrapper.hasLine()) {
            for (int r = 0; r + 1 < cfg_.screen_h; ++r) rows[r] = rows[r + 1];
            rows.back() = wrapper.takeLine();
            ++lines_shown;

            ud_.clear();
            for (int r = 0; r < cfg_.screen_h; ++r) {
                adapter_->printAt(0, r, rows[r]);
            }

            if (!cfg_.no_scrollbar) {
                const double done = static_cast<double>(lines_shown) / static_cast<double>(total_lines);
                for (int r = 0; r < cfg_.screen_h; ++r) {
                    drawProgress(ud_, cfg_.screen_w - 1, r, done);
                }
            }

            // One USB burst. The diff inside flush() makes this cheap: most
            // rows are shifted copies of what was just displayed, so only
            // the newly-arrived bottom row and the progress column move
            // across the bus.
            (void)ud_.flush();

            std::this_thread::sleep_for(std::chrono::seconds(cfg_.scroll_sec));
        }

        ud_.clear();
        if (!g_running) {
            adapter_->printAt(0, 0, "Stopped.");
        } else {
            adapter_->printAt(0, 0, "Story complete");
            if (cfg_.screen_h > 1) adapter_->printAt(0, 1, "Ctrl+C to exit");
        }
        if (!cfg_.no_scrollbar) {
            for (int r = 0; r < cfg_.screen_h; ++r) {
                drawProgress(ud_, cfg_.screen_w - 1, r, 1.0);
            }
        }
        ud_.flush();
    }

private:
    Config                             cfg_;
    UnicodeDisplay                     ud_;
    std::unique_ptr<Font::GlyphSource> font_;      // BitmapFont or GlyphPack
    std::unique_ptr<Font::FontAdapter> adapter_;
    std::string                        text_;
    std::size_t                        total_bytes_ = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// CLI
// ─────────────────────────────────────────────────────────────────────────────

static void printUsage(const char* prog) {
    std::cout <<
        "Usage: " << prog << " [options]\n"
        "  --file <path>        Text file to display (UTF-8).          [assets/story_utf8.txt]\n"
        "  --font <path>        Font source — .glyphs pack or .ttf.    [assets/boutique7x7.glyphs]\n"
        "                       .glyphs packs (from tools/ufo_to_glyphs.py) load faster\n"
        "                       and require no FreeType at runtime.\n"
        "  --font-size <px>     Pixel size (TTF only; ignored for .glyphs). [9]\n"
        "  --width <n>          Screen width in cells (4..20).         [20]\n"
        "  --rows <n>           Screen height in rows (1..4).          [4]\n"
        "  --speed <sec>        Seconds per scroll tick (>=1).         [2]\n"
        "  --bitmap-latin       Also render ASCII from the font (costs CGRAM slots).\n"
        "  --disable-scrollbar  Use the full display width for text; hide the progress bar.\n"
        "  --help               Show this help.\n";
}

int main(int argc, char** argv) {
    Config cfg;

    static const struct option long_opts[] = {
        {"file",              required_argument, nullptr, 'f'},
        {"font",              required_argument, nullptr, 'F'},
        {"font-size",         required_argument, nullptr, 's'},
        {"width",             required_argument, nullptr, 'w'},
        {"rows",              required_argument, nullptr, 'r'},
        {"speed",             required_argument, nullptr, 'p'},
        {"bitmap-latin",      no_argument,       nullptr, 'b'},
        {"disable-scrollbar", no_argument,       nullptr, 'S'},
        {"help",              no_argument,       nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "f:F:s:w:r:p:bSh", long_opts, nullptr)) != -1) {
        switch (opt) {
            case 'f': cfg.file      = optarg; break;
            case 'F': cfg.font_path = optarg; break;
            case 's': cfg.font_size = std::clamp(std::atoi(optarg), 6, 16); break;
            case 'w': cfg.screen_w  = std::clamp(std::atoi(optarg), 4, 20); break;
            case 'r': cfg.screen_h  = std::clamp(std::atoi(optarg), 1, 4);  break;
            case 'p': cfg.scroll_sec= std::max(1, std::atoi(optarg));       break;
            case 'b': cfg.bitmap_latin = true; break;
            case 'S': cfg.no_scrollbar = true; break;
            case 'h': printUsage(argv[0]); return 0;
            default:  printUsage(argv[0]); return 1;
        }
    }

    std::signal(SIGINT, onSigint);

    std::cout << "=== LCD2USB++ UTF-8 Storyteller ===\n"
              << "  file: "         << cfg.file       << "\n"
              << "  font: "         << cfg.font_path
              << " @ "               << cfg.font_size  << "px\n"
              << "  display: "      << cfg.screen_w   << "x" << cfg.screen_h << "\n"
              << "  scroll speed: " << cfg.scroll_sec << "s\n"
              << "  latin from font: " << (cfg.bitmap_latin ? "yes" : "no (ROM)") << "\n"
              << "  scrollbar: "    << (cfg.no_scrollbar ? "off (full-width text)" : "on") << "\n";

    LCDDisplay lcd;
    if (!lcd.initialize()) {
        std::cerr << "LCD init failed\n";
        return 1;
    }
    lcd.setContrast(200);
    lcd.setBrightness(255);
    lcd.clear();

    Storyteller st(lcd, cfg);
    if (!st.initialize()) return 1;
    if (!st.loadFile())   return 1;
    st.run();

    return 0;
}
