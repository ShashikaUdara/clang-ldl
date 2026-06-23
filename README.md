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
| `make build` | Compile C/C++ shared library |
| `make clean` | Remove build artifacts and caches |
| `make install-dev` | Editable Python install with pytest |
| `make test` | Run `tests/` |
| `make languages` | List supported languages (21) |
| `make example` | Run detection CLI on synthetic image |
| `make env` | Print `PYTHONPATH` and `CLANG_LDL_LIB` exports |

Legacy script (still works): `./scripts/build_native.sh`

## Architecture

- **native/** — C++ computer vision (load, binarize, segment, template-match Latin glyphs)
- **python/clang_ldl/** — ctypes bindings + Unicode script → language analysis
- **examples/** — CLI test program
- **docs/clang.md** — feature list, phases, ETAs, progress

## Environment

- `CLANG_LDL_LIB` — optional path to `libclang_ldl.so` if not in `native/build/`

## Phase 1 limitations

- **Image OCR** works best on high-contrast printed **Latin** text (`HELLO`, `ABC`, …).
- **All 21 languages** are identified via **Unicode script analysis** when you pass text with `--synthetic` (non-Latin) or `--text`.
- Non-Latin `--synthetic` renders a PNG and identifies language from the Unicode you provide; C++ OCR for those scripts is Phase 1.14 / Phase 2.

```bash
# Latin — full native OCR pipeline
python3 examples/detect_language.py --synthetic "HELLO"

# Sinhala / Hindi / etc. — script analysis (OCR templates pending)
python3 examples/detect_language.py --synthetic "මෙවලම්"
python3 examples/detect_language.py --synthetic "नमस्ते"

# Direct Unicode (no image)
python3 examples/detect_language.py --text "नमस्ते"
```

See [docs/clang.md](docs/clang.md) for the full roadmap.
