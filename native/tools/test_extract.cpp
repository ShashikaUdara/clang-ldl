#include "clang_ldl/api.h"

#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: test_extract <ppm>\n");
        return 1;
    }
    ClangLdlResult result{};
    const int code = clang_ldl_extract_text(argv[1], &result);
    std::printf("code=%d text=%s glyphs=%d info=%d\n", code,
                result.text ? result.text : "(null)", result.glyph_count, result.glyph_info_count);
    clang_ldl_free_result(&result);
    return code == CLANG_LDL_OK ? 0 : 1;
}
