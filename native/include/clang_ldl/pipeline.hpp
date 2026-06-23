#pragma once

#include "clang_ldl/image.hpp"

#include <string>
#include <vector>

namespace clang_ldl {

/** Preprocess for OCR: grayscale -> blur -> Otsu binary (foreground = 255). */
Image preprocess(const Image& input);

/** Detect horizontal text line bounding boxes (top-down order). */
std::vector<Rect> find_text_lines(const Image& binary);

/** Merge line boxes separated by small vertical gaps (Arabic dots, Thai marks). */
std::vector<Rect> merge_adjacent_text_lines(const std::vector<Rect>& lines, int max_gap);

/** Segment glyphs within a line crop; left-to-right order. */
std::vector<Glyph> segment_glyphs(const Image& line_binary);

} // namespace clang_ldl
