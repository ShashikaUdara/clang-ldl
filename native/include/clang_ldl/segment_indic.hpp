#pragma once

#include "clang_ldl/image.hpp"

#include <string>
#include <vector>

namespace clang_ldl {

/** True when pack_id uses the shared Indic segmentation engine. */
bool pack_is_indic(const std::string& pack_id);

/**
 * Segment a line image into akshara-level glyphs.
 * Uses projection for fixed-width synthetic lines; otherwise shirorekha + CC clustering.
 */
std::vector<Glyph> segment_indic_glyphs(const Image& line_binary);

} // namespace clang_ldl
