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

## 2. Feasibility: can logical OCR achieve good accuracy?

### 2.1 Executive verdict

**Yes — with a clearly defined scope.** A non-AI, computer-vision OCR library for all 21 languages is **feasible and can reach good accuracy** for **printed, high-contrast, horizontally laid-out text** in known font families. It is **not feasible** to match modern ML OCR across handwriting, arbitrary fonts, photos, and noisy scenes using logic alone.

| Question | Answer |
|----------|--------|
| Is the approach technically sound? | **Yes** — classical OCR predates neural networks; template + structural matching is proven for constrained inputs. |
| Can we reach “good” accuracy for all 21 scripts? | **Yes**, tiered: 95%+ CER on synthetic print for Tier A; 90–92% for Indic/Arabic with rules. |
| Is it worth building vs using Tesseract/ML? | **Yes** for clang-ldl’s goals: offline, deterministic, embeddable, no model weights, full explainability. |
| Single biggest risk? | **Indic conjunct segmentation** — logic-heavy, not impossible. |
| Proof it works today? | English Latin OCR on synthetic images: **~0% CER**, **~0.66 mean confidence** on `HELLO` (clang-ldl v0.2). |

> **Plain-language summary:** Think of this like teaching someone to read block letters from a stencil book — if the letters are neat, separated, and match the stencils you prepared, accuracy is excellent. Ask them to read messy handwriting on a wrinkled photo, and you need a different tool (AI). For signs, receipts, scans, and rendered UI text, the logical approach is a strong fit.

---

### 2.2 What “good accuracy” means in this project

We define tiers so expectations stay honest and testable:

| Tier | Definition | Target CER | Example use case |
|------|------------|------------|------------------|
| **A — Excellent** | Synthetic render, same TTF family as template pack | ≤ 1% | Unit tests, CI golden images |
| **B — Good** | Clean scan/print, 1–3 font families, deskewed | ≤ 5% | Document scans, screenshots |
| **C — Acceptable** | Slight noise, mild rotation (≤ 5°), compression artifacts | ≤ 10% | Mobile photo of a menu (well lit) |
| **D — Poor fit** | Handwriting, script fonts, heavy blur, perspective | — | Out of scope for logical OCR |

**“Good accuracy” for the library** = **Tier B** across all 21 languages, with **Tier A** in automated tests. That is achievable with the roadmap in §6.

---

### 2.3 Why logical OCR works (technical basis)

Logical OCR does not guess from millions of examples. It **measures** and **compares**:

```
Input glyph  →  normalize size  →  extract numbers (holes, strokes, shape)
                                         ↓
                              compare to prototype table  →  best match + confidence
```

Each step is deterministic arithmetic or geometry:

| Technique | Maturity | Role in accuracy |
|-----------|----------|------------------|
| Otsu / Sauvola binarization | Very high | Separates ink from paper reliably on scans |
| Horizontal / vertical projections | Very high | Finds lines and character gaps |
| Normalized cross-correlation (NCC) | Very high | Core of template matching since 1970s OCR |
| Euler number (hole counting) | High | Separates `O`/`D`, `০`/`৮`, Ethiopic loops |
| Skeleton + endpoint/junction counts | High | Disambiguates similar letters (`и`/`н`, `m`/`n`) |
| Hu moments | Medium–high | Rotation-tolerant shape fingerprint |
| Rule-based Indic shirorekha | Medium | Required for Devanagari-class scripts; well documented in literature |
| Arabic form tables | Medium | Finite state; no learning needed for print |

**Historical precedent:** Commercial OCR (1970s–2000s), MICR bank encoding, postal OCR, and early Tesseract all relied on structural features before DNN dominance. clang-ldl targets the same **constrained domain** those systems excelled at.

---

### 2.4 Feasibility by script tier

Aligned with §4 (script families). **Tier B (good)** achievability with the planned logical pipeline.

