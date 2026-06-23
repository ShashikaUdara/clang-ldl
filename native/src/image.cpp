#define _USE_MATH_DEFINES
#include "clang_ldl/image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace clang_ldl {

namespace {

uint8_t luminance(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint8_t>(0.299f * r + 0.587f * g + 0.114f * b);
}

} // namespace

Image Image::load_file(const std::string& path) {
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".ppm") {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            throw std::runtime_error("failed to open PPM: " + path);
        }
        std::string magic;
        in >> magic;
        if (magic != "P6" && magic != "P5") {
            throw std::runtime_error("unsupported PPM magic: " + magic);
        }
        int w = 0, h = 0, maxval = 0;
        char c = 0;
        while (in.get(c) && (c == '#' || c == '\n' || c == '\r' || c == ' ' || c == '\t')) {
            if (c == '#') {
                while (in.get(c) && c != '\n') {
                }
            }
        }
        in.putback(c);
        in >> w >> h >> maxval;
        in.get();
        Image img;
        img.width = w;
        img.height = h;
        img.channels = (magic == "P5") ? 1 : 3;
        const size_t n = static_cast<size_t>(w) * h * img.channels;
        img.pixels.resize(n);
        in.read(reinterpret_cast<char*>(img.pixels.data()), static_cast<std::streamsize>(n));
        if (!in) {
            throw std::runtime_error("truncated PPM: " + path);
        }
        if (img.channels == 3) {
            return img.to_grayscale();
        }
        return img;
    }

    int w = 0, h = 0, comp = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &comp, 0);
    if (!data) {
        throw std::runtime_error(std::string("stbi_load failed: ") + stbi_failure_reason());
    }
    Image img;
    img.width = w;
    img.height = h;
    img.channels = comp;
    const size_t n = static_cast<size_t>(w) * h * comp;
    img.pixels.assign(data, data + n);
    stbi_image_free(data);
    if (comp == 1) {
        return img;
    }
    return img.to_grayscale();
}

Image Image::from_bytes(const uint8_t* data, int w, int h, int channels) {
    Image img;
    img.width = w;
    img.height = h;
    img.channels = channels;
    const size_t n = static_cast<size_t>(w) * h * channels;
    img.pixels.assign(data, data + n);
    if (channels == 1) {
        return img;
    }
    return img.to_grayscale();
}

Image Image::to_grayscale() const {
    if (channels == 1) {
        return *this;
    }
    Image out;
    out.width = width;
    out.height = height;
    out.channels = 1;
    out.pixels.resize(static_cast<size_t>(width) * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const size_t i = (static_cast<size_t>(y) * width + x) * channels;
            out.pixels[static_cast<size_t>(y) * width + x] =
                luminance(pixels[i], pixels[i + 1], pixels[i + 2]);
        }
    }
    return out;
}

Image Image::gaussian_blur(int radius) const {
    if (radius <= 0) {
        return *this;
    }
    const int k = radius * 2 + 1;
    std::vector<int> kernel(k);
    int sum = 0;
    for (int i = -radius; i <= radius; ++i) {
        const int v = radius + 1 - std::abs(i);
        kernel[i + radius] = v;
        sum += v;
    }
    Image tmp = *this;
    Image out = *this;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int acc = 0;
            for (int dx = -radius; dx <= radius; ++dx) {
                const int sx = std::clamp(x + dx, 0, width - 1);
                acc += at(sx, y) * kernel[dx + radius];
            }
            tmp.set(x, y, static_cast<uint8_t>(acc / sum));
        }
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int acc = 0;
            for (int dy = -radius; dy <= radius; ++dy) {
                const int sy = std::clamp(y + dy, 0, height - 1);
                acc += tmp.at(x, sy) * kernel[dy + radius];
            }
            out.set(x, y, static_cast<uint8_t>(acc / sum));
        }
    }
    return out;
}

Image Image::otsu_threshold() const {
    std::array<int, 256> hist{};
    for (uint8_t v : pixels) {
        hist[v]++;
    }
    const int total = width * height;
    int sum = 0;
    for (int i = 0; i < 256; ++i) {
        sum += i * hist[i];
    }
    int sumB = 0, wB = 0, wF = 0, maxVar = 0, threshold = 128;
    for (int t = 0; t < 256; ++t) {
        wB += hist[t];
        if (wB == 0) {
            continue;
        }
        wF = total - wB;
        if (wF == 0) {
            break;
        }
        sumB += t * hist[t];
        const double mB = static_cast<double>(sumB) / wB;
        const double mF = static_cast<double>(sum - sumB) / wF;
        const double varBetween = static_cast<double>(wB) * wF * (mB - mF) * (mB - mF);
        if (varBetween > maxVar) {
            maxVar = static_cast<int>(varBetween);
            threshold = t;
        }
    }
    Image out;
    out.width = width;
    out.height = height;
    out.channels = 1;
    out.pixels.resize(pixels.size());
    for (size_t i = 0; i < pixels.size(); ++i) {
        out.pixels[i] = pixels[i] >= threshold ? 255 : 0;
    }
    return out;
}

