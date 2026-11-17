#!/usr/bin/env python3
"""
ufo_to_glyphs.py — Convert a UFO (Unified Font Object) pixel font into the
compact binary .glyphs pack format used by LCD2USB++.

Why this exists
---------------
A UFO is a directory of XML .glif files — ~60 MB of text for a 13k-glyph CJK
font. Parsing that at runtime on a host that drives an LCD over USB would be
absurd. This script does it once, offline, producing a ~300 KB binary pack
that the C++ library can binary-search directly.

Works with any pixel-aligned UFO: every glyph coordinate must be a multiple
of the `--pixel-size` argument (default 256 units, which is what
BoutiqueBitmap7x7 and most 7x7 pixel UFOs use). Non-rectilinear outlines are
rasterized by even-odd scanline fill against pixel centers, which is exact
for pixel fonts (where every edge is axis-aligned anyway).

Usage
-----
    python3 tools/ufo_to_glyphs.py \\
        --ufo path/to/Font.ufo \\
        --out assets/font.glyphs

    # Restrict to specific Unicode blocks to keep the pack small:
    python3 tools/ufo_to_glyphs.py \\
        --ufo Font.ufo --out font.glyphs \\
        --include-blocks 0020-007E,00A0-024F,4E00-9FFF,3040-30FF

    # Adjust the vertical layout within the 5x8 cell:
    python3 tools/ufo_to_glyphs.py \\
        --ufo Font.ufo --out font.glyphs \\
        --baseline 6 --cell-width 5 --cell-height 8
"""

import argparse
import os
import re
import struct
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

# ─── Binary format constants — MUST match include/lcd2usb/glyph_pack.hpp ──
MAGIC        = b"LCDG"
VERSION      = 1
HEADER_SIZE  = 16
SLOT_BYTES   = 24
CELL_WIDTH   = 5
CELL_HEIGHT  = 8


# ─── UFO parsing ─────────────────────────────────────────────────────────

def parse_glif(path):
    """
    Read one .glif file and return (codepoint, advance_x, contours) where
    contours is a list of polygons, each polygon is a list of (x,y) ints.

    Returns None if the glyph has no <unicode hex="..."/> (which means it's
    a composite or a named-only glyph we can't place in the pack).
    """
    tree = ET.parse(path)
    root = tree.getroot()

    unis = [u.get("hex") for u in root.findall("unicode")]
    if not unis:
        return None
    cp = int(unis[0], 16)

    adv = root.find("advance")
    advance_x = int(float(adv.get("width", "0"))) if adv is not None else 0

    contours = []
    outline = root.find("outline")
    if outline is None:
        return (cp, advance_x, contours)

    for contour in outline.findall("contour"):
        pts = []
        for p in contour.findall("point"):
            x = int(round(float(p.get("x", "0"))))
            y = int(round(float(p.get("y", "0"))))
            pts.append((x, y))
        if len(pts) >= 3:
            contours.append(pts)

    return (cp, advance_x, contours)


# ─── Rasterization ───────────────────────────────────────────────────────

def point_in_polygons(px, py, contours):
    """
    Even-odd fill test: is the point (px,py) inside the symmetric difference
    of all the contours? Ray-casts to +inf along the +x axis and counts edge
    crossings. This is the standard non-zero / even-odd winding answer for a
    shape with holes. For pixel-aligned UFOs, it is exact.
    """
    inside = False
    for poly in contours:
        n = len(poly)
        for i in range(n):
            x1, y1 = poly[i]
            x2, y2 = poly[(i + 1) % n]
            # Does the horizontal ray from (px,py) intersect this edge?
            if (y1 > py) == (y2 > py):
                continue                # both endpoints on the same side
            # X-coordinate where the edge crosses y=py.
            x_cross = x1 + (py - y1) * (x2 - x1) / (y2 - y1)
            if px < x_cross:
                inside = not inside
    return inside


