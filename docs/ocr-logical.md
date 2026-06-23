# clang-ldl Logical OCR Roadmap

**Non-AI, computer-vision OCR for 21 languages**

This document is the engineering plan to extend clang-ldl from **English-only image OCR** to **full logical OCR** for all 20 remaining languages — without neural networks, training pipelines, or cloud models.

> **Plain-language pitch:** We teach the library to read letters the way a human engineer would explain it to a computer: measure shapes, count holes, compare strokes to known patterns, and apply script-specific rules. No “black box” AI — every decision is traceable math and geometry.

**Related docs:** [clang.md](clang.md) (project overview) · [language_registry.py](../python/clang_ldl/language_registry.py) (21 languages)

---

## 1. Current state vs target

| Capability | Today | Target |
|------------|-------|--------|
| Languages with **script ID** (Unicode text) | 21 / 21 | 21 / 21 (done) |
| Languages with **image OCR** (C++ core) | 1 / 21 (Latin) | 21 / 21 |
| Approach | Template match on 5×7 Latin bitmaps | Multi-script template packs + structural descriptors |
| Segmentation | Horizontal lines + connected components | Script-aware layout rules |
| Determinism | Yes | Yes (same image → same output) |

**Gap:** Stages 1–2 (ingest, binarize, segment lines) are largely script-agnostic. Stage 3 (glyph → Unicode) exists only for ASCII Latin. Stage 4 (language ID) is complete.

---

## 2. Design philosophy: logical OCR

### What “non-AI” means here

| Allowed | Not in scope for this roadmap |
|---------|-------------------------------|
| Thresholding, projections, morphology | Convolutional neural networks |
| Template / prototype matching | Transformer OCR |
| Geometric moments, Euler number | Training datasets + backprop |
| Skeleton graphs, DTW on stroke order | LLM vision APIs |
| Rule-based script layout (RTL, shirorekha) | |
| Synthetic font atlas generation (offline build tool) | |
| Weighted score fusion (explicit formula) | |

### Core recognition equation

For each segmented glyph bitmap \(G\) and candidate character prototype \(T_i\):

\[
\text{score}(G, T_i) = w_1 \cdot \text{NCC}(G, T_i) + w_2 \cdot \text{struct}(G, T_i) + w_3 \cdot \text{aspect}(G, T_i)
\]

Where:

- **NCC** — normalized cross-correlation of binarized, size-normalized bitmaps
- **struct** — structural similarity: hole count, endpoint count, junction count after skeletonization
- **aspect** — penalty for width/height ratio mismatch vs prototype

The winning codepoint is \(\arg\max_i \text{score}(G, T_i)\) if \(\max > \tau_{\text{script}}\).

All weights and thresholds are **per-script constants** stored in pack metadata — tunable, testable, no learning.

### Architecture extension

```
┌──────────────┐     ┌─────────────────────┐     ┌──────────────────────┐
│ Image        │ --> │ Shared CV pipeline  │ --> │ Script classifier    │
│              │     │ (gray, Otsu, deskew)│     │ (geometry + Unicode  │
└──────────────┘     └─────────────────────┘     │  block priors)       │
                                                  └──────────┬───────────┘
                                                             │
                         ┌───────────────────────────────────┼────────────────────────┐
                         v                                   v                        v
                 ┌───────────────┐                  ┌───────────────┐         ┌───────────────┐
                 │ Latin pack    │                  │ Indic pack    │         │ Arabic pack   │
                 │ (en) ✓        │                  │ (hi,bn,…)     │         │ (ar)          │
                 └───────┬───────┘                  └───────┬───────┘         └───────┬───────┘
                         │                                   │                        │
                         └───────────────────┬───────────────┴────────────────────────┘
                                             v
                                    ┌─────────────────┐
                                    │ Unicode string  │ --> Python language ID (done)
                                    └─────────────────┘
```

---

## 3. Script families — complexity tiers

