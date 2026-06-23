from __future__ import annotations

import ctypes
import os
import sys
from ctypes import POINTER, c_char, c_char_p, c_float, c_int, c_size_t, c_ubyte, c_uint32
from pathlib import Path

from clang_ldl.models import ExtractionResult

CLANG_LDL_OK = 0
CLANG_LDL_ERR_LOAD = -1
CLANG_LDL_ERR_PROCESS = -2
CLANG_LDL_ERR_ALLOC = -3
CLANG_LDL_ERR_NULL = -4


class ClangLdlGlyphInfo(ctypes.Structure):
    _fields_ = [
        ("codepoint", c_uint32),
        ("confidence", c_float),
        ("x", c_int),
        ("y", c_int),
        ("w", c_int),
        ("h", c_int),
        ("ncc_score", c_float),
        ("struct_score", c_float),
        ("pack_id", c_char * 32),
    ]


class ClangLdlResult(ctypes.Structure):
    _fields_ = [
        ("text", c_char_p),
        ("text_len", c_size_t),
        ("mean_confidence", c_float),
        ("glyph_count", c_int),
        ("glyphs", POINTER(ClangLdlGlyphInfo)),
        ("glyph_info_count", c_int),
    ]


def _candidate_lib_paths() -> list[Path]:
    root = Path(__file__).resolve().parent.parent.parent
    names = ["libclang_ldl.so", "libclang_ldl.dylib", "clang_ldl.dll"]
    paths: list[Path] = []
    env = os.environ.get("CLANG_LDL_LIB")
    if env:
        paths.append(Path(env))
    for name in names:
        paths.append(root / "native" / "build" / name)
        paths.append(root / "native" / "build" / "Release" / name)
    return paths


def load_native_library() -> ctypes.CDLL:
    last_error: Exception | None = None
    for path in _candidate_lib_paths():
        if not path.is_file():
            continue
        try:
            lib = ctypes.CDLL(str(path))
            break
        except OSError as exc:
            last_error = exc
    else:
        msg = "clang_ldl native library not found. Run: make build"
        if last_error:
            msg += f" Last error: {last_error}"
        raise OSError(msg)

    # Ensure packs directory is set for native loader.
    packs = _packs_dir()
    if packs.is_dir() and "CLANG_LDL_PACKS_DIR" not in os.environ:
        os.environ["CLANG_LDL_PACKS_DIR"] = str(packs)

    lib.clang_ldl_extract_text.argtypes = [c_char_p, POINTER(ClangLdlResult)]
    lib.clang_ldl_extract_text.restype = c_int
    lib.clang_ldl_extract_text_from_bytes.argtypes = [
        POINTER(c_ubyte),
        c_int,
        c_int,
        c_int,
        POINTER(ClangLdlResult),
    ]
    lib.clang_ldl_extract_text_from_bytes.restype = c_int
    lib.clang_ldl_free_result.argtypes = [POINTER(ClangLdlResult)]
    lib.clang_ldl_free_result.restype = None
    lib.clang_ldl_version.argtypes = []
    lib.clang_ldl_version.restype = c_char_p
    return lib


def _packs_dir() -> Path:
    env = os.environ.get("CLANG_LDL_PACKS_DIR")
    if env:
        return Path(env)
    root = Path(__file__).resolve().parent.parent.parent
    return root / "packs"


_LIB: ctypes.CDLL | None = None


def native_library() -> ctypes.CDLL:
    global _LIB
    if _LIB is None:
        _LIB = load_native_library()
    return _LIB


def _raise_for_status(code: int) -> None:
    errors = {
        CLANG_LDL_ERR_LOAD: "failed to load or decode image",
        CLANG_LDL_ERR_PROCESS: "image processing failed",
        CLANG_LDL_ERR_ALLOC: "native allocation failed",
        CLANG_LDL_ERR_NULL: "null argument passed to native API",
    }
    if code != CLANG_LDL_OK:
        raise RuntimeError(errors.get(code, f"native error {code}"))


def extract_text_from_file(path: str | Path) -> ExtractionResult:
    lib = native_library()
    result = ClangLdlResult()
    code = lib.clang_ldl_extract_text(str(path).encode("utf-8"), ctypes.byref(result))
    try:
        _raise_for_status(code)
        text = result.text.decode("utf-8") if result.text else ""
        return ExtractionResult(
            text=text,
            mean_confidence=float(result.mean_confidence),
            glyph_count=int(result.glyph_count),
        )
    finally:
        lib.clang_ldl_free_result(ctypes.byref(result))
        result.text = None
        result.glyphs = None


def extract_text_from_image_bytes(data: bytes, width: int, height: int, channels: int) -> ExtractionResult:
    lib = native_library()
    buf = (c_ubyte * len(data)).from_buffer_copy(data)
    result = ClangLdlResult()
    code = lib.clang_ldl_extract_text_from_bytes(
        buf, width, height, channels, ctypes.byref(result)
    )
    try:
        _raise_for_status(code)
        text = result.text.decode("utf-8") if result.text else ""
        return ExtractionResult(
            text=text,
            mean_confidence=float(result.mean_confidence),
            glyph_count=int(result.glyph_count),
        )
    finally:
        lib.clang_ldl_free_result(ctypes.byref(result))
        result.text = None
        result.glyphs = None