def rasterize(contours, unit_per_pixel, cell_w_px, cell_h_px, baseline_y_cell, scratch_w_px):
    """
    Rasterize a glyph's contours into an HD44780-compatible cell layout.

    Returns (cell_count, left_row_bytes, right_row_bytes) where each is an
    array of 8 bytes (low 5 bits used per byte).

    Coordinate convention:
        Font design space: y grows upward, (0,0) is the pen origin (baseline).
        Pixel space:       row 0 is the TOP of the 5x8 cell, row 7 the bottom.
                           col 0 is the LEFTMOST column.

    The baseline_y_cell argument says which ROW of the cell the font's y=0
    line corresponds to — e.g. 6 means 6 rows of ascent above and 1 row of
    descent below. Anything clipped by the cell bounds is silently dropped.
    """
    # We rasterize into a scratch array `scratch_w_px` (typically 10 = 2 cells)
    # wide and cell_h_px (8) tall, using pixel centers for even-odd testing.
    scratch = [[False] * scratch_w_px for _ in range(cell_h_px)]

    for cy in range(cell_h_px):
        # Pixel row `cy` (0 at top) corresponds to font y-row:
        #   font_row = baseline_y_cell - cy            (integer)
        # Point-center in font units:
        py_units = (baseline_y_cell - cy) * unit_per_pixel + unit_per_pixel // 2
        for cx in range(scratch_w_px):
            px_units = cx * unit_per_pixel + unit_per_pixel // 2
            if point_in_polygons(px_units, py_units, contours):
                scratch[cy][cx] = True

    # Determine cell count: does any pixel live in the right cell?
    has_right = any(scratch[y][x] for y in range(cell_h_px)
                                 for x in range(cell_w_px, scratch_w_px))
    cell_count = 2 if has_right else 1

    def pack5(c0):
        out = []
        for y in range(cell_h_px):
            b = 0
            for x in range(cell_w_px):
                b <<= 1
                if c0 + x < scratch_w_px and scratch[y][c0 + x]:
                    b |= 1
            out.append(b)
        return bytes(out)

    left  = pack5(0)
    right = pack5(cell_w_px) if has_right else bytes(cell_h_px)
    return (cell_count, left, right)


# ─── Unicode block filter ───────────────────────────────────────────────

def parse_block_spec(spec):
    """'0020-007E,4E00-9FFF' -> [(0x20,0x7E), (0x4E00,0x9FFF)]."""
    if not spec:
        return None
    ranges = []
    for piece in spec.split(","):
        piece = piece.strip()
        if not piece: continue
        m = re.match(r"([0-9A-Fa-f]+)-([0-9A-Fa-f]+)", piece)
        if not m:
            raise ValueError(f"bad block spec: {piece!r}")
        ranges.append((int(m.group(1), 16), int(m.group(2), 16)))
    return ranges

def in_ranges(cp, ranges):
    if ranges is None:
        return True
    return any(a <= cp <= b for (a, b) in ranges)


