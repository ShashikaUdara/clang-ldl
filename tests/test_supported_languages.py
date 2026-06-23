import pytest

from clang_ldl import (
    LanguageAnalyzer,
    SUPPORTED_LANGUAGES,
    get_supported_language,
    language_coverage_summary,
    list_supported_languages,
    supported_language_count,
)
from clang_ldl.language_registry import language_for_codepoint


def test_supported_language_count_is_21():
    assert supported_language_count() == 21


def test_meets_ambition_goal():
    summary = language_coverage_summary()
    assert summary["meets_20_plus_english_goal"] is True
    assert summary["total_supported"] == 21


def test_registry_codes_unique():
    codes = [lang.code for lang in SUPPORTED_LANGUAGES]
    assert len(codes) == len(set(codes))


@pytest.mark.parametrize(
    "code,name",
    [
        ("en", "English"),
        ("ru", "Russian"),
        ("ar", "Arabic"),
        ("he", "Hebrew"),
        ("el", "Greek"),
        ("hy", "Armenian"),
        ("ka", "Georgian"),
        ("hi", "Hindi"),
        ("bn", "Bengali"),
        ("pa", "Punjabi"),
        ("gu", "Gujarati"),
        ("or", "Odia"),
        ("ta", "Tamil"),
        ("te", "Telugu"),
        ("kn", "Kannada"),
        ("ml", "Malayalam"),
        ("si", "Sinhala"),
        ("th", "Thai"),
        ("lo", "Lao"),
        ("my", "Burmese"),
        ("am", "Amharic"),
    ],
)
def test_each_language_detected_from_sample_char(code: str, name: str):
    lang = get_supported_language(code)
    assert lang is not None
    assert lang.name == name

    analyzer = LanguageAnalyzer()
    result = analyzer.detect(lang.sample_char)
    assert result
    assert result[0].code == code
    assert result[0].percent == 100


@pytest.mark.parametrize("lang", list_supported_languages())
def test_sample_char_maps_to_language(lang):
    matched = language_for_codepoint(ord(lang.sample_char))
    assert matched is not None
    assert matched.code == lang.code


def test_mixed_multilingual_text():
    samples = [lang.sample_char for lang in SUPPORTED_LANGUAGES[:5]]
    text = " ".join(samples)
    analyzer = LanguageAnalyzer()
    detected = {lang.code for lang in analyzer.detect(text)}
    assert detected == {"en", "ru", "ar", "he", "el"}


def test_get_supported_language_unknown():
    assert get_supported_language("zz") is None


def test_only_english_has_native_ocr():
    ocr_langs = [lang for lang in SUPPORTED_LANGUAGES if lang.ocr_native]
    assert len(ocr_langs) == 1
    assert ocr_langs[0].code == "en"
