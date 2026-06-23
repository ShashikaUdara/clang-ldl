# clang-ldl — Clang Language Detection Library

**clang-ldl** (Clang Language Detection Library) detects the language or languages present in an image that contains text. It maps visual glyphs to Unicode code points using computer vision and logical analysis, then infers languages from script and character distributions.

> **Pitch (non-technical):** Point the library at a photo or scan with writing on it — a sign, receipt, menu, or screenshot. It reads the shapes of the letters (not a cloud AI model in Phase 1), figures out which alphabet they belong to, and tells you “this is mostly English and a bit of Arabic” with confidence percentages.

---

## Pipeline Overview

```
┌─────────────┐    ┌──────────────────┐    ┌─────────────────┐    ┌──────────────────┐
│ Image input │ -> │ C/C++ CV core    │ -> │ Unicode string  │ -> │ Python language  │
│ (PNG/JPEG)  │    │ segment + match  │    │ + glyph metadata│    │ analysis + JSON  │
└─────────────┘    └──────────────────┘    └─────────────────┘    └──────────────────┘
```

| Stage | Layer | Responsibility |
|-------|-------|----------------|
| 1. Ingest | C++ | Load image, grayscale, denoise, binarize, optional deskew |
| 2. Layout | C++ | Detect text lines (horizontal projection), segment glyphs |
| 3. Recognition | C++ | Normalize glyphs, template-match to Unicode code points |
| 4. Language | Python | Map code points to scripts/languages, aggregate percentages |
| 5. Response | Python | Structured result: text, languages, confidence, diagnostics |

---

## Feature List

### Phase 1 — Computer vision & logical analysis (current)

| ID | Feature | Status |
|----|---------|--------|
| F1.1 | Image load (PNG, JPEG, BMP via stb) | Done |
| F1.2 | Grayscale conversion & Gaussian blur denoise | Done |
| F1.3 | Otsu automatic thresholding (binarization) | Done |
| F1.4 | Text line detection (horizontal projection profile) | Done |
| F1.5 | Glyph segmentation (connected components + vertical gaps) | Done |
| F1.6 | Glyph normalization (resize to fixed grid, center in bounding box) | Done |
| F1.7 | Template-based Latin recognition (A–Z, a–z, 0–9, punctuation) | Done |
| F1.8 | C API (`clang_ldl_extract_text`) for Python ctypes binding | Done |
| F1.9 | Python `LanguageAnalyzer` — Unicode script → language mapping | Done |
| F1.9b | 21-language registry (English + 20 scripts) with introspection API | Done |
| F1.10 | High-level `ImageLanguageDetector` API | Done |
| F1.11 | Example CLI test program with synthetic image generation | Done |
| F1.12 | Unit tests for Python language analysis | Done |
| F1.13 | Deskew (Hough / min-area rectangle) | Planned |
| F1.14 | Multi-script template packs (Cyrillic, Greek, Arabic blocks) | Planned — see [ocr-logical.md](ocr-logical.md) |
| F1.15 | Font metric hints (stroke width, x-height ratio) | Planned |
| F1.16 | Diagnostics API (line boxes, glyph boxes, match scores) | Partial |

### Phase 2 — ML & training (future)

| ID | Feature | Status |
|----|---------|--------|
| F2.1 | CRNN / transformer OCR model integration | Planned |
| F2.2 | Training pipeline (synthetic + labeled datasets) | Planned |
| F2.3 | Font-agnostic recognition | Planned |
| F2.4 | Handwriting support | Planned |
| F2.5 | GPU inference (ONNX Runtime / TensorRT) | Planned |
| F2.6 | Language ID model (fastText / CLD3) on recognized text | Planned |

### Phase 3 — Product hardening (future)

| ID | Feature | Status |
|----|---------|--------|
| F3.1 | Wheel packaging with prebuilt native binaries | Planned |
| F3.2 | Batch / folder processing | Planned |
| F3.3 | REST microservice wrapper | Planned |
| F3.4 | Mobile bindings (JNI / Swift) | Planned |

