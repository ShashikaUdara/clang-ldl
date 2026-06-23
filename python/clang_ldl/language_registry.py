from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class UnicodeRange:
    start: int
    end: int

    def contains(self, codepoint: int) -> bool:
        return self.start <= codepoint <= self.end


@dataclass(frozen=True)
class SupportedLanguage:
    """A language clang-ldl can identify from Unicode script analysis."""

    code: str
    name: str
    script: str
    regions: tuple[str, ...]
    unicode_ranges: tuple[UnicodeRange, ...]
    sample_char: str
    ocr_native: bool = False

    def to_dict(self) -> dict[str, object]:
        return {
            "code": self.code,
            "name": self.name,
            "script": self.script,
            "regions": list(self.regions),
            "unicode_ranges": [
                {"start": f"U+{r.start:04X}", "end": f"U+{r.end:04X}"}
                for r in self.unicode_ranges
            ],
            "sample_char": self.sample_char,
            "ocr_native": self.ocr_native,
            "script_detection": True,
        }


# English + 20 target languages. Order defines match priority (first wins on overlap).
SUPPORTED_LANGUAGES: tuple[SupportedLanguage, ...] = (
    SupportedLanguage(
        code="en",
        name="English",
        script="Latin alphabet",
        regions=("Global", "United States", "United Kingdom", "Australia"),
        unicode_ranges=(
            UnicodeRange(0x0041, 0x007A),
            UnicodeRange(0x00C0, 0x00FF),
            UnicodeRange(0x0100, 0x024F),
        ),
        sample_char="A",
        ocr_native=True,
    ),
    SupportedLanguage(
        code="ru",
        name="Russian",
        script="Cyrillic alphabet",
        regions=("Russia", "Eastern Europe", "Central Asia"),
        unicode_ranges=(
            UnicodeRange(0x0400, 0x04FF),
            UnicodeRange(0x0500, 0x052F),
        ),
        sample_char="а",
    ),
    SupportedLanguage(
        code="ar",
        name="Arabic",
        script="Arabic alphabet",
        regions=("Middle East", "North Africa"),
        unicode_ranges=(
            UnicodeRange(0x0600, 0x06FF),
            UnicodeRange(0x0750, 0x077F),
            UnicodeRange(0x08A0, 0x08FF),
            UnicodeRange(0xFB50, 0xFDFF),
            UnicodeRange(0xFE70, 0xFEFF),
        ),
        sample_char="ا",
    ),
    SupportedLanguage(
        code="he",
        name="Hebrew",
        script="Hebrew alphabet",
        regions=("Israel",),
        unicode_ranges=(UnicodeRange(0x0590, 0x05FF),),
        sample_char="א",
    ),
    SupportedLanguage(
        code="el",
        name="Greek",
        script="Greek alphabet",
        regions=("Greece", "Cyprus"),
        unicode_ranges=(
            UnicodeRange(0x0370, 0x03FF),
            UnicodeRange(0x1F00, 0x1FFF),
        ),
        sample_char="α",
    ),
    SupportedLanguage(
        code="hy",
        name="Armenian",
        script="Armenian alphabet",
        regions=("Armenia",),
        unicode_ranges=(UnicodeRange(0x0530, 0x058F),),
        sample_char="ա",
    ),
    SupportedLanguage(
        code="ka",
        name="Georgian",
        script="Georgian script (Mkhedruli)",
        regions=("Georgia",),
        unicode_ranges=(
            UnicodeRange(0x10A0, 0x10FF),
            UnicodeRange(0x2D00, 0x2D2F),
        ),
        sample_char="ა",
    ),
    SupportedLanguage(
        code="hi",
        name="Hindi",
        script="Devanagari",
        regions=("India",),
        unicode_ranges=(UnicodeRange(0x0900, 0x097F),),
        sample_char="ह",
    ),
    SupportedLanguage(
        code="bn",
        name="Bengali",
        script="Bengali–Assamese script",
        regions=("Bangladesh", "India (West Bengal)"),
        unicode_ranges=(UnicodeRange(0x0980, 0x09FF),),
        sample_char="ব",
    ),
    SupportedLanguage(
        code="pa",
        name="Punjabi",
        script="Gurmukhi script",
        regions=("India (Punjab)",),
        unicode_ranges=(UnicodeRange(0x0A00, 0x0A7F),),
        sample_char="ਪ",
    ),
    SupportedLanguage(
        code="gu",
        name="Gujarati",
        script="Gujarati script",
        regions=("India (Gujarat)",),
        unicode_ranges=(UnicodeRange(0x0A80, 0x0AFF),),
        sample_char="ગ",
    ),
    SupportedLanguage(
        code="or",
        name="Odia",
        script="Odia script",
        regions=("India (Odisha)",),
        unicode_ranges=(UnicodeRange(0x0B00, 0x0B7F),),
        sample_char="ଓ",
    ),
    SupportedLanguage(
        code="ta",
        name="Tamil",
        script="Tamil script",
        regions=("India (Tamil Nadu)", "Sri Lanka", "Singapore"),
        unicode_ranges=(UnicodeRange(0x0B80, 0x0BFF),),
        sample_char="த",
    ),
    SupportedLanguage(
        code="te",
        name="Telugu",
        script="Telugu script",
        regions=("India (Andhra Pradesh)", "India (Telangana)"),
        unicode_ranges=(UnicodeRange(0x0C00, 0x0C7F),),
        sample_char="త",
    ),
    SupportedLanguage(
        code="kn",
        name="Kannada",
        script="Kannada script",
        regions=("India (Karnataka)",),
        unicode_ranges=(UnicodeRange(0x0C80, 0x0CFF),),
        sample_char="ಕ",
    ),
    SupportedLanguage(
        code="ml",
        name="Malayalam",
        script="Malayalam script",
        regions=("India (Kerala)",),
        unicode_ranges=(UnicodeRange(0x0D00, 0x0D7F),),
        sample_char="മ",
    ),
    SupportedLanguage(
        code="si",
        name="Sinhala",
        script="Sinhala script",
        regions=("Sri Lanka",),
        unicode_ranges=(UnicodeRange(0x0D80, 0x0DFF),),
        sample_char="ස",
    ),
    SupportedLanguage(
        code="th",
        name="Thai",
        script="Thai script",
        regions=("Thailand",),
        unicode_ranges=(UnicodeRange(0x0E00, 0x0E7F),),
        sample_char="ท",
    ),
    SupportedLanguage(
        code="lo",
        name="Lao",
        script="Lao script",
        regions=("Laos",),
        unicode_ranges=(UnicodeRange(0x0E80, 0x0EFF),),
        sample_char="ລ",
    ),
    SupportedLanguage(
        code="my",
        name="Burmese",
        script="Burmese script",
        regions=("Myanmar",),
        unicode_ranges=(
            UnicodeRange(0x1000, 0x109F),
            UnicodeRange(0xAA60, 0xAA7F),
        ),
        sample_char="မ",
    ),
    SupportedLanguage(
        code="am",
        name="Amharic",
        script="Ge'ez (Ethiopic) script",
        regions=("Ethiopia",),
        unicode_ranges=(
            UnicodeRange(0x1200, 0x137F),
            UnicodeRange(0x1380, 0x139F),
            UnicodeRange(0x2D80, 0x2DDF),
        ),
        sample_char="አ",
    ),
)

