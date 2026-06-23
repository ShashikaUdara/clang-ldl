#include "clang_ldl/rtl.hpp"

#include <algorithm>
#include <cstdlib>

namespace clang_ldl {

bool pack_is_rtl(const std::string& pack_id) {
    return pack_id == "hebrew" || pack_id == "arabic";
}

void apply_rtl_glyph_order(std::vector<Glyph>& glyphs, const std::string& pack_id) {
    if (!pack_is_rtl(pack_id)) {
        return;
    }
    const char* env = std::getenv("CLANG_LDL_RTL_REVERSE");
    if (!env || (env[0] != '1' && env[0] != 'y' && env[0] != 'Y')) {
        return;
    }
    std::reverse(glyphs.begin(), glyphs.end());
}

} // namespace clang_ldl
