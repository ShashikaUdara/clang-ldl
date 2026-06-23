from __future__ import annotations

from pathlib import Path

from clang_ldl._native import extract_text_from_file, extract_text_from_image_bytes, native_library
from clang_ldl.language_analyzer import LanguageAnalyzer
from clang_ldl.models import DetectionResult


class ImageLanguageDetector:
    """Detect languages present in an image by CV extraction + script analysis."""

    def __init__(self, analyzer: LanguageAnalyzer | None = None) -> None:
        self._analyzer = analyzer or LanguageAnalyzer()

    @property
    def native_version(self) -> str:
        lib = native_library()
        raw = lib.clang_ldl_version()
        return raw.decode("utf-8") if raw else "unknown"

    def detect_from_file(self, path: str | Path) -> DetectionResult:
        extraction = extract_text_from_file(path)
        return self._to_detection(extraction)

    def detect_from_pil_image(self, image) -> DetectionResult:
        rgb = image.convert("RGB")
        w, h = rgb.size
        data = rgb.tobytes()
        extraction = extract_text_from_image_bytes(data, w, h, 3)
        return self._to_detection(extraction)

    def detect_from_text(self, text: str) -> DetectionResult:
        languages = self._analyzer.detect(text, text.splitlines())
        return DetectionResult(text=text, languages=languages)

    def _to_detection(self, extraction) -> DetectionResult:
        lines = [line for line in extraction.text.splitlines() if line.strip()]
        languages = self._analyzer.detect(extraction.text, lines or None)
        return DetectionResult(
            text=extraction.text,
            languages=languages,
            mean_confidence=extraction.mean_confidence,
            glyph_count=extraction.glyph_count,
        )
