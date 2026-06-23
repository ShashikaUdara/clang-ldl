from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class ScriptRange:
    code: str
    name: str
    start: int
    end: int


# Specific scripts are checked before broader Latin ranges.
SCRIPT_RANGES: tuple[ScriptRange, ...] = (
    ScriptRange("si", "Sinhala", 0x0D80, 0x0DFF),
    ScriptRange("ta", "Tamil", 0x0B80, 0x0BFF),
    ScriptRange("hi", "Hindi", 0x0900, 0x097F),
    ScriptRange("bn", "Bengali", 0x0980, 0x09FF),
    ScriptRange("th", "Thai", 0x0E00, 0x0E7F),
    ScriptRange("ar", "Arabic", 0x0600, 0x06FF),
    ScriptRange("he", "Hebrew", 0x0590, 0x05FF),
    ScriptRange("ru", "Russian", 0x0400, 0x04FF),
    ScriptRange("el", "Greek", 0x0370, 0x03FF),
    ScriptRange("ja", "Japanese", 0x3040, 0x30FF),
    ScriptRange("ko", "Korean", 0xAC00, 0xD7AF),
    ScriptRange("zh", "Chinese", 0x4E00, 0x9FFF),
    ScriptRange("en", "English", 0x0041, 0x007A),
    ScriptRange("en", "English", 0x00C0, 0x00FF),
    ScriptRange("en", "English", 0x0100, 0x024F),
)