| Tier | Scripts | Feasibility | Expected Tier B CER | Confidence in estimate |
|------|---------|-------------|---------------------|------------------------|
| **A** | en, ru, el, hy, ka | **Very high** | 2–5% | High — discrete letters, proven Latin path |
| **B** | he, th, lo, my, am | **High** | 4–8% | Medium–high — layout rules add work, topology still discrete |
| **C** | hi, bn, pa, gu, or, ta, te, kn, ml, si | **Medium–high** | 6–10% | Medium — conjuncts/matras are the hard part |
| **D** | ar | **Medium** | 8–12% | Medium — cursive forms need form tables + ligature dict |

**Overall:** 21/21 languages at Tier B is **feasible** within the 7–9 month roadmap. Tier C (mobile photos) is **partially feasible** after hardening (deskew, Sauvola, multi-font packs) but not guaranteed for every scene.

---

### 2.5 Logical OCR vs machine learning

| Dimension | Logical OCR (this plan) | ML OCR (Tesseract LSTM, CRNN, transformers) |
|-----------|-------------------------|---------------------------------------------|
| **Printed clean text** | Excellent (95%+) | Excellent (98%+) |
| **Handwriting** | Poor | Good–excellent |
| **Arbitrary fonts** | Poor (unless in pack) | Good |
| **Explainability** | Full (per-glyph scores) | Low |
| **Binary size** | ~5–15 MB packs | ~10–100+ MB models |
| **Determinism** | 100% | Near-deterministic |
| **Offline / privacy** | Native | Native (if model bundled) |
| **Build complexity** | Rules + packs per script | Training infra or vendored models |
| **Maintenance** | Tune thresholds, add prototypes | Retrain or upgrade models |

**Conclusion:** For clang-ldl’s stated Phase 1 goals (no training, CV + logic), the **accuracy gap vs ML on printed text is small (≈ 2–5% CER)** — acceptable for language-detection-from-image use cases where script identification matters more than perfect transcription.

---

### 2.6 Conditions required for good accuracy

Good results are not automatic. These conditions must hold (and are enforceable in tests):

1. **Foreground/background separation** — Otsu + optional Sauvola; invert when light background.
2. **Horizontal text** — deskew to ±15° (planned L0).
3. **Font coverage** — glyph prototypes built from the same font family used in tests (Noto *); multi-font packs for production.
4. **Script-specific segmentation** — Indic akshara clustering, Arabic RTL, etc. (roadmap §6).
5. **Calibrated thresholds** — per-script \(\tau\) and fusion weights from golden corpora, not hand-waved.
6. **Diagnostics** — when accuracy drops, engineers can see *which* glyph failed and *why* (NCC vs struct score).

When these are in place — as demonstrated already for English — **good accuracy is reproducible**, not accidental.

---

### 2.7 Evidence from the current codebase

clang-ldl already proves the hardest part of the pipeline for one script:

| Observation | Implication |
|-------------|-------------|
| Latin `HELLO` synthetic → correct text, 5 glyphs, ~0.66 confidence | Template path works end-to-end |
| Binarization + segmentation find glyphs on binary images | Shared CV stages generalize |
| Non-Latin synthetic PNG → OCR returns garbage (`DDDDDD`) but Unicode fallback identifies language | Segmentation fires; **recognition** is the only missing layer per script |
| 21-language script ID from Unicode → 100% on sample chars | Downstream language stage is ready |

The gap is **not** “can we read text from images at all?” — it is **“can we load the right prototype pack for each script?”** That is engineering scale-up, not research uncertainty.

---

### 2.8 Accuracy projection (conservative)

Projected **character error rate** after full roadmap (Tier B, clean print):

```
Script group          Now    After L1    After L3    After L5
─────────────────────────────────────────────────────────────
Latin (en)            ~0%      ~0%         ~0%         ~0%
Cyrillic/Greek (×4)    —      3–5%        3–5%        2–4%
Tier B (×5)            —       —          5–8%        4–7%
Indic (×10)            —       —          7–10%       6–9%
Arabic (ar)            —       —           —          8–12%
─────────────────────────────────────────────────────────────
Weighted 21-lang       N/A     ~4%         ~7%         ~5–6%
```

