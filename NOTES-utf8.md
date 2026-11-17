# LCD2USB++ Unicode Layer — Design Notes

This document explains the UTF-8 extension added on top of the existing
`LCDDisplay` byte-oriented API. Nothing in the original library was removed;
the new layer lives in three files:

- `include/lcd2usb/utf8.hpp`          — standalone UTF-8 decoder.
- `include/lcd2usb/glyph_table.hpp`   — codepoint → glyph registry.
- `include/lcd2usb/unicode_display.hpp`— shadow framebuffer + CGRAM LRU + diff.

Byte-level code (`lcd.print("hello")`, custom-character demos, the GIF player,
the font editor, …) is entirely unaffected.

## The hardware budget, stated plainly

Every write to the LCD is one libusb `usb_control_msg`. Each control message
carries up to **4 payload bytes** (the two 16-bit fields `wValue` and
`wIndex`). At USB low/full-speed with device-side processing, a realistic
upper bound is a few hundred transfers per second. So:

- Repainting the full 20×4 grid (80 cells) costs ≥ 20 data transfers +
  cursor positioning = 30–40 transfers = 60–150 ms.
- Uploading one 5×8 CGRAM glyph costs ≈ 3 transfers (CGRAM address,
  bitmap bytes, DDRAM-mode restore).
- The ROM has ASCII + an extended region (katakana on A00, Latin-1 on A02).
  It has **no accents**, **no arrows**, **no euro**, **no degree sign**.
- Only **7 usable CGRAM slots** at any one instant (the existing code avoids
  slot 0 due to an HD44780 quirk; we kept that).

There is therefore no way to "display any Unicode". The only useful
definition of UTF-8 support on this hardware is: *decode correctly, then be
smart about what to do with each codepoint.*

## Three-layer pipeline

```
        UTF-8 bytes
             │
             ▼                    [1] decoder  ──→  codepoint stream
        Utf8::decode_next
             │
             ▼                    [2] glyph table  ──→  per-codepoint plan
        GlyphTable::resolve
        ├── Rom(byte)              (passthrough; no CGRAM cost)
        ├── Bitmap(8×5 pixels)     (needs a CGRAM slot)
        └── Missing                (use replacement char)
             │
             ▼                    [3] shadow + diff
        UnicodeDisplay
        ├── desired_   : codepoints we want on screen
        ├── shown_byte_: bytes the LCD actually has
        └── slots_[0..7]: LRU-managed CGRAM cache
             │
             ▼
        flush() — one USB burst per call
```

### Why the layers are separate

You can use the decoder without the table (one of the examples in the
`multi_char_display` code path could parse UTF-8 filenames and never touch
an LCD).  You can use the glyph table without the shadow (a caller that
drives the LCD row-at-a-time for its own reasons just wants the
codepoint → (byte, bitmap) decision).  And you can swap `UnicodeDisplay`
for a different renderer (say, a test-mode console renderer) while keeping
the same table.

## The bandwidth tricks, in order of importance

### 1. Transliteration before CGRAM allocation

`é → e`, `ñ → n`, `« → "`, `– → -`, etc. Every such mapping is one
codepoint that costs **zero** CGRAM slots and folds straight into the
byte diff stream. Western European prose on this device is now free of
CGRAM pressure.

Visible in the test output:
```
[Test 2] Piña transliterated: 10 cells, 6 transfers, CGRAM=0
```

### 2. Shadow framebuffer + byte diff

`UnicodeDisplay` never talks to the LCD until you call `flush()`. At flush
time it compares the desired byte for each cell against a shadow of what's
currently on screen, and only sends the differences.

```
[Test 4] Unchanged re-flush: 0 cells, 0 transfers
[Test 5] Flip '8' → '9':     1 cells, 2 transfers
```

The difference this makes for animations is large: a seconds-counter
updating once a second moves from "repaint the whole row (80ms)" to "one
byte + cursor (4ms)".

### 3. Run coalescing

Contiguous changed cells on the same row are emitted as a single cursor
positioning followed by a single byte run packed into the 4-byte control-msg
buffer. Six changed cells in a row take **3 transfers**, not 12.

```
[Test 8] 6-cell run: 6 cells, 1 cursor + 1 data calls
```

### 4. CGRAM cache across frames

