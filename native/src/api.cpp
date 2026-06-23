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

void fill_glyph_info(ClangLdlGlyphInfo& info, const clang_ldl::Glyph& g) {
    info.codepoint = g.codepoint;
    info.confidence = g.confidence;
    info.x = g.box.x;
    info.y = g.box.y;
    info.w = g.box.w;
    info.h = g.box.h;
    info.ncc_score = g.ncc_score;
    info.struct_score = g.struct_score;
    std::memset(info.pack_id, 0, sizeof(info.pack_id));
    const std::string& pid = g.pack_id.empty() ? "latin" : g.pack_id;
    std::strncpy(info.pack_id, pid.c_str(), sizeof(info.pack_id) - 1);
}

void append_utf8(std::string& out, uint32_t codepoint) {
    if (codepoint == 0) {
        return;
    }
    if (codepoint <= 0x7F) {
        out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
        out.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
}

int extract_internal(const clang_ldl::Image& input, ClangLdlResult* out) {
    if (!out) {
        return CLANG_LDL_ERR_NULL;
    }
    std::memset(out, 0, sizeof(*out));

    try {
        const clang_ldl::Image binary = clang_ldl::preprocess(input);
        auto lines = clang_ldl::find_text_lines(binary);
        lines = clang_ldl::merge_adjacent_text_lines(lines, std::max(8, binary.height / 12));
        std::vector<clang_ldl::Glyph> all_glyphs;

        for (const auto& line_box : lines) {
            clang_ldl::Image line;
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
            clang_ldl::recognize_glyphs(glyphs, line);
            all_glyphs.insert(all_glyphs.end(), glyphs.begin(), glyphs.end());
        }

        if (all_glyphs.empty()) {
            auto glyphs = clang_ldl::segment_glyphs(binary);
            clang_ldl::recognize_glyphs(glyphs, binary);
            all_glyphs = std::move(glyphs);
        }

        std::string text;
        float conf_sum = 0.f;
        int conf_n = 0;
        clang_ldl::Glyph* prev = nullptr;
        for (auto& g : all_glyphs) {
            if (prev) {
                const int gap = g.box.x - (prev->box.x + prev->box.w);
                const int min_w = std::min(prev->box.w, g.box.w);
                if (gap > std::max(min_w, 4)) {
                    text.push_back(' ');
                }
            }
            if (g.codepoint != '?' && g.codepoint != 0) {
                append_utf8(text, static_cast<uint32_t>(g.codepoint));
            }
            conf_sum += g.confidence;
            ++conf_n;
            prev = &g;
        }

        const std::string& s = text;
        out->text_len = s.size();
        out->text = static_cast<char*>(std::malloc(s.size() + 1));
        if (!out->text) {
            return CLANG_LDL_ERR_ALLOC;
        }
        std::memcpy(out->text, s.c_str(), s.size() + 1);
        out->glyph_count = conf_n;
        out->mean_confidence = conf_n > 0 ? conf_sum / conf_n : 0.f;

        out->glyph_info_count = static_cast<int>(all_glyphs.size());
        if (!all_glyphs.empty()) {
            out->glyphs = static_cast<ClangLdlGlyphInfo*>(
                std::malloc(sizeof(ClangLdlGlyphInfo) * all_glyphs.size()));
            if (!out->glyphs) {
                std::free(out->text);
                out->text = nullptr;
                return CLANG_LDL_ERR_ALLOC;
            }
            for (size_t i = 0; i < all_glyphs.size(); ++i) {
                fill_glyph_info(out->glyphs[i], all_glyphs[i]);
            }
        }
        return CLANG_LDL_OK;
    } catch (...) {
        return CLANG_LDL_ERR_PROCESS;
    }
}

} // namespace

extern "C" {

const char* clang_ldl_version(void) {
    return "0.7.0";
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
    std::free(result->glyphs);
    result->text = nullptr;
    result->glyphs = nullptr;
    result->text_len = 0;
    result->glyph_count = 0;
    result->glyph_info_count = 0;
    result->mean_confidence = 0.f;
}

} // extern "C"
