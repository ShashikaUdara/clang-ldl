#!/usr/bin/env python3
"""Aggregate OCR CER across all 21 native-OCR language corpora."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# (lang, max_cer) — must match Makefile test-ocr-* targets
LANG_SPECS: list[tuple[str, float]] = [
    ("en", 0.05),
    ("ru", 0.05),
    ("el", 0.05),
    ("hy", 0.05),
    ("ka", 0.05),
    ("he", 0.05),
    ("th", 0.05),
    ("lo", 0.05),
    ("my", 0.05),
    ("am", 0.05),
    ("hi", 0.08),
    ("bn", 0.08),
    ("pa", 0.08),
    ("gu", 0.08),
    ("or", 0.08),
    ("ta", 0.08),
    ("te", 0.08),
    ("kn", 0.08),
    ("ml", 0.08),
    ("si", 0.08),
    ("ar", 0.10),
]


def run_lang(lang: str, max_cer: float) -> tuple[str, int]:
    script = ROOT / "scripts" / ("test_ocr_en.py" if lang == "en" else "test_ocr_corpus.py")
    cmd = [sys.executable, str(script)]
    if lang != "en":
        cmd += [lang, "--max-cer", str(max_cer)]
    proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    line = ""
    for raw in (proc.stdout + proc.stderr).splitlines():
        if "CER=" in raw:
            line = raw.strip()
    return line, proc.returncode


def main() -> int:
    failures = 0
    print("clang-ldl OCR CER report (21 languages)")
    print("-" * 48)
    for lang, max_cer in LANG_SPECS:
        summary, code = run_lang(lang, max_cer)
        status = "OK" if code == 0 else "FAIL"
        if code != 0:
            failures += 1
        print(f"  {lang:4}  [{status}]  {summary or '(no output)'}")
    print("-" * 48)
    if failures:
        print(f"{failures} language(s) over CER budget")
        return 1
    print("All languages within CER budget")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