When a bitmap codepoint used in frame N is also needed in frame N+1, its
slot is kept — no re-upload. The allocator's two-pass design is the reason
this works reliably:

- **Pass 1** walks the next frame and locks slots for codepoints already
  resident in CGRAM.
- **Pass 2** allocates new slots for codepoints the table maps to bitmaps,
  evicting only from the set of *unlocked* (i.e. not needed this frame)
  slots.

If more than 7 distinct bitmap codepoints are needed simultaneously, the
excess gracefully fall back to the replacement char instead of corrupting
existing ones. Test 7 covers this case.

### 5. `printBytes` instead of `print(char)` in the hot path

The original `LCDDisplay::print(char)` flushes the buffer after every single
byte — fine for single-character writes, disastrous for runs. The added
`printBytes(data, len)` fills the 4-byte buffer to the brim before each
transfer. `UnicodeDisplay::flush` uses `printBytes` exclusively.

## The CGRAM LRU, in detail

Each of the 8 slots tracks:

```cpp
struct Slot {
    bool     occupied;            // holds a codepoint
    uint32_t codepoint;
    Bitmap   bitmap;              // 8 bytes
    bool     dirty;               // needs re-upload
    uint64_t last_used;           // monotonic tick
    bool     locked_this_frame;   // needed by the frame being flushed
};
```

Slot 0 is never used (hardware quirk). Slots 1..7 are the working set.

When a bitmap codepoint is resolved:

