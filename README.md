# clang-ldl

**Clang Language Detection Library** — detect languages in images that contain text.

## Quick start

```bash
cd clang-ldl

# See all commands
make help

# Full setup: compile C++ core, install Python package, verify
make setup

# Or step by step:
make build          # compile libclang_ldl.so
make install-dev    # pip install -e python/[dev]
make test           # run unit tests
make languages      # show 21 supported languages
make example        # detect language from synthetic HELLO image
```

### Without pip (development from source)

```bash
make develop        # builds native lib + prints env exports
make test-smoke     # quick validation
make example
```

### Using a virtual environment

```bash
make venv
source .venv/bin/activate
make example
```

## Makefile targets (summary)

| Target | Description |
|--------|-------------|
| `make setup` | Build + install + verify (recommended first run) |
| `make build` | Compile C/C++ shared library + glyph packs |
| `make build-packs` | Regenerate all `.clpk` glyph packs (Latin + Tier A) |
| `make test-ocr-en` | English OCR golden corpus (0% CER target) |
| `make test-ocr-tier-a` | Russian, Greek, Armenian, Georgian OCR corpora |
| `make verify` | Build, languages, smoke + OCR tests |
| `make clean` | Remove build artifacts and caches |
| `make install-dev` | Editable Python install with pytest |
| `make test` | Run `tests/` |
| `make languages` | List supported languages (21) |
| `make example` | Run detection CLI on synthetic image |
| `make env` | Print `PYTHONPATH` and `CLANG_LDL_LIB` exports |

Legacy script (still works): `./scripts/build_native.sh`

## Architecture

- **native/** — C++ computer vision (load, binarize, segment, `.clpk` template matching)
- **packs/** — binary glyph template packs (Latin: `latin.clpk`)
- **python/clang_ldl/** — ctypes bindings + Unicode script → language analysis
- **examples/** — CLI test program
- **docs/clang.md** — feature list, phases, ETAs, progress
- **docs/ocr-logical.md** — non-AI OCR roadmap for all 21 languages

## Environment

- `CLANG_LDL_LIB` — optional path to `libclang_ldl.so` if not in `native/build/`
- `CLANG_LDL_PACKS_DIR` — directory containing `.clpk` glyph packs (default: `packs/`)
- `CLANG_LDL_PACK_ID` — force OCR pack (`latin`, `cyrillic`, `greek`, `armenian`, `georgian`)
- `CLANG_LDL_DESKEW=1` — enable experimental deskew in preprocessing (off by default)

## Phase 1 limitations

- **Image OCR** works on high-contrast synthetic text for **Latin, Cyrillic, Greek, Armenian, and Georgian** (see `make test-ocr-tier-a`).
- **All 21 languages** are identified via **Unicode script analysis** when native OCR is unavailable.
- Remaining scripts are planned in [docs/ocr-logical.md](docs/ocr-logical.md) (Phases L2–L5).

```bash
# Latin — terminal font OCR
python3 examples/detect_language.py --synthetic "HELLO"

# Russian — native Tier A OCR
python3 examples/detect_language.py --synthetic "москва"

# Sinhala / Hindi / etc. — script analysis (OCR templates pending)
python3 examples/detect_language.py --synthetic "මෙවලම්"
python3 examples/detect_language.py --synthetic "नमस्ते"

# Direct Unicode (no image)
python3 examples/detect_language.py --text "नमस्ते"
```

See [docs/clang.md](docs/clang.md) for the project overview and [docs/ocr-logical.md](docs/ocr-logical.md) for the OCR implementation plan.
