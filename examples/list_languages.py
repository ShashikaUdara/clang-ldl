#!/usr/bin/env python3
"""List languages currently supported by clang-ldl."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

_REPO_PYTHON = Path(__file__).resolve().parents[1] / "python"
if str(_REPO_PYTHON) not in sys.path:
    sys.path.insert(0, str(_REPO_PYTHON))

from clang_ldl import language_coverage_summary, list_supported_languages, supported_language_count


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Show how many languages clang-ldl supports and list them"
    )
    parser.add_argument("--json", action="store_true", help="Print machine-readable report")
    parser.add_argument("--code", metavar="ISO", help="Show details for one language code")
    args = parser.parse_args()

    if args.code:
        from clang_ldl import get_supported_language

        lang = get_supported_language(args.code)
        if lang is None:
            print(f"Unknown language code: {args.code}", file=sys.stderr)
            return 1
        if args.json:
            print(json.dumps(lang.to_dict(), indent=2))
        else:
            print(f"{lang.name} ({lang.code})")
            print(f"  Script: {lang.script}")
            print(f"  Regions: {', '.join(lang.regions)}")
            print(f"  OCR from image (native): {'yes' if lang.ocr_native else 'script analysis only'}")
        return 0

    summary = language_coverage_summary()
    if args.json:
        print(json.dumps(summary, indent=2))
        return 0

    total = supported_language_count()
    print(f"clang-ldl supports {total} languages (goal: 21 = English + 20 others)")
    print()
    print(
        f"  Script-based identification: {summary['script_detection_count']} / {total}"
    )
    print(
        f"  Native image OCR (C++ templates): {summary['ocr_native_count']} / {total}"
    )
    print()
    print("Code  Language     Script                         OCR")
    print("----  -----------  -----------------------------  -----")
    for lang in list_supported_languages():
        ocr = "yes" if lang.ocr_native else "—"
        print(f"{lang.code:<4}  {lang.name:<11}  {lang.script:<29}  {ocr}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
