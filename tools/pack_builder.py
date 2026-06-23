#!/usr/bin/env python3
"""Build .clpk glyph template packs for clang-ldl logical OCR."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "python"))

import importlib.util

_spec = importlib.util.spec_from_file_location(
    "test_image", ROOT / "python" / "clang_ldl" / "test_image.py"
)
_mod = importlib.util.module_from_spec(_spec)
assert _spec.loader is not None
_spec.loader.exec_module(_mod)
FONT5x7 = _mod.FONT5x7

sys.path.insert(0, str(ROOT / "tools"))
from struct_features import compute_features  # noqa: E402

CLPK_MAGIC = b"CLPK"
CLPK_VERSION = 1

GRID_W = 20
GRID_H = 28
FONT_SCALE = 4


def render_glyph_bitmap(codepoint: int) -> list[int]:
    bitmap = [0] * (GRID_W * GRID_H)
    if codepoint < 32 or codepoint > 126:
        return bitmap
    glyph = FONT5x7[codepoint - 32]
    offset_x = (GRID_W - 5 * FONT_SCALE) // 2
    offset_y = (GRID_H - 7 * FONT_SCALE) // 2
    for row in range(7):
        for col in range(5):
            if (glyph[row] >> (4 - col)) & 1:
                for sy in range(FONT_SCALE):
                    for sx in range(FONT_SCALE):
                        x = offset_x + col * FONT_SCALE + sx
                        y = offset_y + row * FONT_SCALE + sy
                        if 0 <= x < GRID_W and 0 <= y < GRID_H:
                            bitmap[y * GRID_W + x] = 255
    return bitmap


def build_latin_pack() -> list[dict]:
    glyphs = []
    for cp in range(32, 127):
        bitmap = render_glyph_bitmap(cp)
        feats = compute_features(bitmap, GRID_W, GRID_H)
        glyphs.append(
            {
                "codepoint": cp,
                "bitmap": bitmap,
                "holes": int(feats["holes"]),
                "endpoints": int(feats["endpoints"]),
                "junctions": int(feats["junctions"]),
                "aspect": float(feats["aspect"]),
            }
        )
    return glyphs


def write_clpk(
    path: Path,
    pack_id: str,
    glyphs: list[dict],
    *,
    grid_w: int = GRID_W,
    grid_h: int = GRID_H,
    threshold: float = 0.30,
    w_ncc: float = 0.55,
    w_struct: float = 0.25,
    w_aspect: float = 0.20,
) -> None:
    pack_bytes = pack_id.encode("utf-8")
    buf = bytearray()
    buf += CLPK_MAGIC
    buf += struct.pack("<I", CLPK_VERSION)
    buf += struct.pack("<H", len(pack_bytes))
    buf += pack_bytes
    buf += struct.pack("<HH", grid_w, grid_h)
    buf += struct.pack("<ffff", threshold, w_ncc, w_struct, w_aspect)
    buf += struct.pack("<I", len(glyphs))
    for g in glyphs:
        bitmap = g["bitmap"]
        expected = grid_w * grid_h
        if len(bitmap) != expected:
            raise ValueError(f"codepoint U+{g['codepoint']:04X}: bitmap size {len(bitmap)} != {expected}")
        buf += struct.pack("<I", g["codepoint"])
        buf += struct.pack(
            "<BBBB",
            min(g["holes"], 255),
            min(g["endpoints"], 255),
            min(g["junctions"], 255),
            0,
        )
        buf += struct.pack("<f", g["aspect"])
        buf += struct.pack("<I", len(bitmap))
        buf += bytes(bitmap)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(buf)
    print(f"Wrote {path} ({len(glyphs)} glyphs, {len(buf)} bytes)")


def main() -> int:
    parser = argparse.ArgumentParser(description="Build clang-ldl .clpk glyph packs")
    parser.add_argument(
        "--pack",
        choices=["latin"],
        default="latin",
        help="Pack to build (default: latin)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=ROOT / "packs" / "latin.clpk",
        help="Output .clpk path",
    )
    args = parser.parse_args()

    if args.pack == "latin":
        glyphs = build_latin_pack()
        write_clpk(args.output, "latin", glyphs)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