1. If the codepoint is already resident, its slot is returned (and the
   bitmap is re-uploaded only if the table's bitmap changed at runtime).
2. Else, if any slot is unoccupied, take it and mark dirty.
3. Else, evict the oldest `last_used` among slots **not locked this frame**
   and mark the new occupant dirty.
4. Else (every slot is locked by this frame), return `-1` → fall back to
   the replacement char.

When a slot is evicted, *every cell in the shadow that referenced that
slot's byte is marked for repaint*. Without this step, the stale cells
would display the wrong glyph until they happened to be rewritten.

## What was touched in the existing code

Minimal surface-area changes, all mechanical:

| File | Change |
|---|---|
| `include/lcd2usb/lcd2usb.hpp` | Added `ROW_ADDR` table, `FIRST_USABLE_SLOT`, `USABLE_SLOT_COUNT`. Comment on `BUFFER_MAX_CMD` explaining its role. |
| `include/lcd2usb/lcd_display.hpp` | Added `printBytes` declaration. |
| `include/lcd2usb/lcd2usb_lib.hpp` | Included the three new headers. |
| `src/lcd_display.cpp` | Removed duplicate `#include`. Replaced the switch statements in `writeToLine` and `setCursor` with `ROW_ADDR` lookups. Added `printBytes`. |
| `src/CMakeLists.txt` | Added the three new sources. |
| `examples/CMakeLists.txt` | Added `utf8_demo` target. |

Every existing example still compiles and runs. `font_editor`, `multi_char_display`,
and the GIF/BMP code never see any of the new classes.

## Future extensions (not implemented here)

- **Per-controller diffing.** LCD2USB can drive two HD44780 controllers;
  currently we always target `CTRL_0`. Splitting the shadow per controller
  is mechanical.
- **Extending `FontManager` to Unicode.** The `.lcf` file format currently
  keys characters by `uint8_t`. Widening to `uint32_t` and adding a magic
  `LCF2` for the new layout would let users save/load Unicode-keyed glyph
  packs; the header field is already one byte wider than needed.
- **ROM variant detection.** The current default table assumes ROM A00 for
  ASCII, which is correct everywhere. A future `makeA02Table()` could map
  Latin-1 codepoints straight to their 0x80..0xFF ROM bytes on displays
  that have them — trading ROM coverage for fewer transliterations.
- **UTF-8 scrolling.** The disabled `ScrollingText` class would need to
  operate on codepoints rather than bytes. Rewriting it on top of
  `UnicodeDisplay` is the cleanest path and would let scrolling benefit
  from the same diff/caching.

---

# The TTF / bitmap-font layer

Sits on top of the UTF-8 pipeline above. Added in the second refactor pass
because a stock ROM table can't cover Chinese, Japanese, or Korean — there
are thousands of glyphs and the ROM only has ~128 usable slots.

Source files:

- `include/lcd2usb/bitmap_font.hpp`  — `BitmapFont`, `FontAdapter`.
- `src/bitmap_font.cpp`              — FreeType-backed rasterizer.
- `examples/storyteller_utf8.cpp`    — end-to-end example.

## The "two-cell glyph" problem

The HD44780 character cell is **5 pixels wide, 8 pixels tall**. At a
sensible rasterization size for ChillBitmap (`pixel_size=9`):

| Script   | Typical glyph size | Fits in one cell? |
|----------|-------------------:|-------------------|
| Latin    | 5 × 6              | yes               |
| Latin-1  | 5 × 6              | yes               |
| CJK      | 7 × 7 — 8 × 8      | **no — two cells**|

There is no way around the physics: a Chinese character requires two
adjacent HD44780 cells, displayed side-by-side. The 20-column display
therefore holds 20 Latin characters, 10 CJK characters, or any mix
(a character of cell-width *W* consumes *W* columns).

## The RIGHT_HALF_FLAG convention

To keep the `UnicodeDisplay` pipeline one-cell-per-codepoint, we split wide
glyphs into two independent codepoints:

```
cp        →  left  half bitmap
cp | 0x80000000  →  right half bitmap   (cp | Font::RIGHT_HALF_FLAG)
```

`0x80000000` is above `U+10FFFF` (the maximum real Unicode codepoint), so
the flag cannot collide with anything the UTF-8 decoder produces. Both
halves are registered in the same `GlyphTable`. From the allocator's point
of view they are just two bitmap codepoints that always happen to appear
adjacent — the CGRAM LRU, the per-slot dirty tracking, and the run-coalesced
diff work without any change.

Consequence for CGRAM budget: each distinct CJK character on screen costs
**two slots**. With 7 usable slots, that's at most 3 unique CJK glyphs
coexisting, plus one Latin/symbol slot. In practice:

- Scrolling prose: rarely a problem — lines rotate and the LRU tracks them.
- Static dense CJK page: the 4th unique glyph falls back to `?`.

## FreeType interface

`BitmapFont` wraps FreeType behind opaque `void*` handles so callers don't
need `<ft2build.h>` on their include path. The `render()` path is:

1. `FT_Get_Char_Index(face, cp)` — tells real glyphs apart from the
   `.notdef` tofu. A zero here short-circuits to `cell_width=0` (the
   "missing glyph" sentinel), so the caller can fall back cleanly.
   *Without this probe, `FT_Load_Char` silently loads .notdef and we'd
   happily render tofu as if the glyph existed.*
2. `FT_Load_Char(FT_LOAD_RENDER | FT_LOAD_MONOCHROME | FT_LOAD_TARGET_MONO)`
   — pixel-perfect 1-bit output, no antialiasing.
3. Blit the FreeType bitmap into a 10×8 scratch buffer, using the metrics
   `bitmap_left` and `bitmap_top` to place the glyph against a configurable
   baseline row (default 6 — one row reserved for descenders).
4. If any pixel landed in columns 5–9, `cell_width` becomes 2 and the
   right half is packed into `Rendered::right`; otherwise it's a 1-cell
   glyph.

Results are cached in an `unordered_map<uint32_t, Rendered>`, so the hot
loop is a hash-map lookup after first rasterization.

## FontAdapter

Glue between a `BitmapFont` and a `UnicodeDisplay`:

- Lazily installs bitmaps into the display's `GlyphTable` on first use of
  each codepoint.
- Emits right-half sentinels to `UnicodeDisplay::printCodepoints` when a
  wide glyph is written.
- Soft-wraps wide glyphs at end-of-row so both halves stay on the same row.
- Defaults to **ROM passthrough for ASCII** (`setLatinFromFont(false)`).
  This matters: a line like "Hello world" has 7 unique ASCII characters,
  already the full CGRAM budget — bitmap-font ASCII + CJK on the same line
  would thrash. Users who want a pure pixel-font look can opt in.

The `print` call is wide-glyph-aware; `measureCells` reports cell count
(not codepoint count) for layout decisions; and `preloadGlyphs` eagerly
populates the cache for callers that want to avoid first-paint latency.

## Storyteller-UTF8

`examples/storyteller_utf8.cpp` is the reference integration. It:

1. Loads a TTF via `BitmapFont::loadTTF(path, px_size)`.
2. Uses a UTF-8-aware word wrapper that:
   - Counts cells (so CJK takes 2, Latin 1).
   - Breaks between any two CJK characters (per CJK typographic convention).
   - Breaks on spaces for Latin words.
   - Splits mid-word only for words longer than the line budget.
3. Registers a **progress bar as 6 procedural bitmaps at PUA codepoints
   `U+F000..U+F005`** — the same code path as any other wide-capable glyph.
   UnicodeDisplay's LRU keeps them resident as long as they're written
   every frame (which they are).
4. Scrolls top-to-bottom at a configurable cadence, doing one `ud.flush()`
   per step. The diff layer pushes only the new bottom row + the progress
   column each tick, so a 4-row 20-column display updates over ~20 cells
   × 2 USB transfers per cell ≈ 40 transfers per tick — well within the
   control-transfer budget even at `--speed 1`.

### CLI

```
storyteller_utf8 --file story.txt --font assets/ChillBitmap7x.ttf
                 --font-size 9 --width 20 --rows 4 --speed 2
                 [--bitmap-latin] [--disable-scrollbar]
```

By default the rightmost column is reserved for a per-row progress bar
that fills as the story scrolls past. The bar is rendered as 6 procedural
1-cell glyphs registered at PUA codepoints (see "Storyteller-UTF8" above).
Pass `--disable-scrollbar` to reclaim that column for text — useful on
narrow displays (e.g. `--width 16` where one column is 6% of the screen)
or when you'd rather have one extra character of layout room than the
visual feedback. Disabling also skips the glyph registration entirely,
so the progress glyphs never compete with text for CGRAM cache slots.

The original byte-oriented `storyteller` accepts the same flag with
identical semantics.

### What the font can't do

ChillBitmap covers a large CJK Unified Ideographs range plus Hiragana and
Katakana. It does **not** cover Hangul Syllables — 한글 will report
`cell_width=0` and render as the replacement character. A Korean-capable
bitmap TTF would drop into the same interface with no code changes.

## What was touched to add this layer

| File | Change |
|---|---|
| `include/lcd2usb/bitmap_font.hpp` | **New.** `BitmapFont`, `FontAdapter`, `Rendered`, `RIGHT_HALF_FLAG`. |
| `src/bitmap_font.cpp`             | **New.** FreeType integration, 10×8 scratch buffer, LRU-friendly caching. |
| `include/lcd2usb/unicode_display.hpp` | Added `printCodepoints(uint32_t*, size_t)` and `cursorCol() / cursorRow()`. |
| `src/unicode_display.cpp`             | Implemented `printCodepoints` — same wrap/newline semantics as `print`. |
| `include/lcd2usb/lcd2usb_lib.hpp`     | Included `bitmap_font.hpp`. |
| `src/CMakeLists.txt`                  | `find_package(Freetype REQUIRED)` + link `Freetype::Freetype` PUBLIC. |
| `examples/storyteller_utf8.cpp`       | **New.** UTF-8-aware storyteller. |
| `examples/CMakeLists.txt`             | Added `storyteller_utf8` target. |
| `assets/ChillBitmap7x.ttf`            | **New.** Shipped with the project (OFL-licensed). |
| `assets/story_utf8.txt`               | **New.** Sample multi-language story. |

None of the earlier code paths were broken. `storyteller.cpp` (the
original, byte-oriented reader) still builds and runs.

---

# The `.glyphs` pack format and the `tools/` directory

Added in the third pass, after the TTF workflow revealed two practical
problems:

1. **FreeType is a heavy runtime dependency.** Embedded/headless targets
   may not have it, and even on desktops it's ~500 KB of loaded code plus
   the 3.9 MB TTF file just to get ~15 k glyphs.
2. **UFO is what font designers actually edit.** The Boutique Bitmap 7x7
   ships only as a `.ufo` directory — 60 MB of XML, unthinkable to parse
   at runtime.

Both problems have the same shape: rasterize once at build time, load a
tiny binary at runtime. That's the `.glyphs` format.

## Shared abstraction: `GlyphSource`

`include/lcd2usb/glyph_source.hpp` defines the minimal interface both font
backends expose:

```cpp
class GlyphSource {
public:
    virtual Rendered render(uint32_t codepoint) = 0;
    virtual int      cellWidth(uint32_t codepoint);   // default via render()
    int              measure(std::string_view utf8);  // default loop
};
```

`BitmapFont` (TTF via FreeType) and `GlyphPack` (packed binary) both
inherit `GlyphSource`. `FontAdapter` binds against the base, so swapping
backends at runtime is one line:

```cpp
std::unique_ptr<GlyphSource> source =
    isGlyphsPack(path) ? std::make_unique<GlyphPack>()
                       : std::make_unique<BitmapFont>();
FontAdapter adapter(*source, ud);
```

This also drops a hard link on FreeType for callers that only use packs —
if you don't include `bitmap_font.hpp` you don't pull in `<ft2build.h>`.

## The `.glyphs` binary format

Documented verbatim in `include/lcd2usb/glyph_pack.hpp`. Summary:

```
  offset  size  field
  ──────  ────  ──────────────────────────────────────────────────
  0       4     magic      = "LCDG"
  4       1     version    = 1
  5       1     cell_width = 5
  6       1     cell_height= 8
  7       1     slot_bytes = 24
  8       4     glyph_count (u32 LE)
  12      4     reserved
  16+n*24       glyph entries, sorted ascending by codepoint

  per entry (24 bytes):
    u32 LE  codepoint
    u8      cell_count (1 or 2)
    3B      padding
    8B      left  bitmap (row 0..7, low 5 bits per byte, bit 4 leftmost)
    8B      right bitmap (zero-filled for 1-cell glyphs)
```

Design choices worth calling out:

- **Fixed-size records, sorted by codepoint.** Binary search directly in
  the mmap'd buffer — no decoding, no allocation, no hash map. 15 k
  entries → ~14 comparisons per lookup.
- **Little-endian wire format, byte-assembled reader.** The reader in
  `GlyphPack` doesn't rely on host endianness. Same file works on LE and
  BE hosts.
- **Ascending-codepoint invariant is validated at load time.** Cheap
  O(N) check that catches a whole class of pack-writer bugs that would
  otherwise silently break binary search in weird ways.
- **No runtime compression.** At 24 B/glyph the full Boutique pack is
  352 KB. Compressing would complicate the binary-search path and save
  ~50 %, which is not worth the engineering on any host that can afford
  to run an LCD driver.

Size comparison for the same font coverage (Boutique 7x7, 15 014 glyphs):

| Format                         | Size      | Runtime deps        |
|--------------------------------|----------:|---------------------|
| UFO source                     | ~60 MB    | XML parser          |
| TTF (rasterized from UFO)      | ~3.9 MB   | FreeType            |
| `.glyphs` pack (this format)   | 352 KB    | none                |

## The `tools/` directory

### `tools/ufo_to_glyphs.py` — UFO → `.glyphs` converter

Offline converter, stdlib-only Python (no PIL, no fontTools). The core
algorithm is even-odd scanline fill against **pixel centers**, which is
exact for pixel fonts (every edge is axis-aligned and on the pixel grid).

```
python3 tools/ufo_to_glyphs.py \
    --ufo path/to/Font.ufo \
    --out assets/font.glyphs

# Or keep only specific Unicode ranges to shrink the pack:
python3 tools/ufo_to_glyphs.py \
    --ufo Font.ufo --out font.glyphs \
    --include-blocks 0020-007E,00A0-024F,4E00-9FFF,3040-30FF
```

Flags:

- `--pixel-size` — font units per pixel (256 for Boutique; UFOs vary).
- `--cell-width`, `--cell-height` — HD44780 geometry (defaults 5×8).
- `--baseline` — which row of the 5×8 cell holds font y=0 (default 6,
  leaving 1 row for descenders).
- `--scratch-width` — total pixel columns to rasterize (default 10 = two
  HD44780 cells wide, which is the maximum supported glyph width).
- `--include-blocks` — comma-separated hex ranges, e.g.
  `0020-007E,4E00-9FFF`. Default: keep everything with a `<unicode>` entry.

The tool skips:

- Glyphs without a `<unicode hex="..."/>` tag (composite references,
  named-only glyphs).
- Glyphs whose every pixel lies outside the cell (a space is an exception
  and is kept).
- Glyphs with non-pixel-aligned coordinates (not a pixel font).

### `tools/glyph_inspect` — ASCII-art preview

A C++ CLI that loads either a `.glyphs` pack or a `.ttf` and prints the
requested codepoint(s) as 5-column ASCII art with a `|` separator between
the two halves of 2-cell glyphs. Useful for verifying a pack after bake,
or comparing how the same codepoint renders in two different sources.

```
glyph_inspect assets/boutique7x7.glyphs 'A'
glyph_inspect assets/boutique7x7.glyphs 0x4E2D
glyph_inspect assets/boutique7x7.glyphs 'Hello 中国'
glyph_inspect assets/ChillBitmap7x.ttf  'A' --ttf --px 9
```

Sample output:

```
U+668E  cell_width=2
  .....|.....
  ...#.|#....
  #####|##...
  #.##.|#....
  ###.#|.#...
  #.###|##...
  ##..#|.....
  ..##.|##...
  left  = { 0x00, 0x02, 0x1F, 0x16, 0x1D, 0x17, 0x19, 0x06 }
  right = { 0x00, 0x10, 0x18, 0x10, 0x08, 0x18, 0x00, 0x18 }
```

The hex arrays are pasteable straight into C source.

## End-to-end round-trip verification

The user-supplied `uni668E.glif` was run through the full pipeline:

```
uni668E.glif (UFO)
  ↓  tools/ufo_to_glyphs.py
boutique7x7.glyphs  (1 or 15 014 entries)
  ↓  GlyphPack::loadFile + render(0x668E)
Rendered { left = {0x00,0x02,0x1F,0x16,0x1D,0x17,0x19,0x06},
           right = {0x00,0x10,0x18,0x10,0x08,0x18,0x00,0x18} }
```

The same bytes come out whether the pack contains just the one glyph or
the full 15 k font. A visual comparison of the ASCII render against the
original UFO outline confirms pixel-perfect fidelity within the 5×8 cell.

## Storyteller integration

`examples/storyteller_utf8.cpp` now defaults to the `.glyphs` pack path
and falls back to TTF when the `--font` path doesn't end in `.glyphs`:

```
storyteller_utf8                                          # pack, default
storyteller_utf8 --font assets/ChillBitmap7x.ttf --font-size 9   # TTF
```

The switch is polymorphic — the two code paths converge at
`FontAdapter(*font_, ud)`.

## File inventory (third pass)

| File | Change |
|---|---|
| `include/lcd2usb/glyph_source.hpp`  | **New.** Abstract base, shared `Rendered` + `RIGHT_HALF_FLAG`. |
| `src/glyph_source.cpp`              | **New.** Default `measure()` implementation. |
| `include/lcd2usb/glyph_pack.hpp`    | **New.** `.glyphs` file-format spec + `GlyphPack` class. |
| `src/glyph_pack.cpp`                | **New.** Endian-clean reader, O(log N) binary-search lookup. |
| `include/lcd2usb/bitmap_font.hpp`   | `BitmapFont` now inherits `GlyphSource`; `FontAdapter` takes `GlyphSource&`. |
| `src/bitmap_font.cpp`               | Signature updates; dropped duplicated `measure`. |
| `tools/ufo_to_glyphs.py`            | **New.** Offline UFO → `.glyphs` converter, stdlib Python. |
| `tools/glyph_inspect.cpp`           | **New.** ASCII-art preview tool. |
| `tools/CMakeLists.txt`              | **New.** Builds `glyph_inspect`, installs the Python tool. |
| `CMakeLists.txt`                    | Adds `tools/` subdirectory. |
| `src/CMakeLists.txt`                | Adds `glyph_source.cpp`, `glyph_pack.cpp`. |
| `include/lcd2usb/lcd2usb_lib.hpp`   | Includes the two new headers. |
| `examples/storyteller_utf8.cpp`     | Polymorphic font loading, `.glyphs` default. |
| `assets/boutique7x7.glyphs`         | **New.** Pre-baked Boutique 7x7 pack, 15 014 glyphs. |

Previous-pass test suites (`utf8_test`, `ud_test`, `font_test`) all still
pass. New `pack_test` covers binary-search lookup, missing-codepoint
handling, interop with `FontAdapter`, and the full UFO-demo round-trip.