Scripts are grouped by **segmentation difficulty** and **glyph topology**, not just language count.

### Tier A — Alphabetic, discrete glyphs (easiest)

| Code | Language | Script | Chars (approx.) | Notes |
|------|----------|--------|-----------------|-------|
| ru | Russian | Cyrillic | ~66 letters | Same pipeline as Latin; larger template pack |
| el | Greek | Greek | ~48 letters | Diacritics as optional secondary pass |
| hy | Armenian | Armenian | ~76 letters | Distinct letterforms |
| ka | Georgian | Mkhedruli | ~33 letters | Rounded shapes; good for template NCC |

**Strategy:** Extend current `glyph_matcher.cpp` pattern — one prototype per codepoint, fixed grid normalization.

**ETA:** 3–4 weeks after foundation (see §6).

---

### Tier B — Complex layout, mostly discrete letters

| Code | Language | Script | Notes |
|------|----------|--------|-------|
| he | Hebrew | Hebrew | RTL line reorder; final form variants |
| th | Thai | Thai | Tall ascenders; vowel marks above/below |
| lo | Lao | Lao | Similar to Thai; smaller alphabet |
| my | Burmese | Myanmar | Stacked characters; circular components |
| am | Amharic | Ethiopic | Syllabic units (consonant+vowel fused) |

**Strategy:**

- **RTL pass** after line recognition: reverse glyph order per line (Hebrew, Arabic).
- **Thai/Lao:** segment main consonant body first; attach mark blobs by vertical offset rules.
- **Ethiopic:** treat each syllable cell as one template (not bare consonant).

**ETA:** 4–6 weeks after Tier A.

---

### Tier C — Indic scripts (hardest logical OCR)

| Code | Language | Script | Shared issues |
|------|----------|--------|---------------|
| hi | Hindi | Devanagari | Shirorekha (headline), matras, conjuncts |
| bn | Bengali | Bengali | Similar topology; different curves |
| pa | Punjabi | Gurmukhi | Devanagari-like; fewer conjuncts |
| gu | Gujarati | Gujarati | Headline can be broken |
| or | Odia | Odia | Rounded matras |
| ta | Tamil | Tamil | Simpler conjunct rules |
| te | Telugu | Telugu | Many compound glyphs |
| kn | Kannada | Kannada | Ottakshara (below-base conjuncts) |
| ml | Malayalam | Malayalam | Complex stacked forms |
| si | Sinhala | Sinhala | Partly Indic; extra ligatures |

**Strategy (rule-based, no ML):**

1. **Shirorekha detection** — horizontal Hough / row-density peak in upper third of line crop; subtract headline before matching consonants.
2. **Akshara segmentation** — cluster consonant + attached matras into single logical units using vertical overlap rules (not one connected component = one char).
3. **Matra classification** — small blobs above/below/beside base → vowel sign lookup table by relative `(dx, dy)` bins.
4. **Conjunct splitting** — when width > 1.8× median glyph width, vertical cut at minimum projection valley.
5. **Per-script template packs** — share Devanagari engine for `hi`; fork prototypes for `bn`, `gu`, `or`, `pa`; Tamil/Telugu/Kannada/Malayalam/Sinhala each get packs but reuse Indic segmentation module.

**ETA:** 10–14 weeks (largest effort).

---

### Tier D — Arabic (contextual, cursive)

| Code | Language | Script | Notes |
|------|----------|--------|-------|
| ar | Arabic | Arabic | 4 form variants per letter; RTL; ligatures |

**Strategy:**

1. **RTL** line and word order.
2. **Baseline detection** — lower envelope of glyph bounding boxes.
3. **Sliding window over word image** — match against isolated + initial + medial + final prototypes (4× alphabet size).
4. **Ligature table** — explicit prototypes for `لا`, `لأ`, etc. (finite dictionary, not learned).
5. **Vowel marks (tashkeel)** — optional second pass; small diacritic blobs.

