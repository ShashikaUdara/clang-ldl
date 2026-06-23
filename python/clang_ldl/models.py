from __future__ import annotations

from dataclasses import dataclass, field


@dataclass(frozen=True)
class DetectedLanguage:
    code: str
    name: str
    percent: int
    reliable: bool = True

    def to_dict(self) -> dict[str, object]:
        return {
            "code": self.code,
            "name": self.name,
            "percent": self.percent,
            "reliable": self.reliable,
        }


@dataclass
class ExtractionResult:
    text: str
    mean_confidence: float
    glyph_count: int


@dataclass
class DetectionResult:
    text: str
    languages: list[DetectedLanguage] = field(default_factory=list)
    mean_confidence: float = 0.0
    glyph_count: int = 0
    ocr_text: str = ""
    identification_source: str = "ocr"

    def to_dict(self) -> dict[str, object]:
        return {
            "text": self.text,
            "languages": [lang.to_dict() for lang in self.languages],
            "mean_confidence": self.mean_confidence,
            "glyph_count": self.glyph_count,
            "ocr_text": self.ocr_text,
            "identification_source": self.identification_source,
        }
