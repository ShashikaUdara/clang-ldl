#!/usr/bin/env python3
"""Verify all 21 languages report native OCR in the registry."""

from __future__ import annotations

from clang_ldl.language_registry import SUPPORTED_LANGUAGES, language_coverage_summary


def main() -> int:
    summary = language_coverage_summary()
    ocr_count = int(summary["ocr_native_count"])
    total = int(summary["total_supported"])
    missing = [lang.code for lang in SUPPORTED_LANGUAGES if not lang.ocr_native]

    print(f"Native OCR coverage: {ocr_count} / {total}")
    if missing:
        print("Missing ocr_native:", ", ".join(missing))
        return 1
    if ocr_count != 21:
        print(f"Expected 21 native OCR languages, got {ocr_count}")
        return 1
    print("verify-ocr-coverage passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
