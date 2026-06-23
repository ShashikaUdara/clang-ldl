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
    """True when text is covered by a loaded native OCR pack (Tier A + Latin)."""
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
    """Map text to a native .clpk id when unambiguous (Tier A + Latin)."""
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
        else:
            return None
    if not pack_votes:
        return None
    return max(pack_votes, key=pack_votes.get)


def has_letter(text: str) -> bool:
    return any(unicodedata.category(ch).startswith("L") for ch in text)