**ETA:** 6–8 weeks after Indic core stabilizes.

---

## 4. Shared engineering work (all scripts)

These modules benefit every language and should land **before** scaling templates.

### 4.1 Template pack format (`.clpk` — clang-ldl pack)

Binary or JSON + raw bitmaps:

```
packs/
  latin.clpk      # en (existing, migrate)
  cyrillic.clpk   # ru
  devanagari.clpk # hi (+ shared Indic seg)
  ...
```

**Per-glyph record:**

| Field | Type | Purpose |
|-------|------|---------|
| `codepoint` | `uint32` | Unicode scalar |
| `width`, `height` | `uint16` | Grid size |
| `bitmap` | `uint8[]` | Normalized binary mask |
| `holes` | `uint8` | Euler characteristic / 4 |
| `aspect` | `float` | w/h ratio |
| `endpoints` | `uint8` | Skeleton endpoint count |
| `junctions` | `uint8` | Skeleton branch points |

**Build tool (Python, offline):** `tools/pack_builder.py`

- Input: TTF (Noto Sans *), codepoint list from `language_registry.py`
- Render at canonical sizes (20×28, 24×32 for Indic)
- Compute structural features automatically
- Output: `.clpk` consumed by C++ `PackLoader`

### 4.2 Refactor native matcher

| File | Change |
|------|--------|
| `native/include/clang_ldl/glyph_matcher.hpp` | Pluggable `Recognizer` interface |
| `native/src/glyph_matcher.cpp` | Split Latin into `latin_recognizer.cpp` |
| `native/src/pack_loader.cpp` | Load `.clpk` at runtime or link-time embed |
| `native/src/struct_features.cpp` | Holes, skeleton, moments |
| `native/src/script_router.cpp` | Pick pack from line-level script hints |
| `native/src/segment_indic.cpp` | Shirorekha + akshara rules |
| `native/src/segment_arabic.cpp` | Baseline + sliding window |
| `native/src/rtl.cpp` | Glyph order reversal |

### 4.3 Script router (logical, not ML)

Before matching, classify each **line** using measurable signals:

| Signal | Computation | Indicates |
|--------|-------------|-----------|
| Mean glyph aspect ratio | \(\bar{w/h}\) | Latin ~0.6, Devanagari ~0.8, Thai taller |
| Top-heavy ink density | ink in top 25% / total | Indic (shirorekha), Thai vowels |
| Connectivity ratio | CC count / line width | Arabic cursive (low), Latin (high) |
| Hole frequency | holes per glyph | Burmese, Ethiopic loops |
| Horizontal stroke peak | row projection | Shirorekha line |

Score each script family \(S_j\):

\[
S_j = \sum_k a_{jk} \cdot f_k
\]

Pick \(\arg\max_j S_j\) with margin > δ, else fall back to multi-pack search (slower).

### 4.4 Segmentation upgrades

| ID | Feature | Scripts | Method |
|----|---------|---------|--------|
| S1 | Deskew | All | `minAreaRect` on ink pixels; rotate ≤ ±15° |
| S2 | Adaptive threshold | All | Sauvola fallback when Otsu bimodal assumption fails |
| S3 | RTL reorder | ar, he | Reverse glyph indices per line |
| S4 | Shirorekha cut | Indic | Subtract headline band [§3 Tier C] |
| S5 | Akshara clustering | Indic | Merge base + matra CCs |
| S6 | Word gap detection | ar | Wide horizontal gap → word boundary |
| S7 | Mark attachment | th, lo, hi | Position-classify diacritic blobs |

### 4.5 Diagnostics API (required for tuning)

Extend C API:

```c
typedef struct ClangLdlGlyphInfo {
    uint32_t codepoint;
    float confidence;
    int x, y, w, h;
    float ncc_score;
    float struct_score;
    const char* pack_id;
} ClangLdlGlyphInfo;
```

Enables Python visualization and per-script threshold tuning without guesswork.

