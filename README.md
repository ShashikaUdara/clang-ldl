# clang-ldl

**Clang Language Detection Library** — detect languages in images that contain text.

## Quick start

```bash
# Build C++ core
./scripts/build_native.sh

# Install Python package (editable)
cd python && pip install -e ".[dev]"

# Run example on synthetic English text
python ../examples/detect_language.py --synthetic "HELLO"

# See how many languages are supported (21)
python ../examples/list_languages.py
python ../examples/list_languages.py --json

# Run tests
pytest
```

## Architecture

- **native/** — C++ computer vision (load, binarize, segment, template-match Latin glyphs)
- **python/clang_ldl/** — ctypes bindings + Unicode script → language analysis
- **examples/** — CLI test program
- **docs/clang.md** — feature list, phases, ETAs, progress

## Environment

- `CLANG_LDL_LIB` — optional path to `libclang_ldl.so` if not in `native/build/`

## Phase 1 limitations

Works best on high-contrast printed Latin text. See [docs/clang.md](docs/clang.md) for the full roadmap.
