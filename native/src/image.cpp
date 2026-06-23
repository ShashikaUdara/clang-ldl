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

uint8_t Image::at(int x, int y) const {
    return pixels[static_cast<size_t>(y) * width + x];
}

void Image::set(int x, int y, uint8_t v) {
    pixels[static_cast<size_t>(y) * width + x] = v;
}

} // namespace clang_ldl
