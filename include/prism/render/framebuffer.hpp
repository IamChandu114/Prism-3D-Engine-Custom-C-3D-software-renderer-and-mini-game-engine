#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace prism::render {

class Framebuffer {
public:
    Framebuffer(int width, int height);

    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }
    [[nodiscard]] size_t total_pixels() const noexcept {
        return static_cast<size_t>(width_) * height_;
    }

    void clear(uint32_t color, float depth = 1.0f) noexcept;

    inline void set_pixel(int x, int y, uint32_t color) noexcept {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) {
            color_buffer_[static_cast<size_t>(y) * width_ + x] = color;
        }
    }

    [[nodiscard]] inline uint32_t get_pixel(int x, int y) const noexcept {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) {
            return color_buffer_[static_cast<size_t>(y) * width_ + x];
        }
        return 0;
    }

    inline bool set_pixel_depth(int x, int y, float depth, uint32_t color) noexcept {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) {
            size_t idx = static_cast<size_t>(y) * width_ + x;
            if (depth < depth_buffer_[idx]) {
                depth_buffer_[idx] = depth;
                color_buffer_[idx] = color;
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] inline float get_depth(int x, int y) const noexcept {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) {
            return depth_buffer_[static_cast<size_t>(y) * width_ + x];
        }
        return 1.0f;
    }

    void draw_line(int x0, int y0, int x1, int y1, uint32_t color) noexcept;
    void draw_char(int x, int y, char c, uint32_t color, int scale = 1) noexcept;
    void draw_string(int x, int y, const std::string& text, uint32_t color, int scale = 1) noexcept;

    [[nodiscard]] bool save_ppm(const std::string& filepath) const;

    [[nodiscard]] const uint32_t* color_data() const noexcept { return color_buffer_.data(); }
    [[nodiscard]] uint32_t* color_data() noexcept { return color_buffer_.data(); }

    [[nodiscard]] const float* depth_data() const noexcept { return depth_buffer_.data(); }
    [[nodiscard]] float* depth_data() noexcept { return depth_buffer_.data(); }

private:
    int width_{0};
    int height_{0};
    std::vector<uint32_t> color_buffer_;
    std::vector<float> depth_buffer_;
};

} // namespace prism::render
