#include "clang_ldl/pipeline.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <vector>

namespace clang_ldl {

Image preprocess(const Image& input) {
    Image gray = input.channels == 1 ? input : input.to_grayscale();
    Image blurred = gray.gaussian_blur(1);
    Image binary = blurred.otsu_threshold();
    int64_t sum = 0;
    for (uint8_t v : binary.pixels) {
        sum += v;
    }
    const double mean = static_cast<double>(sum) / static_cast<double>(binary.pixels.size());
    // Light backgrounds produce dark text after Otsu; ensure ink = 255 for downstream CV.
    if (mean > 127.0) {
        binary = binary.invert();
    }
    return binary;
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

std::vector<Glyph> segment_glyphs(const Image& line_binary) {
    const int w = line_binary.width;
    const int h = line_binary.height;
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

    std::vector<Glyph> merged;
    merged.push_back(glyphs.front());
    for (size_t i = 1; i < glyphs.size(); ++i) {
        Glyph& prev = merged.back();
        const Glyph& cur = glyphs[i];
        const int gap = cur.box.x - (prev.box.x + prev.box.w);
        const int avg_h = (prev.box.h + cur.box.h) / 2;
        // Merge only touching or overlapping parts of the same glyph (not inter-letter gaps).
        if (gap <= 1 && std::abs(prev.box.y - cur.box.y) <= avg_h / 4) {
            const int nx = prev.box.x;
            const int ny = std::min(prev.box.y, cur.box.y);
            const int nmaxx = std::max(prev.box.x + prev.box.w, cur.box.x + cur.box.w);
            const int nmaxy = std::max(prev.box.y + prev.box.h, cur.box.y + cur.box.h);
            const int nw = nmaxx - nx;
            const int nh = nmaxy - ny;
            std::vector<uint8_t> nb(static_cast<size_t>(nw) * nh, 0);
            auto blit = [&](const Glyph& g) {
                for (int yy = 0; yy < g.box.h; ++yy) {
                    for (int xx = 0; xx < g.box.w; ++xx) {
                        const uint8_t v = g.bitmap[static_cast<size_t>(yy) * g.box.w + xx];
                        if (v > 127) {
                            nb[static_cast<size_t>(g.box.y - ny + yy) * nw + (g.box.x - nx + xx)] = 255;
                        }
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
}

} // namespace clang_ldl
