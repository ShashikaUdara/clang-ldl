from __future__ import annotations

import unicodedata

# Curly/smart quotes often pasted from editors — normalize for CLI input.
_QUOTE_MAP = str.maketrans({
    "\u201c": '"',
    "\u201d": '"',
    "\u2018": "'",
    "\u2019": "'",
    "\u00ab": '"',
    "\u00bb": '"',
})


def normalize_cli_text(text: str) -> str:
    return text.strip().translate(_QUOTE_MAP)


def is_ascii_ocr_text(text: str) -> bool:
    """True when every letter/digit/punct is in the native Latin OCR set."""
    for char in text:
        if char.isspace():
            continue
        code = ord(char)
        if code < 32 or code > 126:
            return False
    return bool(text.strip())


def uses_native_ocr(text: str) -> bool:
    """True when text is covered by a loaded native OCR pack (all tiers + Latin)."""
    from clang_ldl.language_registry import SUPPORTED_LANGUAGES

    if not text.strip():
        return False
    if is_ascii_ocr_text(text):
        return True
    for ch in text:
        if ch.isspace() or not ch.isalpha():
            continue
        cp = ord(ch)
        for lang in SUPPORTED_LANGUAGES:
            if not lang.ocr_native:
                continue
            if any(r.contains(cp) for r in lang.unicode_ranges):
                break
        else:
            return False
    return True


def ocr_pack_id_for_text(text: str) -> str | None:
    """Map text to a native .clpk id when unambiguous (all native tiers + Latin)."""
    if not text.strip():
        return None
    if is_ascii_ocr_text(text):
        return "latin"

    pack_votes: dict[str, int] = {}
    for ch in text:
        if ch.isspace() or not ch.isalpha():
            continue
        cp = ord(ch)
        if 0x0400 <= cp <= 0x04FF:
            pack_votes["cyrillic"] = pack_votes.get("cyrillic", 0) + 1
        elif 0x0370 <= cp <= 0x03FF or 0x1F00 <= cp <= 0x1FFF:
            pack_votes["greek"] = pack_votes.get("greek", 0) + 1
        elif 0x0530 <= cp <= 0x058F:
            pack_votes["armenian"] = pack_votes.get("armenian", 0) + 1
        elif 0x10A0 <= cp <= 0x10FF or 0x2D00 <= cp <= 0x2D2F:
            pack_votes["georgian"] = pack_votes.get("georgian", 0) + 1
        elif 0x0590 <= cp <= 0x05FF:
            pack_votes["hebrew"] = pack_votes.get("hebrew", 0) + 1
        elif 0x0E00 <= cp <= 0x0E7F:
            pack_votes["thai"] = pack_votes.get("thai", 0) + 1
        elif 0x0E80 <= cp <= 0x0EFF:
            pack_votes["lao"] = pack_votes.get("lao", 0) + 1
        elif 0x1000 <= cp <= 0x109F or 0xAA60 <= cp <= 0xAA7F:
            pack_votes["myanmar"] = pack_votes.get("myanmar", 0) + 1
        elif 0x1200 <= cp <= 0x137F or 0x1380 <= cp <= 0x139F or 0x2D80 <= cp <= 0x2DDF:
            pack_votes["ethiopic"] = pack_votes.get("ethiopic", 0) + 1
        elif 0x0900 <= cp <= 0x097F:
            pack_votes["devanagari"] = pack_votes.get("devanagari", 0) + 1
        elif 0x0980 <= cp <= 0x09FF:
            pack_votes["bengali"] = pack_votes.get("bengali", 0) + 1
        elif 0x0A00 <= cp <= 0x0A7F:
            pack_votes["gurmukhi"] = pack_votes.get("gurmukhi", 0) + 1
        elif 0x0A80 <= cp <= 0x0AFF:
            pack_votes["gujarati"] = pack_votes.get("gujarati", 0) + 1
        elif 0x0B00 <= cp <= 0x0B7F:
            pack_votes["odia"] = pack_votes.get("odia", 0) + 1
        elif 0x0B80 <= cp <= 0x0BFF:
            pack_votes["tamil"] = pack_votes.get("tamil", 0) + 1
        elif 0x0C00 <= cp <= 0x0C7F:
            pack_votes["telugu"] = pack_votes.get("telugu", 0) + 1
        elif 0x0C80 <= cp <= 0x0CFF:
            pack_votes["kannada"] = pack_votes.get("kannada", 0) + 1
        elif 0x0D00 <= cp <= 0x0D7F:
            pack_votes["malayalam"] = pack_votes.get("malayalam", 0) + 1
        elif 0x0D80 <= cp <= 0x0DFF:
            pack_votes["sinhala"] = pack_votes.get("sinhala", 0) + 1
        else:
            return None
    if not pack_votes:
        return None
    return max(pack_votes, key=pack_votes.get)


def has_letter(text: str) -> bool:
    return any(unicodedata.category(ch).startswith("L") for ch in text)
