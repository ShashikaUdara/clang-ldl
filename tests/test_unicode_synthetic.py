from clang_ldl import ImageLanguageDetector
from clang_ldl.test_image import render_synthetic_image
from clang_ldl.text_utils import normalize_cli_text


def test_sinhala_synthetic(tmp_path):
    text = "මෙවලම්"
    path, renderer = render_synthetic_image(text, tmp_path / "sinhala.png")
    assert renderer == "unicode_png"

    detector = ImageLanguageDetector()
    result = detector.detect_synthetic(text, path)
    assert result.identification_source == "unicode"
    assert result.languages
    assert result.languages[0].code == "si"


def test_hindi_synthetic(tmp_path):
    text = "नमस्ते"
    path, renderer = render_synthetic_image(text, tmp_path / "hindi.png")
    assert renderer == "unicode_png"

    detector = ImageLanguageDetector()
    result = detector.detect_synthetic(text, path)
    assert result.identification_source == "unicode"
    assert result.languages
    assert result.languages[0].code == "hi"


def test_curly_quotes_normalized():
    text = normalize_cli_text("\u201cनमस्ते\u201d")
    assert text == '"नमस्ते"'


def test_detect_from_text_flag_equivalent():
    detector = ImageLanguageDetector()
    si = detector.detect_from_text("මෙවලම්")
    hi = detector.detect_from_text("नमस्ते")
    assert si.languages[0].code == "si"
    assert hi.languages[0].code == "hi"
