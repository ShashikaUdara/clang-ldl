#!/usr/bin/env python3
"""Minimal smoke tests — no pytest required."""

from clang_ldl import ImageLanguageDetector, LanguageAnalyzer, supported_language_count
from clang_ldl.language_registry import SUPPORTED_LANGUAGES


def main() -> None:
    assert supported_language_count() == 21

    analyzer = LanguageAnalyzer()
    for lang in SUPPORTED_LANGUAGES:
        result = analyzer.detect(lang.sample_char)
        assert result and result[0].code == lang.code, lang.code

    detector = ImageLanguageDetector()
    assert detector.detect_from_text("Hello").languages[0].code == "en"

    print("smoke tests passed")


if __name__ == "__main__":
    main()
