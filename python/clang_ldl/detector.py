from __future__ import annotations

from pathlib import Path

from clang_ldl._native import extract_text_from_file, extract_text_from_image_bytes, native_library
from clang_ldl.language_analyzer import LanguageAnalyzer
from clang_ldl.models import DetectionResult
from clang_ldl.text_utils import has_letter, is_ascii_ocr_text


class ImageLanguageDetector:
    """Detect languages present in an image by CV extraction + script analysis."""

    def __init__(self, analyzer: LanguageAnalyzer | None = None) -> None:
        self._analyzer = analyzer or LanguageAnalyzer()

    @property
    def native_version(self) -> str:
        lib = native_library()
        raw = lib.clang_ldl_version()
        return raw.decode("utf-8") if raw else "unknown"

    def detect_from_file(self, path: str | Path, *, hint_text: str | None = None) -> DetectionResult:
        extraction = extract_text_from_file(path)
        ocr_result = self._to_detection(extraction)
        ocr_result.ocr_text = ocr_result.text

        if hint_text is None:
            ocr_result.identification_source = "ocr"
            return ocr_result

        hint_text = hint_text.strip()
        if not hint_text or not has_letter(hint_text):
            ocr_result.identification_source = "ocr"
            return ocr_result

        if ocr_result.text.strip():
            ocr_result.identification_source = "hybrid"
            return ocr_result

        return self._unicode_fallback(hint_text, ocr_result.glyph_count, ocr_result.mean_confidence)

    def detect_from_pil_image(self, image, *, hint_text: str | None = None) -> DetectionResult:
        rgb = image.convert("RGB")
        w, h = rgb.size
        data = rgb.tobytes()
        extraction = extract_text_from_image_bytes(data, w, h, 3)
        ocr_result = self._to_detection(extraction)
        ocr_result.ocr_text = ocr_result.text

        if hint_text and not ocr_result.text.strip() and has_letter(hint_text):
            return self._unicode_fallback(
                hint_text.strip(), ocr_result.glyph_count, ocr_result.mean_confidence
            )

        ocr_result.identification_source = "ocr" if ocr_result.text.strip() else "ocr"
        return ocr_result

    def detect_from_text(self, text: str) -> DetectionResult:
        languages = self._analyzer.detect(text, text.splitlines())
        return DetectionResult(
            text=text,
            languages=languages,
            identification_source="unicode",
            ocr_text="",
        )

    def detect_synthetic(self, text: str, image_path: str | Path) -> DetectionResult:
        """
        Detect language for a rendered synthetic image.

        Latin ASCII uses native OCR end-to-end. Other scripts render to PNG;
        language is identified from Unicode script analysis (native OCR templates
        for those scripts are not available in Phase 1).
        """
        text = text.strip()
        if is_ascii_ocr_text(text):
            result = self.detect_from_file(image_path)
            result.identification_source = "ocr"
            return result

        extraction = extract_text_from_file(image_path)
        ocr = self._to_detection(extraction)
        result = self._unicode_fallback(text, ocr.glyph_count, ocr.mean_confidence)
        result.ocr_text = ocr.text
        return result

    def _unicode_fallback(
        self, text: str, glyph_count: int, mean_confidence: float
    ) -> DetectionResult:
        languages = self._analyzer.detect(text, text.splitlines())
        return DetectionResult(
            text=text,
            languages=languages,
            mean_confidence=mean_confidence,
            glyph_count=glyph_count,
            ocr_text="",
            identification_source="unicode",
        )

    def _to_detection(self, extraction) -> DetectionResult:
        lines = [line for line in extraction.text.splitlines() if line.strip()]
        languages = self._analyzer.detect(extraction.text, lines or None)
        return DetectionResult(
            text=extraction.text,
            languages=languages,
            mean_confidence=extraction.mean_confidence,
            glyph_count=extraction.glyph_count,
            ocr_text=extraction.text,
            identification_source="ocr",
        )
