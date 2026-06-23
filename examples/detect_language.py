#!/usr/bin/env python3
"""Example CLI for clang-ldl: detect language from a text image."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

# Allow running without pip install when executed from repo root.
_REPO_PYTHON = Path(__file__).resolve().parents[1] / "python"
if str(_REPO_PYTHON) not in sys.path:
    sys.path.insert(0, str(_REPO_PYTHON))

from clang_ldl import ImageLanguageDetector
from clang_ldl.test_image import render_terminal_ppm


def render_test_image(text: str, out_path: Path) -> Path:
    """Render with the native 5x7 font so template matching succeeds."""
    if out_path.suffix.lower() == ".ppm":
        return render_terminal_ppm(text, out_path)
    # Fallback: PIL raster font (lower OCR accuracy vs native templates)
    width, height = max(320, len(text) * 28 + 48), 120
    img = Image.new("RGB", (width, height), color="white")
    draw = ImageDraw.Draw(img)
    try:
        font = ImageFont.truetype("DejaVuSans.ttf", 48)
    except OSError:
        font = ImageFont.load_default()
    draw.text((24, 32), text, fill="black", font=font)
    img.save(out_path)
    return out_path


def main() -> int:
    parser = argparse.ArgumentParser(description="Detect language from a text image")
    parser.add_argument("image", nargs="?", help="Path to PNG/JPEG/BMP image")
    parser.add_argument(
        "--synthetic",
        metavar="TEXT",
        help="Render synthetic test image with this text instead of loading a file",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Where to save synthetic image (with --synthetic)",
    )
    parser.add_argument("--json", action="store_true", help="Print JSON result")
    args = parser.parse_args()

    detector = ImageLanguageDetector()

    if args.synthetic:
        out = args.output or Path("clang_ldl_synthetic.ppm")
        image_path = render_test_image(args.synthetic, out)
        result = detector.detect_from_file(image_path)
    elif args.image:
        result = detector.detect_from_file(args.image)
    else:
        parser.error("provide IMAGE path or --synthetic TEXT")

    if args.json:
        print(json.dumps(result.to_dict(), indent=2))
    else:
        print(f"Native core: {detector.native_version}")
        print(f"Recognized text: {result.text!r}")
        print(f"Glyph count: {result.glyph_count}, mean confidence: {result.mean_confidence:.2f}")
        if result.languages:
            print("Languages:")
            for lang in result.languages:
                print(f"  - {lang.name} ({lang.code}): {lang.percent}%")
        else:
            print("Languages: (none detected)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
