#!/usr/bin/env python3
"""
Full-circle language detection from a text image.

Pipeline (image → text → language):
  1. Render or load a text image
  2. Native C++ OCR extracts Unicode text
  3. Python script analysis ranks likely languages

Run from the repo root (no pip install required when PYTHONPATH is set by make):

    make example-full-circle
    make test-example-full-circle
"""

from __future__ import annotations

import argparse
import json
import sys
import tempfile
import unicodedata
from dataclasses import dataclass
from pathlib import Path

_REPO = Path(__file__).resolve().parents[1]
_REPO_PYTHON = _REPO / "python"
if str(_REPO_PYTHON) not in sys.path:
    sys.path.insert(0, str(_REPO_PYTHON))

from clang_ldl import ImageLanguageDetector, language_coverage_summary, list_supported_languages  # noqa: E402
from clang_ldl.test_image import render_synthetic_image  # noqa: E402
from clang_ldl.text_utils import normalize_cli_text  # noqa: E402

_SOURCE_LABELS = {
    "ocr": "native C++ OCR",
    "unicode": "Unicode script analysis",
    "hybrid": "native OCR + script analysis",
}


@dataclass
class CircleStep:
    lang: str
    sample_text: str
    image_path: Path
    renderer: str
    ocr_text: str
    detected_code: str
    detected_name: str
    percent: int
    mean_confidence: float
    glyph_count: int
    identification_source: str
    ok: bool
    detail: str


def normalize_ocr(text: str) -> str:
    s = unicodedata.normalize("NFC", text)
    return "".join(ch for ch in s if not ch.isspace()).casefold()


def corpus_sample(lang: str) -> str:
    path = _REPO / "tests" / "ocr" / lang / "corpus.txt"
    if not path.is_file():
        raise FileNotFoundError(f"missing OCR corpus for {lang}: {path}")
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.strip():
            return line.strip()
    raise ValueError(f"empty corpus for {lang}")


def run_one(
    detector: ImageLanguageDetector,
    lang: str,
    text: str,
    *,
    work_dir: Path,
    strict: bool,
    quiet: bool,
) -> CircleStep:
    text = normalize_cli_text(text)
    image_path, renderer = render_synthetic_image(text, work_dir / f"{lang}_sample")

    result = detector.detect_synthetic(text, image_path)
    top = result.languages[0] if result.languages else None
    detected_code = top.code if top else ""
    detected_name = top.name if top else ""
    percent = top.percent if top else 0

    ok = detected_code == lang
    detail = "language match"
    if strict and result.identification_source in ("ocr", "hybrid"):
        if normalize_ocr(result.text) != normalize_ocr(text):
            ok = False
            detail = f"OCR mismatch: expected {text!r}, got {result.text!r}"

    if not quiet:
        source = _SOURCE_LABELS.get(result.identification_source, result.identification_source)
        status = "OK" if ok else "FAIL"
        print(f"[{status}] {lang}  sample={text!r}")
        print(f"       image: {image_path} ({renderer})")
        print(f"       OCR text: {result.text!r}  (confidence={result.mean_confidence:.2f}, glyphs={result.glyph_count})")
        print(f"       languages: {detected_name} ({detected_code}) {percent}%  [{source}]")
        if not ok:
            print(f"       reason: {detail}")
        print()

    return CircleStep(
        lang=lang,
        sample_text=text,
        image_path=image_path,
        renderer=renderer,
        ocr_text=result.text,
        detected_code=detected_code,
        detected_name=detected_name,
        percent=percent,
        mean_confidence=result.mean_confidence,
        glyph_count=result.glyph_count,
        identification_source=result.identification_source,
        ok=ok,
        detail=detail,
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Simulate image → OCR → language detection for clang-ldl"
    )
    parser.add_argument(
        "--lang",
        metavar="CODE",
        action="append",
        help="Run one language (repeatable). Default: all 21 native OCR languages.",
    )
    parser.add_argument("--text", metavar="TEXT", help="Custom sample text (requires --lang once)")
    parser.add_argument("--image", type=Path, help="Use an existing image instead of rendering")
    parser.add_argument("--all", action="store_true", help="Run all 21 languages (default without --lang)")
    parser.add_argument("--strict", action="store_true", help="Also require OCR text to match the sample")
    parser.add_argument("--json", action="store_true", help="Print machine-readable report")
    parser.add_argument("--quiet", action="store_true", help="Suppress per-language progress lines")
    args = parser.parse_args()

    detector = ImageLanguageDetector()

    if args.image:
        if not args.lang or len(args.lang) != 1:
            parser.error("--image requires exactly one --lang CODE")
        hint = normalize_cli_text(args.text) if args.text else None
        result = detector.detect_from_file(args.image, hint_text=hint)
        top = result.languages[0] if result.languages else None
        payload = {
            "lang_expected": args.lang[0],
            "image": str(args.image),
            "text": result.text,
            "languages": [lang.to_dict() for lang in result.languages],
            "mean_confidence": result.mean_confidence,
            "glyph_count": result.glyph_count,
            "identification_source": result.identification_source,
            "ok": top is not None and top.code == args.lang[0],
        }
        if args.json:
            print(json.dumps(payload, indent=2, ensure_ascii=False))
        else:
            print(f"Native core: {detector.native_version}")
            print(f"Image: {args.image}")
            print(f"Recognized text: {result.text!r}")
            if result.languages:
                lang = result.languages[0]
                print(f"Top language: {lang.name} ({lang.code}) — {lang.percent}%")
        return 0 if payload["ok"] else 1

    if args.text and (not args.lang or len(args.lang) != 1):
        parser.error("--text requires exactly one --lang CODE")

    langs = args.lang if args.lang else [lang.code for lang in list_supported_languages() if lang.ocr_native]
    if not langs:
        parser.error("no languages selected")

    if not args.quiet and not args.json:
        summary = language_coverage_summary()
        print("clang-ldl full-circle: image → OCR → language")
        print(f"Native library: {detector.native_version}")
        print(
            f"Coverage: {summary['ocr_native_count']}/{summary['total_supported']} languages with native OCR"
        )
        print()

    steps: list[CircleStep] = []
    with tempfile.TemporaryDirectory(prefix="clang_ldl_circle_") as td:
        work_dir = Path(td)
        for lang in langs:
            sample = normalize_cli_text(args.text) if args.text else corpus_sample(lang)
            steps.append(
                run_one(
                    detector,
                    lang,
                    sample,
                    work_dir=work_dir,
                    strict=args.strict,
                    quiet=args.quiet or args.json,
                )
            )

    failures = [s for s in steps if not s.ok]
    if args.json:
        print(
            json.dumps(
                {
                    "passed": len(steps) - len(failures),
                    "failed": len(failures),
                    "strict": args.strict,
                    "steps": [
                        {
                            "lang": s.lang,
                            "sample_text": s.sample_text,
                            "image": str(s.image_path),
                            "renderer": s.renderer,
                            "ocr_text": s.ocr_text,
                            "detected_code": s.detected_code,
                            "detected_name": s.detected_name,
                            "percent": s.percent,
                            "mean_confidence": s.mean_confidence,
                            "glyph_count": s.glyph_count,
                            "identification_source": s.identification_source,
                            "ok": s.ok,
                            "detail": s.detail,
                        }
                        for s in steps
                    ],
                },
                indent=2,
                ensure_ascii=False,
            )
        )
    elif failures:
        print(f"Failed: {len(failures)} / {len(steps)}")
        for step in failures:
            print(f"  {step.lang}: {step.detail}")
    else:
        print(f"All {len(steps)} full-circle checks passed.")

    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
