#!/usr/bin/env python3
"""Example CLI for clang-ldl: detect language from a text image."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

# Allow running without pip install when executed from repo root.
_REPO_PYTHON = Path(__file__).resolve().parents[1] / "python"
if str(_REPO_PYTHON) not in sys.path:
    sys.path.insert(0, str(_REPO_PYTHON))

from clang_ldl import ImageLanguageDetector, language_coverage_summary
from clang_ldl.test_image import render_synthetic_image
from clang_ldl.text_utils import is_ascii_ocr_text, normalize_cli_text

_SOURCE_LABELS = {
    "ocr": "native C++ OCR",
    "unicode": "Unicode script analysis (native OCR not available for this script yet)",
    "hybrid": "native OCR + script analysis",
}


def main() -> int:
    parser = argparse.ArgumentParser(description="Detect language from a text image")
    parser.add_argument("image", nargs="?", help="Path to PNG/JPEG/BMP/PPM image")
    parser.add_argument(
        "--synthetic",
        metavar="TEXT",
        help="Render synthetic test image with this text instead of loading a file",
    )
    parser.add_argument(
        "--text",
        metavar="TEXT",
        help="Detect language directly from Unicode text (no image/OCR)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Where to save synthetic image (with --synthetic)",
    )
    parser.add_argument("--json", action="store_true", help="Print JSON result")
    parser.add_argument(
        "--show-support",
        action="store_true",
        help="Print supported language count before detection",
    )
    args = parser.parse_args()

    if args.show_support:
        summary = language_coverage_summary()
        print(
            f"Supported languages: {summary['total_supported']} "
            f"(script ID: {summary['script_detection_count']}, "
            f"native OCR: {summary['ocr_native_count']})"
        )

    detector = ImageLanguageDetector()

    if args.text:
        text = normalize_cli_text(args.text)
        result = detector.detect_from_text(text)
        image_note = None
    elif args.synthetic:
        text = normalize_cli_text(args.synthetic)
        if not text:
            parser.error("--synthetic TEXT cannot be empty")
        default_name = (
            "clang_ldl_synthetic.ppm"
            if is_ascii_ocr_text(text)
            else "clang_ldl_synthetic.png"
        )
        out = args.output or Path(default_name)
        image_path, renderer = render_synthetic_image(text, out)
        result = detector.detect_synthetic(text, image_path)
        image_note = f"Synthetic image: {image_path} ({renderer})"
    elif args.image:
        result = detector.detect_from_file(args.image)
        image_note = f"Image: {args.image}"
    else:
        parser.error("provide IMAGE, --synthetic TEXT, or --text TEXT")

    if args.json:
        payload = result.to_dict()
        if image_note:
            payload["image"] = image_note
        print(json.dumps(payload, indent=2, ensure_ascii=False))
    else:
        print(f"Native core: {detector.native_version}")
        if image_note:
            print(image_note)
        source = _SOURCE_LABELS.get(result.identification_source, result.identification_source)
        print(f"Identification: {source}")
        if result.identification_source == "unicode":
            print(f"Recognized text (Unicode input): {result.text!r}")
            if result.ocr_text.strip():
                print(f"Native OCR attempt (unreliable for this script): {result.ocr_text!r}")
            else:
                print("Native OCR text: (empty — script not supported by C++ templates yet)")
        else:
            print(f"Recognized text: {result.text!r}")
            if result.ocr_text != result.text:
                print(f"Native OCR text: {result.ocr_text!r}")
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