### 4.6 Synthetic test harness

| Tool | Purpose |
|------|---------|
| `tools/render_pack_text.py` | Render strings using same TTF as pack builder |
| `tests/ocr/<script>/` | Golden images + expected Unicode |
| `make test-ocr-ru` | Per-script Makefile targets |

**Acceptance metric per script:** Character Error Rate (CER) ≤ 5% on synthetic rendered text at 300 DPI, single font family.

---

## 5. Implementation roadmap

### Overview timeline

```
2026 Q3          Q4              2027 Q1
|---- Foundation ----|-- Tier A --|-- Tier B --|
                      |------ Tier C (Indic) ------|
                                    |-- Tier D --|
                                    | Hardening |
```

**Total estimate:** 7–9 months for all 20 scripts at synthetic-print quality (single-font family), 1 engineer full-time.

---

### Phase L0 — Foundation (weeks 1–3)

**Goal:** Extensible OCR core; Latin migrated to pack format.

| Task | Deliverable | ETA |
|------|-------------|-----|
| L0.1 | `.clpk` spec + `pack_builder.py` | Week 1 |
| L0.2 | `PackLoader` + `struct_features.cpp` | Week 1–2 |
| L0.3 | Refactor Latin to pack; parity with current HELLO tests | Week 2 |
| L0.4 | `script_router.cpp` stub (Latin only) | Week 2 |
| L0.5 | Diagnostics struct in C API | Week 3 |
| L0.6 | Deskew (S1) + Sauvola (S2) | Week 3 |

**Exit criteria:**

- [ ] `make test-ocr-en` passes (CER = 0% on synthetic Latin corpus)
- [ ] Pack rebuild is reproducible from TTF + registry
- [ ] `ocr_native_count` in Python registry updatable per language

---

### Phase L1 — Tier A scripts (weeks 4–7)

| Script pack | Languages | Codepoints | ETA |
|-------------|-----------|------------|-----|
| `cyrillic.clpk` | ru | А–Я, а–я, Ё, ё | Week 4 |
| `greek.clpk` | el | Α–Ω, α–ω | Week 5 |
| `armenian.clpk` | hy | U+0531–U+0587 | Week 5 |
| `georgian.clpk` | ka | U+10D0–U+10F0 | Week 6 |
| Integration | script_router Tier A | | Week 7 |

**Per-language work:**

1. Add codepoint list to `pack_builder` config
2. Generate pack from Noto font
3. Add 20+ synthetic test strings
4. Tune \(\tau\) and weights \(w_1, w_2, w_3\)
5. Set `ocr_native=True` in `language_registry.py`

**Exit criteria:** CER ≤ 5% per language on synthetic corpus; `detect_language.py --synthetic` works without Unicode fallback.

---

### Phase L2 — Tier B scripts (weeks 8–13)

| Order | Pack | Language | Special module | ETA |
|-------|------|----------|----------------|-----|
| 1 | `hebrew.clpk` | he | `rtl.cpp` | Week 8 |
| 2 | `thai.clpk` | th | mark attachment | Week 9–10 |
| 3 | `lao.clpk` | lo | reuse Thai marks logic | Week 10 |
| 4 | `myanmar.clpk` | my | hole-heavy struct weights | Week 11 |
| 5 | `ethiopic.clpk` | am | syllable-sized templates | Week 12–13 |

**Exit criteria:** 9 languages total with native OCR (en + 8).

---

### Phase L3 — Indic engine (weeks 14–27)

**Goal:** One segmentation engine; multiple prototype packs.

| Week | Deliverable |
|------|-------------|
| 14–16 | `segment_indic.cpp`: shirorekha, akshara clustering |
| 17–18 | `devanagari.clpk` → **hi** |
| 19 | `bengali.clpk` → **bn** |
| 20 | `gurmukhi.clpk` → **pa** |
| 21 | `gujarati.clpk` → **gu** |
| 22 | `odia.clpk` → **or** |
| 23 | `tamil.clpk` → **ta** |
| 24 | `telugu.clpk` → **te** |
| 25 | `kannada.clpk` → **kn** |
| 26 | `malayalam.clpk` → **ml** |
| 27 | `sinhala.clpk` → **si** |