_LANGUAGES_BY_CODE: dict[str, SupportedLanguage] = {lang.code: lang for lang in SUPPORTED_LANGUAGES}


def supported_language_count() -> int:
    """Return how many languages clang-ldl can identify."""
    return len(SUPPORTED_LANGUAGES)


def list_supported_languages() -> list[SupportedLanguage]:
    """Return all supported languages in registry order."""
    return list(SUPPORTED_LANGUAGES)


def get_supported_language(code: str) -> SupportedLanguage | None:
    """Look up a language by ISO 639-1 code (e.g. ``ru``, ``hi``)."""
    return _LANGUAGES_BY_CODE.get(code.lower())


def language_for_codepoint(codepoint: int) -> SupportedLanguage | None:
    """Map a Unicode code point to the first matching supported language."""
    for language in SUPPORTED_LANGUAGES:
        for block in language.unicode_ranges:
            if block.contains(codepoint):
                return language
    return None


def language_coverage_summary() -> dict[str, object]:
    """Summarize identification capabilities for tooling and CLI."""
    total = supported_language_count()
    ocr_native = sum(1 for lang in SUPPORTED_LANGUAGES if lang.ocr_native)
    script_only = total - ocr_native
    return {
        "total_supported": total,
        "target_ambition": 21,
        "script_detection_count": total,
        "ocr_native_count": ocr_native,
        "script_only_count": script_only,
        "meets_20_plus_english_goal": total >= 21,
        "languages": [lang.to_dict() for lang in SUPPORTED_LANGUAGES],
    }