Image Image::invert() const {
    Image out = *this;
    for (auto& v : out.pixels) {
        v = 255 - v;
    }
    return out;
}

Image Image::rotate(float radians) const {
    if (std::fabs(radians) < 1e-4f) {
        return *this;
    }
    const float cos_a = std::cos(radians);
    const float sin_a = std::sin(radians);
    const float cx = (width - 1) * 0.5f;
    const float cy = (height - 1) * 0.5f;
    const int new_w = width;
    const int new_h = height;
    Image out;
    out.width = new_w;
    out.height = new_h;
    out.channels = 1;
    out.pixels.assign(static_cast<size_t>(new_w) * new_h, 255);
    for (int y = 0; y < new_h; ++y) {
        for (int x = 0; x < new_w; ++x) {
            const float dx = x - cx;
            const float dy = y - cy;
            const float sx = cos_a * dx + sin_a * dy + cx;
            const float sy = -sin_a * dx + cos_a * dy + cy;
            const int ix = static_cast<int>(std::round(sx));
            const int iy = static_cast<int>(std::round(sy));
            if (ix >= 0 && iy >= 0 && ix < width && iy < height) {
                out.set(x, y, at(ix, iy));
            }
        }
    }
    return out;
}

Image Image::deskew(float max_degrees) const {
    if (width < 8 || height < 8) {
        return *this;
    }
    int count = 0;
    double cx = 0, cy = 0, mu11 = 0, mu20 = 0, mu02 = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (at(x, y) < 200) {
                cx += x;
                cy += y;
                ++count;
            }
        }
    }
    if (count < 32) {
        return *this;
    }
    cx /= count;
    cy /= count;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (at(x, y) < 200) {
                const double dx = x - cx;
                const double dy = y - cy;
                mu11 += dx * dy;
                mu20 += dx * dx;
                mu02 += dy * dy;
            }
        }
    }
    const double denom = mu20 - mu02;
    if (std::abs(denom) < 1e-6 && std::abs(mu11) < 1e-6) {
        return *this;
    }
    const double angle_rad = 0.5 * std::atan2(2.0 * mu11, denom);
    if (!std::isfinite(angle_rad)) {
        return *this;
    }
    const float max_rad = max_degrees * 3.14159265f / 180.f;
    float clamped = static_cast<float>(angle_rad);
    if (clamped > max_rad) {
        clamped = max_rad;
    }
    if (clamped < -max_rad) {
        clamped = -max_rad;
    }
    if (std::fabs(clamped) < 0.02f) {
        return *this;
    }
    return rotate(-clamped);
}

Image Image::sauvola_threshold(int window, float k, float R) const {
    Image out;
    out.width = width;
    out.height = height;
    out.channels = 1;
    out.pixels.resize(pixels.size());
    const int half = std::max(1, window / 2);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int count = 0;
            double sum = 0;
            double sum_sq = 0;
            for (int dy = -half; dy <= half; ++dy) {
                for (int dx = -half; dx <= half; ++dx) {
                    const int sx = std::clamp(x + dx, 0, width - 1);
                    const int sy = std::clamp(y + dy, 0, height - 1);
                    const double v = at(sx, sy);
                    sum += v;
                    sum_sq += v * v;
                    ++count;
                }
            }
            const double mean = sum / count;
            const double variance = std::max(0.0, sum_sq / count - mean * mean);
            const double stddev = std::sqrt(variance);
            const double thresh = mean * (1.0 + k * ((stddev / R) - 1.0));
            out.pixels[static_cast<size_t>(y) * width + x] =
                at(x, y) >= static_cast<uint8_t>(std::clamp(thresh, 0.0, 255.0)) ? 255 : 0;
        }
    }
    return out;
}

uint8_t Image::at(int x, int y) const {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return 255;
    }
    return pixels[static_cast<size_t>(y) * width + x];
}

void Image::set(int x, int y, uint8_t v) {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return;
    }
    pixels[static_cast<size_t>(y) * width + x] = v;
}

} // namespace clang_ldl