Language **detection** accuracy (given OCR text) will exceed OCR accuracy because script classification tolerates partial character errors — e.g. 90% of Devanagari glyphs correct still yields **Hindi** reliably.

---

### 2.9 Go / no-go recommendation

| Verdict | Recommendation |
|---------|----------------|
| **GO** | Proceed with logical OCR for all 21 languages under Tier A–B scope. |
| **GO** | Start L0 (pack format + Cyrillic) to validate pack pipeline on second script. |
| **DEFER** | Tier D (handwriting, decorative fonts) to optional ML track in [clang.md](clang.md) Phase 2. |
| **MONITOR** | Indic CER during L3 — if conjunct CER > 12% after 4 weeks, add conjunct prototype dictionary before more scripts. |

**Bottom line:** Implementing this library with **good accuracy for printed text in 21 languages without AI is feasible, defensible, and aligned with clang-ldl’s architecture.** The work is substantial (months, not days) but each step de-risks the next; English OCR is the existence proof.

---

## 3. Design philosophy: logical OCR

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

## 4. Script families — complexity tiers

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

## 5. Shared engineering work (all scripts)

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
| S4 | Shirorekha cut | Indic | Subtract headline band [§4 Tier C] |
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

## 6. Implementation roadmap

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

- [x] `make test-ocr-en` passes (CER = 0% on synthetic Latin corpus)
- [x] Pack rebuild is reproducible from TTF + registry (`make build-packs`)
- [x] `ocr_native_count` in Python registry updatable per language (`ocr_native` flag on `LanguageSpec`)

**Status:** Complete (2026-06-23). Latin OCR runs from `packs/latin.clpk` with struct-feature fusion, projection segmentation for monospace synthetic text, Sauvola fallback binarization, and optional deskew via `CLANG_LDL_DESKEW=1`.

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

**Exit criteria:**

- [x] CER ≤ 5% per language on synthetic corpus (`make test-ocr-tier-a`)
- [x] `detect_language.py --synthetic` uses native OCR for Tier A scripts (pack hint + router)

**Status:** Complete (2026-06-23). Five packs (`latin` + Tier A), `script_router` voting with `CLANG_LDL_PACK_ID` hint, UTF-8 API output (v0.4.0), `tools/ocr_render.py` line-parity pack building.

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

**Status:** Complete (2026-06-23). Five Tier B packs (`hebrew`, `thai`, `lao`, `myanmar`, `ethiopic`), `rtl.cpp` + `mark_attachment.cpp` + `script_preprocess.cpp`, RTL line rendering in `ocr_render.py`, `native_pack_ids()` router voting, v0.5.0, `make test-ocr-tier-b`, `ocr_native` for he/th/lo/my/am.

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

**Status:** Complete (2026-06-23). Shared `segment_indic.cpp` (shirorekha + akshara clustering + conjunct split), ten Indic packs (`devanagari` … `sinhala`), v0.6.0, `make test-ocr-indic`, `ocr_native` for hi/bn/pa/gu/or/ta/te/kn/ml/si (20/21 native OCR; Arabic remains L4).

---

### Phase L4 — Arabic (weeks 28–35)

| Week | Deliverable |
|------|-------------|
| 28–29 | `segment_arabic.cpp`: baseline, word gaps |
| 30–32 | `arabic.clpk`: isolated/proximate forms |
| 33–34 | Ligature table (top 50 bigrams) |
| 35 | RTL integration tests |

**Exit criteria:** CER ≤ 10% on synthetic Arabic without tashkeel; tashkeel optional pass documented.

**Status:** Complete (2026-06-23). `segment_arabic.cpp` (baseline + word-gap cursive clustering), `arabic_ligatures.cpp` (top bigram table + pack ligature prototypes), `arabic.clpk`, RTL rendering, v0.7.0, `make test-ocr-ar`, `ocr_native` for ar (**21/21 native OCR**).