---

## Implementation Plan

### Architecture

```
clang-ldl/
├── docs/clang.md              # This document
├── native/                    # C/C++ computer vision core
│   ├── CMakeLists.txt
│   ├── include/clang_ldl/
│   └── src/
├── python/clang_ldl/          # Python logical analysis + bindings
├── examples/detect_language.py
├── tests/
└── scripts/build_native.sh
```

**Design principles**

- **Separation of concerns:** C++ owns pixels; Python owns linguistics.
- **Stable C ABI:** Python binds via `ctypes`; no C++ in the Python extension ABI.
- **Zero heavy deps in Phase 1:** `stb_image` for I/O; custom CV (no OpenCV required to build).
- **Deterministic:** Same image → same output; no network, no model weights.

---

### Phase 1 — CV + logical analysis

**Goal:** End-to-end language detection from clean, rendered Latin text images without ML.

| Milestone | Deliverable | ETA |
|-----------|-------------|-----|
| P1-M1 | Project scaffold, `clang.md`, CMake, C API header | Week 1 (Jun 23–29, 2026) |
| P1-M2 | Image pipeline (load, gray, blur, Otsu) | Week 1 |
| P1-M3 | Line + glyph segmentation | Week 2 (Jun 30–Jul 6, 2026) |
| P1-M4 | Latin template matcher + Unicode output | Week 2 |
| P1-M5 | Python `LanguageAnalyzer` + `ImageLanguageDetector` | Week 2 |
| P1-M6 | Example CLI, unit tests, README | Week 2 |
| P1-M7 | Deskew + extended script templates | Week 3 (Jul 7–13, 2026) |
| P1-M8 | Glyph diagnostics & confidence tuning | Week 3 |

**Phase 1 exit criteria**

- [x] Build native library with `make build` (or `./scripts/build_native.sh`)
- [x] `pip install -e python/` installs Python package
- [x] Example detects English from a synthetic PNG
- [x] pytest passes for language analyzer
- [ ] Recognize mixed Latin + Cyrillic on synthetic images (P1-M7)
- [ ] Deskew rotated scans up to ±15° (P1-M7)

---

### Phase 2 — ML & training

**Goal:** Replace template matching with trained models; support photos and varied fonts.

| Milestone | Deliverable | ETA |
|-----------|-------------|-----|
| P2-M1 | Dataset spec (synthetic renderer + ICDAR subsets) | Week 4–5 (Jul 14–27, 2026) |
| P2-M2 | PyTorch training loop for character classifier | Week 5–6 |
| P2-M3 | Export ONNX; C++ inference backend | Week 6–7 |
| P2-M4 | Sequence model (CRNN) for line-level OCR | Week 7–9 |
| P2-M5 | fastText language ID on OCR output | Week 9–10 |
| P2-M6 | Benchmark suite vs Phase 1 baseline | Week 10 |

**Phase 2 exit criteria**

- Word error rate &lt; 5% on rendered Latin test set
- Language ID F1 ≥ 0.90 on 10-language corpus
- Inference &lt; 200 ms per 640×480 image on CPU

---

### Phase 3 — Product hardening

**Goal:** Distributable library and optional service.

| Milestone | Deliverable | ETA |
|-----------|-------------|-----|
| P3-M1 | manylinux wheels + auditwheel | Week 11–12 (Aug 2026) |
| P3-M2 | Batch CLI + JSON Lines output | Week 12 |
| P3-M3 | FastAPI service + Docker image | Week 13 |
| P3-M4 | Documentation site & cookbook | Week 14 |

---

## Progress Log

| Date | Update |
|------|--------|
| 2026-06-23 | Created `clang.md`, project scaffold, C++ core (pipeline, segmentation, Latin templates), C API, Python package, example CLI, unit tests. Milestones P1-M1 through P1-M6 implemented. |
| 2026-06-23 | Fixed foreground inversion, glyph merge heuristics, template/grid alignment (20×28 @ 4× scale). Synthetic PPM tests recognize `HELLO`, `ABC`, `THE` at ~0.66 mean confidence. |
| 2026-06-23 | Added `language_registry.py` with 21 languages (English + 20 target scripts). Introspection via `supported_language_count()`, `list_supported_languages()`, `language_coverage_summary()`, and `examples/list_languages.py`. |

