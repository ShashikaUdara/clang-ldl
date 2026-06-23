"""Unicode script ranges — derived from the language registry."""

from __future__ import annotations

from dataclasses import dataclass

from clang_ldl.language_registry import SUPPORTED_LANGUAGES, UnicodeRange


@dataclass(frozen=True)
class ScriptRange:
    code: str
    name: str
    start: int
    end: int


def _ranges_from_registry() -> tuple[ScriptRange, ...]:
    out: list[ScriptRange] = []
    for language in SUPPORTED_LANGUAGES:
        for block in language.unicode_ranges:
            out.append(
                ScriptRange(
                    code=language.code,
                    name=language.name,
                    start=block.start,
                    end=block.end,
                )
            )
    return tuple(out)


# Flattened ranges in registry priority order (specific scripts before Latin catch-all).
SCRIPT_RANGES: tuple[ScriptRange, ...] = _ranges_from_registry()

__all__ = ["ScriptRange", "SCRIPT_RANGES", "UnicodeRange"]
