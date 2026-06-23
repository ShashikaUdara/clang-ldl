# clang-ldl — top-level Makefile
# Run `make help` for all targets.

ROOT        := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
NATIVE_DIR  := $(ROOT)/native
PYTHON_DIR  := $(ROOT)/python
TESTS_DIR   := $(ROOT)/tests
EXAMPLES    := $(ROOT)/examples
BUILD_DIR   := $(NATIVE_DIR)/build
NATIVE_LIB  := $(BUILD_DIR)/libclang_ldl.so

PYTHON      ?= python3
PIP         ?= $(PYTHON) -m pip
PYTEST      ?= $(PYTHON) -m pytest

PACKS_DIR     := $(ROOT)/packs

# Used by test / example targets when the package is not pip-installed.
export PYTHONPATH := $(PYTHON_DIR)$(if $(PYTHONPATH),:$(PYTHONPATH),)
export CLANG_LDL_LIB := $(NATIVE_LIB)
export CLANG_LDL_PACKS_DIR := $(PACKS_DIR)

JOBS        ?= $(shell nproc 2>/dev/null || echo 4)
VENV_DIR    := $(ROOT)/.venv

.PHONY: help all setup build native native-cmake native-make clean \
        install install-dev develop uninstall \
        test test-all test-text verify test-ocr-en build-packs \
        example example-json languages languages-json \
        env print-env check-deps venv

.DEFAULT_GOAL := help

## help: Show this help message
help:
	@echo "clang-ldl — available targets"
	@echo ""
	@grep -E '^## ' $(MAKEFILE_LIST) | sed 's/^## /  make /' | column -t -s ':'
	@echo ""
	@echo "Quick start:"
	@echo "  make setup          # build native lib + install Python package + verify"
	@echo "  make example        # detect language from synthetic HELLO image"
	@echo "  make languages      # list 21 supported languages"

## all: Build native library and install Python package
all: build install

## setup: Full local setup — build, install Python package, run verification
setup: build install verify
	@echo ""
	@echo "Setup complete."
	@echo "  Native library: $(NATIVE_LIB)"
	@echo "  Python package: editable install from $(PYTHON_DIR)"
	@echo ""
	@echo "Try:"
	@echo "  make example"
	@echo "  make languages"

## build: Build the C/C++ shared library (alias: native)
build: build-packs native

## build-packs: Generate .clpk glyph template packs
build-packs:
	$(PYTHON) "$(ROOT)/tools/pack_builder.py" --pack latin --output "$(PACKS_DIR)/latin.clpk"

## native: Compile libclang_ldl.so (CMake if available, else native/Makefile)
native:
	@if command -v cmake >/dev/null 2>&1; then \
		"$(MAKE)" native-cmake; \
	else \
		"$(MAKE)" native-make; \
	fi

## native-cmake: Build via CMake (Release)
native-cmake:
	cmake -S "$(NATIVE_DIR)" -B "$(BUILD_DIR)" -DCMAKE_BUILD_TYPE=Release
	cmake --build "$(BUILD_DIR)" --parallel $(JOBS)
	@test -f "$(NATIVE_LIB)" || (echo "error: $(NATIVE_LIB) not found" && exit 1)
	@echo "Built: $(NATIVE_LIB)"

## native-make: Build via native/Makefile (no CMake required)
native-make:
	"$(MAKE)" -C "$(NATIVE_DIR)"
	@test -f "$(NATIVE_LIB)" || (echo "error: $(NATIVE_LIB) not found" && exit 1)
	@echo "Built: $(NATIVE_LIB)"

