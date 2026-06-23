from __future__ import annotations

import unicodedata

from clang_ldl.language_registry import SUPPORTED_LANGUAGES, language_for_codepoint
from clang_ldl.models import DetectedLanguage


class LanguageAnalyzer:
    """Map Unicode text to supported languages using script-aware code point ranges."""

    def __init__(self) -> None:
        self._supported_codes = {lang.code for lang in SUPPORTED_LANGUAGES}

    @property
    def supported_language_count(self) -> int:
        return len(SUPPORTED_LANGUAGES)

    def detect(self, text: str, lines: list[str] | None = None) -> list[DetectedLanguage]:
        counts: dict[str, tuple[str, int]] = {}
        if lines:
            samples = [line for line in lines if line and line.strip()]
        elif text and text.strip():
            samples = [text]
        else:
            samples = []

        for sample in samples:
            for char in sample:
                language = self._language_for_char(char)
                if language is None:
                    continue
                code, name = language
                if code not in counts:
                    counts[code] = (name, 0)
                counts[code] = (name, counts[code][1] + 1)

        total = sum(count for _, count in counts.values())
        if total == 0:
            return []

        languages = [
            DetectedLanguage(
                code=code,
                name=name,
                percent=round((count / total) * 100),
                reliable=code in self._supported_codes,
            )
            for code, (name, count) in counts.items()
        ]
        return sorted(languages, key=lambda lang: (-lang.percent, lang.name))

    def _language_for_char(self, char: str) -> tuple[str, str] | None:
        if not unicodedata.category(char).startswith("L"):
            return None

        matched = language_for_codepoint(ord(char))
        if matched is not None:
            return matched.code, matched.name

        if "LATIN" in unicodedata.name(char, ""):
            return "en", "English"

        return None
