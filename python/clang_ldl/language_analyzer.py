from __future__ import annotations

import unicodedata

from clang_ldl.models import DetectedLanguage
from clang_ldl.script_ranges import SCRIPT_RANGES


class LanguageAnalyzer:
    """Map Unicode text to likely languages using script ranges."""

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
                script = self._script_for_char(char)
                if script is None:
                    continue
                code, name = script
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
                reliable=True,
            )
            for code, (name, count) in counts.items()
        ]
        return sorted(languages, key=lambda lang: (-lang.percent, lang.name))

    def _script_for_char(self, char: str) -> tuple[str, str] | None:
        if not unicodedata.category(char).startswith("L"):
            return None

        codepoint = ord(char)
        for script in SCRIPT_RANGES:
            if script.start <= codepoint <= script.end:
                return script.code, script.name

        if "LATIN" in unicodedata.name(char, ""):
            return "en", "English"

        return None
