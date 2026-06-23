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


def has_letter(text: str) -> bool:
    return any(unicodedata.category(ch).startswith("L") for ch in text)