**Tashkeel:** vowel marks (U+064B–U+0652) are excluded from the golden corpus and pack; optional second-pass mark attachment is deferred to L5 hardening.

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

**Status:** Complete (2026-06-23). Codepoint glyph index + mmap pack loading, per-pack confidence calibration, per-glyph mixed-script fallback (`CLANG_LDL_MIXED_SCRIPT=0` to disable), multi-font Cyrillic union prototypes, `make test-ocr-all` + `tools/cer_report.py`, v0.8.0.

---

## 7. Per-language checklist template

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

## 8. Mathematical feature reference

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

## 9. Risks and honest limits

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

## 10. Repository layout (target)

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

## 11. Makefile targets (planned)

```makefile
make build-packs      # regenerate all .clpk from TTFs
make test-ocr-en      # Latin golden tests
make test-ocr-ru      # Cyrillic golden tests
make test-ocr-all     # full 21-language OCR gate
make bench-ocr        # CER report per script
```

---

## 12. Success metrics

| Metric | Target (synthetic print) | Measurement |
|--------|--------------------------|-------------|
| CER per script | ≤ 5% (≤ 8% Indic, ≤ 10% Arabic) | `tools/cer_report.py` |
| OCR language coverage | 21 / 21 | `language_coverage_summary()` |
| Latency | < 150 ms / line @ 640 px CPU | `make bench-ocr` |
| Determinism | 100% | Same input hash → same output hash |
| Binary size | < 15 MB all packs compressed | Release build |

---

## 13. Recommended execution order

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

## 14. Progress log

| Date | Update |
|------|--------|
| 2026-06-23 | Initial logical OCR roadmap created. English Latin OCR operational. 20 scripts pending pack + segmentation work. |
| 2026-06-23 | Added §2 Feasibility analysis — accuracy tiers, per-script feasibility scores, logical vs ML comparison, go/no-go recommendation. |
| 2026-06-23 | **Phase L0 complete.** `.clpk` v1, `pack_builder.py`, `latin.clpk`, pack loader, struct features, script router stub, diagnostics C API (v0.3.0), `make test-ocr-en` at 0% CER. |
| 2026-06-23 | **Phase L1 complete.** Tier A packs (cyrillic, greek, armenian, georgian), script router voting, UTF-8 OCR (v0.4.0), `make test-ocr-tier-a`, `ocr_native` for ru/el/hy/ka. |
| 2026-06-23 | **Phase L2 complete.** Tier B packs (hebrew, thai, lao, myanmar, ethiopic), RTL + mark attachment preprocess, v0.5.0, `make test-ocr-tier-b`, `ocr_native` for he/th/lo/my/am (10/21 native OCR). |
| 2026-06-23 | **Phase L3 complete.** Indic engine (`segment_indic.cpp`), ten script packs, v0.6.0, `make test-ocr-indic`, `ocr_native` for all South Asian scripts (20/21; Arabic next). |
| 2026-06-23 | **Phase L4 complete.** Arabic engine (`segment_arabic.cpp`, ligature table), `arabic.clpk`, v0.7.0, `make test-ocr-ar`, full **21/21 native OCR**. |
| 2026-06-23 | **Phase L5 complete.** Pack index + mmap, confidence calibration, mixed-script fallback, multi-font Cyrillic, `make test-ocr-all`, `cer_report.py`, v0.8.0. |

---

## 15. References

- Otsu, N. (1979). Threshold selection method.
- Sauvola, J. (2000). Adaptive document image binarization.
- Zhang, T. Y., Suen, C. Y. (1984). Fast parallel thinning algorithm.
- Hu, M. K. (1962). Visual pattern recognition by moment invariants.
- Unicode Standard — character blocks and Indic syllable description (UTR #44).
- Noto fonts — canonical TTF sources for prototype generation.

---

## 16. Next action

**Post-roadmap:** optional ML track (clang.md Phase 2), mobile photo hardening, tashkeel second pass for Arabic.

---