## clean: Remove native build artifacts, caches, and egg-info
clean:
	-"$(MAKE)" -C "$(NATIVE_DIR)" clean
	rm -rf "$(ROOT)/.pytest_cache"
	rm -rf "$(PYTHON_DIR)"/*.egg-info
	rm -rf "$(PYTHON_DIR)/clang_ldl.egg-info"
	rm -f "$(ROOT)/clang_ldl_synthetic.ppm"
	@echo "Cleaned build artifacts"

## install: Install Python package (editable)
install: build
	cd "$(PYTHON_DIR)" && $(PIP) install -e .

## install-dev: Install Python package with dev dependencies (pytest)
install-dev: build
	cd "$(PYTHON_DIR)" && $(PIP) install -e ".[dev]"

## develop: Use package from source without pip (prints env instructions)
develop: build
	@echo "Development mode — no pip install required."
	@"$(MAKE)" print-env
	@echo "Run tests: make test"

## uninstall: Remove installed clang-ldl package
uninstall:
	-$(PIP) uninstall -y clang-ldl 2>/dev/null || true

## test: Run unit tests (requires pytest or uses inline fallback)
test: build
	@if $(PYTHON) -c "import pytest" 2>/dev/null; then \
		cd "$(ROOT)" && $(PYTEST) "$(TESTS_DIR)" -q; \
	else \
		echo "pytest not found — running built-in smoke tests"; \
		"$(MAKE)" test-smoke; \
	fi

## test-all: Same as test (alias)
test-all: test

## test-smoke: Minimal tests without pytest
test-smoke: build
	$(PYTHON) "$(ROOT)/scripts/smoke_test.py"

## test-text: Quick check — detect languages from Unicode sample strings
test-text:
	@$(PYTHON) -c "from clang_ldl import LanguageAnalyzer; samples = {'en': 'Hello', 'ru': 'Привет', 'hi': 'हिन्दी', 'ar': 'مرحبا', 'th': 'สวัสดี'}; a = LanguageAnalyzer(); \
[print(c + ': ' + a.detect(t)[0].name) for c, t in samples.items()]"

## verify: Build, list languages, and run tests
verify: build languages test-smoke test-ocr-en
	@echo "Verification passed."

## test-ocr-en: English OCR golden corpus (target 0% CER)
test-ocr-en: build
	$(PYTHON) "$(ROOT)/scripts/test_ocr_en.py"

## example: Run language detection on synthetic HELLO image
example: build
	$(PYTHON) "$(EXAMPLES)/detect_language.py" --synthetic "HELLO" --show-support

## example-json: Same as example but JSON output
example-json: build
	$(PYTHON) "$(EXAMPLES)/detect_language.py" --synthetic "HELLO" --json

## languages: Print supported language count and table
languages:
	$(PYTHON) "$(EXAMPLES)/list_languages.py"

## languages-json: Machine-readable language coverage report
languages-json:
	$(PYTHON) "$(EXAMPLES)/list_languages.py" --json

## env: Print shell exports for manual development
env: print-env

## print-env: Show PYTHONPATH and CLANG_LDL_LIB exports
print-env:
	@echo "export PYTHONPATH=\"$(PYTHON_DIR):\$$PYTHONPATH\""
	@echo "export CLANG_LDL_LIB=\"$(NATIVE_LIB)\""
	@echo "export CLANG_LDL_PACKS_DIR=\"$(PACKS_DIR)\""

## check-deps: Verify compiler and Python are available
check-deps:
	@echo -n "g++: "; command -v g++ || (echo "MISSING" && exit 1)
	@echo -n "$(PYTHON): "; command -v $(PYTHON) || (echo "MISSING" && exit 1)
	@command -v cmake >/dev/null 2>&1 && echo "cmake: $$(cmake --version | head -1)" || echo "cmake: not installed (will use native/Makefile)"
	@$(PYTHON) -c "import pytest" 2>/dev/null && echo "pytest: installed" || echo "pytest: not installed (make install-dev to add)"
	@echo "Dependencies OK."

## venv: Create .venv and install package with dev deps
venv: build
	@test -d "$(VENV_DIR)" || $(PYTHON) -m venv "$(VENV_DIR)"
	"$(VENV_DIR)/bin/pip" install -U pip
	"$(VENV_DIR)/bin/pip" install -e "$(PYTHON_DIR)[dev]"
	@echo ""
	@echo "Virtualenv ready: $(VENV_DIR)"
	@echo "Activate: source \"$(VENV_DIR)/bin/activate\""
