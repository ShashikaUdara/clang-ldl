#include "clang_ldl/pipeline.hpp"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <queue>
#include <vector>

namespace clang_ldl {

namespace {

bool poor_binarization(const Image& binary) {
    int ink = 0;
    for (uint8_t v : binary.pixels) {
        if (v > 127) {
            ++ink;
        }
    }
    const double ratio = static_cast<double>(ink) / static_cast<double>(binary.pixels.size());
    return ratio < 0.001 || ratio > 0.5;
}

Image ensure_ink_foreground(Image binary) {
    int64_t sum = 0;
    for (uint8_t v : binary.pixels) {
        sum += v;
    }
    const double mean = static_cast<double>(sum) / static_cast<double>(binary.pixels.size());
    if (mean > 127.0) {
        binary = binary.invert();
    }
    return binary;
}

} // namespace

Image preprocess(const Image& input) {
    Image gray = input.channels == 1 ? input : input.to_grayscale();
    int extreme = 0;
    for (uint8_t v : gray.pixels) {
        if (v < 20 || v > 235) {
            ++extreme;
        }
    }
    const bool near_binary =
        !gray.pixels.empty()
        && static_cast<double>(extreme) / static_cast<double>(gray.pixels.size()) > 0.92;
    Image blurred = near_binary ? gray : gray.gaussian_blur(1);
    Image deskewed = blurred;
    if (const char* env = std::getenv("CLANG_LDL_DESKEW")) {
        if (env[0] == '1' || env[0] == 'y' || env[0] == 'Y') {
            deskewed = blurred.deskew(15.f);
            if (deskewed.width <= 0 || deskewed.height <= 0) {
                deskewed = blurred;
            }
        }
    }
    Image binary = deskewed.otsu_threshold();
    if (poor_binarization(binary)) {
        binary = deskewed.sauvola_threshold();
    }
    return ensure_ink_foreground(binary);
}

std::vector<Rect> find_text_lines(const Image& binary) {
    std::vector<int> row_sum(binary.height, 0);
    for (int y = 0; y < binary.height; ++y) {
        for (int x = 0; x < binary.width; ++x) {
            if (binary.at(x, y) > 127) {
                row_sum[y]++;
            }
        }
    }
    const int max_sum = *std::max_element(row_sum.begin(), row_sum.end());
    const int threshold = std::max(1, max_sum / 20);

    std::vector<Rect> lines;
    int y = 0;
    while (y < binary.height) {
        while (y < binary.height && row_sum[y] < threshold) {
            ++y;
        }
        if (y >= binary.height) {
            break;
        }
        const int y0 = y;
        while (y < binary.height && row_sum[y] >= threshold) {
            ++y;
        }
        const int y1 = y;
        const int h = y1 - y0;
        if (h < 4) {
            continue;
        }
        int x0 = binary.width, x1 = 0;
        for (int yy = y0; yy < y1; ++yy) {
            for (int x = 0; x < binary.width; ++x) {
                if (binary.at(x, yy) > 127) {
                    x0 = std::min(x0, x);
                    x1 = std::max(x1, x);
                }
            }
        }
        if (x1 > x0) {
            lines.push_back({x0, y0, x1 - x0 + 1, h});
        }
    }
    return lines;
}

std::vector<Rect> merge_adjacent_text_lines(const std::vector<Rect>& lines, int max_gap) {
    if (lines.size() < 2) {
        return lines;
    }
    std::vector<Rect> merged;
    Rect cur = lines.front();
    for (size_t i = 1; i < lines.size(); ++i) {
        const Rect& next = lines[i];
        const int gap = next.y - (cur.y + cur.h);
        if (gap <= max_gap) {
            const int nx = std::min(cur.x, next.x);
            const int ny = cur.y;
            const int nmaxx = std::max(cur.x + cur.w, next.x + next.w);
            const int nmaxy = std::max(cur.y + cur.h, next.y + next.h);
            cur = {nx, ny, nmaxx - nx, nmaxy - ny};
        } else {
            merged.push_back(cur);
            cur = next;
        }
    }
    merged.push_back(cur);
    return merged;
}

std::vector<Rect> split_tall_text_lines(
    const std::vector<Rect>& lines, const Image& binary, int max_h) {
    if (max_h <= 0) {
        return lines;
    }
    std::vector<Rect> out;
    out.reserve(lines.size());
    for (const Rect& r : lines) {
        if (r.h <= max_h) {
            out.push_back(r);
            continue;
        }
        Image crop;
        crop.width = r.w;
        crop.height = r.h;
        crop.channels = 1;
        crop.pixels.resize(static_cast<size_t>(r.w) * r.h);
        for (int y = 0; y < r.h; ++y) {
            for (int x = 0; x < r.w; ++x) {
                crop.set(x, y, binary.at(r.x + x, r.y + y));
            }
        }
        auto sub = find_text_lines(crop);
        if (sub.empty()) {
            out.push_back(r);
            continue;
        }
        for (const Rect& s : sub) {
            out.push_back({r.x + s.x, r.y + s.y, s.w, s.h});
        }
    }
    return out;
}

std::vector<Glyph> segment_glyphs(const Image& line_binary) {
    const int w = line_binary.width;
    const int h = line_binary.height;
    if (w <= 0 || h <= 0) {
        return {};
    }

    // Monospace terminal font: split on all-empty columns (reliable for synthetic L0 corpus).
    std::vector<int> col_sum(w, 0);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (line_binary.at(x, y) > 127) {
                ++col_sum[x];
            }
        }
    }

    struct Span {
        int x0;
        int x1;
    };
    std::vector<Span> spans;
    int x = 0;
    while (x < w) {
        while (x < w && col_sum[x] == 0) {
            ++x;
        }
        if (x >= w) {
            break;
        }
        const int x0 = x;
        while (x < w && col_sum[x] > 0) {
            ++x;
        }
        if (x - x0 >= 2) {
            spans.push_back({x0, x});
        }
    }

    if (!spans.empty()) {
        std::vector<int> widths;
        widths.reserve(spans.size());
        for (const auto& span : spans) {
            widths.push_back(span.x1 - span.x0);
        }
        std::sort(widths.begin(), widths.end());
        const int median_w = widths[widths.size() / 2];
        const int min_span_w = std::max(6, median_w / 2);
        std::vector<Span> filtered;
        filtered.reserve(spans.size());
        for (const auto& span : spans) {
            if (span.x1 - span.x0 >= min_span_w) {
                filtered.push_back(span);
            }
        }
        if (!filtered.empty()) {
            spans = std::move(filtered);
        }
    }

    if (!spans.empty()) {
        int line_miny = h;
        int line_maxy = 0;
        for (int yy = 0; yy < h; ++yy) {
            for (int xx = 0; xx < w; ++xx) {
                if (line_binary.at(xx, yy) > 127) {
                    line_miny = std::min(line_miny, yy);
                    line_maxy = std::max(line_maxy, yy);
                }
            }
        }
        if (line_maxy < line_miny) {
            return {};
        }
        const int line_h = line_maxy - line_miny + 1;

        std::vector<Glyph> projected;
        projected.reserve(spans.size());
        for (const auto& span : spans) {
            const int gw = span.x1 - span.x0;
            Glyph g;
            g.box = {span.x0, line_miny, gw, line_h};
            g.bitmap.resize(static_cast<size_t>(gw) * line_h);
            for (int yy = 0; yy < line_h; ++yy) {
                for (int xx = 0; xx < gw; ++xx) {
                    g.bitmap[static_cast<size_t>(yy) * gw + xx] =
                        line_binary.at(span.x0 + xx, line_miny + yy) > 127 ? 255 : 0;
                }
            }
            projected.push_back(std::move(g));
        }
        if (!projected.empty()) {
            const bool lone_oversized =
                h > 60
                && projected.size() == 1
                && projected.front().box.w > w * 6 / 10;
            if (!lone_oversized) {
                return projected;
            }
        }
    }

    // Fallback: connected components + merge (natural images / non-monospace).
    std::vector<int> labels(static_cast<size_t>(w) * h, -1);
    int next_label = 0;
    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, 1, -1};

    auto idx = [w](int x, int y) { return static_cast<size_t>(y) * w + x; };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (line_binary.at(x, y) <= 127 || labels[idx(x, y)] >= 0) {
                continue;
            }
            int minx = x, maxx = x, miny = y, maxy = y;
            std::queue<std::pair<int, int>> q;
            q.push({x, y});
            labels[idx(x, y)] = next_label;
            while (!q.empty()) {
                auto [cx, cy] = q.front();
                q.pop();
                minx = std::min(minx, cx);
                maxx = std::max(maxx, cx);
                miny = std::min(miny, cy);
                maxy = std::max(maxy, cy);
                for (int k = 0; k < 4; ++k) {
                    const int nx = cx + dx[k];
                    const int ny = cy + dy[k];
                    if (nx < 0 || ny < 0 || nx >= w || ny >= h) {
                        continue;
                    }
                    if (line_binary.at(nx, ny) <= 127 || labels[idx(nx, ny)] >= 0) {
                        continue;
                    }
                    labels[idx(nx, ny)] = next_label;
                    q.push({nx, ny});
                }
            }
            ++next_label;
        }
    }

    struct Comp {
        int label;
        int minx, miny, maxx, maxy, count;
    };
    std::vector<Comp> comps(next_label, {-1, w, h, 0, 0, 0});
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int lab = labels[idx(x, y)];
            if (lab < 0) {
                continue;
            }
            auto& c = comps[lab];
            c.label = lab;
            c.minx = std::min(c.minx, x);
            c.maxx = std::max(c.maxx, x);
            c.miny = std::min(c.miny, y);
            c.maxy = std::max(c.maxy, y);
            c.count++;
        }
    }

    const int min_pixels = std::max(4, (h * h) / 200);
    const int max_pixels = w * h;
    std::vector<Glyph> glyphs;
    for (const auto& c : comps) {
        if (c.label < 0 || c.count < min_pixels || c.count > max_pixels) {
            continue;
        }
        const int gw = c.maxx - c.minx + 1;
        const int gh = c.maxy - c.miny + 1;
        if (gw < 2 || gh < 4 || gw > w * 9 / 10) {
            continue;
        }
        Glyph g;
        g.box = {c.minx, c.miny, gw, gh};
        g.bitmap.resize(static_cast<size_t>(gw) * gh);
        for (int yy = 0; yy < gh; ++yy) {
            for (int xx = 0; xx < gw; ++xx) {
                g.bitmap[static_cast<size_t>(yy) * gw + xx] =
                    line_binary.at(c.minx + xx, c.miny + yy) > 127 ? 255 : 0;
            }
        }
        glyphs.push_back(std::move(g));
    }

    std::sort(glyphs.begin(), glyphs.end(), [](const Glyph& a, const Glyph& b) {
        if (a.box.y != b.box.y) {
            return a.box.y < b.box.y;
        }
        return a.box.x < b.box.x;
    });

    if (glyphs.size() < 2) {
        return glyphs;
    }

    auto merge_pass = [](const std::vector<Glyph>& glyphs) {
        std::vector<Glyph> merged;
        merged.push_back(glyphs.front());
        for (size_t i = 1; i < glyphs.size(); ++i) {
            Glyph& prev = merged.back();
            const Glyph& cur = glyphs[i];
            const int gap = cur.box.x - (prev.box.x + prev.box.w);
            const int avg_h = (prev.box.h + cur.box.h) / 2;
            const int avg_w = (prev.box.w + cur.box.w) / 2;
            const int x_overlap = std::min(prev.box.x + prev.box.w, cur.box.x + cur.box.w)
                - std::max(prev.box.x, cur.box.x);
            const bool horizontal_touch = gap <= std::max(2, avg_w / 8);
            const bool vertical_stack = x_overlap > std::max(1, avg_w / 3)
                && std::abs((prev.box.y + prev.box.h) - cur.box.y) <= std::max(2, avg_h / 4);
            const bool same_row = std::abs(prev.box.y - cur.box.y) <= avg_h / 2;
            if ((horizontal_touch && same_row) || vertical_stack) {
                const int nx = std::min(prev.box.x, cur.box.x);
                const int ny = std::min(prev.box.y, cur.box.y);
                const int nmaxx = std::max(prev.box.x + prev.box.w, cur.box.x + cur.box.w);
                const int nmaxy = std::max(prev.box.y + prev.box.h, cur.box.y + cur.box.h);
                const int nw = nmaxx - nx;
                const int nh = nmaxy - ny;
                if (nw <= 0 || nh <= 0) {
                    merged.push_back(cur);
                    continue;
                }
                std::vector<uint8_t> nb(static_cast<size_t>(nw) * nh, 0);
                auto blit = [&](const Glyph& g) {
                    for (int yy = 0; yy < g.box.h; ++yy) {
                        for (int xx = 0; xx < g.box.w; ++xx) {
                            const uint8_t v = g.bitmap[static_cast<size_t>(yy) * g.box.w + xx];
                            if (v <= 127) {
                                continue;
                            }
                            const int dst_x = g.box.x - nx + xx;
                            const int dst_y = g.box.y - ny + yy;
                            if (dst_x < 0 || dst_y < 0 || dst_x >= nw || dst_y >= nh) {
                                continue;
                            }
                            nb[static_cast<size_t>(dst_y) * nw + dst_x] = 255;
                        }
                    }
                };
                blit(prev);
                blit(cur);
                prev.box = {nx, ny, nw, nh};
                prev.bitmap = std::move(nb);
            } else {
                merged.push_back(cur);
            }
        }
        return merged;
    };

    std::vector<Glyph> merged = glyphs;
    for (int pass = 0; pass < 4; ++pass) {
        const auto next = merge_pass(merged);
        if (next.size() == merged.size()) {
            break;
        }
        merged = std::move(next);
    }
    return merged;
}

} // namespace clang_ldl