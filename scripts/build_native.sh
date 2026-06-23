#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NATIVE="${ROOT}/native"

if command -v cmake >/dev/null 2>&1; then
  BUILD_DIR="${NATIVE}/build"
  mkdir -p "${BUILD_DIR}"
  cmake -S "${NATIVE}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
  cmake --build "${BUILD_DIR}" --parallel "$(nproc 2>/dev/null || echo 4)"
  echo "Built: ${BUILD_DIR}/libclang_ldl.so"
else
  make -C "${NATIVE}"
  echo "Built: ${NATIVE}/build/libclang_ldl.so"
fi
