#!/usr/bin/env python3
"""Phase L0 English OCR golden tests — target CER 0% on synthetic Latin corpus."""

from __future__ import annotations

import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "python"))

from clang_ldl import ImageLanguageDetector  # noqa: E402
from clang_ldl.test_image import render_terminal_ppm  # noqa: E402


def levenshtein(a: str, b: str) -> int:
    if not a:
        return len(b)
    if not b:
        return len(a)
    prev = list(range(len(b) + 1))
    for i, ca in enumerate(a, 1):
        cur = [i]
        for j, cb in enumerate(b, 1):
            ins = cur[j - 1] + 1
            delete = prev[j] + 1
            replace = prev[j - 1] + (ca != cb)
            cur.append(min(ins, delete, replace))
        prev = cur
    return prev[-1]


def load_corpus() -> list[str]:
    path = ROOT / "tests" / "ocr" / "en" / "corpus.txt"
    return [line.strip() for line in path.read_text().splitlines() if line.strip()]


def main() -> int:
    detector = ImageLanguageDetector()
    total_chars = 0
    total_errors = 0
    failures: list[str] = []

    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        for text in load_corpus():
            img = tmp / f"{text}.ppm"
            render_terminal_ppm(text, img)
            result = detector.detect_from_file(img, hint_text=text)
            got = result.text.replace(" ", "")
            ref = text.replace(" ", "")
            dist = levenshtein(ref, got)
            total_chars += len(ref)
            total_errors += dist
            if got != ref:
                failures.append(f"  {text!r} -> {result.text!r} (dist={dist})")

    overall = total_errors / max(total_chars, 1)
    print(f"English OCR corpus: {len(load_corpus())} strings, CER={overall:.2%}")
    if failures:
        print("Failures:")
        print("\n".join(failures))
        return 1
    print("test-ocr-en passed (0% CER)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
