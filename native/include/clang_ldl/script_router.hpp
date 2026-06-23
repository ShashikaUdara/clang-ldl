#pragma once

#include "clang_ldl/image.hpp"
#include <string>

namespace clang_ldl {

/** Select OCR pack id for a text line image (L0: latin only). */
std::string route_script_for_line(const Image& line_binary);

} // namespace clang_ldl
