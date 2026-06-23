#!/usr/bin/env python3
"""OCR golden tests for a language corpus — target CER <= 5% (Tier A)."""

from __future__ import annotations

import argparse
import sys
import tempfile
import unicodedata
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "python"))
sys.path.insert(0, str(ROOT / "tools"))

from clang_ldl import ImageLanguageDetector  # noqa: E402
from clang_ldl.test_image import render_ocr_corpus_image  # noqa: E402


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


def normalize_ocr(s: str) -> str:
    s = unicodedata.normalize("NFC", s)
    folded = "".join(ch for ch in s if not ch.isspace())
    return folded.casefold()


def load_corpus(lang: str) -> list[str]:
    path = ROOT / "tests" / "ocr" / lang / "corpus.txt"
    if not path.is_file():
        raise FileNotFoundError(f"missing corpus: {path}")
    return [line.strip() for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("lang", help="Language code (en, ru, el, hy, ka, he, th, lo, my, am, hi, bn, pa, gu, or, ta, te, kn, ml, si)")
    parser.add_argument("--max-cer", type=float, default=0.05, help="Maximum allowed CER")
    args = parser.parse_args()

    detector = ImageLanguageDetector()
    corpus = load_corpus(args.lang)
    total_chars = 0
    total_errors = 0
    failures: list[str] = []

    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        for text in corpus:
            img = tmp / f"{args.lang}_{hash(text)}.png"
            render_ocr_corpus_image(text, img, args.lang)
            result = detector.detect_synthetic(text, img)
            got = normalize_ocr(result.text)
            ref = normalize_ocr(text)
            dist = levenshtein(ref, got)
            total_chars += len(ref)
            total_errors += dist
            if got != ref:
                failures.append(f"  {text!r} -> {result.text!r} (dist={dist})")

    overall = total_errors / max(total_chars, 1)
    print(f"{args.lang} OCR corpus: {len(corpus)} strings, CER={overall:.2%}")
    if failures:
        print("Failures:")
        print("\n".join(failures))
        if overall > args.max_cer:
            return 1
        print(f"warning: failures within CER budget ({args.max_cer:.0%})")
    else:
        print(f"test-ocr-{args.lang} passed")
    return 0 if overall <= args.max_cer else 1


if __name__ == "__main__":
    raise SystemExit(main())
