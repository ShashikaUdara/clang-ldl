#!/usr/bin/env python3
"""Build .clpk glyph template packs for clang-ldl logical OCR."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "python"))
sys.path.insert(0, str(ROOT / "tools"))

import importlib.util

_spec = importlib.util.spec_from_file_location(
    "test_image", ROOT / "python" / "clang_ldl" / "test_image.py"
)
_mod = importlib.util.module_from_spec(_spec)
assert _spec.loader is not None
_spec.loader.exec_module(_mod)
FONT5x7 = _mod.FONT5x7

from ocr_render import GRID_H, GRID_W, render_cell_glyph_bitmap, resolve_font  # noqa: E402
from struct_features import compute_features  # noqa: E402

CLPK_MAGIC = b"CLPK"
CLPK_VERSION = 1
FONT_SCALE = 4

PACK_SPECS: dict[str, dict] = {
    "latin": {
        "kind": "bitmap5x7",
        "threshold": 0.30,
        "w_ncc": 0.55,
        "w_struct": 0.25,
        "w_aspect": 0.20,
    },
    "cyrillic": {
        "kind": "ttf",
        "font": "NotoSans-Regular.ttf",
        "codepoints": list(range(0x0410, 0x0430)) + list(range(0x0430, 0x0450)) + [0x0401, 0x0451],
        "grid_w": 28,
        "grid_h": 40,
        "threshold": 0.15,
        "w_ncc": 0.85,
        "w_struct": 0.05,
        "w_aspect": 0.10,
    },
    "greek": {
        "kind": "ttf",
        "font": "NotoSans-Regular.ttf",
        "codepoints": list(range(0x0391, 0x03AA)) + list(range(0x03B1, 0x03CA)),
        "grid_w": 28,
        "grid_h": 40,
        "threshold": 0.15,
        "w_ncc": 0.85,
        "w_struct": 0.05,
        "w_aspect": 0.10,
    },
    "armenian": {
        "kind": "ttf",
        "font": "NotoSansArmenian-Regular.ttf",
        "codepoints": list(range(0x0531, 0x0557)) + list(range(0x0561, 0x0588)),
        "grid_w": 28,
        "grid_h": 40,
        "threshold": 0.15,
        "w_ncc": 0.85,
        "w_struct": 0.05,
        "w_aspect": 0.10,
    },
    "georgian": {
        "kind": "ttf",
        "font": "NotoSansGeorgian-Regular.ttf",
        "codepoints": list(range(0x10D0, 0x10F1)),
        "grid_w": 28,
        "grid_h": 40,
        "threshold": 0.15,
        "w_ncc": 0.85,
        "w_struct": 0.05,
        "w_aspect": 0.10,
    },
    "hebrew": {
        "kind": "ttf",
        "font": "NotoSansHebrew-Regular.ttf",
        "codepoints": list(range(0x05D0, 0x05EB)),
        "grid_w": 28,
        "grid_h": 40,
        "rtl": True,
        "threshold": 0.15,
        "w_ncc": 0.85,
        "w_struct": 0.05,
        "w_aspect": 0.10,
    },
    "thai": {
        "kind": "ttf",
        "font": "NotoSansThai-Regular.ttf",
        "codepoints": list(range(0x0E01, 0x0E2F))
        + list(range(0x0E30, 0x0E3B))
        + list(range(0x0E40, 0x0E4F)),
        "grid_w": 28,
        "grid_h": 40,
        "threshold": 0.15,
        "w_ncc": 0.82,
        "w_struct": 0.08,
        "w_aspect": 0.10,
    },
    "lao": {
        "kind": "ttf",
        "font": "NotoSansLao-Regular.ttf",
        "codepoints": list(range(0x0E81, 0x0EAE))
        + list(range(0x0EAF, 0x0EDD)),
        "grid_w": 28,
        "grid_h": 40,
        "threshold": 0.15,
        "w_ncc": 0.82,
        "w_struct": 0.08,
        "w_aspect": 0.10,
    },
    "myanmar": {
        "kind": "ttf",
        "font": "NotoSansMyanmar-Regular.ttf",
        "codepoints": list(range(0x1000, 0x102C)),
        "grid_w": 28,
        "grid_h": 40,
        "threshold": 0.14,
        "w_ncc": 0.55,
        "w_struct": 0.35,
        "w_aspect": 0.10,
    },
    "ethiopic": {
        "kind": "ttf",
        "font": "NotoSansEthiopic-Regular.ttf",
        "codepoints": list(range(0x1200, 0x1248)),
        "grid_w": 40,
        "grid_h": 56,
        "threshold": 0.14,
        "w_ncc": 0.80,
        "w_struct": 0.10,
        "w_aspect": 0.10,
    },
    "devanagari": {
        "kind": "ttf",
        "font": "NotoSansDevanagari-Regular.ttf",
        "codepoints": list(range(0x0905, 0x0915))
        + list(range(0x0915, 0x093A))
        + list(range(0x093E, 0x094D)),
        "grid_w": 32,
        "grid_h": 48,
        "threshold": 0.14,
        "w_ncc": 0.75,
        "w_struct": 0.15,
        "w_aspect": 0.10,
    },
    "bengali": {
        "kind": "ttf",
        "font": "NotoSansBengali-Regular.ttf",
        "codepoints": list(range(0x0985, 0x0995))
        + list(range(0x0995, 0x09BA))
        + list(range(0x09BE, 0x09CD)),
        "grid_w": 32,
        "grid_h": 48,
        "threshold": 0.14,
        "w_ncc": 0.75,
        "w_struct": 0.15,
        "w_aspect": 0.10,
    },
    "gurmukhi": {
        "kind": "ttf",
        "font": "NotoSansGurmukhi-Regular.ttf",
        "codepoints": list(range(0x0A05, 0x0A15))
        + list(range(0x0A15, 0x0A3A))
        + list(range(0x0A3E, 0x0A4D)),
        "grid_w": 32,
        "grid_h": 48,
        "threshold": 0.14,
        "w_ncc": 0.75,
        "w_struct": 0.15,
        "w_aspect": 0.10,
    },
    "gujarati": {
        "kind": "ttf",
        "font": "NotoSansGujarati-Regular.ttf",
        "codepoints": list(range(0x0A85, 0x0A95))
        + list(range(0x0A95, 0x0AB9))
        + list(range(0x0ABE, 0x0ACD)),
        "grid_w": 32,
        "grid_h": 48,
        "threshold": 0.14,
        "w_ncc": 0.75,
        "w_struct": 0.15,
        "w_aspect": 0.10,
    },
    "odia": {
        "kind": "ttf",
        "font": "NotoSansOriya-Regular.ttf",
        "codepoints": list(range(0x0B05, 0x0B15))
        + list(range(0x0B15, 0x0B3A))
        + list(range(0x0B3E, 0x0B4D)),
        "grid_w": 32,
        "grid_h": 48,
        "threshold": 0.14,
        "w_ncc": 0.75,
        "w_struct": 0.15,
        "w_aspect": 0.10,
    },
    "tamil": {
        "kind": "ttf",
        "font": "NotoSansTamil-Regular.ttf",
        "codepoints": list(range(0x0B85, 0x0B95))
        + list(range(0x0B95, 0x0BB8))
        + list(range(0x0BBE, 0x0BCD)),
        "grid_w": 32,
        "grid_h": 48,
        "threshold": 0.14,
        "w_ncc": 0.75,
        "w_struct": 0.15,
        "w_aspect": 0.10,
    },
    "telugu": {
        "kind": "ttf",
        "font": "NotoSansTelugu-Regular.ttf",
        "codepoints": list(range(0x0C05, 0x0C15))
        + list(range(0x0C15, 0x0C3A))
        + list(range(0x0C3E, 0x0C4D)),
        "grid_w": 32,
        "grid_h": 48,
        "threshold": 0.14,
        "w_ncc": 0.75,
        "w_struct": 0.15,
        "w_aspect": 0.10,
    },
    "kannada": {
        "kind": "ttf",
        "font": "NotoSansKannada-Regular.ttf",
        "codepoints": list(range(0x0C85, 0x0C95))
        + list(range(0x0C95, 0x0CB9))
        + list(range(0x0CBE, 0x0CCD)),
        "grid_w": 32,
        "grid_h": 48,
        "threshold": 0.14,
        "w_ncc": 0.75,
        "w_struct": 0.15,
        "w_aspect": 0.10,
    },
    "malayalam": {
        "kind": "ttf",
        "font": "NotoSansMalayalam-Regular.ttf",
        "codepoints": list(range(0x0D05, 0x0D15))
        + list(range(0x0D15, 0x0D3A))
        + list(range(0x0D3E, 0x0D4D)),
        "grid_w": 32,
        "grid_h": 48,
        "threshold": 0.14,
        "w_ncc": 0.75,
        "w_struct": 0.15,
        "w_aspect": 0.10,
    },
    "sinhala": {
        "kind": "ttf",
        "font": "NotoSansSinhala-Regular.ttf",
        "codepoints": list(range(0x0D85, 0x0D97))
        + list(range(0x0D9A, 0x0DC7))
        + list(range(0x0DCF, 0x0DDF)),
        "grid_w": 32,
        "grid_h": 48,
        "threshold": 0.14,
        "w_ncc": 0.75,
        "w_struct": 0.15,
        "w_aspect": 0.10,
    },
}


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


def build_ttf_pack(
    font_name: str,
    codepoints: list[int],
    *,
    grid_w: int,
    grid_h: int,
    rtl: bool = False,
) -> list[dict]:
    font_path = resolve_font(font_name)
    glyphs = []
    seen: set[int] = set()
    for cp in codepoints:
        if cp in seen:
            continue
        seen.add(cp)
        bitmap = render_cell_glyph_bitmap(
            cp, font_path, grid_w=grid_w, grid_h=grid_h, rtl=rtl
        )
        if not any(bitmap):
            continue
        feats = compute_features(bitmap, grid_w, grid_h)
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


def build_pack(pack_id: str) -> tuple[list[dict], int, int]:
    spec = PACK_SPECS[pack_id]
    grid_w = spec.get("grid_w", GRID_W)
    grid_h = spec.get("grid_h", GRID_H)
    if spec["kind"] == "bitmap5x7":
        return build_latin_pack(), grid_w, grid_h
    return build_ttf_pack(
        spec["font"],
        spec["codepoints"],
        grid_w=grid_w,
        grid_h=grid_h,
        rtl=bool(spec.get("rtl", False)),
    ), grid_w, grid_h


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
        choices=sorted(PACK_SPECS),
        default="latin",
        help="Pack to build (default: latin)",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Build every native OCR pack (Latin + Tier A + Tier B + Indic)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Output .clpk path (default: packs/<pack>.clpk)",
    )
    args = parser.parse_args()

    pack_ids = sorted(PACK_SPECS) if args.all else [args.pack]
    for pack_id in pack_ids:
        spec = PACK_SPECS[pack_id]
        glyphs, grid_w, grid_h = build_pack(pack_id)
        out = args.output if args.output and not args.all else ROOT / "packs" / f"{pack_id}.clpk"
        write_clpk(
            out,
            pack_id,
            glyphs,
            grid_w=grid_w,
            grid_h=grid_h,
            threshold=spec["threshold"],
            w_ncc=spec["w_ncc"],
            w_struct=spec["w_struct"],
            w_aspect=spec["w_aspect"],
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
