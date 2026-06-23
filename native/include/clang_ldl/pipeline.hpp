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

/** Re-split line boxes taller than max_h (web screenshots / merged paragraphs). */
std::vector<Rect> split_tall_text_lines(const std::vector<Rect>& lines, const Image& binary, int max_h);

/** Segment glyphs within a line crop; left-to-right order. */
std::vector<Glyph> segment_glyphs(const Image& line_binary);

} // namespace clang_ldl