# ─── Main ───────────────────────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[1] if __doc__ else "")
    ap.add_argument("--ufo", required=True, help="Path to the .ufo directory")
    ap.add_argument("--out", required=True, help="Output .glyphs pack")
    ap.add_argument("--pixel-size", type=int, default=256,
                    help="Font design units per pixel (Boutique=256)")
    ap.add_argument("--cell-width",  type=int, default=CELL_WIDTH,  help="HD44780 cell width (default 5)")
    ap.add_argument("--cell-height", type=int, default=CELL_HEIGHT, help="HD44780 cell height (default 8)")
    ap.add_argument("--baseline", type=int, default=6,
                    help="Cell row (0=top) where font y=0 sits. Default 6 leaves 1 descender row.")
    ap.add_argument("--scratch-width", type=int, default=10,
                    help="Total pixel columns to rasterize (2 cells = 10 is default)")
    ap.add_argument("--include-blocks", default=None,
                    help="Comma-separated Unicode ranges to keep, e.g. '0020-007E,4E00-9FFF'")
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    ufo = Path(args.ufo)
    glyphs_dir = ufo / "glyphs"
    if not glyphs_dir.is_dir():
        print(f"error: {glyphs_dir} is not a directory", file=sys.stderr)
        sys.exit(1)

    blocks = parse_block_spec(args.include_blocks)

    glyph_files = sorted(glyphs_dir.glob("*.glif"))
    if not glyph_files:
        print(f"error: no .glif files found in {glyphs_dir}", file=sys.stderr)
        sys.exit(1)
    print(f"[ufo_to_glyphs] scanning {len(glyph_files)} .glif files")

    entries = []          # list of (cp, cell_count, left_bytes, right_bytes)
    n_skipped_noupdate = 0
    n_skipped_block    = 0
    n_skipped_empty    = 0
    n_skipped_badgrid  = 0

    for path in glyph_files:
        try:
            info = parse_glif(path)
        except ET.ParseError as e:
            if args.verbose: print(f"  skip (XML parse error): {path.name}: {e}", file=sys.stderr)
            continue
        if info is None:
            n_skipped_noupdate += 1
            continue
        cp, advance, contours = info
        if not in_ranges(cp, blocks):
            n_skipped_block += 1
            continue

        # Enforce pixel alignment. Non-aligned coords mean the font isn't a
        # pixel font and our rasterizer's assumptions break. Print and skip.
        bad = False
        for poly in contours:
            for (x, y) in poly:
                if x % args.pixel_size != 0 or y % args.pixel_size != 0:
                    bad = True; break
            if bad: break
        if bad:
            n_skipped_badgrid += 1
            if args.verbose:
                print(f"  skip (non-pixel-aligned coords): U+{cp:04X}", file=sys.stderr)
            continue

        cell_count, left, right = rasterize(
            contours,
            unit_per_pixel=args.pixel_size,
            cell_w_px=args.cell_width,
            cell_h_px=args.cell_height,
            baseline_y_cell=args.baseline,
            scratch_w_px=args.scratch_width,
        )

        # If the rasterization produced nothing visible AND the glyph isn't
        # a whitespace (i.e. has no advance or has contours), skip — we'd
        # otherwise fill the pack with empty entries for rare ideographs.
        # Space (U+0020) is the exception: blank is its correct rendering.
        any_pixel = any(left) or any(right)
        if not any_pixel and cp != 0x20 and contours:
            n_skipped_empty += 1
            continue

        entries.append((cp, cell_count, left, right))

    entries.sort(key=lambda e: e[0])

    # Sanity: no duplicate codepoints.
    for i in range(1, len(entries)):
        if entries[i][0] == entries[i-1][0]:
            print(f"error: duplicate codepoint U+{entries[i][0]:04X}", file=sys.stderr)
            sys.exit(2)

    print(f"[ufo_to_glyphs] kept {len(entries)} glyphs "
          f"(skipped: {n_skipped_noupdate} no-unicode, "
          f"{n_skipped_block} out-of-block, "
          f"{n_skipped_empty} empty, "
          f"{n_skipped_badgrid} bad-grid)")

    # ── write .glyphs ───────────────────────────────────────────────────
    with open(args.out, "wb") as f:
        header = struct.pack(
            "<4sBBBBII",
            MAGIC,
            VERSION,
            args.cell_width,
            args.cell_height,
            SLOT_BYTES,
            len(entries),
            0,      # reserved
        )
        assert len(header) == HEADER_SIZE, (len(header), HEADER_SIZE)
        f.write(header)

        for cp, cc, left, right in entries:
            # 4 bytes cp (LE) | 1 byte cell_count | 3 bytes pad | 8 left | 8 right
            slot = struct.pack("<IB3x", cp, cc) + left + right
            assert len(slot) == SLOT_BYTES, len(slot)
            f.write(slot)

    size = os.path.getsize(args.out)
    print(f"[ufo_to_glyphs] wrote {args.out} ({size} bytes, "
          f"{size // max(len(entries),1)} B/glyph)")


if __name__ == "__main__":
    main()