**Exit criteria:** CER ≤ 8% on synthetic Indic (conjunct-heavy strings harder); matra-only errors counted separately.

---

### Phase L4 — Arabic (weeks 28–35)

| Week | Deliverable |
|------|-------------|
| 28–29 | `segment_arabic.cpp`: baseline, word gaps |
| 30–32 | `arabic.clpk`: isolated/proximate forms |
| 33–34 | Ligature table (top 50 bigrams) |
| 35 | RTL integration tests |

**Exit criteria:** CER ≤ 10% on synthetic Arabic without tashkeel; tashkeel optional pass documented.

---

### Phase L5 — Hardening (weeks 36–40)

| Task | Purpose |
|------|---------|
| Multi-font tolerance | 2–3 TTF families per script; union prototypes |
| Confidence calibration | Per-script Platt-style linear map (fixed coeffs) |
| Mixed-script lines | Router + per-glyph pack override |
| Performance | Pack lookup index (codepoint hash); SIMD NCC |
| Memory | mmap packs; lazy load per script |
| Docs + `make test-ocr-all` | CI gate |

**Exit criteria:** `ocr_native_count == 21`; `language_coverage_summary()` reports full OCR.

---

## 6. Per-language checklist template

Use this for each of the 20 languages when implementing:

```markdown
### [code] Language name

- [ ] Codepoint inventory finalized (Unicode block range)
- [ ] Noto TTF source pinned (version + path)
- [ ] `.clpk` generated and committed (or CI-built)
- [ ] Segmentation module identified (shared / custom)
- [ ] Synthetic test corpus (≥ 20 strings)
- [ ] CER measured on corpus
- [ ] Weights τ, w₁, w₂, w₃ tuned
- [ ] `ocr_native=True` in language_registry.py
- [ ] `examples/detect_language.py --synthetic` passes
- [ ] Diagnostics spot-checked (glyph boxes)
```

---

## 7. Mathematical feature reference

Features computed on binarized glyph \(G\):

