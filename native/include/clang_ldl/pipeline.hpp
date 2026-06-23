#pragma once

#include "clang_ldl/image.hpp"

#include <string>
#include <vector>

namespace clang_ldl {

/** Preprocess for OCR: grayscale -> blur -> Otsu binary (foreground = 255). */
Image preprocess(const Image& input);

/** Detect horizontal text line bounding boxes (top-down order). */
std::vector<Rect> find_text_lines(const Image& binary);

/** Segment glyphs within a line crop; left-to-right order. */
std::vector<Glyph> segment_glyphs(const Image& line_binary);

} // namespace clang_ldl
