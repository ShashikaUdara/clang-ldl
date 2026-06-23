#include "clang_ldl/api.h"
#include "clang_ldl/glyph_matcher.hpp"
#include "clang_ldl/image.hpp"
#include "clang_ldl/pipeline.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace {

int extract_internal(const clang_ldl::Image& input, ClangLdlResult* out) {
    if (!out) {
        return CLANG_LDL_ERR_NULL;
    }
    std::memset(out, 0, sizeof(*out));

    try {
        const clang_ldl::Image binary = clang_ldl::preprocess(input);
        const auto lines = clang_ldl::find_text_lines(binary);
        std::vector<clang_ldl::Glyph> all_glyphs;

        for (const auto& line_box : lines) {
            clang_ldl::Image line = clang_ldl::Image();
            line.width = line_box.w;
            line.height = line_box.h;
            line.channels = 1;
            line.pixels.resize(static_cast<size_t>(line_box.w) * line_box.h);
            for (int y = 0; y < line_box.h; ++y) {
                for (int x = 0; x < line_box.w; ++x) {
                    line.set(x, y, binary.at(line_box.x + x, line_box.y + y));
                }
            }
            auto glyphs = clang_ldl::segment_glyphs(line);
            all_glyphs.insert(all_glyphs.end(), glyphs.begin(), glyphs.end());
        }

        if (all_glyphs.empty()) {
            auto glyphs = clang_ldl::segment_glyphs(binary);
            all_glyphs = std::move(glyphs);
        }

        clang_ldl::recognize_glyphs(all_glyphs);

        std::ostringstream text;
        float conf_sum = 0.f;
        int conf_n = 0;
        clang_ldl::Glyph* prev = nullptr;
        for (auto& g : all_glyphs) {
            if (prev) {
                const int gap = g.box.x - (prev->box.x + prev->box.w);
                const int min_w = std::min(prev->box.w, g.box.w);
                if (gap > std::max(min_w, 4)) {
                    text << ' ';
                }
            }
            if (g.codepoint >= 32 && g.codepoint < 127) {
                text << static_cast<char>(g.codepoint);
            }
            conf_sum += g.confidence;
            ++conf_n;
            prev = &g;
        }

        const std::string s = text.str();
        out->text_len = s.size();
        out->text = static_cast<char*>(std::malloc(s.size() + 1));
        if (!out->text) {
            return CLANG_LDL_ERR_ALLOC;
        }
        std::memcpy(out->text, s.c_str(), s.size() + 1);
        out->glyph_count = conf_n;
        out->mean_confidence = conf_n > 0 ? conf_sum / conf_n : 0.f;
        return CLANG_LDL_OK;
    } catch (...) {
        return CLANG_LDL_ERR_PROCESS;
    }
}

} // namespace

extern "C" {

const char* clang_ldl_version(void) {
    return "0.1.0";
}

int clang_ldl_extract_text(const char* image_path, ClangLdlResult* out) {
    if (!image_path || !out) {
        return CLANG_LDL_ERR_NULL;
    }
    try {
        const clang_ldl::Image img = clang_ldl::Image::load_file(image_path);
        return extract_internal(img, out);
    } catch (...) {
        return CLANG_LDL_ERR_LOAD;
    }
}

int clang_ldl_extract_text_from_bytes(
    const unsigned char* data,
    int width,
    int height,
    int channels,
    ClangLdlResult* out) {
    if (!data || width <= 0 || height <= 0 || channels < 1 || !out) {
        return CLANG_LDL_ERR_NULL;
    }
    try {
        const clang_ldl::Image img = clang_ldl::Image::from_bytes(data, width, height, channels);
        return extract_internal(img, out);
    } catch (...) {
        return CLANG_LDL_ERR_LOAD;
    }
}

void clang_ldl_free_result(ClangLdlResult* result) {
    if (!result) {
        return;
    }
    std::free(result->text);
    result->text = nullptr;
    result->text_len = 0;
    result->glyph_count = 0;
    result->mean_confidence = 0.f;
}

} // extern "C"
