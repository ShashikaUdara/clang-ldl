from clang_ldl import ImageLanguageDetector, LanguageAnalyzer


def test_english_from_text():
    detector = ImageLanguageDetector()
    result = detector.detect_from_text("Hello world from clang-ldl")
    assert result.languages
    assert result.languages[0].code == "en"
    assert result.languages[0].percent == 100


def test_empty_text():
    analyzer = LanguageAnalyzer()
    assert analyzer.detect("") == []


def test_mixed_script_ranges():
    analyzer = LanguageAnalyzer()
    langs = analyzer.detect("English русский")
    codes = {lang.code for lang in langs}
    assert "en" in codes
    assert "ru" in codes


def test_synthetic_image_pipeline(tmp_path):
    from clang_ldl.test_image import render_terminal_ppm

    path = tmp_path / "hello.ppm"
    render_terminal_ppm("HELLO", path)

    detector = ImageLanguageDetector()
    result = detector.detect_from_file(path)
    assert result.glyph_count >= 1
    assert "H" in result.text or "HELLO" in result.text.replace(" ", "")
    assert result.languages
    assert result.languages[0].code == "en"