| Feature | Formula / algorithm | Discriminative for |
|---------|---------------------|------------------|
| **NCC** | \(\frac{\sum (G-\bar G)(T-\bar T)}{\|G-\bar G\| \|T-\bar T\|}\) | All scripts |
| **Euler number** | \(E = \#components - \#holes\) via flood-fill | O, B, Burmese, Ethiopic |
| **Aspect ratio** | \(w / h\) | Latin vs Indic vs Thai |
| **Solidity** | \(A_{ink} / A_{convexhull}\) | Arabic cursive vs print |
| **Hu moments** | 7 rotation-invariant moments | Similar letter pairs (и/н, o/0) |
| **Horizontal projection** | Row sum vector | Shirorekha y-position |
| **Vertical projection** | Col sum vector | Conjunct split points |
| **Skeleton endpoints** | Zhang-Suen thinning count | m/n/r distinction |
| **Ink centroid** | \((\bar x, \bar y)\) | Matra position classes |

**Fusion rule (fixed, no learning):**

\[
\text{match} = 0.55 \cdot \text{NCC} + 0.25 \cdot \text{struct\_sim} + 0.10 \cdot \text{Hu\_dist} + 0.10 \cdot \text{aspect\_penalty}
\]

Coefficients stored in `packs/<id>/meta.json` for transparency.

---

## 8. Risks and honest limits

| Risk | Mitigation |
|------|------------|
| Indic conjunct explosion | Limit to common conjunct prototypes; split rare ones |
| Arabic contextual forms | Finite form table + ligature dictionary |
| Font variance | Multi-font packs; accept 2–3 families not all fonts |
| Photos / perspective | Deskew + Sauvola; document “printed scan” scope |
| Handwriting | Out of scope for logical OCR; future ML track in clang.md Phase 2 |
| Performance on mobile | Lazy pack loading; SIMD; optional pack subset |

**What logical OCR will NOT solve (set expectations):**

- Cursive handwriting
- Decorative / script fonts
- Heavy noise or motion blur
- Arbitrary camera angles

For those, [clang.md Phase 2](clang.md) ML path remains the optional upgrade — not required for the 21-language **printed text** goal.

---

## 9. Repository layout (target)

```
clang-ldl/
├── docs/
│   ├── clang.md
│   └── ocr-logical.md          # this document
├── native/
│   ├── src/
│   │   ├── pack_loader.cpp
│   │   ├── struct_features.cpp
│   │   ├── script_router.cpp
│   │   ├── segment_indic.cpp
│   │   ├── segment_arabic.cpp
│   │   ├── rtl.cpp
│   │   └── recognizers/
│   │       ├── latin.cpp
│   │       └── template_matcher.cpp
│   └── include/clang_ldl/
├── packs/                       # .clpk files (or generated in CI)
│   ├── latin.clpk
│   ├── cyrillic.clpk
│   └── ...
├── tools/
│   ├── pack_builder.py
│   └── render_pack_text.py
├── tests/ocr/
│   ├── en/
│   ├── ru/
│   └── ...
└── python/clang_ldl/
    └── language_registry.py     # ocr_native flags updated per phase
```

---

## 10. Makefile targets (planned)

```makefile
make build-packs      # regenerate all .clpk from TTFs
make test-ocr-en      # Latin golden tests
make test-ocr-ru      # Cyrillic golden tests
make test-ocr-all     # full 21-language OCR gate
make bench-ocr        # CER report per script
```

---

## 11. Success metrics

| Metric | Target (synthetic print) | Measurement |
|--------|--------------------------|-------------|
| CER per script | ≤ 5% (≤ 8% Indic, ≤ 10% Arabic) | `tools/cer_report.py` |
| OCR language coverage | 21 / 21 | `language_coverage_summary()` |
| Latency | < 150 ms / line @ 640 px CPU | `make bench-ocr` |
| Determinism | 100% | Same input hash → same output hash |
| Binary size | < 15 MB all packs compressed | Release build |

---

## 12. Recommended execution order

For a single engineer, implement in this order (highest ROI first):

1. **L0 Foundation** — unlocks everything else
2. **ru (Cyrillic)** — validates pack pipeline; closest to Latin
3. **hi (Devanagari)** — builds Indic engine early; hardest; de-risks schedule
4. **el, hy, ka** — quick Tier A wins
5. **th, he** — layout rules (RTL, marks)
6. **Remaining Indic** — packs only once engine exists
7. **ar** — last major segmentation challenge
8. **am, my, lo, si** — fill remaining packs

---

## 13. Progress log

| Date | Update |
|------|--------|
| 2026-06-23 | Initial logical OCR roadmap created. English Latin OCR operational. 20 scripts pending pack + segmentation work. |

---

## 14. References

- Otsu, N. (1979). Threshold selection method.
- Sauvola, J. (2000). Adaptive document image binarization.
- Zhang, T. Y., Suen, C. Y. (1984). Fast parallel thinning algorithm.
- Hu, M. K. (1962). Visual pattern recognition by moment invariants.
- Unicode Standard — character blocks and Indic syllable description (UTR #44).
- Noto fonts — canonical TTF sources for prototype generation.

---

## 15. Next action

Start **Phase L0**:

1. Create `tools/pack_builder.py`
2. Define `.clpk` binary format v1
3. Migrate Latin from embedded `FONT5x7` to `packs/latin.clpk`
4. Add `make build-packs` and `make test-ocr-en`

When L0 exits, begin `cyrillic.clpk` (Russian) as first non-Latin OCR target.