---

## Supported Languages (21)

clang-ldl identifies **21 languages**: English plus the 20 scripts in the project ambition list.

| # | Code | Language | Script | Image OCR (Phase 1) |
|---|------|----------|--------|---------------------|
| 1 | en | English | Latin | Yes (native templates) |
| 2 | ru | Russian | Cyrillic | Script analysis |
| 3 | ar | Arabic | Arabic | Script analysis |
| 4 | he | Hebrew | Hebrew | Script analysis |
| 5 | el | Greek | Greek | Script analysis |
| 6 | hy | Armenian | Armenian | Script analysis |
| 7 | ka | Georgian | Georgian (Mkhedruli) | Script analysis |
| 8 | hi | Hindi | Devanagari | Script analysis |
| 9 | bn | Bengali | Bengali–Assamese | Script analysis |
| 10 | pa | Punjabi | Gurmukhi | Script analysis |
| 11 | gu | Gujarati | Gujarati | Script analysis |
| 12 | or | Odia | Odia | Script analysis |
| 13 | ta | Tamil | Tamil | Script analysis |
| 14 | te | Telugu | Telugu | Script analysis |
| 15 | kn | Kannada | Kannada | Script analysis |
| 16 | ml | Malayalam | Malayalam | Script analysis |
| 17 | si | Sinhala | Sinhala | Script analysis |
| 18 | th | Thai | Thai | Script analysis |
| 19 | lo | Lao | Lao | Script analysis |
| 20 | my | Burmese | Burmese | Script analysis |
| 21 | am | Amharic | Ge'ez (Ethiopic) | Script analysis |

**How to check support at runtime:**

```python
from clang_ldl import supported_language_count, language_coverage_summary

print(supported_language_count())  # 21
print(language_coverage_summary())
```

```bash
python examples/list_languages.py
python examples/list_languages.py --json
python examples/list_languages.py --code hi
```

**Important distinction**

- **Script identification (all 21):** Once text is available as Unicode — from OCR, user input, or another engine — clang-ldl maps characters to languages.
- **Image OCR (English only today):** The C++ template matcher reads Latin glyphs from images. Other scripts need the [logical OCR roadmap](ocr-logical.md) (non-AI template packs + rule-based segmentation).


## API Sketch

### Python

```python
from clang_ldl import ImageLanguageDetector, supported_language_count

print(supported_language_count())  # 21

detector = ImageLanguageDetector()
result = detector.detect_from_file("sign.png")
print(result.text)        # recognized Unicode string
print(result.languages)   # [{"code": "en", "name": "English", "percent": 100, ...}]

# Unicode text path (works for all 21 scripts)
result = detector.detect_from_text("Привет हिन्दी")
```

### C

```c
#include "clang_ldl/api.h"

ClangLdlResult result = {0};
if (clang_ldl_extract_text("image.png", &result) == CLANG_LDL_OK) {
    printf("%s\n", result.text);
    clang_ldl_free_result(&result);
}
```

---

## Limitations (Phase 1)

- Best **image OCR** results on high-contrast, horizontal, printed **Latin** text.
- All **21 languages** are supported for **script-based identification** when Unicode text is available.
- Handwriting, heavy noise, rotation, and decorative fonts reduce OCR accuracy.
- Non-Latin **image** recognition follows the [logical OCR roadmap](ocr-logical.md); ML is optional in Phase 2.
- Language detection is **script-based** (e.g. Cyrillic → Russian, Devanagari → Hindi), not semantic disambiguation across languages that share a script.

---

## References

- Unicode Standard — script property ranges
- Otsu, N. (1979). A threshold selection method from gray-level histograms.
- Phase 2 targets: ICDAR datasets, ONNX Runtime, fastText language identification.
